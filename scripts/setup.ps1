#requires -Version 5.1
<#
.SYNOPSIS
    Prepares a buildable copy of the REC HUD plugin.

.DESCRIPTION
    The OBS plugin template (obsproject/obs-plugintemplate) provides the CMake
    toolchain, dependency downloads and packaging rules. This script downloads a
    pristine copy of it into .work\ and overlays the REC HUD sources on top.

    Git is NOT required: the template is fetched as a zip archive.
#>
[CmdletBinding()]
param(
    [string]$Ref = 'master'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root '.work'
$zip = Join-Path $root '.work-template.zip'

Write-Host '==> Cleaning previous workspace' -ForegroundColor Cyan
if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force }
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }

Write-Host "==> Downloading obs-plugintemplate ($Ref)" -ForegroundColor Cyan
$url = "https://codeload.github.com/obsproject/obs-plugintemplate/zip/refs/heads/$Ref"
Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing

Write-Host '==> Extracting template' -ForegroundColor Cyan
Expand-Archive -LiteralPath $zip -DestinationPath $work -Force

$inner = Get-ChildItem -LiteralPath $work -Directory | Select-Object -First 1
if (-not $inner) { throw 'Unexpected archive layout.' }

Get-ChildItem -LiteralPath $inner.FullName -Force | ForEach-Object {
    Move-Item -LiteralPath $_.FullName -Destination $work -Force
}
Remove-Item -LiteralPath $inner.FullName -Recurse -Force
Remove-Item -LiteralPath $zip -Force

Write-Host '==> Applying REC HUD files' -ForegroundColor Cyan

foreach ($file in @('CMakeLists.txt', 'CMakePresets.json', 'buildspec.json', '.gitignore')) {
    $src = Join-Path $root $file
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $work $file) -Force
    }
}

$workSrc = Join-Path $work 'src'
Get-ChildItem -LiteralPath $workSrc -Force | Remove-Item -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'src') -Destination $work -Recurse -Force

$workLocale = Join-Path $work 'data\locale'
Remove-Item -LiteralPath $workLocale -Recurse -Force
New-Item -ItemType Directory -Path $workLocale -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'data\locale') -Filter '*.ini' |
    Copy-Item -Destination $workLocale -Force

Write-Host ''
Write-Host '==> Setup complete.' -ForegroundColor Green
Write-Host "    Build with:  .\scripts\build.ps1" -ForegroundColor Green