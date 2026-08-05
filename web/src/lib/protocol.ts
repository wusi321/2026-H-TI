import type { ParserStats, TelemetryFrame } from "../types";

export const FRAME_HEADER_0 = 0xaa;
export const FRAME_HEADER_1 = 0x55;
export const FRAME_VERSION = 1;
export const FRAME_SIZE = 32;
export const CRC_OFFSET = 30;

const EMPTY_STATS: ParserStats = {
  bytesReceived: 0,
  validFrames: 0,
  crcErrors: 0,
  formatErrors: 0,
  discardedBytes: 0,
};

export function crc16Ccitt(data: Uint8Array): number {
  let crc = 0xffff;

  for (const byte of data) {
    crc ^= byte << 8;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc & 0x8000) !== 0 ? ((crc << 1) ^ 0x1021) : crc << 1;
      crc &= 0xffff;
    }
  }

  return crc;
}

function findHeader(data: Uint8Array): number {
  for (let index = 0; index < data.length - 1; index += 1) {
    if (data[index] === FRAME_HEADER_0 && data[index + 1] === FRAME_HEADER_1) {
      return index;
    }
  }
  return -1;
}

function decodeFrame(frame: Uint8Array, receivedAt: number): TelemetryFrame | null {
  const view = new DataView(frame.buffer, frame.byteOffset, frame.byteLength);
  const decoded: TelemetryFrame = {
    sequence: view.getUint32(4, true),
    speedSetpoint: view.getFloat32(8, true),
    leftSpeed: view.getFloat32(12, true),
    rightSpeed: view.getFloat32(16, true),
    yaw: view.getFloat32(20, true),
    workMode: view.getInt16(24, true),
    uptimeSeconds: view.getUint16(26, true),
    peerOnline: (frame[28] & 0x01) !== 0,
    receivedAt,
  };

  if (
    !Number.isFinite(decoded.speedSetpoint) ||
    !Number.isFinite(decoded.leftSpeed) ||
    !Number.isFinite(decoded.rightSpeed) ||
    !Number.isFinite(decoded.yaw)
  ) {
    return null;
  }

  return decoded;
}

export class UartFrameParser {
  private buffer = new Uint8Array(0);
  private stats: ParserStats = { ...EMPTY_STATS };

  push(chunk: Uint8Array, receivedAt = performance.now()): TelemetryFrame[] {
    this.stats.bytesReceived += chunk.byteLength;
    const merged = new Uint8Array(this.buffer.byteLength + chunk.byteLength);
    merged.set(this.buffer);
    merged.set(chunk, this.buffer.byteLength);
    this.buffer = merged;

    const frames: TelemetryFrame[] = [];

    while (this.buffer.byteLength > 0) {
      const headerIndex = findHeader(this.buffer);
      if (headerIndex < 0) {
        const keepHeaderByte = this.buffer.at(-1) === FRAME_HEADER_0 ? 1 : 0;
        this.stats.discardedBytes += this.buffer.byteLength - keepHeaderByte;
        this.buffer = keepHeaderByte === 1
          ? this.buffer.slice(this.buffer.byteLength - 1)
          : new Uint8Array(0);
        break;
      }

      if (headerIndex > 0) {
        this.stats.discardedBytes += headerIndex;
        this.buffer = this.buffer.slice(headerIndex);
      }

      if (this.buffer.byteLength < 4) {
        break;
      }

      if (this.buffer[2] !== FRAME_VERSION || this.buffer[3] !== FRAME_SIZE) {
        this.stats.formatErrors += 1;
        this.stats.discardedBytes += 1;
        this.buffer = this.buffer.slice(1);
        continue;
      }

      if (this.buffer.byteLength < FRAME_SIZE) {
        break;
      }

      const candidate = this.buffer.slice(0, FRAME_SIZE);
      const view = new DataView(candidate.buffer);
      const receivedCrc = view.getUint16(CRC_OFFSET, true);
      const calculatedCrc = crc16Ccitt(candidate.subarray(0, CRC_OFFSET));

      if (receivedCrc !== calculatedCrc) {
        this.stats.crcErrors += 1;
        this.stats.discardedBytes += 1;
        this.buffer = this.buffer.slice(1);
        continue;
      }

      const decoded = decodeFrame(candidate, receivedAt);
      if (decoded === null) {
        this.stats.formatErrors += 1;
      } else {
        this.stats.validFrames += 1;
        frames.push(decoded);
      }
      this.buffer = this.buffer.slice(FRAME_SIZE);
    }

    return frames;
  }

  getStats(): ParserStats {
    return { ...this.stats };
  }

  reset(): void {
    this.buffer = new Uint8Array(0);
    this.stats = { ...EMPTY_STATS };
  }
}

export function encodeTelemetryFrame(
  telemetry: Omit<TelemetryFrame, "receivedAt">,
): Uint8Array {
  const frame = new Uint8Array(FRAME_SIZE);
  const view = new DataView(frame.buffer);
  frame[0] = FRAME_HEADER_0;
  frame[1] = FRAME_HEADER_1;
  frame[2] = FRAME_VERSION;
  frame[3] = FRAME_SIZE;
  view.setUint32(4, telemetry.sequence, true);
  view.setFloat32(8, telemetry.speedSetpoint, true);
  view.setFloat32(12, telemetry.leftSpeed, true);
  view.setFloat32(16, telemetry.rightSpeed, true);
  view.setFloat32(20, telemetry.yaw, true);
  view.setInt16(24, telemetry.workMode, true);
  view.setUint16(26, telemetry.uptimeSeconds, true);
  frame[28] = telemetry.peerOnline ? 0x01 : 0x00;
  frame[29] = 0;
  view.setUint16(CRC_OFFSET, crc16Ccitt(frame.subarray(0, CRC_OFFSET)), true);
  return frame;
}
