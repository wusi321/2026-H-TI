class CandidateConfirmation:
    def __init__(
        self,
        required_frames=2,
        max_move_cm=1.8,
        max_diameter_change_px=8.0,
        max_gap_ms=120,
    ):
        self.required_frames = max(1, int(required_frames))
        self.max_move_cm = float(max_move_cm)
        self.max_diameter_change_px = float(max_diameter_change_px)
        self.max_gap_ms = int(max_gap_ms)
        self.reset()

    def reset(self):
        self.count = 0
        self.x_cm = 0.0
        self.diameter_px = 0.0
        self.source = None
        self.last_ms = None

    def update(self, candidate, now_ms):
        now_ms = int(now_ms)
        if candidate is None:
            if self.last_ms is not None and now_ms - self.last_ms > self.max_gap_ms:
                self.reset()
            return False
        if self.last_ms is not None and now_ms - self.last_ms > self.max_gap_ms:
            self.reset()
        x_cm = float(candidate["x_cm"])
        diameter = float(candidate.get("diameter_px", candidate["radius_px"] * 2.0))
        source = candidate.get("source", "unknown")
        if self.count == 0:
            self.count = 1
        elif (
            abs(x_cm - self.x_cm) <= self.max_move_cm
            and abs(diameter - self.diameter_px) <= self.max_diameter_change_px
        ):
            self.count += 1
        else:
            self.count = 1
        self.x_cm = x_cm
        self.diameter_px = diameter
        self.source = source
        self.last_ms = now_ms
        return self.count >= self.required_frames
