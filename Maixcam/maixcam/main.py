from maix import app, camera, display, image, time

import config
from balance_controller import BalanceController
from ball_detector import BallDetector
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
from position_calibration import PositionCalibration
from state_estimator import AlphaBetaTracker
from servo_output import ServoPwmOutput
from uart_transport import write_all
from vision_protocol import encode_measurement
from web_stream import native_webrtc_available, start_native_webrtc
from yolo_reacquire import YoloReacquirer


EXPECTED_CONFIG_VERSION = "2026-08-01-h-ball-simple-13"


def validate_project_files():
    actual = getattr(config, "VERSION", "unknown")
    if actual != EXPECTED_CONFIG_VERSION:
        raise RuntimeError(
            "project files mismatch: expected config {} got {}. "
            "Open the complete maixcam folder and run the project.".format(
                EXPECTED_CONFIG_VERSION,
                actual,
            )
        )


def elapsed_ms(start_raw_ms, end_raw_ms=None):
    if end_raw_ms is None:
        end_raw_ms = time.ticks_ms()
    return max(0, int(time.ticks_diff(start_raw_ms, end_raw_ms)))


def read_camera_frame(cam):
    try:
        return cam.read(block=True, block_ms=config.CAMERA_READ_BLOCK_MS), None
    except TypeError:
        try:
            return cam.read(), None
        except Exception as exc:
            return None, exc
    except Exception as exc:
        return None, exc


def enable_fill_light():
    from maix import gpio, pinmap

    result = pinmap.set_pin_function(
        config.FILL_LIGHT_PIN,
        config.FILL_LIGHT_FUNCTION,
    )
    if result != 0:
        raise RuntimeError(
            "fill light pinmap failed: {} -> {} result={}".format(
                config.FILL_LIGHT_PIN,
                config.FILL_LIGHT_FUNCTION,
                result,
            )
        )
    fill_light = gpio.GPIO(config.FILL_LIGHT_FUNCTION, gpio.Mode.OUT)
    fill_light.value(config.FILL_LIGHT_ACTIVE_LEVEL)
    print("fill light enabled: {} high".format(config.FILL_LIGHT_PIN))
    return fill_light


def detector_settings():
    return {
        "thresholds": config.LAB_THRESHOLDS,
        "adaptive_l_enabled": config.ADAPTIVE_L_ENABLED,
        "adaptive_l_interval_frames": config.ADAPTIVE_L_INTERVAL_FRAMES,
        "adaptive_l_warmup_samples": config.ADAPTIVE_L_WARMUP_SAMPLES,
        "adaptive_l_gain": config.ADAPTIVE_L_GAIN,
        "adaptive_l_smoothing": config.ADAPTIVE_L_SMOOTHING,
        "adaptive_l_min": config.ADAPTIVE_L_MIN,
        "adaptive_l_max": config.ADAPTIVE_L_MAX,
        "adaptive_l_bins": config.ADAPTIVE_L_BINS,
        "x_stride": config.BLOB_X_STRIDE,
        "y_stride": config.BLOB_Y_STRIDE,
        "area_threshold": config.BLOB_AREA_THRESHOLD,
        "pixels_threshold": config.BLOB_PIXELS_THRESHOLD,
        "merge": config.BLOB_MERGE,
        "margin": config.BLOB_MARGIN,
        "min_diameter_px": config.BALL_MIN_DIAMETER_PX,
        "max_diameter_px": config.BALL_MAX_DIAMETER_PX,
        "expected_diameter_px": config.BALL_EXPECTED_DIAMETER_PX,
        "min_aspect": config.BALL_MIN_ASPECT,
        "max_aspect": config.BALL_MAX_ASPECT,
        "roundness_reference": config.BALL_ROUNDNESS_REFERENCE,
        "min_density": config.BALL_MIN_DENSITY,
        "max_density": config.BALL_MAX_DENSITY,
        "expected_density": config.BALL_EXPECTED_DENSITY,
        "density_tolerance": config.BALL_DENSITY_TOLERANCE,
        "max_centroid_offset_ratio": config.BALL_MAX_CENTROID_OFFSET_RATIO,
        "max_axis_distance_px": config.BALL_MAX_AXIS_DISTANCE_PX,
        "track_roi_padding_px": config.TRACK_ROI_PADDING_PX,
        "min_score_search": config.BALL_MIN_SCORE_SEARCH,
        "min_score_track": config.BALL_MIN_SCORE_TRACK,
        "diagnostic_limit": config.BALL_DIAGNOSTIC_LIMIT,
        "bbox_center_weight": config.BALL_BBOX_CENTER_WEIGHT,
    }


def create_tracker():
    return AlphaBetaTracker(
        alpha=config.TRACK_ALPHA,
        beta=config.TRACK_BETA,
        min_gate_cm=config.TRACK_MIN_GATE_CM,
        max_gate_cm=config.TRACK_MAX_GATE_CM,
        max_speed_cm_s=config.TRACK_MAX_SPEED_CM_S,
        reacquire_after_ms=config.TRACK_REACQUIRE_AFTER_MS,
        valid_hold_ms=config.TRACK_VALID_HOLD_MS,
    )


def create_servo_control(calibration):
    if not config.ENABLE_SERVO_CONTROL:
        return None, None
    if not calibration.calibrated:
        raise RuntimeError("servo control refused: position calibration is placeholder")
    controller = BalanceController(
        config.CONTROL_TARGET_X_CM,
        config.CONTROL_KP_DEG_PER_CM,
        config.CONTROL_KD_DEG_PER_CM_S,
        config.CONTROL_DEADBAND_CM,
        config.CONTROL_MAX_TILT_DEG,
        config.CONTROL_MAX_SLEW_DEG_S,
        config.CONTROL_DIRECTION,
    )
    output = ServoPwmOutput(
        config.SERVO_PWM_PIN,
        config.SERVO_PWM_FREQUENCY_HZ,
        config.SERVO_CENTER_DUTY,
        config.SERVO_MIN_DUTY,
        config.SERVO_MAX_DUTY,
        config.SERVO_DUTY_PER_ROD_DEG,
        config.SERVO_DIRECTION,
    )
    if config.SERVO_PWM_PIN in ("A16", "A17", "A18", "A19"):
        print(
            "WARNING: direct PWM on {} may conflict with Wi-Fi or MaixVision pin functions".format(
                config.SERVO_PWM_PIN
            )
        )
    return controller, output


def create_uart(calibration):
    if not config.ENABLE_UART:
        return None
    if not calibration.calibrated:
        if config.UART_ALLOW_PLACEHOLDER_CONTROL:
            print("WARNING: UART bench control uses placeholder calibration")
        else:
            print(
                "WARNING: UART test mode; calibration is placeholder, "
                "all packets will have VALID=0"
            )
    try:
        from maix import pinmap, uart

        tx_result = pinmap.set_pin_function(config.UART_TX_PIN, config.UART_TX_FUNCTION)
        rx_result = pinmap.set_pin_function(config.UART_RX_PIN, config.UART_RX_FUNCTION)
        if tx_result != 0 or rx_result != 0:
            raise RuntimeError("pinmap failed: tx={} rx={}".format(tx_result, rx_result))
        serial = uart.UART(config.UART_DEVICE, config.UART_BAUD)
        print("vision UART ready: {} @ {}".format(config.UART_DEVICE, config.UART_BAUD))
        return serial
    except Exception as exc:
        print("vision UART disabled:", exc)
        return None


def apply_camera_settings(cam):
    if config.MANUAL_EXPOSURE_US is not None:
        cam.exposure(int(config.MANUAL_EXPOSURE_US))
        print("manual exposure us:", config.MANUAL_EXPOSURE_US)
    if config.MANUAL_GAIN is not None:
        cam.gain(int(config.MANUAL_GAIN))
        print("manual gain:", config.MANUAL_GAIN)


def create_camera_pipeline():
    if config.ENABLE_WEBRTC and native_webrtc_available():
        vision_cam = camera.Camera(
            config.CAMERA_WIDTH,
            config.CAMERA_HEIGHT,
            image.Format.FMT_RGB888,
            fps=config.CAMERA_FPS,
            buff_num=config.WEBRTC_VISION_BUFFERS,
        )
        stream_cam = vision_cam.add_channel(
            config.WEBRTC_STREAM_WIDTH,
            config.WEBRTC_STREAM_HEIGHT,
            image.Format.FMT_YVU420SP,
            fps=config.WEBRTC_STREAM_FPS,
            buff_num=config.WEBRTC_STREAM_BUFFERS,
        )
        apply_camera_settings(vision_cam)
        stream_server = start_native_webrtc(
            stream_cam,
            config.WEBRTC_BITRATE,
            config.WEBRTC_GOP,
        )
        print(
            "WebRTC stream:{}x{}@{} vision:{}x{}@{}".format(
                config.WEBRTC_STREAM_WIDTH,
                config.WEBRTC_STREAM_HEIGHT,
                config.WEBRTC_STREAM_FPS,
                config.CAMERA_WIDTH,
                config.CAMERA_HEIGHT,
                config.CAMERA_FPS,
            )
        )
        return stream_cam, vision_cam, stream_server, "webrtc"

    if config.ENABLE_WEBRTC:
        print(
            "WebRTC unavailable in this MaixPy firmware; "
            "trying the RTSP fallback."
        )

    if config.ENABLE_RTSP:
        from maix import rtsp

        stream_cam = camera.Camera(
            config.CAMERA_WIDTH,
            config.CAMERA_HEIGHT,
            image.Format.FMT_YVU420SP,
            fps=config.CAMERA_FPS,
            buff_num=config.RTSP_STREAM_BUFFERS,
        )
        vision_cam = stream_cam.add_channel(
            config.CAMERA_WIDTH,
            config.CAMERA_HEIGHT,
            image.Format.FMT_RGB888,
            config.CAMERA_FPS,
            config.RTSP_VISION_BUFFERS,
        )
        apply_camera_settings(stream_cam)
        stream_server = rtsp.Rtsp()
        stream_server.bind_camera(stream_cam)
        stream_server.start()
        print(
            "RTSP buffers: stream={} vision={}".format(
                config.RTSP_STREAM_BUFFERS,
                config.RTSP_VISION_BUFFERS,
            )
        )
        print("RTSP URLs:", stream_server.get_urls())
        return stream_cam, vision_cam, stream_server, "rtsp"

    vision_cam = camera.Camera(
        config.CAMERA_WIDTH,
        config.CAMERA_HEIGHT,
        fps=config.CAMERA_FPS,
        buff_num=config.CAMERA_BUFFERS,
    )
    apply_camera_settings(vision_cam)
    return vision_cam, vision_cam, None, "disabled"


def format_candidate_details(details):
    parts = []
    for item in details:
        parts.append(
            "{}x{} p:{} r:{:.2f} d:{:.2f} ar:{:.2f} c:{:.2f} y:{:.1f} s:{:.2f} {}".format(
                item["width"],
                item["height"],
                item["pixels"],
                item["roundness"],
                item["density"],
                item["aspect"],
                item["centroid_offset"],
                item["axis_distance"],
                item["score"],
                item["reason"].upper(),
            )
        )
    return "[{}]".format("; ".join(parts))


def draw_debug(img, calibration, raw_candidate, accepted_candidate, state, measured, fps):
    roi = calibration.roi
    img.draw_rect(roi[0], roi[1], roi[2], roi[3], image.COLOR_BLUE)
    img.draw_line(
        int(calibration.axis_start[0]),
        int(calibration.axis_start[1]),
        int(calibration.axis_end[0]),
        int(calibration.axis_end[1]),
        image.COLOR_BLUE,
    )

    for x_cm in (-12.5, -10.0, -5.0, 0.0, 5.0, 10.0, 12.5):
        x1, y1, x2, y2 = calibration.tick_segment(
            x_cm,
            10.0 if x_cm == 0.0 else 6.0,
        )
        color = image.COLOR_YELLOW if x_cm == 0.0 else image.COLOR_BLUE
        img.draw_line(int(x1), int(y1), int(x2), int(y2), color)

    draw_candidate = accepted_candidate if accepted_candidate is not None else raw_candidate
    if draw_candidate is not None:
        candidate_color = image.COLOR_GREEN if accepted_candidate is not None else image.COLOR_YELLOW
        rect = draw_candidate["rect"]
        img.draw_rect(rect[0], rect[1], rect[2], rect[3], candidate_color)
        img.draw_circle(
            int(round(draw_candidate["x_px"])),
            int(round(draw_candidate["y_px"])),
            draw_candidate["radius_px"],
            candidate_color,
            2,
        )

    status = "MEAS" if measured else (
        "CAND" if raw_candidate is not None else ("PRED" if state["valid"] else "LOST")
    )
    color = image.COLOR_GREEN if measured else (
        image.COLOR_YELLOW if raw_candidate is not None or state["valid"] else image.COLOR_RED
    )
    img.draw_string(
        6,
        6,
        "{} x:{:+.2f}cm v:{:+.1f}cm/s {:.1f}fps".format(
            status,
            state["x_cm"],
            state["v_cm_s"],
            fps,
        ),
        color,
    )


def main():
    validate_project_files()
    fill_light = enable_fill_light()
    print("ball_balance_simple version:", config.VERSION)
    print(
        "camera:{}x{}@{} display:{} webrtc:{} rtsp_fallback:{} "
        "uart:{} yolo:{} servo:{}".format(
            config.CAMERA_WIDTH,
            config.CAMERA_HEIGHT,
            config.CAMERA_FPS,
            config.ENABLE_DISPLAY,
            config.ENABLE_WEBRTC,
            config.ENABLE_RTSP,
            config.ENABLE_UART,
            config.ENABLE_YOLO_REACQUIRE,
            config.ENABLE_SERVO_CONTROL,
        )
    )
    print(
        "adaptive_l:{} interval:{} warmup:{} range:{}..{}".format(
            config.ADAPTIVE_L_ENABLED,
            config.ADAPTIVE_L_INTERVAL_FRAMES,
            config.ADAPTIVE_L_WARMUP_SAMPLES,
            config.ADAPTIVE_L_MIN,
            config.ADAPTIVE_L_MAX,
        )
    )

    calibration = PositionCalibration.load(config.CALIBRATION_PATH)
    if (
        calibration.image_width != config.CAMERA_WIDTH
        or calibration.image_height != config.CAMERA_HEIGHT
    ):
        raise ValueError("position calibration image size does not match camera")
    if not calibration.calibrated:
        if config.UART_ALLOW_PLACEHOLDER_CONTROL:
            print("WARNING: placeholder calibration; bench UART control is enabled")
        else:
            print("WARNING: placeholder calibration; control measurements are disabled")
    if abs(calibration.rod_length_cm - config.ROD_LENGTH_CM) > 1e-6:
        raise ValueError("rod length in config and calibration does not match")
    if abs(calibration.ball_diameter_cm - config.BALL_DIAMETER_CM) > 1e-6:
        raise ValueError("ball diameter in config and calibration does not match")

    detector = BallDetector(calibration, detector_settings())
    tracker = create_tracker()
    confirmer = CandidateConfirmation(
        config.REACQUIRE_CONFIRM_FRAMES,
        config.REACQUIRE_MAX_MOVE_CM,
        config.REACQUIRE_MAX_DIAMETER_CHANGE_PX,
        config.REACQUIRE_MAX_GAP_MS,
    )
    yolo = YoloReacquirer(
        config.ENABLE_YOLO_REACQUIRE,
        config.YOLO_MODEL_PATHS,
        calibration,
        config.YOLO_CONF_THRESHOLD,
        config.YOLO_IOU_THRESHOLD,
        config.YOLO_KEYPOINT_THRESHOLD,
        config.YOLO_PRESERVE_ASPECT,
        config.YOLO_USE_BEAM_CROP,
        config.YOLO_MIN_DIAMETER_PX,
        config.YOLO_MAX_DIAMETER_PX,
        config.YOLO_MIN_ASPECT,
        config.YOLO_MAX_AXIS_DISTANCE_PX,
        config.YOLO_KEYPOINT_BOX_MARGIN_RATIO,
    )
    serial = create_uart(calibration)
    balance_controller, servo_output = create_servo_control(calibration)
    stream_cam, vision_cam, stream_server, stream_mode = create_camera_pipeline()
    print("network stream mode:", stream_mode)
    screen = display.Display() if config.ENABLE_DISPLAY else None

    frame_index = 0
    sequence = 0
    profile_start_ms = int(time.ticks_ms())
    profile_capture_ms = 0
    profile_detect_ms = 0
    profile_display_ms = 0
    profile_process_ms = 0
    profile_measured = 0
    profile_yolo_ms = 0
    profile_yolo_runs = 0
    profile_lighting_ms = 0
    profile_lighting_runs = 0
    profile_read_timeouts = 0
    camera_read_timeouts_total = 0
    consecutive_read_timeouts = 0
    last_read_timeout_warning_ms = None
    fps = 0.0
    fusion_status = "none"
    yolo_force_frames = 0
    control_state = None

    try:
        while not app.need_exit():
            frame_start_raw = time.ticks_ms()
            img, read_error = read_camera_frame(vision_cam)
            if img is None:
                timeout_now_ms = int(time.ticks_ms())
                camera_read_timeouts_total += 1
                profile_read_timeouts += 1
                consecutive_read_timeouts += 1
                if (
                    last_read_timeout_warning_ms is None
                    or elapsed_ms(last_read_timeout_warning_ms, timeout_now_ms)
                    >= config.CAMERA_READ_WARNING_INTERVAL_MS
                ):
                    print(
                        "camera read retry total:{} consecutive:{} error:{}".format(
                            camera_read_timeouts_total,
                            consecutive_read_timeouts,
                            read_error,
                        )
                    )
                    last_read_timeout_warning_ms = timeout_now_ms
                time.sleep_ms(config.CAMERA_READ_RETRY_MS)
                continue
            consecutive_read_timeouts = 0
            capture_end_raw = time.ticks_ms()
            now_ms = int(time.ticks_ms())

            predicted_cm = tracker.predict(now_ms)
            gate_cm = tracker.search_gate_cm(now_ms)
            reacquiring = tracker.should_reacquire(now_ms)

            detect_start_raw = time.ticks_ms()
            lighting_due = (
                detector.adaptive_l_enabled
                and frame_index % max(1, config.ADAPTIVE_L_INTERVAL_FRAMES) == 0
            )
            lighting_start_raw = time.ticks_ms()
            detector.update_lighting(img, frame_index)
            if lighting_due:
                profile_lighting_ms += elapsed_ms(
                    lighting_start_raw,
                    time.ticks_ms(),
                )
                profile_lighting_runs += 1
            blob_candidate = detector.detect(
                img,
                predicted_cm=predicted_cm,
                gate_cm=gate_cm,
                allow_global=reacquiring,
            )
            global_fallback = False
            if (
                blob_candidate is None
                and not reacquiring
                and config.ENABLE_SAME_FRAME_GLOBAL_FALLBACK
            ):
                blob_candidate = detector.detect(
                    img,
                    predicted_cm=predicted_cm,
                    gate_cm=gate_cm,
                    allow_global=True,
                )
                global_fallback = True

            yolo_candidate = None
            force_yolo = yolo_force_frames > 0
            if (
                config.ENABLE_YOLO_REACQUIRE
                and yolo.enabled
                and (
                    force_yolo
                    or should_run_yolo(
                        frame_index,
                        blob_candidate,
                        predicted_cm,
                        gate_cm,
                        config.YOLO_HEARTBEAT_INTERVAL_FRAMES,
                        config.YOLO_BURST_INTERVAL_FRAMES,
                    )
                )
            ):
                if force_yolo:
                    yolo_force_frames -= 1
                yolo_start_raw = time.ticks_ms()
                yolo_candidate = yolo.detect(img)
                profile_yolo_ms += elapsed_ms(yolo_start_raw, time.ticks_ms())
                profile_yolo_runs += 1

            raw_candidate, fusion_status = fuse_candidates(
                blob_candidate,
                yolo_candidate,
                config.YOLO_AGREEMENT_CM,
            )
            if fusion_status == "disagree" and yolo_candidate is not None:
                # A stale local false candidate can hide a real fast roll.
                # Search the whole groove again, ranking around YOLO's center.
                corroborating_blob = detector.detect(
                    img,
                    predicted_cm=yolo_candidate["x_cm"],
                    gate_cm=config.YOLO_AGREEMENT_CM,
                    allow_global=True,
                )
                global_fallback = True
                corroborated, corroboration_status = fuse_candidates(
                    corroborating_blob,
                    yolo_candidate,
                    config.YOLO_AGREEMENT_CM,
                )
                if corroboration_status == "agree":
                    raw_candidate = corroborated
                    fusion_status = "agree_global"
                    yolo_force_frames = 0
                elif should_hold_tracked_blob(
                    blob_candidate,
                    predicted_cm,
                    gate_cm,
                ):
                    raw_candidate = dict(blob_candidate)
                    raw_candidate["verified"] = False
                    raw_candidate["fusion"] = "hold_tracked_blob"
                    fusion_status = "hold_tracked_blob"
                    yolo_force_frames = max(
                        yolo_force_frames,
                        config.YOLO_CONFLICT_BURST_FRAMES,
                    )
                elif yolo_conflict_is_strong(
                    yolo_candidate,
                    config.YOLO_CONFLICT_CONF_THRESHOLD,
                ):
                    raw_candidate = make_pending_yolo_candidate(yolo_candidate)
                    fusion_status = "yolo_pending"
                    yolo_force_frames = max(
                        yolo_force_frames,
                        config.YOLO_CONFLICT_BURST_FRAMES,
                    )
                else:
                    raw_candidate = None
                    fusion_status = "conflict_rejected"
                    yolo_force_frames = max(
                        yolo_force_frames,
                        config.YOLO_CONFLICT_BURST_FRAMES,
                    )

            accepted_candidate = raw_candidate
            far_candidate = is_far_candidate(raw_candidate, predicted_cm, gate_cm)
            jump_reacquire = bool(
                far_candidate
                and raw_candidate is not None
                and raw_candidate.get("verified", False)
            )
            if candidate_requires_confirmation(raw_candidate, predicted_cm, gate_cm):
                if not confirmer.update(raw_candidate, now_ms):
                    accepted_candidate = None
                else:
                    jump_reacquire = predicted_cm is not None
                    confirmer.reset()
            else:
                confirmer.reset()
            detect_end_raw = time.ticks_ms()

            measured = accepted_candidate is not None
            if measured:
                if jump_reacquire:
                    tracker.reacquire(
                        accepted_candidate["x_cm"],
                        now_ms,
                        accepted_candidate["score"],
                    )
                else:
                    tracker.update(
                        accepted_candidate["x_cm"],
                        now_ms,
                        accepted_candidate["score"],
                    )
                profile_measured += 1
            else:
                tracker.miss(now_ms)
            state = tracker.snapshot(now_ms)
            if balance_controller is not None:
                control_state = balance_controller.update(state, now_ms)
                control_state["duty"] = servo_output.command_tilt(
                    control_state["tilt_deg"]
                )

            processing_ms = elapsed_ms(frame_start_raw, time.ticks_ms())
            if serial is not None:
                try:
                    write_all(
                        serial,
                        encode_measurement(
                            sequence,
                            now_ms,
                            state,
                            measured,
                            processing_us=processing_ms * 1000,
                            force_invalid=(
                                not calibration.calibrated
                                and not config.UART_ALLOW_PLACEHOLDER_CONTROL
                            ),
                        ),
                    )
                except Exception as exc:
                    print("vision UART disabled after write failure:", exc)
                    try:
                        serial.close()
                    except Exception:
                        pass
                    serial = None

            frame_index += 1
            sequence = (sequence + 1) & 0xFFFF
            profile_capture_ms += elapsed_ms(frame_start_raw, capture_end_raw)
            profile_detect_ms += elapsed_ms(detect_start_raw, detect_end_raw)
            profile_process_ms += processing_ms

            if frame_index % config.POSITION_PRINT_EVERY_FRAMES == 0:
                print(
                    "ball valid:{} measured:{} x:{:+.2f}cm part:{} v:{:+.1f}cm/s conf:{:.2f} source:{} fusion:{} px:{}".format(
                        state["valid"],
                        measured,
                        state["x_cm"],
                        calibration.section_for_cm(state["x_cm"]) if state["valid"] else "none",
                        state["v_cm_s"],
                        state["confidence"],
                        accepted_candidate["source"] if measured else "none",
                        fusion_status,
                        "({:.1f},{:.1f})".format(
                            accepted_candidate["x_px"],
                            accepted_candidate["y_px"],
                        ) if measured else "none",
                    )
                )
                if config.BLOB_DIAGNOSTICS:
                    stats = detector.last_stats
                    print(
                        "blob roi:{} raw:{} sizes:{} accepted:{} reject:size{}(small{} large{}) aspect{} density{} centroid{} axis{} score{} fallback:{} light:{} Lmed:{} Lmax:{}".format(
                            stats["roi"],
                            stats["raw"],
                            stats["raw_sizes"],
                            stats["accepted"],
                            stats["reject_size"],
                            stats["reject_size_small"],
                            stats["reject_size_large"],
                            stats["reject_aspect"],
                            stats["reject_density"],
                            stats["reject_centroid"],
                            stats["reject_axis"],
                            stats["reject_score"],
                            global_fallback,
                            "adaptive" if stats["adaptive_l"] else "fixed",
                            "{:.1f}".format(stats["l_median"])
                            if stats["l_median"] is not None
                            else "warmup",
                            stats["l_threshold"],
                        )
                    )
                    if stats["candidate_details"]:
                        print("candidate", format_candidate_details(stats["candidate_details"]))
                if control_state is not None:
                    print(
                        "control target:{:+.2f}cm error:{:+.2f}cm tilt:{:+.2f}deg duty:{:.3f}% {}".format(
                            control_state["target_x_cm"],
                            control_state["error_cm"],
                            control_state["tilt_deg"],
                            control_state["duty"],
                            control_state["reason"],
                        )
                    )

            if screen is not None and frame_index % max(1, config.DISPLAY_EVERY) == 0:
                display_start_raw = time.ticks_ms()
                if config.DRAW_DEBUG:
                    draw_debug(
                        img,
                        calibration,
                        raw_candidate,
                        accepted_candidate,
                        state,
                        measured,
                        fps,
                    )
                screen.show(img)
                profile_display_ms += elapsed_ms(display_start_raw, time.ticks_ms())

            if frame_index % config.PROFILE_EVERY_FRAMES == 0:
                profile_end_ms = int(time.ticks_ms())
                profile_elapsed_ms = max(1, profile_end_ms - profile_start_ms)
                fps = config.PROFILE_EVERY_FRAMES * 1000.0 / profile_elapsed_ms
                print(
                    "profile frame:{} fps:{:.1f} measured:{}/{} process:{:.1f}ms capture:{:.1f}ms detect:{:.1f}ms light:{:.1f}ms/{} yolo:{:.1f}ms/{} display:{:.1f}ms read_timeout:{}/{} valid:{} age:{}ms".format(
                        frame_index,
                        fps,
                        profile_measured,
                        config.PROFILE_EVERY_FRAMES,
                        profile_process_ms / config.PROFILE_EVERY_FRAMES,
                        profile_capture_ms / config.PROFILE_EVERY_FRAMES,
                        profile_detect_ms / config.PROFILE_EVERY_FRAMES,
                        profile_lighting_ms / max(1, profile_lighting_runs),
                        profile_lighting_runs,
                        profile_yolo_ms / max(1, profile_yolo_runs),
                        profile_yolo_runs,
                        profile_display_ms / config.PROFILE_EVERY_FRAMES,
                        profile_read_timeouts,
                        camera_read_timeouts_total,
                        state["valid"],
                        state["measurement_age_ms"],
                    )
                )
                profile_start_ms = profile_end_ms
                profile_capture_ms = 0
                profile_detect_ms = 0
                profile_display_ms = 0
                profile_process_ms = 0
                profile_measured = 0
                profile_yolo_ms = 0
                profile_yolo_runs = 0
                profile_lighting_ms = 0
                profile_lighting_runs = 0
                profile_read_timeouts = 0
    finally:
        if serial is not None:
            try:
                serial.close()
            except Exception:
                pass
        if servo_output is not None:
            servo_output.close()
        if stream_server is not None:
            try:
                stream_server.stop()
            except Exception:
                pass
        if vision_cam is not stream_cam:
            try:
                vision_cam.close()
            except Exception:
                pass
        try:
            stream_cam.close()
        except Exception:
            pass
        print("ball_balance_simple stopped")


if __name__ == "__main__":
    main()
