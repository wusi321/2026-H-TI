# 2026-H-TI 车载平衡滚球运动控制系统

本仓库整理 2026 年全国大学生电子设计竞赛 H 题“车载平衡滚球运动控制系统”的完整完赛代码与文档，包含 MaixCAM 视觉测量、MaixCAM 图传录制、MSPM0G3507 小车主控、两套独立 Web 工具和设计报告。

系统目标是在车载横梁/PPR 管上检测并控制 1 cm 钢珠位置，同时完成题目要求的车辆行驶、循迹、停车和位置保持任务。

## 总体架构

```text
MaixCAM 视觉工程
  采集 640x360 图像
  检测钢珠位置/速度
  UART 输出视觉帧
        |
        v
MSPM0G3507 主控工程
  灰度循迹 / 轮速 / IMU / 航向
  任务 2-7 状态机
  钢珠位置闭环
  UART3 总线舵机控制
  UART2 蓝牙遥测
        |
        v
调参 Web 控制台
  Web Serial 接收遥测
  记录/回放/指标/仿真/参数导出

运动轨迹 Web 控制台
  Web Serial 接收小车运动遥测
  轮速、偏航角、工作模式和路径积分显示

MaixCAM 图传工程
  原生 WebRTC 视频流
  PC 浏览器本地录制
```

## 顶层目录

| 路径 | 说明 |
| --- | --- |
| `Maixcam/` | MaixCAM 视觉测量主工程：钢珠检测、标定、YOLO 辅助、UART 输出和 WebRTC 图传。 |
| `mspm0g3507_26ti/` | MSPM0G3507 主控工程：行车控制、循迹、IMU、钢珠闭环、任务状态机和遥测。 |
| `tuchuan/` | MaixCAM 图传专用版本：只启动 WebRTC，用于比赛视频预览和录制。 |
| `tiaocan/` | 钢珠调参 Web 控制台：记录小车运行状态、车速、小球运动和舵机状态，用于实车调参、回放、仿真和参数导出。 |
| `web/` | 小车运动轨迹 Web 控制台：记录方向、轮速、偏航角、工作模式并积分显示车辆路径。与 `tiaocan/` 互不依赖。 |
| `latex/` | 设计报告 LaTeX 源文件和已生成 PDF。 |
| `PINOUT.md` | 硬件引脚和接线说明。 |
| `2026年全国大学生电子设计竞赛H题：车载平衡滚球运动控制系统.md` | 题目说明 Markdown。 |
| `H 题《车载平衡滚球运动控制系统》作品测试记录与评分表.md` | 测试记录与评分表 Markdown。 |

## 硬件分工

- MaixCAM：固定在车体上方，识别管道中钢珠的一维位置和速度；可同时提供 WebRTC 图传。
- MSPM0G3507：系统主控，负责传感器采集、车辆运动、任务调度、钢珠闭环和执行器输出。
- 12 路灰度传感器：赛道循迹和停车线检测。
- 编码器电机：车辆速度与里程控制。
- IMU：航向角和角速度估计。
- 总线舵机：控制横梁/管道角度，使钢珠移动或保持目标位置。
- HC-05 蓝牙：将 MCU 遥测发送到 PC 调参台。

## 通信链路

| 链路 | 方向 | 用途 | 速率/协议 |
| --- | --- | --- | --- |
| MaixCAM UART -> MSPM0 UART1 | MaixCAM 到 MCU | 钢珠位置、速度、置信度 | 115200 8N1，20 字节二进制帧，CRC16 |
| MSPM0 UART3 -> 总线舵机 | MCU 到舵机 | 舵机角度和速度命令 | 见 `driver/ftServo.c` |
| MSPM0 UART2 -> HC-05 -> PC | MCU 到 `tiaocan` | 任务遥测、实测曲线、钢珠/舵机参数调试 | 9600 8N1，20 Hz 二进制遥测 |
| 运动遥测串口 -> PC | MCU 到 `web` | 小车方向、轮速、偏航角、工作模式和路径记录 | 9600 8N1，32 字节 `AA 55` 帧 |
| MaixCAM WebRTC -> PC | MaixCAM 到浏览器 | 视频预览和录制 | HTTP/WebRTC，默认端口 8000 |

## 运行入口

### 1. MaixCAM 视觉测量

```text
MaixVision 打开 Maixcam/maixcam
运行 maixcam/main.py
```

重点检查：

- `maixcam/config.py` 中相机、UART、WebRTC、YOLO 和标定配置；
- `maixcam/calibration/position_calibration.json` 是否为实车标定；
- 比赛时 `ENABLE_SERVO_CONTROL` 应保持 `False`，由 MCU 控制舵机。

详细说明见 `Maixcam/README.md`。

### 2. MSPM0G3507 主控

Keil：

```text
mspm0g3507_26ti/keil/ncontroller.uvprojx
```

CCS / TI ARM Clang：

```text
mspm0g3507_26ti/ticlang/ncontroller.projectspec
```

详细说明见 `mspm0g3507_26ti/README.md`。

### 3. 图传录制

MaixCAM 端运行 `tuchuan/main/maixcam` 图传专用工程后，PC 侧：

```powershell
cd tuchuan\main\host
.\start_webrtc_recorder.cmd
```

打开 `http://127.0.0.1:18765`，填写 MaixCAM IP 并录制。详细说明见 `tuchuan/README.md` 和 `tuchuan/main/README.md`。

### 4. 钢珠调参 Web 控制台

`tiaocan/` 是独立前端工程，用于记录小车运行状态、车速、小球运动、舵机状态并辅助调参：

```powershell
cd tiaocan
npm install
npm run dev -- --host 127.0.0.1 --port 4317
```

使用 Chrome 或 Edge 打开 `http://127.0.0.1:4317`，通过 Web Serial 连接 HC-05。详细说明见 `tiaocan/README.md`。

### 5. 小车运动轨迹 Web 控制台

`web/` 是另一套独立前端工程，用于显示和记录小车方向、轮速、偏航角、工作模式和积分路径：

```powershell
cd web
npm install
npm run dev
```

使用 Chrome 或 Edge 打开 Vite 输出的本机地址，通过 Web Serial 选择对应串口。详细说明见 `web/README.md`。

## 标定与调试顺序

1. 完成车体接线，核对 `PINOUT.md`、`ncontroller.syscfg` 和实际硬件。
2. 单独验证左右轮、电机 PWM、编码器方向和速度闭环。
3. 单独验证 IMU 航向角方向、灰度传感器顺序和停车线判断。
4. 固定 MaixCAM、管道和补光，完成钢珠位置标定。
5. 验证 MaixCAM UART 视觉帧：CRC、序号、坐标方向、超时回中。
6. 测量总线舵机中位、机械极限、舵机方向和横梁角度比例。
7. 静态调钢珠位置闭环，再低速车辆联调。
8. 按任务 2-7 逐项验收，每次有效实验使用调参台保存遥测记录。
9. 将最终参数同步到代码、设计报告和测试记录。

## 文档索引

- `Maixcam/maixcam/calibration/README.md`：视觉位置标定。
- `Maixcam/docs/mcu_control.md`：MaixCAM 与 MCU 控制权划分。
- `Maixcam/docs/task_execution.md`：任务目标和状态机说明。
- `mspm0g3507_26ti/UART2_TELEMETRY.md`：钢珠调参蓝牙遥测协议。
- `mspm0g3507_26ti/变量定义及调参说明.md`：MCU 调参变量说明。
- `tiaocan/README.md`：钢珠调参 Web 控制台说明。
- `web/README.md`：小车运动轨迹 Web 控制台说明。
- `latex/README.md`：设计报告编译说明。

## Git 提交范围

仓库保留源码、配置、文档、模型说明和必要资源。以下内容默认不提交：

- `node_modules/`、`dist/`、测试结果和前端构建缓存；
- Python `__pycache__`、`.pytest_cache`；
- Keil/编译中间产物和日志；
- WebRTC 录制文件、浏览器 profile、运行时缓存；
- LaTeX 编译中间文件。

如需发布某次比赛视频、构建固件或大型模型文件，建议使用 GitHub Release 或单独网盘，不直接放入源码仓库。
