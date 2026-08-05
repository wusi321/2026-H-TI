import { Activity, Crosshair, Gauge, Timer, Waves } from "lucide-react";
import type { PerformanceMetrics } from "../types";

interface MetricsStripProps {
  actual: PerformanceMetrics | null;
  simulated: PerformanceMetrics | null;
}

function value(actual: number | undefined, simulated: number | undefined, unit: string) {
  if (actual === undefined) return "--";
  const base = `${actual.toFixed(2)}${unit}`;
  return simulated === undefined ? base : `${base} / ${simulated.toFixed(2)}${unit}`;
}

export function MetricsStrip({ actual, simulated }: MetricsStripProps) {
  const items = [
    { icon: Crosshair, label: "RMS 误差", value: value(actual?.rmsErrorCm, simulated?.rmsErrorCm, " cm") },
    { icon: Activity, label: "最大误差", value: value(actual?.maxAbsoluteErrorCm, simulated?.maxAbsoluteErrorCm, " cm") },
    { icon: Gauge, label: "峰值球速", value: value(actual?.peakVelocityCmS, simulated?.peakVelocityCmS, " cm/s") },
    { icon: Waves, label: "容差内占比", value: value(actual?.inTolerancePercent, simulated?.inTolerancePercent, "%") },
    { icon: Timer, label: "稳定时间", value: actual?.settlingTimeS === null || actual === null ? "--" : `${actual.settlingTimeS.toFixed(2)} s` },
  ];
  return (
    <div className="metrics-strip">
      {items.map(({ icon: Icon, label, value: metric }) => (
        <article className="metric" key={label}>
          <Icon size={16} />
          <span>{label}</span>
          <strong>{metric}</strong>
        </article>
      ))}
    </div>
  );
}
