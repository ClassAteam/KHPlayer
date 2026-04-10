#!/usr/bin/env bash
# Builds the Android APK, starts the VideoServer on this machine, and deploys to TV.
#
# Usage: deploy-android.sh [server-address] [videos-directory]
#   server-address    Host:port this machine is reachable at from the TV.
#                     Default: 192.168.0.106:8080
#   videos-directory  Directory served by VideoServer. Default: fixtures/
#
# Requirements:
#   - Android SDK with build-tools installed
#   - ADB in PATH and device connected (USB or WiFi)
#   - third_party/ populated by scripts/setup-android-deps.sh

set -euo pipefail

export JAVA_HOME="${JAVA_HOME:-/opt/android-studio/jbr}"
export ANDROID_NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/29.0.14206865}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ANDROID_DIR="$ROOT/android"
APK="$ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
PACKAGE="com.simplevideoplayer"
ACTIVITY=".MainActivity"
SERVER_ADDR="${1:-192.168.0.106:8080}"
VIDEOS_DIR="${2:-$ROOT/fixtures}"
SERVER_BIN="$ROOT/build/bin/VideoServer"
RPATH="$ROOT/third_party/ffmpeg/linux-x86_64"
SERVER_PORT="${SERVER_ADDR##*:}"
SERVER_PID=""
LOGCAT_PID=""

cleanup() {
    [[ -n "$LOGCAT_PID" ]] && kill "$LOGCAT_PID" 2>/dev/null || true
    if [[ -n "$SERVER_PID" ]]; then
        echo "==> Stopping VideoServer (pid $SERVER_PID)..."
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# ── Start VideoServer ─────────────────────────────────────────────────────────
if [[ ! -x "$SERVER_BIN" ]]; then
    echo "ERROR: VideoServer not built. Run: cmake --build build --target VideoServer"
    exit 1
fi
echo "==> Starting VideoServer on port $SERVER_PORT, serving: $VIDEOS_DIR"
LD_LIBRARY_PATH="$RPATH" "$SERVER_BIN" "$VIDEOS_DIR" "$SERVER_PORT" &>/dev/null &
SERVER_PID=$!
sleep 0.5
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: VideoServer failed to start."
    exit 1
fi
echo "    VideoServer running (pid $SERVER_PID)"

# ── Check ADB ─────────────────────────────────────────────────────────────────
if ! command -v adb &>/dev/null; then
    echo "ERROR: adb not found. Install Android SDK platform-tools and add to PATH."
    exit 1
fi

# Auto-connect to TV over WiFi if not already connected
TV_ADDR="192.168.0.101:5555"
if ! adb devices | grep -q "$TV_ADDR"; then
    echo "==> Connecting to TV at $TV_ADDR..."
    adb connect "$TV_ADDR"
    sleep 1
fi

if ! adb -s "$TV_ADDR" get-state &>/dev/null; then
    echo "ERROR: Could not reach TV at $TV_ADDR."
    exit 1
fi

ADB="adb -s $TV_ADDR"
echo "==> Device: $TV_ADDR"

# ── Bootstrap Gradle wrapper if needed ────────────────────────────────────────
cd "$ANDROID_DIR"
if [ ! -f "gradlew" ]; then
    echo "==> Bootstrapping Gradle wrapper..."
    if command -v gradle &>/dev/null; then
        gradle wrapper --gradle-version 8.6
    else
        echo "ERROR: gradlew not found and gradle not in PATH."
        echo "  Install Gradle or run 'gradle wrapper' in android/ manually."
        exit 1
    fi
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo "==> Cleaning native build cache..."
./gradlew cleanExternalNativeBuildDebug
echo "==> Building debug APK..."
./gradlew assembleDebug

# ── Deploy ────────────────────────────────────────────────────────────────────
echo "==> Force-stopping old instance..."
$ADB shell am force-stop "$PACKAGE" || true

echo "==> Installing APK..."
$ADB install -r "$APK"

echo "==> Granting storage permission..."
$ADB shell pm grant "$PACKAGE" android.permission.READ_EXTERNAL_STORAGE

echo "==> Writing server address ($SERVER_ADDR) to device..."
echo "$SERVER_ADDR" > /tmp/svp_server.txt
$ADB shell "mkdir -p /sdcard/Android/data/com.simplevideoplayer/files"
$ADB push /tmp/svp_server.txt /sdcard/Android/data/com.simplevideoplayer/files/svp_server.txt

# ── Launch ────────────────────────────────────────────────────────────────────
echo "==> Launching app..."
$ADB shell am start -n "${PACKAGE}/${ACTIVITY}"

echo ""
echo "==> App launched. VideoServer is running — use the TV remote to pick a file."
echo "    Tailing logs (Ctrl+C stops server and exits):"
echo ""

# Tail logs and keep server alive until the user interrupts
$ADB logcat -s SDL:V SimpleVideoPlayer:V &
LOGCAT_PID=$!
wait "$SERVER_PID" 2>/dev/null || true
SERVER_PID=""
