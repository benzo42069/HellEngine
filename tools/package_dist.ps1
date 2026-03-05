Param(
    [string]$BuildDir = "build-release",
    [string]$DistRoot = "dist",
    [string]$PortableDirName = "portable",
    [string]$ArchiveName = "EngineDemo-portable.zip",
    [switch]$Clean,
    [switch]$Deterministic
)

$ErrorActionPreference = "Stop"

$distDir = Join-Path $DistRoot $PortableDirName
$archivePath = Join-Path $DistRoot $ArchiveName

if (-not (Test-Path (Join-Path $BuildDir "Release/EngineDemo.exe")) -and -not (Test-Path (Join-Path $BuildDir "EngineDemo.exe"))) {
    Write-Host "[package_dist] Release build not found. Running build_release.ps1 first."
    & "$PSScriptRoot/build_release.ps1" -BuildDir $BuildDir -Clean:$Clean -Deterministic:$Deterministic
}

if ($Clean -and (Test-Path $DistRoot)) {
    Write-Host "[package_dist] Removing existing dist root: $DistRoot"
    Remove-Item -Recurse -Force $DistRoot
}

Write-Host "[package_dist] Preparing portable folder: $distDir"
if (Test-Path $distDir) {
    Remove-Item -Recurse -Force $distDir
}
New-Item -ItemType Directory -Path $distDir | Out-Null

$engineExeCandidates = @(
    (Join-Path $BuildDir "Release/EngineDemo.exe"),
    (Join-Path $BuildDir "EngineDemo.exe")
)
foreach ($candidate in $engineExeCandidates) {
    if (Test-Path $candidate) {
        Copy-Item $candidate (Join-Path $distDir "EngineDemo.exe")
        break
    }
}

$contentPackerCandidates = @(
    (Join-Path $BuildDir "Release/ContentPacker.exe"),
    (Join-Path $BuildDir "ContentPacker.exe")
)
foreach ($candidate in $contentPackerCandidates) {
    if (Test-Path $candidate) {
        Copy-Item $candidate (Join-Path $distDir "ContentPacker.exe")
        break
    }
}

if (Test-Path "engine_config.json") {
    Copy-Item "engine_config.json" (Join-Path $distDir "engine_config.json")
}
if (Test-Path "data") {
    Copy-Item "data" (Join-Path $distDir "data") -Recurse
}
if (Test-Path "assets") {
    Copy-Item "assets" (Join-Path $distDir "assets") -Recurse
}
if (Test-Path "docs/BuildAndRun.md") {
    Copy-Item "docs/BuildAndRun.md" (Join-Path $distDir "BuildAndRun.md")
}
if (Test-Path "version/VERSION.txt") {
    Copy-Item "version/VERSION.txt" (Join-Path $distDir "VERSION.txt")
}

if (-not (Test-Path $DistRoot)) {
    New-Item -ItemType Directory -Path $DistRoot | Out-Null
}
if (Test-Path $archivePath) {
    Remove-Item -Force $archivePath
}

Write-Host "[package_dist] Creating zip archive: $archivePath"
Compress-Archive -Path "$distDir/*" -DestinationPath $archivePath

Write-Host "[package_dist] Done"
