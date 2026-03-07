Param(
    [string]$DistDir = "dist/portable",
    [string]$InstallerOut = "dist/EngineDemoInstaller.exe",
    [switch]$SkipIfToolingMissing
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path "$DistDir/EngineDemo.exe") -and -not (Test-Path "$DistDir/EngineDemo")) {
    Write-Host "[build_installer] Portable distribution missing. Running package_dist.ps1 first."
    & "$PSScriptRoot/package_dist.ps1"
}

if (-not (Test-Path $DistDir)) {
    throw "Distribution directory does not exist: $DistDir"
}

$makensis = Get-Command makensis -ErrorAction SilentlyContinue
if (-not $makensis) {
    if ($SkipIfToolingMissing) {
        Write-Warning "NSIS (makensis) not found. Skipping installer generation by request."
        exit 0
    }

    Write-Warning "NSIS (makensis) not found. Creating installer placeholder note instead."
    @"
Installer tooling not detected.
To produce a Windows installer, install NSIS and rerun:
  ./build_installer.ps1

Packaging inputs expected in:
  $DistDir
"@ | Set-Content -Path "$InstallerOut.txt"
    exit 0
}

$installerDir = Split-Path -Parent $InstallerOut
if (-not [string]::IsNullOrWhiteSpace($installerDir) -and -not (Test-Path $installerDir)) {
    New-Item -ItemType Directory -Path $installerDir | Out-Null
}

$resolvedDistDir = (Resolve-Path $DistDir).Path
$resolvedOut = [System.IO.Path]::GetFullPath($InstallerOut)
$nsisScript = Join-Path $env:TEMP "enginedemo_installer.nsi"
@"
OutFile \"$resolvedOut\"
InstallDir \"$PROGRAMFILES\\EngineDemo\"
Page directory
Page instfiles
Section
  SetOutPath \"$INSTDIR\"
  File /r \"$resolvedDistDir\\*\"
SectionEnd
"@ | Set-Content -Path $nsisScript

Write-Host "[build_installer] Building installer at $InstallerOut"
& $makensis.Source $nsisScript
if ($LASTEXITCODE -ne 0) {
    throw "makensis failed with exit code $LASTEXITCODE"
}

Write-Host "[build_installer] Done"
