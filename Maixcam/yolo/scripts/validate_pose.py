from __future__ import annotations

import argparse
from pathlib import Path

from ultralytics import YOLO


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Validate a trained steel-ball pose model.")
    parser.add_argument(
        "--model",
        type=Path,
        default=root / "runs" / "pose" / "yolo11n_pose_320" / "weights" / "best.pt",
    )
    parser.add_argument("--data", type=Path, default=root / "gangzhu_pose" / "dataset.yaml")
    parser.add_argument("--imgsz", type=int, default=320)
    parser.add_argument("--device", default="0")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.model.is_file():
        raise FileNotFoundError(args.model)
    model = YOLO(str(args.model))
    metrics = model.val(data=str(args.data), imgsz=args.imgsz, device=args.device, plots=True)
    print(metrics.results_dict)


if __name__ == "__main__":
    main()

