from __future__ import annotations

import argparse
from pathlib import Path

from ultralytics import YOLO


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Export YOLO11n-Pose for the MaixCAM converter.")
    parser.add_argument(
        "--model",
        type=Path,
        default=root / "results" / "weights" / "best.pt",
    )
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=320)
    parser.add_argument("--device", default="cpu")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.model.is_file():
        raise FileNotFoundError(args.model)
    model = YOLO(str(args.model))
    output = model.export(
        format="onnx",
        imgsz=[args.height, args.width],
        batch=1,
        dynamic=False,
        simplify=True,
        opset=17,
        device=args.device,
    )
    print("onnx:", output)


if __name__ == "__main__":
    main()
