# miniscope-sender-simul.py
#
# Usage:
#   python3 miniscope-sender-simul.py [--backend qcharts|qcustomplot]
#
#   --backend qcharts       Use Qt Charts backend (default)
#   --backend qcustomplot   Use QCustomPlot backend (faster real-time rendering)
#
# What it does:
#   1. Builds the Qt app with the selected backend (cmake --build Qt-app/build).
#      Automatically cleans and reconfigures if the backend changed since last build.
#   2. Creates a virtual serial PTY pair via socat.
#   3. Launches the Qt app with the slave PTY path as a CLI argument,
#      so the port is pre-filled and acquisition starts automatically.
#   4. Streams two synthetic signals at 1 kHz over the master PTY:
#        CH0: 2 * sin(2π * 1 Hz * t)   — 2 V amplitude, 1 Hz
#        CH1: 0.01 * cos(2π * 5 Hz * t) — 10 mV amplitude, 5 Hz
#   5. Resets the time base (t0) whenever the Qt app sends "reset\n"
#      (which happens automatically on connect).
#   Ctrl+C stops the simulator, Qt app, and socat cleanly.
#
# Requirements:
#   pip install pyserial
#   apt install socat
#   cmake -B Qt-app/build Qt-app/   # run once to configure

import argparse
import serial
import time
import math
import re
import subprocess
import os
import threading
import shutil

parser = argparse.ArgumentParser(description="Miniscope simulator")
parser.add_argument(
    '--backend', choices=['qcharts', 'qcustomplot'], default=None,
    help='Plotting backend to build and launch (default: qcharts)'
)
cli = parser.parse_args()

if cli.backend is None:
    parser.print_help()
    print()
    cli.backend = 'qcharts'

use_qcp    = cli.backend == 'qcustomplot'
cmake_flag = f'-DUSE_QCUSTOMPLOT={"ON" if use_qcp else "OFF"}'

f1, f2 = 1, 5

script_dir = os.path.dirname(os.path.abspath(__file__))
source_dir = os.path.join(script_dir, "Qt-app")
build_dir  = os.path.join(script_dir, "Qt-app", "build")

def cached_backend_is_qcp(build_dir):
    """Read USE_QCUSTOMPLOT from CMakeCache.txt; returns None if not found."""
    cache = os.path.join(build_dir, "CMakeCache.txt")
    if not os.path.exists(cache):
        return None
    with open(cache) as f:
        for line in f:
            if line.startswith("USE_QCUSTOMPLOT:BOOL="):
                return line.strip().split("=", 1)[1].upper() == "ON"
    return None

needs_configure = not os.path.exists(os.path.join(build_dir, "build.ninja"))
backend_changed = cached_backend_is_qcp(build_dir) != use_qcp

if needs_configure or backend_changed:
    if backend_changed and not needs_configure:
        # AUTOMOC caches moc output per-build; must clean when switching backends
        print(f">>> Backend changed to {cli.backend} — cleaning build dir...")
        result = subprocess.run(
            ['cmake', '--build', build_dir, '--target', 'clean'],
            cwd=script_dir
        )

    if needs_configure:
        for stale in ["CMakeCache.txt", "CMakeFiles"]:
            path = os.path.join(build_dir, stale)
            if os.path.isfile(path):
                os.remove(path)
            elif os.path.isdir(path):
                shutil.rmtree(path)

    print(f">>> Configuring ({cli.backend})...")
    result = subprocess.run(
        ['cmake', '-B', build_dir, source_dir, cmake_flag],
        cwd=script_dir
    )
    if result.returncode != 0:
        raise SystemExit("Configure failed.")

print(f">>> Building ({cli.backend})...")
result = subprocess.run(
    ['cmake', '--build', build_dir, f'-j{os.cpu_count()}'],
    cwd=script_dir
)
if result.returncode != 0:
    raise SystemExit("Build failed.")
print(">>> Build OK")

socat = subprocess.Popen(
    ['socat', '-d', '-d', 'pty,raw,echo=0', 'pty,raw,echo=0'],
    stderr=subprocess.PIPE
)

ptys = []
while len(ptys) < 2:
    line = socat.stderr.readline().decode()
    m = re.search(r'/dev/pts/\d+', line)
    if m:
        ptys.append(m.group())

# drain socat's stderr so its pipe never fills and blocks its relay loop
threading.Thread(target=lambda: socat.stderr.read(), daemon=True).start()

pty_sim, pty_miniscope = ptys
print(f">>> Miniscope port: {pty_miniscope}")

qt_bin = os.path.join(build_dir, "miniscope")
qt_proc = subprocess.Popen([qt_bin, pty_miniscope])

t0 = time.time()
ser = serial.Serial(pty_sim, 115200, timeout=1)

try:
    while True:
        if ser.in_waiting > 0:
            cmd = ser.read(ser.in_waiting).decode('utf-8')
            print(f"Received command: {cmd.strip()}")
            if cmd == "reset\n":
                t0 = time.time()
        t = time.time() - t0
        v1 = 2 * math.sin(2 * math.pi * f1 * t)
        v2 = 0.01 * math.cos(2 * math.pi * f2 * t)
        ser.write(f'0 {t} {v1}\n'.encode('utf-8'))
        ser.write(f'1 {t} {v2}\n'.encode('utf-8'))
        time.sleep(0.001)
except KeyboardInterrupt:
    pass
finally:
    ser.close()
    qt_proc.terminate()
    socat.terminate()
