param(
    [Parameter(Mandatory = $true)]
    [string]$Port
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (Get-Command "esptool" -ErrorAction SilentlyContinue) {
    $Tool = @("esptool")
} elseif (Get-Command "esptool.py" -ErrorAction SilentlyContinue) {
    $Tool = @("esptool.py")
} else {
    $Tool = @("python3", "-m", "esptool")
}

$FlashArgsPath = Join-Path $ScriptDir "flash_args"
$Lines = Get-Content -Path $FlashArgsPath
$WriteArgs = @()
$FileArgs = @()
if ($Lines.Length -gt 0) {
    $WriteArgs = $Lines[0] -split '\s+' | Where-Object { $_ -ne "" }
}
if ($Lines.Length -gt 1) {
    $FileArgs = $Lines[1] -split '\s+' | Where-Object { $_ -ne "" }
}

function Invoke-Tool {
    param([string[]]$ToolCmd, [string[]]$Args)
    $All = @($ToolCmd + $Args)
    if ($All.Length -gt 1) {
        & $All[0] @($All[1..($All.Length - 1)])
    } else {
        & $All[0]
    }
}

Invoke-Tool -ToolCmd $Tool -Args @("--port", $Port, "erase_flash")
$WriteFlashArgs = @("--port", $Port, "write_flash") + $WriteArgs + $FileArgs
Invoke-Tool -ToolCmd $Tool -Args $WriteFlashArgs
