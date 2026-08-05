class ServoPwmOutput:
    def __init__(
        self,
        pin_name,
        frequency_hz,
        center_duty,
        min_duty,
        max_duty,
        duty_per_rod_deg,
        direction=1.0,
    ):
        if pin_name is None:
            raise ValueError("SERVO_PWM_PIN is not configured")
        values = (center_duty, min_duty, max_duty, duty_per_rod_deg)
        if any(value is None for value in values):
            raise ValueError("servo duty calibration is incomplete")
        self.pin_name = str(pin_name)
        self.frequency_hz = int(frequency_hz)
        self.center_duty = float(center_duty)
        self.min_duty = float(min_duty)
        self.max_duty = float(max_duty)
        self.duty_per_rod_deg = abs(float(duty_per_rod_deg))
        self.direction = 1.0 if float(direction) >= 0.0 else -1.0
        if not self.min_duty < self.center_duty < self.max_duty:
            raise ValueError("servo duty limits must contain center duty")
        if self.duty_per_rod_deg <= 0.0:
            raise ValueError("SERVO_DUTY_PER_ROD_DEG must be positive")

        from maix import pinmap, pwm

        functions = pinmap.get_pin_functions(self.pin_name)
        pwm_function = None
        pwm_id = None
        for function in functions:
            index = function.find("PWM")
            if index >= 0:
                pwm_function = function
                pwm_id = int(function[index + 3:])
                break
        if pwm_function is None:
            raise ValueError("pin {} has no PWM function".format(self.pin_name))
        if pinmap.set_pin_function(self.pin_name, pwm_function) != 0:
            raise RuntimeError("failed to map {} to {}".format(self.pin_name, pwm_function))
        self.pwm = pwm.PWM(
            pwm_id,
            freq=self.frequency_hz,
            duty=self.center_duty,
            enable=True,
        )
        self.last_duty = self.center_duty
        print(
            "servo PWM ready: {} {} {}Hz center:{:.3f}%".format(
                self.pin_name,
                pwm_function,
                self.frequency_hz,
                self.center_duty,
            )
        )

    def command_tilt(self, rod_tilt_deg):
        duty = self.center_duty + (
            self.direction * float(rod_tilt_deg) * self.duty_per_rod_deg
        )
        duty = max(self.min_duty, min(self.max_duty, duty))
        self.pwm.duty(duty)
        self.last_duty = duty
        return duty

    def close(self):
        self.pwm.duty(self.center_duty)
        self.pwm.disable()
