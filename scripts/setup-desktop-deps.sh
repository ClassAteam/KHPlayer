#!/usr/bin/env bash
# Builds FFmpeg 7.1 from source for Linux x86_64 desktop.
# Run once before the first desktop build.
#
# Requirements:
#   - curl, make, yasm or nasm, gcc

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
THIRD_PARTY="$ROOT/third_party"
FFMPEG_VERSION="7.1"
ABI=linux-x86_64

FFMPEG_PREFIX="$THIRD_PARTY/ffmpeg"
FFMPEG_ABI_DIR="$FFMPEG_PREFIX/$ABI"

echo "==> Setting up desktop dependencies in $THIRD_PARTY"

mkdir -p "$THIRD_PARTY"

if [ -d "$FFMPEG_ABI_DIR" ] && [ -f "$FFMPEG_PREFIX/include/libavcodec/avcodec.h" ]; then
    echo "[FFmpeg] Already built, skipping."
else
    echo "[FFmpeg] Downloading FFmpeg $FFMPEG_VERSION..."
    curl -L "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz" \
        -o /tmp/ffmpeg.tar.gz
    tar -xf /tmp/ffmpeg.tar.gz -C /tmp
    rm /tmp/ffmpeg.tar.gz

    echo "[FFmpeg] Configuring and building for $ABI (this takes a few minutes)..."
    cd "/tmp/ffmpeg-${FFMPEG_VERSION}"
    ./configure \
        --prefix="$FFMPEG_PREFIX" \
        --libdir="$FFMPEG_ABI_DIR" \
        --enable-shared \
        --disable-static \
        --disable-programs \
        --disable-doc \
        --disable-avdevice \
        --disable-postproc \
        --disable-stripping
    make -j"$(nproc)"
    make install

    cd "$ROOT"
    rm -rf "/tmp/ffmpeg-${FFMPEG_VERSION}"
    echo "[FFmpeg] Done."
fi

echo ""
echo "==> Dependencies ready."
echo "    FFmpeg: $FFMPEG_PREFIX"
echo ""
echo "Next: run build.sh to build the player."
