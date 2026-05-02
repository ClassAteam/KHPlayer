#!/usr/bin/env python3
import atexit
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ANDROID = os.path.join(ROOT, "android")
APK = os.path.join(
    ANDROID, "integration-test/build/outputs/apk/debug/integration-test-debug.apk"
)
PACKAGE = "com.simplevideoplayer.integrationtest"
# TODO this ugly hardcoding will go to device discoverability
# functionality for our clients and servers on the same network
TV_ADDR = "192.168.0.101:5555"
SERVER_ADDR = "192.168.0.106:8080"
SERVER_BIN = os.path.join(ROOT, "build/bin/video_server")
FFMPEG_LIB = os.path.join(ROOT, "third_party/ffmpeg/linux-x86_64")

env = os.environ.copy()
env.setdefault("JAVA_HOME", "/opt/android-studio/jbr")
env.setdefault("ANDROID_HOME", os.path.expanduser("~/Android/Sdk"))
env.setdefault("ANDROID_NDK", os.path.expanduser("~/Android/Sdk/ndk/29.0.14206865"))

server_proc = None
logcat_proc = None


def cleanup():
    if logcat_proc:
        logcat_proc.terminate()
    if server_proc:
        print(f"==> Stopping VideoServer (pid {server_proc.pid})...")
        server_proc.terminate()
        server_proc.wait()


atexit.register(cleanup)


def run(cmd, **kw):
    subprocess.run(cmd, check=True, **kw)


def adb(*args):
    run(["adb", "-s", TV_ADDR, *args])


# ── VideoServer ───────────────────────────────────────────────────────────────
server_env = {**env, "LD_LIBRARY_PATH": FFMPEG_LIB}
server_proc = subprocess.Popen(
    [SERVER_BIN, os.path.join(ROOT, "fixtures"), "8080"], env=server_env
)
time.sleep(0.5)
if server_proc.poll() is not None:
    sys.exit("ERROR: VideoServer failed to start")
print(f"==> VideoServer running (pid {server_proc.pid})")

# ── ADB ───────────────────────────────────────────────────────────────────────
if (
    TV_ADDR
    not in subprocess.run(
        ["adb", "devices"], capture_output=True, text=True, check=True
    ).stdout
):
    run(["adb", "connect", TV_ADDR])
    time.sleep(1)

# ── Build ─────────────────────────────────────────────────────────────────────
os.chdir(ANDROID)
run(["./gradlew", "clean", ":integration-test:assembleDebug"], env=env)

# ── Deploy ────────────────────────────────────────────────────────────────────
adb("shell", "am", "force-stop", PACKAGE)
adb("install", "-r", APK)
adb("shell", "pm", "grant", PACKAGE, "android.permission.READ_EXTERNAL_STORAGE")

with open("/tmp/svp_server.txt", "w") as f:
    f.write(SERVER_ADDR)
adb("shell", f"mkdir -p /sdcard/Android/data/{PACKAGE}/files")
adb(
    "push",
    "/tmp/svp_server.txt",
    f"/sdcard/Android/data/{PACKAGE}/files/svp_server.txt",
)

# ── Launch ────────────────────────────────────────────────────────────────────
adb("logcat", "-c")
adb("shell", "am", "start", "-n", f"{PACKAGE}/.MainActivity")
time.sleep(2)

pid = subprocess.run(
    ["adb", "-s", TV_ADDR, "shell", "pidof", PACKAGE],
    capture_output=True,
    text=True,
    # if the app already crashed, pidof returns exit 1 — we still want
    # to tail logcat to see why
    check=False,
).stdout.strip()
print(f"==> App launched (pid {pid or 'unknown — may have crashed'}). Tailing logs (Ctrl+C to stop):")

logcat_cmd = ["adb", "-s", TV_ADDR, "logcat"]
if pid:
    logcat_cmd += ["--pid", pid]
logcat_cmd += ["-s", "SimpleVideoPlayer:V", "AndroidRuntime:E", "System.err:W", "DEBUG:E"]
logcat_proc = subprocess.Popen(logcat_cmd, stdout=subprocess.PIPE, text=True)

exit_code = 0
for line in logcat_proc.stdout:
    print(line, end="", flush=True)
    if "INTEGRATION TEST PASSED" in line:
        print("==> Integration test passed.")
        break
    if "INTEGRATION TEST FAILED" in line:
        print("==> Integration test FAILED.")
        exit_code = 1
        break

logcat_proc.terminate()
sys.exit(exit_code)
