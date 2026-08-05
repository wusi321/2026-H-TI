class BallDetector:
    def __init__(self, calibration, settings):
        self.calibration = calibration
        self.settings = settings
        self.adaptive_l_enabled = bool(settings.get("adaptive_l_enabled", False))
        self.adaptive_l_interval_frames = max(
            1, int(settings.get("adaptive_l_interval_frames", 8))
        )
        self.adaptive_l_warmup_samples = max(
            1, int(settings.get("adaptive_l_warmup_samples", 4))
        )
        self.adaptive_l_gain = float(settings.get("adaptive_l_gain", 0.75))
        self.adaptive_l_smoothing = self._clamp01(
            settings.get("adaptive_l_smoothing", 0.35)
        )
        self.adaptive_l_min = float(settings.get("adaptive_l_min", 42))
        self.adaptive_l_max = float(settings.get("adaptive_l_max", 70))
        self.adaptive_l_bins = max(8, int(settings.get("adaptive_l_bins", 32)))
        self.base_l_max = float(settings["thresholds"][0][1])
        self.current_l_max = self.base_l_max
        self.reference_l_median = None
        self.last_l_median = None
        self._warmup_l_sum = 0.0
        self._lighting_samples = 0
        self.last_stats = self._empty_stats(list(calibration.roi))

    @staticmethod
    def _empty_stats(roi):
        return {
            "roi": list(roi),
            "raw": 0,
            "accepted": 0,
            "reject_size": 0,
            "reject_size_small": 0,
            "reject_size_large": 0,
            "reject_aspect": 0,
            "reject_density": 0,
            "reject_centroid": 0,
            "reject_axis": 0,
            "reject_score": 0,
            "raw_sizes": [],
            "candidate_details": [],
        }

    @staticmethod
    def _clamp01(value):
        return max(0.0, min(1.0, float(value)))

    def _search_roi(self, predicted_cm, gate_cm, allow_global):
        if allow_global or predicted_cm is None or gate_cm is None:
            return list(self.calibration.roi)
        padding = int(
            round(
                self.settings["max_diameter_px"] * 0.5
                + self.settings.get("track_roi_padding_px", 10)
            )
        )
        return self.calibration.tracking_roi(predicted_cm, gate_cm, padding)

    def _active_thresholds(self):
        thresholds = [list(item) for item in self.settings["thresholds"]]
        if self.adaptive_l_enabled:
            thresholds[0][1] = int(round(self.current_l_max))
        return thresholds

    def _add_lighting_stats(self, stats):
        stats["adaptive_l"] = self.adaptive_l_enabled
        stats["l_median"] = self.last_l_median
        stats["l_threshold"] = int(round(self.current_l_max))
        stats["lighting_samples"] = self._lighting_samples

    def update_lighting(self, img, frame_index):
        if not self.adaptive_l_enabled:
            return False
        if int(frame_index) % self.adaptive_l_interval_frames != 0:
            return False
        try:
            statistics = img.get_statistics(
                roi=list(self.calibration.roi),
                l_bins=self.adaptive_l_bins,
                a_bins=8,
                b_bins=8,
            )
            median = max(0.0, min(100.0, float(statistics.l_median())))
        except Exception as exc:
            self.adaptive_l_enabled = False
            self.current_l_max = self.base_l_max
            print("adaptive L disabled:", exc)
            return False

        self.last_l_median = median
        self._lighting_samples += 1
        if self.reference_l_median is None:
            self._warmup_l_sum += median
            if self._lighting_samples >= self.adaptive_l_warmup_samples:
                self.reference_l_median = self._warmup_l_sum / self._lighting_samples
            return True

        target = self.base_l_max + self.adaptive_l_gain * (
            median - self.reference_l_median
        )
        target = max(self.adaptive_l_min, min(self.adaptive_l_max, target))
        alpha = self.adaptive_l_smoothing
        self.current_l_max += alpha * (target - self.current_l_max)
        self.current_l_max = max(
            self.adaptive_l_min,
            min(self.adaptive_l_max, self.current_l_max),
        )
        return True

    def detect(self, img, predicted_cm=None, gate_cm=None, allow_global=False):
        roi = self._search_roi(predicted_cm, gate_cm, allow_global)
        blobs = img.find_blobs(
            self._active_thresholds(),
            invert=False,
            roi=roi,
            x_stride=self.settings["x_stride"],
            y_stride=self.settings["y_stride"],
            area_threshold=self.settings["area_threshold"],
            pixels_threshold=self.settings["pixels_threshold"],
            merge=self.settings["merge"],
            margin=self.settings["margin"],
        )
        stats = self._empty_stats(roi)
        self._add_lighting_stats(stats)
        stats["raw"] = len(blobs)
        candidates = []
        min_diameter = float(self.settings["min_diameter_px"])
        max_diameter = float(self.settings["max_diameter_px"])
        expected_diameter = float(self.settings["expected_diameter_px"])

        for blob in blobs:
            width = float(blob.w())
            height = float(blob.h())
            stats["raw_sizes"].append((int(width), int(height)))
            if width > max_diameter or height > max_diameter:
                stats["reject_size"] += 1
                stats["reject_size_large"] += 1
                continue
            if width < min_diameter or height < min_diameter:
                stats["reject_size"] += 1
                stats["reject_size_small"] += 1
                continue

            aspect_ratio = width / height
            if not self.settings["min_aspect"] <= aspect_ratio <= self.settings["max_aspect"]:
                stats["reject_aspect"] += 1
                continue

            roundness = float(blob.roundness())
            density = float(blob.density())
            pixels = int(blob.pixels())
            blob_center_x = float(blob.cx())
            blob_center_y = float(blob.cy())
            rect_center_x = float(blob.x()) + width * 0.5
            rect_center_y = float(blob.y()) + height * 0.5
            centroid_offset = (
                abs(blob_center_x - rect_center_x) / width
                + abs(blob_center_y - rect_center_y) / height
            )
            rect_weight = self.settings["bbox_center_weight"]
            center_x = rect_weight * rect_center_x + (1.0 - rect_weight) * blob_center_x
            center_y = rect_weight * rect_center_y + (1.0 - rect_weight) * blob_center_y
            s_px, normal_px = self.calibration.project_pixel(center_x, center_y)
            x_cm = self.calibration.pixel_to_cm(s_px)
            diameter = 0.5 * (width + height)
            aspect_score = min(width, height) / max(width, height)
            width_score = 1.0 - min(
                abs(width - expected_diameter) / max(1.0, expected_diameter), 1.0
            )
            height_score = 1.0 - min(
                abs(height - expected_diameter) / max(1.0, expected_diameter), 1.0
            )
            size_score = 0.65 * min(width_score, height_score) + 0.35 * (
                0.5 * (width_score + height_score)
            )
            density_score = 1.0 - min(
                abs(density - self.settings["expected_density"])
                / max(0.01, self.settings["density_tolerance"]),
                1.0,
            )
            axis_score = 1.0 - min(
                abs(normal_px) / self.settings["max_axis_distance_px"],
                1.0,
            )
            centroid_score = 1.0 - min(
                centroid_offset / self.settings["max_centroid_offset_ratio"],
                1.0,
            )
            # A reflective steel ball is often segmented as a crescent. Keep
            # roundness as weak evidence instead of rejecting it outright.
            roundness_score = min(
                roundness / max(0.01, self.settings["roundness_reference"]),
                1.0,
            )
            score = self._clamp01(
                0.28 * aspect_score
                + 0.27 * size_score
                + 0.17 * density_score
                + 0.13 * axis_score
                + 0.10 * centroid_score
                + 0.05 * roundness_score
            )
            min_score = (
                self.settings["min_score_search"]
                if allow_global
                else self.settings["min_score_track"]
            )
            motion_error_cm = (
                abs(x_cm - predicted_cm) if predicted_cm is not None else 0.0
            )
            reason = "ok"
            if not self.settings["min_density"] <= density <= self.settings["max_density"]:
                stats["reject_density"] += 1
                reason = "density"
            elif centroid_offset > self.settings["max_centroid_offset_ratio"]:
                stats["reject_centroid"] += 1
                reason = "centroid"
            elif (
                not self.calibration.contains_pixel(center_x, center_y)
                or abs(normal_px) > self.settings["max_axis_distance_px"]
            ):
                stats["reject_axis"] += 1
                reason = "axis"
            elif score < min_score:
                stats["reject_score"] += 1
                reason = "score"

            detail = {
                "width": int(width),
                "height": int(height),
                "pixels": pixels,
                "roundness": roundness,
                "density": density,
                "aspect": aspect_ratio,
                "centroid_offset": centroid_offset,
                "axis_distance": abs(normal_px),
                "score": score,
                "reason": reason,
            }
            if len(stats["candidate_details"]) < self.settings["diagnostic_limit"]:
                stats["candidate_details"].append(detail)
            if reason != "ok":
                continue

            candidates.append(
                {
                    "source": "blob",
                    "x_px": center_x,
                    "y_px": center_y,
                    "s_px": s_px,
                    "ratio": self.calibration.pixel_to_ratio(s_px),
                    "x_cm": x_cm,
                    "section": self.calibration.section_for_cm(x_cm),
                    "score": score,
                    "motion_error_cm": motion_error_cm,
                    "diameter_px": diameter,
                    "rect": [int(blob.x()), int(blob.y()), int(blob.w()), int(blob.h())],
                    "radius_px": max(1, int(round(diameter * 0.5))),
                    "search_scope": "global" if allow_global else "local",
                }
            )

        stats["accepted"] = len(candidates)
        self.last_stats = stats
        if not candidates:
            return None

        def rank(candidate):
            penalty = 0.0
            if predicted_cm is not None and gate_cm:
                penalty = min(candidate["motion_error_cm"] / gate_cm, 2.0) * 0.18
            return candidate["score"] - penalty

        return max(candidates, key=rank)
