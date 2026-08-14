from __future__ import annotations

from pathlib import Path

import onnx
from onnxsim import simplify


INPUT_MODEL = Path("model.onnx")
TEMP_MODEL = Path("tmp_extract.onnx")
OUTPUT_MODEL = Path("export.onnx")
INPUT_NAMES = ["images"]
OUTPUT_NAMES = [
    "/model.23/dfl/conv/Conv_output_0",
    "/model.23/Sigmoid_output_0",
    "/model.23/Concat_output_0",
]


def tensor_shape(value_info: onnx.ValueInfoProto) -> list[int | str]:
    dims: list[int | str] = []
    for dim in value_info.type.tensor_type.shape.dim:
        dims.append(dim.dim_value if dim.dim_value else dim.dim_param)
    return dims


def main() -> None:
    if not INPUT_MODEL.is_file():
        raise FileNotFoundError(f"Input model not found: {INPUT_MODEL.resolve()}")

    source = onnx.load(INPUT_MODEL)
    graph_values = {
        item.name
        for item in [
            *source.graph.input,
            *source.graph.output,
            *source.graph.value_info,
        ]
    }
    missing = [name for name in [*INPUT_NAMES, *OUTPUT_NAMES] if name not in graph_values]
    if missing:
        raise RuntimeError(f"Required ONNX nodes are missing: {missing}")

    onnx.utils.extract_model(
        str(INPUT_MODEL), str(TEMP_MODEL), INPUT_NAMES, OUTPUT_NAMES, check_model=True
    )
    extracted = onnx.load(TEMP_MODEL)
    simplified, check_ok = simplify(extracted)
    if not check_ok:
        raise RuntimeError("onnxsim failed to verify the simplified model")
    onnx.checker.check_model(simplified)
    onnx.save(simplified, OUTPUT_MODEL)

    actual_outputs = [output.name for output in simplified.graph.output]
    if actual_outputs != OUTPUT_NAMES:
        raise RuntimeError(
            f"Unexpected output order: expected {OUTPUT_NAMES}, got {actual_outputs}"
        )

    print(f"input: {simplified.graph.input[0].name} {tensor_shape(simplified.graph.input[0])}")
    for output in simplified.graph.output:
        print(f"output: {output.name} {tensor_shape(output)}")
    print(f"saved: {OUTPUT_MODEL.resolve()} ({OUTPUT_MODEL.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
