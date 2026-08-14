$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Python = Join-Path $Root ".venv\Scripts\python.exe"

if (-not (Test-Path -LiteralPath $Python)) {
    throw "Training environment not found. Run .\install_env.ps1 first."
}

& $Python (Join-Path $Root "train_pose_v2.py") @args
exit $LASTEXITCODE
