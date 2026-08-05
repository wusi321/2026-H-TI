import os


APP_DIR = os.path.dirname(os.path.abspath(__file__))
VERSION = "2026-08-01-h-ball-simple-13"

# MaixCAM onboard fill light: B3 is active high.
FILL_LIGHT_PIN = "B3"
FILL_LIGHT_FUNCTION = "GPIOB3"
FILL_LIGHT_ACTIVE_LEVEL = 1

# GC4653 uses the high-frame-rate sensor mode at or below 1280x720.
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 360
CAMERA_FPS = 60
# Single-channel mode keeps one buffer for the lowest available latency.
CAMERA_BUFFERS = 1
WEBRTC_VISION_BUFFERS = 3
WEBRTC_STREAM_BUFFERS = 3
WEBRTC_STREAM_WIDTH = 640
WEBRTC_STREAM_HEIGHT = 360
WEBRTC_STREAM_FPS = 15
WEBRTC_BITRATE = 1_000_000
WEBRTC_GOP = 30
# RTSP consumes one camera channel while vision reads another. The MaixPy
# camera FAQ and RTSP example require enough buffers plus retry-on-timeout.
RTSP_STREAM_BUFFERS = 3
RTSP_VISION_BUFFERS = 3
CAMERA_READ_BLOCK_MS = 80
CAMERA_READ_RETRY_MS = 10
CAMERA_READ_WARNING_INTERVAL_MS = 2000

# Tune these with the final diffuse light, then lock them. None keeps auto mode.
MANUAL_EXPOSURE_US = None
MANUAL_GAIN = None

# The local screen is for commissioning. Native WebRTC serves its page at
# http://<device-ip>:8000; RTSP remains a fallback for older MaixPy firmware.
ENABLE_DISPLAY = True
DISPLAY_EVERY = 2
DRAW_DEBUG = True
ENABLE_WEBRTC = True
ENABLE_RTSP = False

# Bench-only override: use the placeholder pixel-to-position map so UART can
# drive the MCU while servo direction, gain and detector boxes are tuned.
# Set this back to False after commissioning if calibration is still incomplete.
UART_ALLOW_PLACEHOLDER_CONTROL = False

# Without the bench override, placeholder calibration sends only VALID=0 packets.
# UART1 avoids the UART0 channel used by MaixVision.
ENABLE_UART = True
UART_DEVICE = "/dev/ttyS0"
UART_BAUD = 115200
UART_TX_PIN = "A16"
UART_RX_PIN = "A17"
UART_TX_FUNCTION = "UART0_TX"
UART_RX_FUNCTION = "UART0_RX"

CALIBRATION_PATH = os.path.join(APP_DIR, "calibration", "position_calibration.json")

# Detect the neutral dark shell/reflection of the steel ball. L=75 also joins
# the ball to the long dark groove edges in the first real recording; L=55
# separates those structures while retaining the ball signature.
LAB_THRESHOLDS = [[0, 55, -30, 30, -30, 30]]
# Sample only the fixed groove ROI every few frames. The startup median becomes
# the lighting reference; later global brightness shifts adjust only LAB L max.
# YOLO continues to receive the untouched RGB frame.
ADAPTIVE_L_ENABLED = True
ADAPTIVE_L_INTERVAL_FRAMES = 8
ADAPTIVE_L_WARMUP_SAMPLES = 4
ADAPTIVE_L_GAIN = 0.75
ADAPTIVE_L_SMOOTHING = 0.35
ADAPTIVE_L_MIN = 42
ADAPTIVE_L_MAX = 70
ADAPTIVE_L_BINS = 32
BLOB_X_STRIDE = 2
BLOB_Y_STRIDE = 1
BLOB_AREA_THRESHOLD = 24
BLOB_PIXELS_THRESHOLD = 24
BLOB_MERGE = False
BLOB_MARGIN = 2

# The complete 1 cm ball is about 24 px wide, but a thresholded reflective ball
# is often a 10-30 px crescent rather than a filled circle. The axis and time
# gates below reject narrow scale marks and unrelated dark regions.
BALL_MIN_DIAMETER_PX = 10
BALL_MAX_DIAMETER_PX = 38
BALL_EXPECTED_DIAMETER_PX = 24
BALL_MIN_ASPECT = 0.55
BALL_MAX_ASPECT = 1.82
# Maix find_blobs reports very low roundness for the real reflective ball.
# Roundness is therefore only a soft score reference, never a hard reject.
BALL_ROUNDNESS_REFERENCE = 0.20
BALL_MIN_DENSITY = 0.18
BALL_MAX_DENSITY = 0.98
BALL_EXPECTED_DENSITY = 0.55
BALL_DENSITY_TOLERANCE = 0.45
BALL_MAX_CENTROID_OFFSET_RATIO = 0.32
BALL_MAX_AXIS_DISTANCE_PX = 32
TRACK_ROI_PADDING_PX = 12
BALL_MIN_SCORE_SEARCH = 0.62
BALL_MIN_SCORE_TRACK = 0.52
BALL_DIAGNOSTIC_LIMIT = 8
BALL_BBOX_CENTER_WEIGHT = 0.80

# The beam coordinate is one-dimensional: left=-12.5 cm, center=0 cm,
# right=+12.5 cm. The ball center can normally travel to about +/-12 cm.
ROD_LENGTH_CM = 25.0
BALL_DIAMETER_CM = 1.0

# Position/velocity observer and dynamic measurement gate.
TRACK_ALPHA = 0.84
TRACK_BETA = 0.20
TRACK_MIN_GATE_CM = 1.0
TRACK_MAX_GATE_CM = 6.0
TRACK_MAX_SPEED_CM_S = 240.0
TRACK_REACQUIRE_AFTER_MS = 55
TRACK_VALID_HOLD_MS = 70

# A far-away candidate needs consecutive confirmation before it can reset the
# tracker. This suppresses jumps to a scale mark or reflection.
REACQUIRE_CONFIRM_FRAMES = 2
# Do not reject a real fast roll based on displacement. Shape, beam position,
# size consistency and optional YOLO verification protect reacquisition.
REACQUIRE_MAX_MOVE_CM = 12.0
REACQUIRE_MAX_DIAMETER_CHANGE_PX = 8
REACQUIRE_MAX_GAP_MS = 80

# Traditional vision remains the high-rate position source. YOLO runs as a
# low-rate identity heartbeat and increases its rate after a miss or far jump.
ENABLE_YOLO_REACQUIRE = True
YOLO_MODEL_PATHS = [
    "/root/models/gangzhu_yolo11n_pose_320.mud",
    "/root/models/gangzhu_yolo11n_pose_320_int8.mud",
    "/root/models/gangzhu_yolo11n_pose_320_int8.cvimodel",
]
YOLO_CONF_THRESHOLD = 0.60
YOLO_IOU_THRESHOLD = 0.45
YOLO_KEYPOINT_THRESHOLD = 0.45
YOLO_HEARTBEAT_INTERVAL_FRAMES = 6
YOLO_BURST_INTERVAL_FRAMES = 2
YOLO_CONFLICT_BURST_FRAMES = 2
YOLO_AGREEMENT_CM = 1.5
# A conflicting YOLO result has no traditional-vision corroboration. Require
# a stricter score than normal detection before starting temporal confirmation.
YOLO_CONFLICT_CONF_THRESHOLD = 0.70
YOLO_PRESERVE_ASPECT = True
YOLO_USE_BEAM_CROP = False
YOLO_MIN_DIAMETER_PX = 8
YOLO_MAX_DIAMETER_PX = 72
YOLO_MIN_ASPECT = 0.50
YOLO_MAX_AXIS_DISTANCE_PX = 38
YOLO_KEYPOINT_BOX_MARGIN_RATIO = 0.25

# If the narrow predicted window misses, scan the complete groove in the same
# frame. This avoids waiting for the tracker timeout when the ball accelerates.
ENABLE_SAME_FRAME_GLOBAL_FALLBACK = True

# Bench-only direct-servo path retained for diagnostics. The H-task system uses
# UART measurements and keeps the task, balance and servo loops on the MCU.
ENABLE_SERVO_CONTROL = False
CONTROL_TARGET_X_CM = 0.0
CONTROL_KP_DEG_PER_CM = 0.20
CONTROL_KD_DEG_PER_CM_S = 0.015
CONTROL_DEADBAND_CM = 0.08
CONTROL_MAX_TILT_DEG = 3.0
CONTROL_MAX_SLEW_DEG_S = 30.0
CONTROL_DIRECTION = 1.0

# Direct PWM reference only. Exposed PWM4..9 share resources with Wi-Fi on
# MaixCAM Pro, so an I2C PWM driver is preferable when hotspot video is needed.
SERVO_PWM_PIN = None
SERVO_PWM_FREQUENCY_HZ = 50
SERVO_CENTER_DUTY = None
SERVO_MIN_DUTY = None
SERVO_MAX_DUTY = None
SERVO_DUTY_PER_ROD_DEG = None
SERVO_DIRECTION = 1.0

PROFILE_EVERY_FRAMES = 120
POSITION_PRINT_EVERY_FRAMES = 20
BLOB_DIAGNOSTICS = True
