Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-release",
    [string]$Generator = "Ninja"
)

$ErrorActionPreference = "Stop"

Write-Host "[build_release] Configuring $BuildDir (Release)"
cmake -S $SourceDir -B $BuildDir -G $Generator -DCMAKE_BUILD_TYPE=Release

Write-Host "[build_release] Building targets"
cmake --build $BuildDir --config Release

Write-Host "[build_release] Running tests"
ctest --test-dir $BuildDir -C Release --output-on-failure

Write-Host "[build_release] Done"
