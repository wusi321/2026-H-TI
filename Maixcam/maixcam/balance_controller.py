class BalanceController:
    def __init__(
        self,
        target_x_cm,
        kp_deg_per_cm,
        kd_deg_per_cm_s,
        deadband_cm,
        max_tilt_deg,
        max_slew_deg_s,
        direction=1.0,
    ):
        self.target_x_cm = float(target_x_cm)
        self.kp = float(kp_deg_per_cm)
        self.kd = float(kd_deg_per_cm_s)
        self.deadband_cm = max(0.0, float(deadband_cm))
        self.max_tilt_deg = abs(float(max_tilt_deg))
        self.max_slew_deg_s = abs(float(max_slew_deg_s))
        self.direction = 1.0 if float(direction) >= 0.0 else -1.0
        self.tilt_deg = 0.0
        self.last_ms = None

    @staticmethod
    def _clamp(value, low, high):
        return max(low, min(high, float(value)))

    def set_target(self, target_x_cm):
        self.target_x_cm = float(target_x_cm)

    def update(self, state, now_ms):
        now_ms = int(now_ms)
        valid = bool(state.get("valid", False))
        x_cm = float(state.get("x_cm", 0.0))
        velocity = float(state.get("v_cm_s", 0.0))
        error = self.target_x_cm - x_cm

        if not valid:
            desired = 0.0
            reason = "vision_lost"
        else:
            position_error = 0.0 if abs(error) <= self.deadband_cm else error
            desired = self.direction * (
                self.kp * position_error - self.kd * velocity
            )
            desired = self._clamp(
                desired,
                -self.max_tilt_deg,
                self.max_tilt_deg,
            )
            reason = "tracking"

        if self.last_ms is None:
            dt_s = 0.0
        else:
            dt_s = max(0.0, min((now_ms - self.last_ms) / 1000.0, 0.1))
        self.last_ms = now_ms

        if dt_s > 0.0 and self.max_slew_deg_s > 0.0:
            max_change = self.max_slew_deg_s * dt_s
            desired = self._clamp(
                desired,
                self.tilt_deg - max_change,
                self.tilt_deg + max_change,
            )
        elif dt_s == 0.0:
            desired = 0.0

        self.tilt_deg = desired
        return {
            "enabled": True,
            "valid": valid,
            "target_x_cm": self.target_x_cm,
            "error_cm": error,
            "tilt_deg": self.tilt_deg,
            "reason": reason,
        }
