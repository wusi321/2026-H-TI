import argparse
import csv
import json
import math


def load_samples(path):
    samples = []
    with open(path, "r", encoding="utf-8-sig", newline="") as file_obj:
        reader = csv.DictReader(file_obj)
        required = {"x_cm", "cx_px", "cy_px"}
        if reader.fieldnames is None or not required.issubset(set(reader.fieldnames)):
            raise ValueError("CSV must contain x_cm,cx_px,cy_px")
        for row in reader:
            samples.append(
                {
                    "x_cm": float(row["x_cm"]),
                    "cx_px": float(row["cx_px"]),
                    "cy_px": float(row["cy_px"]),
                }
            )
    samples.sort(key=lambda item: item["x_cm"])
    if len(samples) < 5:
        raise ValueError("at least five measured positions are required")
    if not any(abs(item["x_cm"]) < 1e-6 for item in samples):
        raise ValueError("a measured 0 cm sample is required")
    for index in range(1, len(samples)):
        if samples[index]["x_cm"] <= samples[index - 1]["x_cm"]:
            raise ValueError("x_cm values must be strictly increasing")
    return samples


def principal_axis(samples):
    count = float(len(samples))
    mean_x = sum(item["cx_px"] for item in samples) / count
    mean_y = sum(item["cy_px"] for item in samples) / count
    cov_xx = sum((item["cx_px"] - mean_x) ** 2 for item in samples)
    cov_yy = sum((item["cy_px"] - mean_y) ** 2 for item in samples)
    cov_xy = sum(
        (item["cx_px"] - mean_x) * (item["cy_px"] - mean_y) for item in samples
    )
    if cov_xx + cov_yy < 1.0:
        raise ValueError("calibration points do not span a usable beam axis")
    angle = 0.5 * math.atan2(2.0 * cov_xy, cov_xx - cov_yy)
    unit_x = math.cos(angle)
    unit_y = math.sin(angle)
    projections = []
    for item in samples:
        projection = (
            (item["cx_px"] - mean_x) * unit_x
            + (item["cy_px"] - mean_y) * unit_y
        )
        projections.append((item["x_cm"], projection, item))
    covariance = sum(x_cm * projection for x_cm, projection, _ in projections)
    if covariance < 0.0:
        unit_x = -unit_x
        unit_y = -unit_y
        projections = [(x_cm, -projection, item) for x_cm, projection, item in projections]
    projections.sort(key=lambda value: value[0])
    return mean_x, mean_y, unit_x, unit_y, projections


def interpolate_projection(x_cm, points):
    if x_cm <= points[0][0]:
        left, right = points[0], points[1]
    elif x_cm >= points[-1][0]:
        left, right = points[-2], points[-1]
    else:
        left, right = points[0], points[1]
        for index in range(1, len(points)):
            if x_cm <= points[index][0]:
                left, right = points[index - 1], points[index]
                break
    ratio = (float(x_cm) - left[0]) / (right[0] - left[0])
    return left[1] + ratio * (right[1] - left[1])


def build_calibration(
    samples,
    width,
    height,
    roi,
    beam_length_cm=25.0,
    ball_diameter_cm=1.0,
    axis_start_px=None,
    axis_end_px=None,
):
    if (axis_start_px is None) != (axis_end_px is None):
        raise ValueError("axis start and end must be supplied together")

    if axis_start_px is not None:
        axis_start = [float(value) for value in axis_start_px]
        axis_end = [float(value) for value in axis_end_px]
        roi_x, roi_y, roi_width, roi_height = [int(value) for value in roi]
        for point in (axis_start, axis_end):
            if not (
                0.0 <= point[0] < int(width)
                and 0.0 <= point[1] < int(height)
                and roi_x <= point[0] <= roi_x + roi_width
                and roi_y <= point[1] <= roi_y + roi_height
            ):
                raise ValueError("fixed beam axis is outside the image or roi")
        dx = axis_end[0] - axis_start[0]
        dy = axis_end[1] - axis_start[1]
        axis_length = math.sqrt(dx * dx + dy * dy)
        if axis_length < 10.0:
            raise ValueError("fixed beam axis is too short")
        unit_x = dx / axis_length
        unit_y = dy / axis_length
        projections = [
            (
                item["x_cm"],
                (item["cx_px"] - axis_start[0]) * unit_x
                + (item["cy_px"] - axis_start[1]) * unit_y,
                item,
            )
            for item in samples
        ]
        for index, (_, projection, _) in enumerate(projections):
            if projection < 0.0 or projection > axis_length:
                raise ValueError("measured point is outside the fixed beam axis")
            if index and projection <= projections[index - 1][1]:
                raise ValueError("fixed-axis pixel positions are not increasing")
        calibration_samples = [
            {"s_px": round(projection, 4), "x_cm": float(x_cm)}
            for x_cm, projection, _ in projections
        ]
    else:
        mean_x, mean_y, unit_x, unit_y, projections = principal_axis(samples)
        half_length_cm = 0.5 * float(beam_length_cm)
        start_projection = interpolate_projection(-half_length_cm, projections)
        end_projection = interpolate_projection(half_length_cm, projections)
        if end_projection - start_projection < 10.0:
            raise ValueError("fitted beam axis is too short")
        axis_start = [
            mean_x + start_projection * unit_x,
            mean_y + start_projection * unit_y,
        ]
        axis_end = [
            mean_x + end_projection * unit_x,
            mean_y + end_projection * unit_y,
        ]
        calibration_samples = [
            {
                "s_px": round(projection - start_projection, 4),
                "x_cm": float(x_cm),
            }
            for x_cm, projection, _ in projections
        ]
    return {
        "version": 1,
        "calibrated": True,
        "image_width": int(width),
        "image_height": int(height),
        "rod_length_cm": float(beam_length_cm),
        "ball_diameter_cm": float(ball_diameter_cm),
        "roi": [int(value) for value in roi],
        "axis_start_px": [round(value, 4) for value in axis_start],
        "axis_end_px": [round(value, 4) for value in axis_end],
        "samples": calibration_samples,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Build a MaixCAM fixed-beam 1D position calibration"
    )
    parser.add_argument("input_csv")
    parser.add_argument("output_json")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=360)
    parser.add_argument("--roi", type=int, nargs=4, default=[0, 108, 640, 92])
    parser.add_argument("--beam-length-cm", type=float, default=25.0)
    parser.add_argument("--ball-diameter-cm", type=float, default=1.0)
    parser.add_argument("--axis-start-px", type=float, nargs=2)
    parser.add_argument("--axis-end-px", type=float, nargs=2)
    args = parser.parse_args()

    samples = load_samples(args.input_csv)
    result = build_calibration(
        samples,
        args.width,
        args.height,
        args.roi,
        args.beam_length_cm,
        args.ball_diameter_cm,
        args.axis_start_px,
        args.axis_end_px,
    )
    with open(args.output_json, "w", encoding="utf-8") as file_obj:
        json.dump(result, file_obj, indent=2, ensure_ascii=True)
        file_obj.write("\n")
    print("calibration written:", args.output_json)


if __name__ == "__main__":
    main()
