import { describe, expect, it } from "vitest";
import {
  CRC_OFFSET,
  UartFrameParser,
  crc16Ccitt,
  encodeTelemetryFrame,
} from "./protocol";

const sample = encodeTelemetryFrame({
  sequence: 42,
  speedSetpoint: 36.5,
  leftSpeed: 34.25,
  rightSpeed: 38.75,
  yaw: -17.5,
  workMode: 8,
  uptimeSeconds: 1234,
  peerOnline: true,
});

describe("UART3 binary protocol", () => {
  it("decodes a valid 32-byte frame", () => {
    const parser = new UartFrameParser();
    const result = parser.push(sample, 1000);

    expect(result).toHaveLength(1);
    expect(result[0]).toMatchObject({
      sequence: 42,
      speedSetpoint: 36.5,
      leftSpeed: 34.25,
      rightSpeed: 38.75,
      yaw: -17.5,
      workMode: 8,
      uptimeSeconds: 1234,
      peerOnline: true,
      receivedAt: 1000,
    });
  });

  it("reassembles split frames and skips noise", () => {
    const parser = new UartFrameParser();
    expect(parser.push(new Uint8Array([0x01, 0x02, 0xaa]))).toHaveLength(0);
    expect(parser.push(sample.slice(1, 11))).toHaveLength(0);
    expect(parser.push(sample.slice(11))).toHaveLength(1);
    expect(parser.getStats().discardedBytes).toBe(2);
  });

  it("rejects a frame with an invalid CRC", () => {
    const parser = new UartFrameParser();
    const damaged = sample.slice();
    damaged[12] ^= 0x40;
    expect(parser.push(damaged)).toHaveLength(0);
    expect(parser.getStats().crcErrors).toBe(1);
  });

  it("writes the firmware-compatible little-endian CRC", () => {
    const view = new DataView(sample.buffer);
    expect(view.getUint16(CRC_OFFSET, true)).toBe(
      crc16Ccitt(sample.subarray(0, CRC_OFFSET)),
    );
  });
});
