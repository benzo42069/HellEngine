Param(
    [string]$DistDir = "dist/portable",
    [string]$InstallerOut = "dist/EngineDemoInstaller.exe",
    [switch]$SkipIfToolingMissing
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path "$DistDir/EngineDemo.exe")) {
    Write-Host "[build_installer] Portable distribution missing. Running package_dist.ps1 first."
    & "$PSScriptRoot/package_dist.ps1"
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
"@ | Set-Content -Path "$InstallerOut.txt"
    exit 0
}

$nsisScript = Join-Path $env:TEMP "enginedemo_installer.nsi"
@"
OutFile \"$InstallerOut\"
InstallDir \"$PROGRAMFILES\\EngineDemo\"
Page directory
Page instfiles
Section
  SetOutPath \"$INSTDIR\"
  File /r \"$DistDir\\*\"
SectionEnd
"@ | Set-Content -Path $nsisScript

Write-Host "[build_installer] Building installer at $InstallerOut"
& $makensis.Source $nsisScript

Write-Host "[build_installer] Done"
