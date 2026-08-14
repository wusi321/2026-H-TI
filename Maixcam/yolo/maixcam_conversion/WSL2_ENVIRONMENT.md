# WSL2 / TPU-MLIR 配置说明

MaixCAM / MaixCAM Pro 的 `.cvimodel` 通常需要在 Linux 环境中用 TPU-MLIR 生成。Windows 用户推荐使用 WSL2 Ubuntu。

## 环境要求

- Windows 10/11 + WSL2
- Ubuntu 20.04/22.04 或兼容发行版
- TPU-MLIR，可执行工具包含：
  - `model_transform.py`
  - `run_calibration.py`
  - `model_deploy.py`
  - `model_tool.py`

默认脚本假设 TPU-MLIR 安装在：

```text
$HOME/venvs/tpu-mlir/bin
```

如果你的安装位置不同，可以通过环境变量或脚本参数指定。

## 检查工具链

在 WSL2 中运行：

```bash
export TPU_MLIR_BIN="$HOME/venvs/tpu-mlir/bin"
"$TPU_MLIR_BIN/model_transform.py" --version
"$TPU_MLIR_BIN/python" -c "import onnx; print(onnx.__version__)"
```

能看到 TPU-MLIR 版本和 ONNX 版本就说明基础环境可用。

## 从 Windows 调用

PowerShell：

```powershell
cd maixcam_conversion
.\run_convert_wsl.ps1
```

指定发行版或 TPU-MLIR 路径：

```powershell
.\run_convert_wsl.ps1 -Distro Ubuntu -TpuMlirBin "/path/to/tpu-mlir/bin"
```

## 从 WSL2 调用

进入仓库中的转换目录：

```bash
cd /mnt/<drive>/<path-to-repo>/maixcam_conversion
export TPU_MLIR_BIN="$HOME/venvs/tpu-mlir/bin"
bash convert.sh
```

其中 `/mnt/<drive>/<path-to-repo>` 替换为你的仓库在 WSL2 中的实际路径。
