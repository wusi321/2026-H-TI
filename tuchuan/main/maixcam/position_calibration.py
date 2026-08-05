import json
import math


class CalibrationError(ValueError):
    pass


class PositionCalibration:
    """Project image points onto a fixed beam axis and map pixels to cm."""

    def __init__(self, data):
        self.version = int(data.get("version", 1))
        self.calibrated = bool(data.get("calibrated", False))
        self.image_width = int(data["image_width"])
        self.image_height = int(data["image_height"])
        self.rod_length_cm = float(data.get("rod_length_cm", 25.0))
        self.ball_diameter_cm = float(data.get("ball_diameter_cm", 1.0))
        self.roi = [int(value) for value in data["roi"]]
        self.axis_start = tuple(float(value) for value in data["axis_start_px"])
        self.axis_end = tuple(float(value) for value in data["axis_end_px"])
        self.samples = sorted(
            [(float(item["s_px"]), float(item["x_cm"])) for item in data["samples"]],
            key=lambda item: item[0],
        )
        self._validate()
        dx = self.axis_end[0] - self.axis_start[0]
        dy = self.axis_end[1] - self.axis_start[1]
        self.axis_length_px = math.sqrt(dx * dx + dy * dy)
        self.axis_unit = (dx / self.axis_length_px, dy / self.axis_length_px)
        self.normal_unit = (-self.axis_unit[1], self.axis_unit[0])

    @classmethod
    def load(cls, path):
        with open(path, "r", encoding="utf-8") as file_obj:
            return cls(json.load(file_obj))

    def _validate(self):
        if self.version != 1:
            raise CalibrationError("unsupported calibration version")
        if self.rod_length_cm <= 0.0:
            raise CalibrationError("rod_length_cm must be positive")
        if not 0.0 < self.ball_diameter_cm < self.rod_length_cm:
            raise CalibrationError("ball_diameter_cm must be inside rod length")
        if len(self.roi) != 4 or self.roi[2] <= 0 or self.roi[3] <= 0:
            raise CalibrationError("roi must be [x, y, width, height]")
        x, y, width, height = self.roi
        if x < 0 or y < 0 or x + width > self.image_width or y + height > self.image_height:
            raise CalibrationError("roi is outside the image")
        dx = self.axis_end[0] - self.axis_start[0]
        dy = self.axis_end[1] - self.axis_start[1]
        if math.sqrt(dx * dx + dy * dy) < 10.0:
            raise CalibrationError("beam axis is too short")
        for name, point in (("axis_start_px", self.axis_start), ("axis_end_px", self.axis_end)):
            if not (
                0.0 <= point[0] < self.image_width
                and 0.0 <= point[1] < self.image_height
            ):
                raise CalibrationError("{} is outside the image".format(name))
            if not (
                x <= point[0] <= x + width
                and y <= point[1] <= y + height
            ):
                raise CalibrationError("{} is outside the roi".format(name))
        if len(self.samples) < 3:
            raise CalibrationError("at least three calibration samples are required")
        for index in range(1, len(self.samples)):
            if self.samples[index][0] <= self.samples[index - 1][0]:
                raise CalibrationError("sample s_px values must be strictly increasing")
            if self.samples[index][1] <= self.samples[index - 1][1]:
                raise CalibrationError("sample x_cm values must be strictly increasing")
        axis_length = math.sqrt(dx * dx + dy * dy)
        if self.samples[0][0] < 0.0 or self.samples[-1][0] > axis_length:
            raise CalibrationError("calibration samples must lie on the beam axis")

    @staticmethod
    def _piecewise(value, points):
        if value <= points[0][0]:
            left, right = points[0], points[1]
        elif value >= points[-1][0]:
            left, right = points[-2], points[-1]
        else:
            left, right = points[0], points[1]
            for index in range(1, len(points)):
                if value <= points[index][0]:
                    left, right = points[index - 1], points[index]
                    break
        span = right[0] - left[0]
        ratio = (float(value) - left[0]) / span
        return left[1] + ratio * (right[1] - left[1])

    def project_pixel(self, x_px, y_px):
        dx = float(x_px) - self.axis_start[0]
        dy = float(y_px) - self.axis_start[1]
        s_px = dx * self.axis_unit[0] + dy * self.axis_unit[1]
        normal_px = dx * self.normal_unit[0] + dy * self.normal_unit[1]
        return s_px, normal_px

    def point_at_s(self, s_px):
        return (
            self.axis_start[0] + float(s_px) * self.axis_unit[0],
            self.axis_start[1] + float(s_px) * self.axis_unit[1],
        )

    def pixel_to_cm(self, s_px):
        value = self._piecewise(float(s_px), self.samples)
        return self.clamp_cm(value)

    def cm_to_pixel(self, x_cm):
        inverse = [(x_cm_value, s_px) for s_px, x_cm_value in self.samples]
        return self._piecewise(float(x_cm), inverse)

    def pixel_to_ratio(self, s_px):
        return max(0.0, min(1.0, float(s_px) / self.axis_length_px))

    def ratio_to_cm(self, ratio):
        value = max(0.0, min(1.0, float(ratio)))
        return (value - 0.5) * self.rod_length_cm

    def clamp_cm(self, x_cm):
        half_length = 0.5 * self.rod_length_cm
        return max(-half_length, min(half_length, float(x_cm)))

    def ball_center_limits_cm(self):
        limit = 0.5 * (self.rod_length_cm - self.ball_diameter_cm)
        return (-limit, limit)

    def section_for_cm(self, x_cm):
        value = self.clamp_cm(x_cm)
        if value < -7.5:
            return "left_end"
        if value < -2.5:
            return "left"
        if value <= 2.5:
            return "center"
        if value <= 7.5:
            return "right"
        return "right_end"

    def image_point_for_cm(self, x_cm):
        return self.point_at_s(self.cm_to_pixel(x_cm))

    def contains_pixel(self, x_px, y_px, margin_px=0.0):
        x, y, width, height = self.roi
        margin = max(0.0, float(margin_px))
        inside_roi = (
            x - margin <= float(x_px) <= x + width + margin
            and y - margin <= float(y_px) <= y + height + margin
        )
        s_px, _ = self.project_pixel(x_px, y_px)
        return inside_roi and -margin <= s_px <= self.axis_length_px + margin

    def tracking_roi(self, center_cm, half_width_cm, padding_px):
        if center_cm is None or half_width_cm is None:
            return list(self.roi)
        point_a = self.image_point_for_cm(float(center_cm) - float(half_width_cm))
        point_b = self.image_point_for_cm(float(center_cm) + float(half_width_cm))
        padding = max(1, int(padding_px))
        roi_x, roi_y, roi_w, roi_h = self.roi
        left = max(roi_x, int(min(point_a[0], point_b[0])) - padding)
        top = max(roi_y, int(min(point_a[1], point_b[1])) - padding)
        right = min(roi_x + roi_w, int(max(point_a[0], point_b[0])) + padding + 1)
        bottom = min(roi_y + roi_h, int(max(point_a[1], point_b[1])) + padding + 1)
        if right <= left or bottom <= top:
            return list(self.roi)
        return [left, top, right - left, bottom - top]

    def tick_segment(self, x_cm, half_length_px):
        center = self.image_point_for_cm(x_cm)
        half_length = float(half_length_px)
        return (
            center[0] - self.normal_unit[0] * half_length,
            center[1] - self.normal_unit[1] * half_length,
            center[0] + self.normal_unit[0] * half_length,
            center[1] + self.normal_unit[1] * half_length,
        )
