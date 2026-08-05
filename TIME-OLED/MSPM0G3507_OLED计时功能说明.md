# MSPM0G3507 OLED 计时功能说明

## 添加的功能

本工程在 TI MSPM0G3507 上实现 OLED 计时显示功能。

核心逻辑：

- PA15 为高电平时开始或继续计时。
- PA15 为低电平时暂停计时，时间保持不变。
- PB21 按键只用于清零计时。
- OLED 只显示 `TIME` 和 `STATE`，并使用放大字体方便观察。

## 当前使用的工程

当前仓库内工程：

```text
TIME-OLED\keil\test.uvprojx
```

该 Keil 工程复用仓库内 `mspm0g3507_26ti\source` 下的 TI MSPM0 SDK/DriverLib；`TIME-OLED\source` 属于本地重复拷贝，不作为仓库内容提交。

Keil 构建后生成的烧录文件：

```text
TIME-OLED\keil\Objects\test.axf
```

原始主程序已备份为：

```text
TIME-OLED\keil\main_before_stopwatch.c
```

该工程就是本仓库中的独立计时器板工程。主控工程 `../mspm0g3507_26ti` 通过 `PA15/TIMER_GATE` 输出高低电平，本工程通过 `PA15/E2B` 输入该电平并控制 OLED 计时显示。

## 引脚分配

| 功能 | MSPM0G3507 引脚 | 工程宏定义 | 说明 |
| --- | --- | --- | --- |
| 计时控制输入 | PA15 | `PORTA_E2B_PIN` | 高电平运行，低电平暂停 |
| 清零按键 | PB21 | `PORTB_KEY_PIN` | 低电平有效，按下接 GND |
| OLED SCL | PB9 | `PORTB_OLED_SCL_PIN` | OLED 时钟线 |
| OLED SDA | PB8 | `PORTB_OLED_SDA_PIN` | OLED 数据线 |
| 状态 LED | PB22 | `PORTB_LED_PIN` | 程序运行心跳 |

## 触发方式

### PA15 计时控制

PA15 使用电平控制，不再使用跳变触发。

```text
PA15 = 高电平：RUN，计时增加
PA15 = 低电平：PAUSE，计时暂停
```

推荐测试接法：

```text
外部 3.3V 信号 -> PA15
外部 GND      -> MSPM0 GND
```

必须共地，否则 PA15 可能读不到稳定高电平。

### PB21 清零按键

PB21 为低电平有效。

```text
PB21 按下接 GND：计时清零
```

清零后：

- 如果 PA15 仍为高电平，会从 `00:00.000` 继续计时。
- 如果 PA15 为低电平，会保持 `00:00.000` 暂停。

## OLED 显示

OLED 当前只显示两项：

```text
TIME
00:00.000

STATE
RUN / PAUSE
```

状态含义：

| OLED 状态 | 含义 |
| --- | --- |
| `RUN` | PA15 为高电平，正在计时 |
| `PAUSE` | PA15 为低电平，计时暂停 |

## 注意事项

1. 外部信号输入 PA15 时，外部设备必须和 MSPM0 共地。
2. PA15 是 `GPIOA.15`，不是芯片物理第 15 脚。
3. 当前工程里 PA15 对应 `E2B`，接线时请找板子上标注的 `PA15` / `A15` / `E2B`。
4. 请确认 Keil 烧录的是：

```text
TIME-OLED\keil\Objects\test.axf
```
