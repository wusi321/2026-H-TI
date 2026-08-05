import type { CSSProperties } from "react";

interface VehicleSchematicProps {
  leftSpeed: number;
  rightSpeed: number;
  setpoint: number;
  yaw: number;
}

function wheelStyle(speed: number): CSSProperties {
  const duration = Math.max(0.18, 1.7 / Math.max(1, Math.abs(speed)));
  return {
    animationDuration: `${duration}s`,
    animationDirection: speed < 0 ? "reverse" : "normal",
    animationPlayState: Math.abs(speed) < 0.2 ? "paused" : "running",
  };
}

function WheelReadout({ side, speed }: { side: "L" | "R"; speed: number }) {
  const magnitude = Math.min(100, Math.abs(speed) / 1.2);
  return (
    <div className="wheel-readout">
      <div className="wheel-readout__heading">
        <span className={`wheel-side wheel-side--${side.toLowerCase()}`}>{side}</span>
        <strong>{speed.toFixed(1)}</strong>
        <span>cm/s</span>
      </div>
      <div className="wheel-speed-track">
        <span
          className={speed < 0 ? "is-reverse" : ""}
          style={{ width: `${magnitude}%` }}
        />
      </div>
    </div>
  );
}

export function VehicleSchematic({
  leftSpeed,
  rightSpeed,
  setpoint,
  yaw,
}: VehicleSchematicProps) {
  return (
    <div className="vehicle-layout">
      <div className="vehicle-visual" aria-label="小车俯视运动状态">
        <div className="vehicle-heading" style={{ transform: `rotate(${yaw}deg)` }}>
          <span />
        </div>
        <div className="vehicle-wheel vehicle-wheel--left-front" style={wheelStyle(leftSpeed)} />
        <div className="vehicle-wheel vehicle-wheel--left-rear" style={wheelStyle(leftSpeed)} />
        <div className="vehicle-body">
          <div className="vehicle-nose" />
          <div className="vehicle-controller">NC</div>
          <div className="vehicle-setpoint">
            <strong>{setpoint.toFixed(1)}</strong>
            <span>cm/s</span>
          </div>
        </div>
        <div className="vehicle-wheel vehicle-wheel--right-front" style={wheelStyle(rightSpeed)} />
        <div className="vehicle-wheel vehicle-wheel--right-rear" style={wheelStyle(rightSpeed)} />
      </div>
      <div className="wheel-readouts">
        <WheelReadout side="L" speed={leftSpeed} />
        <WheelReadout side="R" speed={rightSpeed} />
      </div>
    </div>
  );
}
