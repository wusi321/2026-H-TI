# MaixCAM 原生 WebRTC 录制台

本目录是 `tuchuan/main` 图传专用工程的 PC 侧录制工具。它打开 MaixCAM 原生 WebRTC 页面，并通过浏览器 MediaRecorder API 保存本地 MP4/WebM 文件。

当前图传工程只提供视频流，不包含钢珠识别、UART 遥测或 MCU 控制。

## 工作方式

```text
MaixCAM 图传专用程序
-> http://<device-ip>:8000 原生 WebRTC 页面
-> 本地录制台 http://127.0.0.1:18765
-> 浏览器扩展读取视频轨
-> MediaRecorder 本地录制并下载
```

## 文件说明

| 文件/目录 | 说明 |
| --- | --- |
| `server.js` | 本地 HTTP 服务。 |
| `device.json` | 默认设备 IP、WebRTC 端口和本地网页端口。 |
| `start_web_recorder.bat` | 双击启动录制台。 |
| `web/` | 录制台页面。 |
| `browser_extension/` | Edge/Chrome 浏览器扩展。 |

## 首次安装扩展

Edge：

1. 打开 `edge://extensions/`。
2. 启用“开发人员模式”。
3. 点击“加载解压缩的扩展”。
4. 选择 `browser_extension` 文件夹。

Chrome：

1. 打开 `chrome://extensions/`。
2. 启用“开发者模式”。
3. 点击“加载已解压的扩展程序”。
4. 选择 `browser_extension` 文件夹。

旧版扩展需要先重新加载或删除后重新加载。

## 使用步骤

1. 在 MaixCAM 上运行 `tuchuan/main/maixcam` 工程。
2. 直接访问 `http://<MaixCAM-IP>:8000`，确认原生 WebRTC 页面可用。
3. 在 `tuchuan/main/host` 目录运行：

```powershell
.\start_webrtc_recorder.cmd
```

4. 打开 `http://127.0.0.1:18765`。
5. 输入 MaixCAM IP 并点击连接。
6. 画面在线后开始录制，停止后下载文件。

## 配置文件

`device.json`：

```json
{
  "deviceIp": "192.168.0.61",
  "webrtcPort": 8000,
  "webPort": 18765
}
```

网页输入框中的 IP 优先级高于配置文件。

## 排查

- “未检测到扩展”：检查扩展是否启用并刷新页面。
- “连接失败”：先直接打开 `http://<MaixCAM-IP>:8000` 验证 MaixCAM 端服务。
- “18765 端口占用”：关闭旧录制台或结束旧 `server.js` Node 进程。
- “画面卡顿”：降低 MaixCAM 端码率、分辨率或帧率。
