class AlphaBetaTracker:
    def __init__(
        self,
        alpha=0.72,
        beta=0.16,
        min_gate_cm=1.2,
        max_gate_cm=7.0,
        max_speed_cm_s=120.0,
        reacquire_after_ms=70,
        valid_hold_ms=80,
    ):
        self.alpha = float(alpha)
        self.beta = float(beta)
        self.min_gate_cm = float(min_gate_cm)
        self.max_gate_cm = float(max_gate_cm)
        self.max_speed_cm_s = float(max_speed_cm_s)
        self.reacquire_after_ms = int(reacquire_after_ms)
        self.valid_hold_ms = int(valid_hold_ms)
        self.reset()

    def reset(self):
        self.x_cm = 0.0
        self.v_cm_s = 0.0
        self.state_time_ms = None
        self.last_measurement_ms = None
        self.last_measurement_x_cm = None
        self.confidence = 0.0
        self.measurement_count = 0

    def initialized(self):
        return self.state_time_ms is not None

    def _advance(self, now_ms):
        now_ms = int(now_ms)
        if self.state_time_ms is None:
            return 0.0
        dt = max(0.0, min((now_ms - self.state_time_ms) / 1000.0, 0.25))
        self.x_cm += self.v_cm_s * dt
        self.state_time_ms = now_ms
        return dt

    def predict(self, now_ms):
        if not self.initialized():
            return None
        dt = max(0.0, min((int(now_ms) - self.state_time_ms) / 1000.0, 0.25))
        return self.x_cm + self.v_cm_s * dt

    def measurement_age_ms(self, now_ms):
        if self.last_measurement_ms is None:
            return 1 << 30
        return max(0, int(now_ms) - self.last_measurement_ms)

    def search_gate_cm(self, now_ms):
        if not self.initialized():
            return self.max_gate_cm
        age_s = self.measurement_age_ms(now_ms) / 1000.0
        dynamic = self.min_gate_cm + abs(self.v_cm_s) * age_s + 40.0 * age_s * age_s
        return min(self.max_gate_cm, max(self.min_gate_cm, dynamic))

    def should_reacquire(self, now_ms):
        return (not self.initialized()) or self.measurement_age_ms(now_ms) >= self.reacquire_after_ms

    def update(self, measurement_cm, now_ms, confidence):
        now_ms = int(now_ms)
        measurement_cm = float(measurement_cm)
        confidence = max(0.0, min(float(confidence), 1.0))
        if not self.initialized():
            self.x_cm = measurement_cm
            self.v_cm_s = 0.0
            self.state_time_ms = now_ms
        else:
            previous_time_ms = self.state_time_ms
            dt = max(0.001, min((now_ms - previous_time_ms) / 1000.0, 0.25))
            self._advance(now_ms)
            residual = measurement_cm - self.x_cm
            alpha = min(0.95, max(0.35, self.alpha * (0.55 + 0.45 * confidence)))
            beta = min(0.45, max(0.04, self.beta * (0.55 + 0.45 * confidence)))
            self.x_cm += alpha * residual
            self.v_cm_s += beta * residual / dt
            self.v_cm_s = max(-self.max_speed_cm_s, min(self.max_speed_cm_s, self.v_cm_s))
        self.last_measurement_ms = now_ms
        self.last_measurement_x_cm = measurement_cm
        self.confidence = confidence
        self.measurement_count += 1

    def reacquire(self, measurement_cm, now_ms, confidence):
        measurement_cm = float(measurement_cm)
        now_ms = int(now_ms)
        velocity = 0.0
        if self.last_measurement_ms is not None and self.last_measurement_x_cm is not None:
            dt_s = (now_ms - self.last_measurement_ms) / 1000.0
            if dt_s >= 0.005:
                velocity = (measurement_cm - self.last_measurement_x_cm) / dt_s
        self.x_cm = measurement_cm
        self.v_cm_s = max(
            -self.max_speed_cm_s,
            min(self.max_speed_cm_s, velocity),
        )
        self.state_time_ms = now_ms
        self.last_measurement_ms = now_ms
        self.last_measurement_x_cm = measurement_cm
        self.confidence = max(0.0, min(float(confidence), 1.0))
        self.measurement_count += 1

    def miss(self, now_ms):
        if self.initialized():
            self._advance(now_ms)
            self.confidence *= 0.90

    def snapshot(self, now_ms):
        age_ms = self.measurement_age_ms(now_ms)
        return {
            "initialized": self.initialized(),
            "valid": self.initialized() and age_ms <= self.valid_hold_ms,
            "x_cm": self.predict(now_ms) if self.initialized() else 0.0,
            "v_cm_s": self.v_cm_s if self.initialized() else 0.0,
            "confidence": self.confidence if self.initialized() else 0.0,
            "measurement_age_ms": age_ms,
        }
