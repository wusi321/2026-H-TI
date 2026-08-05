import { Uart2TelemetryParser } from "./protocol";
import type { ParserStats, TelemetryValues } from "../types";

export interface SerialReceiverCallbacks {
  onFrames: (frames: TelemetryValues[], receivedAtMs: number) => void;
  onStats: (stats: ParserStats) => void;
  onDisconnect: (reason?: string) => void;
}

export function supportsWebSerial(): boolean {
  return typeof navigator !== "undefined" && navigator.serial !== undefined;
}

export class SerialReceiver {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private running = false;
  private parser = new Uart2TelemetryParser();

  constructor(private callbacks: SerialReceiverCallbacks) {}

  async connect(baudRate = 9600): Promise<SerialPortInfo> {
    if (!navigator.serial) throw new Error("当前浏览器不支持 Web Serial");
    this.port = await navigator.serial.requestPort();
    await this.port.open({
      baudRate,
      dataBits: 8,
      stopBits: 1,
      parity: "none",
      flowControl: "none",
      bufferSize: 4096,
    });
    this.running = true;
    this.parser.reset();
    void this.readLoop();
    return this.port.getInfo();
  }

  async disconnect(): Promise<void> {
    this.running = false;
    await this.reader?.cancel().catch(() => undefined);
    this.reader = null;
    await this.port?.close().catch(() => undefined);
    this.port = null;
  }

  private async readLoop(): Promise<void> {
    const readable = this.port?.readable;
    if (!readable) return;
    this.reader = readable.getReader();
    try {
      while (this.running) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (!value || value.byteLength === 0) continue;
        const frames = this.parser.push(value);
        if (frames.length > 0) this.callbacks.onFrames(frames, Date.now());
        this.callbacks.onStats(this.parser.getStats());
      }
    } catch (error) {
      if (this.running) {
        this.callbacks.onDisconnect(
          error instanceof Error ? error.message : "串口读取失败",
        );
      }
    } finally {
      this.reader?.releaseLock();
      this.reader = null;
      if (this.running) this.callbacks.onDisconnect();
      this.running = false;
    }
  }
}
