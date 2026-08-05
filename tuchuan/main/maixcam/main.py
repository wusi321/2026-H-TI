from maix import app, camera, image, time

import config
from web_stream import native_webrtc_available, start_native_webrtc


def apply_camera_settings(cam):
    if config.MANUAL_EXPOSURE_US is not None:
        cam.exposure(int(config.MANUAL_EXPOSURE_US))
        print("manual exposure us:", config.MANUAL_EXPOSURE_US)
    if config.MANUAL_GAIN is not None:
        cam.gain(int(config.MANUAL_GAIN))
        print("manual gain:", config.MANUAL_GAIN)


def create_webrtc_camera():
    if not config.ENABLE_WEBRTC:
        raise RuntimeError("ENABLE_WEBRTC must be True for the video-only app")
    if not native_webrtc_available():
        raise RuntimeError(
            "native WebRTC is unavailable in this MaixPy firmware. "
            "Update MaixPy or use the old RTSP recorder path."
        )

    stream_cam = camera.Camera(
        config.WEBRTC_STREAM_WIDTH,
        config.WEBRTC_STREAM_HEIGHT,
        image.Format.FMT_YVU420SP,
        fps=config.WEBRTC_STREAM_FPS,
        buff_num=config.WEBRTC_STREAM_BUFFERS,
    )
    apply_camera_settings(stream_cam)
    stream_server = start_native_webrtc(
        stream_cam,
        config.WEBRTC_BITRATE,
        config.WEBRTC_GOP,
    )
    print(
        "video stream only: WebRTC {}x{}@{} bitrate:{} gop:{}".format(
            config.WEBRTC_STREAM_WIDTH,
            config.WEBRTC_STREAM_HEIGHT,
            config.WEBRTC_STREAM_FPS,
            config.WEBRTC_BITRATE,
            config.WEBRTC_GOP,
        )
    )
    return stream_cam, stream_server


def main():
    print("maixcam video stream version:", config.VERSION)
    print("recognition, UART, YOLO, tracker, display and fill light are not used")

    stream_cam = None
    stream_server = None

    try:
        stream_cam, stream_server = create_webrtc_camera()
        while not app.need_exit():
            time.sleep_ms(500)
    finally:
        if stream_server is not None:
            try:
                stream_server.stop()
            except Exception:
                pass
        if stream_cam is not None:
            try:
                stream_cam.close()
            except Exception:
                pass
        print("maixcam video stream stopped")


if __name__ == "__main__":
    main()
