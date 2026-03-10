#!/usr/bin/env bash
# Builds the Android APK and deploys it to a connected device via ADB.
#
# Requirements:
#   - Android SDK with build-tools installed
#   - ADB in PATH and device connected (USB or WiFi)
#   - third_party/ populated by scripts/setup-android-deps.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ANDROID_DIR="$ROOT/android"
APK="$ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
PACKAGE="com.simplevideoplayer"
ACTIVITY=".MainActivity"

# ── Check ADB ─────────────────────────────────────────────────────────────────
if ! command -v adb &>/dev/null; then
    echo "ERROR: adb not found. Install Android SDK platform-tools and add to PATH."
    exit 1
fi

if ! adb get-state &>/dev/null; then
    echo "ERROR: No Android device connected."
    echo "  USB: plug in the device with USB debugging enabled."
    echo "  WiFi: run 'adb connect <device-ip>:5555' first."
    exit 1
fi

echo "==> Device: $(adb devices | grep -v 'List of' | grep 'device$' | awk '{print $1}')"

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
echo "==> Building debug APK..."
./gradlew assembleDebug

# ── Deploy ────────────────────────────────────────────────────────────────────
echo "==> Installing APK..."
adb install -r "$APK"

# ── Launch ────────────────────────────────────────────────────────────────────
echo "==> Launching app..."
adb shell am start -n "${PACKAGE}/${ACTIVITY}"

echo ""
echo "==> Done. Stream logs with:"
echo "    adb logcat -s SDL,SimpleVideoPlayer"
