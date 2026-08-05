import type { ParserStats, TaskProfileId, TelemetryValues } from "../types";

export const FRAME_HEAD = 0xa5;
export const FRAME_TAIL = 0x5a;
export const PAYLOAD_SIZE = 38;
export const FRAME_SIZE = 42;
export const LEGACY_V2_PAYLOAD_SIZE = 34;
export const LEGACY_V2_FRAME_SIZE = 38;
export const LEGACY_PAYLOAD_SIZE = 36;
export const LEGACY_FRAME_SIZE = 40;
export const TELEMETRY_PROTOCOL_VERSION = 2;

const V2_INT16_MIN = -32768;
const V2_INT16_MAX = 32767;

const EMPTY_STATS: ParserStats = {
  bytesReceived: 0,
  validFrames: 0,
  checksumErrors: 0,
  formatErrors: 0,
  discardedBytes: 0,
};

function finite(values: number[]): boolean {
  return values.every(Number.isFinite);
}

function int16(value: number | undefined, fallback = 0): number {
  const numeric = Number.isFinite(value) ? value! : fallback;
  return Math.max(V2_INT16_MIN, Math.min(V2_INT16_MAX, Math.round(numeric)));
}

export function taskMatchesProfile(task: TaskProfileId, actualTaskId: number): boolean {
  if (task === "task3") return actualTaskId === 3;
  if (task === "task6") return actualTaskId === 6;
  if (task === "task7") return actualTaskId === 7;
  return actualTaskId === 4 || actualTaskId === 5;
}

export class Uart2TelemetryParser {
  private buffer = new Uint8Array(0);
  private stats: ParserStats = { ...EMPTY_STATS };

  push(chunk: Uint8Array): TelemetryValues[] {
    this.stats.bytesReceived += chunk.byteLength;
    const merged = new Uint8Array(this.buffer.byteLength + chunk.byteLength);
    merged.set(this.buffer);
    merged.set(chunk, this.buffer.byteLength);
    this.buffer = merged;

    const decoded: TelemetryValues[] = [];
    while (this.buffer.byteLength > 0) {
      const head = this.buffer.indexOf(FRAME_HEAD);
      if (head < 0) {
        this.stats.discardedBytes += this.buffer.byteLength;
        this.buffer = new Uint8Array(0);
        break;
      }
      if (head > 0) {
        this.stats.discardedBytes += head;
        this.buffer = this.buffer.slice(head);
      }
      if (this.buffer.byteLength < 2) break;
      const payloadSize = this.buffer[1];
      if (payloadSize !== PAYLOAD_SIZE && payloadSize !== LEGACY_V2_PAYLOAD_SIZE && payloadSize !== LEGACY_PAYLOAD_SIZE) {
        this.stats.formatErrors += 1;
        this.stats.discardedBytes += 1;
        this.buffer = this.buffer.slice(1);
        continue;
      }
      const frameSize = payloadSize + 4;
      if (this.buffer.byteLength < frameSize) break;

      const frame = this.buffer.slice(0, frameSize);
      const expectedChecksum = frame
        .slice(1, frameSize - 2)
        .reduce((sum, byte) => (sum + byte) & 0xff, 0);
      if (frame[frameSize - 1] !== FRAME_TAIL) {
        this.stats.formatErrors += 1;
        this.stats.discardedBytes += 1;
        this.buffer = this.buffer.slice(1);
        continue;
      }
      if (frame[frameSize - 2] !== expectedChecksum) {
        this.stats.checksumErrors += 1;
        this.stats.discardedBytes += 1;
        this.buffer = this.buffer.slice(1);
        continue;
      }

      const view = new DataView(frame.buffer, frame.byteOffset, frame.byteLength);
      if ((payloadSize === PAYLOAD_SIZE || payloadSize === LEGACY_V2_PAYLOAD_SIZE) && frame[2] === TELEMETRY_PROTOCOL_VERSION) {
        const taskId = frame[3];
        const values = Array.from({ length: payloadSize === PAYLOAD_SIZE ? 15 : 13 }, (_, index) =>
          view.getInt16(10 + index * 2, true),
        );
        if (taskId < 3 || taskId > 7) {
          this.stats.formatErrors += 1;
        } else {
          const ballVelocityCmS = values[1] / 10;
          decoded.push({
            protocolVersion: TELEMETRY_PROTOCOL_VERSION,
            ballPositionCm: values[0] / 10,
            ballVelocityCmS,
            ballDirection: (frame[6] & 1) === 0 ? 2 : Math.abs(values[1]) < 5 ? 0 : Math.sign(values[1]),
            targetPositionCm: values[2] / 10,
            vehicleSpeedCmS: values[3] / 10,
            vehicleCommandSpeedCmS: values[4] / 10,
            vehicleMeasuredAccelerationCmS2: payloadSize === PAYLOAD_SIZE ? values[5] / 10 : undefined,
            vehicleCommandAccelerationCmS2: payloadSize === PAYLOAD_SIZE ? values[6] / 10 : undefined,
            leftMotorSpeedCmS: values[payloadSize === PAYLOAD_SIZE ? 7 : 5] / 10,
            rightMotorSpeedCmS: values[payloadSize === PAYLOAD_SIZE ? 8 : 6] / 10,
            yawDeg: values[payloadSize === PAYLOAD_SIZE ? 9 : 7] / 100,
            yawRateDps: values[payloadSize === PAYLOAD_SIZE ? 10 : 8] / 100,
            servoAngleDeg: values[payloadSize === PAYLOAD_SIZE ? 11 : 9] / 100,
            servoSpeed: values[payloadSize === PAYLOAD_SIZE ? 12 : 10],
            feedforwardServoDeg: values[payloadSize === PAYLOAD_SIZE ? 13 : 11] / 100,
            turnCompensationServoDeg: values[payloadSize === PAYLOAD_SIZE ? 14 : 12] / 100,
            taskPhase: frame[4],
            routeMode: frame[5],
            statusFlags: frame[6],
            sequence: view.getUint16(8, true),
            taskId,
          });
          this.stats.validFrames += 1;
        }
      } else if (payloadSize === LEGACY_PAYLOAD_SIZE) {
        const fields = Array.from({ length: 9 }, (_, index) =>
          view.getFloat32(2 + index * 4, true),
        );
        const taskId = Math.round(fields[8]);
        if (!finite(fields) || Math.abs(fields[8] - taskId) > 0.001 || taskId < 3 || taskId > 7) {
          this.stats.formatErrors += 1;
        } else {
          decoded.push({
            protocolVersion: 1,
            ballPositionCm: fields[0],
            ballVelocityCmS: fields[1],
            ballDirection: fields[2],
            targetPositionCm: fields[3],
            vehicleSpeedCmS: fields[4],
            vehicleCommandSpeedCmS: fields[4],
            leftMotorSpeedCmS: fields[4],
            rightMotorSpeedCmS: fields[4],
            yawDeg: fields[5],
            yawRateDps: 0,
            servoAngleDeg: fields[6],
            servoSpeed: fields[7],
            feedforwardServoDeg: 0,
            turnCompensationServoDeg: 0,
            taskPhase: 0,
            routeMode: 0,
            statusFlags: 0,
            sequence: 0,
            taskId,
          });
          this.stats.validFrames += 1;
        }
      } else {
        this.stats.formatErrors += 1;
      }
      this.buffer = this.buffer.slice(frameSize);
    }
    return decoded;
  }

  getStats(): ParserStats {
    return { ...this.stats };
  }

  reset(): void {
    this.buffer = new Uint8Array(0);
    this.stats = { ...EMPTY_STATS };
  }
}

export function encodeTelemetryFrame(values: TelemetryValues): Uint8Array {
  const frame = new Uint8Array(FRAME_SIZE);
  const view = new DataView(frame.buffer);
  frame[0] = FRAME_HEAD;
  frame[1] = PAYLOAD_SIZE;
  frame[2] = TELEMETRY_PROTOCOL_VERSION;
  frame[3] = values.taskId;
  frame[4] = values.taskPhase ?? 0;
  frame[5] = values.routeMode ?? 0;
  frame[6] = values.statusFlags ?? 0;
  view.setUint16(8, Math.max(0, Math.min(65535, Math.round(values.sequence ?? 0))), true);
  const fields = [
    int16(values.ballPositionCm * 10),
    int16(values.ballVelocityCmS * 10),
    int16(values.targetPositionCm * 10),
    int16(values.vehicleSpeedCmS * 10),
    int16((values.vehicleCommandSpeedCmS ?? values.vehicleSpeedCmS) * 10),
    int16((values.vehicleMeasuredAccelerationCmS2 ?? 0) * 10),
    int16((values.vehicleCommandAccelerationCmS2 ?? 0) * 10),
    int16((values.leftMotorSpeedCmS ?? values.vehicleSpeedCmS) * 10),
    int16((values.rightMotorSpeedCmS ?? values.vehicleSpeedCmS) * 10),
    int16(values.yawDeg * 100),
    int16((values.yawRateDps ?? 0) * 100),
    int16(values.servoAngleDeg * 100),
    int16(values.servoSpeed),
    int16((values.feedforwardServoDeg ?? 0) * 100),
    int16((values.turnCompensationServoDeg ?? 0) * 100),
  ];
  fields.forEach((value, index) => view.setInt16(10 + index * 2, Math.round(value), true));
  frame[40] = frame.slice(1, 40).reduce((sum, byte) => (sum + byte) & 0xff, 0);
  frame[41] = FRAME_TAIL;
  return frame;
}

export function encodeLegacyTelemetryFrame(values: TelemetryValues): Uint8Array {
  const frame = new Uint8Array(LEGACY_FRAME_SIZE);
  const view = new DataView(frame.buffer);
  frame[0] = FRAME_HEAD;
  frame[1] = LEGACY_PAYLOAD_SIZE;
  const fields = [
    values.ballPositionCm,
    values.ballVelocityCmS,
    values.ballDirection,
    values.targetPositionCm,
    values.vehicleSpeedCmS,
    values.yawDeg,
    values.servoAngleDeg,
    values.servoSpeed,
    values.taskId,
  ];
  fields.forEach((value, index) => view.setFloat32(2 + index * 4, value, true));
  frame[38] = frame.slice(1, 38).reduce((sum, byte) => (sum + byte) & 0xff, 0);
  frame[39] = FRAME_TAIL;
  return frame;
}
