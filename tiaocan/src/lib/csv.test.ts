import { describe, expect, it } from "vitest";
import type { RunMetadata, TelemetrySample } from "../types";
import { parseCsv, runToCsv } from "./csv";

const RUN: RunMetadata = {
  id: "task4-test",
  name: "任务 4 实测",
  task: "task45",
  actualTaskId: 4,
  startedAtMs: 1_000,
  endedAtMs: 1_050,
  sampleCount: 1,
  baudRate: 9600,
  notes: "",
};

const SAMPLE: TelemetrySample = {
  protocolVersion: 2,
  index: 0,
  receivedAtMs: 1_000,
  elapsedMs: 0,
  ballPositionCm: 0.2,
  ballVelocityCmS: -0.4,
  ballDirection: -1,
  targetPositionCm: 0,
  vehicleSpeedCmS: 30,
  vehicleCommandSpeedCmS: 30,
  vehicleMeasuredAccelerationCmS2: 0,
  vehicleCommandAccelerationCmS2: 0,
  leftMotorSpeedCmS: 29.5,
  rightMotorSpeedCmS: 30.5,
  yawDeg: 16,
  yawRateDps: 2,
  servoAngleDeg: 2,
  servoSpeed: 800,
  feedforwardServoDeg: 1,
  turnCompensationServoDeg: 0,
  taskPhase: 2,
  routeMode: 1,
  statusFlags: 3,
  sequence: 4,
  taskId: 4,
};

const EXTENDED_SAMPLE: TelemetrySample = {
  ...SAMPLE,
  protocolVersion: 2,
  taskPhase: 4,
  routeMode: 2,
  statusFlags: 7,
  sequence: 18,
  vehicleCommandSpeedCmS: 31,
  vehicleMeasuredAccelerationCmS2: 12.5,
  vehicleCommandAccelerationCmS2: 15,
  leftMotorSpeedCmS: 29.5,
  rightMotorSpeedCmS: 30.5,
  yawRateDps: -2,
  feedforwardServoDeg: 3.5,
  turnCompensationServoDeg: -0.5,
};

const LEGACY_SAMPLE: TelemetrySample = {
  index: 0,
  receivedAtMs: 1_000,
  elapsedMs: 0,
  ballPositionCm: 0.2,
  ballVelocityCmS: -0.4,
  ballDirection: -1,
  targetPositionCm: 0,
  vehicleSpeedCmS: 30,
  yawDeg: 16,
  servoAngleDeg: 2,
  servoSpeed: 800,
  taskId: 4,
};

describe("UART2 CSV", () => {
  it("preserves the actual task ID", () => {
    const parsed = parseCsv(runToCsv(RUN, [SAMPLE]));
    expect(parsed.task).toBe("task45");
    expect(parsed.samples).toEqual([SAMPLE]);
  });

  it("round-trips v2 wheel and controller telemetry", () => {
    const parsed = parseCsv(runToCsv(RUN, [EXTENDED_SAMPLE]));
    expect(parsed.samples).toEqual([EXTENDED_SAMPLE]);
  });

  it("keeps the original 12-column CSV shape", () => {
    const csv = runToCsv(RUN, [LEGACY_SAMPLE]);
    expect(csv.split("\n").at(-2)).toContain("ball_position_cm");
    expect(parseCsv(csv).samples).toEqual([LEGACY_SAMPLE]);
  });

  it("keeps task 7 in an independent profile", () => {
    const run = { ...RUN, task: "task7" as const, actualTaskId: 7 };
    const sample = { ...SAMPLE, taskId: 7 };
    const parsed = parseCsv(runToCsv(run, [sample]));
    expect(parsed.task).toBe("task7");
    expect(parsed.samples).toEqual([sample]);
  });
});
