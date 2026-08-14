# ONNX 转 MaixCAM 专属格式

这里整理的是 `results/weights/best.onnx` 转 MaixCAM / MaixCAM Pro 可加载模型的资料。

MaixCAM 使用的部署文件是一组：

```text
deploy/gangzhu_yolo11n_pose_320.mud
deploy/gangzhu_yolo11n_pose_320_int8.cvimodel
```

`.cvimodel` 是 CV181x NPU 可运行的 INT8 模型，`.mud` 是 MaixPy 读取的模型描述文件，两者必须放在同一个目录。

## 已生成的部署文件

当前目录已经包含转换完成的文件：

```text
deploy/gangzhu_yolo11n_pose_320.mud
deploy/gangzhu_yolo11n_pose_320_int8.cvimodel
```

对应 `.mud` 内容为：

```ini
[basic]
type = cvimodel
model = gangzhu_yolo11n_pose_320_int8.cvimodel

[extra]
model_type = yolo11
type = pose
input_type = rgb
mean = 0, 0, 0
scale = 0.00392156862745098,0.00392156862745098,0.00392156862745098
labels = gangzhu
```

部署到 MaixCAM 时复制这两个文件到同一目录，例如：

```text
/root/models/
```

MaixPy 代码中加载 `.mud` 文件：

```python
from maix import nn

detector = nn.YOLO11(model="/root/models/gangzhu_yolo11n_pose_320.mud")
```

## 转换环境

转换使用 TPU-MLIR，历史转换记录见 `CONVERSION_REPORT.md`：

- TPU-MLIR：`1.28.1-20260429`
- Processor：`cv181x`
- Quantization：INT8
- 输入：`1x3x320x320`
- 输出：YOLO11 pose head 的 3 个中间输出

WSL2/TPU-MLIR 配置说明见 `WSL2_ENVIRONMENT.md`。默认脚本假设 TPU-MLIR 工具位于 `$HOME/venvs/tpu-mlir/bin`，也可以通过参数指定其他路径。

## 重建步骤

先确保 `results/weights/best.onnx` 是最新模型，然后在 Windows 侧准备转换目录：

```powershell
python maixcam_conversion\prepare_conversion_data.py
```

这个脚本会生成：

```text
maixcam_conversion/model.onnx
maixcam_conversion/test.jpg
maixcam_conversion/images/
maixcam_conversion/calibration_manifest.csv
```

再进入装有 TPU-MLIR 的 Linux/WSL 环境运行：

```bash
cd /mnt/<drive>/<path-to-repo>/maixcam_conversion
export TPU_MLIR_BIN="$HOME/venvs/tpu-mlir/bin"
bash convert.sh
```

也可以直接在 Windows PowerShell 中调用：

```powershell
cd maixcam_conversion
.\run_convert_wsl.ps1
```

`convert.sh` 的核心流程：

1. `extract_onnx.py` 从 Ultralytics 导出的 ONNX 中抽取 YOLO11 pose head 需要的 3 个输出。
2. `model_transform.py` 将 `export.onnx` 转为 MLIR。
3. `run_calibration.py` 使用 `images/` 中的 200 张真实图做 INT8 校准。
4. `model_deploy.py` 生成 `gangzhu_yolo11n_pose_320_int8.cvimodel`。

转换成功后，把新的 `.cvimodel` 和 `.mud` 放入 `deploy/`。

## 关键输出节点

`extract_onnx.py` 固定抽取以下输出：

```text
/model.23/dfl/conv/Conv_output_0
/model.23/Sigmoid_output_0
/model.23/Concat_output_0
```

历史转换报告中的输出形状：

```text
/model.23/dfl/conv/Conv_output_0  [1,1,4,2100]
/model.23/Sigmoid_output_0        [1,1,2100]
/model.23/Concat_output_0         [1,3,2100]
```

如果将来升级 Ultralytics 或模型结构，节点名可能变化，需要先用 Netron 或 ONNX 工具确认输出节点。

## 附带参考

`reference_examples/` 里放了 MaixCAM `.mud` 示例，用来对照字段：

- `model_280927.mud`：YOLOv5 示例
- `model_300366.mud`：YOLOv5 钢珠示例

它们不是当前 YOLO11 pose 模型，只用于参考 `.mud` 文件结构。
