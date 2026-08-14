from __future__ import annotations

import argparse
import json
import os
import time
from datetime import datetime
from pathlib import Path


os.environ.setdefault("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp")

import cv2


VIDEO_SUFFIX = ".mp4"


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Preview and record the MaixCAM RTSP stream on this PC."
    )
    parser.add_argument("--camera-ip", default="192.168.137.201")
    parser.add_argument(
        "--url",
        default=None,
        help="Override the RTSP URL. The default is rtsp://CAMERA_IP:8554/live.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=root / "captures" / "videos",
    )
    parser.add_argument(
        "--name",
        default=None,
        help="Output stem without an extension. A timestamp is used by default.",
    )
    parser.add_argument(
        "--record-fps",
        type=float,
        default=None,
        help="Override the FPS reported by the RTSP stream.",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=0.0,
        help="Stop automatically after this many seconds; 0 waits for Q/Ctrl+C.",
    )
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Record without opening a preview window.",
    )
    return parser.parse_args()


def valid_fps(value: float) -> bool:
    return 5.0 <= float(value) <= 120.0


def open_capture(stream_url: str) -> cv2.VideoCapture:
    capture = cv2.VideoCapture(stream_url, cv2.CAP_FFMPEG)
    capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    if not capture.isOpened():
        capture.release()
        raise RuntimeError(
            "Cannot open {}. Enable RTSP on MaixCAM and check the hotspot IP.".format(
                stream_url
            )
        )
    return capture


def create_writer(path: Path, fps: float, width: int, height: int) -> cv2.VideoWriter:
    writer = cv2.VideoWriter(
        str(path),
        cv2.VideoWriter_fourcc(*"mp4v"),
        fps,
        (width, height),
    )
    if not writer.isOpened():
        writer.release()
        raise RuntimeError("Cannot create video: {}".format(path))
    return writer


def write_metadata(path: Path, metadata: dict[str, object]) -> None:
    metadata_path = path.with_suffix(".json")
    metadata_path.write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def main() -> None:
    args = parse_args()
    stream_url = args.url or "rtsp://{}:8554/live".format(args.camera_ip)
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    stem = args.name or "maixcam_{}".format(datetime.now().strftime("%Y%m%d_%H%M%S"))
    output_path = output_dir / (stem + VIDEO_SUFFIX)
    if output_path.exists():
        raise FileExistsError(output_path)

    capture = open_capture(stream_url)
    writer = None
    frame_count = 0
    started_wall = datetime.now().astimezone()
    started = time.monotonic()
    last_report = started
    stopped_by = "stream_end"

    print("stream:", stream_url)
    print("output:", output_path)
    if not args.headless:
        print("Press Q in the preview window to stop and finalize the MP4 file.")

    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                if frame_count == 0:
                    raise RuntimeError("The stream opened but returned no video frames")
                print("stream ended; finalizing the recorded video")
                stopped_by = "stream_end"
                break

            if writer is None:
                height, width = frame.shape[:2]
                reported_fps = float(capture.get(cv2.CAP_PROP_FPS))
                fps = float(args.record_fps) if args.record_fps else reported_fps
                if not valid_fps(fps):
                    fps = 30.0
                writer = create_writer(output_path, fps, width, height)
                print("recording:{}x{} @ {:.3f} fps".format(width, height, fps))

            writer.write(frame)
            frame_count += 1
            now = time.monotonic()

            if not args.headless:
                preview = frame.copy()
                receive_fps = frame_count / max(0.001, now - started)
                cv2.putText(
                    preview,
                    "REC frames:{} recv:{:.1f}fps".format(frame_count, receive_fps),
                    (12, 28),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.65,
                    (0, 0, 255),
                    2,
                    cv2.LINE_AA,
                )
                cv2.imshow("MaixCAM recorder", preview)
                if cv2.waitKey(1) & 0xFF in (ord("q"), ord("Q")):
                    stopped_by = "keyboard_q"
                    break

            if args.duration > 0.0 and now - started >= args.duration:
                stopped_by = "duration"
                break

            if now - last_report >= 2.0:
                print(
                    "frames:{} receive_fps:{:.1f}".format(
                        frame_count,
                        frame_count / max(0.001, now - started),
                    )
                )
                last_report = now
    except KeyboardInterrupt:
        stopped_by = "ctrl_c"
    finally:
        capture.release()
        if writer is not None:
            writer.release()
        if not args.headless:
            cv2.destroyAllWindows()

    if writer is None or frame_count == 0:
        raise RuntimeError("No video frames were recorded")

    elapsed = max(0.001, time.monotonic() - started)
    metadata = {
        "source": stream_url,
        "video": str(output_path),
        "started_at": started_wall.isoformat(),
        "elapsed_seconds": round(elapsed, 3),
        "frames": frame_count,
        "receive_fps": round(frame_count / elapsed, 3),
        "record_fps": round(fps, 3),
        "width": width,
        "height": height,
        "stopped_by": stopped_by,
    }
    write_metadata(output_path, metadata)
    print("saved:", output_path)
    print("metadata:", output_path.with_suffix(".json"))
    print("frames:{} receive_fps:{:.1f}".format(frame_count, frame_count / elapsed))


if __name__ == "__main__":
    main()
