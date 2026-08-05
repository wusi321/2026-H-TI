import { Download, FileInput, FlaskConical, Trash2 } from "lucide-react";
import { useRef } from "react";
import type { RunMetadata } from "../types";

interface RunLibraryProps {
  runs: RunMetadata[];
  activeRunId: string | null;
  onSelect: (run: RunMetadata) => void;
  onDownload: (run: RunMetadata) => void;
  onDelete: (run: RunMetadata) => void;
  onImport: (file: File) => void;
  onDemo: () => void;
}

const TASK_NAMES = {
  task3: "任务 3",
  task45: "任务 4/5",
  task6: "任务 6",
  task7: "任务 7",
};

function runTaskName(run: RunMetadata): string {
  return run.actualTaskId ? `任务 ${run.actualTaskId}` : TASK_NAMES[run.task];
}

export function RunLibrary({
  runs,
  activeRunId,
  onSelect,
  onDownload,
  onDelete,
  onImport,
  onDemo,
}: RunLibraryProps) {
  const fileRef = useRef<HTMLInputElement>(null);
  return (
    <section className="run-library">
      <div className="section-heading">
        <div>
          <span className="eyebrow">IndexedDB</span>
          <h2>实机运行记录</h2>
        </div>
        <div className="section-actions">
          <button type="button" onClick={onDemo}><FlaskConical size={15} />演示数据</button>
          <button type="button" onClick={() => fileRef.current?.click()}><FileInput size={15} />导入 CSV</button>
          <input
            ref={fileRef}
            hidden
            type="file"
            accept=".csv,text/csv"
            onChange={(event) => {
              const file = event.target.files?.[0];
              if (file) onImport(file);
              event.target.value = "";
            }}
          />
        </div>
      </div>
      <div className="table-wrap">
        <table>
          <thead>
            <tr><th>名称</th><th>任务</th><th>开始时间</th><th>样本</th><th>时长</th><th aria-label="操作" /></tr>
          </thead>
          <tbody>
            {runs.length === 0 ? (
              <tr><td colSpan={6} className="empty-cell">暂无记录</td></tr>
            ) : runs.map((run) => (
              <tr
                className={activeRunId === run.id ? "active" : ""}
                key={run.id}
                onClick={() => onSelect(run)}
              >
                <td>{run.name}</td>
                <td>{runTaskName(run)}</td>
                <td>{new Date(run.startedAtMs).toLocaleString()}</td>
                <td>{run.sampleCount}</td>
                <td>{run.endedAtMs ? `${((run.endedAtMs - run.startedAtMs) / 1000).toFixed(1)} s` : "记录中"}</td>
                <td className="row-actions">
                  <button title="导出 CSV" type="button" onClick={(event) => { event.stopPropagation(); onDownload(run); }}><Download size={15} /></button>
                  <button title="删除记录" type="button" onClick={(event) => { event.stopPropagation(); onDelete(run); }}><Trash2 size={15} /></button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}
