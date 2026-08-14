from __future__ import annotations

import argparse
import json
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path


CLASS_NAME = "gangzhu"


def read_split(path: Path) -> list[str]:
    names = [line.strip() for line in path.read_text(encoding="utf-8").splitlines()]
    return [name for name in names if name]


def load_annotations(annotation_dir: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for xml_path in sorted(annotation_dir.glob("*.xml")):
        root = ET.parse(xml_path).getroot()
        filename = (root.findtext("filename") or "").strip()
        if not filename:
            raise ValueError(f"Missing filename in {xml_path}")
        if filename in result:
            raise ValueError(f"Duplicate annotation for {filename}")
        result[filename] = xml_path
    return result


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(value, high))


def convert_xml(xml_path: Path) -> tuple[list[str], int, int]:
    root = ET.parse(xml_path).getroot()
    width = int(root.findtext("size/width", "0"))
    height = int(root.findtext("size/height", "0"))
    if width <= 0 or height <= 0:
        raise ValueError(f"Invalid image size in {xml_path}: {width}x{height}")

    labels: list[str] = []
    skipped = 0
    for obj in root.findall("object"):
        if (obj.findtext("name") or "").strip() != CLASS_NAME:
            skipped += 1
            continue
        box = obj.find("bndbox")
        if box is None:
            skipped += 1
            continue

        xmin = clamp(float(box.findtext("xmin", "0")), 0.0, float(width))
        ymin = clamp(float(box.findtext("ymin", "0")), 0.0, float(height))
        xmax = clamp(float(box.findtext("xmax", "0")), 0.0, float(width))
        ymax = clamp(float(box.findtext("ymax", "0")), 0.0, float(height))
        box_width = xmax - xmin
        box_height = ymax - ymin
        if box_width <= 0 or box_height <= 0:
            skipped += 1
            continue

        center_x = (xmin + xmax) * 0.5 / width
        center_y = (ymin + ymax) * 0.5 / height
        norm_width = box_width / width
        norm_height = box_height / height

        # The MaixHub VOC export has boxes only. The initial keypoint is the
        # tight-box center and should be reviewed where boxes are asymmetric.
        values = (
            0,
            center_x,
            center_y,
            norm_width,
            norm_height,
            center_x,
            center_y,
            2,
        )
        labels.append(
            f"{values[0]} " + " ".join(f"{value:.8f}" for value in values[1:7]) + f" {values[7]}"
        )

    return labels, skipped, width * height


def write_dataset_yaml(destination: Path) -> None:
    dataset_path = destination.resolve().as_posix()
    content = f"""path: {dataset_path}
train: images/train
val: images/val

names:
  0: {CLASS_NAME}

kpt_shape: [1, 3]
flip_idx: [0]
"""
    (destination / "dataset.yaml").write_text(content, encoding="utf-8")


def prepare(source: Path, destination: Path) -> dict[str, object]:
    source = source.resolve()
    destination = destination.resolve()
    image_dir = source / "images"
    annotation_dir = source / "annotations"
    if not image_dir.is_dir() or not annotation_dir.is_dir():
        raise FileNotFoundError(f"Expected images/ and annotations/ under {source}")

    annotations = load_annotations(annotation_dir)
    report: dict[str, object] = {
        "source": str(source),
        "destination": str(destination),
        "keypoint_source": "voc_bounding_box_center",
        "splits": {},
        "warnings": [],
    }
    seen: set[str] = set()

    for split in ("train", "val"):
        names = read_split(source / f"{split}.txt")
        out_images = destination / "images" / split
        out_labels = destination / "labels" / split
        out_images.mkdir(parents=True, exist_ok=True)
        out_labels.mkdir(parents=True, exist_ok=True)

        image_count = 0
        positive_images = 0
        negative_images = 0
        object_count = 0
        skipped_objects = 0
        for name in names:
            if name in seen:
                raise ValueError(f"Image appears in multiple splits: {name}")
            seen.add(name)
            source_image = image_dir / name
            if not source_image.is_file():
                raise FileNotFoundError(f"Split references missing image: {source_image}")

            shutil.copy2(source_image, out_images / name)
            xml_path = annotations.get(name)
            if xml_path is None:
                labels: list[str] = []
                skipped = 0
                negative_images += 1
            else:
                labels, skipped, _ = convert_xml(xml_path)
                if labels:
                    positive_images += 1
                else:
                    negative_images += 1
            skipped_objects += skipped
            object_count += len(labels)
            (out_labels / f"{Path(name).stem}.txt").write_text(
                "\n".join(labels) + ("\n" if labels else ""), encoding="utf-8"
            )
            image_count += 1

        report["splits"][split] = {
            "images": image_count,
            "positive_images": positive_images,
            "negative_images": negative_images,
            "objects": object_count,
            "skipped_objects": skipped_objects,
        }

    source_images = {path.name for path in image_dir.iterdir() if path.is_file()}
    if source_images != seen:
        missing_from_splits = sorted(source_images - seen)
        missing_from_disk = sorted(seen - source_images)
        report["warnings"].append(
            {
                "missing_from_splits": missing_from_splits,
                "missing_from_disk": missing_from_disk,
            }
        )

    write_dataset_yaml(destination)
    (destination / "conversion_report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    return report


def parse_args() -> argparse.Namespace:
    project_root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Convert MaixHub VOC boxes to one-keypoint YOLO pose labels.")
    parser.add_argument("--source", type=Path, default=project_root / "gangzhu")
    parser.add_argument("--destination", type=Path, default=project_root / "gangzhu_pose")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    report = prepare(args.source, args.destination)
    print(json.dumps(report, ensure_ascii=False, indent=2))
    print("WARNING: keypoints were initialized from VOC box centers; review labels before final training.")


if __name__ == "__main__":
    main()

