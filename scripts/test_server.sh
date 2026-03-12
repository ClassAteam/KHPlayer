#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
SERVER="$ROOT/build/bin/VideoServer"
FIXTURES="$ROOT/fixtures"
PORT=18081
BASE="http://localhost:$PORT"
RPATH="$ROOT/third_party/ffmpeg/linux-x86_64"
PASS=0
FAIL=0

ok()      { echo "  PASS: $*"; PASS=$((PASS + 1)); }
fail()    { echo "  FAIL: $*"; FAIL=$((FAIL + 1)); }
cleanup() { kill "$SERVER_PID" 2>/dev/null; wait "$SERVER_PID" 2>/dev/null || true; }

if [[ ! -x "$SERVER" ]]; then
    echo "VideoServer not built. Run: cmake --build build --target VideoServer"
    exit 1
fi

echo "--- Starting VideoServer on port $PORT ---"
LD_LIBRARY_PATH="$RPATH" "$SERVER" "$FIXTURES" "$PORT" &>/dev/null &
SERVER_PID=$!
trap cleanup EXIT
sleep 0.4

# 1. /files returns JSON array
echo
echo "Test 1: GET /files"
RESPONSE=$(curl -sf "$BASE/files" || true)
if echo "$RESPONSE" | grep -qE '^\[.*\.mp4.*\]$'; then
    ok "/files returned JSON: $RESPONSE"
else
    fail "/files unexpected response: '$RESPONSE'"
fi

# 2. /files lists sample.mp4
echo
echo "Test 2: sample.mp4 appears in /files"
if echo "$RESPONSE" | grep -q "sample.mp4"; then
    ok "sample.mp4 found in listing"
else
    fail "sample.mp4 not found in: $RESPONSE"
fi

# 3. /stream?file=sample.mp4 is a valid media stream (ffprobe detects video+audio)
echo
echo "Test 3: /stream?file=sample.mp4 is a valid media stream"
if command -v ffprobe &>/dev/null; then
    PROBE=$(LD_LIBRARY_PATH="$RPATH" ffprobe -v error \
        -show_entries stream=codec_type \
        -of default=noprint_wrappers=1 \
        "$BASE/stream?file=sample.mp4" 2>&1 || true)
    if echo "$PROBE" | grep -q "codec_type=video"; then
        ok "ffprobe detected video stream"
    else
        fail "ffprobe did not detect video stream. Output: $PROBE"
    fi
    if echo "$PROBE" | grep -q "codec_type=audio"; then
        ok "ffprobe detected audio stream"
    else
        fail "ffprobe did not detect audio stream"
    fi
else
    # Fallback: check MPEG-TS sync byte (0x47) at offsets 0, 188, 376
    RAW=$(curl -sf --max-time 5 "$BASE/stream?file=sample.mp4" 2>/dev/null | head -c 600 | od -An -tx1 | tr -d ' \n')
    B0=${RAW:0:2}; B188=${RAW:376:2}; B376=${RAW:752:2}
    if [[ "$B0" == "47" && "$B188" == "47" && "$B376" == "47" ]]; then
        ok "MPEG-TS sync bytes verified (ffprobe not available)"
    else
        fail "MPEG-TS sync byte check failed (bytes: $B0 $B188 $B376)"
    fi
fi

# 4. Path traversal is blocked
echo
echo "Test 4: path traversal blocked"
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/stream?file=../CMakeLists.txt")
if [[ "$STATUS" == "404" ]]; then
    ok "path traversal returned 404"
else
    fail "path traversal returned $STATUS (expected 404)"
fi

# 5. Unknown path returns 404
echo
echo "Test 5: unknown path returns 404"
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/unknown")
if [[ "$STATUS" == "404" ]]; then
    ok "unknown path returned 404"
else
    fail "unknown path returned $STATUS (expected 404)"
fi

echo
echo "--- Results: $PASS passed, $FAIL failed ---"
[[ $FAIL -eq 0 ]]
