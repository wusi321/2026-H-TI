import { useEffect, useRef, useState } from "react";
import type { TrackPoint } from "../types";

interface PathCanvasProps {
  points: TrackPoint[];
  current: TrackPoint;
  zoom: number;
  viewMode: "follow" | "fit";
}

function chooseGridStep(scale: number): number {
  const candidates = [5, 10, 20, 50, 100, 200, 500, 1000];
  return candidates.find((step) => step * scale >= 56) ?? 1000;
}

function drawVehicle(
  context: CanvasRenderingContext2D,
  x: number,
  y: number,
  yaw: number,
  scale: number,
): void {
  const length = Math.max(24, Math.min(54, 28 * scale));
  const width = Math.max(15, Math.min(34, 17 * scale));
  const wheelLength = length * 0.34;
  const wheelWidth = Math.max(4, width * 0.16);

  context.save();
  context.translate(x, y);
  context.rotate((-yaw * Math.PI) / 180);

  context.fillStyle = "#18211f";
  context.fillRect(-length * 0.34, -width * 0.5, length * 0.68, width);
  context.fillStyle = "#20a482";
  context.fillRect(length * 0.04, -width * 0.38, length * 0.31, width * 0.76);
  context.fillStyle = "#f2b84b";
  context.beginPath();
  context.moveTo(length * 0.49, 0);
  context.lineTo(length * 0.27, -width * 0.3);
  context.lineTo(length * 0.27, width * 0.3);
  context.closePath();
  context.fill();

  context.fillStyle = "#303a37";
  const wheelX = -wheelLength * 0.5;
  context.fillRect(wheelX, -width * 0.5 - wheelWidth, wheelLength, wheelWidth);
  context.fillRect(wheelX, width * 0.5, wheelLength, wheelWidth);
  context.restore();
}

export function PathCanvas({ points, current, zoom, viewMode }: PathCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const observer = new ResizeObserver(([entry]) => {
      const width = Math.max(1, Math.floor(entry.contentRect.width));
      const height = Math.max(1, Math.floor(entry.contentRect.height));
      setSize({ width, height });
    });
    observer.observe(canvas);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || size.width === 0 || size.height === 0) return;

    const pixelRatio = window.devicePixelRatio || 1;
    canvas.width = Math.floor(size.width * pixelRatio);
    canvas.height = Math.floor(size.height * pixelRatio);
    const context = canvas.getContext("2d");
    if (!context) return;
    context.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);
    context.clearRect(0, 0, size.width, size.height);
    context.fillStyle = "#fbfcfb";
    context.fillRect(0, 0, size.width, size.height);

    let cameraX = current.x;
    let cameraY = current.y;
    let scale = zoom;

    if (viewMode === "fit" && points.length > 1) {
      let minX = current.x;
      let maxX = current.x;
      let minY = current.y;
      let maxY = current.y;
      for (const point of points) {
        minX = Math.min(minX, point.x);
        maxX = Math.max(maxX, point.x);
        minY = Math.min(minY, point.y);
        maxY = Math.max(maxY, point.y);
      }
      cameraX = (minX + maxX) / 2;
      cameraY = (minY + maxY) / 2;
      const rangeX = Math.max(30, maxX - minX + 40);
      const rangeY = Math.max(30, maxY - minY + 40);
      scale = Math.max(
        0.12,
        Math.min(8, Math.min(size.width / rangeX, size.height / rangeY) * zoom),
      );
    }

    const toScreenX = (x: number) => size.width / 2 + (x - cameraX) * scale;
    const toScreenY = (y: number) => size.height / 2 - (y - cameraY) * scale;
    const gridStep = chooseGridStep(scale);

    context.lineWidth = 1;
    context.strokeStyle = "#e5e9e7";
    const leftWorld = cameraX - size.width / 2 / scale;
    const rightWorld = cameraX + size.width / 2 / scale;
    const bottomWorld = cameraY - size.height / 2 / scale;
    const topWorld = cameraY + size.height / 2 / scale;

    for (
      let x = Math.floor(leftWorld / gridStep) * gridStep;
      x <= rightWorld;
      x += gridStep
    ) {
      const screenX = Math.round(toScreenX(x)) + 0.5;
      context.beginPath();
      context.moveTo(screenX, 0);
      context.lineTo(screenX, size.height);
      context.stroke();
    }
    for (
      let y = Math.floor(bottomWorld / gridStep) * gridStep;
      y <= topWorld;
      y += gridStep
    ) {
      const screenY = Math.round(toScreenY(y)) + 0.5;
      context.beginPath();
      context.moveTo(0, screenY);
      context.lineTo(size.width, screenY);
      context.stroke();
    }

    context.strokeStyle = "#b9c2bf";
    context.lineWidth = 1.5;
    if (leftWorld <= 0 && rightWorld >= 0) {
      context.beginPath();
      context.moveTo(toScreenX(0), 0);
      context.lineTo(toScreenX(0), size.height);
      context.stroke();
    }
    if (bottomWorld <= 0 && topWorld >= 0) {
      context.beginPath();
      context.moveTo(0, toScreenY(0));
      context.lineTo(size.width, toScreenY(0));
      context.stroke();
    }

    if (points.length > 1) {
      context.strokeStyle = "#16856e";
      context.lineWidth = 2.5;
      context.lineJoin = "round";
      context.lineCap = "round";
      context.beginPath();
      context.moveTo(toScreenX(points[0].x), toScreenY(points[0].y));
      for (let index = 1; index < points.length; index += 1) {
        context.lineTo(toScreenX(points[index].x), toScreenY(points[index].y));
      }
      context.stroke();
    }

    context.fillStyle = "#d99a2b";
    context.beginPath();
    context.arc(toScreenX(0), toScreenY(0), 4, 0, Math.PI * 2);
    context.fill();

    drawVehicle(
      context,
      toScreenX(current.x),
      toScreenY(current.y),
      current.yaw,
      scale,
    );

    context.fillStyle = "#5e6966";
    context.font = "12px Segoe UI, sans-serif";
    context.fillText(`${gridStep} cm`, 14, size.height - 14);
  }, [current, points, size, viewMode, zoom]);

  return (
    <div className="canvas-shell">
      <canvas ref={canvasRef} className="path-canvas" aria-label="小车运动路径" />
      {points.length <= 1 && (
        <div className="canvas-empty">等待有效遥测帧</div>
      )}
    </div>
  );
}
