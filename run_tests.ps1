# Set up MSVC environment
$vsPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
$rawEnv = cmd /c "`"$vsPath`" x64 && set" 2>&1
foreach ($line in $rawEnv) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}

Set-Location 'C:\Users\UserName\Dev\HellEngine'

# Generate content pack for content_packer_tests (run from project root so relative paths in manifest resolve)
Write-Host "=== Generating content pack ==="
$packResult = & .\build\ContentPacker.exe --input 'C:\Users\UserName\Dev\HellEngine\data' --output 'C:\Users\UserName\Dev\HellEngine\build\content_test.pak' 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[WARN] ContentPacker failed (exit $LASTEXITCODE):"
    $packResult | ForEach-Object { Write-Host "  $_" }
} else {
    Write-Host "[OK] content_test.pak generated"
}

Set-Location 'C:\Users\UserName\Dev\HellEngine\build'

$tests = @(
    'timing_tests', 'config_tests', 'render2d_tests', 'projectile_tests',
    'pattern_tests', 'content_packer_tests', 'content_import_pipeline_tests',
    'entity_tests', 'trait_tests', 'archetype_tests', 'boss_phase_tests',
    'run_structure_tests', 'meta_progression_tests', 'persistence_tests',
    'editor_tools_tests', 'replay_tests', 'upgrade_ui_tests', 'upgrade_ui_model_tests',
    'projectile_behavior_tests', 'pattern_graph_tests', 'pattern_generator_tests',
    'pattern_graph_perf_tests', 'encounter_graph_tests', 'difficulty_scaling_tests',
    'diagnostics_tests', 'gpu_bullets_tests', 'golden_replay_tests',
    'benchmark_suite_tests', 'public_api_tests', 'renderer_smoke_tests',
    'palette_fx_templates_tests', 'modern_renderer_tests', 'defensive_special_tests',
    'gameplay_session_state_tests', 'standards_tests', 'collision_correctness_tests',
    'determinism_property_tests', 'content_fuzz_tests', 'determinism_cross_config_test'
)

$passed = 0
$failed = 0
$failedTests = @()

foreach ($test in $tests) {
    $exe = ".\$test.exe"
    if (Test-Path $exe) {
        $result = & $exe 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[PASS] $test"
            $passed++
        } else {
            Write-Host "[FAIL] $test (exit $LASTEXITCODE)"
            $result | Select-Object -Last 20 | ForEach-Object { Write-Host "  $_" }
            $failed++
            $failedTests += $test
        }
    } else {
        Write-Host "[SKIP] $test.exe not found"
    }
}

Write-Host ""
Write-Host "=== Results: $passed passed, $failed failed ==="
if ($failedTests.Count -gt 0) {
    Write-Host "Failed: $($failedTests -join ', ')"
    exit 1
}
exit 0
