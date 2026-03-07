Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "x64",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipTests,
    [switch]$SkipBenchmarks,
    [switch]$SkipPack,
    [switch]$SkipReplayVerify,
    [switch]$SkipSamplePack,
    [string]$PackInputDir = "examples/content_packs",
    [string]$PackOutput = "content.pak",
    [string]$PackId = "starter",
    [int]$ReplayTicks = 600,
    [uint64]$ReplaySeed = 1337
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

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[build_release] Removing existing build directory: $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

$cmakeArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", $Generator,
    "-A", $Platform,
    "-DENGINE_DETERMINISTIC_BUILD=$($Deterministic.IsPresent ? 'ON' : 'OFF')",
    "-DFETCHCONTENT_UPDATES_DISCONNECTED=ON"
)

Write-Host "[build_release] Configuring $BuildDir (Release)"
& cmake @cmakeArgs

Write-Host "[build_release] Building targets"
cmake --build $BuildDir --config Release

if (-not $SkipTests) {
    Write-Host "[build_release] Running tests"
    ctest --test-dir $BuildDir -C Release --output-on-failure
}

if (-not $SkipBenchmarks) {
    $benchExe = Get-BuiltExecutablePath -BuildDir $BuildDir -Name "benchmark_suite_tests"
    Write-Host "[build_release] Running benchmark threshold checks: $benchExe"
    & $benchExe --assert-thresholds
    if ($LASTEXITCODE -ne 0) {
        throw "Benchmark thresholds failed with exit code $LASTEXITCODE"
    }
}

if (-not $SkipPack) {
    $packerExe = Get-BuiltExecutablePath -BuildDir $BuildDir -Name "ContentPacker"
    Write-Host "[build_release] Building default content pack: $PackOutput"
    & $packerExe --input data --output $PackOutput --pack-id $PackId
    if ($LASTEXITCODE -ne 0) {
        throw "ContentPacker failed for data with exit code $LASTEXITCODE"
    }

    if (-not $SkipSamplePack) {
        $sampleOutput = Join-Path (Split-Path -Parent $PackOutput) "sample-content.pak"
        if ([string]::IsNullOrWhiteSpace($sampleOutput)) {
            $sampleOutput = "sample-content.pak"
        }
        Write-Host "[build_release] Building sample content pack: $sampleOutput"
        & $packerExe --input $PackInputDir --output $sampleOutput --pack-id "$PackId-sample"
        if ($LASTEXITCODE -ne 0) {
            throw "ContentPacker failed for sample pack with exit code $LASTEXITCODE"
        }
    }
}

if (-not $SkipReplayVerify) {
    $engineExe = Get-BuiltExecutablePath -BuildDir $BuildDir -Name "EngineDemo"
    Write-Host "[build_release] Replay verify against pack '$PackOutput'"
    & $engineExe --replay-verify --headless --ticks $ReplayTicks --seed $ReplaySeed --content-pack $PackOutput
    if ($LASTEXITCODE -ne 0) {
        throw "Replay verification failed with exit code $LASTEXITCODE"
    }
}

Write-Host "[build_release] Done"
