# miniscope-sender-simul.py
#
# Usage:
#   python3 miniscope-sender-simul.py
#
# What it does:
#   1. Builds the Qt app (cmake --build Qt-app/build).
#   2. Creates a virtual serial PTY pair via socat.
#   3. Launches the Qt app with the slave PTY path as a CLI argument,
#      so the port is pre-filled and acquisition starts automatically.
#   4. Streams two synthetic signals at 1 kHz over the other PTY slave:
#        CH0: 2 * sin(2π * 1 Hz * t)   — 2 V amplitude, 1 Hz
#        CH1: 0.01 * cos(2π * 5 Hz * t) — 10 mV amplitude, 5 Hz
#   5. Resets the time base (t0) whenever the Qt app sends "reset\n"
#      (which happens automatically on connect).
#
# Requirements:
#   pip install pyserial
#   apt install socat
#   cmake -B Qt-app/build Qt-app/   # run once to configure

import serial
import time
import math
import re
import subprocess
import os
import threading

f1, f2 = 1, 5

script_dir = os.path.dirname(os.path.abspath(__file__))
build_dir = os.path.join(script_dir, "Qt-app", "build")

print(">>> Building...")
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
