param(
    [string]$Distro = "Ubuntu",
    [string]$TpuMlirBin = ""
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Resolved = Resolve-Path -LiteralPath $Root
$Drive = $Resolved.Path.Substring(0, 1).ToLowerInvariant()
$Rest = $Resolved.Path.Substring(2).Replace("\", "/")
$WslDir = "/mnt/$Drive$Rest"

$Export = ""
if ($TpuMlirBin) {
    $Export = "export TPU_MLIR_BIN='$TpuMlirBin' && "
}

wsl.exe -d $Distro -- bash -lc "cd '$WslDir' && ${Export}bash convert.sh"
exit $LASTEXITCODE
