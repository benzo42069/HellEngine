$vsPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
$output = cmd /c "`"$vsPath`" x64 && set" 2>&1
$output | Where-Object { $_ -match '^(LIB|PATH|INCLUDE|CL)=' } | Out-File 'env_vars.txt' -Encoding utf8
Write-Host "Done. Lines captured: $($output.Count)"
