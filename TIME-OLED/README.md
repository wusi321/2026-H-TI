# 独立 OLED 计时器工程

本目录是比赛用独立 MSPM0G3507 计时器板工程。主控小车板不直接在本板上运行控制逻辑，只通过一根 GPIO 线给出计时启停电平；本工程在另一块 M0 板上读取该电平，并在 OLED 上显示计时结果。

本工程复用仓库内主控工程的 TI MSPM0 SDK/DriverLib，Keil 工程中的 SDK 路径指向 `../mspm0g3507_26ti/source`。本地如果存在 `TIME-OLED/source/`，它只是复制工程时带来的重复 SDK，不需要提交。

## 与主控的连接关系

| 信号 | 主控工程 `mspm0g3507_26ti` | 计时器工程 `TIME-OLED` | 含义 |
| --- | --- | --- | --- |
| 计时门控 | `PA15 / TIMER_GATE` 输出 | `PA15 / E2B` 输入 | 高电平运行，低电平暂停/停止 |
| 地 | GND | GND | 必须共地 |

主控侧由 `driver/ntimer.c` 中的 `Timer_Gate_Set()` 控制 `PA15`。任务状态机在开始计时时输出高电平，到达结束条件后拉低。工作模式 `28` 可强制输出高电平用于诊断。

计时器侧由 `keil/main.c` 读取 `PORTA_E2B_PIN`。当前使用下拉输入，未接线或主控低电平时保持 `PAUSE`。

## 计时器板引脚

| 功能 | 引脚 | 工程宏 | 说明 |
| --- | --- | --- | --- |
| 计时控制输入 | `PA15` | `PORTA_E2B_PIN` | 高电平计时，低电平暂停 |
| 清零按键 | `PB21` | `PORTB_KEY_PIN` | 低电平有效 |
| OLED SCL | `PB9` | `PORTB_OLED_SCL_PIN` | OLED 时钟 |
| OLED SDA | `PB8` | `PORTB_OLED_SDA_PIN` | OLED 数据 |
| 状态 LED | `PB22` | `PORTB_LED_PIN` | 运行心跳 |

## 运行逻辑

- `PA15 = 1`：状态显示 `RUN`，毫秒计时累加。
- `PA15 = 0`：状态显示 `PAUSE`，时间保持不变。
- 按下 `PB21`：清零到 `00:00.000`；如果 PA15 仍为高电平，会从零继续计时。

## 工程入口

Keil 打开：

```text
keil/test.uvprojx
```

主程序：

```text
keil/main.c
```

OLED、按键、计时和底层外设驱动：

```text
ndrivers/
```

历史主程序备份：

```text
keil/main_before_stopwatch.c
```

更详细的功能说明见 `MSPM0G3507_OLED计时功能说明.md`。
