# 验证记录

日期：2026-09-10。源码版本：0.1.0-dev。以下是本地执行结果，不是 GitHub Actions 的运行记录。

## 环境

- Windows x64，MSVC 19.51.36246，CMake 3.26.4，Ninja，Release。
- CPU 验证使用微软官方 `onnxruntime-win-x64-1.20.1.zip`。
- SDK SHA-256：`78d447051e48bd2e1e778bba378bec4ece11191c9e538cf7b2c4a4565e8f5581`。
- 官方包记录的源提交：`5c1b7ccbff7e5141c1da7a9d963d660e5741c319`。
- Python 标准库生成合成 ONNX 图；没有下载真人声音模型。

## 已通过

1. 独立 CMake 构建 `dvc.dll`、WAV CLI、C ABI 测试和 C++ 模型协议测试。
2. `api`：从纯 C 编译，校验 ABI 版本、配置边界、未加载模型、输出容量、错误信息和空销毁。
3. `model_contract`：运行完整 content/F0/voice ONNX 链路，校验非零有限输出、pitch 参数进入图、原地转换、错误模型拒绝、加载失败保留旧模型、speaker 边界、NaN 拒绝、分块长度、连续块、重置和多实例隔离。
4. WAV CLI：人工 PCM16 输入到 PCM16 输出，检查采样率/样本数及非零音频；拒绝覆盖输出和截断 WAV。
5. `cmake --install` 后，用独立 CMake 消费者 `find_package(dvc CONFIG REQUIRED)` 编译链接并运行，打印 `0.1.0-dev`。
6. DirectML 分支编译链接通过；这是编译检查，未执行 GPU 推理。
7. CPU DLL 导入表包含 ONNX Runtime、MSVC/UCRT 和 Windows 系统运行时。
8. 源码包解压到独立目录后，使用官方 SDK 从零构建、运行两组 CTest 和 WAV CLI 测试、安装库与许可证均通过。包内不含 SDK、模型或构建产物。

## 尚未验收

- 真人音色、实际导出器、音高/音色质量、不同采样率的质量回归。
- 麦克风/虚拟声卡端到端延迟、长时间运行、显存压力和流式尾部时间轴。
- DirectML 的真实 GPU 运行。单独的 DirectML 编译检查使用本地 API 19 SDK，只验证分支能编译，不用于发布依赖。
- Linux/macOS、CUDA、二进制发行包和 GitHub 托管 CI 实际运行。

合成模型用于发现 API/ONNX/状态错误：内容和 F0 为常量，voice 图以 F0 均值调制人工正弦波幅度，以验证参数传递。它们不执行真实的声音转换，不能作为音质、推理速度或生产可用性的证据。
