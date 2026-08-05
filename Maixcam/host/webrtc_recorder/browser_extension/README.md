# MaixCAM Pro 原生 WebRTC 网页录像台

本版本直接使用 MaixCAM 的原生 WebRTC 页面，不再经过 RTSP、go2rtc、
FFmpeg、PotPlayer 或 Node.js。

## 安装（只需一次）

### Microsoft Edge

1. 在地址栏打开 `edge://extensions/`。
2. 打开左侧的“开发人员模式”。
3. 点击“加载解压缩的扩展”。
4. 选择本文件所在的整个文件夹：
   `maixcam_native_webrtc_recorder_extension`

### Google Chrome

1. 在地址栏打开 `chrome://extensions/`。
2. 打开右上角“开发者模式”。
3. 点击“加载已解压的扩展程序”。
4. 选择本文件所在的整个文件夹。

不要只选择 `manifest.json`，要选择包含它的文件夹。

## 使用

1. 在 MaixCAM Pro 上运行原生 WebRTC 程序。
2. 可以直接打开 `http://192.168.0.61:8000` 使用右下角录像台。
3. 也可以运行旧版界面目录中的 `start_web_recorder.bat`，在原来的
   `http://127.0.0.1:18765` 炫酷页面中完成录像。

当原生 WebRTC 页面被嵌入旧版界面时，本扩展会自动切换为录像桥接模式，
不会在内嵌画面中重复显示控制面板。

录像默认优先保存为 H.264/MP4；若当前浏览器不支持 MP4 MediaRecorder，
则自动改用 WebM。

## 性能说明

- 未录制时：扩展只检查页面内已有的视频状态，不解码第二路视频、不转码。
- 录制时：浏览器使用 MediaRecorder 记录当前 WebRTC 视频轨。
- MaixCAM 只发送一条原生 WebRTC 流，不再同时运行 RTSP。
- 录像停止前会暂存在浏览器内存中，建议单次录像不要长时间超过一小时。

## 地址变化

扩展 1.2.0 会自动识别局域网私有 IP 的 `8000` 端口，支持：

- `10.x.x.x:8000`
- `172.16.x.x`–`172.31.x.x:8000`
- `192.168.x.x:8000`

换 Wi-Fi 后只需在旧版网页底部输入新的设备 IP 并点击“连接”，不需要修改
`manifest.json`。如果刚刚更新了扩展，仍需在扩展管理页面点击一次“重新加载”。
