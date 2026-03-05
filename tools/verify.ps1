param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

Write-Host "[verify] configure"
cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=$Config

Write-Host "[verify] build"
cmake --build $BuildDir --config $Config

Write-Host "[verify] ctest"
ctest --test-dir $BuildDir -C $Config --output-on-failure

Write-Host "[verify] smoke run"
$demoPath = Join-Path $BuildDir "EngineDemo"
if (-not (Test-Path $demoPath)) {
    $demoPath = Join-Path $BuildDir "$Config/EngineDemo.exe"
}

if (-not (Test-Path $demoPath)) {
    throw "EngineDemo executable not found in $BuildDir"
}

& $demoPath --headless --renderer-smoke-test --ticks 10
if ($LASTEXITCODE -ne 0) {
    throw "EngineDemo smoke run failed with exit code $LASTEXITCODE"
}

Write-Host "[verify] success"
