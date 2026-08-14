#!/usr/bin/env bash
set -euo pipefail

WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TPU_MLIR_BIN="${TPU_MLIR_BIN:-$HOME/venvs/tpu-mlir/bin}"
MODEL_NAME="gangzhu_yolo11n_pose_320"
OUTPUT_NAMES="/model.23/dfl/conv/Conv_output_0,/model.23/Sigmoid_output_0,/model.23/Concat_output_0"

# TPU-MLIR launches helper scripts by name. Keep every helper on the same
# virtual-environment interpreter as the top-level commands.
export PATH="$TPU_MLIR_BIN:$PATH"

cd "$WORK_DIR"

"$TPU_MLIR_BIN/python" extract_onnx.py

"$TPU_MLIR_BIN/model_transform.py" \
  --model_name "$MODEL_NAME" \
  --model_def ./export.onnx \
  --input_shapes "[[1,3,320,320]]" \
  --mean "0,0,0" \
  --scale "0.00392156862745098,0.00392156862745098,0.00392156862745098" \
  --keep_aspect_ratio \
  --pixel_format rgb \
  --channel_format nchw \
  --output_names "$OUTPUT_NAMES" \
  --test_input ./test.jpg \
  --test_result "${MODEL_NAME}_top_outputs.npz" \
  --tolerance 0.99,0.99 \
  --mlir "${MODEL_NAME}.mlir"

"$TPU_MLIR_BIN/run_calibration.py" "${MODEL_NAME}.mlir" \
  --dataset ./images \
  --input_num 200 \
  -o "${MODEL_NAME}_cali_table"

"$TPU_MLIR_BIN/model_deploy.py" \
  --mlir "${MODEL_NAME}.mlir" \
  --quantize INT8 \
  --quant_input \
  --calibration_table "${MODEL_NAME}_cali_table" \
  --processor cv181x \
  --test_input "${MODEL_NAME}_in_f32.npz" \
  --test_reference "${MODEL_NAME}_top_outputs.npz" \
  --tolerance 0.9,0.6 \
  --model "${MODEL_NAME}_int8.cvimodel"

sha256sum "${MODEL_NAME}_int8.cvimodel" "${MODEL_NAME}.mud"
echo "Conversion complete: $WORK_DIR/${MODEL_NAME}_int8.cvimodel"
