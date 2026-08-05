import type { RunMetadata, TaskProfileId, TelemetrySample } from "../types";

const BASE_HEADERS = [
  "index",
  "received_at_ms",
  "elapsed_ms",
  "ball_position_cm",
  "ball_velocity_cm_s",
  "ball_direction",
  "target_position_cm",
  "vehicle_speed_cm_s",
  "yaw_deg",
  "servo_angle_deg",
  "servo_speed",
  "task_id",
] as const;

const HEADERS = [
  ...BASE_HEADERS,
  "protocol_version",
  "task_phase",
  "route_mode",
  "status_flags",
  "sequence",
  "vehicle_command_speed_cm_s",
  "vehicle_measured_acceleration_cm_s2",
  "vehicle_command_acceleration_cm_s2",
  "left_motor_speed_cm_s",
  "right_motor_speed_cm_s",
  "yaw_rate_dps",
  "feedforward_servo_deg",
  "turn_compensation_servo_deg",
] as const;

const LEGACY_HEADERS = BASE_HEADERS.slice(0, -1);

function defaultTaskId(task: TaskProfileId): number {
  if (task === "task3") return 3;
  if (task === "task6") return 6;
  if (task === "task7") return 7;
  return 0;
}

export function runToCsv(run: RunMetadata, samples: TelemetrySample[]): string {
  const metadata = [
    `# run_id=${run.id}`,
    `# name=${run.name}`,
    `# task=${run.task}`,
    `# actual_task_id=${run.actualTaskId ?? samples[0]?.taskId ?? 0}`,
    `# started_at_ms=${run.startedAtMs}`,
    `# baud_rate=${run.baudRate}`,
  ];
  const extended = samples.some((sample) => sample.protocolVersion === 2);
  const headers = extended ? HEADERS : BASE_HEADERS;
  const rows = samples.map((sample) => {
    const base = [
      sample.index,
      sample.receivedAtMs,
      sample.elapsedMs,
      sample.ballPositionCm,
      sample.ballVelocityCmS,
      sample.ballDirection,
      sample.targetPositionCm,
      sample.vehicleSpeedCmS,
      sample.yawDeg,
      sample.servoAngleDeg,
      sample.servoSpeed,
      sample.taskId,
    ];
    if (!extended) return base.join(",");
    return [
      ...base,
      sample.protocolVersion ?? 2,
      sample.taskPhase ?? 0,
      sample.routeMode ?? 0,
      sample.statusFlags ?? 0,
      sample.sequence ?? 0,
      sample.vehicleCommandSpeedCmS ?? sample.vehicleSpeedCmS,
      sample.vehicleMeasuredAccelerationCmS2 ?? 0,
      sample.vehicleCommandAccelerationCmS2 ?? 0,
      sample.leftMotorSpeedCmS ?? sample.vehicleSpeedCmS,
      sample.rightMotorSpeedCmS ?? sample.vehicleSpeedCmS,
      sample.yawRateDps ?? 0,
      sample.feedforwardServoDeg ?? 0,
      sample.turnCompensationServoDeg ?? 0,
    ].join(",");
  });
  return [...metadata, headers.join(","), ...rows].join("\n");
}

export function downloadCsv(run: RunMetadata, samples: TelemetrySample[]): void {
  const blob = new Blob([runToCsv(run, samples)], { type: "text/csv;charset=utf-8" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = `${run.task}-${new Date(run.startedAtMs).toISOString().replace(/[:.]/g, "-")}.csv`;
  link.click();
  URL.revokeObjectURL(link.href);
}

export function parseCsv(text: string): {
  task: TaskProfileId;
  name: string;
  samples: TelemetrySample[];
} {
  const lines = text.split(/\r?\n/).filter(Boolean);
  const metadata = new Map<string, string>();
  while (lines[0]?.startsWith("# ")) {
    const [key, ...value] = lines.shift()!.slice(2).split("=");
    metadata.set(key, value.join("="));
  }
  const rawTask = metadata.get("task");
  const task: TaskProfileId = rawTask === "task3" || rawTask === "task6" || rawTask === "task7"
    ? rawTask
    : "task45";
  const header = lines.shift()?.split(",");
  const extendedFormat = header?.join(",") === HEADERS.join(",");
  const currentFormat = header?.join(",") === BASE_HEADERS.join(",");
  const legacyFormat = header?.join(",") === LEGACY_HEADERS.join(",");
  if (!header || (!extendedFormat && !currentFormat && !legacyFormat)) {
    throw new Error("CSV 字段与 UART2 调参格式不匹配");
  }
  const samples = lines.map((line) => {
    const values = line.split(",").map(Number);
    if (values.length !== header.length || values.some((value) => !Number.isFinite(value))) {
      throw new Error("CSV 包含无效样本");
    }
    const base = {
      index: values[0],
      receivedAtMs: values[1],
      elapsedMs: values[2],
      ballPositionCm: values[3],
      ballVelocityCmS: values[4],
      ballDirection: values[5],
      targetPositionCm: values[6],
      vehicleSpeedCmS: values[7],
      yawDeg: values[8],
      servoAngleDeg: values[9],
      servoSpeed: values[10],
      taskId: (extendedFormat || currentFormat) ? values[11] : defaultTaskId(task),
    } satisfies TelemetrySample;
    if (!extendedFormat) return base;
    return {
      ...base,
      protocolVersion: values[12],
      taskPhase: values[13],
      routeMode: values[14],
      statusFlags: values[15],
      sequence: values[16],
      vehicleCommandSpeedCmS: values[17],
      vehicleMeasuredAccelerationCmS2: values[18],
      vehicleCommandAccelerationCmS2: values[19],
      leftMotorSpeedCmS: values[20],
      rightMotorSpeedCmS: values[21],
      yawRateDps: values[22],
      feedforwardServoDeg: values[23],
      turnCompensationServoDeg: values[24],
    } satisfies TelemetrySample;
  });
  return { task, name: metadata.get("name") ?? "导入记录", samples };
}
