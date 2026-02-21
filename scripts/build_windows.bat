@echo off
REM ============================================================
REM  Solfeggio Frequencies Plugin - Windows Build Script (CMD)
REM  Usage: build_windows.bat [clean]
REM  Requires: CMake, Visual Studio 2022 Build Tools
REM ============================================================

echo.
echo ========================================
echo  Solfeggio Frequencies - Windows Build
echo ========================================
echo.

REM --- Check CMake ---
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found!
    echo   Install: winget install Kitware.CMake
    echo   Restart your terminal after installation.
    pause
    exit /b 1
)

REM --- Check cl.exe ---
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [WARNING] MSVC compiler ^(cl.exe^) not in PATH.
    echo   If build fails, run from "x64 Native Tools Command Prompt for VS 2022"
    echo.
)

REM --- Clean build if requested or build folder has errors ---
if "%1"=="clean" (
    echo [INFO] Cleaning build directory...
    if exist build rmdir /s /q build
)

REM --- Always clean if build folder exists with stale cache ---
if exist build\CMakeCache.txt (
    findstr /c:"14.43.34808" build\CMakeCache.txt >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo [INFO] Stale MSVC cache detected, cleaning build...
        rmdir /s /q build
    )
)

REM --- Configure ---
echo [1/3] Configuring CMake...
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo   Try: rmdir /s /q build
    echo   Then run this script again.
    pause
    exit /b 1
)

REM --- Build ---
echo.
echo [2/3] Building Release (VST3 + Standalone)...
cmake --build build --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

REM --- Result ---
echo.
echo ========================================
echo  [3/3] Build Successful!
echo ========================================
echo.
echo   VST3:
dir /s /b build\*Solfeggio*.vst3 2>nul
echo.
echo   Standalone:
dir /s /b "build\*Solfeggio Frequencies.exe" 2>nul | findstr /i Standalone
echo.

REM --- Run Standalone ---
set /p RUN="Run Standalone now? (y/n): "
if /i "%RUN%"=="y" (
    for /f "delims=" %%i in ('dir /s /b "build\*Solfeggio Frequencies.exe" 2^>nul ^| findstr /i Standalone') do (
        start "" "%%i"
        goto :end
    )
    echo [ERROR] Standalone .exe not found!
)

:end
echo.
echo To install VST3, copy the .vst3 folder to:
echo   C:\Program Files\Common Files\VST3\
echo.
pause
