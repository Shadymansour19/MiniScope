import serial
import time
import math
import sys

f1, f2 = 1, 5
t0 = time.time()
ser = serial.Serial(f'/dev/pts/{sys.argv[1]}', 115200, timeout=1)

while True:
    if ser.in_waiting > 0:
        cmd = ser.read(ser.in_waiting).decode('utf-8')
        if cmd == "reset\n":
            t0 = time.time()
    t = time.time() - t0
    #print(t)
    v1 = 2 * math.sin(2 * math.pi * f1 * t)
    v2 = 0.01 * math.cos(2 * math.pi * f2 * t)
    ser.write(f'0 {t} {v1}\n'.encode("utf-8"))
    ser.write(f'1 {t} {v2}\n'.encode("utf-8"))
    time.sleep(0.001)

