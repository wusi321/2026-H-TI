from __future__ import annotations

import argparse
import random
from pathlib import Path

import cv2


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Render pose-label previews for center-point review.")
    parser.add_argument("--dataset", type=Path, default=root / "gangzhu_pose")
    parser.add_argument("--split", choices=("train", "val"), default="train")
    parser.add_argument("--count", type=int, default=80)
    parser.add_argument("--seed", type=int, default=42)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    image_dir = args.dataset / "images" / args.split
    label_dir = args.dataset / "labels" / args.split
    output_dir = Path(__file__).resolve().parent / "artifacts" / f"label_preview_{args.split}"
    output_dir.mkdir(parents=True, exist_ok=True)

    image_paths = sorted(path for path in image_dir.iterdir() if path.is_file())
    random.Random(args.seed).shuffle(image_paths)
    rendered = 0
    for image_path in image_paths[: args.count]:
        image = cv2.imdecode(
            __import__("numpy").fromfile(str(image_path), dtype=__import__("numpy").uint8), cv2.IMREAD_COLOR
        )
        if image is None:
            print("skip unreadable:", image_path)
            continue
        height, width = image.shape[:2]
        label_path = label_dir / f"{image_path.stem}.txt"
        for line in label_path.read_text(encoding="utf-8").splitlines():
            parts = line.split()
            if len(parts) != 8:
                raise ValueError(f"Expected 8 columns in {label_path}: {line}")
            _, cx, cy, bw, bh, kx, ky, visibility = map(float, parts)
            left = int(round((cx - bw * 0.5) * width))
            top = int(round((cy - bh * 0.5) * height))
            right = int(round((cx + bw * 0.5) * width))
            bottom = int(round((cy + bh * 0.5) * height))
            point = (int(round(kx * width)), int(round(ky * height)))
            cv2.rectangle(image, (left, top), (right, bottom), (0, 220, 255), 1)
            if visibility > 0:
                cv2.drawMarker(image, point, (0, 0, 255), cv2.MARKER_CROSS, 11, 2)

        ok, encoded = cv2.imencode(image_path.suffix, image)
        if ok:
            encoded.tofile(str(output_dir / image_path.name))
            rendered += 1

    print(f"rendered={rendered} output={output_dir}")


if __name__ == "__main__":
    main()

