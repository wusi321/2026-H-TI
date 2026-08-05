# UART2 HC-05 Telemetry

## Serial settings

- MCU peripheral: physical UART2 (`UART_2_INST` in generated code)
- TX: PA21 -> HC-05 RXD
- RX: PA22 <- HC-05 TXD
- Baud rate: 9600
- Format: 8 data bits, no parity, 1 stop bit, no flow control
- Transmit rate: 20 frames per second while the selected task is active

Transmission starts only after the useful tuning interval begins:

- Task 3: after vision has held the stationary ball for 1 second and starts the
  `0 -> +5 -> -5 cm` motion.
- Tasks 4 and 5: when the vehicle forward speed command leaves zero.
- Tasks 6 and 7: after the first key press captures the ball position, a second
  press within the 1 second selection window chooses task 6 target `-7.40 cm`
  or task 7 target `+7.20 cm`; with no second press the captured position is
  used. Telemetry starts when the vehicle forward speed command leaves zero.

Changing `sdk_work_mode` resets this start gate. No task 0/1/2/debug telemetry
is sent by this stream.

## Binary frame

Each frame is 40 bytes. Multi-byte values use little-endian byte order.

| Offset | Size | Type | Meaning |
| --- | ---: | --- | --- |
| 0 | 1 | `uint8` | Header `0xA5` |
| 1 | 1 | `uint8` | Payload length `0x24` (36 bytes) |
| 2 | 4 | `float32` | Filtered ball position, cm |
| 6 | 4 | `float32` | Filtered ball velocity, cm/s |
| 10 | 4 | `float32` | Ball direction: `-1`, `0`, `1`; `2` means vision invalid |
| 14 | 4 | `float32` | Ball target position, cm |
| 18 | 4 | `float32` | Vehicle average wheel speed, cm/s |
| 22 | 4 | `float32` | IMU yaw angle, degrees |
| 26 | 4 | `float32` | Servo angle command, degrees |
| 30 | 4 | `float32` | Servo speed command |
| 34 | 4 | `float32` | Actual contest task ID: `3`, `4`, `5`, `6`, or `7` |
| 38 | 1 | `uint8` | Sum of bytes 1 through 37, modulo 256 |
| 39 | 1 | `uint8` | Tail `0x5A` |

The ball direction deadband is 5 mm/s. Position and velocity follow the sign
defined by the MaixCAM position calibration.

The task ID is part of every sample so task 3 stationary-ball tuning cannot be
mixed with task 4/5/6/7 vehicle-motion tuning. A recorder should lock one run to
the first task ID it receives and reject a different task ID until that run is
closed.

## PC capture

Install pyserial, then list ports and start a CSV recording:

```powershell
python -m pip install pyserial
python tools/uart2_telemetry.py --list
python tools/uart2_telemetry.py --port COM7 --csv uart2_run.csv
```

The monitor validates frame boundaries and checksums. It also calculates the
target position error before printing and recording each sample.
