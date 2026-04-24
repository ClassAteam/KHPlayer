#!/usr/bin/env bash
set -euo pipefail

cd /home/yuri/repo/video/build
cmake ..
cmake --build .

cd /home/yuri/repo/video/android

export JAVA_HOME=/opt/android-studio/jbr
export ANDROID_NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/29.0.14206865}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"

./gradlew externalNativeBuildDebug


LIBS_DIR=/home/yuri/repo/video/android/app/build/intermediates/cxx/Debug/3j444va2/obj/armeabi-v7a
BIN=/home/yuri/repo/video/android/app/build/intermediates/cxx/Debug/3j444va2/obj/armeabi-v7a/decoder_test


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

adb push $LIBS_DIR/libavcodec.so /data/local/tmp/
adb push $LIBS_DIR/libavformat.so /data/local/tmp/
adb push $LIBS_DIR/libavutil.so /data/local/tmp/
adb push $LIBS_DIR/libswresample.so /data/local/tmp/
adb push $LIBS_DIR/libSDL2.so /data/local/tmp/
adb push $BIN /data/local/tmp/

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SERVER_BIN="$ROOT/build/bin/video_server"
SERVER_ADDR="${1:-192.168.0.106:8080}"
SERVER_PORT="${SERVER_ADDR##*:}"
VIDEOS_DIR="${2:-$ROOT/fixtures}"
RPATH="$ROOT/third_party/ffmpeg/linux-x86_64"

# Kill any leftover process on the port
lsof -ti:"$SERVER_PORT" | xargs -r kill 2>/dev/null || true

# Start video server on host machine
if [[ ! -x "$SERVER_BIN" ]]; then
    echo "ERROR: VideoServer not built. Run: cmake --build build --target video_server"
    exit 1
fi

echo "==> Starting VideoServer on port $SERVER_PORT, serving: $VIDEOS_DIR"
LD_LIBRARY_PATH="$RPATH" "$SERVER_BIN" "$VIDEOS_DIR" "$SERVER_PORT" &
SERVER_PID=$!
sleep 0.5
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: VideoServer failed to start."
    exit 1
fi
echo "    VideoServer running (pid $SERVER_PID)"

# Capture MediaCodec/OMX logs while the test runs
adb logcat -c
adb logcat -s OMX MediaCodec ACodec 2>&1 &
LOGCAT_PID=$!

# Launching the test
adb shell "chmod +x /data/local/tmp/decoder_test && LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/decoder_test"

kill $LOGCAT_PID 2>/dev/null || true
