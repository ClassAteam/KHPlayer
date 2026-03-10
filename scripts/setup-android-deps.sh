#!/usr/bin/env bash
# Sets up third_party/ with SDL2 source and FFmpeg built from source for Android.
# Run once before the first Android build.
#
# Requirements:
#   - curl, make, yasm or nasm
#   - ANDROID_NDK must be set (e.g. export ANDROID_NDK=~/Android/Sdk/ndk/29.0.14206865)

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
THIRD_PARTY="$ROOT/third_party"
SDL2_VERSION="2.32.2"
FFMPEG_VERSION="7.1"
ANDROID_API=21
ABI=armeabi-v7a

echo "==> Setting up Android dependencies in $THIRD_PARTY"

if [ -z "${ANDROID_NDK:-}" ]; then
    echo "ERROR: ANDROID_NDK is not set."
    echo "  export ANDROID_NDK=~/Android/Sdk/ndk/<version>"
    exit 1
fi

mkdir -p "$THIRD_PARTY"

# ── SDL2 ─────────────────────────────────────────────────────────────────────
SDL2_DIR="$THIRD_PARTY/SDL2"
if [ -d "$SDL2_DIR" ]; then
    echo "[SDL2] Already present, skipping."
else
    echo "[SDL2] Downloading SDL2 $SDL2_VERSION..."
    curl -L "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-${SDL2_VERSION}.tar.gz" \
        -o /tmp/SDL2.tar.gz
    tar -xf /tmp/SDL2.tar.gz -C "$THIRD_PARTY"
    mv "$THIRD_PARTY/SDL2-${SDL2_VERSION}" "$SDL2_DIR"
    rm /tmp/SDL2.tar.gz
    echo "[SDL2] Done."
fi

# Copy SDL2 Java sources into the Android project
SDL2_JAVA_SRC="$SDL2_DIR/android-project/app/src/main/java/org/libsdl/app"
SDL2_JAVA_DST="$ROOT/android/app/src/main/java/org/libsdl/app"
if [ -d "$SDL2_JAVA_DST" ]; then
    echo "[SDL2 Java] Already copied, skipping."
else
    echo "[SDL2 Java] Copying Java sources..."
    mkdir -p "$SDL2_JAVA_DST"
    cp "$SDL2_JAVA_SRC"/*.java "$SDL2_JAVA_DST/"
    echo "[SDL2 Java] Done."
fi

# ── FFmpeg (built from source with NDK) ───────────────────────────────────────
FFMPEG_PREFIX="$THIRD_PARTY/ffmpeg"
FFMPEG_ABI_DIR="$FFMPEG_PREFIX/$ABI"

if [ -d "$FFMPEG_ABI_DIR" ] && [ -f "$FFMPEG_PREFIX/include/libavcodec/avcodec.h" ]; then
    echo "[FFmpeg] Already built, skipping."
else
    TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64"
    CC="$TOOLCHAIN/bin/armv7a-linux-androideabi${ANDROID_API}-clang"
    CXX="$TOOLCHAIN/bin/armv7a-linux-androideabi${ANDROID_API}-clang++"

    echo "[FFmpeg] Downloading FFmpeg $FFMPEG_VERSION..."
    curl -L "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz" \
        -o /tmp/ffmpeg.tar.gz
    tar -xf /tmp/ffmpeg.tar.gz -C /tmp
    rm /tmp/ffmpeg.tar.gz

    echo "[FFmpeg] Configuring and building for $ABI (this takes a few minutes)..."

    # Write build script to file — avoids variable expansion issues with inline heredocs
    BUILD_SCRIPT="/tmp/build-ffmpeg-android.sh"
    cat > "$BUILD_SCRIPT" <<SCRIPT
#!/usr/bin/env bash
set -euo pipefail
cd "/tmp/ffmpeg-${FFMPEG_VERSION}"
./configure \\
    --prefix="${FFMPEG_PREFIX}" \\
    --libdir="${FFMPEG_ABI_DIR}" \\
    --enable-shared \\
    --disable-static \\
    --target-os=android \\
    --arch=arm \\
    --cpu=armv7-a \\
    --cc="${CC}" \\
    --cxx="${CXX}" \\
    --enable-cross-compile \\
    --sysroot="${TOOLCHAIN}/sysroot" \\
    --disable-programs \\
    --disable-doc \\
    --disable-avdevice \\
    --disable-postproc \\
    --disable-network \\
    --disable-stripping \\
    --enable-small
make -j\$(nproc)
make install
SCRIPT
    chmod +x "$BUILD_SCRIPT"
    bash "$BUILD_SCRIPT"
    rm "$BUILD_SCRIPT"

    cd "$ROOT"
    rm -rf "/tmp/ffmpeg-${FFMPEG_VERSION}"
    echo "[FFmpeg] Done."
fi

echo ""
echo "==> Dependencies ready."
echo "    SDL2:   $SDL2_DIR"
echo "    FFmpeg: $FFMPEG_PREFIX"
echo ""
echo "Next: run scripts/deploy-android.sh to build and deploy the APK."
