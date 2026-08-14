$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Python = Join-Path $Root ".export-venv\Scripts\python.exe"

if (-not (Test-Path -LiteralPath $Python)) {
    throw "Export environment not found. Run .\install_export_env.ps1 first."
}

& $Python (Join-Path $Root "export_onnx.py") @args
exit $LASTEXITCODE
