# MSPM0G3507 工程引脚定义

本文档汇总当前工程的全部已配置 MCU 引脚。内容以 `ncontroller.syscfg`、`ti_msp_dl_config.h/.c` 和驱动实际调用为准。

方向均以 MCU 为参照：`输出` 表示 MSPM0G3507 驱动外设，`输入` 表示 MSPM0G3507 接收外设信号。

## 通信接口

| MCU 引脚 | 外设功能 | 方向 | 波特率/速率 | 工程用途与外部连接 |
|---|---|---|---:|---|
| `PA10` | `UART0_TX` | 输出 | 460800 baud | 无名创新地面站；接 USB-TTL 的 RXD |
| `PA11` | `UART0_RX` | 输入 | 460800 baud | 无名创新地面站；接 USB-TTL 的 TXD |
| `PA8` | `UART1_TX` | 输出 | 115200 baud | MaixCAM Pro；接 MaixCAM A18/RX |
| `PA9` | `UART1_RX` | 输入 | 115200 baud | MaixCAM Pro；接 MaixCAM A19/TX |
| `PA21` | `UART2_TX` | 输出 | 9600 baud | 蓝牙串口模块；接模块 RXD，TX 使用 DMA 通道 0 |
| `PA22` | `UART2_RX` | 输入 | 9600 baud | 蓝牙串口模块；接模块 TXD |
| `PB2` | `UART3_TX` | 输出 | 115200 baud | 总线舵机 DATA 写入线，当前舵机 ID=2 |
| `PB3` | `UART3_RX` | 输入 | 115200 baud | 总线舵机回读预留，当前主要写命令 |
| `PA29` | `I2C1_SCL` | 双向开漏 | 1 MHz | ICM42688 SCL，沿用原 MPU6050 接口 |
| `PA30` | `I2C1_SDA` | 双向开漏 | 1 MHz | ICM42688 SDA，自动检测地址 `0x68/0x69` |

## 电机与舵机 PWM

### 双路直流电机

`TIMA0` 产生四路电机 H 桥 PWM。应用层当前使用如下映射：

| MCU 引脚 | 定时器通道 | 方向 | 工程信号 | 外部连接 |
|---|---|---|---|---|
| `PA4` | `TIMA0_CCP3` | 输出 | 右电机 PWM 1 | 右侧电调/H 桥 `INA1` |
| `PA7` | `TIMA0_CCP2` | 输出 | 右电机 PWM 2 | 右侧电调/H 桥 `INA2` |
| `PA3` | `TIMA0_CCP1` | 输出 | 左电机 PWM 1 | 左侧电调/H 桥 `INB1` |
| `PB14` | `TIMA0_CCP0` | 输出 | 左电机 PWM 2 | 左侧电调/H 桥 `INB2` |

电机软件输出范围为 `0..999`，正反转通过同一电机的两路 PWM 交替输出实现。最终正负方向还受 Flash 中的电机方向配置参数影响。

### 舵机/执行器 PWM

| MCU 引脚 | 定时器通道 | 方向 | 软件接口 | 当前用途 |
|---|---|---|---|---|
| `PA15` | GPIO | 输出 | `Timer_Gate_Set()` | 外部计时器使能 `TIMER_GATE`：高电平开始/继续计时，低电平暂停/停止，默认低电平 |
| `PB1` | `TIMA1_CCP1` | 输出 | `steer_servo_pwm_m1p1()` | 预留执行器/PWM 2 |
| `PA23` | `TIMG7_CCP0` | 输出 | `steer_servo_pwm_m1p2()` | 预留执行器/PWM 3 |
| `PA2` | `TIMG7_CCP1` | 输出 | `steer_servo_pwm_m1p3()` | 前轮转向舵机；车辆任务主要使用此路 |

当前可用舵机 PWM 为 `PB1`、`PA23`、`PA2` 三路，周期为 20 ms，接口参数单位为微秒脉宽。`PA15` 已从旧的预留 PWM 用途改为外部计时器 GPIO，不再作为舵机 PWM 输出。

主控 `PA15/TIMER_GATE` 用于连接独立计时器板 `TIME-OLED` 的 `PA15/E2B` 输入。两块板必须共地；主控输出高电平时计时器 OLED 显示 `RUN` 并累加时间，输出低电平时显示 `PAUSE` 并保持当前时间。

PA15 诊断方式：将工作模式切换到 `28` 会在电机保持停止的情况下强制输出高电平；屏幕第 `31` 页显示 `request`、`dout`、`output en`、`pin level` 四个状态。正常高电平时四项均为 `1`。若前三项为 `1` 而 `pin level` 为 `0`，应检查外部电路是否拉低 PA15；若四项均为 `1` 而测量仍为低电平，应确认测量的是 LQFP64 封装第 8 脚对应的 PA15，并与控制板共地。

上表以 `ncontroller.syscfg`、`ti_msp_dl_config.h` 和 `driver/ntimer.c` 的实际调用为准。

## 编码器接口

| MCU 引脚 | 工程信号 | 方向 | 中断/采样方式 | 外部连接 |
|---|---|---|---|---|
| `PB4` | `RIGHT_PULSE` | 输入 | 上升沿和下降沿 GPIO 中断 | 右轮编码器倍频脉冲 P1 |
| `PB5` | `LEFT_PULSE` | 输入 | 上升沿和下降沿 GPIO 中断 | 左轮编码器倍频脉冲 P2 |
| `PB6` | `RIGHT_DIR` | 输入 | GPIO 电平读取 | 右轮编码器方向/鉴相 D1 |
| `PB7` | `LEFT_DIR` | 输入 | GPIO 电平读取 | 左轮编码器方向/鉴相 D2 |

编码器方向可通过软件参数反转，无需调换管脚。

## 12 路灰度传感器

| 灰度通道 | MCU 引脚 | 工程宏 | 方向 |
|---:|---|---|---|
| P1 | `PA31` | `GRAY_BIT0` | 输入 |
| P2 | `PA28` | `GRAY_BIT1` | 输入 |
| P3 | `PA1` | `GRAY_BIT2` | 输入 |
| P4 | `PA0` | `GRAY_BIT3` | 输入 |
| P5 | `PA25` | `GRAY_BIT4` | 输入 |
| P6 | `PA24` | `GRAY_BIT5` | 输入 |
| P7 | `PB24` | `GRAY_BIT6` | 输入 |
| P8 | `PB23` | `GRAY_BIT7` | 输入 |
| P9 | `PB19` | `GRAY_BIT8` | 输入 |
| P10 | `PB18` | `GRAY_BIT9` | 输入 |
| P11 | `PA16` | `GRAY_BIT10` | 输入 |
| P12 | `PB13` | `GRAY_BIT11` | 输入 |

当前驱动直接读取数字电平，可按工作模式处理 7 路或 12 路灰度数据。

## 显示屏接口

显示驱动使用独立的软件时序 GPIO。`LCD_SDA` 会在输出和带上拉输入之间切换，以支持需要读取 SDA 的显示控制器。

| MCU 引脚 | 工程信号 | 方向 | 说明 |
|---|---|---|---|
| `PA17` | `LCD_SCL` | 输出 | 显示屏串行时钟 |
| `PB15` | `LCD_SDA` | 双向 | 显示屏串行数据 |
| `PB16` | `LCD_RST` | 输出 | 显示屏复位，低电平有效 |
| `PB17` | `LCD_DC` | 输出 | 数据/命令选择 |
| `PB20` | `LCD_CS` | 输出 | 显示屏片选，低电平有效 |

## W25Q64 外部 Flash

W25Q64 使用软件 SPI，保存控制参数、传感器校准值等数据。

| MCU 引脚 | 工程信号 | 方向 | 说明 |
|---|---|---|---|
| `PA12` | `SPI0_SCLK` | 输出 | W25Q64 时钟 |
| `PA14` | `SPI0_MOSI` | 输出 | MCU 到 W25Q64 数据 |
| `PA13` | `SPI0_MISO` | 输入 | W25Q64 到 MCU 数据 |
| `PB25` | `W25Q64_CS` | 输出 | W25Q64 片选，低电平有效，默认拉高 |

这里的 `SPI0` 是工程中的 GPIO 分组名称，驱动通过 GPIO 位操作实现软件 SPI，并非 SysConfig 硬件 SPI 外设实例。

## 按键与人机交互

### 独立按键

| MCU 引脚 | 工程信号 | 方向 | 软件含义 | 有效电平 |
|---|---|---|---|---|
| `PA18` | `S2` | 输入 | UP/上键 | 软件按高电平为按下处理；工程未配置内部上下拉，依赖板级电路 |
| `PB21` | `S3` | 输入，上拉 | DOWN/下键 | 低电平按下 |

### 五向按键

五向按键均配置为带内部上拉的数字输入，低电平表示按下。

| MCU 引脚 | 工程信号 | 软件含义 |
|---|---|---|
| `PB8` | `D3_1` | 下 |
| `PB9` | `D3_2` | 左 |
| `PB10` | `D3_3` | 右 |
| `PB11` | `D3_4` | 中键/确认 |
| `PB12` | `D3_5` | 上 |

## RGB、蜂鸣器和加热器

| MCU 引脚 | 工程信号 | 方向 | 有效电平 | 说明 |
|---|---|---|---|---|
| `PB26` | `RGB_RED` | 输出 | 高电平点亮 | RGB 红灯 |
| `PB27` | `RGB_GREEN` | 输出 | 高电平点亮 | RGB 绿灯；200 Hz 任务中还用于运行指示翻转 |
| `PB22` | `RGB_BLUE` | 输出 | 高电平点亮 | RGB 蓝灯 |
| `PA27` | `beep` | 输出 | 高电平有效 | 蜂鸣器/提示输出 |
| `PB0` | `HEATER` | 输出 | 高电平加热 | IMU 可选加热器；当前无温控版本保持低电平 |

## ADC 电池电压

| MCU 引脚 | ADC 功能 | 方向 | 说明 |
|---|---|---|---|
| `PA26` | `ADC0 / ADC12 channel 1` | 模拟输入 | 电池电压采样，3.3 V 参考，外部必须先分压 |

禁止将电池原始电压直接接入 `PA26`。

## 时钟与调试引脚

| MCU 引脚 | 功能 | 方向 | 说明 |
|---|---|---|---|
| `PA5` | `HFXIN` | 模拟/时钟输入 | 40 MHz 外部高频晶振输入 |
| `PA6` | `HFXOUT` | 模拟/时钟输出 | 40 MHz 外部高频晶振输出 |
| `PA19` | `SWDIO` | 双向 | SWD 下载与调试数据 |
| `PA20` | `SWCLK` | 输入 | SWD 下载与调试时钟 |

工程系统主频为 80 MHz。`PA5/PA6` 和 `PA19/PA20` 不应复用为普通业务 GPIO。

## 无外部引脚的内部定时器

| 外设 | 周期 | 频率 | 主要任务 |
|---|---:|---:|---|
| `TIMG12` | 1 ms | 1000 Hz | `duty_1000hz()` |
| `TIMG0` | 5 ms | 200 Hz | `maple_duty_200hz()`，包括 IMU/AHRS 和主要控制任务 |
| `TIMG6` | 10 ms | 100 Hz | `duty_100hz()` |
| `TIMG8` | 100 ms | 10 Hz | `duty_10hz()` |
| `SysTick` | 1 ms | 1000 Hz | 系统毫秒时间基准 |

这些定时器仅在芯片内部工作，不对应外部接线。

## 按 MCU 端口汇总

### GPIOA

| 引脚 | 当前定义 | 引脚 | 当前定义 |
|---|---|---|---|
| `PA0` | 灰度 P4 | `PA16` | 灰度 P11 |
| `PA1` | 灰度 P3 | `PA17` | LCD SCL |
| `PA2` | 前轮转向舵机 PWM | `PA18` | S2 上键 |
| `PA3` | 左电机 PWM/INB1 | `PA19` | SWDIO |
| `PA4` | 右电机 PWM/INA1 | `PA20` | SWCLK |
| `PA5` | 40 MHz HFXIN | `PA21` | 蓝牙 UART2 TX |
| `PA6` | 40 MHz HFXOUT | `PA22` | 蓝牙 UART2 RX |
| `PA7` | 右电机 PWM/INA2 | `PA23` | 预留 PWM 3 |
| `PA8` | MaixCAM Pro UART1 TX | `PA24` | 灰度 P6 |
| `PA9` | MaixCAM Pro UART1 RX | `PA25` | 灰度 P5 |
| `PA10` | 地面站 UART0 TX | `PA26` | 电池 ADC 输入 |
| `PA11` | 地面站 UART0 RX | `PA27` | 蜂鸣器 |
| `PA12` | W25Q64 SCLK | `PA28` | 灰度 P2 |
| `PA13` | W25Q64 MISO | `PA29` | ICM42688 I2C SCL |
| `PA14` | W25Q64 MOSI | `PA30` | ICM42688 I2C SDA |
| `PA15` | 外部计时器使能 GPIO（默认低） | `PA31` | 灰度 P1 |

### GPIOB

| 引脚 | 当前定义 | 引脚 | 当前定义 |
|---|---|---|---|
| `PB0` | IMU 加热器，可选/当前关闭 | `PB14` | 左电机 PWM/INB2 |
| `PB1` | 预留 PWM 2 | `PB15` | LCD SDA |
| `PB2` | UART3 总线舵机 TX | `PB16` | LCD RST |
| `PB3` | UART3 总线舵机 RX 预留 | `PB17` | LCD DC |
| `PB4` | 右编码器脉冲 | `PB18` | 灰度 P10 |
| `PB5` | 左编码器脉冲 | `PB19` | 灰度 P9 |
| `PB6` | 右编码器方向 | `PB20` | LCD CS |
| `PB7` | 左编码器方向 | `PB21` | S3 下键 |
| `PB8` | 五向键下 | `PB22` | RGB 蓝灯 |
| `PB9` | 五向键左 | `PB23` | 灰度 P8 |
| `PB10` | 五向键右 | `PB24` | 灰度 P7 |
| `PB11` | 五向键中/确认 | `PB25` | W25Q64 CS |
| `PB12` | 五向键上 | `PB26` | RGB 红灯 |
| `PB13` | 灰度 P12 | `PB27` | RGB 绿灯 |

当前 64 引脚器件封装中，工程可配置的 `PA0..PA31` 与 `PB0..PB27` 均已有用途。修改任何引脚前，应先检查复用冲突、PCB 走线和 `ncontroller.syscfg`。

## 当前未分配的可选接口

- `PPM`: `nppm.c/.h` 中保留了解码逻辑和 GPIO 中断模板，但当前 SysConfig 没有定义 `PORTA_PPM_PIN`，因此没有可直接接线的 PPM 输入脚。
- 原 MPU6050/ICM206xx: 驱动文件仍保留，但当前工程构建使用 ICM42688，原驱动不参与编译。
- IMU 加热器: `PB0` 已分配，但 `temperature_ctrl_enable` 当前为 `0`，属于硬件可选、软件关闭状态。

## 维护要求

若修改 `ncontroller.syscfg` 并重新生成代码，请同时核对：

1. `ti_msp_dl_config.h` 中的端口、引脚和外设实例。
2. `motor_control.c` 中电机 PWM 通道与左右电机的对应关系。
3. `ntimer.c` 中三路舵机 PWM 接口和 `Timer_Gate_Set()` 的 PA15 门控输出。
4. `gray_detection.c` 中 12 路灰度顺序。
5. `main.c` 顶部接线注释。
6. 本文档与 `README.md`。
