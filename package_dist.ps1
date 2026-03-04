Param(
    [string]$BuildDir = "build-release",
    [string]$DistDir = "dist/portable",
    [string]$ArchivePath = "dist/EngineDemo-portable.zip"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path "$BuildDir/EngineDemo.exe")) {
    Write-Host "[package_dist] Release build not found. Running build_release.ps1 first."
    & "$PSScriptRoot/build_release.ps1" -BuildDir $BuildDir
}

Write-Host "[package_dist] Preparing portable folder: $DistDir"
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item "$BuildDir/EngineDemo.exe" "$DistDir/EngineDemo.exe"
if (Test-Path "$BuildDir/ContentPacker.exe") {
    Copy-Item "$BuildDir/ContentPacker.exe" "$DistDir/ContentPacker.exe"
}

if (Test-Path "engine_config.json") {
    Copy-Item "engine_config.json" "$DistDir/engine_config.json"
}
if (Test-Path "data") {
    Copy-Item "data" "$DistDir/data" -Recurse
}
if (Test-Path "assets") {
    Copy-Item "assets" "$DistDir/assets" -Recurse
}
if (Test-Path "docs") {
    Copy-Item "docs/BuildAndRun.md" "$DistDir/BuildAndRun.md"
}

$archiveDir = Split-Path -Parent $ArchivePath
if (-not (Test-Path $archiveDir)) {
    New-Item -ItemType Directory -Path $archiveDir | Out-Null
}
if (Test-Path $ArchivePath) {
    Remove-Item -Force $ArchivePath
}

Write-Host "[package_dist] Creating zip archive: $ArchivePath"
Compress-Archive -Path "$DistDir/*" -DestinationPath $ArchivePath

Write-Host "[package_dist] Done"
