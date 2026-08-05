# MCU control and MaixCAM telemetry

## Responsibilities

The H-task architecture keeps vision on MaixCAM and all deterministic control
on the MCU. MaixCAM must not drive servo PWM in this configuration. YOLO timing
and camera exposure can vary from frame to frame, while the MCU keeps a fixed
local control period when vision is late or temporarily invalid.

Connection:

```text
MaixCAM Pro A19/TX -> MCU RX
MaixCAM Pro GND    -> MCU GND
115200, 8-N-1
```

The fixed 20-byte little-endian packet is defined in `mcu/vision_protocol.h`.
Important fields are:

| Field | Unit | Meaning |
|---|---:|---|
| `position_mm` | mm | Ball position relative to physical O |
| `velocity_mm_s` | mm/s | Ball velocity along the positive beam axis |
| `confidence_milli` | 0..1000 | Visual confidence |
| `processing_us` | us | Current-frame processing time |
| `timestamp_ms` | ms | MaixCAM uptime timestamp |

The signed one-dimensional coordinate is defined over the 25 cm rod:

```text
left endpoint = -12.5 cm
rod center    =   0.0 cm
right endpoint= +12.5 cm
```

Because the ball diameter is about 1 cm, its center normally remains within
approximately `-12.0 cm` to `+12.0 cm`.

Flags:

- `VALID`: position may be used by the controller;
- `MEASURED`: a ball was detected in the current frame;
- `TRACKED`: current output is short-term prediction, not a new measurement.

`MEASURED` and `TRACKED` are mutually exclusive and are only set together with
`VALID`. An invalid packet uses `position_mm=32767` and `velocity_mm_s=0`; the
`vision_link` adapter converts these values to a false observation with zeroed
position and velocity.

The MCU must use its local receive timestamp for communication timeout. Do not
compare independent MCU and MaixCAM uptime clocks directly.

## Position controller

Use position and velocity to generate a small beam-angle target:

```text
error_mm = target_mm - position_mm
theta_ref = clamp(Kp * error_mm - Kd * velocity_mm_s, -theta_max, +theta_max)
```

The sign must be confirmed on the real mechanism. The MCU is the only active
position controller in the H-task application; keep MaixCAM
`ENABLE_SERVO_CONTROL = False`.

The velocity term is essential. Returning the beam to level only after the ball
reaches the target does not stop a smooth steel ball, so it will overshoot.

## Safety

- Reject bad CRC and repeated sequence numbers.
- If no valid packet arrives for 100 ms, stop position control and command a
  safe beam angle.
- Limit beam angle, angular velocity and actuator command.
- Do not integrate position error while visual data is invalid.
- Keep infrared vehicle line following independent from camera processing.

Use `ball_vision_link_push()` for the receive path so rejected duplicate or
out-of-order packets do not refresh `received_ms`. If MaixCAM or its UART is
explicitly restarted, call `ball_vision_link_reset()` once as part of
that restart handling.

After static control works, vehicle IMU acceleration along the beam may be
added as feedforward. It is an optimization, not a prerequisite for the first
closed-loop test.
