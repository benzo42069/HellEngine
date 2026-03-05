Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "x64",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

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

Write-Host "[build_release] Done"
