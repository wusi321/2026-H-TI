import os


class ResizeMapper:
    def __init__(self, source_width, source_height, target_width, target_height, preserve_aspect):
        self.source_width = float(source_width)
        self.source_height = float(source_height)
        self.target_width = float(target_width)
        self.target_height = float(target_height)
        self.preserve_aspect = bool(preserve_aspect)
        if self.preserve_aspect:
            self.scale_x = self.scale_y = min(
                self.target_width / self.source_width,
                self.target_height / self.source_height,
            )
            self.offset_x = 0.5 * (self.target_width - self.source_width * self.scale_x)
            self.offset_y = 0.5 * (self.target_height - self.source_height * self.scale_y)
        else:
            self.scale_x = self.target_width / self.source_width
            self.scale_y = self.target_height / self.source_height
            self.offset_x = 0.0
            self.offset_y = 0.0

    def point_to_source(self, x_value, y_value):
        return (
            (float(x_value) - self.offset_x) / self.scale_x,
            (float(y_value) - self.offset_y) / self.scale_y,
        )

    def rect_to_source(self, x_value, y_value, width, height):
        left, top = self.point_to_source(x_value, y_value)
        right, bottom = self.point_to_source(
            float(x_value) + float(width),
            float(y_value) + float(height),
        )
        left = max(0.0, min(self.source_width, left))
        top = max(0.0, min(self.source_height, top))
        right = max(0.0, min(self.source_width, right))
        bottom = max(0.0, min(self.source_height, bottom))
        return (left, top, max(0.0, right - left), max(0.0, bottom - top))


class YoloReacquirer:
    def __init__(
        self,
        enabled,
        model_paths,
        calibration,
        conf,
        iou,
        keypoint,
        preserve_aspect=True,
        use_beam_crop=False,
        min_diameter_px=8,
        max_diameter_px=72,
        min_aspect=0.50,
        max_axis_distance_px=38,
        keypoint_box_margin_ratio=0.25,
    ):
        self.enabled = bool(enabled)
        self.calibration = calibration
        self.conf = float(conf)
        self.iou = float(iou)
        self.keypoint = float(keypoint)
        self.preserve_aspect = bool(preserve_aspect)
        self.use_beam_crop = bool(use_beam_crop)
        self.min_diameter_px = float(min_diameter_px)
        self.max_diameter_px = float(max_diameter_px)
        self.min_aspect = float(min_aspect)
        self.max_axis_distance_px = float(max_axis_distance_px)
        self.keypoint_box_margin_ratio = float(keypoint_box_margin_ratio)
        self.detector = None
        self.model_path = None
        if not self.enabled:
            return
        for path in model_paths:
            if os.path.exists(path):
                self.model_path = path
                break
        if self.model_path is None:
            print("YOLO reacquire disabled: model not found")
            self.enabled = False
            return
        from maix import nn

        self.detector = nn.YOLO11(model=self.model_path, dual_buff=False)
        print("YOLO reacquire ready:", self.model_path)

    def detect(self, img):
        if not self.enabled or self.detector is None:
            return None
        input_width = self.detector.input_width()
        input_height = self.detector.input_height()
        source_img = img
        offset_x = 0
        offset_y = 0
        if self.use_beam_crop:
            roi_x, roi_y, roi_w, roi_h = self.calibration.roi
            source_img = img.crop(roi_x, roi_y, roi_w, roi_h)
            offset_x = roi_x
            offset_y = roi_y

        mapper = ResizeMapper(
            source_img.width(),
            source_img.height(),
            input_width,
            input_height,
            self.preserve_aspect,
        )
        if source_img.width() == input_width and source_img.height() == input_height:
            img_ai = source_img
        elif self.preserve_aspect:
            from maix import image

            img_ai = source_img.resize(input_width, input_height, image.Fit.FIT_CONTAIN)
        else:
            img_ai = source_img.resize(input_width, input_height)
        objects = self.detector.detect(
            img_ai,
            conf_th=self.conf,
            iou_th=self.iou,
            keypoint_th=self.keypoint,
        )
        best = None
        for obj in objects:
            local_x, local_y, width, height = mapper.rect_to_source(
                obj.x, obj.y, obj.w, obj.h
            )
            if width <= 0.0 or height <= 0.0:
                continue
            aspect = min(width, height) / max(width, height)
            if aspect < self.min_aspect:
                continue
            if (
                min(width, height) < self.min_diameter_px
                or max(width, height) > self.max_diameter_px
            ):
                continue
            center_x, center_y = mapper.point_to_source(
                float(obj.x) + float(obj.w) * 0.5,
                float(obj.y) + float(obj.h) * 0.5,
            )
            center_x += offset_x
            center_y += offset_y
            points = getattr(obj, "points", None)
            if points is not None and len(points) >= 2:
                point_x = float(points[0])
                point_y = float(points[1])
                if point_x > 0.0 and point_y > 0.0:
                    point_local_x, point_local_y = mapper.point_to_source(point_x, point_y)
                    margin = max(width, height) * self.keypoint_box_margin_ratio
                    if not (
                        local_x - margin <= point_local_x <= local_x + width + margin
                        and local_y - margin <= point_local_y <= local_y + height + margin
                    ):
                        continue
                    center_x, center_y = point_local_x, point_local_y
                    center_x += offset_x
                    center_y += offset_y
            if not self.calibration.contains_pixel(center_x, center_y):
                continue
            s_px, normal_px = self.calibration.project_pixel(center_x, center_y)
            if abs(normal_px) > self.max_axis_distance_px:
                continue
            score = float(obj.score)
            x_cm = self.calibration.pixel_to_cm(s_px)
            diameter = 0.5 * (width + height)
            candidate = {
                "source": "yolo",
                "x_px": center_x,
                "y_px": center_y,
                "s_px": s_px,
                "ratio": self.calibration.pixel_to_ratio(s_px),
                "x_cm": x_cm,
                "section": self.calibration.section_for_cm(x_cm),
                "score": score,
                "rect": [
                    int(local_x + offset_x),
                    int(local_y + offset_y),
                    int(width),
                    int(height),
                ],
                "diameter_px": diameter,
                "radius_px": max(1, int(round(diameter * 0.5))),
                "search_scope": "global",
            }
            if best is None or candidate["score"] > best["score"]:
                best = candidate
        return best
