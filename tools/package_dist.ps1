Param(
    [string]$BuildDir = "build-release",
    [string]$DistRoot = "dist",
    [string]$PortableDirName = "portable",
    [string]$ArchiveName = "EngineDemo-portable.zip",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipBuild,
    [switch]$SkipValidation
)

$ErrorActionPreference = "Stop"

function Get-BuiltExecutablePath {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $candidates = @(
        (Join-Path $BuildDir "Release/$Name.exe"),
        (Join-Path $BuildDir "$Name.exe"),
        (Join-Path $BuildDir "Release/$Name"),
        (Join-Path $BuildDir $Name)
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Unable to locate executable '$Name' in build output '$BuildDir'."
}

$distDir = Join-Path $DistRoot $PortableDirName
$archivePath = Join-Path $DistRoot $ArchiveName

if (-not $SkipBuild) {
    Write-Host "[package_dist] Ensuring release build is present"
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

$engineExe = Get-BuiltExecutablePath -BuildDir $BuildDir -Name "EngineDemo"
$contentPackerExe = Get-BuiltExecutablePath -BuildDir $BuildDir -Name "ContentPacker"

Copy-Item $engineExe (Join-Path $distDir "EngineDemo$(if ($engineExe.EndsWith('.exe')) {'.exe'})")
Copy-Item $contentPackerExe (Join-Path $distDir "ContentPacker$(if ($contentPackerExe.EndsWith('.exe')) {'.exe'})")

if (Test-Path "engine_config.json") {
    Copy-Item "engine_config.json" (Join-Path $distDir "engine_config.json")
}

if (Test-Path "content.pak") {
    Copy-Item "content.pak" (Join-Path $distDir "content.pak")
}

if (Test-Path "sample-content.pak") {
    Copy-Item "sample-content.pak" (Join-Path $distDir "sample-content.pak")
}

if (Test-Path "data") {
    Copy-Item "data" (Join-Path $distDir "data") -Recurse
}
if (Test-Path "assets") {
    Copy-Item "assets" (Join-Path $distDir "assets") -Recurse
}
if (Test-Path "examples") {
    Copy-Item "examples" (Join-Path $distDir "examples") -Recurse
}
if (Test-Path "docs/BuildAndRun.md") {
    Copy-Item "docs/BuildAndRun.md" (Join-Path $distDir "BuildAndRun.md")
}
if (Test-Path "docs/BuildAndRelease.md") {
    Copy-Item "docs/BuildAndRelease.md" (Join-Path $distDir "BuildAndRelease.md")
}
if (Test-Path "version/VERSION.txt") {
    Copy-Item "version/VERSION.txt" (Join-Path $distDir "VERSION.txt")
}

$runtimeDlls = @("SDL2.dll")
foreach ($dll in $runtimeDlls) {
    $candidate = Join-Path $BuildDir "Release/$dll"
    if (Test-Path $candidate) {
        Copy-Item $candidate (Join-Path $distDir $dll)
    }
}

if (-not $SkipValidation) {
    $portableEngine = Join-Path $distDir "EngineDemo$(if ($engineExe.EndsWith('.exe')) {'.exe'})"
    if (-not (Test-Path $portableEngine)) {
        throw "Portable validation failed: missing EngineDemo executable"
    }

    $portablePacker = Join-Path $distDir "ContentPacker$(if ($contentPackerExe.EndsWith('.exe')) {'.exe'})"
    if (-not (Test-Path $portablePacker)) {
        throw "Portable validation failed: missing ContentPacker executable"
    }

    Write-Host "[package_dist] Validating content pack can be generated from portable bundle"
    Push-Location $distDir
    try {
        & $portablePacker --input examples/content_packs --output dist-verify-sample-content.pak --pack-id dist-verify
        if ($LASTEXITCODE -ne 0) {
            throw "Portable ContentPacker validation failed with exit code $LASTEXITCODE"
        }

        Write-Host "[package_dist] Running headless smoke test from portable bundle"
        & $portableEngine --headless --renderer-smoke-test --ticks 10 --content-pack content.pak
        if ($LASTEXITCODE -ne 0) {
            throw "Portable EngineDemo smoke validation failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
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
