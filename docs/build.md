# 构建与验证

## CPU 基线

- CMake >= 3.24；C++17 编译器；Windows x64 SDK 与 MSVC。
- ONNX Runtime C/C++ SDK。选定的可复现基线为 [官方 1.20.1](https://github.com/microsoft/onnxruntime/releases/tag/v1.20.1)，不是“最新版本”声明；正式发布前仍需检查依赖更新和适用安全公告。
- 使用同一发行包的头文件、导入库和运行时，避免混搭。
- Python >= 3.9 仅用于测试夹具/源码打包，无第三方 Python 包依赖。

下载 `onnxruntime-win-x64-1.20.1.zip`，解压到 `.local/` 或外部 SDK 目录。`.local` 不进入源码包。

```powershell
python tests/make_fixtures.py build/fixtures
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_ROOT=C:/sdk/onnxruntime-win-x64-1.20.1 -DDVC_TEST_MODELS="$PWD/build/fixtures"
cmake --build build
Copy-Item C:/sdk/onnxruntime-win-x64-1.20.1/lib/onnxruntime.dll build/
ctest --test-dir build --output-on-failure
cmake --install build --prefix install
```

`api` 从纯 C 编译，验证 ABI 和失败路径。`model_contract` 运行三个由脚本生成的 ONNX 图，验证接口协议、状态重置和实例隔离。这些图输出人工正弦波/常量，不是变声模型或质量基准。

若使用 Visual Studio 多配置生成器，构建/安装加 `--config Release`，ctest 加 `-C Release`，运行时 DLL 放到 `build/Release/`。

自定义 SDK 布局可以显式指定 `ONNXRUNTIME_INCLUDE_DIR` 和 `ONNXRUNTIME_LIBRARY`。

## CMake 消费者

安装 DVC 后：

```cmake
find_package(dvc CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE dvc::dvc)
```

配置消费者时用 `-DCMAKE_PREFIX_PATH=.../install`。DVC 安装包含库和公共头文件，不自动打包 ONNX Runtime 或模型。运行程序时仍须让操作系统找到 DVC 和 ONNX Runtime 动态库。

## DirectML（实验性）

使用带 DirectML EP 的 ONNX Runtime SDK，确保有 `dml_provider_factory.h`，并按 [官方构建说明](https://onnxruntime.ai/docs/build/eps.html#directml) 准备运行时及它要求的 DirectML DLL。

配置加 `-DDVC_ENABLE_DIRECTML=ON`，调用方设置 `config.provider = DVC_DIRECTML` 和 `config.device_id`。CPU SDK 不能仅靠开关变成 DirectML SDK。不支持的 provider 会明确返回错误，不自动回退 CPU。

DirectML 路径设置顺序执行和禁用 memory pattern。GPU 性能、不同显卡和真实模型运行尚待验收，不能从 CPU 合成测试推导。

## 无模型功能演示

可以用合成夹具跑通 CLI；必须告诉查看者这是测试声音，不是变声效果。测试资源仅在 `build/fixtures` 生成，发布包不会包含 ONNX 权重。

## 发布包

```powershell
python scripts/package_source.py
```

默认在 `.release/` 生成 `dvc-0.1.0-dev-source.zip`、包校验值和逐文件 SHA-256 清单，按允许列表收集源码和文档，排除 SDK、构建日志和本地模型。发布步骤见 [发布指南](releasing.md)。
