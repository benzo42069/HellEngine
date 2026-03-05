Param(
    [string]$BuildType = "Debug",
    [switch]$SkipReplayVerify
)

$ErrorActionPreference = "Stop"

Write-Host "[ci_local] Configure"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=$BuildType

Write-Host "[ci_local] Build"
cmake --build build -j 4

Write-Host "[ci_local] Unit/Integration tests"
ctest --test-dir build --output-on-failure

Write-Host "[ci_local] Build content pack"
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter

if (-not $SkipReplayVerify) {
    Write-Host "[ci_local] Replay verify"
    ./build/EngineDemo --replay-verify --headless --ticks 600 --seed 1337 --content-pack content.pak
}

Write-Host "[ci_local] Benchmarks with thresholds"
./build/benchmark_suite_tests --assert-thresholds

Write-Host "[ci_local] PASS"
