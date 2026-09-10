# 设计与技术参考

DVC 提供原生 C API，将内容编码、F0 提取和 RVC 风格声音合成组合为音频转换管线。

## 组件职责

| 组件 | 职责 |
|---|---|
| C API | 实例生命周期、配置、模型加载、音频输入输出和错误码 |
| ONNX Runtime | 执行 content、pitch 和 voice 模型 |
| 音频处理 | 重采样、F0 对齐、升降调和合成参数准备 |
| 流式处理 | 历史上下文、SOLA 对齐和交叉淡化 |
| 宿主应用 | 音频设备、线程调度、队列和可选网络服务 |

每个实例独立持有模型会话和随机状态。三个模型路径由调用方传入；具体张量名称、维度和采样率约定见 [模型协议](models.md)。

C ABI 的参数校验与异常转换集中在库边界。流式调用按固定块处理，错误时保留输出缓冲和已有流状态。生命周期及线程约定见 [API](api.md)。

## 技术参考

- [RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI)：RVC 声音转换项目。
- [ONNX Runtime](https://github.com/microsoft/onnxruntime)：模型执行引擎。
- [ONNX](https://github.com/onnx/onnx)：模型和张量协议；测试图使用其公开格式。

相关版权及许可文本见 [第三方声明](../THIRD_PARTY_NOTICES.md)。
