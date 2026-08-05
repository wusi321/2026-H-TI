def position_error_cm(candidate, predicted_cm):
    if candidate is None or predicted_cm is None:
        return 0.0
    return abs(float(candidate["x_cm"]) - float(predicted_cm))


def is_far_candidate(candidate, predicted_cm, gate_cm):
    if candidate is None or predicted_cm is None or gate_cm is None:
        return False
    return position_error_cm(candidate, predicted_cm) > float(gate_cm)


def candidate_requires_confirmation(candidate, predicted_cm, gate_cm):
    if candidate is None:
        return False
    if candidate.get("require_confirmation", False):
        return True
    if candidate.get("verified", False):
        return False
    if predicted_cm is None:
        return True
    return is_far_candidate(candidate, predicted_cm, gate_cm)


def should_run_yolo(
    frame_index,
    blob_candidate,
    predicted_cm,
    gate_cm,
    heartbeat_interval,
    burst_interval,
):
    heartbeat = int(frame_index) % max(1, int(heartbeat_interval)) == 0
    burst = (
        blob_candidate is None
        and int(frame_index) % max(1, int(burst_interval)) == 0
    )
    return heartbeat or burst or is_far_candidate(
        blob_candidate, predicted_cm, gate_cm
    )


def should_hold_tracked_blob(blob_candidate, predicted_cm, gate_cm):
    return (
        blob_candidate is not None
        and predicted_cm is not None
        and not is_far_candidate(blob_candidate, predicted_cm, gate_cm)
    )


def yolo_conflict_is_strong(yolo_candidate, minimum_score):
    if yolo_candidate is None:
        return False
    return float(yolo_candidate.get("score", 0.0)) >= float(minimum_score)


def fuse_candidates(blob_candidate, yolo_candidate, agreement_cm):
    if blob_candidate is None:
        if yolo_candidate is None:
            return None, "none"
        result = dict(yolo_candidate)
        result["verified"] = False
        result["fusion"] = "yolo_only"
        return result, "yolo_only"

    result = dict(blob_candidate)
    result["verified"] = False
    if yolo_candidate is None:
        result["fusion"] = "blob_only"
        return result, "blob_only"

    difference = abs(
        float(blob_candidate["x_cm"]) - float(yolo_candidate["x_cm"])
    )
    result["yolo_x_cm"] = float(yolo_candidate["x_cm"])
    result["yolo_difference_cm"] = difference
    if difference <= float(agreement_cm):
        result["source"] = "blob+yolo"
        result["verified"] = True
        result["fusion"] = "agree"
        result["score"] = max(
            float(blob_candidate.get("score", 0.0)),
            float(yolo_candidate.get("score", 0.0)),
        )
        return result, "agree"

    result["fusion"] = "disagree"
    return result, "disagree"


def make_pending_yolo_candidate(yolo_candidate):
    if yolo_candidate is None:
        return None
    result = dict(yolo_candidate)
    result["verified"] = False
    result["fusion"] = "yolo_pending"
    result["require_confirmation"] = True
    return result
