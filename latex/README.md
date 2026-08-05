# 设计报告 LaTeX 文档

本目录保存 H 题“车载平衡滚球运动控制系统”的设计报告 LaTeX 源文件和已生成 PDF。

## 文件说明

| 文件 | 说明 |
| --- | --- |
| `设计报告.tex` | 报告主源文件。 |
| `设计报告.pdf` | 已编译生成的设计报告。 |
| `设计报告.aux`、`设计报告.out`、`设计报告.log`、`设计报告.fls`、`设计报告.fdb_latexmk`、`设计报告.synctex.gz` | LaTeX 编译中间文件，已在根仓库 `.gitignore` 中默认忽略。 |

## 编译建议

建议使用 TeX Live 或 MiKTeX，并使用支持中文的 XeLaTeX 编译：

```powershell
cd latex
xelatex 设计报告.tex
xelatex 设计报告.tex
```

如果使用 `latexmk`：

```powershell
cd latex
latexmk -xelatex 设计报告.tex
```

## 与代码工程的关系

报告内容对应根目录下的完整参赛代码：

- `Maixcam/`：视觉测量、标定、UART 输出和图传；
- `mspm0g3507_26ti/`：MSPM0G3507 主控、车体和钢珠闭环；
- `tuchuan/`：图传专用版本；
- `tiaocan/` 与 `web/`：调参 Web 控制台源码和工程配置。

修改控制策略、参数或硬件连接后，应同步更新报告中的系统框图、算法描述、参数表和测试记录。
