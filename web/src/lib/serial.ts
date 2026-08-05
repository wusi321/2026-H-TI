import type { PortIdentity, SerialConfig } from "../types";

export type SerialDataHandler = (data: Uint8Array) => void;
export type SerialErrorHandler = (error: Error) => void;

export function isWebSerialSupported(): boolean {
  return typeof navigator !== "undefined" && navigator.serial !== undefined;
}

export class SerialReceiver {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private readTask: Promise<void> | null = null;
  private closing = false;

  async connect(
    config: SerialConfig,
    onData: SerialDataHandler,
    onError: SerialErrorHandler,
  ): Promise<PortIdentity> {
    if (!navigator.serial) {
      throw new Error("当前浏览器不支持 Web Serial，请使用最新版 Chrome 或 Edge");
    }
    if (this.port !== null) {
      throw new Error("串口已连接");
    }

    const port = await navigator.serial.requestPort();
    await port.open({
      baudRate: config.baudRate,
      dataBits: config.dataBits,
      stopBits: config.stopBits,
      parity: config.parity,
      flowControl: config.flowControl,
      bufferSize: 4096,
    });

    this.port = port;
    this.closing = false;
    this.readTask = this.readLoop(onData, onError);
    return port.getInfo();
  }

  async disconnect(): Promise<void> {
    if (this.port === null) {
      return;
    }

    this.closing = true;
    try {
      await this.reader?.cancel();
    } catch {
      // A removed USB device may reject cancellation; cleanup still continues.
    }
    await this.readTask;

    try {
      await this.port.close();
    } finally {
      this.port = null;
      this.readTask = null;
      this.closing = false;
    }
  }

  private async readLoop(
    onData: SerialDataHandler,
    onError: SerialErrorHandler,
  ): Promise<void> {
    const readable = this.port?.readable;
    if (readable === null || readable === undefined) {
      onError(new Error("串口没有可读取的数据流"));
      return;
    }

    this.reader = readable.getReader();
    try {
      while (!this.closing) {
        const { value, done } = await this.reader.read();
        if (done) {
          break;
        }
        if (value && value.byteLength > 0) {
          onData(value);
        }
      }
    } catch (reason) {
      if (!this.closing) {
        onError(reason instanceof Error ? reason : new Error(String(reason)));
      }
    } finally {
      this.reader.releaseLock();
      this.reader = null;
    }
  }
}
