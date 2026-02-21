#!/usr/bin/env bash
# ============================================================
#  Solfeggio Frequencies Plugin - macOS Build Script
#  Usage: ./build_macos.sh [clean] [run] [install]
# ============================================================

set -e

echo ""
echo "========================================"
echo " Solfeggio Frequencies - macOS Build"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# --- Check dependencies ---
if ! command -v cmake &>/dev/null; then
    echo "[ERROR] CMake not found!"
    echo "  Install: brew install cmake"
    exit 1
fi

if ! xcode-select -p &>/dev/null; then
    echo "[ERROR] Xcode Command Line Tools not found!"
    echo "  Install: xcode-select --install"
    exit 1
fi

# --- Detect architecture ---
ARCH=$(uname -m)
echo "[INFO] Architecture: $ARCH"

CMAKE_EXTRA_ARGS=""
if [ "$ARCH" = "arm64" ]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=arm64"
elif [ "$ARCH" = "x86_64" ]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=x86_64"
fi

# --- Clean ---
if [ "$1" = "clean" ] || [ ! -d build ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf build
fi

# --- Configure ---
echo ""
echo "[1/3] Configuring CMake..."
cmake -B build -DCMAKE_BUILD_TYPE=Release $CMAKE_EXTRA_ARGS -DCODESIGN_IDENTITY=-

# --- Build ---
echo ""
echo "[2/3] Building Release..."
cmake --build build --config Release --parallel "$(sysctl -n hw.ncpu)"

# --- Result ---
echo ""
echo "========================================"
echo " [3/3] Build Successful!"
echo "========================================"

VST3_PATH=$(find build -name "*.vst3" -type d 2>/dev/null | head -1)
AU_PATH=$(find build -name "*.component" -type d 2>/dev/null | head -1)
STANDALONE_PATH=$(find build -path "*/Standalone/*" -name "Solfeggio Frequencies" -type f 2>/dev/null | head -1)

echo ""
[ -n "$VST3_PATH" ]       && echo "  VST3:       $VST3_PATH"
[ -n "$AU_PATH" ]         && echo "  AU:         $AU_PATH"
[ -n "$STANDALONE_PATH" ] && echo "  Standalone: $STANDALONE_PATH"
echo ""

# --- Install plugins ---
if [ "$1" = "install" ] || [ "$2" = "install" ]; then
    if [ -n "$VST3_PATH" ]; then
        VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
        mkdir -p "$VST3_DEST"
        cp -R "$VST3_PATH" "$VST3_DEST/"
        echo "[OK] VST3 installed to $VST3_DEST"
    fi
    if [ -n "$AU_PATH" ]; then
        AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
        mkdir -p "$AU_DEST"
        cp -R "$AU_PATH" "$AU_DEST/"
        echo "[OK] AU installed to $AU_DEST"
    fi
fi

# --- Run Standalone ---
if [ "$1" = "run" ] || [ "$2" = "run" ]; then
    if [ -n "$STANDALONE_PATH" ]; then
        echo "[INFO] Launching Standalone..."
        open "$STANDALONE_PATH" 2>/dev/null || "$STANDALONE_PATH" &
    else
        echo "[ERROR] Standalone not found!"
    fi
fi
