from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

import numpy as np
from ultralytics import YOLO


def box_iou(a: np.ndarray, b: np.ndarray) -> float:
    left = max(float(a[0]), float(b[0]))
    top = max(float(a[1]), float(b[1]))
    right = min(float(a[2]), float(b[2]))
    bottom = min(float(a[3]), float(b[3]))
    intersection = max(0.0, right - left) * max(0.0, bottom - top)
    area_a = max(0.0, float(a[2] - a[0])) * max(0.0, float(a[3] - a[1]))
    area_b = max(0.0, float(b[2] - b[0])) * max(0.0, float(b[3] - b[1]))
    union = area_a + area_b - intersection
    return intersection / union if union > 0 else 0.0


def load_ground_truth(label_path: Path, width: int, height: int) -> tuple[np.ndarray, np.ndarray]:
    boxes: list[list[float]] = []
    points: list[list[float]] = []
    for line in label_path.read_text(encoding="utf-8").splitlines():
        parts = [float(value) for value in line.split()]
        if len(parts) != 8:
            raise ValueError(f"Expected 8 columns in {label_path}: {line}")
        _, cx, cy, bw, bh, kx, ky, visibility = parts
        boxes.append(
            [
                (cx - bw * 0.5) * width,
                (cy - bh * 0.5) * height,
                (cx + bw * 0.5) * width,
                (cy + bh * 0.5) * height,
            ]
        )
        points.append([kx * width, ky * height] if visibility > 0 else [math.nan, math.nan])
    return np.asarray(boxes, dtype=np.float32).reshape(-1, 4), np.asarray(points, dtype=np.float32).reshape(-1, 2)


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Measure center-point error in original-image pixels.")
    parser.add_argument(
        "--model",
        type=Path,
        default=root / "runs" / "pose" / "yolo11n_pose_320" / "weights" / "best.pt",
    )
    parser.add_argument("--dataset", type=Path, default=root / "gangzhu_pose")
    parser.add_argument("--imgsz", type=int, default=320)
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--match-iou", type=float, default=0.5)
    parser.add_argument("--device", default="0")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    image_dir = args.dataset / "images" / "val"
    label_dir = args.dataset / "labels" / "val"
    image_paths = sorted(path for path in image_dir.iterdir() if path.is_file())
    model = YOLO(str(args.model))

    errors: list[float] = []
    true_positive = 0
    false_positive = 0
    false_negative = 0
    results = model.predict(
        source=[str(path) for path in image_paths],
        imgsz=args.imgsz,
        conf=args.conf,
        iou=0.45,
        device=args.device,
        stream=True,
        verbose=False,
    )
    for image_path, result in zip(image_paths, results):
        height, width = result.orig_shape
        gt_boxes, gt_points = load_ground_truth(label_dir / f"{image_path.stem}.txt", width, height)
        pred_boxes = result.boxes.xyxy.cpu().numpy() if result.boxes is not None else np.empty((0, 4))
        if result.keypoints is not None and result.keypoints.xy.numel():
            pred_points = result.keypoints.xy[:, 0, :].cpu().numpy()
        else:
            pred_points = np.empty((0, 2))

        candidates: list[tuple[float, int, int]] = []
        for pred_index, pred_box in enumerate(pred_boxes):
            for gt_index, gt_box in enumerate(gt_boxes):
                iou = box_iou(pred_box, gt_box)
                if iou >= args.match_iou:
                    candidates.append((iou, pred_index, gt_index))
        candidates.sort(reverse=True)
        used_predictions: set[int] = set()
        used_ground_truth: set[int] = set()
        for _, pred_index, gt_index in candidates:
            if pred_index in used_predictions or gt_index in used_ground_truth:
                continue
            used_predictions.add(pred_index)
            used_ground_truth.add(gt_index)
            true_positive += 1
            if pred_index < len(pred_points) and np.isfinite(gt_points[gt_index]).all():
                errors.append(float(np.linalg.norm(pred_points[pred_index] - gt_points[gt_index])))

        false_positive += len(pred_boxes) - len(used_predictions)
        false_negative += len(gt_boxes) - len(used_ground_truth)

    error_array = np.asarray(errors, dtype=np.float32)
    precision = true_positive / max(1, true_positive + false_positive)
    recall = true_positive / max(1, true_positive + false_negative)
    report = {
        "images": len(image_paths),
        "matched_objects": true_positive,
        "false_positive": false_positive,
        "false_negative": false_negative,
        "precision": round(precision, 6),
        "recall": round(recall, 6),
        "center_error_px": {
            "mean": None if not len(error_array) else round(float(error_array.mean()), 4),
            "median": None if not len(error_array) else round(float(np.median(error_array)), 4),
            "p95": None if not len(error_array) else round(float(np.percentile(error_array, 95)), 4),
            "max": None if not len(error_array) else round(float(error_array.max()), 4),
        },
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

