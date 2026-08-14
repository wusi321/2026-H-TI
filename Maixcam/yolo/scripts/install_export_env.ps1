param(
    [string]$BasePython = "python"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$VenvPython = Join-Path $Root ".export-venv\Scripts\python.exe"

if (-not (Test-Path -LiteralPath $VenvPython)) {
    & $BasePython -m venv (Join-Path $Root ".export-venv")
}

& $VenvPython -m pip install --upgrade pip setuptools wheel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $VenvPython -m pip install `
    torch==2.5.1 torchvision==0.20.1 `
    --index-url https://download.pytorch.org/whl/cpu
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $VenvPython -m pip install -r (Join-Path $Root "requirements-export.txt")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $VenvPython -c "import onnx, torch, ultralytics; print('export environment:', onnx.__version__, torch.__version__, ultralytics.__version__)"
exit $LASTEXITCODE
