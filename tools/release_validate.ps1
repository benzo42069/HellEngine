Param(
    [string]$BuildDir = "build-release",
    [string]$DistRoot = "dist",
    [string]$PortableDirName = "portable",
    [switch]$Clean,
    [switch]$Deterministic
)

$ErrorActionPreference = "Stop"

$portableDir = Join-Path $DistRoot $PortableDirName
$archivePath = Join-Path $DistRoot "EngineDemo-portable.zip"

Write-Host "[release_validate] Build + test + pack"
& "$PSScriptRoot/build_release.ps1" -BuildDir $BuildDir -Clean:$Clean -Deterministic:$Deterministic

Write-Host "[release_validate] Portable dist + archive"
& "$PSScriptRoot/package_dist.ps1" -BuildDir $BuildDir -Clean:$Clean -Deterministic:$Deterministic -SkipBuild

if (-not (Test-Path $archivePath)) {
    throw "[release_validate] Expected archive missing: $archivePath"
}
if (-not (Test-Path (Join-Path $portableDir "EngineDemo.exe")) -and -not (Test-Path (Join-Path $portableDir "EngineDemo"))) {
    throw "[release_validate] Portable EngineDemo missing"
}
if (-not (Test-Path (Join-Path $portableDir "ContentPacker.exe")) -and -not (Test-Path (Join-Path $portableDir "ContentPacker"))) {
    throw "[release_validate] Portable ContentPacker missing"
}
if (-not (Test-Path (Join-Path $portableDir "content.pak"))) {
    throw "[release_validate] Portable content.pak missing"
}
if (-not (Test-Path (Join-Path $portableDir "sample-content.pak"))) {
    throw "[release_validate] Portable sample-content.pak missing"
}

$manifestPath = Join-Path $portableDir "RELEASE_MANIFEST.txt"
if (-not (Test-Path $manifestPath)) {
    throw "[release_validate] Portable release manifest missing: $manifestPath"
}

$manifestRequiredEntries = @(
    "EngineDemo",
    "ContentPacker",
    "content.pak",
    "sample-content.pak",
    "engine_config.json",
    "VERSION.txt"
)

$manifestText = Get-Content $manifestPath -Raw
foreach ($entry in $manifestRequiredEntries) {
    if ($manifestText -notmatch [Regex]::Escape($entry)) {
        throw "[release_validate] Manifest missing required entry fragment: $entry"
    }
}

Write-Host "[release_validate] PASS"
