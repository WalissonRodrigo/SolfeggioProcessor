# ============================================================
#  Solfeggio Frequencies Plugin - Windows Build (PowerShell)
#  Usage: .\build_windows.ps1 [-Clean] [-Run] [-InstallVST3]
# ============================================================

param(
    [switch]$Clean,
    [switch]$Run,
    [switch]$InstallVST3
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Solfeggio Frequencies - Windows Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ProjectDir = $PSScriptRoot
if (-not $ProjectDir) { $ProjectDir = Get-Location }

# --- Check CMake ---
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] CMake not found!" -ForegroundColor Red
    Write-Host "  Install: winget install Kitware.CMake" -ForegroundColor Yellow
    exit 1
}

# --- Check Visual Studio ---
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    if ($vsPath) {
        Write-Host "[OK] Visual Studio: $vsPath" -ForegroundColor Green
    }
} else {
    Write-Host "[WARNING] Visual Studio not detected." -ForegroundColor Yellow
    Write-Host "  Install: winget install Microsoft.VisualStudio.2022.BuildTools" -ForegroundColor Yellow
}

# --- Clean ---
if ($Clean -or -not (Test-Path "$ProjectDir\build")) {
    if (Test-Path "$ProjectDir\build") {
        Write-Host "[INFO] Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "$ProjectDir\build"
    }
}

# --- Configure ---
Push-Location $ProjectDir
try {
    Write-Host ""
    Write-Host "[1/3] Configuring CMake..." -ForegroundColor Cyan
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        Write-Host "  Try: .\build_windows.ps1 -Clean" -ForegroundColor Yellow
        exit 1
    }

    Write-Host ""
    Write-Host "[2/3] Building Release..." -ForegroundColor Cyan
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Build failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "[3/3] Build Successful!" -ForegroundColor Green
    Write-Host ""

    # --- Find artifacts ---
    $vst3Dir = Get-ChildItem -Path build -Recurse -Filter "*.vst3" -Directory | Select-Object -First 1
    $standalone = Get-ChildItem -Path build -Recurse -Filter "Solfeggio Frequencies.exe" |
                  Where-Object { $_.DirectoryName -match "Standalone" } |
                  Select-Object -First 1

    Write-Host "========================================" -ForegroundColor Cyan
    if ($vst3Dir) { Write-Host "  VST3:       $($vst3Dir.FullName)" -ForegroundColor Green }
    if ($standalone) { Write-Host "  Standalone: $($standalone.FullName)" -ForegroundColor Green }
    Write-Host "========================================" -ForegroundColor Cyan

    # --- Install VST3 ---
    if ($InstallVST3 -and $vst3Dir) {
        $dest = "C:\Program Files\Common Files\VST3"
        Write-Host "[INFO] Installing VST3 to $dest ..." -ForegroundColor Cyan
        Copy-Item -Recurse -Force $vst3Dir.FullName "$dest\"
        Write-Host "[OK] VST3 installed!" -ForegroundColor Green
    }

    # --- Run Standalone ---
    if ($Run -and $standalone) {
        Write-Host "[INFO] Launching Standalone..." -ForegroundColor Cyan
        Start-Process $standalone.FullName
    }
}
finally {
    Pop-Location
}
