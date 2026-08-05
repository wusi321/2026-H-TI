# MCU files

`vision_protocol.h` and `vision_protocol.c` implement the 20-byte MaixCAM ball
measurement parser and CRC16-CCITT check. Feed every received UART byte to
`ball_vision_parser_push()` and only use packets for which it returns `true`.
The parser decodes little-endian fields explicitly and retains a possible frame
header after a damaged or truncated packet.

`vision_link.h` and `vision_link.c` are the preferred controller-facing API.
They reject duplicate or out-of-order sequence numbers and update a
`ball_observation_t` with the MCU's local receive time. Call
`ball_vision_link_reset()` when the UART peer is deliberately reset or
reinitialized so a restarted MaixCAM sequence can be accepted.

Controller integration and failsafe requirements are documented in
`docs/mcu_control.md`.

`task_controller.h` and `task_controller.c` implement the portable task 2-6
state machine derived from the K230 reference. The module outputs a signed
millimeter target and a route mode; board-specific line following, servo PWM
and angle feedback remain in the MCU application. See `docs/task_execution.md`.
Call `ball_task_stop()` for cancel, emergency stop and end-of-test handling.

`tests/test_vision_protocol.c` is a host-side C test for the golden wire packet,
deleted-byte resynchronization and duplicate sequence rejection.
