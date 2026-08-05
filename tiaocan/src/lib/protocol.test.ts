import { describe, expect, it } from "vitest";
import {
  encodeTelemetryFrame,
  encodeLegacyTelemetryFrame,
  taskMatchesProfile,
  Uart2TelemetryParser,
} from "./protocol";
import type { TelemetryValues } from "../types";

const SAMPLE: TelemetryValues = {
  protocolVersion: 2,
  ballPositionCm: 4.3,
  ballVelocityCmS: -2.5,
  ballDirection: -1,
  targetPositionCm: 5,
  vehicleSpeedCmS: 30,
  vehicleCommandSpeedCmS: 30,
  vehicleMeasuredAccelerationCmS2: 0,
  vehicleCommandAccelerationCmS2: 0,
  leftMotorSpeedCmS: 29.5,
  rightMotorSpeedCmS: 30.5,
  yawDeg: 12.5,
  yawRateDps: -3.25,
  servoAngleDeg: -8,
  servoSpeed: 900,
  feedforwardServoDeg: 4.5,
  turnCompensationServoDeg: -1.2,
  taskPhase: 3,
  routeMode: 1,
  statusFlags: 7,
  sequence: 12,
  taskId: 3,
};

describe("Uart2TelemetryParser", () => {
  it("decodes fragmented valid frames", () => {
    const parser = new Uart2TelemetryParser();
    const frame = encodeTelemetryFrame(SAMPLE);
    expect(parser.push(frame.slice(0, 9))).toEqual([]);
    const [decoded] = parser.push(frame.slice(9));
    expect(decoded).toEqual(SAMPLE);
    expect(parser.getStats().validFrames).toBe(1);
  });

  it("rejects a bad checksum and recovers", () => {
    const parser = new Uart2TelemetryParser();
    const bad = encodeTelemetryFrame(SAMPLE);
    bad[12] ^= 0x40;
    const stream = new Uint8Array(bad.length + 2 + bad.length);
    stream.set(bad);
    stream.set([0, 0xff], bad.length);
    stream.set(encodeTelemetryFrame(SAMPLE), bad.length + 2);
    expect(parser.push(stream)).toEqual([SAMPLE]);
    expect(parser.getStats().checksumErrors).toBe(1);
  });

  it("keeps decoding the original float frame", () => {
    const parser = new Uart2TelemetryParser();
    const [decoded] = parser.push(encodeLegacyTelemetryFrame(SAMPLE));
    expect(decoded).toMatchObject({
      protocolVersion: 1,
      ballVelocityCmS: SAMPLE.ballVelocityCmS,
      targetPositionCm: SAMPLE.targetPositionCm,
      vehicleSpeedCmS: SAMPLE.vehicleSpeedCmS,
      taskId: SAMPLE.taskId,
    });
    expect(decoded?.ballPositionCm).toBeCloseTo(SAMPLE.ballPositionCm, 5);
    expect(decoded?.vehicleCommandSpeedCmS).toBe(SAMPLE.vehicleSpeedCmS);
    expect(decoded?.leftMotorSpeedCmS).toBe(SAMPLE.vehicleSpeedCmS);
    expect(decoded?.rightMotorSpeedCmS).toBe(SAMPLE.vehicleSpeedCmS);
    expect(parser.getStats().validFrames).toBe(1);
  });

  it("keeps stationary and moving task profiles separate", () => {
    expect(taskMatchesProfile("task3", 3)).toBe(true);
    expect(taskMatchesProfile("task3", 4)).toBe(false);
    expect(taskMatchesProfile("task45", 4)).toBe(true);
    expect(taskMatchesProfile("task45", 5)).toBe(true);
    expect(taskMatchesProfile("task45", 6)).toBe(false);
    expect(taskMatchesProfile("task6", 6)).toBe(true);
    expect(taskMatchesProfile("task6", 7)).toBe(false);
    expect(taskMatchesProfile("task7", 7)).toBe(true);
    expect(taskMatchesProfile("task7", 6)).toBe(false);
  });

  it("accepts task 7 telemetry frames", () => {
    const parser = new Uart2TelemetryParser();
    const sample = { ...SAMPLE, taskId: 7 };
    expect(parser.push(encodeTelemetryFrame(sample))).toEqual([sample]);
  });
});
