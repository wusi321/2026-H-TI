# Optional YOLO assistance

YOLO is not required for the application to start. Traditional vision measures
the ball on every frame. With `ENABLE_YOLO_REACQUIRE = True`, a model found in
one of the configured paths runs as a low-rate identity heartbeat.

Copy a compatible MaixCAM YOLO11-Pose model to one of the paths in `config.py`.
The model should provide the steel-ball center keypoint and must include
empty-groove, scale-mark, reflection and motion-blur negative samples.

YOLO is called every six frames, after a miss, and immediately for a far
candidate. Agreement verifies the traditional center. A disagreement triggers
a full-groove traditional cross-check and temporal confirmation; a single YOLO
result never resets the tracker by itself.
