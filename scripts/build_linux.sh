#!/usr/bin/env bash
# ============================================================
#  Solfeggio Frequencies Plugin - Linux Build Script
#  Usage: ./build_linux.sh [clean] [run] [install]
# ============================================================

set -e

echo ""
echo "========================================"
echo " Solfeggio Frequencies - Linux Build"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# --- Check dependencies ---
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "[ERROR] $1 not found!"
        echo "  Install: $2"
        exit 1
    fi
}

check_dep cmake "sudo apt install cmake"
check_dep g++   "sudo apt install build-essential"

# --- Check JUCE Linux dependencies ---
MISSING_DEPS=""
for pkg in libasound2-dev libjack-jackd2-dev libfreetype6-dev libx11-dev \
           libxrandr-dev libxinerama-dev libxcursor-dev libgtk-3-dev \
           libwebkit2gtk-4.1-dev libcurl4-openssl-dev pkg-config; do
    if ! dpkg -s "$pkg" &>/dev/null 2>&1; then
        MISSING_DEPS="$MISSING_DEPS $pkg"
    fi
done

if [ -n "$MISSING_DEPS" ]; then
    echo "[INFO] Missing dependencies: $MISSING_DEPS"
    echo "[WARN] Please install them manually if build fails."
    # sudo apt-get update
    # sudo apt-get install -y $MISSING_DEPS
fi

# --- Clean ---
if [ "$1" = "clean" ] || [ ! -d build ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf build
fi

# --- Configure ---
echo ""
echo "[1/3] Configuring CMake..."
cmake -B build -S .. -DCMAKE_BUILD_TYPE=Release

# --- Build ---
echo ""
echo "[2/3] Building Release..."
cmake --build build --config Release --parallel "$(nproc)"

# --- Result ---
echo ""
echo "========================================"
echo " [3/3] Build Successful!"
echo "========================================"

VST3_PATH=$(find build -name "*.vst3" -type d 2>/dev/null | head -1)
STANDALONE_PATH=$(find build -path "*/Standalone/*" -name "Solfeggio Frequencies" -type f 2>/dev/null | head -1)

echo ""
[ -n "$VST3_PATH" ]       && echo "  VST3:       $VST3_PATH"
[ -n "$STANDALONE_PATH" ] && echo "  Standalone: $STANDALONE_PATH"
echo ""

# --- Install VST3 ---
if [ "$1" = "install" ] || [ "$2" = "install" ]; then
    VST3_DEST="$HOME/.vst3"
    mkdir -p "$VST3_DEST"
    if [ -n "$VST3_PATH" ]; then
        cp -r "$VST3_PATH" "$VST3_DEST/"
        echo "[OK] VST3 installed to $VST3_DEST"
    fi
fi

# --- Run Standalone ---
if [ "$1" = "run" ] || [ "$2" = "run" ]; then
    if [ -n "$STANDALONE_PATH" ]; then
        echo "[INFO] Launching Standalone..."
        "$STANDALONE_PATH" &
    else
        echo "[ERROR] Standalone not found!"
    fi
fi
