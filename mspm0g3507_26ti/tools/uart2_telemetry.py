#!/usr/bin/env python3
"""Decode and record UART2 HC-05 telemetry from the MSPM0 controller."""

from __future__ import annotations

import argparse
import csv
import struct
import sys
import time
from pathlib import Path
from typing import Iterable


FRAME_HEAD = 0xA5
FRAME_TAIL = 0x5A
PAYLOAD_SIZE = 36
FRAME_SIZE = PAYLOAD_SIZE + 4
FIELDS = (
    "ball_position_cm",
    "ball_velocity_cm_s",
    "ball_direction",
    "target_position_cm",
    "vehicle_speed_cm_s",
    "yaw_deg",
    "servo_angle_deg",
    "servo_speed",
    "task_id",
)


class TelemetryParser:
    def __init__(self) -> None:
        self._buffer = bytearray()
        self.checksum_errors = 0
        self.format_errors = 0

    def feed(self, data: bytes) -> list[dict[str, float]]:
        self._buffer.extend(data)
        samples: list[dict[str, float]] = []

        while True:
            try:
                head = self._buffer.index(FRAME_HEAD)
            except ValueError:
                self._buffer.clear()
                break

            if head:
                del self._buffer[:head]
            if len(self._buffer) < 2:
                break
            if self._buffer[1] != PAYLOAD_SIZE:
                self.format_errors += 1
                del self._buffer[0]
                continue
            if len(self._buffer) < FRAME_SIZE:
                break

            frame = self._buffer[:FRAME_SIZE]
            if frame[-1] != FRAME_TAIL:
                self.format_errors += 1
                del self._buffer[0]
                continue
            expected_sum = sum(frame[1 : 2 + PAYLOAD_SIZE]) & 0xFF
            if frame[-2] != expected_sum:
                self.checksum_errors += 1
                del self._buffer[0]
                continue

            values = struct.unpack_from("<9f", frame, 2)
            task_id = round(values[8])
            if abs(values[8] - task_id) > 0.001 or task_id not in (3, 4, 5, 6, 7):
                self.format_errors += 1
                del self._buffer[:FRAME_SIZE]
                continue
            samples.append(dict(zip(FIELDS, values)))
            del self._buffer[:FRAME_SIZE]

        return samples


def build_test_frame(values: Iterable[float]) -> bytes:
    payload = struct.pack("<9f", *values)
    frame = bytearray((FRAME_HEAD, PAYLOAD_SIZE))
    frame.extend(payload)
    frame.append(sum(frame[1:]) & 0xFF)
    frame.append(FRAME_TAIL)
    return bytes(frame)


def list_ports() -> int:
    try:
        from serial.tools import list_ports as serial_list_ports
    except ImportError:
        print("pyserial is required: python -m pip install pyserial", file=sys.stderr)
        return 2

    ports = list(serial_list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 0
    for port in ports:
        print(f"{port.device}: {port.description}")
    return 0


def self_test() -> int:
    expected = (1.25, -2.5, -1.0, 0.0, 30.0, 12.5, -4.0, 800.0, 3.0)
    parser = TelemetryParser()
    samples = parser.feed(build_test_frame(expected))
    if len(samples) != 1:
        print("self-test failed: frame was not decoded", file=sys.stderr)
        return 1
    actual = tuple(samples[0][name] for name in FIELDS)
    if actual != expected:
        print(f"self-test failed: {actual!r}", file=sys.stderr)
        return 1
    print("UART2 telemetry parser self-test passed.")
    return 0


def run_capture(port: str, baud: int, csv_path: Path | None) -> int:
    try:
        import serial
    except ImportError:
        print("pyserial is required: python -m pip install pyserial", file=sys.stderr)
        return 2

    output = None
    writer = None
    if csv_path is not None:
        output = csv_path.open("w", newline="", encoding="utf-8")
        writer = csv.DictWriter(
            output,
            fieldnames=("pc_time_s", *FIELDS, "position_error_cm"),
        )
        writer.writeheader()

    parser = TelemetryParser()
    print(
        "time     ball_cm ball_cm_s dir target_cm error_cm "
        "car_cm_s yaw_deg servo_deg servo_speed task"
    )
    try:
        with serial.Serial(port, baudrate=baud, timeout=0.2) as uart:
            while True:
                for sample in parser.feed(uart.read(256)):
                    now = time.time()
                    error = (
                        sample["target_position_cm"]
                        - sample["ball_position_cm"]
                    )
                    print(
                        f"{time.strftime('%H:%M:%S')} "
                        f"{sample['ball_position_cm']:7.3f} "
                        f"{sample['ball_velocity_cm_s']:9.3f} "
                        f"{sample['ball_direction']:3.0f} "
                        f"{sample['target_position_cm']:9.3f} "
                        f"{error:8.3f} "
                        f"{sample['vehicle_speed_cm_s']:8.3f} "
                        f"{sample['yaw_deg']:7.2f} "
                        f"{sample['servo_angle_deg']:9.3f} "
                        f"{sample['servo_speed']:11.0f} "
                        f"{sample['task_id']:4.0f}",
                        flush=True,
                    )
                    if writer is not None:
                        row = {
                            "pc_time_s": f"{now:.6f}",
                            **sample,
                            "position_error_cm": error,
                        }
                        writer.writerow(row)
                        output.flush()
    except KeyboardInterrupt:
        return 0
    finally:
        if output is not None:
            output.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--csv", type=Path, help="optional output CSV path")
    parser.add_argument("--list", action="store_true", help="list serial ports")
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.self_test:
        return self_test()
    if args.list:
        return list_ports()
    if not args.port:
        print("--port is required unless --list or --self-test is used", file=sys.stderr)
        return 2
    return run_capture(args.port, args.baud, args.csv)


if __name__ == "__main__":
    raise SystemExit(main())
