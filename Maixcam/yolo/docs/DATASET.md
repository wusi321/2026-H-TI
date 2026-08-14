# 数据集说明

## 最终数据集

`datasets/gangzhu_pose_v2` 是可以直接用于 YOLO pose 训练的数据集。

- 格式：Ultralytics YOLO pose
- 类别数：1
- 类别名：`gangzhu`
- 关键点数：1
- 关键点含义：钢珠几何中心
- 标签格式：`class cx cy w h kpt_x kpt_y visible`

其中 `cx cy w h kpt_x kpt_y` 均为归一化坐标，`visible=2` 表示关键点可见。负样本标签文件为空。

## 数据来源

- 正样本来源：`datasets/positive_source_voc`
- 负样本来源：`datasets/negative_samples_185508`
- 最终 YOLO 数据集：由 `scripts/prepare_pose_v2.py` 转换生成

`datasets/source_voc_review_20260730` 是额外保留的复核版 Pascal VOC 数据，便于回看标注和 contact sheet。

原始正样本是 Pascal VOC 标注，包含钢珠框。转换时用 VOC 框中心初始化关键点，所以中心点精度依赖原始框是否贴合钢珠。

## 划分策略

为了避免相邻视频帧同时进入训练集和测试集，V2 采用连续时间段留出：

- test chunks：`3`、`11`
- val chunks：`5`、`9`
- 每个 chunk：300 帧
- guard frames：30 帧

负样本为空凹槽画面，参与 train 和 val，用来降低误检。

| 集合 | 总数 | 正样本 | 负样本 | 增强图 |
| --- | ---: | ---: | ---: | ---: |
| train | 610 | 522 | 88 | 305 |
| val | 68 | 56 | 12 | 0 |
| test | 60 | 60 | 0 | 0 |

完整统计见：

- `docs/dataset_report.json`
- `datasets/gangzhu_pose_v2/dataset_report.json`
- `datasets/gangzhu_pose_v2/manifest.csv`
