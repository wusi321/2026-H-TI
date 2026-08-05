import {
  Clipboard,
  RotateCcw,
  Save,
  ScanSearch,
  SlidersHorizontal,
} from "lucide-react";
import type { PlantFitResult, TuningParameters } from "../types";

type NumericParameter = Exclude<keyof TuningParameters, "servoPolarity">;

interface ParameterDefinition {
  key: NumericParameter;
  label: string;
  unit: string;
  minimum: number;
  maximum: number;
  step: number;
}

const GROUPS: { title: string; parameters: ParameterDefinition[] }[] = [
  {
    title: "串级控制",
    parameters: [
      { key: "positionKpS", label: "位置到速度增益", unit: "1/s", minimum: 0.1, maximum: 4, step: 0.05 },
      { key: "maxTargetVelocityCmS", label: "目标速度上限", unit: "cm/s", minimum: 1, maximum: 15, step: 0.25 },
      { key: "minimumTargetVelocityCmS", label: "最小移动速度", unit: "cm/s", minimum: 0, maximum: 8, step: 0.25 },
      { key: "predictionTimeS", label: "运动预测时间", unit: "s", minimum: 0, maximum: 0.5, step: 0.01 },
      { key: "brakingAccelerationCmS2", label: "规划减速度", unit: "cm/s2", minimum: 0.5, maximum: 30, step: 0.5 },
      { key: "velocityKpS", label: "速度环增益", unit: "1/s", minimum: 0.2, maximum: 12, step: 0.1 },
      { key: "accelerationLimitNearCmS2", label: "近目标加速度", unit: "cm/s2", minimum: 0.5, maximum: 30, step: 0.5 },
      { key: "accelerationLimitFarCmS2", label: "远目标加速度", unit: "cm/s2", minimum: 1, maximum: 50, step: 0.5 },
      { key: "accelerationLimitBrakeCmS2", label: "制动加速度", unit: "cm/s2", minimum: 1, maximum: 80, step: 1 },
    ],
  },
  {
    title: "舵机与前馈",
    parameters: [
      { key: "servoDegPerAccelerationCmS2", label: "加速度到舵机比例", unit: "deg/(cm/s2)", minimum: 0.05, maximum: 2, step: 0.025 },
      { key: "servoAngleLimitDeg", label: "舵机角度上限", unit: "deg", minimum: 4, maximum: 40, step: 0.5 },
      { key: "nearTargetMinAngleDeg", label: "近目标最小角限", unit: "deg", minimum: 0, maximum: 20, step: 0.5 },
      { key: "nearTargetFullErrorCm", label: "完整角度距离", unit: "cm", minimum: 0.5, maximum: 10, step: 0.25 },
      { key: "feedforwardGain", label: "车辆前馈增益", unit: "", minimum: 0, maximum: 2.5, step: 0.05 },
      { key: "feedforwardLimitDeg", label: "前馈角度上限", unit: "deg", minimum: 0, maximum: 35, step: 0.5 },
      { key: "servoAccelSlewDegS", label: "加速角速度", unit: "deg/s", minimum: 5, maximum: 400, step: 5 },
      { key: "servoBrakeSlewDegS", label: "制动角速度", unit: "deg/s", minimum: 10, maximum: 600, step: 5 },
      { key: "servoLevelSlewDegS", label: "回平角速度", unit: "deg/s", minimum: 20, maximum: 800, step: 10 },
    ],
  },
  {
    title: "实机模型",
    parameters: [
      { key: "servoLatencyMs", label: "舵机链路延迟", unit: "ms", minimum: 0, maximum: 350, step: 10 },
      { key: "servoTimeConstantMs", label: "舵机响应惯性", unit: "ms", minimum: 10, maximum: 300, step: 10 },
      { key: "plantGain", label: "机构有效增益", unit: "", minimum: 0.1, maximum: 5, step: 0.05 },
      { key: "viscousDampingS", label: "滚动阻尼", unit: "1/s", minimum: 0, maximum: 8, step: 0.1 },
      { key: "staticFrictionCmS2", label: "静摩擦阈值", unit: "cm/s2", minimum: 0, maximum: 35, step: 0.5 },
      { key: "vehicleCoupling", label: "车辆惯性耦合", unit: "", minimum: -2, maximum: 3, step: 0.05 },
      { key: "beamLengthMm", label: "水管有效长度", unit: "mm", minimum: 200, maximum: 300, step: 1 },
      { key: "gearRadiusMm", label: "齿轮半径", unit: "mm", minimum: 10, maximum: 25, step: 0.5 },
      { key: "rollingAccelerationRatio", label: "滚动加速度比例", unit: "", minimum: 0.2, maximum: 1, step: 0.01 },
    ],
  },
];

interface ParameterPanelProps {
  parameters: TuningParameters;
  fit: PlantFitResult | null;
  canFit: boolean;
  onChange: (parameters: TuningParameters) => void;
  onFit: () => void;
  onApplyFit: () => void;
  onSave: () => void;
  onReset: () => void;
  onCopyC: () => void;
}

export function ParameterPanel({
  parameters,
  fit,
  canFit,
  onChange,
  onFit,
  onApplyFit,
  onSave,
  onReset,
  onCopyC,
}: ParameterPanelProps) {
  const setValue = (key: NumericParameter, value: number) => {
    if (Number.isFinite(value)) onChange({ ...parameters, [key]: value });
  };
  return (
    <aside className="parameter-panel">
      <div className="panel-heading">
        <div>
          <span className="eyebrow">任务参数档</span>
          <h2>控制与实机模型</h2>
        </div>
        <SlidersHorizontal size={19} />
      </div>
      <div className="parameter-actions">
        <button type="button" onClick={onSave}><Save size={15} />保存</button>
        <button type="button" onClick={onReset}><RotateCcw size={15} />复位</button>
        <button type="button" onClick={onCopyC}><Clipboard size={15} />复制 C</button>
      </div>
      <div className="polarity-row">
        <span>舵机正方向</span>
        <div className="segmented compact">
          {([1, -1] as const).map((polarity) => (
            <button
              className={parameters.servoPolarity === polarity ? "active" : ""}
              key={polarity}
              onClick={() => onChange({ ...parameters, servoPolarity: polarity })}
              type="button"
            >
              {polarity === 1 ? "+" : "-"}
            </button>
          ))}
        </div>
      </div>
      {GROUPS.map((group, groupIndex) => (
        <details open={groupIndex < 2} key={group.title}>
          <summary>{group.title}</summary>
          <div className="parameter-list">
            {group.parameters.map((definition) => (
              <label className="parameter-row" key={definition.key}>
                <span>{definition.label}<small>{definition.unit}</small></span>
                <input
                  type="number"
                  min={definition.minimum}
                  max={definition.maximum}
                  step={definition.step}
                  value={parameters[definition.key]}
                  onChange={(event) => setValue(definition.key, Number(event.target.value))}
                />
                <input
                  aria-label={`${definition.label}滑块`}
                  type="range"
                  min={definition.minimum}
                  max={definition.maximum}
                  step={definition.step}
                  value={parameters[definition.key]}
                  onChange={(event) => setValue(definition.key, Number(event.target.value))}
                />
              </label>
            ))}
          </div>
        </details>
      ))}
      <section className="fit-panel">
        <div className="fit-panel__title">
          <ScanSearch size={17} />
          <strong>实机模型辨识</strong>
        </div>
        {fit ? (
          <dl>
            <div><dt>延迟</dt><dd>{fit.latencyMs} ms</dd></div>
            <div><dt>机构增益</dt><dd>{fit.plantGain.toFixed(3)}</dd></div>
            <div><dt>阻尼</dt><dd>{fit.viscousDampingS.toFixed(3)}</dd></div>
            <div><dt>车辆耦合</dt><dd>{fit.vehicleCoupling.toFixed(3)}</dd></div>
            <div><dt>残差</dt><dd>{fit.rmsResidualCmS2.toFixed(2)} cm/s2</dd></div>
          </dl>
        ) : <div className="fit-empty">至少需要 2 秒有效记录</div>}
        <div className="fit-buttons">
          <button type="button" disabled={!canFit} onClick={onFit}><ScanSearch size={15} />辨识</button>
          <button type="button" disabled={!fit} onClick={onApplyFit}>应用结果</button>
        </div>
      </section>
    </aside>
  );
}
