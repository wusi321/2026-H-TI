import { describe, expect, it } from "vitest";
import type { TelemetrySample } from "../types";
import {
  DEFAULT_PARAMETERS,
  metricsForActual,
  simulateRun,
} from "./simulation";

function makeSamples(): TelemetrySample[] {
  return Array.from({ length: 101 }, (_, index) => ({
    index,
    receivedAtMs: 1_000 + index * 50,
    elapsedMs: index * 50,
    protocolVersion: 2,
    ballPositionCm: 5 * (1 - Math.exp(-index / 20)),
    ballVelocityCmS: 0,
    ballDirection: 0,
    targetPositionCm: 5,
    vehicleSpeedCmS: 0,
    vehicleCommandSpeedCmS: 0,
    leftMotorSpeedCmS: 0,
    rightMotorSpeedCmS: 0,
    yawDeg: 0,
    yawRateDps: 0,
    servoAngleDeg: 0,
    servoSpeed: 800,
    feedforwardServoDeg: 0,
    turnCompensationServoDeg: 0,
    taskPhase: 0,
    routeMode: 0,
    statusFlags: 0,
    sequence: index,
    taskId: 3,
  }));
}

describe("simulation", () => {
  it("returns one simulated value per input sample", () => {
    const samples = makeSamples();
    const simulated = simulateRun(samples, DEFAULT_PARAMETERS.task3);
    expect(simulated).toHaveLength(samples.length);
    expect(simulated.every((sample) => Number.isFinite(sample.simulatedPositionCm))).toBe(true);
  });

  it("calculates finite run metrics", () => {
    const metrics = metricsForActual(makeSamples());
    expect(metrics.durationS).toBe(5);
    expect(metrics.rmsErrorCm).toBeGreaterThan(0);
    expect(metrics.score).toBeGreaterThanOrEqual(0);
  });
});
