Param(
    [string]$BuildDir = "build-release",
    [string]$DistRoot = "dist",
    [string]$PortableDirName = "portable",
    [string]$ArchiveName = "EngineDemo-portable.zip",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipBuild,
    [switch]$SkipValidation,
    [string]$ManifestName = "RELEASE_MANIFEST.txt"
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

function Copy-RuntimeDependencies {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$DestinationDir
    )

    $releaseDir = Join-Path $BuildDir "Release"
    $candidateDirs = @($releaseDir, $BuildDir) | Where-Object { Test-Path $_ }
    $copied = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)

    foreach ($dir in $candidateDirs) {
        Get-ChildItem -Path $dir -Filter "*.dll" -File | Sort-Object Name | ForEach-Object {
            if ($copied.Add($_.Name)) {
                Copy-Item $_.FullName (Join-Path $DestinationDir $_.Name)
            }
        }
    }

    if ($copied.Count -eq 0) {
        Write-Host "[package_dist] No runtime DLLs discovered in build output"
    }
    else {
        Write-Host "[package_dist] Bundled runtime DLLs: $($copied -join ', ')"
    }
}

function Write-ReleaseManifest {
    param(
        [Parameter(Mandatory = $true)][string]$PortableDir,
        [Parameter(Mandatory = $true)][string]$ManifestPath
    )

    $manifestLines = @(
        "# HellEngine Portable Release Manifest",
        "FormatVersion=1",
        "",
        "## File Inventory"
    )

    $files = Get-ChildItem -Path $PortableDir -File -Recurse | ForEach-Object {
        [PSCustomObject]@{
            Relative = [System.IO.Path]::GetRelativePath($PortableDir, $_.FullName)
            FullName = $_.FullName
            Length = $_.Length
        }
    } | Sort-Object Relative

    foreach ($file in $files) {
        $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash
        $manifestLines += "$($file.Relative) | $($file.Length) bytes | SHA256=$hash"
    }

    Set-Content -Path $ManifestPath -Value $manifestLines -Encoding UTF8
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

Copy-RuntimeDependencies -BuildDir $BuildDir -DestinationDir $distDir

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

        if (Test-Path "sample-content.pak") {
            Write-Host "[package_dist] Running sample pack replay verify from portable bundle"
            & $portableEngine --replay-verify --headless --ticks 180 --seed 1337 --content-pack sample-content.pak
            if ($LASTEXITCODE -ne 0) {
                throw "Portable sample-content replay verification failed with exit code $LASTEXITCODE"
            }
        }
    }
    finally {
        Pop-Location
    }
}

$manifestPath = Join-Path $distDir $ManifestName
Write-Host "[package_dist] Writing release manifest: $manifestPath"
Write-ReleaseManifest -PortableDir $distDir -ManifestPath $manifestPath

if (-not (Test-Path $DistRoot)) {
    New-Item -ItemType Directory -Path $DistRoot | Out-Null
}
if (Test-Path $archivePath) {
    Remove-Item -Force $archivePath
}

Write-Host "[package_dist] Creating zip archive: $archivePath"
Compress-Archive -Path "$distDir/*" -DestinationPath $archivePath

Write-Host "[package_dist] Done"
