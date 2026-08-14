# 钢珠 YOLO11n-Pose 训练项目

这个文件夹是单独整理出的 YOLO 训练仓库，用于训练 MaixCAM/桌面端可用的钢珠中心点检测模型。

模型任务：

- 类别：`gangzhu`
- 关键点：1 个，表示钢珠几何中心
- 输入尺寸：`320x320`
- 模型：YOLO11n-Pose
- 推荐部署置信度：`0.40`

## 目录结构

```text
.
├── datasets/
│   ├── gangzhu_pose_v2/              # 最终 YOLO pose 数据集，可直接训练
│   ├── positive_source_voc/           # 实际正样本源，Pascal VOC 格式
│   ├── source_voc_review_20260730/   # 原始/复核 Pascal VOC 数据
│   └── negative_samples_185508/      # 空凹槽负样本
├── docs/                             # 数据集报告、评估结果、训练总结
├── models/
│   └── yolo11n-pose.pt               # 预训练起点
├── maixcam_conversion/               # ONNX 转 MaixCAM cvimodel/mud 的资料
├── results/
│   ├── weights/                      # best.pt、last.pt、best.onnx
│   └── *.png / *.csv                 # 训练曲线和可视化结果
└── scripts/                          # 数据准备、训练、评估、导出脚本
```

## 数据集

最终训练数据在 `datasets/gangzhu_pose_v2`，已经是 Ultralytics YOLO pose 格式：

| 集合 | 图片 | 标签 | 说明 |
| --- | ---: | ---: | --- |
| train | 610 | 610 | 含正样本、负样本和增强图 |
| val | 68 | 68 | 含正样本和负样本 |
| test | 60 | 60 | 连续时间段留出的正样本 |

数据集配置文件：

```text
datasets/gangzhu_pose_v2/dataset.yaml
```

## 安装环境

PowerShell：

```powershell
cd scripts
.\install_env.ps1
```

如果需要指定 Python：

```powershell
.\install_env.ps1 -BasePython "C:\Path\To\python.exe"
```

也可以手动安装：

```powershell
python -m venv scripts\.venv
.\scripts\.venv\Scripts\python.exe -m pip install --upgrade pip
.\scripts\.venv\Scripts\python.exe -m pip install torch==2.5.1 torchvision==0.20.1 --index-url https://download.pytorch.org/whl/cu121
.\scripts\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

## 训练

快速冒烟测试：

```powershell
cd scripts
.\run_train.ps1 --smoke --device cpu
```

正式训练：

```powershell
cd scripts
.\run_train.ps1 --device 0
```

默认训练参数在 `scripts/train_pose_v2.py` 中，核心设置为：

- `imgsz=320`
- `epochs=120`
- `batch=32`
- `optimizer=AdamW`
- `seed=20260730`
- `patience=25`

训练输出默认写入：

```text
results/runs/pose/gangzhu_yolo11n_pose_320_v2
```

## 评估

PyTorch 权重评估：

```powershell
.\.venv\Scripts\python.exe .\evaluate_pose_v2.py `
  --model ..\results\weights\best.pt `
  --dataset ..\datasets\gangzhu_pose_v2 `
  --output ..\docs\eval_latest.json `
  --conf 0.40
```

已有评估结果保存在 `docs/`：

- `v2_evaluation_conf040.json`
- `v2_onnx_evaluation_conf040.json`
- `v2_light_robustness.json`
- `TRAINING_SUMMARY.md`

关键结果：

- test 召回率：`96.7%`，58/60
- test 中心点平均误差：`1.98 px`
- test 中心点 P95 误差：`4.73 px`
- 负样本在 `conf=0.40` 下误检数：`0`

## 导出 ONNX

```powershell
cd scripts
.\install_export_env.ps1
.\run_export.ps1
```

默认导出 `results/weights/best.pt`，固定输入为 `1x3x320x320`。后续转换 MaixCAM 专属格式见：

```text
maixcam_conversion/README.md
```

仓库内已经整理了转换脚本和已生成的部署文件：

```text
maixcam_conversion/deploy/gangzhu_yolo11n_pose_320.mud
maixcam_conversion/deploy/gangzhu_yolo11n_pose_320_int8.cvimodel
```

## 坐标约定

模型预测钢珠中心 `(ball_x, ball_y)`。对于 `320x320` 图像：

```python
error_x = ball_x - 160.0
error_y = 160.0 - ball_y
```

屏幕中心为 `(0, 0)`，向右为正 X，向上为正 Y。
