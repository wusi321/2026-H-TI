import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { LucideIcon } from "lucide-react";
import {
  Activity,
  AlertTriangle,
  Cable,
  CheckCircle2,
  Clock3,
  Compass,
  Download,
  Gauge,
  LocateFixed,
  Pause,
  Play,
  RadioTower,
  RotateCcw,
  Route,
  Settings2,
  Unplug,
  Usb,
  WifiOff,
  ZoomIn,
  ZoomOut,
} from "lucide-react";
import { PathCanvas } from "./components/PathCanvas";
import { SpeedChart } from "./components/SpeedChart";
import { VehicleSchematic } from "./components/VehicleSchematic";
import { UartFrameParser, encodeTelemetryFrame } from "./lib/protocol";
import { SerialReceiver, isWebSerialSupported } from "./lib/serial";
import type {
  ParserStats,
  PortIdentity,
  SerialConfig,
  TelemetryFrame,
  TrackPoint,
} from "./types";

type ConnectionState = "disconnected" | "connecting" | "connected" | "error";

const EMPTY_STATS: ParserStats = {
  bytesReceived: 0,
  validFrames: 0,
  crcErrors: 0,
  formatErrors: 0,
  discardedBytes: 0,
};

const ORIGIN: TrackPoint = { x: 0, y: 0, yaw: 0, timestamp: 0 };

const MODE_NAMES: Record<number, string> = {
  [-10]: "电机方向调试",
  [-9]: "舵机中值调试",
  [-2]: "遥控调速测试",
  [-1]: "按键调速测试",
  0: "遥控控制",
  1: "灰度自主循迹",
  2: "顺时针转向",
  3: "逆时针转向",
  4: "定向子任务",
  5: "定角速度旋转",
  6: "超声波避撞",
  7: "两轮平衡",
  8: "差速平台控制",
  9: "航点控制",
  10: "OpenMV 视觉循迹",
  11: "舵机遥控",
  12: "舵机视觉循迹",
  13: "倒车入库",
  14: "侧方停车",
  15: "赛道跟随",
  16: "2024 H题任务一",
  17: "2024 H题任务二",
  18: "2024 H题任务三",
  19: "2024 H题任务四",
  20: "2024 H题发挥",
  21: "原地掉头",
  22: "从机跟随",
};

function modeName(mode: number): string {
  return MODE_NAMES[mode] ?? `自定义模式 ${mode}`;
}

function formatUptime(totalSeconds: number): string {
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return [hours, minutes, seconds]
    .map((value) => value.toString().padStart(2, "0"))
    .join(":");
}

function formatHex(value?: number): string {
  return value === undefined ? "----" : value.toString(16).toUpperCase().padStart(4, "0");
}

function portLabel(identity: PortIdentity | null): string {
  if (!identity) return "等待选择设备";
  if (identity.bluetoothServiceClassId) return "Bluetooth SPP";
  return `USB ${formatHex(identity.usbVendorId)}:${formatHex(identity.usbProductId)}`;
}

function MetricCard({
  icon: Icon,
  label,
  value,
  unit,
  tone = "default",
}: {
  icon: LucideIcon;
  label: string;
  value: string;
  unit?: string;
  tone?: "default" | "green" | "amber" | "red";
}) {
  return (
    <article className={`metric-card metric-card--${tone}`}>
      <div className="metric-card__top">
        <span className="metric-card__icon"><Icon size={17} /></span>
        <span>{label}</span>
      </div>
      <div className="metric-card__value">
        <strong>{value}</strong>
        {unit && <span>{unit}</span>}
      </div>
    </article>
  );
}

function ConfigSelect({
  label,
  value,
  disabled,
  onChange,
  children,
}: {
  label: string;
  value: string | number;
  disabled: boolean;
  onChange: (value: string) => void;
  children: React.ReactNode;
}) {
  return (
    <label className="config-field">
      <span>{label}</span>
      <select
        value={value}
        disabled={disabled}
        onChange={(event) => onChange(event.target.value)}
      >
        {children}
      </select>
    </label>
  );
}

export default function App() {
  const serialRef = useRef(new SerialReceiver());
  const parserRef = useRef(new UartFrameParser());
  const previousSequenceRef = useRef<number | null>(null);
  const recentFrameTimesRef = useRef<number[]>([]);
  const lastValidFrameAtRef = useRef<number | null>(null);
  const trackPausedRef = useRef(false);
  const positionRef = useRef({ ...ORIGIN, lastTimestamp: null as number | null });

  const [config, setConfig] = useState<SerialConfig>({
    baudRate: 9600,
    dataBits: 8,
    stopBits: 1,
    parity: "none",
    flowControl: "none",
  });
  const [connectionState, setConnectionState] = useState<ConnectionState>("disconnected");
  const [portIdentity, setPortIdentity] = useState<PortIdentity | null>(null);
  const [errorMessage, setErrorMessage] = useState("");
  const [demoActive, setDemoActive] = useState(false);
  const [linkOnline, setLinkOnline] = useState(false);
  const [telemetry, setTelemetry] = useState<TelemetryFrame | null>(null);
  const [stats, setStats] = useState<ParserStats>(EMPTY_STATS);
  const [droppedFrames, setDroppedFrames] = useState(0);
  const [frameRate, setFrameRate] = useState(0);
  const [records, setRecords] = useState<TelemetryFrame[]>([]);
  const [trackPoints, setTrackPoints] = useState<TrackPoint[]>([]);
  const [currentPosition, setCurrentPosition] = useState<TrackPoint>(ORIGIN);
  const [trackPaused, setTrackPaused] = useState(false);
  const [zoom, setZoom] = useState(2);
  const [viewMode, setViewMode] = useState<"follow" | "fit">("follow");

  const serialSupported = isWebSerialSupported();
  const isPortOpen = connectionState === "connected" || connectionState === "error";

  useEffect(() => {
    trackPausedRef.current = trackPaused;
  }, [trackPaused]);

  const clearSessionData = useCallback(() => {
    parserRef.current.reset();
    previousSequenceRef.current = null;
    recentFrameTimesRef.current = [];
    lastValidFrameAtRef.current = null;
    positionRef.current = { ...ORIGIN, lastTimestamp: null };
    setTelemetry(null);
    setStats({ ...EMPTY_STATS });
    setDroppedFrames(0);
    setFrameRate(0);
    setRecords([]);
    setTrackPoints([]);
    setCurrentPosition({ ...ORIGIN });
    setLinkOnline(false);
  }, []);

  const acceptFrame = useCallback((frame: TelemetryFrame) => {
    setTelemetry(frame);
    setRecords((previous) => [...previous, frame].slice(-10000));
    lastValidFrameAtRef.current = performance.now();
    setLinkOnline(true);

    const previousSequence = previousSequenceRef.current;
    if (previousSequence !== null) {
      const delta = (frame.sequence - previousSequence) >>> 0;
      if (delta > 1 && delta < 0x80000000) {
        setDroppedFrames((count) => count + delta - 1);
      }
    }
    previousSequenceRef.current = frame.sequence;

    const now = performance.now();
    const recent = recentFrameTimesRef.current.filter((time) => now - time < 1000);
    recent.push(now);
    recentFrameTimesRef.current = recent;
    setFrameRate(recent.length);

    const position = positionRef.current;
    if (position.lastTimestamp === null) {
      const first = { x: 0, y: 0, yaw: frame.yaw, timestamp: frame.receivedAt };
      positionRef.current = { ...first, lastTimestamp: frame.receivedAt };
      setCurrentPosition(first);
      setTrackPoints([first]);
      return;
    }

    const elapsed = Math.max(0, Math.min(0.25, (frame.receivedAt - position.lastTimestamp) / 1000));
    if (trackPausedRef.current) {
      positionRef.current.lastTimestamp = frame.receivedAt;
      positionRef.current.yaw = frame.yaw;
      setCurrentPosition({
        x: position.x,
        y: position.y,
        yaw: frame.yaw,
        timestamp: frame.receivedAt,
      });
      return;
    }

    const speed = (frame.leftSpeed + frame.rightSpeed) / 2;
    const yawRadians = (frame.yaw * Math.PI) / 180;
    const next: TrackPoint = {
      x: position.x + speed * elapsed * Math.cos(yawRadians),
      y: position.y + speed * elapsed * Math.sin(yawRadians),
      yaw: frame.yaw,
      timestamp: frame.receivedAt,
    };
    positionRef.current = { ...next, lastTimestamp: frame.receivedAt };
    setCurrentPosition(next);
    setTrackPoints((previous) => [...previous, next].slice(-5000));
  }, []);

  const receiveBytes = useCallback((bytes: Uint8Array) => {
    const frames = parserRef.current.push(bytes, performance.now());
    setStats(parserRef.current.getStats());
    frames.forEach(acceptFrame);
  }, [acceptFrame]);

  useEffect(() => {
    const timer = window.setInterval(() => {
      const lastFrame = lastValidFrameAtRef.current;
      if (lastFrame !== null && performance.now() - lastFrame > 1000) {
        setLinkOnline(false);
        setFrameRate(0);
      }
    }, 250);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => {
    if (!demoActive) return;
    let sequence = 0;
    let elapsed = 0;
    let yaw = 0;
    const interval = window.setInterval(() => {
      elapsed += 0.05;
      const setpoint = 38 + Math.sin(elapsed * 0.6) * 9;
      const turn = Math.sin(elapsed * 0.9) * 10 + Math.sin(elapsed * 0.23) * 5;
      const leftSpeed = setpoint - turn;
      const rightSpeed = setpoint + turn;
      yaw += (((rightSpeed - leftSpeed) / 12.8) * 180 / Math.PI) * 0.05;
      yaw = ((yaw + 180) % 360) - 180;
      const frame = encodeTelemetryFrame({
        sequence,
        speedSetpoint: setpoint,
        leftSpeed,
        rightSpeed,
        yaw,
        workMode: 22,
        uptimeSeconds: Math.floor(elapsed),
        peerOnline: true,
      });
      const split = 5 + (sequence % 21);
      receiveBytes(frame.slice(0, split));
      receiveBytes(frame.slice(split));
      sequence = (sequence + 1) >>> 0;
    }, 50);
    return () => window.clearInterval(interval);
  }, [demoActive, receiveBytes]);

  useEffect(() => {
    return () => {
      void serialRef.current.disconnect();
    };
  }, []);

  const connect = async () => {
    setDemoActive(false);
    clearSessionData();
    setErrorMessage("");
    setConnectionState("connecting");
    try {
      const identity = await serialRef.current.connect(
        config,
        receiveBytes,
        (error) => {
          setErrorMessage(error.message);
          setConnectionState("error");
          setLinkOnline(false);
        },
      );
      setPortIdentity(identity);
      setConnectionState("connected");
    } catch (reason) {
      const error = reason instanceof Error ? reason : new Error(String(reason));
      setErrorMessage(error.name === "NotFoundError" ? "未选择串口设备" : error.message);
      setConnectionState("disconnected");
    }
  };

  const disconnect = async () => {
    try {
      await serialRef.current.disconnect();
    } catch (reason) {
      setErrorMessage(reason instanceof Error ? reason.message : String(reason));
    } finally {
      setConnectionState("disconnected");
      setPortIdentity(null);
      setLinkOnline(false);
    }
  };

  const toggleDemo = () => {
    if (demoActive) {
      setDemoActive(false);
      setLinkOnline(false);
      return;
    }
    clearSessionData();
    setErrorMessage("");
    setDemoActive(true);
  };

  const resetTrack = () => {
    const resetPoint = {
      ...ORIGIN,
      yaw: telemetry?.yaw ?? 0,
      timestamp: telemetry?.receivedAt ?? 0,
    };
    positionRef.current = {
      ...resetPoint,
      lastTimestamp: telemetry?.receivedAt ?? null,
    };
    setCurrentPosition(resetPoint);
    setTrackPoints(telemetry ? [resetPoint] : []);
  };

  const exportCsv = () => {
    if (records.length === 0) return;
    const header = [
      "sequence",
      "speed_setpoint_cmps",
      "left_speed_cmps",
      "right_speed_cmps",
      "yaw_deg",
      "work_mode",
      "uptime_seconds",
      "peer_online",
    ];
    const rows = records.map((frame) => [
      frame.sequence,
      frame.speedSetpoint.toFixed(4),
      frame.leftSpeed.toFixed(4),
      frame.rightSpeed.toFixed(4),
      frame.yaw.toFixed(4),
      frame.workMode,
      frame.uptimeSeconds,
      frame.peerOnline ? 1 : 0,
    ].join(","));
    const blob = new Blob([[header.join(","), ...rows].join("\n")], {
      type: "text/csv;charset=utf-8",
    });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = `ncontroller-telemetry-${new Date().toISOString().replaceAll(":", "-")}.csv`;
    anchor.click();
    URL.revokeObjectURL(url);
  };

  const distance = useMemo(() => {
    let total = 0;
    for (let index = 1; index < trackPoints.length; index += 1) {
      total += Math.hypot(
        trackPoints[index].x - trackPoints[index - 1].x,
        trackPoints[index].y - trackPoints[index - 1].y,
      );
    }
    return total;
  }, [trackPoints]);

  const headerState = demoActive
    ? { label: "演示运行", tone: "demo" }
    : connectionState === "connecting"
      ? { label: "正在连接", tone: "waiting" }
      : connectionState === "error"
        ? { label: "连接异常", tone: "error" }
        : connectionState === "connected" && linkOnline
          ? { label: "数据在线", tone: "online" }
          : connectionState === "connected"
            ? { label: "等待数据", tone: "waiting" }
            : { label: "未连接", tone: "offline" };

  const current = telemetry ?? {
    sequence: 0,
    speedSetpoint: 0,
    leftSpeed: 0,
    rightSpeed: 0,
    yaw: 0,
    workMode: 0,
    uptimeSeconds: 0,
    peerOnline: false,
    receivedAt: 0,
  };

  return (
    <div className="app-shell">
      <header className="app-header">
        <div className="brand">
          <span className="brand-mark"><RadioTower size={22} /></span>
          <div>
            <h1>NC Motion Console</h1>
            <p>MSPM0G3507 UART3 遥测</p>
          </div>
        </div>
        <div className={`header-status header-status--${headerState.tone}`}>
          <span className="status-dot" />
          {headerState.label}
        </div>
      </header>

      <main className="dashboard">
        <section className="connection-band" aria-labelledby="serial-title">
          <div className="connection-heading">
            <span className="section-icon"><Settings2 size={18} /></span>
            <div>
              <h2 id="serial-title">串口连接</h2>
              <p>{portLabel(portIdentity)}</p>
            </div>
          </div>
          <div className="serial-config">
            <ConfigSelect
              label="波特率"
              value={config.baudRate}
              disabled={isPortOpen || connectionState === "connecting"}
              onChange={(value) => setConfig({ ...config, baudRate: Number(value) })}
            >
              {[9600, 19200, 38400, 57600, 115200].map((value) => (
                <option key={value} value={value}>{value}</option>
              ))}
            </ConfigSelect>
            <ConfigSelect
              label="数据位"
              value={config.dataBits}
              disabled={isPortOpen || connectionState === "connecting"}
              onChange={(value) => setConfig({ ...config, dataBits: Number(value) as 7 | 8 })}
            >
              <option value={8}>8</option>
              <option value={7}>7</option>
            </ConfigSelect>
            <ConfigSelect
              label="停止位"
              value={config.stopBits}
              disabled={isPortOpen || connectionState === "connecting"}
              onChange={(value) => setConfig({ ...config, stopBits: Number(value) as 1 | 2 })}
            >
              <option value={1}>1</option>
              <option value={2}>2</option>
            </ConfigSelect>
            <ConfigSelect
              label="校验"
              value={config.parity}
              disabled={isPortOpen || connectionState === "connecting"}
              onChange={(value) => setConfig({ ...config, parity: value as SerialConfig["parity"] })}
            >
              <option value="none">无校验</option>
              <option value="even">偶校验</option>
              <option value="odd">奇校验</option>
            </ConfigSelect>
            <ConfigSelect
              label="流控"
              value={config.flowControl}
              disabled={isPortOpen || connectionState === "connecting"}
              onChange={(value) => setConfig({ ...config, flowControl: value as SerialConfig["flowControl"] })}
            >
              <option value="none">无</option>
              <option value="hardware">硬件</option>
            </ConfigSelect>
          </div>
          <div className="connection-actions">
            {isPortOpen ? (
              <button className="button button--danger" onClick={() => void disconnect()}>
                <Unplug size={17} />断开
              </button>
            ) : (
              <button
                className="button button--primary"
                disabled={!serialSupported || connectionState === "connecting"}
                onClick={() => void connect()}
              >
                <Usb size={17} />
                {connectionState === "connecting" ? "连接中" : "选择并连接"}
              </button>
            )}
            <button
              className={`button button--secondary ${demoActive ? "is-active" : ""}`}
              disabled={isPortOpen || connectionState === "connecting"}
              onClick={toggleDemo}
            >
              {demoActive ? <Pause size={17} /> : <Play size={17} />}
              {demoActive ? "停止演示" : "演示数据"}
            </button>
          </div>
        </section>

        {!serialSupported && (
          <div className="notice notice--warning">
            <AlertTriangle size={18} />
            <span>当前浏览器不支持 Web Serial，请使用最新版 Chrome 或 Edge，通过本机地址访问。</span>
          </div>
        )}
        {errorMessage && (
          <div className="notice notice--error">
            <AlertTriangle size={18} />
            <span>{errorMessage}</span>
            <button aria-label="关闭错误信息" title="关闭" onClick={() => setErrorMessage("")}>×</button>
          </div>
        )}

        <section className="metrics-grid" aria-label="实时遥测数据">
          <MetricCard icon={Gauge} label="速度设定" value={current.speedSetpoint.toFixed(1)} unit="cm/s" tone="amber" />
          <MetricCard icon={Activity} label="左轮速度" value={current.leftSpeed.toFixed(1)} unit="cm/s" tone="green" />
          <MetricCard icon={Activity} label="右轮速度" value={current.rightSpeed.toFixed(1)} unit="cm/s" tone="red" />
          <MetricCard icon={Compass} label="偏航角" value={current.yaw.toFixed(1)} unit="deg" />
          <MetricCard icon={Cable} label="工作模式" value={String(current.workMode)} unit={modeName(current.workMode)} />
          <MetricCard icon={Clock3} label="开机时间" value={formatUptime(current.uptimeSeconds)} />
        </section>

        <section className="primary-grid">
          <article className="panel path-panel">
            <div className="panel-header">
              <div>
                <h2><Route size={18} />运动路径</h2>
                <p>X {currentPosition.x.toFixed(1)} cm · Y {currentPosition.y.toFixed(1)} cm · 路程 {distance.toFixed(1)} cm</p>
              </div>
              <div className="icon-toolbar">
                <button
                  className={viewMode === "follow" ? "is-active" : ""}
                  aria-label="跟随小车"
                  title="跟随小车"
                  onClick={() => setViewMode(viewMode === "follow" ? "fit" : "follow")}
                ><LocateFixed size={17} /></button>
                <button aria-label="放大路径" title="放大" onClick={() => setZoom((value) => Math.min(8, value * 1.25))}><ZoomIn size={17} /></button>
                <button aria-label="缩小路径" title="缩小" onClick={() => setZoom((value) => Math.max(0.2, value / 1.25))}><ZoomOut size={17} /></button>
                <button
                  className={trackPaused ? "is-active" : ""}
                  aria-label={trackPaused ? "继续记录路径" : "暂停记录路径"}
                  title={trackPaused ? "继续记录" : "暂停记录"}
                  onClick={() => setTrackPaused((value) => !value)}
                >{trackPaused ? <Play size={17} /> : <Pause size={17} />}</button>
                <button aria-label="清空路径" title="清空路径" onClick={resetTrack}><RotateCcw size={17} /></button>
              </div>
            </div>
            <PathCanvas points={trackPoints} current={currentPosition} zoom={zoom} viewMode={viewMode} />
          </article>

          <article className="panel vehicle-panel">
            <div className="panel-header">
              <div>
                <h2><Gauge size={18} />车体状态</h2>
                <p>航向 {current.yaw.toFixed(1)}° · {modeName(current.workMode)}</p>
              </div>
              <span className={`peer-state ${current.peerOnline ? "is-online" : ""}`}>
                {current.peerOnline ? <CheckCircle2 size={15} /> : <WifiOff size={15} />}
                对端{current.peerOnline ? "在线" : "未确认"}
              </span>
            </div>
            <VehicleSchematic
              leftSpeed={current.leftSpeed}
              rightSpeed={current.rightSpeed}
              setpoint={current.speedSetpoint}
              yaw={current.yaw}
            />
          </article>
        </section>

        <section className="secondary-grid">
          <article className="panel chart-panel">
            <div className="panel-header">
              <div>
                <h2><Activity size={18} />轮速趋势</h2>
                <p>最近 {Math.min(records.length, 240)} 帧</p>
              </div>
              <div className="chart-legend">
                <span className="legend-target">设定</span>
                <span className="legend-left">左轮</span>
                <span className="legend-right">右轮</span>
              </div>
            </div>
            <SpeedChart frames={records} />
          </article>

          <article className="panel diagnostics-panel">
            <div className="panel-header">
              <div>
                <h2><RadioTower size={18} />通信诊断</h2>
                <p>CRC-16/CCITT-FALSE · 32 bytes</p>
              </div>
              <span className="rate-badge">{frameRate} fps</span>
            </div>
            <div className="diagnostic-grid">
              <div><span>有效帧</span><strong>{stats.validFrames.toLocaleString()}</strong></div>
              <div><span>接收字节</span><strong>{stats.bytesReceived.toLocaleString()}</strong></div>
              <div><span>丢失帧</span><strong>{droppedFrames.toLocaleString()}</strong></div>
              <div><span>CRC 错误</span><strong className={stats.crcErrors ? "text-danger" : ""}>{stats.crcErrors}</strong></div>
              <div><span>格式错误</span><strong className={stats.formatErrors ? "text-danger" : ""}>{stats.formatErrors}</strong></div>
              <div><span>丢弃字节</span><strong>{stats.discardedBytes.toLocaleString()}</strong></div>
            </div>
          </article>
        </section>

        <section className="panel records-panel">
          <div className="panel-header">
            <div>
              <h2><Cable size={18} />最近数据帧</h2>
              <p>序号 {current.sequence.toLocaleString()} · 本次记录 {records.length.toLocaleString()} 帧</p>
            </div>
            <button className="button button--compact" disabled={records.length === 0} onClick={exportCsv}>
              <Download size={16} />导出 CSV
            </button>
          </div>
          <div className="table-scroll">
            <table>
              <thead>
                <tr>
                  <th>序号</th><th>设定 cm/s</th><th>左轮 cm/s</th><th>右轮 cm/s</th>
                  <th>偏航 deg</th><th>模式</th><th>开机时间</th><th>对端</th>
                </tr>
              </thead>
              <tbody>
                {records.length === 0 ? (
                  <tr><td colSpan={8} className="empty-cell">暂无有效数据帧</td></tr>
                ) : records.slice(-8).reverse().map((frame) => (
                  <tr key={`${frame.sequence}-${frame.receivedAt}`}>
                    <td>{frame.sequence}</td>
                    <td>{frame.speedSetpoint.toFixed(2)}</td>
                    <td>{frame.leftSpeed.toFixed(2)}</td>
                    <td>{frame.rightSpeed.toFixed(2)}</td>
                    <td>{frame.yaw.toFixed(2)}</td>
                    <td><span className="mode-cell">{frame.workMode} · {modeName(frame.workMode)}</span></td>
                    <td>{formatUptime(frame.uptimeSeconds)}</td>
                    <td><span className={`table-status ${frame.peerOnline ? "is-online" : ""}`}>{frame.peerOnline ? "在线" : "未确认"}</span></td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>
      </main>
      <footer>
        <span>UART3 · AA 55 · Version 1</span>
        <span>{linkOnline ? "最近 1 秒内收到有效帧" : "链路未就绪"}</span>
      </footer>
    </div>
  );
}
