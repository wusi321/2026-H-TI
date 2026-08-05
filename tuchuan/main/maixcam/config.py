import os


APP_DIR = os.path.dirname(os.path.abspath(__file__))
VERSION = "2026-08-04-video-stream-only"

# Native WebRTC video stream. The visual recognition pipeline is intentionally
# disabled in this build, so this is the only camera channel.
ENABLE_WEBRTC = True
WEBRTC_STREAM_WIDTH = 320
WEBRTC_STREAM_HEIGHT = 180
WEBRTC_STREAM_FPS = 15
WEBRTC_STREAM_BUFFERS = 3
WEBRTC_BITRATE = 600_000
WEBRTC_GOP = 30

# None keeps the camera in auto mode.
MANUAL_EXPOSURE_US = None
MANUAL_GAIN = None
