Param(
    [string]$SourceDir = ".",
    [string]$BuildDir = "build-release",
    [string]$Config = "Release",
    [string]$DistRoot = "dist",
    [string]$AppName = "EngineDemo",
    [string]$AppPublisher = "EngineDemo",
    [switch]$Clean,
    [switch]$Deterministic,
    [switch]$SkipBuild,
    [switch]$SkipPortable,
    [switch]$AllowPortableOnly
)

$ErrorActionPreference = "Stop"

function Get-InnoCompiler {
    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $knownPaths = @(
        "${env:ProgramFiles(x86)}\\Inno Setup 6\\ISCC.exe",
        "${env:ProgramFiles}\\Inno Setup 6\\ISCC.exe"
    )
    foreach ($path in $knownPaths) {
        if ($path -and (Test-Path $path)) { return $path }
    }

    return $null
}

function Get-AppVersion {
    param([string]$RepoRoot)

    $versionFile = Join-Path $RepoRoot "version/VERSION.txt"
    if (Test-Path $versionFile) {
        $value = (Get-Content $versionFile -Raw).Trim()
        if ($value) { return $value }
    }

    return "0.0.0"
}

$repoRoot = (Resolve-Path $SourceDir).Path
$distDir = Join-Path $DistRoot $AppName
$issPath = Join-Path $repoRoot "installer/EngineDemo.iss"
$appVersion = Get-AppVersion -RepoRoot $repoRoot

if (-not $SkipPortable) {
    Write-Host "[build_installer] Preparing portable distribution layout"
    & "$PSScriptRoot/package_dist.ps1" -SourceDir $repoRoot -BuildDir $BuildDir -Config $Config -DistRoot $DistRoot -AppName $AppName -Clean:$Clean -Deterministic:$Deterministic -SkipBuild:$SkipBuild
    if ($LASTEXITCODE -ne 0) {
        throw "Portable packaging failed; installer step aborted."
    }
}

if (-not (Test-Path (Join-Path $distDir "EngineDemo.exe"))) {
    throw "Missing staged executable at '$distDir/EngineDemo.exe'."
}

if (-not (Test-Path $issPath)) {
    throw "Missing installer script: $issPath"
}

$innoCompiler = Get-InnoCompiler
if (-not $innoCompiler) {
    $msg = "Inno Setup compiler (ISCC.exe) not found. Portable package is available at '$distDir' and '$DistRoot/$AppName-portable.zip'."
    if ($AllowPortableOnly) {
        Write-Warning $msg
        exit 0
    }
    throw $msg
}

$distDirWin = $distDir -replace '/', '\\'
$distRootWin = $DistRoot -replace '/', '\\'

Write-Host "[build_installer] Building installer with Inno Setup"
& $innoCompiler "/DMyAppVersion=$appVersion" "/DMyAppPublisher=$AppPublisher" "/DMyDistDir=$distDirWin" "/DMyOutputDir=$distRootWin" $issPath
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE"
}

$installerPath = Join-Path $DistRoot "$($AppName)Installer.exe"
if (-not (Test-Path $installerPath)) {
    throw "Installer generation reported success, but output not found at '$installerPath'"
}

Write-Host "[build_installer] Installer ready: $installerPath"
Write-Host "[build_installer] Portable zip: $(Join-Path $DistRoot "$AppName-portable.zip")"
