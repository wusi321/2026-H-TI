from __future__ import annotations

import argparse
import csv
import json
import math
import random
import re
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path

import cv2
import numpy as np


CLASS_NAME = "gangzhu"
FRAME_PATTERN = re.compile(r"_f(\d+)_t(\d+)")
TEST_CHUNKS = {3, 11}
VAL_CHUNKS = {5, 9}
CHUNK_FRAMES = 300
GUARD_FRAMES = 30
NEGATIVE_TRAIN_MAX_FRAME = 430
NEGATIVE_VAL_MIN_FRAME = 470


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build the final-scene one-keypoint steel-ball Pose V2 dataset."
    )
    parser.add_argument("--positive-root", type=Path, required=True)
    parser.add_argument("--negative-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=20260730)
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def read_image(path: Path):
    data = np.fromfile(str(path), dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise RuntimeError("Cannot read image: {}".format(path))
    return image


def write_jpeg(path: Path, image, quality: int = 95) -> None:
    ok, encoded = cv2.imencode(
        ".jpg", image, [cv2.IMWRITE_JPEG_QUALITY, quality]
    )
    if not ok:
        raise RuntimeError("Cannot encode image: {}".format(path))
    encoded.tofile(str(path))


def parse_frame(name: str) -> int:
    match = FRAME_PATTERN.search(name)
    if match is None:
        raise ValueError("Cannot parse frame index: {}".format(name))
    return int(match.group(1))


def positive_split(frame: int) -> str | None:
    chunk = frame // CHUNK_FRAMES
    if chunk in TEST_CHUNKS:
        return "test"
    if chunk in VAL_CHUNKS:
        return "val"

    for held_chunk in TEST_CHUNKS | VAL_CHUNKS:
        start = held_chunk * CHUNK_FRAMES
        end_exclusive = (held_chunk + 1) * CHUNK_FRAMES
        if 0 < start - frame <= GUARD_FRAMES:
            return None
        if 0 < frame - (end_exclusive - 1) <= GUARD_FRAMES:
            return None
    return "train"


def negative_split(frame: int) -> str | None:
    if frame <= NEGATIVE_TRAIN_MAX_FRAME:
        return "train"
    if frame >= NEGATIVE_VAL_MIN_FRAME:
        return "val"
    return None


def convert_positive_xml(xml_path: Path) -> tuple[str, list[str]]:
    root = ET.parse(xml_path).getroot()
    filename = (root.findtext("filename") or "").strip()
    width = int(root.findtext("size/width", "0"))
    height = int(root.findtext("size/height", "0"))
    if not filename or width <= 0 or height <= 0:
        raise ValueError("Invalid VOC metadata: {}".format(xml_path))

    objects = [
        obj
        for obj in root.findall("object")
        if (obj.findtext("name") or "").strip() == CLASS_NAME
    ]
    if len(objects) != 1:
        raise ValueError(
            "Expected exactly one {} object in {}, got {}".format(
                CLASS_NAME, xml_path, len(objects)
            )
        )

    box = objects[0].find("bndbox")
    if box is None:
        raise ValueError("Missing bndbox: {}".format(xml_path))
    xmin = max(0.0, min(float(width), float(box.findtext("xmin", "0"))))
    ymin = max(0.0, min(float(height), float(box.findtext("ymin", "0"))))
    xmax = max(0.0, min(float(width), float(box.findtext("xmax", "0"))))
    ymax = max(0.0, min(float(height), float(box.findtext("ymax", "0"))))
    if xmin >= xmax or ymin >= ymax:
        raise ValueError("Invalid bndbox: {}".format(xml_path))

    center_x = 0.5 * (xmin + xmax) / width
    center_y = 0.5 * (ymin + ymax) / height
    box_width = (xmax - xmin) / width
    box_height = (ymax - ymin) / height
    values = (
        0,
        center_x,
        center_y,
        box_width,
        box_height,
        center_x,
        center_y,
        2,
    )
    label = "{} {} {}".format(
        values[0],
        " ".join("{:.8f}".format(value) for value in values[1:7]),
        values[7],
    )
    return filename, [label]


def make_soft_ellipse_mask(shape, rng: random.Random):
    height, width = shape[:2]
    mask = np.zeros((height, width), dtype=np.float32)
    center = (
        rng.randint(0, max(0, width - 1)),
        rng.randint(0, max(0, height - 1)),
    )
    axes = (
        rng.randint(max(8, width // 5), max(9, width // 2)),
        rng.randint(max(8, height // 5), max(9, height // 2)),
    )
    cv2.ellipse(mask, center, axes, rng.uniform(0.0, 180.0), 0, 360, 1.0, -1)
    sigma = max(3.0, min(width, height) * rng.uniform(0.04, 0.10))
    return cv2.GaussianBlur(mask, (0, 0), sigmaX=sigma, sigmaY=sigma)


def apply_gamma(image, gamma: float):
    table = np.array(
        [((value / 255.0) ** gamma) * 255.0 for value in range(256)],
        dtype=np.uint8,
    )
    return cv2.LUT(image, table)


def apply_motion_blur(image, rng: random.Random):
    kernel_size = rng.choice((3, 5, 7))
    kernel = np.zeros((kernel_size, kernel_size), dtype=np.float32)
    angle = math.radians(rng.uniform(-20.0, 20.0))
    radius = (kernel_size - 1) * 0.5
    center = int(radius)
    dx = int(round(math.cos(angle) * radius))
    dy = int(round(math.sin(angle) * radius))
    cv2.line(
        kernel,
        (center - dx, center - dy),
        (center + dx, center + dy),
        1.0,
        1,
    )
    total = float(kernel.sum())
    if total <= 0.0:
        kernel[center, :] = 1.0
        total = float(kernel.sum())
    kernel /= total
    return cv2.filter2D(image, -1, kernel)


def augment_image(image, seed: int) -> tuple[np.ndarray, list[str]]:
    rng = random.Random(seed)
    result = image.copy()
    operations: list[str] = []

    if rng.random() < 0.14:
        gray = cv2.cvtColor(result, cv2.COLOR_BGR2GRAY)
        result = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
        operations.append("gray3")

    if rng.random() < 0.70:
        gamma = rng.uniform(0.65, 1.45)
        result = apply_gamma(result, gamma)
        operations.append("gamma_{:.2f}".format(gamma))

    if rng.random() < 0.55:
        hsv = cv2.cvtColor(result, cv2.COLOR_BGR2HSV).astype(np.float32)
        saturation = rng.uniform(0.75, 1.20)
        value = rng.uniform(0.78, 1.28)
        hsv[:, :, 1] = np.clip(hsv[:, :, 1] * saturation, 0, 255)
        hsv[:, :, 2] = np.clip(hsv[:, :, 2] * value, 0, 255)
        result = cv2.cvtColor(hsv.astype(np.uint8), cv2.COLOR_HSV2BGR)
        operations.append("hsv")

    if rng.random() < 0.25:
        mask = make_soft_ellipse_mask(result.shape, rng)
        strength = rng.uniform(0.25, 0.55)
        factor = 1.0 - mask[:, :, None] * strength
        result = np.clip(result.astype(np.float32) * factor, 0, 255).astype(np.uint8)
        operations.append("shadow")

    if rng.random() < 0.12:
        mask = make_soft_ellipse_mask(result.shape, rng)
        strength = rng.uniform(20.0, 55.0)
        result = np.clip(
            result.astype(np.float32) + mask[:, :, None] * strength,
            0,
            255,
        ).astype(np.uint8)
        operations.append("glare")

    if rng.random() < 0.18:
        result = apply_motion_blur(result, rng)
        operations.append("motion_blur")
    elif rng.random() < 0.10:
        sigma = rng.uniform(0.4, 1.1)
        result = cv2.GaussianBlur(result, (0, 0), sigmaX=sigma, sigmaY=sigma)
        operations.append("gaussian_blur")

    if rng.random() < 0.10:
        sigma = rng.uniform(1.5, 4.0)
        noise = np.random.default_rng(seed).normal(0.0, sigma, result.shape)
        result = np.clip(result.astype(np.float32) + noise, 0, 255).astype(np.uint8)
        operations.append("noise")

    if not operations:
        result = apply_gamma(result, 0.82)
        operations.append("gamma_0.82")
    return result, operations


def write_label(path: Path, labels: list[str]) -> None:
    path.write_text("\n".join(labels) + ("\n" if labels else ""), encoding="utf-8")


def add_item(
    image_path: Path,
    labels: list[str],
    split: str,
    output: Path,
    frame: int,
    source_kind: str,
    records: list[dict[str, object]],
    seed: int,
) -> None:
    image = read_image(image_path)
    output_image = output / "images" / split / image_path.name
    output_label = output / "labels" / split / (image_path.stem + ".txt")
    shutil.copy2(image_path, output_image)
    write_label(output_label, labels)
    records.append(
        {
            "image": image_path.name,
            "split": split,
            "source": source_kind,
            "frame": frame,
            "positive": int(bool(labels)),
            "augmented": 0,
            "operations": "original",
        }
    )

    if split != "train":
        return
    augmented, operations = augment_image(image, seed)
    augmented_name = image_path.stem + "_aug.jpg"
    write_jpeg(output / "images" / split / augmented_name, augmented)
    write_label(output / "labels" / split / (Path(augmented_name).stem + ".txt"), labels)
    records.append(
        {
            "image": augmented_name,
            "split": split,
            "source": source_kind,
            "frame": frame,
            "positive": int(bool(labels)),
            "augmented": 1,
            "operations": "+".join(operations),
        }
    )


def write_yaml(output: Path) -> None:
    dataset_path = output.resolve().as_posix()
    content = """path: {path}
train: images/train
val: images/val
test: images/test

names:
  0: {class_name}

kpt_shape: [1, 3]
flip_idx: [0]
""".format(path=dataset_path, class_name=CLASS_NAME)
    (output / "dataset.yaml").write_text(content, encoding="utf-8")


def main() -> None:
    args = parse_args()
    positive_root = args.positive_root.resolve()
    negative_root = args.negative_root.resolve()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        if not args.overwrite:
            raise FileExistsError(output)
        shutil.rmtree(output)

    for split in ("train", "val", "test"):
        (output / "images" / split).mkdir(parents=True, exist_ok=True)
        (output / "labels" / split).mkdir(parents=True, exist_ok=True)

    records: list[dict[str, object]] = []
    skipped: list[dict[str, object]] = []
    positive_images = sorted((positive_root / "images").glob("*.jpg"))
    if not positive_images:
        raise FileNotFoundError("No positive images under {}".format(positive_root))
    for index, image_path in enumerate(positive_images):
        frame = parse_frame(image_path.name)
        split = positive_split(frame)
        if split is None:
            skipped.append({"image": image_path.name, "reason": "split_guard"})
            continue
        xml_path = positive_root / "annotations" / (image_path.stem + ".xml")
        filename, labels = convert_positive_xml(xml_path)
        if filename != image_path.name:
            raise ValueError("VOC filename mismatch: {}".format(xml_path))
        add_item(
            image_path,
            labels,
            split,
            output,
            frame,
            "final_scene_positive",
            records,
            args.seed + index,
        )

    negative_images = sorted((negative_root / "images").glob("*.jpg"))
    if not negative_images:
        raise FileNotFoundError("No negative images under {}".format(negative_root))
    for index, image_path in enumerate(negative_images):
        frame = parse_frame(image_path.name)
        split = negative_split(frame)
        if split is None:
            skipped.append({"image": image_path.name, "reason": "negative_split_guard"})
            continue
        xml_path = negative_root / "xml" / (image_path.stem + ".xml")
        xml_root = ET.parse(xml_path).getroot()
        if xml_root.findall("object"):
            raise ValueError("Negative XML contains object: {}".format(xml_path))
        add_item(
            image_path,
            [],
            split,
            output,
            frame,
            "final_scene_negative",
            records,
            args.seed + 100000 + index,
        )

    fields = [
        "image",
        "split",
        "source",
        "frame",
        "positive",
        "augmented",
        "operations",
    ]
    with (output / "manifest.csv").open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(records)
    with (output / "skipped.json").open("w", encoding="utf-8") as handle:
        json.dump(skipped, handle, ensure_ascii=False, indent=2)

    summary: dict[str, object] = {
        "positive_source": str(positive_root),
        "negative_source": str(negative_root),
        "test_chunks": sorted(TEST_CHUNKS),
        "val_chunks": sorted(VAL_CHUNKS),
        "chunk_frames": CHUNK_FRAMES,
        "guard_frames": GUARD_FRAMES,
        "splits": {},
        "skipped": len(skipped),
    }
    for split in ("train", "val", "test"):
        items = [record for record in records if record["split"] == split]
        summary["splits"][split] = {
            "images": len(items),
            "positive": sum(int(record["positive"]) for record in items),
            "negative": sum(1 - int(record["positive"]) for record in items),
            "augmented": sum(int(record["augmented"]) for record in items),
        }
    write_yaml(output)
    (output / "dataset_report.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    print("dataset:", output)


if __name__ == "__main__":
    main()
