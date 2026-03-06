Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-release",
    [string]$Config = "Release",
    [string]$DistRoot = "dist",
    [string]$AppName = "EngineDemo",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipBuild,
    [switch]$SkipPackBuild
)

$ErrorActionPreference = "Stop"

function Resolve-BinaryPath {
    param(
        [string]$BuildDir,
        [string]$Config,
        [string]$BaseName
    )

    $candidates = @(
        (Join-Path $BuildDir "$Config/$BaseName.exe"),
        (Join-Path $BuildDir "$BaseName.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Could not find $BaseName.exe in '$BuildDir' (config '$Config')."
}

function Copy-IfExists {
    param(
        [string]$From,
        [string]$To
    )

    if (Test-Path $From) {
        Copy-Item $From $To -Recurse -Force
        return $true
    }

    return $false
}

$repoRoot = (Resolve-Path $SourceDir).Path
$stageDir = Join-Path $DistRoot $AppName
$portableZip = Join-Path $DistRoot "$AppName-portable.zip"

if (-not $SkipBuild) {
    Write-Host "[package_dist] Ensuring release build exists"
    & "$PSScriptRoot/build_release.ps1" -SourceDir $repoRoot -BuildDir $BuildDir -Clean:$Clean -Deterministic:$Deterministic -SkipTests
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed; cannot package distribution."
    }
}

$engineExe = Resolve-BinaryPath -BuildDir $BuildDir -Config $Config -BaseName "EngineDemo"
$contentPackerExe = Resolve-BinaryPath -BuildDir $BuildDir -Config $Config -BaseName "ContentPacker"
$exeDir = Split-Path -Parent $engineExe

if ($Clean -and (Test-Path $DistRoot)) {
    Write-Host "[package_dist] Removing existing dist root: $DistRoot"
    Remove-Item -Recurse -Force $DistRoot
}

if (Test-Path $stageDir) {
    Remove-Item -Recurse -Force $stageDir
}

$null = New-Item -ItemType Directory -Path $stageDir -Force
$null = New-Item -ItemType Directory -Path (Join-Path $stageDir "assets") -Force
$null = New-Item -ItemType Directory -Path (Join-Path $stageDir "data") -Force
$null = New-Item -ItemType Directory -Path (Join-Path $stageDir "packs") -Force
$null = New-Item -ItemType Directory -Path (Join-Path $stageDir "config") -Force

Write-Host "[package_dist] Staging binaries"
Copy-Item $engineExe (Join-Path $stageDir "EngineDemo.exe") -Force
Copy-Item $contentPackerExe (Join-Path $stageDir "ContentPacker.exe") -Force

Get-ChildItem $exeDir -Filter "*.dll" -File -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $stageDir $_.Name) -Force
}

if (Test-Path (Join-Path $repoRoot "assets")) {
    Copy-Item (Join-Path $repoRoot "assets/*") (Join-Path $stageDir "assets") -Recurse -Force
}
if (Test-Path (Join-Path $repoRoot "data")) {
    Copy-Item (Join-Path $repoRoot "data/*") (Join-Path $stageDir "data") -Recurse -Force
}
if (Test-Path (Join-Path $repoRoot "shaders")) {
    $null = New-Item -ItemType Directory -Path (Join-Path $stageDir "shaders") -Force
    Copy-Item (Join-Path $repoRoot "shaders/*") (Join-Path $stageDir "shaders") -Recurse -Force
}

if (Test-Path (Join-Path $repoRoot "engine_config.json")) {
    Copy-Item (Join-Path $repoRoot "engine_config.json") (Join-Path $stageDir "config/engine_config.json") -Force
}

if (-not $SkipPackBuild -and (Test-Path (Join-Path $repoRoot "examples/content_packs"))) {
    Write-Host "[package_dist] Building starter content pack"
    & $contentPackerExe --input (Join-Path $repoRoot "examples/content_packs") --output (Join-Path $stageDir "packs/content.pak") --pack-id starter
    if ($LASTEXITCODE -ne 0) {
        throw "ContentPacker failed while generating packs/content.pak"
    }
}

if (Test-Path (Join-Path $repoRoot "version/VERSION.txt")) {
    Copy-Item (Join-Path $repoRoot "version/VERSION.txt") (Join-Path $stageDir "VERSION.txt") -Force
}

if (Test-Path (Join-Path $repoRoot "docs/BuildAndRun.md")) {
    Copy-Item (Join-Path $repoRoot "docs/BuildAndRun.md") (Join-Path $stageDir "README.txt") -Force
}

if (Test-Path (Join-Path $repoRoot "LICENSE")) {
    Copy-Item (Join-Path $repoRoot "LICENSE") (Join-Path $stageDir "LICENSE.txt") -Force
}

$manifest = @{
    appName = $AppName
    buildDir = $BuildDir
    config = $Config
    generatedAt = (Get-Date).ToString("o")
    files = (Get-ChildItem $stageDir -Recurse -File | ForEach-Object { $_.FullName.Substring($stageDir.Length + 1) })
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $stageDir "dist_manifest.json") -Encoding UTF8

if (Test-Path $portableZip) {
    Remove-Item -Force $portableZip
}
Write-Host "[package_dist] Creating portable zip: $portableZip"
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $portableZip

Write-Host "[package_dist] Portable staging complete"
Write-Host "[package_dist] Stage: $stageDir"
Write-Host "[package_dist] Zip:   $portableZip"
