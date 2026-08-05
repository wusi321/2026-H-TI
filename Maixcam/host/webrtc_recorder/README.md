# MaixCAM 原生 WebRTC 录制台

本目录是 PC 侧 WebRTC 录制工具，用于打开 MaixCAM 原生 WebRTC 页面，并通过浏览器 MediaRecorder API 录制本地视频文件。它不再依赖旧版 RTSP/go2rtc 转码链路。

## 工作方式

```text
MaixCAM 原生 WebRTC 页面 http://<device-ip>:8000
-> 本地 Node 服务 http://127.0.0.1:18765
-> 浏览器扩展注入录制桥
-> MediaRecorder 录制正在播放的视频流
-> 浏览器下载 MP4/WebM
```

Node 服务只负责提供录制台网页，不做视频解码、转码或中继。录制动作发生在浏览器中。

## 文件说明

| 文件/目录 | 说明 |
| --- | --- |
| `server.js` | 本地录制台 HTTP 服务。 |
| `device.json` | 默认设备 IP、WebRTC 端口和本地网页端口。 |
| `start_web_recorder.bat` | 双击启动录制台。 |
| `web/` | 录制台前端页面。 |
| `browser_extension/` | Chrome/Edge 扩展，用于访问嵌入页面的视频轨并建立录制桥。 |
| `start_web_recorder.log` | 运行日志，已在根仓库 `.gitignore` 中默认忽略。 |

## 首次安装浏览器扩展

Microsoft Edge：

1. 打开 `edge://extensions/`。
2. 打开“开发人员模式”。
3. 点击“加载解压缩的扩展”。
4. 选择本目录下完整的 `browser_extension` 文件夹。

Chrome：

1. 打开 `chrome://extensions/`。
2. 打开“开发者模式”。
3. 点击“加载已解压的扩展程序”。
4. 选择 `browser_extension` 文件夹。

如果之前加载过旧版扩展，先在扩展管理页点击“重新加载”，或删除旧扩展后重新加载本目录版本。

## 每次使用

1. 在 MaixCAM 上运行启用了 WebRTC 的工程。
2. 确认浏览器可直接访问 `http://<MaixCAM-IP>:8000`。
3. 双击 `start_web_recorder.bat`，或在上级 `host` 目录运行：

```powershell
.\start_webrtc_recorder.cmd
```

4. 浏览器打开 `http://127.0.0.1:18765`。
5. 在页面底部输入 MaixCAM 当前 IP，点击连接。
6. 状态显示视频在线后点击开始录制。
7. 停止录制后等待 Blob 生成，点击下载本次录像。

## 配置

`device.json` 示例：

```json
{
  "deviceIp": "192.168.0.61",
  "webrtcPort": 8000,
  "webPort": 18765
}
```

页面中手动输入的 IP 优先级高于 `device.json`。扩展支持常见私有网段：`10.x.x.x`、`172.16.x.x` 到 `172.31.x.x`、`192.168.x.x`。

## 常见问题

### 页面提示未检测到录制桥扩展

扩展未启用或未重新加载。打开扩展管理页，确认扩展已启用，然后刷新录制台页面。

### 页面一直连接中

先直接访问 `http://<MaixCAM-IP>:8000`。如果打不开，问题在 MaixCAM 端 WebRTC 服务、IP 或网络连接；如果能打开，再检查录制台填写的 IP 和浏览器扩展状态。

### 端口 18765 被占用

关闭旧的录制台命令行窗口，或在任务管理器中结束旧版 `server.js` 对应的 Node 进程。不要结束 MaixVision 自带的 Node 进程。

### 连接后画面卡顿

优先降低 MaixCAM 端 `WEBRTC_BITRATE`，再考虑降低分辨率或帧率。录制台本身不做转码，性能瓶颈通常在 MaixCAM 编码、无线链路或浏览器解码。
