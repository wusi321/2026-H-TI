# 图传工程目录

`tuchuan` 保存 MaixCAM 图传专用版本，用于比赛过程视频预览和录制。实际工程位于 `main/`，当前只启动原生 WebRTC，不执行钢珠识别或 MCU 通信。

快速入口：

- MaixCAM 端：`main/maixcam/main.py`
- 配置：`main/maixcam/config.py`
- PC 录制台：`main/host/webrtc_recorder`
- 详细说明：`main/README.md`

运行方式：

```powershell
cd tuchuan\main\host
.\start_webrtc_recorder.cmd
```

浏览器打开 `http://127.0.0.1:18765`，填写 MaixCAM IP 后连接 `http://<MaixCAM-IP>:8000` 的原生 WebRTC 页面并录制。

完整视觉测量、UART 输出和 YOLO 辅助识别请使用根目录 `Maixcam/`。
