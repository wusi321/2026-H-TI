# Web 调参台工程壳与依赖配置

本目录保存调参 Web 控制台的 Vite/React 工程配置、依赖锁定文件和已有构建产物。当前业务源码不在本目录，而在同级 `../tiaocan/src`。

## 当前目录状态

| 内容 | 状态 |
| --- | --- |
| `package.json` | 存在，定义 Vite、TypeScript、Vitest、Playwright 脚本。 |
| `package-lock.json` | 存在，锁定 npm 依赖版本。 |
| `vite.config.ts` / `vitest.config.ts` | 存在。 |
| `tsconfig*.json` | 存在。 |
| `dist/` | 存在，为历史构建产物，已在根仓库 `.gitignore` 中默认忽略。 |
| `src/` | 当前不存在，源码主体位于 `../tiaocan/src`。 |
| `node_modules/` | 本地依赖缓存，已在根仓库 `.gitignore` 中默认忽略。 |

因此，本目录不能在当前状态下独立完成源码构建，除非补回 `src`。

## 与 `tiaocan` 的关系

`tiaocan` 是调参台源码目录，包含：

- UART2 遥测协议解析；
- Web Serial 串口读取；
- 实验记录 IndexedDB 存储；
- CSV 导入/导出；
- 实测指标计算；
- 简化物理模型仿真；
- `BallBalanceConfig` C 参数片段导出；
- React UI 组件和测试。

`web` 提供 npm 工程元数据。若要恢复可运行状态，可采用以下任一方式：

1. 在 `web` 下创建/复制 `src`，内容来自 `../tiaocan/src`；
2. 或把本目录的 `package.json`、锁文件和配置复制到 `../tiaocan`，以 `tiaocan` 作为工程根目录。

## 依赖与脚本

`package.json` 中定义：

```json
{
  "dev": "vite",
  "build": "tsc -b && vite build",
  "test": "vitest run",
  "test:e2e": "playwright test",
  "preview": "vite preview"
}
```

主要依赖：

- React 18；
- Vite 6；
- TypeScript 5；
- Vitest；
- Playwright；
- lucide-react 图标库。

## 恢复运行示例

如果选择把 `tiaocan/src` 复制到 `web/src`：

```powershell
cd web
npm install
npm run dev -- --host 127.0.0.1 --port 4317
```

浏览器使用 Chrome 或 Edge 打开：

```text
http://127.0.0.1:4317
```

Web Serial 需要安全上下文和用户手动选择串口；Firefox 不支持。

## 注意

- 不应提交 `node_modules/`、`dist/`、`*.tsbuildinfo` 和 Vite 日志。
- 调参台协议和运行说明以 `../tiaocan/README.md` 为准。
- 若后续整理项目，建议合并 `web` 与 `tiaocan`，保留一个完整的前端工程根目录。
