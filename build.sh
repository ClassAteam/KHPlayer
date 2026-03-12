set -euo pipefail

PROFILE=${1:-}

if [ ! -f "third_party/ffmpeg/linux-x86_64/libavcodec.so" ]; then
    bash scripts/setup-desktop-deps.sh
fi

if [ "$PROFILE" = "--profile" ]; then
    cmake -B build-prof -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" -DTRACY_ENABLE=OFF -DTRACY_PROFILER=OFF
    cmake --build build-prof
    perf record --call-graph dwarf ./build-prof/bin/SimpleVideoPlayer fixtures/sample.mp4
    perf report
else
    cmake -B build -S . -DTRACY_ENABLE=OFF -DTRACY_PROFILER=OFF
    cmake --build build
    ./build/bin/SimpleVideoPlayer fixtures/sample.mp4
fi
