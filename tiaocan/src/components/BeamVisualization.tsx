import { useEffect, useRef } from "react";
import type { TelemetryValues } from "../types";

interface BeamVisualizationProps {
  sample: TelemetryValues | null;
  simulatedPosition?: number;
  simulatedServo?: number;
}

export function BeamVisualization({
  sample,
  simulatedPosition,
  simulatedServo,
}: BeamVisualizationProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const draw = () => {
      const bounds = canvas.getBoundingClientRect();
      const ratio = window.devicePixelRatio || 1;
      canvas.width = Math.max(1, Math.round(bounds.width * ratio));
      canvas.height = Math.max(1, Math.round(bounds.height * ratio));
      const context = canvas.getContext("2d");
      if (!context) return;
      context.setTransform(ratio, 0, 0, ratio, 0, 0);
      context.clearRect(0, 0, bounds.width, bounds.height);
      context.fillStyle = "#f8faf9";
      context.fillRect(0, 0, bounds.width, bounds.height);

      const actualPosition = sample?.ballPositionCm ?? 0;
      const target = sample?.targetPositionCm ?? 0;
      const servo = simulatedServo ?? sample?.servoAngleDeg ?? 0;
      const ballPosition = simulatedPosition ?? actualPosition;
      const left = Math.max(54, bounds.width * 0.12);
      const right = Math.min(bounds.width - 76, bounds.width * 0.82);
      const baseline = bounds.height * 0.56;
      const beamLength = right - left;
      const rackTravel = 17 * Math.sin(servo * Math.PI / 180);
      const beamAngle = Math.atan2(-rackTravel, 250);
      const endpoint = {
        x: left + Math.cos(beamAngle) * beamLength,
        y: baseline + Math.sin(beamAngle) * beamLength,
      };
      const pointOnBeam = (positionCm: number) => {
        const normalized = (positionCm + 12.5) / 25;
        return {
          x: left + Math.cos(beamAngle) * beamLength * normalized,
          y: baseline + Math.sin(beamAngle) * beamLength * normalized,
        };
      };

      context.strokeStyle = "#cbd2ce";
      context.lineWidth = 1;
      context.beginPath();
      context.moveTo(left, baseline + 42);
      context.lineTo(endpoint.x + 38, baseline + 42);
      context.stroke();
      for (const mark of [-10, -5, 0, 5, 10]) {
        const point = pointOnBeam(mark);
        context.beginPath();
        context.moveTo(point.x, point.y - 11);
        context.lineTo(point.x, point.y + 11);
        context.stroke();
        context.fillStyle = "#66706a";
        context.font = "11px system-ui";
        context.textAlign = "center";
        context.fillText(`${mark}`, point.x, point.y + 28);
      }

      const targetPoint = pointOnBeam(target);
      context.strokeStyle = "#16805b";
      context.lineWidth = 2;
      context.setLineDash([4, 4]);
      context.beginPath();
      context.moveTo(targetPoint.x, targetPoint.y - 28);
      context.lineTo(targetPoint.x, targetPoint.y + 28);
      context.stroke();
      context.setLineDash([]);

      context.strokeStyle = "#303735";
      context.lineWidth = 16;
      context.lineCap = "round";
      context.beginPath();
      context.moveTo(left, baseline);
      context.lineTo(endpoint.x, endpoint.y);
      context.stroke();
      context.strokeStyle = "#e7ece9";
      context.lineWidth = 9;
      context.beginPath();
      context.moveTo(left, baseline);
      context.lineTo(endpoint.x, endpoint.y);
      context.stroke();

      const ball = pointOnBeam(ballPosition);
      context.fillStyle = "#b8c0bc";
      context.strokeStyle = "#4c5551";
      context.lineWidth = 2;
      context.beginPath();
      context.arc(ball.x, ball.y - 9, 9, 0, Math.PI * 2);
      context.fill();
      context.stroke();

      context.fillStyle = "#303735";
      context.beginPath();
      context.arc(left, baseline + 4, 8, 0, Math.PI * 2);
      context.fill();
      context.strokeStyle = "#7a847f";
      context.lineWidth = 5;
      context.beginPath();
      context.moveTo(endpoint.x, endpoint.y + 4);
      context.lineTo(endpoint.x, baseline + 42);
      context.stroke();

      const gearCenter = { x: endpoint.x + 35, y: baseline + 30 };
      context.strokeStyle = "#a36d2f";
      context.lineWidth = 4;
      context.beginPath();
      context.arc(gearCenter.x, gearCenter.y, 18, 0, Math.PI * 2);
      context.stroke();
      context.beginPath();
      context.moveTo(gearCenter.x, gearCenter.y);
      context.lineTo(
        gearCenter.x + 16 * Math.cos(servo * Math.PI / 180),
        gearCenter.y + 16 * Math.sin(servo * Math.PI / 180),
      );
      context.stroke();

      context.fillStyle = "#202523";
      context.font = "600 13px system-ui";
      context.textAlign = "left";
      context.fillText(`球 ${ballPosition.toFixed(2)} cm`, 18, 24);
      context.fillText(`舵机 ${servo.toFixed(2)} deg`, 18, 44);
      context.fillStyle = "#66706a";
      context.font = "11px system-ui";
      context.fillText("25 cm", (left + endpoint.x) / 2 - 14, baseline + 60);
    };
    draw();
    const observer = new ResizeObserver(draw);
    observer.observe(canvas);
    return () => observer.disconnect();
  }, [sample, simulatedPosition, simulatedServo]);

  return <canvas ref={canvasRef} className="beam-canvas" aria-label="滚球机构状态" />;
}
