from __future__ import annotations

import argparse
import csv
from datetime import datetime
from pathlib import Path

import cv2


VIDEO_EXTENSIONS = {".mp4", ".avi", ".mov", ".mkv", ".m4v"}


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Extract RGB training images from one video or a video directory."
    )
    parser.add_argument("input", type=Path, help="Video file or directory of videos.")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=root / "captures" / "frames",
    )
    parser.add_argument(
        "--every",
        type=int,
        default=30,
        help="Save one image every N decoded frames. Default: 30.",
    )
    parser.add_argument("--start-seconds", type=float, default=0.0)
    parser.add_argument(
        "--end-seconds",
        type=float,
        default=0.0,
        help="Stop at this timestamp; 0 processes to the end.",
    )
    parser.add_argument(
        "--max-images",
        type=int,
        default=0,
        help="Maximum images per video; 0 has no limit.",
    )
    parser.add_argument("--jpeg-quality", type=int, default=95)
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite generated files with the same names.",
    )
    return parser.parse_args()


def discover_videos(input_path: Path) -> list[Path]:
    input_path = input_path.resolve()
    if input_path.is_file():
        if input_path.suffix.lower() not in VIDEO_EXTENSIONS:
            raise ValueError("Unsupported video extension: {}".format(input_path))
        return [input_path]
    if not input_path.is_dir():
        raise FileNotFoundError(input_path)
    videos = sorted(
        path
        for path in input_path.iterdir()
        if path.is_file() and path.suffix.lower() in VIDEO_EXTENSIONS
    )
    if not videos:
        raise FileNotFoundError("No video files found under {}".format(input_path))
    return videos


def save_jpeg(path: Path, frame, quality: int, overwrite: bool) -> None:
    if path.exists() and not overwrite:
        raise FileExistsError(
            "{} already exists; choose another output directory or use --overwrite".format(
                path
            )
        )
    ok, encoded = cv2.imencode(
        ".jpg",
        frame,
        [cv2.IMWRITE_JPEG_QUALITY, quality],
    )
    if not ok:
        raise RuntimeError("JPEG encoding failed for {}".format(path))
    encoded.tofile(str(path))


def extract_one(
    video_path: Path,
    output_root: Path,
    every: int,
    start_seconds: float,
    end_seconds: float,
    max_images: int,
    jpeg_quality: int,
    overwrite: bool,
) -> list[dict[str, object]]:
    capture = cv2.VideoCapture(str(video_path))
    if not capture.isOpened():
        capture.release()
        raise RuntimeError("Cannot open video: {}".format(video_path))

    fps = float(capture.get(cv2.CAP_PROP_FPS))
    if fps <= 0.0:
        fps = 30.0
    start_frame = max(0, int(round(start_seconds * fps)))
    end_frame = int(round(end_seconds * fps)) if end_seconds > 0.0 else None
    capture.set(cv2.CAP_PROP_POS_FRAMES, start_frame)

    session_dir = output_root / video_path.stem
    session_dir.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []
    frame_index = start_frame
    saved = 0

    try:
        while True:
            if end_frame is not None and frame_index >= end_frame:
                break
            ok, frame = capture.read()
            if not ok:
                break
            if (frame_index - start_frame) % every == 0:
                timestamp_ms = int(round(frame_index * 1000.0 / fps))
                image_name = "{}_f{:08d}_t{:010d}.jpg".format(
                    video_path.stem,
                    frame_index,
                    timestamp_ms,
                )
                image_path = session_dir / image_name
                save_jpeg(image_path, frame, jpeg_quality, overwrite)
                height, width = frame.shape[:2]
                records.append(
                    {
                        "session": video_path.stem,
                        "source_video": str(video_path),
                        "frame_index": frame_index,
                        "timestamp_ms": timestamp_ms,
                        "fps": round(fps, 6),
                        "width": width,
                        "height": height,
                        "image": str(image_path),
                    }
                )
                saved += 1
                if max_images > 0 and saved >= max_images:
                    break
            frame_index += 1
    finally:
        capture.release()

    print(
        "video:{} fps:{:.3f} every:{} saved:{} -> {}".format(
            video_path.name,
            fps,
            every,
            saved,
            session_dir,
        )
    )
    return records


def write_manifest(path: Path, records: list[dict[str, object]]) -> None:
    fields = [
        "session",
        "source_video",
        "frame_index",
        "timestamp_ms",
        "fps",
        "width",
        "height",
        "image",
    ]
    with path.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(records)


def main() -> None:
    args = parse_args()
    if args.every <= 0:
        raise ValueError("--every must be greater than zero")
    if args.start_seconds < 0.0 or args.end_seconds < 0.0:
        raise ValueError("start/end seconds cannot be negative")
    if args.end_seconds > 0.0 and args.end_seconds <= args.start_seconds:
        raise ValueError("--end-seconds must be greater than --start-seconds")
    if not 1 <= args.jpeg_quality <= 100:
        raise ValueError("--jpeg-quality must be in 1..100")

    videos = discover_videos(args.input)
    output_root = args.output_dir.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []
    for video_path in videos:
        records.extend(
            extract_one(
                video_path,
                output_root,
                args.every,
                args.start_seconds,
                args.end_seconds,
                args.max_images,
                args.jpeg_quality,
                args.overwrite,
            )
        )

    manifest_path = output_root / "frames_manifest_{}.csv".format(
        datetime.now().strftime("%Y%m%d_%H%M%S")
    )
    write_manifest(manifest_path, records)
    print("total images:", len(records))
    print("manifest:", manifest_path)


if __name__ == "__main__":
    main()
