# MSPM0G3507 主控与车体控制工程

本目录是 2026 年全国大学生电子设计竞赛 H 题“车载平衡滚球运动控制系统”的 MSPM0G3507 主控工程。MCU 负责整车行驶、灰度循迹、IMU 姿态/航向、钢珠位置闭环、舵机总线输出、任务调度、蓝牙遥测和安全保护。

MaixCAM 只提供钢珠位置/速度观测。控制权在 MCU 侧，避免把摄像头处理延迟直接接入执行器。

## 系统职责

```text
MaixCAM UART1 视觉帧
-> BallBalance_UART1_RxByte()
-> 位置/速度滤波与有效性判断
-> 任务目标生成 auto_vision_2026_*
-> 200 Hz 钢珠位置/速度/加速度串级控制
-> UART3 总线舵机角度与速度命令
-> 小车速度、循迹、航向、停车线和任务完成判断
-> UART2 蓝牙 20 Hz 遥测
```

## 目录说明

| 路径 | 内容 |
| --- | --- |
| `user/` | 主程序、周期任务入口、公共头文件和数据类型。 |
| `apply/` | 应用层控制逻辑，包括钢珠平衡、灰度循迹、姿态、PID、UI、传感器、数学工具等。 |
| `apply/developer/` | 比赛任务状态机和开发调试模式。 |
| `apply/Fusion/` | Fusion AHRS 姿态融合库。 |
| `driver/` | UART、I2C、PWM、编码器、舵机、OLED、Flash、按键、ADC、IMU 等底层驱动。 |
| `keil/` | Keil MDK 工程文件、链接脚本和历史构建日志。 |
| `ticlang/` | CCS/TI ARM Clang projectspec、Makefile 和链接配置。 |
| `tools/` | SysConfig、Keil 辅助脚本和 UART2 遥测采集脚本。 |
| `source/` | TI MSPM0 SDK、CMSIS 和 DriverLib 源码/头文件。 |
| `openmv_python/` | 历史视觉参考代码。当前比赛视觉以 `../Maixcam` 为准。 |

## 关键源码入口

- `user/main.c`：系统初始化、主循环、1000 Hz/200 Hz/100 Hz 周期任务。
- `apply/ball_balance.c/.h`：钢珠位置闭环、车辆加速度前馈、转弯补偿、舵机限幅/限速和任务专用参数。
- `apply/vision.c/.h`：视觉输入处理相关逻辑。
- `apply/developer/subtask.c/.h`：H 题任务 2-7 自动视觉调度接口。
- `driver/ftServo.c/.h`：UART3 总线舵机角度和速度控制。
- `driver/nuart.c/.h`：UART 驱动和蓝牙/视觉/地面站通信基础。
- `tools/uart2_telemetry.py`：PC 侧串口采集 UART2 蓝牙遥测 CSV。

## 周期任务

| 周期 | 函数 | 主要工作 |
| ---: | --- | --- |
| 1000 Hz | `duty_1000hz()` | 灰度输入、停车线处理、视觉输入读取、模拟 PWM 输出。 |
| 200 Hz | `maple_duty_200hz()` | 遥控输入、轮速采样、SDK 任务、电机输出、钢珠闭环、舵机循环、IMU/AHRS、电池与按键。 |
| 100 Hz | `duty_100hz()` | UART2 蓝牙遥测、地面站状态机、低频任务。 |

钢珠控制在 200 Hz 更新，舵机通信由 `Servo_Y_Loop()` 节流输出。

## 硬件连接

核心引脚请以根目录 `PINOUT.md` 和 `ncontroller.syscfg` 为准。当前代码中的主要连接：

- MaixCAM Pro：UART1，`PA8 TX` / `PA9 RX`，115200 8N1。
- 蓝牙 HC-05：UART2，`PA21 TX` / `PA22 RX`，用于 20 Hz 调参遥测。
- 总线舵机：UART3，`PB2 TX`，写角度/速度命令。
- 12 路灰度：`PA31, PA28, PA1, PA0, PA25, PA24, PB24, PB23, PB19, PB18, PA16, PB13`。
- 电机 PWM：`PA4, PA7, PA3, PB14`。
- 编码器：`PB4/PB5` 脉冲，`PB6/PB7` 方向。
- IMU I2C：`PA29 SCL` / `PA30 SDA`。
- 电池电压：`PA26 ADC`，需外部分压。

## MaixCAM 视觉协议

MCU 通过 UART1 接收 MaixCAM 20 字节帧：

```text
AA 55 | version | flags | sequence | timestamp_ms
      | position_mm | velocity_mm_s | confidence_milli
      | processing_us | crc16
```

解析逻辑包含：

- 帧头搜索与错位重同步；
- CRC16-CCITT 校验；
- 重复/旧序号拒绝；
- MaixCAM 重启后的序号恢复；
- 视觉超时与安全回中。

完整协议见 `../Maixcam/maixcam/vision_protocol.py`、`apply/ball_balance.c` 和 `UART2_TELEMETRY.md`。

## 钢珠控制策略

控制器输入为滤波后的钢珠位置、速度，以及车体实测/命令速度与航向角速度。核心结构是位置到速度、速度到加速度、加速度到横梁/舵机角度的串级控制，并叠加车辆加速度前馈、转弯补偿和任务专用限幅。

关键保护：

- 视觉置信度、物理范围和超时判断；
- 舵机角度限幅、速度限幅和回中；
- 积分限幅与失联冻结；
- 起步阶段前馈保持；
- 近目标刹车和防超调；
- 不同任务独立参数档位。

主要参数位于 `BallBalanceConfig g_ball_balance_config`，详细变量含义见 `变量定义及调参说明.md`。

## 比赛任务

当前任务入口集中在 `apply/developer/subtask.c`：

```c
auto_vision_2026_task2(route_complete);
auto_vision_2026_task3();
auto_vision_2026_task4(route_complete);
auto_vision_2026_task5(route_complete);
auto_vision_2026_task6(route_complete);
auto_vision_2026_task7(route_complete);
```

任务概览：

| 工作模式 | 对应任务 | 控制目标 |
| ---: | --- | --- |
| 2 | 任务 2 | 灰度循迹一圈并识别 A 点停车线。 |
| 3 | 任务 3 | 车辆静止，钢珠按 `0 -> +55 mm -> -55 mm` 移动。 |
| 4 | 任务 4 | A 到 B 直线行驶，同时保持钢珠目标位置。 |
| 5 | 任务 5 | 灰度循迹一圈并在 A 点停止。 |
| 6 | 任务 6 | 记忆任意钢珠位置，循迹一圈并保持该位置。 |
| 7 | 任务 7 | 任务 6 的独立控制参数实验档。 |

## 编译方法

### Keil MDK

打开：

```text
keil/ncontroller.uvprojx
```

要求 ARM Compiler 6 和匹配的 MSPM0 Device Family Pack。保持工程相对路径不变。

### CCS / TI ARM Clang

在 CCS 中导入：

```text
ticlang/ncontroller.projectspec
```

也可参考 `ticlang/makefile` 进行命令行构建。修改 `ncontroller.syscfg` 后需重新生成 `ti_msp_dl_config.c/.h`，并复核 UART、GPIO、定时器、PWM 和 ADC 配置。

## 调参与遥测

UART2 通过 HC-05 输出 20 Hz 二进制遥测，用于 `../tiaocan` 调参台或命令行 CSV 采集。

命令行采集示例：

```powershell
python -m pip install pyserial
python tools\uart2_telemetry.py --list
python tools\uart2_telemetry.py --port COM7 --csv task3_run.csv
```

遥测协议见 `UART2_TELEMETRY.md`。调参变量和建议流程见 `变量定义及调参说明.md`。

## 联调建议顺序

1. 断开钢珠机构，确认左右轮方向、编码器方向和速度闭环。
2. 静止校准 IMU，确认航向角方向和角速度方向。
3. 验证灰度传感器顺序、阈值、路线方向和停车线判断。
4. 单独运行 MaixCAM，完成真实标定并确认坐标左负右正。
5. 注入或接收视觉帧，检查 CRC、序号、超时和安全回中。
6. 测量舵机中位、机械极限、舵机方向和横梁角度比例。
7. 静态调钢珠闭环，再低速车辆联调。
8. 最后按任务 2-7 验收，保存每组遥测记录。
