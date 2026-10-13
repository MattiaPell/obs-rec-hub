#requires -Version 5.1
<#
.SYNOPSIS
    Configures and compiles the REC HUD plugin.

.DESCRIPTION
    The Visual Studio generator is auto-detected from the installed CMake, so
    this works on VS 2019/2022/2026 without editing any file.
#>
[CmdletBinding()]
param(
    [string]$Configuration = 'RelWithDebInfo',
    [string]$Generator = '',
    [string]$Architecture = 'x64'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root '.work'
$buildDir = Join-Path $work 'build_x64'

if (-not (Test-Path -LiteralPath (Join-Path $work 'CMakeLists.txt'))) {
    throw 'Workspace not prepared. Run .\scripts\setup.ps1 first.'
}

# ---------------------------------------------------------------- locate cmake
function Resolve-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "$env:ProgramFiles\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\CMake\bin\cmake.exe"
    ) | Where-Object { Test-Path -LiteralPath $_ }

    if ($candidates) { return ($candidates | Select-Object -First 1) }
    throw 'cmake was not found. Install it with: winget install Kitware.CMake'
}

$cmake = Resolve-CMake
Write-Host "==> Using cmake: $cmake" -ForegroundColor DarkGray
& $cmake --version | Select-Object -First 1

# ------------------------------------------------------- detect VS generator
if (-not $Generator) {
    $help = & $cmake --help
    $available = @($help | ForEach-Object {
            if ($_ -match '^\s*\*{0,2}\s*(Visual Studio\s+\d+\s+\d{4})') { $Matches[1] }
        })

    if ($available.Count -eq 0) {
        throw 'No Visual Studio generator found. Install the "Desktop development with C++" workload.'
    }

    # Prefer the newest installed Visual Studio.
    $Generator = $available[0].Trim()
    Write-Host "==> Using generator: $Generator" -ForegroundColor DarkGray
}

# -------------------------------------------------------------------- build
Push-Location $work
try {
    Write-Host ''
    Write-Host '==> Configuring' -ForegroundColor Cyan
    & $cmake -S $work -B $buildDir -G $Generator -A $Architecture `
        -DENABLE_FRONTEND_API=ON `
        -DENABLE_QT=ON `
        -DCMAKE_INSTALL_PREFIX="$env:ALLUSERSPROFILE/obs-studio/plugins"
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

    Write-Host ''
    Write-Host "==> Building ($Configuration)" -ForegroundColor Cyan
    & $cmake --build $buildDir --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed (exit $LASTEXITCODE)" }
}
finally {
    Pop-Location
}

$rundir = Join-Path $buildDir "rundir\$Configuration"

Write-Host ''
if (Test-Path -LiteralPath (Join-Path $rundir 'obs-rec-hud.dll')) {
    Write-Host '==> Build complete.' -ForegroundColor Green
    Write-Host "    Output: $rundir" -ForegroundColor Green
    Write-Host '    Install with:  .\scripts\install.ps1' -ForegroundColor Green
}
else {
    Write-Host '==> Build finished but obs-rec-hud.dll was not found in:' -ForegroundColor Yellow
    Write-Host "    $rundir" -ForegroundColor Yellow
    Write-Host '    Check the build log above for errors.' -ForegroundColor Yellow
}