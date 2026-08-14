from __future__ import annotations

import argparse
from pathlib import Path

import torch
from ultralytics import YOLO


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Train the steel-ball one-keypoint YOLO11n-Pose model.")
    parser.add_argument("--data", type=Path, default=root / "gangzhu_pose" / "dataset.yaml")
    parser.add_argument("--model", default="yolo11n-pose.pt")
    parser.add_argument("--epochs", type=int, default=180)
    parser.add_argument("--imgsz", type=int, default=320)
    parser.add_argument("--batch", type=int, default=32)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--device", default="0")
    parser.add_argument("--name", default="yolo11n_pose_320")
    parser.add_argument("--smoke", action="store_true", help="Run one epoch on a small subset.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    root = Path(__file__).resolve().parent
    if str(args.device) != "cpu" and not torch.cuda.is_available():
        raise RuntimeError("CUDA is unavailable. Run check_environment.py before training.")
    if not args.data.is_file():
        raise FileNotFoundError(f"Dataset config not found: {args.data}. Run prepare_pose_dataset.py first.")

    model = YOLO(args.model)
    smoke_suffix = "_smoke" if args.smoke else ""
    model.train(
        data=str(args.data),
        imgsz=args.imgsz,
        epochs=1 if args.smoke else args.epochs,
        batch=min(args.batch, 8) if args.smoke else args.batch,
        workers=args.workers,
        device=args.device,
        project=str(root / "runs" / "pose"),
        name=args.name + smoke_suffix,
        exist_ok=args.smoke,
        fraction=0.12 if args.smoke else 1.0,
        pretrained=True,
        optimizer="auto",
        patience=35,
        seed=42,
        deterministic=True,
        amp=True,
        cache=False,
        plots=True,
        save=True,
        save_period=10,
        close_mosaic=15,
        hsv_h=0.015,
        hsv_s=0.45,
        hsv_v=0.35,
        degrees=5.0,
        translate=0.12,
        scale=0.35,
        shear=0.0,
        perspective=0.0,
        flipud=0.0,
        fliplr=0.5,
        mosaic=0.5,
        mixup=0.0,
    )


if __name__ == "__main__":
    main()

