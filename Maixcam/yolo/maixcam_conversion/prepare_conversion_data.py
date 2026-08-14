from __future__ import annotations

import argparse
import csv
import hashlib
import random
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATASET = REPO_ROOT / "datasets" / "gangzhu_pose_v2"
DEFAULT_MODEL = REPO_ROOT / "results" / "weights" / "best.onnx"
DEFAULT_OUTPUT = Path(__file__).resolve().parent
SEED = 20260726
GROUP_TARGETS = {
    "positive": 180,
    "negative": 20,
}


@dataclass(frozen=True)
class Sample:
    image: Path
    label: Path
    split: str
    group: str
    object_count: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Prepare a deterministic MaixCAM INT8 calibration set."
    )
    parser.add_argument("--dataset", type=Path, default=DEFAULT_DATASET)
    parser.add_argument("--model", type=Path, default=DEFAULT_MODEL)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def label_for(image: Path) -> Path:
    parts = list(image.parts)
    image_index = parts.index("images")
    parts[image_index] = "labels"
    return Path(*parts).with_suffix(".txt")


def classify(image: Path, object_count: int) -> Optional[str]:
    return "positive" if object_count else "negative"


def collect_samples(dataset: Path) -> list[Sample]:
    samples: list[Sample] = []
    image_root = dataset / "images"
    for image in sorted(image_root.rglob("*.jpg")):
        label = label_for(image)
        if not label.exists():
            raise FileNotFoundError(f"Missing label for {image}: {label}")
        lines = [line for line in label.read_text(encoding="utf-8").splitlines() if line.strip()]
        group = classify(image, len(lines))
        if group is None:
            continue
        samples.append(
            Sample(
                image=image,
                label=label,
                split=image.parent.name,
                group=group,
                object_count=len(lines),
            )
        )
    return samples


def select_samples(samples: list[Sample]) -> list[Sample]:
    rng = random.Random(SEED)
    selected: list[Sample] = []
    for group, count in GROUP_TARGETS.items():
        candidates = [sample for sample in samples if sample.group == group]
        if len(candidates) < count:
            raise RuntimeError(
                f"Not enough {group} samples: found {len(candidates)}, need {count}"
            )
        selected.extend(rng.sample(candidates, count))
    rng.shuffle(selected)
    return selected


def prepare_output(output: Path, model: Path, selected: list[Sample]) -> None:
    images_dir = output / "images"
    images_dir.mkdir(parents=True, exist_ok=True)
    for old_image in images_dir.iterdir():
        if old_image.is_file():
            old_image.unlink()

    manifest_rows = []
    for index, sample in enumerate(selected):
        target_name = f"{index:03d}_{sample.group}_{sample.split}_{sample.image.name}"
        target = images_dir / target_name
        shutil.copy2(sample.image, target)
        try:
            source = sample.image.resolve().relative_to(REPO_ROOT.resolve())
        except ValueError:
            source = sample.image
        manifest_rows.append(
            {
                "file": target.name,
                "group": sample.group,
                "split": sample.split,
                "object_count": sample.object_count,
                "source": source.as_posix(),
            }
        )

    with (output / "calibration_manifest.csv").open(
        "w", encoding="utf-8", newline=""
    ) as file:
        writer = csv.DictWriter(file, fieldnames=manifest_rows[0].keys())
        writer.writeheader()
        writer.writerows(manifest_rows)

    test_sample = next(sample for sample in selected if sample.group == "positive")
    shutil.copy2(test_sample.image, output / "test.jpg")
    shutil.copy2(model, output / "model.onnx")


def main() -> None:
    args = parse_args()
    if not args.dataset.is_dir():
        raise FileNotFoundError(f"Dataset not found: {args.dataset}")
    if not args.model.is_file():
        raise FileNotFoundError(f"ONNX model not found: {args.model}")

    samples = collect_samples(args.dataset)
    selected = select_samples(samples)
    prepare_output(args.output, args.model, selected)

    print(f"output: {args.output}")
    print(f"calibration images: {len(selected)}")
    for group in GROUP_TARGETS:
        count = sum(sample.group == group for sample in selected)
        print(f"  {group}: {count}")
    print(f"model sha256: {sha256(args.output / 'model.onnx')}")


if __name__ == "__main__":
    main()
