# API 与音频行为

公共 ABI 见 [`include/dvc/dvc.h`](../include/dvc/dvc.h)。使用 `dvc_default_*` 初始化结构体，再调整字段。0.1 系列 ABI 尚未冻结，升级时应重新编译宿主集成代码。当前不提供 HTTP、WebSocket 或 gRPC 网络接口。

生命周期：`dvc_create → dvc_load_model → dvc_convert / dvc_process → dvc_destroy`。

- `dvc_convert`：独立离线片段，每次从配置 seed 建立随机状态，不读写流式历史。输出样本数等于输入样本数；某些模型导出器少出的末尾帧会补零。
- `dvc_process`：每次恰好 `block_size`，历史窗口大小为 `context_samples + block_size + crossfade_samples + search_samples`。追加输入、执行整窗推理、寻找衔接位置、交叉淡化并返回一个块。
- 流式历史以零初始化；额外输出时间偏移名义上为 `crossfade_samples + search_samples`，SOLA 匹配可在 `search_samples` 范围内变化。还需计入宿主块采集、设备缓冲和实际推理耗时，不能把它当作端到端延迟测量。
- 无独立 flush API。流结束后可送零块取回尾部，宿主按自己的时间轴保留/裁剪；新流调用 `dvc_reset`。上下文是过去的历史，不应简单按整个 context 长度裁掉输出。
- `dvc_reset` 保留模型/参数，清空历史并复位随机状态。设置 seed 会更新随机数；更换模型成功会重置流状态。
- 加载失败保留旧模型；处理失败返回 `written=0`、不改输出或流式历史。不把原声作为静默的失败回退。
- 同一实例禁止同时加载、处理、改参或销毁；宿主负责串行化。不同实例可并行，但拥有独立模型拷贝，内存消耗相应增加。
- `dvc_last_error` 是线程本地缓冲，应该在失败后立即在同一线程读取并复制。不能作为跨线程持久字符串。
- PCM 为单声道、有限的 float32 数值，通常归一化到 `[-1,1]`。库不做自动峰值限制；WAV 示例在量化为 PCM16 时裁剪。
- C ABI 捕获 C++/ORT 异常并返回错误码，但调用者仍必须提供真实有效、容量足够的指针。不能传悬空指针或在销毁后调用。

本实现同步推理并分配临时内存，没有硬实时保证。设备回调只负责缓冲读写，推理放到工作线程；回调欠载策略由宿主定义。
