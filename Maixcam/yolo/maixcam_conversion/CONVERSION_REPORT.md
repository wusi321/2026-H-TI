# Conversion report

- Source: `best.onnx`, YOLO11n-Pose, input `1x3x320x320`
- Source SHA-256: `f2e1b56616f9e89410537ccb4d65eb97e38c88bfcb418fa45ebbabbb4f93d2f7`
- TPU-MLIR: `1.28.1-20260429`
- Processor: `cv181x` (MaixCAM/MaixCAM Pro)
- Quantization: INT8 input and weights, FP32 exported outputs
- Calibration: 200 real images, deterministic seed `20260726`
- Calibration groups: 140 `ball_positive`, 40 `target_positive`, 20 `ball_negative`
- Build time: `2026-07-26 20:37:46`
- CVI model size: 2,954,304 bytes
- Required ION memory reported by `model_tool.py`: 3.63 MB

## Outputs

```text
/model.23/dfl/conv/Conv_output_0  [1,1,4,2100]
/model.23/Sigmoid_output_0        [1,1,2100]
/model.23/Concat_output_0         [1,3,2100]
```

## Deployment hashes

```text
gangzhu_yolo11n_pose_320_int8.cvimodel
18a8c2011dd7a57b6418acc8244a24832cb153500481e1466dd362575a5102ca

gangzhu_yolo11n_pose_320.mud
747575c87ab202a853f28d288467ceee96fd7f18d2bd806d3031a36d6d8ab2b8
```

TPU-MLIR passed ONNX-to-MLIR comparison and final INT8 deployment reference
comparison with the official tolerances `0.99,0.99` and `0.9,0.6`.
