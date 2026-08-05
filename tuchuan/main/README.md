# MaixCAM 图传专用工程

本目录是 MaixCAM 图传专用版本，当前 `maixcam/main.py` 只启动 MaixCAM 原生 WebRTC 服务，用于 PC 端录制比赛视频。它不运行钢珠识别、YOLO、UART 遥测、MCU 控制、本地调试画面、补光灯或舵机输出。

如果需要完整视觉测量与 MCU 通信，请使用同级根目录下的 `../../Maixcam` 工程。

## 当前数据流

```text
320x180 NV21 摄像头
-> MaixCAM 原生 WebRTC 服务
-> PC 侧 host/webrtc_recorder
-> 浏览器 MediaRecorder 本地下载 MP4/WebM
```

## 目录说明

| 路径 | 内容 |
| --- | --- |
| `maixcam/main.py` | 图传专用入口，只创建 WebRTC 摄像头并保持服务运行。 |
| `maixcam/config.py` | 图传分辨率、帧率、码率、GOP 和曝光配置。 |
| `maixcam/web_stream.py` | MaixCAM 原生 WebRTC API 封装。 |
| `host/webrtc_recorder/` | PC 侧录制网页、Node 服务和浏览器扩展。 |
| `host/start_webrtc_recorder.cmd` | 启动 PC 侧录制台。 |
| `host/recordings/` | 本地录制输出目录，已在根仓库 `.gitignore` 中默认忽略。 |
| `docs/`、`mcu/`、`tests/`、其他 `maixcam/*.py` | 从完整视觉工程保留的历史参考文件，当前 `app.yaml` 不调用。 |

## 配置

当前版本号：

```python
VERSION = "2026-08-04-video-stream-only"
```

默认图传参数：

| 配置项 | 默认值 |
| --- | ---: |
| `ENABLE_WEBRTC` | `True` |
| `WEBRTC_STREAM_WIDTH` | `320` |
| `WEBRTC_STREAM_HEIGHT` | `180` |
| `WEBRTC_STREAM_FPS` | `15` |
| `WEBRTC_STREAM_BUFFERS` | `3` |
| `WEBRTC_BITRATE` | `600_000` |
| `WEBRTC_GOP` | `30` |

`MANUAL_EXPOSURE_US` 和 `MANUAL_GAIN` 默认为 `None`，表示使用相机自动曝光/增益。

## MaixCAM 端运行

1. 在 MaixVision 中打开完整的 `tuchuan/main/maixcam` 文件夹。
2. 运行工程，而不是只拷贝单个脚本。
3. 终端应输出类似：

```text
maixcam video stream version: 2026-08-04-video-stream-only
recognition, UART, YOLO, tracker, display and fill light are not used
video stream only: WebRTC 320x180@15 bitrate:600000 gop:30
```

4. 在浏览器直接访问 `http://<MaixCAM-IP>:8000` 可验证原生 WebRTC 页面是否可用。

## PC 侧录制

```powershell
cd tuchuan\main\host
.\start_webrtc_recorder.cmd
```

录制台默认打开：

```text
http://127.0.0.1:18765
```

首次使用需要加载 `host/webrtc_recorder/browser_extension` 浏览器扩展。详细步骤见 `host/webrtc_recorder/README.md`。

## 与完整视觉工程的区别

| 项目 | `tuchuan/main` | `Maixcam` |
| --- | --- | --- |
| 目标 | 比赛视频图传/录制 | 钢珠视觉测量、UART 输出和图传 |
| 摄像头分辨率 | `320x180@15` | 视觉 `640x360@60`，图传 `640x360@15` |
| UART 到 MCU | 不启用 | 默认启用 |
| YOLO/传统视觉 | 不启用 | 可启用 |
| 标定文件 | 不使用 | 使用 `calibration/position_calibration.json` |
| 舵机测试 | 不启用 | 保留台架诊断开关，比赛默认关闭 |

## 常见问题

- 若页面一直连接失败，先直接访问 `http://<MaixCAM-IP>:8000`，确认 MaixCAM 原生 WebRTC 服务已启动。
- 若 PC 录制台端口 `18765` 被占用，关闭旧的 `server.js` Node 进程后重试。
- 若画面卡顿，可先降低 `WEBRTC_BITRATE`，再考虑降低帧率或分辨率。
- 当前工程不输出钢珠位置；不要把它接到 MCU 闭环控制链路中。
