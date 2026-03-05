Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-debug",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "x64",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[build_debug] Removing existing build directory: $BuildDir"
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

Write-Host "[build_debug] Configuring $BuildDir (Debug)"
& cmake @cmakeArgs

Write-Host "[build_debug] Building targets"
cmake --build $BuildDir --config Debug

if ($RunTests) {
    Write-Host "[build_debug] Running tests"
    ctest --test-dir $BuildDir -C Debug --output-on-failure
}

Write-Host "[build_debug] Done"
