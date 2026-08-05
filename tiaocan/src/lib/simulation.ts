import type {
  PerformanceMetrics,
  PlantFitResult,
  SimulationSample,
  TaskProfileId,
  TelemetrySample,
  TuningParameters,
} from "../types";

const GRAVITY_CM_S2 = 980;
const TOLERANCE_CM = 1;

const BASE_PARAMETERS: TuningParameters = {
  positionKpS: 1.2,
  maxTargetVelocityCmS: 8,
  minimumTargetVelocityCmS: 3,
  predictionTimeS: 0.18,
  brakingAccelerationCmS2: 4,
  velocityKpS: 4,
  accelerationLimitNearCmS2: 5,
  accelerationLimitFarCmS2: 10,
  accelerationLimitBrakeCmS2: 25,
  servoDegPerAccelerationCmS2: 0.6,
  servoAngleLimitDeg: 28,
  nearTargetMinAngleDeg: 4,
  nearTargetFullErrorCm: 5,
  feedforwardGain: 0.85,
  feedforwardLimitDeg: 28,
  servoAccelSlewDegS: 30,
  servoBrakeSlewDegS: 180,
  servoLevelSlewDegS: 500,
  servoLatencyMs: 120,
  servoTimeConstantMs: 80,
  beamLengthMm: 250,
  gearRadiusMm: 17,
  rollingAccelerationRatio: 5 / 7,
  plantGain: 1,
  viscousDampingS: 0.8,
  staticFrictionCmS2: 16,
  vehicleCoupling: 1,
  servoPolarity: 1,
};

export const DEFAULT_PARAMETERS: Record<TaskProfileId, TuningParameters> = {
  task3: {
    ...BASE_PARAMETERS,
    positionKpS: 1.56,
    feedforwardGain: 0,
  },
  task45: { ...BASE_PARAMETERS },
  task6: {
    ...BASE_PARAMETERS,
    positionKpS: 1.25,
    maxTargetVelocityCmS: 7,
  },
  task7: {
    ...BASE_PARAMETERS,
    positionKpS: 1.25,
    maxTargetVelocityCmS: 7,
  },
};

function clamp(value: number, minimum: number, maximum: number): number {
  return Math.min(maximum, Math.max(minimum, value));
}

function moveToward(value: number, target: number, maximumDelta: number): number {
  return value + clamp(target - value, -maximumDelta, maximumDelta);
}

function signOr(value: number, fallback: number): number {
  if (Math.abs(value) > 1e-6) return Math.sign(value);
  return Math.sign(fallback || 1);
}

function linkageAcceleration(angleDeg: number, parameters: TuningParameters): number {
  const servoRad = angleDeg * Math.PI / 180;
  const rackTravelMm = parameters.gearRadiusMm * Math.sin(servoRad);
  const beamAngleRad = Math.atan2(rackTravelMm, parameters.beamLengthMm);
  return parameters.servoPolarity
    * parameters.plantGain
    * parameters.rollingAccelerationRatio
    * GRAVITY_CM_S2
    * Math.sin(beamAngleRad);
}

function interpolateInput(
  samples: TelemetrySample[],
  elapsedMs: number,
  cursor: number,
): { cursor: number; target: number; vehicleSpeed: number; actual: number } {
  while (cursor + 1 < samples.length && samples[cursor + 1].elapsedMs <= elapsedMs) {
    cursor += 1;
  }
  const current = samples[cursor];
  const next = samples[Math.min(cursor + 1, samples.length - 1)];
  const span = Math.max(1, next.elapsedMs - current.elapsedMs);
  const mix = clamp((elapsedMs - current.elapsedMs) / span, 0, 1);
  return {
    cursor,
    target: current.targetPositionCm
      + (next.targetPositionCm - current.targetPositionCm) * mix,
    vehicleSpeed: current.vehicleSpeedCmS
      + (next.vehicleSpeedCmS - current.vehicleSpeedCmS) * mix,
    actual: current.ballPositionCm
      + (next.ballPositionCm - current.ballPositionCm) * mix,
  };
}

export function simulateRun(
  samples: TelemetrySample[],
  parameters: TuningParameters,
): SimulationSample[] {
  if (samples.length < 2) return [];
  const integrationStepS = 0.01;
  const durationMs = samples.at(-1)!.elapsedMs;
  const latencySteps = Math.max(0, Math.round(parameters.servoLatencyMs / 10));
  const commandQueue = Array.from({ length: latencySteps + 1 }, () => 0);
  const output: SimulationSample[] = [];

  let position = samples[0].ballPositionCm;
  let velocity = samples[0].ballVelocityCmS;
  let servoCommand = samples[0].servoAngleDeg;
  let servoActual = servoCommand;
  let vehicleSpeed = samples[0].vehicleSpeedCmS;
  let cursor = 0;
  let outputCursor = 0;

  for (let elapsedMs = 0; elapsedMs <= durationMs; elapsedMs += 10) {
    const input = interpolateInput(samples, elapsedMs, cursor);
    cursor = input.cursor;
    const vehicleAcceleration = (input.vehicleSpeed - vehicleSpeed) / integrationStepS;
    vehicleSpeed = input.vehicleSpeed;

    const predictedPosition = position + velocity * parameters.predictionTimeS;
    const predictedError = input.target - predictedPosition;
    const positionError = input.target - position;
    const brakingVelocity = Math.sqrt(
      Math.max(0, 2 * parameters.brakingAccelerationCmS2 * Math.abs(positionError)),
    );
    const proportionalVelocity = Math.abs(parameters.positionKpS * predictedError);
    let targetVelocityMagnitude = Math.min(
      parameters.maxTargetVelocityCmS,
      proportionalVelocity,
      brakingVelocity,
    );
    if (Math.abs(positionError) > 0.5) {
      const minimumScale = clamp(Math.abs(positionError) / 1, 0, 1);
      targetVelocityMagnitude = Math.max(
        targetVelocityMagnitude,
        parameters.minimumTargetVelocityCmS * minimumScale,
      );
      targetVelocityMagnitude = Math.min(targetVelocityMagnitude, brakingVelocity);
    }
    const targetVelocity = Math.sign(predictedError) * targetVelocityMagnitude;
    const velocityError = targetVelocity - velocity;
    const braking = velocity * positionError < 0
      || Math.abs(velocity) > Math.max(brakingVelocity, 0.5);
    const distanceMix = clamp(
      Math.abs(positionError) / Math.max(0.1, parameters.nearTargetFullErrorCm),
      0,
      1,
    );
    const normalAccelerationLimit = parameters.accelerationLimitNearCmS2
      + (parameters.accelerationLimitFarCmS2
        - parameters.accelerationLimitNearCmS2) * distanceMix;
    const accelerationLimit = braking
      ? parameters.accelerationLimitBrakeCmS2
      : normalAccelerationLimit;
    const accelerationCommand = clamp(
      parameters.velocityKpS * velocityError,
      -accelerationLimit,
      accelerationLimit,
    );

    const feedbackServo = accelerationCommand
      * parameters.servoDegPerAccelerationCmS2;
    const feedforwardServo = clamp(
      parameters.feedforwardGain
        * vehicleAcceleration
        * parameters.servoDegPerAccelerationCmS2,
      -parameters.feedforwardLimitDeg,
      parameters.feedforwardLimitDeg,
    );
    const velocityAngleScale = clamp(
      Math.abs(velocity) / Math.max(1, parameters.maxTargetVelocityCmS),
      0,
      1,
    );
    const nearAngleLimit = parameters.nearTargetMinAngleDeg
      + (parameters.servoAngleLimitDeg - parameters.nearTargetMinAngleDeg)
      * Math.max(distanceMix, velocityAngleScale);
    const desiredServo = clamp(
      feedbackServo + feedforwardServo,
      -nearAngleLimit,
      nearAngleLimit,
    );
    const opposingMotion = desiredServo * velocity < 0;
    const reducingAngle = Math.abs(desiredServo) < Math.abs(servoCommand);
    const slew = opposingMotion
      ? parameters.servoBrakeSlewDegS
      : reducingAngle
        ? parameters.servoLevelSlewDegS
        : parameters.servoAccelSlewDegS;
    servoCommand = moveToward(
      servoCommand,
      desiredServo,
      slew * integrationStepS,
    );
    commandQueue.push(servoCommand);
    const delayedCommand = commandQueue.shift() ?? servoCommand;
    const servoAlpha = clamp(
      integrationStepS / Math.max(0.01, parameters.servoTimeConstantMs / 1000),
      0,
      1,
    );
    servoActual += (delayedCommand - servoActual) * servoAlpha;

    const tiltAcceleration = linkageAcceleration(servoActual, parameters);
    const externalDrive = tiltAcceleration
      - parameters.vehicleCoupling * vehicleAcceleration
      - parameters.viscousDampingS * velocity;
    let ballAcceleration = externalDrive;
    if (Math.abs(velocity) < 0.04 && Math.abs(externalDrive) < parameters.staticFrictionCmS2) {
      ballAcceleration = 0;
      velocity = 0;
    } else {
      ballAcceleration -= 0.75 * parameters.staticFrictionCmS2
        * signOr(velocity, externalDrive);
    }

    velocity += ballAcceleration * integrationStepS;
    position += velocity * integrationStepS;
    if (position > 12.5 || position < -12.5) {
      position = clamp(position, -12.5, 12.5);
      velocity *= -0.15;
    }

    while (
      outputCursor < samples.length
      && samples[outputCursor].elapsedMs <= elapsedMs
    ) {
      const actualSample = samples[outputCursor];
      output.push({
        elapsedMs: actualSample.elapsedMs,
        targetPositionCm: actualSample.targetPositionCm,
        actualPositionCm: actualSample.ballPositionCm,
        simulatedPositionCm: position,
        simulatedVelocityCmS: velocity,
        simulatedServoAngleDeg: servoActual,
        vehicleSpeedCmS: actualSample.vehicleSpeedCmS,
      });
      outputCursor += 1;
    }
  }
  return output;
}

function calculateMetricsFromSeries(
  elapsedMs: number[],
  position: number[],
  velocity: number[],
  target: number[],
  servo: number[],
): PerformanceMetrics {
  if (position.length === 0) {
    return {
      durationS: 0,
      meanAbsoluteErrorCm: 0,
      rmsErrorCm: 0,
      maxAbsoluteErrorCm: 0,
      peakVelocityCmS: 0,
      peakServoAngleDeg: 0,
      inTolerancePercent: 0,
      settlingTimeS: null,
      targetCrossings: 0,
      score: 0,
    };
  }
  const errors = position.map((value, index) => target[index] - value);
  const absoluteErrors = errors.map(Math.abs);
  const meanAbsoluteErrorCm = absoluteErrors.reduce((a, b) => a + b, 0) / errors.length;
  const rmsErrorCm = Math.sqrt(errors.reduce((sum, value) => sum + value * value, 0) / errors.length);
  const maxAbsoluteErrorCm = Math.max(...absoluteErrors);
  const peakVelocityCmS = Math.max(...velocity.map(Math.abs));
  const peakServoAngleDeg = Math.max(...servo.map(Math.abs));
  const inTolerancePercent = 100
    * absoluteErrors.filter((value) => value <= TOLERANCE_CM).length
    / errors.length;
  let targetCrossings = 0;
  for (let index = 1; index < errors.length; index += 1) {
    if (errors[index - 1] * errors[index] < 0) targetCrossings += 1;
  }

  let lastTargetChange = 0;
  for (let index = 1; index < target.length; index += 1) {
    if (Math.abs(target[index] - target[index - 1]) > 0.2) lastTargetChange = index;
  }
  const stableWindowMs = 500;
  let settlingTimeS: number | null = null;
  for (let index = lastTargetChange; index < errors.length; index += 1) {
    const endTime = elapsedMs[index] + stableWindowMs;
    let end = index;
    while (end < errors.length && elapsedMs[end] <= endTime) end += 1;
    if (end === errors.length && elapsedMs.at(-1)! < endTime) break;
    const stable = errors.slice(index, end).every((value) => Math.abs(value) <= TOLERANCE_CM)
      && velocity.slice(index, end).every((value) => Math.abs(value) <= 1);
    if (stable) {
      settlingTimeS = (elapsedMs[index] - elapsedMs[lastTargetChange]) / 1000;
      break;
    }
  }
  const score = clamp(
    inTolerancePercent
      - rmsErrorCm * 10
      - Math.max(0, peakVelocityCmS - 8) * 2
      - targetCrossings * 0.5,
    0,
    100,
  );
  return {
    durationS: elapsedMs.at(-1)! / 1000,
    meanAbsoluteErrorCm,
    rmsErrorCm,
    maxAbsoluteErrorCm,
    peakVelocityCmS,
    peakServoAngleDeg,
    inTolerancePercent,
    settlingTimeS,
    targetCrossings,
    score,
  };
}

export function metricsForActual(samples: TelemetrySample[]): PerformanceMetrics {
  return calculateMetricsFromSeries(
    samples.map((sample) => sample.elapsedMs),
    samples.map((sample) => sample.ballPositionCm),
    samples.map((sample) => sample.ballVelocityCmS),
    samples.map((sample) => sample.targetPositionCm),
    samples.map((sample) => sample.servoAngleDeg),
  );
}

export function metricsForSimulation(samples: SimulationSample[]): PerformanceMetrics {
  return calculateMetricsFromSeries(
    samples.map((sample) => sample.elapsedMs),
    samples.map((sample) => sample.simulatedPositionCm),
    samples.map((sample) => sample.simulatedVelocityCmS),
    samples.map((sample) => sample.targetPositionCm),
    samples.map((sample) => sample.simulatedServoAngleDeg),
  );
}

function solveLinearSystem(matrix: number[][], vector: number[]): number[] | null {
  const size = vector.length;
  const augmented = matrix.map((row, index) => [...row, vector[index]]);
  for (let pivot = 0; pivot < size; pivot += 1) {
    let best = pivot;
    for (let row = pivot + 1; row < size; row += 1) {
      if (Math.abs(augmented[row][pivot]) > Math.abs(augmented[best][pivot])) best = row;
    }
    if (Math.abs(augmented[best][pivot]) < 1e-9) return null;
    [augmented[pivot], augmented[best]] = [augmented[best], augmented[pivot]];
    const divisor = augmented[pivot][pivot];
    for (let column = pivot; column <= size; column += 1) {
      augmented[pivot][column] /= divisor;
    }
    for (let row = 0; row < size; row += 1) {
      if (row === pivot) continue;
      const factor = augmented[row][pivot];
      for (let column = pivot; column <= size; column += 1) {
        augmented[row][column] -= factor * augmented[pivot][column];
      }
    }
  }
  return augmented.map((row) => row[size]);
}

export function fitPlantModel(
  samples: TelemetrySample[],
  parameters: TuningParameters,
): PlantFitResult | null {
  if (samples.length < 40) return null;
  let best: PlantFitResult | null = null;
  for (let latencyMs = 0; latencyMs <= 300; latencyMs += 20) {
    const rows: { features: number[]; observed: number }[] = [];
    let delayedIndex = 0;
    for (let index = 2; index < samples.length - 2; index += 1) {
      const previous = samples[index - 1];
      const current = samples[index];
      const next = samples[index + 1];
      const dt = (next.elapsedMs - previous.elapsedMs) / 1000;
      if (dt <= 0.02 || dt > 0.2 || current.ballDirection === 2) continue;
      const observed = (next.ballVelocityCmS - previous.ballVelocityCmS) / dt;
      if (!Number.isFinite(observed) || Math.abs(observed) > 250) continue;
      const delayedTime = current.elapsedMs - latencyMs;
      while (
        delayedIndex + 1 < index
        && samples[delayedIndex + 1].elapsedMs <= delayedTime
      ) delayedIndex += 1;
      const delayedServo = samples[delayedIndex].servoAngleDeg;
      const tilt = linkageAcceleration(delayedServo, {
        ...parameters,
        plantGain: 1,
      });
      const vehicleDt = Math.max(0.02, (next.elapsedMs - previous.elapsedMs) / 1000);
      const vehicleAcceleration =
        (next.vehicleSpeedCmS - previous.vehicleSpeedCmS) / vehicleDt;
      rows.push({
        features: [tilt, -current.ballVelocityCmS, -vehicleAcceleration, 1],
        observed,
      });
    }
    if (rows.length < 30) continue;
    const normal = Array.from({ length: 4 }, () => Array(4).fill(0) as number[]);
    const target = Array(4).fill(0) as number[];
    rows.forEach(({ features, observed }) => {
      for (let row = 0; row < 4; row += 1) {
        target[row] += features[row] * observed;
        for (let column = 0; column < 4; column += 1) {
          normal[row][column] += features[row] * features[column];
        }
      }
    });
    const solved = solveLinearSystem(normal, target);
    if (!solved) continue;
    const [plantGain, damping, vehicleCoupling, bias] = solved;
    if (
      plantGain < 0.05 || plantGain > 6
      || damping < 0 || damping > 12
      || vehicleCoupling < -4 || vehicleCoupling > 4
    ) continue;
    const residual = Math.sqrt(
      rows.reduce((sum, row) => {
        const predicted = row.features.reduce(
          (value, feature, index) => value + feature * solved[index],
          0,
        );
        return sum + (row.observed - predicted) ** 2;
      }, 0) / rows.length,
    );
    if (!best || residual < best.rmsResidualCmS2) {
      best = {
        latencyMs,
        plantGain,
        viscousDampingS: damping,
        vehicleCoupling,
        biasCmS2: bias,
        rmsResidualCmS2: residual,
        sampleCount: rows.length,
      };
    }
  }
  return best;
}

export function parametersToC(parameters: TuningParameters): string {
  return [
    `.position_to_velocity_kp_s = ${parameters.positionKpS.toFixed(3)}f,`,
    `.max_target_velocity_mm_s = ${(parameters.maxTargetVelocityCmS * 10).toFixed(1)}f,`,
    `.minimum_target_velocity_mm_s = ${(parameters.minimumTargetVelocityCmS * 10).toFixed(1)}f,`,
    `.motion_prediction_time_s = ${parameters.predictionTimeS.toFixed(3)}f,`,
    `.braking_acceleration_mm_s2 = ${(parameters.brakingAccelerationCmS2 * 10).toFixed(1)}f,`,
    `.velocity_to_acceleration_kp_s = ${parameters.velocityKpS.toFixed(3)}f,`,
    `.acceleration_limit_near_mm_s2 = ${(parameters.accelerationLimitNearCmS2 * 10).toFixed(1)}f,`,
    `.acceleration_limit_far_mm_s2 = ${(parameters.accelerationLimitFarCmS2 * 10).toFixed(1)}f,`,
    `.acceleration_limit_brake_mm_s2 = ${(parameters.accelerationLimitBrakeCmS2 * 10).toFixed(1)}f,`,
    `.servo_degrees_per_acceleration_mm_s2 = ${(parameters.servoDegPerAccelerationCmS2 / 10).toFixed(4)}f,`,
    `.servo_normal_angle_limit_deg = ${parameters.servoAngleLimitDeg.toFixed(2)}f,`,
    `.servo_near_target_min_limit_deg = ${parameters.nearTargetMinAngleDeg.toFixed(2)}f,`,
    `.vehicle_feedforward_gain = ${parameters.feedforwardGain.toFixed(3)}f,`,
    `.vehicle_feedforward_servo_limit_deg = ${parameters.feedforwardLimitDeg.toFixed(2)}f,`,
    `.servo_accel_slew_deg_s = ${parameters.servoAccelSlewDegS.toFixed(1)}f,`,
    `.servo_brake_slew_deg_s = ${parameters.servoBrakeSlewDegS.toFixed(1)}f,`,
    `.servo_level_slew_deg_s = ${parameters.servoLevelSlewDegS.toFixed(1)}f,`,
  ].join("\n");
}
