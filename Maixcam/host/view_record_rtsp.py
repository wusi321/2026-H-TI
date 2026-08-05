import os
import sys
import time


os.environ.setdefault("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp")

import cv2


def main():
    camera_ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.137.201"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "ball_test.mp4"
    stream_url = "rtsp://{}:8554/live".format(camera_ip)

    capture = cv2.VideoCapture(stream_url, cv2.CAP_FFMPEG)
    capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    if not capture.isOpened():
        raise RuntimeError("cannot open {}".format(stream_url))

    fps = float(capture.get(cv2.CAP_PROP_FPS))
    if fps < 5.0 or fps > 120.0:
        fps = 30.0
    writer = None
    frames = 0
    started = time.monotonic()
    print("OpenCV live view and recording. Press Q in the video window to stop.")
    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                raise RuntimeError("RTSP stream ended before recording was stopped")
            if writer is None:
                height, width = frame.shape[:2]
                writer = cv2.VideoWriter(
                    output_path,
                    cv2.VideoWriter_fourcc(*"mp4v"),
                    fps,
                    (width, height),
                )
                if not writer.isOpened():
                    raise RuntimeError("cannot create {}".format(output_path))
                print("recording {}x{} @ {:.1f} fps".format(width, height, fps))
            writer.write(frame)
            cv2.imshow("Ball Live {}".format(camera_ip), frame)
            frames += 1
            if cv2.waitKey(1) & 0xFF in (ord("q"), ord("Q")):
                break
    finally:
        capture.release()
        if writer is not None:
            writer.release()
        cv2.destroyAllWindows()
    elapsed = max(time.monotonic() - started, 0.001)
    print("saved:{} frames:{} receive_fps:{:.1f}".format(output_path, frames, frames / elapsed))


if __name__ == "__main__":
    main()
