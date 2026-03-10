# Set up MSVC environment and build
$vsPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
$rawEnv = cmd /c "`"$vsPath`" x64 && set" 2>&1
foreach ($line in $rawEnv) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}

Set-Location 'C:\Users\UserName\Dev\HellEngine'

Write-Host "=== Building (incremental) ==="
cmake --build build --config Release -- -j8 2>&1
$buildExit = $LASTEXITCODE
Write-Host "BUILD EXIT: $buildExit"
exit $buildExit
