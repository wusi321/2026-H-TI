import argparse
import os
import sys

import serial


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAIXCAM_DIR = os.path.join(PROJECT_ROOT, "maixcam")
sys.path.insert(0, MAIXCAM_DIR)

from vision_protocol import MAGIC, PACKET_SIZE, decode_measurement


def main():
    parser = argparse.ArgumentParser(description="Monitor MaixCAM ball-position packets")
    parser.add_argument("port", help="for example COM7")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    buffer = bytearray()
    with serial.Serial(args.port, args.baud, timeout=0.1) as port:
        print("listening on {} @ {}".format(args.port, args.baud))
        while True:
            chunk = port.read(128)
            if chunk:
                buffer.extend(chunk)
            while True:
                header_index = buffer.find(MAGIC)
                if header_index < 0:
                    if len(buffer) > 1:
                        del buffer[:-1]
                    break
                if header_index:
                    del buffer[:header_index]
                if len(buffer) < PACKET_SIZE:
                    break
                packet = bytes(buffer[:PACKET_SIZE])
                try:
                    data = decode_measurement(packet)
                except ValueError:
                    del buffer[0]
                    continue
                del buffer[:PACKET_SIZE]
                print(
                    "seq:{:5d} t:{:10d} valid:{} measured:{} x:{}cm v:{}cm/s conf:{:.3f} process:{}us".format(
                        data["seq"],
                        data["timestamp_ms"],
                        data["valid"],
                        data["measured"],
                        "{:+.2f}".format(data["x_cm"]) if data["x_cm"] is not None else "--",
                        "{:+.1f}".format(data["v_cm_s"]) if data["v_cm_s"] is not None else "--",
                        data["confidence"],
                        data["processing_us"],
                    )
                )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
