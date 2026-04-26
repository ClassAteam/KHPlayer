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
elif [ "$PROFILE" = "--tracy" ]; then
    cmake -B build-tracy -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" -DTRACY_ENABLE=ON -DTRACY_PROFILER=ON
    cmake --build build-tracy
    ./build-tracy/bin/SimpleVideoPlayer fixtures/sample.mp4
elif [ "$PROFILE" = "--htop" ]; then
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DTRACY_ENABLE=OFF -DTRACY_PROFILER=OFF
    cmake --build build
    ./build/bin/SimpleVideoPlayer fixtures/sample.mp4 &
    PLAYER_PID=$!
    htop -p $PLAYER_PID
    wait $PLAYER_PID 2>/dev/null || true
else
    cmake -B build -S . -DTRACY_ENABLE=OFF -DTRACY_PROFILER=OFF
    cmake --build build
    ./build/bin/SimpleVideoPlayer fixtures/sample1.mp4
fi
