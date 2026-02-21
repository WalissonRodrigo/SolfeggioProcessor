@echo off
REM ============================================================
REM Solfeggio Frequencies Plugin - Windows Build Script
REM Requires: CMake, Visual Studio 2022 (or Build Tools)
REM ============================================================

echo.
echo ========================================
echo  Solfeggio Frequencies - Windows Build
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found!
    echo.
    echo Install CMake from: https://cmake.org/download/
    echo Or via winget:  winget install Kitware.CMake
    echo Make sure to add CMake to PATH during installation.
    echo.
    pause
    exit /b 1
)

REM Check for Visual Studio / Build Tools
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [WARNING] MSVC compiler (cl.exe) not found in PATH.
    echo.
    echo If you have Visual Studio 2022 installed, run this script from:
    echo   "Developer Command Prompt for VS 2022"
    echo Or "x64 Native Tools Command Prompt for VS 2022"
    echo.
    echo If not installed, get Build Tools from:
    echo   https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo   Select "Desktop development with C++" workload.
    echo.
    echo Attempting build anyway (CMake may find the compiler)...
    echo.
)

echo [1/3] Configuring CMake...
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo Try running from "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)

echo.
echo [2/3] Building plugin (VST3 + Standalone)...
cmake --build build --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Build complete!
echo.
echo ========================================
echo  Build Artifacts:
echo ========================================

REM Find and display the built files
echo.
echo VST3 Plugin:
dir /s /b build\*Solfeggio*.vst3 2>nul
echo.
echo Standalone App:
dir /s /b build\*Solfeggio*.exe 2>nul
echo.

REM Ask to run standalone
set /p RUN_STANDALONE="Run Standalone app now? (y/n): "
if /i "%RUN_STANDALONE%"=="y" (
    echo.
    echo Starting Solfeggio Frequencies Standalone...
    for /f "delims=" %%i in ('dir /s /b build\*Solfeggio*.exe 2^>nul ^| findstr /i Standalone') do (
        start "" "%%i"
        goto :done
    )
    echo [ERROR] Standalone executable not found!
)

:done
echo.
echo To install VST3 plugin, copy the .vst3 folder to:
echo   C:\Program Files\Common Files\VST3\
echo.
pause
