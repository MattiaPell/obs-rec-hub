#requires -Version 5.1
<#
.SYNOPSIS
    Installs the built plugin into an OBS Studio plugin location.

.DESCRIPTION
    Two destinations are supported:

    * ProgramData (default, no admin rights required)
        C:\ProgramData\obs-studio\plugins\obs-rec-hud\bin\64bit\obs-rec-hud.dll
        C:\ProgramData\obs-studio\plugins\obs-rec-hud\data\locale\*.ini

    * The OBS install folder (needs an elevated shell)
        <ObsDir>\obs-plugins\64bit\obs-rec-hud.dll
        <ObsDir>\obs-plugins\obs-rec-hud\data\locale\*.ini
#>
[CmdletBinding()]
param(
    [ValidateSet('ProgramData', 'InstallDir')]
    [string]$Location = 'ProgramData',
    [string]$ObsDir = '',
    [string]$Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root '.work'
$rundir = Join-Path $work "build_x64\rundir\$Configuration"

$dll = Join-Path $rundir 'obs-rec-hud.dll'
if (-not (Test-Path -LiteralPath $dll)) {
    throw 'Plugin not built yet. Run .\scripts\build.ps1 first.'
}

$localeSrc = Join-Path $rundir 'obs-rec-hud\locale'
if (-not (Test-Path -LiteralPath $localeSrc)) {
    throw "Locale files not found in: $localeSrc"
}

$moduleName = 'obs-rec-hud'

if ($Location -eq 'ProgramData') {
    $binDir = "C:\ProgramData\obs-studio\plugins\$moduleName\bin\64bit"
    $dataDir = "C:\ProgramData\obs-studio\plugins\$moduleName\data\locale"
}
else {
    if (-not $ObsDir) {
        $candidates = @(
            (Join-Path $env:ProgramFiles 'obs-studio'),
            (Join-Path ${env:ProgramFiles(x86)} 'obs-studio'),
            (Join-Path $env:LOCALAPPDATA 'Programs\obs-studio')
        ) | Where-Object { $_ -and (Test-Path -LiteralPath (Join-Path $_ 'bin\64bit\obs64.exe')) }

        if (-not $candidates) {
            throw 'OBS Studio not found. Pass -ObsDir "C:\Program Files\obs-studio"'
        }

        # NOTE: select the first *element*; indexing a single string yields a character.
        $ObsDir = $candidates | Select-Object -First 1
    }

    $binDir = Join-Path $ObsDir "obs-plugins\64bit"
    $dataDir = Join-Path $ObsDir "obs-plugins\$moduleName\data\locale"
}

Write-Host "==> Binary -> $binDir" -ForegroundColor Cyan
Write-Host "==> Locale -> $dataDir" -ForegroundColor Cyan

$running = Get-Process -Name 'obs64' -ErrorAction SilentlyContinue
if ($running) {
    Write-Host '    OBS is running: close it and re-run so the DLL is not locked.' -ForegroundColor Yellow
    exit 1
}

New-Item -ItemType Directory -Path $binDir -Force | Out-Null
New-Item -ItemType Directory -Path $dataDir -Force | Out-Null

Copy-Item -LiteralPath $dll -Destination (Join-Path $binDir "$moduleName.dll") -Force
Get-ChildItem -LiteralPath $localeSrc -Filter '*.ini' |
    Copy-Item -Destination $dataDir -Force

Write-Host ''
Write-Host '==> Installed.' -ForegroundColor Green
Get-ChildItem -LiteralPath $binDir -Filter "$moduleName.dll" |
    ForEach-Object { Write-Host "    $($_.FullName)" -ForegroundColor Green }
Get-ChildItem -LiteralPath $dataDir -Filter '*.ini' |
    ForEach-Object { Write-Host "    $($_.FullName)" -ForegroundColor Green }
Write-Host ''
Write-Host '    Start OBS Studio, then: Tools > REC HUD settings...' -ForegroundColor Green