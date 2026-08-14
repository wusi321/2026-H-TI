from __future__ import annotations

import argparse
from pathlib import Path

import torch
from ultralytics import YOLO


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(
        description="Fine-tune the final-scene steel-ball YOLO11n-Pose V2 model."
    )
    parser.add_argument(
        "--data",
        type=Path,
        default=root / "datasets" / "gangzhu_pose_v2" / "dataset.yaml",
    )
    parser.add_argument(
        "--model",
        type=Path,
        default=root / "models" / "yolo11n-pose.pt",
    )
    parser.add_argument(
        "--project",
        type=Path,
        default=root / "results" / "runs" / "pose",
    )
    parser.add_argument("--name", default="gangzhu_yolo11n_pose_320_v2")
    parser.add_argument("--epochs", type=int, default=120)
    parser.add_argument("--batch", type=int, default=32)
    parser.add_argument("--workers", type=int, default=0)
    parser.add_argument("--device", default="0")
    parser.add_argument("--smoke", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.data.is_file():
        raise FileNotFoundError(args.data)
    if not args.model.is_file():
        raise FileNotFoundError(args.model)
    if str(args.device) != "cpu" and not torch.cuda.is_available():
        raise RuntimeError("CUDA is unavailable")

    model = YOLO(str(args.model))
    model.train(
        data=str(args.data),
        imgsz=320,
        epochs=2 if args.smoke else args.epochs,
        batch=min(args.batch, 8) if args.smoke else args.batch,
        workers=args.workers,
        device=args.device,
        project=str(args.project),
        name=args.name + ("_smoke" if args.smoke else ""),
        exist_ok=args.smoke,
        pretrained=True,
        optimizer="AdamW",
        lr0=0.0015,
        lrf=0.05,
        cos_lr=True,
        weight_decay=0.0005,
        warmup_epochs=3.0,
        patience=25,
        seed=20260730,
        deterministic=True,
        amp=True,
        cache=False,
        plots=True,
        save=True,
        save_period=10,
        close_mosaic=20,
        hsv_h=0.01,
        hsv_s=0.25,
        hsv_v=0.35,
        degrees=2.0,
        translate=0.05,
        scale=0.15,
        shear=0.0,
        perspective=0.0,
        flipud=0.0,
        fliplr=0.5,
        mosaic=0.10,
        mixup=0.0,
        copy_paste=0.0,
    )


if __name__ == "__main__":
    main()
