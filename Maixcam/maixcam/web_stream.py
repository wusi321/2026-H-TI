try:
    from maix import webrtc
except ImportError:
    webrtc = None


def native_webrtc_available():
    return webrtc is not None


def start_native_webrtc(cam, bitrate, gop):
    if webrtc is None:
        raise RuntimeError("native WebRTC is unavailable in this MaixPy firmware")
    server = webrtc.WebRTC(
        bitrate=int(bitrate),
        gop=int(gop),
    )
    server.bind_camera(cam)
    server.start()
    print("WebRTC URLs:", server.get_urls())
    return server
