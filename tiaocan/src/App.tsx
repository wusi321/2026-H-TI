import {
  Activity,
  Bluetooth,
  Cable,
  CircleStop,
  Database,
  Pause,
  Play,
  Radio,
  Save,
  Unplug,
} from "lucide-react";
import {
  useCallback,
  useDeferredValue,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import { BeamVisualization } from "./components/BeamVisualization";
import { MetricsStrip } from "./components/MetricsStrip";
import { ParameterPanel } from "./components/ParameterPanel";
import { RunLibrary } from "./components/RunLibrary";
import { TelemetryChart } from "./components/TelemetryChart";
import { downloadCsv, parseCsv } from "./lib/csv";
import { taskMatchesProfile } from "./lib/protocol";
import { SerialReceiver, supportsWebSerial } from "./lib/serial";
import {
  DEFAULT_PARAMETERS,
  fitPlantModel,
  metricsForActual,
  metricsForSimulation,
  parametersToC,
  simulateRun,
} from "./lib/simulation";
import {
  appendSamples,
  createRun,
  deleteRun,
  finishRun,
  listRuns,
  loadProfile,
  loadRunSamples,
  saveProfile,
} from "./lib/storage";
import type {
  ParserStats,
  PlantFitResult,
  RunMetadata,
  TaskProfileId,
  TelemetrySample,
  TelemetryValues,
  TuningParameters,
} from "./types";

type ConnectionState = "disconnected" | "connecting" | "connected" | "error";

interface RecordingContext {
  id: string;
  startedAtMs: number;
  nextIndex: number;
  lastReceivedAtMs: number;
  pending: TelemetrySample[];
  metadata: RunMetadata;
}

const TASKS: { id: TaskProfileId; label: string; target: string }[] = [
  { id: "task3", label: "任务 3", target: "0 / +5 / -5 cm" },
  { id: "task45", label: "任务 4/5", target: "行驶中保持 0 cm" },
  { id: "task6", label: "任务 6", target: "记忆位置 / -7.40 cm" },
  { id: "task7", label: "任务 7", target: "记忆位置 / +7.20 cm" },
];

const EMPTY_STATS: ParserStats = {
  bytesReceived: 0,
  validFrames: 0,
  checksumErrors: 0,
  formatErrors: 0,
  discardedBytes: 0,
};

function taskName(task: TaskProfileId): string {
  return TASKS.find((item) => item.id === task)?.label ?? task;
}

function createDemoSamples(task: TaskProfileId): TelemetrySample[] {
  const durationS = task === "task3" ? 12 : 18;
  let position = task === "task6" ? -6.5 : task === "task7" ? 6.2 : 0;
  let velocity = 0;
  const samples: TelemetrySample[] = [];
  for (let index = 0; index <= durationS * 20; index += 1) {
    const timeS = index * 0.05;
    const target = task === "task3"
      ? timeS < 1.5 ? 0 : timeS < 5.5 ? 5 : -5
      : task === "task6" ? -7.4 : task === "task7" ? 7.2 : 0;
    const vehicleSpeed = task === "task3"
      ? 0
      : timeS < 2.5
        ? 30 * (timeS / 2.5) ** 2 * (3 - 2 * timeS / 2.5)
        : 30 + 3 * Math.sin(timeS * 0.9);
    const vehicleAcceleration = index === 0
      ? 0
      : (vehicleSpeed - samples[index - 1].vehicleSpeedCmS) / 0.05;
    const error = target - position;
    const servo = Math.max(-24, Math.min(24, error * 4.2 - velocity * 1.4 + vehicleAcceleration * 0.08));
    const acceleration = servo * 0.8 - velocity * 1.2 - vehicleAcceleration * 0.7;
    velocity += acceleration * 0.05;
    position += velocity * 0.05;
    const noise = 0.04 * Math.sin(index * 1.73);
    samples.push({
      index,
      receivedAtMs: 1_700_000_000_000 + index * 50,
      elapsedMs: index * 50,
      protocolVersion: 2,
      ballPositionCm: position + noise,
      ballVelocityCmS: velocity,
      ballDirection: Math.abs(velocity) < 0.5 ? 0 : Math.sign(velocity),
      targetPositionCm: target,
      vehicleSpeedCmS: vehicleSpeed,
      vehicleCommandSpeedCmS: vehicleSpeed,
      vehicleMeasuredAccelerationCmS2: vehicleAcceleration,
      vehicleCommandAccelerationCmS2: vehicleAcceleration,
      leftMotorSpeedCmS: vehicleSpeed,
      rightMotorSpeedCmS: vehicleSpeed,
      yawDeg: task === "task3" ? 0 : timeS * 8,
      yawRateDps: task === "task3" ? 0 : 8,
      servoAngleDeg: servo,
      servoSpeed: 800 + Math.min(400, Math.abs(servo) * 20),
      feedforwardServoDeg: task === "task3" ? 0 : vehicleAcceleration * 0.08,
      turnCompensationServoDeg: 0,
      taskPhase: 0,
      routeMode: task === "task3" ? 0 : 1,
      statusFlags: 0x03,
      sequence: index,
      taskId: task === "task3" ? 3 : task === "task6" ? 6 : task === "task7" ? 7 : 4,
    });
  }
  return samples;
}

export function App() {
  const [task, setTask] = useState<TaskProfileId>("task3");
  const [parameters, setParameters] = useState<TuningParameters>(DEFAULT_PARAMETERS.task3);
  const [connection, setConnection] = useState<ConnectionState>("disconnected");
  const [portInfo, setPortInfo] = useState<SerialPortInfo | null>(null);
  const [stats, setStats] = useState<ParserStats>(EMPTY_STATS);
  const [liveSamples, setLiveSamples] = useState<TelemetrySample[]>([]);
  const [runs, setRuns] = useState<RunMetadata[]>([]);
  const [activeRun, setActiveRun] = useState<RunMetadata | null>(null);
  const [loadedSamples, setLoadedSamples] = useState<TelemetrySample[]>([]);
  const [recording, setRecording] = useState(false);
  const [replayIndex, setReplayIndex] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [replaySpeed, setReplaySpeed] = useState(1);
  const [fit, setFit] = useState<PlantFitResult | null>(null);
  const [message, setMessage] = useState("等待连接 HC-05");
  const [taskMismatchFrames, setTaskMismatchFrames] = useState(0);

  const receiverRef = useRef<SerialReceiver | null>(null);
  const recordingRef = useRef<RecordingContext | null>(null);
  const liveStartRef = useRef<number | null>(null);
  const liveIndexRef = useRef(0);
  const liveTaskIdRef = useRef<number | null>(null);
  const taskRef = useRef<TaskProfileId>(task);
  const writeChainRef = useRef<Promise<void>>(Promise.resolve());
  taskRef.current = task;

  const refreshRuns = useCallback(async () => {
    setRuns(await listRuns());
  }, []);

  const queueSamples = useCallback((runId: string, samples: TelemetrySample[]) => {
    if (samples.length === 0) return;
    writeChainRef.current = writeChainRef.current.then(() => appendSamples(runId, samples));
  }, []);

  const finalizeRecording = useCallback(async (context: RecordingContext) => {
    if (recordingRef.current?.id === context.id) recordingRef.current = null;
    if (context.pending.length > 0) {
      queueSamples(context.id, context.pending.splice(0));
    }
    await writeChainRef.current;
    await finishRun(
      context.id,
      Date.now(),
      context.nextIndex,
      context.metadata.actualTaskId,
    );
    setRecording(false);
    await refreshRuns();
    setMessage(`已保存 ${context.nextIndex} 个校验有效样本`);
  }, [queueSamples, refreshRuns]);

  const processFrames = useCallback((frames: TelemetryValues[], receivedAtMs: number) => {
    const context = recordingRef.current;
    const profileFrames = frames.filter((frame) =>
      taskMatchesProfile(taskRef.current, frame.taskId),
    );
    let rejected = frames.length - profileFrames.length;
    let lockedTaskId = context?.metadata.actualTaskId ?? liveTaskIdRef.current;

    if (!context && lockedTaskId !== null && profileFrames.length > 0 &&
        profileFrames.every((frame) => frame.taskId !== lockedTaskId)) {
      lockedTaskId = profileFrames[0].taskId;
      liveTaskIdRef.current = lockedTaskId;
      liveStartRef.current = null;
      liveIndexRef.current = 0;
      setLiveSamples([]);
    }

    const acceptedFrames = profileFrames.filter((frame) => {
      if (lockedTaskId === null || lockedTaskId === undefined) {
        lockedTaskId = frame.taskId;
        liveTaskIdRef.current = frame.taskId;
        if (context) context.metadata.actualTaskId = frame.taskId;
      }
      if (frame.taskId === lockedTaskId) return true;
      rejected += 1;
      return false;
    });
    if (rejected > 0) setTaskMismatchFrames((current) => current + rejected);
    if (acceptedFrames.length === 0) return;

    if (liveStartRef.current === null) liveStartRef.current = receivedAtMs;
    const nominalIntervalMs = 50;
    const batchStart = receivedAtMs - (acceptedFrames.length - 1) * nominalIntervalMs;
    const newSamples = acceptedFrames.map((frame, batchIndex) => {
      const estimatedTime = batchStart + batchIndex * nominalIntervalMs;
      if (context) {
        const time = Math.max(context.lastReceivedAtMs + 1, estimatedTime);
        context.lastReceivedAtMs = time;
        return {
          ...frame,
          index: context.nextIndex++,
          receivedAtMs: time,
          elapsedMs: time - context.startedAtMs,
        };
      }
      const index = liveIndexRef.current++;
      return {
        ...frame,
        index,
        receivedAtMs: estimatedTime,
        elapsedMs: estimatedTime - liveStartRef.current!,
      };
    });
    setLiveSamples((current) => [...current, ...newSamples].slice(-2400));
    setReplayIndex((current) => Math.max(current, newSamples.at(-1)?.index ?? current));
    if (context) {
      context.pending.push(...newSamples);
      if (context.pending.length >= 20) {
        queueSamples(context.id, context.pending.splice(0));
      }
    }
  }, [queueSamples]);

  useEffect(() => {
    receiverRef.current = new SerialReceiver({
      onFrames: processFrames,
      onStats: setStats,
      onDisconnect: (reason) => {
        setConnection(reason ? "error" : "disconnected");
        setMessage(reason ?? "串口已断开");
        const context = recordingRef.current;
        if (context) void finalizeRecording(context);
      },
    });
    void refreshRuns();
    return () => { void receiverRef.current?.disconnect(); };
  }, [finalizeRecording, processFrames, refreshRuns]);

  useEffect(() => {
    let cancelled = false;
    void loadProfile(task).then((profile) => {
      if (!cancelled) setParameters(profile ?? DEFAULT_PARAMETERS[task]);
    });
    setFit(null);
    return () => { cancelled = true; };
  }, [task]);

  const displaySamples = activeRun ? loadedSamples : liveSamples;
  const deferredParameters = useDeferredValue(parameters);
  const simulation = useMemo(
    () => simulateRun(displaySamples, deferredParameters),
    [deferredParameters, displaySamples],
  );
  const actualMetrics = useMemo(
    () => displaySamples.length ? metricsForActual(displaySamples) : null,
    [displaySamples],
  );
  const simulatedMetrics = useMemo(
    () => simulation.length ? metricsForSimulation(simulation) : null,
    [simulation],
  );

  useEffect(() => {
    if (!playing || displaySamples.length === 0) return;
    const timer = window.setInterval(() => {
      setReplayIndex((index) => {
        const next = index + Math.max(1, Math.round(replaySpeed));
        if (next >= displaySamples.length - 1) {
          setPlaying(false);
          return displaySamples.length - 1;
        }
        return next;
      });
    }, 50);
    return () => window.clearInterval(timer);
  }, [displaySamples.length, playing, replaySpeed]);

  const activeIndex = Math.min(
    Math.max(0, replayIndex),
    Math.max(0, displaySamples.length - 1),
  );
  const activeSample = displaySamples[activeIndex] ?? null;
  const activeSimulation = simulation[activeIndex];

  const connect = async () => {
    try {
      setConnection("connecting");
      setMessage("正在请求串口权限");
      const info = await receiverRef.current!.connect(9600);
      setPortInfo(info);
      setStats(EMPTY_STATS);
      setTaskMismatchFrames(0);
      setConnection("connected");
      setMessage("UART2 9600 8N1 已连接");
      setActiveRun(null);
    } catch (error) {
      setConnection("error");
      setMessage(error instanceof Error ? error.message : "串口连接失败");
    }
  };

  const disconnect = async () => {
    const context = recordingRef.current;
    if (context) await finalizeRecording(context);
    await receiverRef.current?.disconnect();
    setConnection("disconnected");
    setPortInfo(null);
    setMessage("串口已断开");
  };

  const startRecording = async () => {
    const startedAtMs = Date.now();
    const id = crypto.randomUUID();
    const metadata: RunMetadata = {
      id,
      name: `${taskName(task)} ${new Date(startedAtMs).toLocaleTimeString()}`,
      task,
      startedAtMs,
      sampleCount: 0,
      baudRate: 9600,
      notes: "",
    };
    await createRun(metadata);
    recordingRef.current = {
      id,
      startedAtMs,
      nextIndex: 0,
      lastReceivedAtMs: startedAtMs - 1,
      pending: [],
      metadata,
    };
    liveStartRef.current = startedAtMs;
    liveIndexRef.current = 0;
    liveTaskIdRef.current = null;
    setLiveSamples([]);
    setActiveRun(null);
    setReplayIndex(0);
    setRecording(true);
    setMessage(`${taskName(task)} 正在记录`);
  };

  const stopRecording = async () => {
    const context = recordingRef.current;
    if (context) await finalizeRecording(context);
  };

  const selectRun = async (run: RunMetadata) => {
    setPlaying(false);
    setActiveRun(run);
    setTask(run.task);
    const samples = await loadRunSamples(run.id);
    setLoadedSamples(samples);
    setReplayIndex(Math.max(0, samples.length - 1));
    setMessage(`已载入 ${samples.length} 个真实样本`);
  };

  const exportRun = async (run: RunMetadata) => {
    downloadCsv(run, await loadRunSamples(run.id));
  };

  const removeRun = async (run: RunMetadata) => {
    await deleteRun(run.id);
    if (activeRun?.id === run.id) {
      setActiveRun(null);
      setLoadedSamples([]);
    }
    await refreshRuns();
  };

  const importRun = async (file: File) => {
    try {
      const parsed = parseCsv(await file.text());
      const startedAtMs = Date.now();
      const id = crypto.randomUUID();
      const metadata: RunMetadata = {
        id,
        name: parsed.name,
        task: parsed.task,
        startedAtMs,
        endedAtMs: startedAtMs + (parsed.samples.at(-1)?.elapsedMs ?? 0),
        sampleCount: parsed.samples.length,
        baudRate: 9600,
        notes: `导入自 ${file.name}`,
        actualTaskId: parsed.samples.find((sample) => sample.taskId > 0)?.taskId,
      };
      await createRun(metadata);
      await appendSamples(id, parsed.samples);
      await refreshRuns();
      await selectRun(metadata);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "CSV 导入失败");
    }
  };

  const addDemo = async () => {
    const samples = createDemoSamples(task);
    const startedAtMs = Date.now();
    const id = crypto.randomUUID();
    const metadata: RunMetadata = {
      id,
      name: `${taskName(task)} 演示数据`,
      task,
      startedAtMs,
      endedAtMs: startedAtMs + samples.at(-1)!.elapsedMs,
      sampleCount: samples.length,
      baudRate: 9600,
      notes: "浏览器生成的界面演示记录",
      actualTaskId: samples[0].taskId,
    };
    await createRun(metadata);
    await appendSamples(id, samples);
    await refreshRuns();
    await selectRun(metadata);
  };

  const runFit = () => {
    const result = fitPlantModel(displaySamples, parameters);
    setFit(result);
    setMessage(result
      ? `模型辨识完成，使用 ${result.sampleCount} 个动态样本`
      : "有效动态样本不足，需至少记录 2 秒球运动数据");
  };

  const applyFit = () => {
    if (!fit) return;
    setParameters((current) => ({
      ...current,
      servoLatencyMs: fit.latencyMs,
      plantGain: fit.plantGain,
      viscousDampingS: fit.viscousDampingS,
      vehicleCoupling: fit.vehicleCoupling,
    }));
  };

  const serialSupported = supportsWebSerial();
  const portLabel = portInfo?.bluetoothServiceClassId
    ? "Bluetooth SPP"
    : portInfo?.usbVendorId
      ? `USB ${portInfo.usbVendorId.toString(16).padStart(4, "0")}`
      : "HC-05 / 串口";

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand-block">
          <div className="brand-mark"><Activity size={22} /></div>
          <div><h1>滚球调参台</h1><span>MSPM0 · MaixCAM · HC-05</span></div>
        </div>
        <div className={`connection-pill ${connection}`}>
          <span className="status-dot" />
          <div><strong>{connection === "connected" ? portLabel : "UART2"}</strong><small>{message}</small></div>
        </div>
      </header>

      <nav className="control-bar" aria-label="采集控制">
        <div className="task-switcher">
          <span>调参任务</span>
          <div className="segmented">
            {TASKS.map((item) => (
              <button
                className={task === item.id ? "active" : ""}
                disabled={recording}
                key={item.id}
                title={item.target}
                type="button"
                onClick={() => {
                  setTask(item.id);
                  setActiveRun(null);
                  setLiveSamples([]);
                  liveTaskIdRef.current = null;
                  liveStartRef.current = null;
                  liveIndexRef.current = 0;
                  setTaskMismatchFrames(0);
                }}
              >{item.label}</button>
            ))}
          </div>
        </div>
        <div className="serial-settings">
          <label>波特率<select value="9600" disabled><option>9600</option></select></label>
          {connection === "connected" ? (
            <button type="button" onClick={() => void disconnect()}><Unplug size={16} />断开</button>
          ) : (
            <button type="button" disabled={!serialSupported || connection === "connecting"} onClick={() => void connect()}><Cable size={16} />选择串口</button>
          )}
          {recording ? (
            <button className="danger" type="button" onClick={() => void stopRecording()}><CircleStop size={16} />停止并保存</button>
          ) : (
            <button className="primary" type="button" disabled={connection !== "connected"} onClick={() => void startRecording()}><Radio size={16} />开始记录</button>
          )}
        </div>
      </nav>

      <main>
        {!serialSupported && (
          <div className="notice"><Bluetooth size={17} />Web Serial 需要桌面版 Chrome 或 Edge，并通过 localhost 打开。</div>
        )}
        <MetricsStrip actual={actualMetrics} simulated={simulatedMetrics} />

        <div className="workspace-grid">
          <div className="analysis-column">
            <section className="analysis-section">
              <div className="section-heading chart-heading">
                <div><span className="eyebrow">实线实机 / 虚线仿真</span><h2>位置、目标与速度</h2></div>
                <div className="legend"><span className="actual">球位置</span><span className="target">目标</span><span className="simulated">仿真</span><span className="velocity">速度</span></div>
              </div>
              <TelemetryChart
                samples={displaySamples}
                simulation={simulation}
                cursorMs={activeSample?.elapsedMs}
              />
              <div className="replay-bar">
                <button
                  className="icon-button"
                  title={playing ? "暂停回放" : "开始回放"}
                  type="button"
                  disabled={displaySamples.length < 2}
                  onClick={() => setPlaying((value) => !value)}
                >{playing ? <Pause size={17} /> : <Play size={17} />}</button>
                <input
                  aria-label="回放位置"
                  type="range"
                  min={0}
                  max={Math.max(0, displaySamples.length - 1)}
                  value={activeIndex}
                  onChange={(event) => { setPlaying(false); setReplayIndex(Number(event.target.value)); }}
                />
                <span>{activeSample ? `${(activeSample.elapsedMs / 1000).toFixed(2)} s` : "0.00 s"}</span>
                <select aria-label="回放速度" value={replaySpeed} onChange={(event) => setReplaySpeed(Number(event.target.value))}>
                  <option value={0.5}>0.5x</option><option value={1}>1x</option><option value={2}>2x</option><option value={4}>4x</option>
                </select>
              </div>
            </section>

            <section className="beam-section">
              <div className="section-heading">
                <div><span className="eyebrow">机构响应</span><h2>水管、钢球与舵机</h2></div>
                <div className="sample-badges">
                  <span>任务 {activeSample?.taskId ?? "--"}</span>
                  <span>车速 {activeSample?.vehicleSpeedCmS.toFixed(1) ?? "--"} cm/s</span>
                   <span>轮速 {activeSample ? `${(activeSample.leftMotorSpeedCmS ?? activeSample.vehicleSpeedCmS).toFixed(1)} / ${(activeSample.rightMotorSpeedCmS ?? activeSample.vehicleSpeedCmS).toFixed(1)}` : "--"} cm/s</span>
                  <span>偏航 {activeSample?.yawDeg.toFixed(1) ?? "--"}°</span>
                   <span>前馈 {activeSample ? (activeSample.feedforwardServoDeg ?? 0).toFixed(1) : "--"}°</span>
                  <span>舵机速度 {activeSample?.servoSpeed.toFixed(0) ?? "--"}</span>
                </div>
              </div>
              <BeamVisualization
                sample={activeSample}
                simulatedPosition={activeSimulation?.simulatedPositionCm}
                simulatedServo={activeSimulation?.simulatedServoAngleDeg}
              />
            </section>

            <section className="quality-strip">
              <div><Database size={16} /><span>有效帧</span><strong>{stats.validFrames}</strong></div>
              <div><span>校验错误</span><strong className={stats.checksumErrors ? "bad" : ""}>{stats.checksumErrors}</strong></div>
              <div><span>格式错误</span><strong className={stats.formatErrors ? "bad" : ""}>{stats.formatErrors}</strong></div>
              <div><span>接收字节</span><strong>{stats.bytesReceived.toLocaleString()}</strong></div>
              <div><span>任务不匹配</span><strong className={taskMismatchFrames ? "bad" : ""}>{taskMismatchFrames}</strong></div>
              <div><span>实机 / 仿真评分</span><strong>{actualMetrics?.score.toFixed(0) ?? "--"} / {simulatedMetrics?.score.toFixed(0) ?? "--"}</strong></div>
            </section>
          </div>

          <ParameterPanel
            parameters={parameters}
            fit={fit}
            canFit={displaySamples.length >= 40}
            onChange={setParameters}
            onFit={runFit}
            onApplyFit={applyFit}
            onSave={() => { void saveProfile(task, parameters); setMessage(`${taskName(task)} 参数档已保存`); }}
            onReset={() => setParameters(DEFAULT_PARAMETERS[task])}
            onCopyC={() => { void navigator.clipboard.writeText(parametersToC(parameters)); setMessage("C 配置片段已复制"); }}
          />
        </div>

        <RunLibrary
          runs={runs}
          activeRunId={activeRun?.id ?? null}
          onSelect={(run) => void selectRun(run)}
          onDownload={(run) => void exportRun(run)}
          onDelete={(run) => void removeRun(run)}
          onImport={(file) => void importRun(file)}
          onDemo={() => void addDemo()}
        />
      </main>
    </div>
  );
}
