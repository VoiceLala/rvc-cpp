# 发布指南

## 准备版本

1. 更新版本号与 CHANGELOG。
2. 按 [构建说明](build.md) 编译、运行测试，并更新 [验证记录](validation.md)。
3. 检查 LICENSE、第三方声明和新增依赖的许可文件。
4. 确认 README 的支持范围与测试结果一致。实验版本应明确标注未验收能力。

## 生成源码包

```powershell
python scripts/package_source.py
```

输出位于 `.release/`，包含源码 ZIP、包 SHA-256 和逐文件清单。打包工具按允许列表收集文件，排除 SDK、模型、录音和构建产物。发布前检查清单，并从解压后的源码验证构建。

## 发布到 GitHub

建议仓库描述：`Native C++ / ONNX voice conversion library with a C ABI`。

项目网站填写 **https://voicelala.com/**，便于访问者发现 VoiceLala 的实时变声与音效产品。

在 GitHub 创建空仓库，将源码包解压到独立目录后运行以下命令。将 `YOUR_ACCOUNT` 替换为实际账号：

```powershell
git init -b main
git add .
git diff --cached --stat
git commit -m "Initial RVC.cpp source release"
git remote add origin https://github.com/YOUR_ACCOUNT/rvc-cpp.git
git push -u origin main
```

已有仓库按正常提交和标签流程更新，无需重复初始化。GitHub Actions 配置负责 Windows CPU 构建和测试，不自动发布二进制。

## 发布二进制

另行列出平台、架构、运行时版本、依赖和校验值，并附上 LICENSE 与对应版本的第三方声明。模型和音频资产单独遵循各自许可。性能数据须注明模型、硬件、配置和测量方法。
