import { useEffect, useRef, useState } from "react";
import type { TelemetryFrame } from "../types";

interface SpeedChartProps {
  frames: TelemetryFrame[];
}

type SeriesKey = "speedSetpoint" | "leftSpeed" | "rightSpeed";

const SERIES: Array<{ key: SeriesKey; color: string; dashed?: boolean }> = [
  { key: "speedSetpoint", color: "#d99a2b", dashed: true },
  { key: "leftSpeed", color: "#16856e" },
  { key: "rightSpeed", color: "#c6534f" },
];

export function SpeedChart({ frames }: SpeedChartProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const observer = new ResizeObserver(([entry]) => {
      setSize({
        width: Math.max(1, Math.floor(entry.contentRect.width)),
        height: Math.max(1, Math.floor(entry.contentRect.height)),
      });
    });
    observer.observe(canvas);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || size.width === 0 || size.height === 0) return;
    const ratio = window.devicePixelRatio || 1;
    canvas.width = Math.floor(size.width * ratio);
    canvas.height = Math.floor(size.height * ratio);
    const context = canvas.getContext("2d");
    if (!context) return;
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, size.width, size.height);
    context.fillStyle = "#ffffff";
    context.fillRect(0, 0, size.width, size.height);

    const plot = { left: 46, top: 14, right: size.width - 14, bottom: size.height - 28 };
    const visible = frames.slice(-240);
    const maxValue = visible.reduce((max, frame) => {
      return Math.max(
        max,
        Math.abs(frame.speedSetpoint),
        Math.abs(frame.leftSpeed),
        Math.abs(frame.rightSpeed),
      );
    }, 20);
    const extent = Math.max(20, Math.ceil(maxValue / 10) * 10);
    const valueToY = (value: number) =>
      plot.top + ((extent - value) / (extent * 2)) * (plot.bottom - plot.top);

    context.strokeStyle = "#e5e9e7";
    context.lineWidth = 1;
    for (let index = 0; index <= 4; index += 1) {
      const y = plot.top + ((plot.bottom - plot.top) * index) / 4;
      context.beginPath();
      context.moveTo(plot.left, y + 0.5);
      context.lineTo(plot.right, y + 0.5);
      context.stroke();
    }

    context.fillStyle = "#65706d";
    context.font = "11px Segoe UI, sans-serif";
    context.textAlign = "right";
    context.fillText(`${extent}`, plot.left - 8, plot.top + 4);
    context.fillText("0", plot.left - 8, valueToY(0) + 4);
    context.fillText(`${-extent}`, plot.left - 8, plot.bottom + 4);
    context.textAlign = "left";
    context.fillText("cm/s", plot.left, size.height - 7);

    if (visible.length < 2) return;
    const indexToX = (index: number) =>
      plot.left + (index / (visible.length - 1)) * (plot.right - plot.left);

    for (const series of SERIES) {
      context.strokeStyle = series.color;
      context.lineWidth = series.dashed ? 1.5 : 2;
      context.setLineDash(series.dashed ? [6, 5] : []);
      context.beginPath();
      visible.forEach((frame, index) => {
        const x = indexToX(index);
        const y = valueToY(frame[series.key]);
        if (index === 0) context.moveTo(x, y);
        else context.lineTo(x, y);
      });
      context.stroke();
    }
    context.setLineDash([]);
  }, [frames, size]);

  return <canvas ref={canvasRef} className="speed-canvas" aria-label="左右轮速度曲线" />;
}
