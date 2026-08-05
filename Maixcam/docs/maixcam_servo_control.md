# Legacy MaixCAM bench servo setup

This path is retained for isolated actuator diagnostics only. The H-task
vehicle uses the MCU architecture in `mcu/task_controller.c`; keep
`ENABLE_SERVO_CONTROL = False` during normal operation and follow
`docs/mcu_control.md`.

In this bench-only mode, MaixCAM owns the complete loop:

```text
camera -> ball x/v -> BalanceController -> rod tilt -> servo output
```

Servo output is disabled by default. Do not enable it with placeholder values.

## Required measurements

Record these values with the linkage assembled and the rod unloaded first:

| Setting | Measurement |
|---|---|
| `SERVO_PWM_PIN` | Selected PWM-capable output pin |
| `SERVO_CENTER_DUTY` | Duty that makes the rod level |
| `SERVO_MIN_DUTY` | Safe mechanical lower limit |
| `SERVO_MAX_DUTY` | Safe mechanical upper limit |
| `SERVO_DUTY_PER_ROD_DEG` | Duty change divided by measured rod-angle change |
| `SERVO_DIRECTION` | `+1` or `-1` so positive tilt matches the configured axis |

Use a separate regulated servo supply and connect its ground to MaixCAM ground.
Do not power a loaded servo directly from the MaixCAM 3.3 V pin.

## Direct PWM

MaixPy uses `pinmap` and `pwm.PWM(..., freq=50)`. MaixCAM Pro exposes PWM4..7
on A16..A19, but the official documentation warns that PWM4..9 shares resources
with Wi-Fi. A16/A17 are also used by MaixVision UART0 during debugging.

For an offline final application, choose a verified free PWM pin. For hotspot
video or wireless recording, use an I2C PWM driver on A15/A27 instead. In both
cases MaixCAM still computes the servo command.

## Bring-up order

1. Keep `ENABLE_SERVO_CONTROL = False` and complete camera calibration.
2. Test only neutral duty with the steel ball removed.
3. Measure safe duty limits before attaching the full linkage load.
4. Set `CONTROL_MAX_TILT_DEG` to no more than 2 to 3 degrees initially.
5. Verify `CONTROL_DIRECTION` and `SERVO_DIRECTION` with a small command.
6. Start with the velocity term enabled and low proportional gain.
7. Enable control with the ball near the center and an emergency power switch.

On invalid vision, the controller slews toward neutral. The slew limit prevents
a single frame from commanding an abrupt mechanical movement.
