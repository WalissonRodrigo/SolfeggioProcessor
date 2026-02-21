# ============================================================
# Solfeggio Frequencies Plugin - Windows Setup & Build Script
# Run this in PowerShell (as Administrator recommended)
# ============================================================

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Solfeggio Frequencies - Windows Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# --- Configuration ---
$ProjectDir = $PSScriptRoot
if (-not $ProjectDir) {
    $ProjectDir = Get-Location
}

Write-Host "[INFO] Project directory: $ProjectDir" -ForegroundColor Green

# --- Check CMake ---
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "[ERROR] CMake not found! Installing via winget..." -ForegroundColor Red
    winget install Kitware.CMake --accept-package-agreements --accept-source-agreements
    $env:PATH += ";C:\Program Files\CMake\bin"
    Write-Host "[INFO] CMake installed. You may need to restart PowerShell." -ForegroundColor Yellow
}

# --- Check Visual Studio ---
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$hasVS = $false
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    if ($vsPath) {
        $hasVS = $true
        Write-Host "[OK] Visual Studio found: $vsPath" -ForegroundColor Green
    }
}

if (-not $hasVS) {
    Write-Host "[WARNING] Visual Studio not found." -ForegroundColor Yellow
    Write-Host "  Install Build Tools: winget install Microsoft.VisualStudio.2022.BuildTools" -ForegroundColor Yellow
    Write-Host "  Then add 'Desktop development with C++' workload." -ForegroundColor Yellow
}

# --- Build ---
Write-Host ""
Write-Host "[1/3] Configuring CMake..." -ForegroundColor Cyan

# Clean old build if exists
if (Test-Path "$ProjectDir\build") {
    Remove-Item -Recurse -Force "$ProjectDir\build"
}

Push-Location $ProjectDir
try {
    cmake -B build -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "[2/3] Building (Release)..." -ForegroundColor Cyan
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Build failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "[3/3] Build complete!" -ForegroundColor Green
    Write-Host ""

    # Find artifacts
    $vst3 = Get-ChildItem -Path build -Recurse -Filter "*.vst3" -Directory | Select-Object -First 1
    $standalone = Get-ChildItem -Path build -Recurse -Filter "Solfeggio Frequencies.exe" | Select-Object -First 1

    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host " Build Artifacts:" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    if ($vst3) {
        Write-Host "  VST3:       $($vst3.FullName)" -ForegroundColor Green
    }
    if ($standalone) {
        Write-Host "  Standalone: $($standalone.FullName)" -ForegroundColor Green
    }

    # Install VST3
    if ($vst3) {
        $vst3Dest = "C:\Program Files\Common Files\VST3"
        Write-Host ""
        $install = Read-Host "Install VST3 to '$vst3Dest'? (y/n)"
        if ($install -eq 'y') {
            Copy-Item -Recurse -Force $vst3.FullName "$vst3Dest\"
            Write-Host "[OK] VST3 installed!" -ForegroundColor Green
        }
    }

    # Run standalone
    if ($standalone) {
        Write-Host ""
        $run = Read-Host "Run Standalone app now? (y/n)"
        if ($run -eq 'y') {
            Start-Process $standalone.FullName
        }
    }
}
finally {
    Pop-Location
}
