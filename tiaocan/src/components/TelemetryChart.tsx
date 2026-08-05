import { useEffect, useRef } from "react";
import type { SimulationSample, TelemetrySample } from "../types";

interface TelemetryChartProps {
  samples: TelemetrySample[];
  simulation: SimulationSample[];
  cursorMs?: number;
}

const COLORS = {
  grid: "#dfe4e1",
  text: "#66706a",
  actual: "#202523",
  target: "#16805b",
  simulated: "#d65c3d",
  velocity: "#3577a8",
  cursor: "#b98a26",
};

export function TelemetryChart({ samples, simulation, cursorMs }: TelemetryChartProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const render = () => {
      const bounds = canvas.getBoundingClientRect();
      const ratio = window.devicePixelRatio || 1;
      canvas.width = Math.max(1, Math.round(bounds.width * ratio));
      canvas.height = Math.max(1, Math.round(bounds.height * ratio));
      const context = canvas.getContext("2d");
      if (!context) return;
      context.setTransform(ratio, 0, 0, ratio, 0, 0);
      const width = bounds.width;
      const height = bounds.height;
      const plot = { left: 48, right: width - 46, top: 24, bottom: height - 34 };
      context.clearRect(0, 0, width, height);
      context.fillStyle = "#ffffff";
      context.fillRect(0, 0, width, height);

      if (samples.length < 2) {
        context.fillStyle = COLORS.text;
        context.font = "13px system-ui";
        context.textAlign = "center";
        context.fillText("等待串口数据或选择历史记录", width / 2, height / 2);
        return;
      }

      const visible = samples.length > 2400 ? samples.slice(-2400) : samples;
      const startMs = visible[0].elapsedMs;
      const endMs = Math.max(startMs + 1000, visible.at(-1)!.elapsedMs);
      const x = (time: number) => plot.left
        + (time - startMs) / (endMs - startMs) * (plot.right - plot.left);
      const positionExtent = Math.max(
        6,
        ...visible.flatMap((sample) => [
          Math.abs(sample.ballPositionCm),
          Math.abs(sample.targetPositionCm),
        ]),
        ...simulation.map((sample) => Math.abs(sample.simulatedPositionCm)),
      );
      const velocityExtent = Math.max(
        5,
        ...visible.map((sample) => Math.abs(sample.ballVelocityCmS)),
      );
      const yPosition = (value: number) => (plot.top + plot.bottom) / 2
        - value / positionExtent * (plot.bottom - plot.top) / 2;
      const yVelocity = (value: number) => (plot.top + plot.bottom) / 2
        - value / velocityExtent * (plot.bottom - plot.top) / 2;

      context.lineWidth = 1;
      context.strokeStyle = COLORS.grid;
      context.fillStyle = COLORS.text;
      context.font = "11px system-ui";
      context.textAlign = "right";
      for (let line = -2; line <= 2; line += 1) {
        const value = line * positionExtent / 2;
        const y = yPosition(value);
        context.beginPath();
        context.moveTo(plot.left, y);
        context.lineTo(plot.right, y);
        context.stroke();
        context.fillText(`${value.toFixed(1)}`, plot.left - 7, y + 4);
      }
      context.textAlign = "center";
      for (let line = 0; line <= 4; line += 1) {
        const time = startMs + (endMs - startMs) * line / 4;
        const px = x(time);
        context.beginPath();
        context.moveTo(px, plot.top);
        context.lineTo(px, plot.bottom);
        context.stroke();
        context.fillText(`${(time / 1000).toFixed(1)}s`, px, plot.bottom + 20);
      }
      context.save();
      context.translate(14, height / 2);
      context.rotate(-Math.PI / 2);
      context.fillText("位置 / cm", 0, 0);
      context.restore();
      context.save();
      context.translate(width - 10, height / 2);
      context.rotate(Math.PI / 2);
      context.fillText("速度 / cm/s", 0, 0);
      context.restore();

      const draw = (
        points: { elapsedMs: number; value: number }[],
        mapY: (value: number) => number,
        color: string,
        dash: number[] = [],
        lineWidth = 1.8,
      ) => {
        if (points.length < 2) return;
        context.beginPath();
        points.forEach((point, index) => {
          const px = x(point.elapsedMs);
          const py = mapY(point.value);
          if (index === 0) context.moveTo(px, py);
          else context.lineTo(px, py);
        });
        context.strokeStyle = color;
        context.lineWidth = lineWidth;
        context.setLineDash(dash);
        context.stroke();
        context.setLineDash([]);
      };
      draw(visible.map((sample) => ({ elapsedMs: sample.elapsedMs, value: sample.targetPositionCm })), yPosition, COLORS.target, [6, 4]);
      draw(visible.map((sample) => ({ elapsedMs: sample.elapsedMs, value: sample.ballPositionCm })), yPosition, COLORS.actual, [], 2.2);
      draw(visible.map((sample) => ({ elapsedMs: sample.elapsedMs, value: sample.ballVelocityCmS })), yVelocity, COLORS.velocity, [], 1.3);
      if (simulation.length > 1) {
        draw(
          simulation
            .filter((sample) => sample.elapsedMs >= startMs)
            .map((sample) => ({ elapsedMs: sample.elapsedMs, value: sample.simulatedPositionCm })),
          yPosition,
          COLORS.simulated,
          [3, 3],
          1.8,
        );
      }
      if (cursorMs !== undefined && cursorMs >= startMs && cursorMs <= endMs) {
        const px = x(cursorMs);
        context.strokeStyle = COLORS.cursor;
        context.lineWidth = 1;
        context.beginPath();
        context.moveTo(px, plot.top);
        context.lineTo(px, plot.bottom);
        context.stroke();
      }
    };
    render();
    const observer = new ResizeObserver(render);
    observer.observe(canvas);
    return () => observer.disconnect();
  }, [cursorMs, samples, simulation]);

  return <canvas ref={canvasRef} className="telemetry-chart" aria-label="位置速度时序图" />;
}
