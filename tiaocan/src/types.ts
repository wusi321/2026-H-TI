export type TaskProfileId = "task3" | "task45" | "task6" | "task7";

export interface TelemetryValues {
  ballPositionCm: number;
  ballVelocityCmS: number;
  ballDirection: number;
  targetPositionCm: number;
  vehicleSpeedCmS: number;
  yawDeg: number;
  servoAngleDeg: number;
  servoSpeed: number;
  taskId: number;
  /** Present for protocol v2 frames; omitted by legacy samples. */
  protocolVersion?: number;
  vehicleCommandSpeedCmS?: number;
  vehicleMeasuredAccelerationCmS2?: number;
  vehicleCommandAccelerationCmS2?: number;
  leftMotorSpeedCmS?: number;
  rightMotorSpeedCmS?: number;
  yawRateDps?: number;
  feedforwardServoDeg?: number;
  turnCompensationServoDeg?: number;
  taskPhase?: number;
  routeMode?: number;
  statusFlags?: number;
  sequence?: number;
}

export interface TelemetrySample extends TelemetryValues {
  index: number;
  receivedAtMs: number;
  elapsedMs: number;
}

export interface ParserStats {
  bytesReceived: number;
  validFrames: number;
  checksumErrors: number;
  formatErrors: number;
  discardedBytes: number;
}

export interface RunMetadata {
  id: string;
  name: string;
  task: TaskProfileId;
  startedAtMs: number;
  endedAtMs?: number;
  sampleCount: number;
  baudRate: number;
  notes: string;
  actualTaskId?: number;
}

export interface StoredSample extends TelemetryValues {
  runId: string;
  index: number;
  receivedAtMs: number;
  elapsedMs: number;
}

export interface TuningParameters {
  positionKpS: number;
  maxTargetVelocityCmS: number;
  minimumTargetVelocityCmS: number;
  predictionTimeS: number;
  brakingAccelerationCmS2: number;
  velocityKpS: number;
  accelerationLimitNearCmS2: number;
  accelerationLimitFarCmS2: number;
  accelerationLimitBrakeCmS2: number;
  servoDegPerAccelerationCmS2: number;
  servoAngleLimitDeg: number;
  nearTargetMinAngleDeg: number;
  nearTargetFullErrorCm: number;
  feedforwardGain: number;
  feedforwardLimitDeg: number;
  servoAccelSlewDegS: number;
  servoBrakeSlewDegS: number;
  servoLevelSlewDegS: number;
  servoLatencyMs: number;
  servoTimeConstantMs: number;
  beamLengthMm: number;
  gearRadiusMm: number;
  rollingAccelerationRatio: number;
  plantGain: number;
  viscousDampingS: number;
  staticFrictionCmS2: number;
  vehicleCoupling: number;
  servoPolarity: 1 | -1;
}

export interface SimulationSample {
  elapsedMs: number;
  targetPositionCm: number;
  actualPositionCm: number;
  simulatedPositionCm: number;
  simulatedVelocityCmS: number;
  simulatedServoAngleDeg: number;
  vehicleSpeedCmS: number;
}

export interface PerformanceMetrics {
  durationS: number;
  meanAbsoluteErrorCm: number;
  rmsErrorCm: number;
  maxAbsoluteErrorCm: number;
  peakVelocityCmS: number;
  peakServoAngleDeg: number;
  inTolerancePercent: number;
  settlingTimeS: number | null;
  targetCrossings: number;
  score: number;
}

export interface PlantFitResult {
  latencyMs: number;
  plantGain: number;
  viscousDampingS: number;
  vehicleCoupling: number;
  biasCmS2: number;
  rmsResidualCmS2: number;
  sampleCount: number;
}
