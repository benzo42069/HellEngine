Param(
    [string]$BuildDir = "build-debug",
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$BuildIfMissing,
    [switch]$Deterministic,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ctestFile = Join-Path $BuildDir "CTestTestfile.cmake"
if ($BuildIfMissing -and -not (Test-Path $ctestFile)) {
    if ($Config -eq "Release") {
        & "$PSScriptRoot/build_release.ps1" -BuildDir $BuildDir -Deterministic:$Deterministic -Clean:$Clean
    }
    else {
        & "$PSScriptRoot/build_debug.ps1" -BuildDir $BuildDir -Deterministic:$Deterministic -Clean:$Clean
    }
}

Write-Host "[run_tests] Running ctest ($Config) in $BuildDir"
ctest --test-dir $BuildDir -C $Config --output-on-failure

Write-Host "[run_tests] Done"
