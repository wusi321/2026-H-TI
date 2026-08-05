export interface TelemetryFrame {
  sequence: number;
  speedSetpoint: number;
  leftSpeed: number;
  rightSpeed: number;
  yaw: number;
  workMode: number;
  uptimeSeconds: number;
  peerOnline: boolean;
  receivedAt: number;
}

export interface ParserStats {
  bytesReceived: number;
  validFrames: number;
  crcErrors: number;
  formatErrors: number;
  discardedBytes: number;
}

export interface TrackPoint {
  x: number;
  y: number;
  yaw: number;
  timestamp: number;
}

export interface SerialConfig {
  baudRate: number;
  dataBits: 7 | 8;
  stopBits: 1 | 2;
  parity: "none" | "even" | "odd";
  flowControl: "none" | "hardware";
}

export interface PortIdentity {
  usbVendorId?: number;
  usbProductId?: number;
  bluetoothServiceClassId?: string;
}
