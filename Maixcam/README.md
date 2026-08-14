# MaixCAM 视觉测量与小球位置估计工程

本目录是 MaixCAM 端完整视觉工程，负责识别 25 cm PPR 管/横梁上的 1 cm 钢珠，输出一维位置、速度和置信度，并通过 UART 发送给 MSPM0G3507 主控。比赛系统中，MaixCAM 只做感知和图像辅助，任务状态机、行车控制、钢珠闭环控制和舵机输出均由 MCU 侧负责。

## 当前定位

主要运行入口为 `maixcam/main.py`，配置集中在 `maixcam/config.py`。当前配置版本为 `2026-08-01-h-ball-simple-13`，主程序启动时会校验版本，避免只拷贝单个 `main.py` 导致文件不匹配。

核心数据流：

```text
640x360 RGB 摄像头
-> 固定管道 ROI
-> LAB 暗色钢珠候选检测
-> 自适应亮度阈值
-> 动态跟踪窗口和全管道回退搜索
-> 可选 YOLO 低频重捕获/身份确认
-> 分段像素-厘米标定
-> Alpha-Beta 位置/速度估计
-> UART 二进制帧
-> MSPM0G3507 控制器
```

WebRTC 图传与视觉处理共用 MaixCAM，但使用独立摄像头通道，便于比赛记录和现场调试。

## 目录说明

| 路径 | 作用 |
| --- | --- |
| `maixcam/` | MaixVision 工程目录，包含主程序、配置、检测、融合、标定、UART 协议和可选 YOLO 模块。 |
| `maixcam/calibration/` | 管道坐标标定数据和标定说明。 |
| `maixcam/models/` | 模型文件说明。实际部署时模型通常放在 MaixCAM 的 `/root/models`。 |
| `host/` | PC 侧标定生成、UART 监听、RTSP/WebRTC 录制辅助脚本。 |
| `host/webrtc_recorder/` | 原生 WebRTC 浏览器录制台和 Edge/Chrome 扩展。 |
| `mcu/` | 便携式 C 语言参考模块：视觉协议解析、视觉链路封装、任务状态机。 |
| `docs/` | MCU 接入、任务执行、舵机控制等说明。 |
| `tests/` | 脱离硬件的 Python/C 协议和核心逻辑测试。 |
| `yolo/` | 钢珠 YOLO11n-Pose 训练、数据集、评估结果、ONNX 导出和 MaixCAM 专属格式转换资料。 |
| `backups/` | 历史实验版本，已在根仓库 `.gitignore` 中默认忽略。 |

## 关键模块

- `ball_detector.py`：在固定 ROI 内寻找钢珠候选。检测不强依赖圆度，因为反光钢珠在二值图中常呈月牙形，而不是实心圆。
- `candidate_confirmation.py`：对远距离跳变候选做连续帧确认，降低刻度线、反光和管壁误检。
- `measurement_fusion.py`：融合传统视觉候选和 YOLO 候选；YOLO 只作为身份确认/重捕获辅助，不直接替代每帧传统视觉测量。
- `position_calibration.py`：把像素坐标映射到管道中心坐标，左负右正，单位 cm。
- `state_estimator.py`：Alpha-Beta 跟踪器，输出滤波后位置和速度。
- `vision_protocol.py`：MaixCAM -> MCU 的 20 字节 UART 帧编码/解码与 CRC16。
- `web_stream.py`：启动 MaixCAM 原生 WebRTC 服务。
- `uart_transport.py`：串口写入封装。

`yolo/` 是独立整理的训练与部署资料目录，主工程运行时只需要最终部署到 MaixCAM 的模型文件；完整数据集、训练脚本、评估报告、`best.pt`、`best.onnx` 和 `.cvimodel/.mud` 转换资料都保存在该目录内，便于复现实验和上传 GitHub 归档。

## 硬件与通信

默认 UART 配置：

```text
MaixCAM A16 / UART0_TX -> MSPM0 PA9 / UART1_RX
MaixCAM A17 / UART0_RX <- MSPM0 PA8 / UART1_TX
GND                   -- GND
115200 baud, 8N1, no flow control
```

协议帧由 `vision_protocol.py` 和 MCU 侧 `mcu/vision_protocol.c` 共同定义：

```text
AA 55 | version | flags | sequence | timestamp_ms
      | position_mm | velocity_mm_s | confidence_milli
      | processing_us | crc16
```

坐标定义：

- 管道中心为 `0`；
- 左侧为负，右侧为正；
- 25 cm 管道物理端点约为 `-12.5 cm` 和 `+12.5 cm`；
- 正常钢珠中心可用范围约为 `±12 cm`。

## 首次运行

1. 在 MaixVision 中打开完整的 `Maixcam/maixcam` 文件夹，不要只运行单个脚本。
2. 确认 `maixcam/config.py` 中的 `VERSION` 与 `main.py` 的 `EXPECTED_CONFIG_VERSION` 一致。
3. 固定摄像头、管道和补光灯，避免标定后机械位置改变。
4. 按 `maixcam/calibration/README.md` 采集标定点并生成 `position_calibration.json`。
5. 未完成真实标定前，保持 `UART_ALLOW_PLACEHOLDER_CONTROL = False`，避免把占位坐标作为有效控制量。
6. 若需要 WebRTC 录制，保持 `ENABLE_WEBRTC = True`，并使用 `host/webrtc_recorder`。
7. 比赛联调时保持 `ENABLE_SERVO_CONTROL = False`，舵机闭环交给 MSPM0G3507。

## 主要配置项

| 配置项 | 默认值 | 说明 |
| --- | ---: | --- |
| `CAMERA_WIDTH/HEIGHT/FPS` | `640x360@60` | 视觉检测输入。 |
| `WEBRTC_STREAM_WIDTH/HEIGHT/FPS` | `640x360@15` | 图传录制通道。 |
| `ENABLE_UART` | `True` | 是否发送 MCU 视觉测量帧。 |
| `ENABLE_WEBRTC` | `True` | 是否启动原生 WebRTC 页面。 |
| `ENABLE_RTSP` | `False` | 旧固件备用图传路径。 |
| `ENABLE_YOLO_REACQUIRE` | `True` | 是否启用 YOLO 低频辅助。模型不存在时会自动降级。 |
| `LAB_THRESHOLDS` | `L<=55` | 钢珠暗色区域阈值，需要随光照复核。 |
| `ADAPTIVE_L_ENABLED` | `True` | 低频估计 ROI 亮度并调整 L 上限。 |
| `TRACK_VALID_HOLD_MS` | `70` | 单帧丢失后的短时预测保持窗口。 |

## YOLO 训练与转换

新增的 `yolo/` 目录包含钢珠中心点检测的 YOLO11n-Pose 训练工程。最终训练数据位于 `yolo/datasets/gangzhu_pose_v2`，已经整理为 Ultralytics YOLO pose 格式；训练和导出脚本位于 `yolo/scripts`；训练曲线、评估结果和权重位于 `yolo/results` 与 `yolo/docs`。

主要产物：

- `yolo/results/weights/best.pt`：PyTorch 最优权重。
- `yolo/results/weights/best.onnx`：导出的 ONNX 模型。
- `yolo/maixcam_conversion/deploy/gangzhu_yolo11n_pose_320_int8.cvimodel`：MaixCAM CV181x NPU 可运行的 INT8 模型。
- `yolo/maixcam_conversion/deploy/gangzhu_yolo11n_pose_320.mud`：MaixPy 加载 `.cvimodel` 所需的模型描述文件。

训练入口：

```powershell
cd Maixcam\yolo\scripts
.\run_train.ps1 --device 0
```

ONNX 导出入口：

```powershell
cd Maixcam\yolo\scripts
.\run_export.ps1
```

ONNX 转 MaixCAM 专属格式的脚本、WSL2/TPU-MLIR 环境说明和历史转换报告见 `yolo/maixcam_conversion/README.md`。部署到 MaixCAM 时，将 `.mud` 和 `.cvimodel` 放在设备同一模型目录下，并在 MaixPy 中加载 `.mud` 文件。

## 图传录制

MaixCAM 端运行后，原生 WebRTC 页面通常位于：

```text
http://<MaixCAM-IP>:8000
```

PC 侧录制：

```powershell
cd Maixcam\host
.\start_webrtc_recorder.cmd
```

浏览器会打开 `http://127.0.0.1:18765`。首次使用需按 `host/webrtc_recorder/README.md` 安装浏览器扩展。录制结果由浏览器本地下载，不经过服务器转码。

## 标定与验证

推荐至少采集 `-10, -5, 0, +5, +10 cm` 五个物理位置。每个位置读取多帧终端输出中的 `px:(cx,cy)` 并取中位数，再用 `host/build_position_calibration.py` 生成标定文件。

验证标准建议：

- 各标定点平均误差不超过 `0.2 cm`；
- 最大误差不超过 `0.5 cm`；
- 坐标方向必须左负右正；
- 小角度倾斜时位置漂移不超过 `0.3 cm`。

## 本地测试

在 PC 上可运行硬件无关测试：

```powershell
cd Maixcam
python -m unittest discover -s tests -v
```

测试覆盖协议编解码和核心逻辑，不等价于实机检测效果。阈值、标定、曝光、补光和机械固定仍必须在最终车体上验证。
