import os
import sys
import unittest


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAIXCAM_DIR = os.path.join(PROJECT_ROOT, "maixcam")
HOST_DIR = os.path.join(PROJECT_ROOT, "host")
sys.path.insert(0, MAIXCAM_DIR)
sys.path.insert(0, HOST_DIR)

from ball_detector import BallDetector
from balance_controller import BalanceController
from build_position_calibration import build_calibration
from candidate_confirmation import CandidateConfirmation
from measurement_fusion import (
    candidate_requires_confirmation,
    fuse_candidates,
    is_far_candidate,
    make_pending_yolo_candidate,
    should_hold_tracked_blob,
    should_run_yolo,
    yolo_conflict_is_strong,
)
from position_calibration import CalibrationError, PositionCalibration
from state_estimator import AlphaBetaTracker
from uart_transport import write_all
from vision_protocol import (
    FLAG_MEASURED,
    FLAG_TRACKED,
    PACKET_SIZE,
    crc16_ccitt,
    decode_measurement,
    encode_measurement,
)
from yolo_reacquire import ResizeMapper, YoloReacquirer


def vertical_calibration_data(calibrated=True):
    return {
        "version": 1,
        "calibrated": calibrated,
        "image_width": 640,
        "image_height": 360,
        "rod_length_cm": 25.0,
        "ball_diameter_cm": 1.0,
        "roi": [260, 20, 120, 320],
        "axis_start_px": [320.0, 30.0],
        "axis_end_px": [320.0, 330.0],
        "samples": [
            {"s_px": 30.0, "x_cm": -10.0},
            {"s_px": 90.0, "x_cm": -5.0},
            {"s_px": 150.0, "x_cm": 0.0},
            {"s_px": 210.0, "x_cm": 5.0},
            {"s_px": 270.0, "x_cm": 10.0},
        ],
    }


class CalibrationTests(unittest.TestCase):
    def test_production_rgb_photo_calibration(self):
        calibration = PositionCalibration.load(
            os.path.join(
                MAIXCAM_DIR,
                "calibration",
                "position_calibration.json",
            )
        )

        self.assertTrue(calibration.calibrated)
        self.assertEqual(calibration.roi, [0, 108, 640, 92])
        measured_marks = (
            (66.0, -10.0),
            (191.0, -5.0),
            (338.0, 0.0),
            (487.0, 5.0),
            (606.0, 10.0),
        )
        for x_px, expected_cm in measured_marks:
            s_px, normal_px = calibration.project_pixel(x_px, 175.0)
            self.assertAlmostEqual(normal_px, 0.0, places=4)
            self.assertAlmostEqual(
                calibration.pixel_to_cm(s_px),
                expected_cm,
                places=4,
            )
            self.assertAlmostEqual(
                calibration.cm_to_pixel(expected_cm),
                x_px,
                places=4,
            )

        segment_scales = [
            (measured_marks[index + 1][0] - measured_marks[index][0]) / 5.0
            for index in range(len(measured_marks) - 1)
        ]
        self.assertGreater(max(segment_scales) - min(segment_scales), 4.0)

    def test_vertical_projection_and_piecewise_mapping(self):
        calibration = PositionCalibration(vertical_calibration_data())
        s_px, normal_px = calibration.project_pixel(325.0, 240.0)
        self.assertAlmostEqual(s_px, 210.0)
        self.assertAlmostEqual(normal_px, -5.0)
        self.assertAlmostEqual(calibration.pixel_to_cm(s_px), 5.0)
        point = calibration.image_point_for_cm(-5.0)
        self.assertAlmostEqual(point[0], 320.0)
        self.assertAlmostEqual(point[1], 120.0)

    def test_full_rod_ratio_coordinate_and_sections(self):
        calibration = PositionCalibration(vertical_calibration_data())
        self.assertAlmostEqual(calibration.ratio_to_cm(0.0), -12.5)
        self.assertAlmostEqual(calibration.ratio_to_cm(0.5), 0.0)
        self.assertAlmostEqual(calibration.ratio_to_cm(1.0), 12.5)
        self.assertEqual(calibration.ball_center_limits_cm(), (-12.0, 12.0))
        self.assertEqual(calibration.section_for_cm(-9.0), "left_end")
        self.assertEqual(calibration.section_for_cm(-5.0), "left")
        self.assertEqual(calibration.section_for_cm(0.0), "center")
        self.assertEqual(calibration.section_for_cm(5.0), "right")
        self.assertEqual(calibration.section_for_cm(9.0), "right_end")

    def test_tracking_roi_is_tall_for_vertical_axis(self):
        calibration = PositionCalibration(vertical_calibration_data())
        roi = calibration.tracking_roi(0.0, 2.0, 20)
        self.assertLess(roi[2], calibration.roi[2])
        self.assertLess(roi[3], calibration.roi[3])
        self.assertGreater(roi[3], roi[2])

    def test_rejects_reversed_physical_coordinate(self):
        data = vertical_calibration_data()
        data["samples"] = list(reversed(data["samples"]))
        for item in data["samples"]:
            item["s_px"] = 300.0 - item["s_px"]
        with self.assertRaises(CalibrationError):
            PositionCalibration(data)


class BuilderTests(unittest.TestCase):
    def test_rebuilds_photo_calibration_with_fixed_axis(self):
        measured = (
            (-10.0, 66.0),
            (-5.0, 191.0),
            (0.0, 338.0),
            (5.0, 487.0),
            (10.0, 606.0),
        )
        samples = [
            {"x_cm": x_cm, "cx_px": x_px, "cy_px": 175.0}
            for x_cm, x_px in measured
        ]

        result = build_calibration(
            samples,
            640,
            360,
            [0, 108, 640, 92],
            axis_start_px=[0.0, 175.0],
            axis_end_px=[639.0, 175.0],
        )

        self.assertEqual(result["axis_start_px"], [0.0, 175.0])
        self.assertEqual(result["axis_end_px"], [639.0, 175.0])
        self.assertEqual(
            [item["s_px"] for item in result["samples"]],
            [item[1] for item in measured],
        )

    def test_builds_vertical_axis_and_full_length_endpoints(self):
        samples = [
            {"x_cm": -10.0, "cx_px": 320.1, "cy_px": 60.0},
            {"x_cm": -5.0, "cx_px": 320.0, "cy_px": 120.0},
            {"x_cm": 0.0, "cx_px": 319.9, "cy_px": 180.0},
            {"x_cm": 5.0, "cx_px": 320.1, "cy_px": 240.0},
            {"x_cm": 10.0, "cx_px": 320.0, "cy_px": 300.0},
        ]
        result = build_calibration(samples, 640, 360, [260, 20, 120, 320])
        self.assertTrue(result["calibrated"])
        self.assertAlmostEqual(result["axis_start_px"][1], 30.0, delta=0.2)
        self.assertAlmostEqual(result["axis_end_px"][1], 330.0, delta=0.2)
        calibration = PositionCalibration(result)
        self.assertAlmostEqual(calibration.pixel_to_cm(150.0), 0.0, delta=0.05)
        self.assertEqual(result["rod_length_cm"], 25.0)
        self.assertEqual(result["ball_diameter_cm"], 1.0)


class FakeBlob:
    def __init__(self, x, y, width, height, roundness=0.8, density=0.65, cx=None, cy=None):
        self._x = x
        self._y = y
        self._width = width
        self._height = height
        self._roundness = roundness
        self._density = density
        self._cx = x + width * 0.5 if cx is None else cx
        self._cy = y + height * 0.5 if cy is None else cy

    def x(self):
        return self._x

    def y(self):
        return self._y

    def w(self):
        return self._width

    def h(self):
        return self._height

    def cx(self):
        return self._cx

    def cy(self):
        return self._cy

    def roundness(self):
        return self._roundness

    def density(self):
        return self._density

    def pixels(self):
        return int(round(self._width * self._height * self._density))


class FakeImage:
    def __init__(self, blobs):
        self.blobs = blobs
        self.last_roi = None
        self.last_thresholds = None

    def find_blobs(self, thresholds, **kwargs):
        self.last_thresholds = thresholds
        self.last_roi = kwargs["roi"]
        return self.blobs


class FakeStatistics:
    def __init__(self, l_median):
        self._l_median = l_median

    def l_median(self):
        return self._l_median


class FakeAdaptiveImage(FakeImage):
    def __init__(self, blobs, l_median):
        super().__init__(blobs)
        self.l_median = l_median
        self.statistics_calls = 0

    def get_statistics(self, **kwargs):
        self.statistics_calls += 1
        return FakeStatistics(self.l_median)


def detector_settings():
    return {
        "thresholds": [[0, 75, -30, 30, -30, 30]],
        "x_stride": 2,
        "y_stride": 1,
        "area_threshold": 24,
        "pixels_threshold": 24,
        "merge": False,
        "margin": 2,
        "min_diameter_px": 6,
        "max_diameter_px": 28,
        "expected_diameter_px": 24,
        "min_aspect": 0.55,
        "max_aspect": 1.82,
        "roundness_reference": 0.20,
        "min_density": 0.18,
        "max_density": 0.98,
        "expected_density": 0.55,
        "density_tolerance": 0.45,
        "max_centroid_offset_ratio": 0.32,
        "max_axis_distance_px": 28,
        "track_roi_padding_px": 10,
        "min_score_search": 0.62,
        "min_score_track": 0.52,
        "diagnostic_limit": 8,
        "bbox_center_weight": 0.80,
    }


class DetectorTests(unittest.TestCase):
    def test_adaptive_l_tracks_global_brightness_at_low_rate(self):
        calibration = PositionCalibration(vertical_calibration_data())
        settings = detector_settings()
        settings.update(
            {
                "adaptive_l_enabled": True,
                "adaptive_l_interval_frames": 2,
                "adaptive_l_warmup_samples": 2,
                "adaptive_l_gain": 0.75,
                "adaptive_l_smoothing": 1.0,
                "adaptive_l_min": 42,
                "adaptive_l_max": 70,
                "adaptive_l_bins": 32,
            }
        )
        detector = BallDetector(calibration, settings)
        image_obj = FakeAdaptiveImage([], 60)
        self.assertTrue(detector.update_lighting(image_obj, 0))
        self.assertFalse(detector.update_lighting(image_obj, 1))
        self.assertTrue(detector.update_lighting(image_obj, 2))
        self.assertAlmostEqual(detector.reference_l_median, 60.0)

        image_obj.l_median = 80
        self.assertTrue(detector.update_lighting(image_obj, 4))
        self.assertEqual(detector.current_l_max, 70.0)
        detector.detect(image_obj, allow_global=True)
        self.assertEqual(image_obj.last_thresholds[0][1], 70)
        self.assertEqual(detector.last_stats["l_median"], 80.0)
        self.assertEqual(image_obj.statistics_calls, 3)

    def test_selects_round_candidate_on_vertical_axis(self):
        calibration = PositionCalibration(vertical_calibration_data())
        image_obj = FakeImage(
            [
                FakeBlob(314, 234, 12, 12),
                FakeBlob(350, 230, 10, 22, roundness=0.2),
            ]
        )
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(image_obj, predicted_cm=5.0, gate_cm=1.5)
        self.assertIsNotNone(candidate)
        self.assertAlmostEqual(candidate["x_cm"], 5.0, delta=0.1)
        self.assertLess(image_obj.last_roi[2], calibration.roi[2])
        self.assertEqual(detector.last_stats["accepted"], 1)

    def test_rejects_candidate_far_from_axis(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(FakeImage([FakeBlob(365, 174, 12, 12)]))
        self.assertIsNone(candidate)
        self.assertEqual(detector.last_stats["reject_axis"], 1)

    def test_reports_small_and_large_size_rejections(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(
            FakeImage(
                [
                    FakeBlob(318, 178, 4, 4),
                    FakeBlob(280, 170, 80, 20),
                ]
            )
        )
        self.assertIsNone(candidate)
        self.assertEqual(detector.last_stats["reject_size"], 2)
        self.assertEqual(detector.last_stats["reject_size_small"], 1)
        self.assertEqual(detector.last_stats["reject_size_large"], 1)
        self.assertEqual(detector.last_stats["raw_sizes"], [(4, 4), (80, 20)])

    def test_accepts_low_roundness_reflective_ball_by_shape_score(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(
            FakeImage([FakeBlob(308, 228, 24, 27, roundness=0.05, density=0.45)]),
            allow_global=True,
        )
        self.assertIsNotNone(candidate)
        self.assertGreaterEqual(candidate["score"], detector_settings()["min_score_search"])
        self.assertEqual(detector.last_stats["candidate_details"][0]["reason"], "ok")

    def test_rejects_elongated_groove_fragment(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(
            FakeImage([FakeBlob(308, 228, 24, 12, roundness=0.05, density=0.55)]),
            allow_global=True,
        )
        self.assertIsNone(candidate)
        self.assertEqual(detector.last_stats["reject_aspect"], 1)

    def test_prefers_complete_ball_over_smaller_fragment(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(
            FakeImage(
                [
                    FakeBlob(309, 231, 21, 16, roundness=0.08, density=0.50),
                    FakeBlob(308, 228, 24, 27, roundness=0.05, density=0.45),
                ]
            ),
            allow_global=True,
        )
        self.assertIsNotNone(candidate)
        self.assertEqual(candidate["rect"], [308, 228, 24, 27])

    def test_far_candidate_is_not_rejected_by_motion(self):
        calibration = PositionCalibration(vertical_calibration_data())
        detector = BallDetector(calibration, detector_settings())
        candidate = detector.detect(
            FakeImage([FakeBlob(308, 288, 24, 24, roundness=0.08, density=0.50)]),
            predicted_cm=-5.0,
            gate_cm=1.0,
            allow_global=False,
        )
        self.assertIsNotNone(candidate)
        self.assertGreater(candidate["motion_error_cm"], 1.0)


class ConfirmationTests(unittest.TestCase):
    @staticmethod
    def candidate(x_cm, diameter=12.0):
        return {
            "x_cm": x_cm,
            "diameter_px": diameter,
            "radius_px": diameter * 0.5,
            "source": "blob",
        }

    def test_requires_two_consistent_candidates(self):
        gate = CandidateConfirmation(2, 1.8, 8.0, 120)
        self.assertFalse(gate.update(self.candidate(1.0), 1000))
        self.assertTrue(gate.update(self.candidate(1.2), 1020))

    def test_candidate_expires_after_gap(self):
        gate = CandidateConfirmation(2, 1.8, 8.0, 100)
        self.assertFalse(gate.update(self.candidate(1.0), 1000))
        self.assertFalse(gate.update(None, 1110))
        self.assertFalse(gate.update(self.candidate(1.0), 1120))


class TrackerTests(unittest.TestCase):
    def test_position_velocity_and_expiry(self):
        tracker = AlphaBetaTracker(valid_hold_ms=70, reacquire_after_ms=55)
        tracker.update(0.0, 1000, 0.9)
        tracker.update(1.0, 1020, 0.9)
        self.assertGreater(tracker.snapshot(1030)["x_cm"], 0.0)
        self.assertTrue(tracker.should_reacquire(1080))
        tracker.miss(1100)
        self.assertFalse(tracker.snapshot(1100)["valid"])

    def test_confirmed_far_reacquire_resets_lag(self):
        tracker = AlphaBetaTracker()
        tracker.update(-8.0, 1000, 0.8)
        tracker.reacquire(8.0, 1033, 0.9)
        state = tracker.snapshot(1033)
        self.assertAlmostEqual(state["x_cm"], 8.0)
        self.assertGreater(state["v_cm_s"], 0.0)
        self.assertLessEqual(state["v_cm_s"], tracker.max_speed_cm_s)


class BalanceControllerTests(unittest.TestCase):
    @staticmethod
    def make_controller():
        return BalanceController(
            target_x_cm=0.0,
            kp_deg_per_cm=0.2,
            kd_deg_per_cm_s=0.015,
            deadband_cm=0.08,
            max_tilt_deg=3.0,
            max_slew_deg_s=30.0,
            direction=1.0,
        )

    def test_position_and_velocity_generate_limited_tilt(self):
        controller = self.make_controller()
        controller.update({"valid": True, "x_cm": -5.0, "v_cm_s": 0.0}, 1000)
        result = controller.update(
            {"valid": True, "x_cm": -5.0, "v_cm_s": 10.0},
            1033,
        )
        self.assertGreater(result["tilt_deg"], 0.0)
        self.assertLessEqual(abs(result["tilt_deg"]), 3.0)
        self.assertLessEqual(abs(result["tilt_deg"]), 30.0 * 0.033 + 1e-6)

    def test_invalid_vision_commands_level(self):
        controller = self.make_controller()
        controller.update({"valid": True, "x_cm": -5.0, "v_cm_s": 0.0}, 1000)
        controller.update({"valid": True, "x_cm": -5.0, "v_cm_s": 0.0}, 1100)
        result = controller.update({"valid": False}, 1200)
        self.assertEqual(result["reason"], "vision_lost")
        self.assertAlmostEqual(result["tilt_deg"], 0.0)


class FusionTests(unittest.TestCase):
    @staticmethod
    def candidate(x_cm, source="blob", score=0.8):
        return {
            "x_cm": float(x_cm),
            "source": source,
            "score": float(score),
            "diameter_px": 24.0,
            "radius_px": 12,
        }

    def test_heartbeat_and_far_jump_schedule_yolo(self):
        blob = self.candidate(8.0)
        self.assertTrue(should_run_yolo(6, blob, 8.0, 1.0, 6, 2))
        self.assertTrue(should_run_yolo(7, blob, 0.0, 1.0, 6, 2))
        self.assertFalse(should_run_yolo(7, blob, 8.0, 1.0, 6, 2))
        self.assertTrue(should_run_yolo(8, None, 8.0, 1.0, 6, 2))

    def test_agreement_verifies_blob_without_replacing_its_center(self):
        blob = self.candidate(4.0)
        yolo = self.candidate(4.6, source="yolo", score=0.9)
        result, status = fuse_candidates(blob, yolo, 1.5)
        self.assertEqual(status, "agree")
        self.assertTrue(result["verified"])
        self.assertEqual(result["source"], "blob+yolo")
        self.assertAlmostEqual(result["x_cm"], 4.0)

    def test_disagreement_keeps_blob_but_does_not_verify_jump(self):
        blob = self.candidate(9.0)
        yolo = self.candidate(-2.0, source="yolo", score=0.95)
        result, status = fuse_candidates(blob, yolo, 1.5)
        self.assertEqual(status, "disagree")
        self.assertFalse(result["verified"])
        self.assertTrue(is_far_candidate(result, 0.0, 2.0))

    def test_single_yolo_conflict_does_not_replace_stable_blob(self):
        blob = self.candidate(4.2)
        self.assertTrue(should_hold_tracked_blob(blob, 4.0, 1.0))
        self.assertFalse(should_hold_tracked_blob(blob, -4.0, 1.0))

    def test_unconfirmed_yolo_conflict_requires_temporal_confirmation(self):
        yolo = self.candidate(9.0, source="yolo", score=0.95)
        result = make_pending_yolo_candidate(yolo)
        self.assertEqual(result["fusion"], "yolo_pending")
        self.assertFalse(result["verified"])
        self.assertTrue(result["require_confirmation"])
        self.assertTrue(is_far_candidate(result, 0.0, 2.0))
        self.assertTrue(candidate_requires_confirmation(result, 9.0, 2.0))

    def test_conflicting_yolo_requires_stricter_confidence(self):
        weak = self.candidate(9.0, source="yolo", score=0.69)
        strong = self.candidate(9.0, source="yolo", score=0.70)
        self.assertFalse(yolo_conflict_is_strong(weak, 0.70))
        self.assertTrue(yolo_conflict_is_strong(strong, 0.70))

    def test_verified_far_candidate_can_reacquire_immediately(self):
        candidate = self.candidate(9.0)
        candidate["verified"] = True
        self.assertFalse(candidate_requires_confirmation(candidate, 0.0, 2.0))


class ProtocolTests(unittest.TestCase):
    @staticmethod
    def valid_state():
        return {
            "initialized": True,
            "valid": True,
            "x_cm": -3.4,
            "v_cm_s": 12.3,
            "confidence": 0.876,
        }

    def test_round_trip_and_size(self):
        packet = encode_measurement(17, 123456, self.valid_state(), True, 4200)
        self.assertEqual(len(packet), PACKET_SIZE)
        decoded = decode_measurement(packet)
        self.assertEqual(decoded["x_cm"], -3.4)
        self.assertEqual(decoded["v_cm_s"], 12.3)
        self.assertTrue(decoded["measured"])

    def test_matches_wire_format_golden_packet(self):
        packet = encode_measurement(17, 123456, self.valid_state(), True, 4200)
        self.assertEqual(
            packet,
            bytes.fromhex(
                "aa 55 01 03 11 00 40 e2 01 00 de ff 7b 00 "
                "6c 03 68 10 b5 b4"
            ),
        )
        self.assertEqual(crc16_ccitt(b"123456789"), 0x29B1)

    def test_invalid_state_has_no_measurement_or_tracking_flags(self):
        state = {
            "initialized": True,
            "valid": False,
            "x_cm": 1.0,
            "v_cm_s": 2.0,
            "confidence": 0.25,
        }
        decoded = decode_measurement(encode_measurement(1, 2, state, True))
        self.assertFalse(decoded["valid"])
        self.assertEqual(decoded["flags"] & (FLAG_MEASURED | FLAG_TRACKED), 0)

    def test_uncalibrated_uart_mode_forces_safe_invalid_packet(self):
        decoded = decode_measurement(
            encode_measurement(
                17,
                123456,
                self.valid_state(),
                True,
                4200,
                force_invalid=True,
            )
        )
        self.assertFalse(decoded["valid"])
        self.assertFalse(decoded["measured"])
        self.assertFalse(decoded["tracked"])
        self.assertIsNone(decoded["x_cm"])
        self.assertEqual(decoded["confidence"], 0.0)

    def test_rejects_bad_crc(self):
        packet = bytearray(
            encode_measurement(17, 123456, self.valid_state(), True, 4200)
        )
        packet[10] ^= 0x01
        with self.assertRaisesRegex(ValueError, "CRC mismatch"):
            decode_measurement(packet)

    def test_rejects_conflicting_state_flags_with_valid_crc(self):
        packet = bytearray(
            encode_measurement(17, 123456, self.valid_state(), True, 4200)
        )
        packet[3] |= FLAG_TRACKED
        packet[-2:] = crc16_ccitt(packet[:-2]).to_bytes(2, "little")
        with self.assertRaisesRegex(ValueError, "conflicting packet state"):
            decode_measurement(packet)


class UartTransportTests(unittest.TestCase):
    class PartialPort:
        def __init__(self, chunk_size):
            self.chunk_size = chunk_size
            self.output = bytearray()

        def write(self, data):
            count = min(self.chunk_size, len(data))
            self.output.extend(data[:count])
            return count

    def test_write_all_retries_partial_writes(self):
        port = self.PartialPort(3)
        packet = b"0123456789"
        self.assertEqual(write_all(port, packet), len(packet))
        self.assertEqual(bytes(port.output), packet)

    def test_write_all_rejects_no_progress(self):
        port = self.PartialPort(0)
        with self.assertRaisesRegex(OSError, "invalid UART write count"):
            write_all(port, b"packet")


class ResizeMapperTests(unittest.TestCase):
    def test_letterbox_mapping(self):
        mapper = ResizeMapper(640, 360, 320, 320, True)
        x_value, y_value = mapper.point_to_source(160.0, 160.0)
        self.assertAlmostEqual(x_value, 320.0)
        self.assertAlmostEqual(y_value, 180.0)


class FakeYoloObject:
    def __init__(self, x, y, width, height, score=0.9, points=None):
        self.x = x
        self.y = y
        self.w = width
        self.h = height
        self.score = score
        self.points = points


class FakeYoloDetector:
    def __init__(self, objects):
        self.objects = objects

    def input_width(self):
        return 640

    def input_height(self):
        return 360

    def detect(self, img, **kwargs):
        return self.objects


class FakeSizedImage:
    def width(self):
        return 640

    def height(self):
        return 360


class YoloValidationTests(unittest.TestCase):
    def make_reacquirer(self, objects):
        calibration = PositionCalibration(vertical_calibration_data())
        reacquirer = YoloReacquirer(
            False,
            [],
            calibration,
            0.60,
            0.45,
            0.45,
            True,
            False,
            8,
            72,
            0.50,
            38,
            0.25,
        )
        reacquirer.enabled = True
        reacquirer.detector = FakeYoloDetector(objects)
        return reacquirer

    def test_accepts_plausible_pose_center_on_rod(self):
        obj = FakeYoloObject(308, 228, 24, 24, points=[320, 240])
        candidate = self.make_reacquirer([obj]).detect(FakeSizedImage())
        self.assertIsNotNone(candidate)
        self.assertEqual(candidate["source"], "yolo")
        self.assertEqual(candidate["section"], "right")

    def test_rejects_huge_box_and_keypoint_outside_box(self):
        huge = FakeYoloObject(270, 180, 100, 100, points=[320, 230])
        outside = FakeYoloObject(308, 228, 24, 24, points=[500, 240])
        candidate = self.make_reacquirer([huge, outside]).detect(FakeSizedImage())
        self.assertIsNone(candidate)


if __name__ == "__main__":
    unittest.main()
