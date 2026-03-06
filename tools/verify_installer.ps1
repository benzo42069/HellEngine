Param(
    [string]$DistRoot = "dist",
    [string]$AppName = "EngineDemo",
    [switch]$RequireInstaller
)

$ErrorActionPreference = "Stop"

$distDir = Join-Path $DistRoot $AppName
$portableZip = Join-Path $DistRoot "$AppName-portable.zip"
$installerExe = Join-Path $DistRoot "$($AppName)Installer.exe"

if (-not (Test-Path $distDir)) {
    throw "Missing dist directory: $distDir"
}

$requiredPaths = @(
    "EngineDemo.exe",
    "ContentPacker.exe",
    "assets",
    "data",
    "packs",
    "config"
)

foreach ($rel in $requiredPaths) {
    $path = Join-Path $distDir $rel
    if (-not (Test-Path $path)) {
        throw "Missing required dist item: $path"
    }
}

if (-not (Test-Path $portableZip)) {
    throw "Missing portable archive: $portableZip"
}

if ($RequireInstaller -and -not (Test-Path $installerExe)) {
    throw "Missing installer executable: $installerExe"
}

Write-Host "[verify_installer] Dist layout looks valid"
Write-Host "[verify_installer] Dist dir: $distDir"
Write-Host "[verify_installer] Portable zip: $portableZip"
if (Test-Path $installerExe) {
    Write-Host "[verify_installer] Installer exe: $installerExe"
}

Write-Host "[verify_installer] Runtime checks to perform on Windows machine:"
Write-Host "  1) Run installer (UI): $installerExe"
Write-Host "  2) Confirm shortcuts: Start Menu + optional Desktop"
Write-Host "  3) Launch from install location and verify assets/data load"
Write-Host "  4) Silent install: EngineDemoInstaller.exe /VERYSILENT /NORESTART"
Write-Host "  5) Uninstall from Apps & Features and confirm install directory removed"
