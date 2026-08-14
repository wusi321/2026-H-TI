from __future__ import annotations

import argparse
import json
from pathlib import Path

import cv2
import numpy as np
from ultralytics import YOLO


IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Evaluate steel-ball recall, center error, and negative false positives."
    )
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--dataset", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--device", default="0")
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument(
        "--batch",
        type=int,
        default=0,
        help="Inference batch size; 0 selects 1 for fixed ONNX and 32 otherwise.",
    )
    return parser.parse_args()


def read_label(path: Path) -> list[list[float]]:
    if not path.is_file():
        raise FileNotFoundError(path)
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            rows.append([float(value) for value in line.split()])
    return rows


def box_iou(first, second) -> float:
    x1 = max(first[0], second[0])
    y1 = max(first[1], second[1])
    x2 = min(first[2], second[2])
    y2 = min(first[3], second[3])
    intersection = max(0.0, x2 - x1) * max(0.0, y2 - y1)
    first_area = max(0.0, first[2] - first[0]) * max(0.0, first[3] - first[1])
    second_area = max(0.0, second[2] - second[0]) * max(0.0, second[3] - second[1])
    union = first_area + second_area - intersection
    return 0.0 if union <= 0.0 else intersection / union


def ground_truth(row, width: int, height: int):
    center_x = row[1] * width
    center_y = row[2] * height
    box_width = row[3] * width
    box_height = row[4] * height
    point_x = row[5] * width
    point_y = row[6] * height
    return {
        "box": [
            center_x - box_width * 0.5,
            center_y - box_height * 0.5,
            center_x + box_width * 0.5,
            center_y + box_height * 0.5,
        ],
        "point": [point_x, point_y],
        "diameter": 0.5 * (box_width + box_height),
    }


def predicted_point(result, index: int):
    if result.keypoints is not None and result.keypoints.xy.numel():
        point = result.keypoints.xy[index, 0].detach().cpu().numpy()
        if float(point[0]) > 0.0 and float(point[1]) > 0.0:
            return [float(point[0]), float(point[1])]
    box = result.boxes.xyxy[index].detach().cpu().numpy()
    return [float(0.5 * (box[0] + box[2])), float(0.5 * (box[1] + box[3]))]


def percentile(values: list[float], value: float):
    return None if not values else round(float(np.percentile(values, value)), 4)


def evaluate_split(
    model: YOLO,
    dataset: Path,
    split: str,
    conf: float,
    device: str,
    batch: int,
):
    image_dir = dataset / "images" / split
    label_dir = dataset / "labels" / split
    image_paths = sorted(
        path
        for path in image_dir.iterdir()
        if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS
    )
    positives = 0
    negatives = 0
    true_positives = 0
    false_positive_images = 0
    center_errors = []
    matched_ious = []
    confidence_values = []
    buckets = {
        "small_lt_10px": [0, 0],
        "medium_10_20px": [0, 0],
        "large_ge_20px": [0, 0],
    }

    predict_options = {
        "imgsz": 320,
        "conf": conf,
        "iou": 0.45,
        "device": device,
        "batch": batch,
        "verbose": False,
    }
    if batch == 1:
        # Fixed-batch ONNX models reject Ultralytics' all-at-once path-list input.
        results = [
            model.predict(source=str(path), **predict_options)[0] for path in image_paths
        ]
    else:
        results = model.predict(
            source=[str(path) for path in image_paths],
            **predict_options,
        )
    for image_path, result in zip(image_paths, results):
        image = cv2.imdecode(np.fromfile(str(image_path), dtype=np.uint8), cv2.IMREAD_COLOR)
        height, width = image.shape[:2]
        labels = read_label(label_dir / (image_path.stem + ".txt"))
        prediction_count = 0 if result.boxes is None else len(result.boxes)
        if not labels:
            negatives += 1
            if prediction_count:
                false_positive_images += 1
            continue

        positives += 1
        target = ground_truth(labels[0], width, height)
        if target["diameter"] < 10.0:
            bucket = "small_lt_10px"
        elif target["diameter"] < 20.0:
            bucket = "medium_10_20px"
        else:
            bucket = "large_ge_20px"
        buckets[bucket][1] += 1
        if not prediction_count:
            continue

        boxes = result.boxes.xyxy.detach().cpu().numpy()
        ious = [box_iou(target["box"], [float(value) for value in box]) for box in boxes]
        best_index = int(np.argmax(ious))
        best_iou = float(ious[best_index])
        if best_iou < 0.20:
            continue
        true_positives += 1
        buckets[bucket][0] += 1
        point = predicted_point(result, best_index)
        center_errors.append(
            float(
                np.hypot(
                    point[0] - target["point"][0],
                    point[1] - target["point"][1],
                )
            )
        )
        matched_ious.append(best_iou)
        confidence_values.append(float(result.boxes.conf[best_index]))

    return {
        "split": split,
        "confidence_threshold": conf,
        "images": len(image_paths),
        "positive_images": positives,
        "negative_images": negatives,
        "matched_positive_images": true_positives,
        "positive_recall": None if positives == 0 else round(true_positives / positives, 4),
        "false_positive_images": false_positive_images,
        "negative_false_positive_rate": (
            None if negatives == 0 else round(false_positive_images / negatives, 4)
        ),
        "center_error_px": {
            "mean": None if not center_errors else round(float(np.mean(center_errors)), 4),
            "p50": percentile(center_errors, 50),
            "p95": percentile(center_errors, 95),
            "max": None if not center_errors else round(max(center_errors), 4),
        },
        "matched_iou": {
            "mean": None if not matched_ious else round(float(np.mean(matched_ious)), 4),
            "p50": percentile(matched_ious, 50),
        },
        "matched_confidence": {
            "mean": (
                None if not confidence_values else round(float(np.mean(confidence_values)), 4)
            ),
            "p05": percentile(confidence_values, 5),
        },
        "diameter_buckets": {
            name: {
                "matched": values[0],
                "total": values[1],
                "recall": None if values[1] == 0 else round(values[0] / values[1], 4),
            }
            for name, values in buckets.items()
        },
    }


def main() -> None:
    args = parse_args()
    if not args.model.is_file():
        raise FileNotFoundError(args.model)
    if not args.dataset.is_dir():
        raise FileNotFoundError(args.dataset)
    batch = args.batch or (1 if args.model.suffix.lower() == ".onnx" else 32)
    model = YOLO(str(args.model))
    report = {
        "model": str(args.model.resolve()),
        "dataset": str(args.dataset.resolve()),
        "batch": batch,
        "evaluations": [],
    }
    for conf in sorted({args.conf, 0.60}):
        for split in ("val", "test"):
            result = evaluate_split(model, args.dataset, split, conf, args.device, batch)
            report["evaluations"].append(result)
            print(json.dumps(result, ensure_ascii=False))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print("report:", args.output)


if __name__ == "__main__":
    main()
