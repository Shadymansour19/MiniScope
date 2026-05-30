# Miniscope — Portable 2-Channel USB Oscilloscope

A real-time, dual-channel oscilloscope desktop application built with Qt 6. Receives analog sample data from an STM32F1 microcontroller over USB serial, displays live waveforms, and computes signal characteristics (amplitude, frequency, DC offset, period) via FFT.

---

## Features

- 2-channel real-time waveform display (Qt Charts, 33 Hz refresh)
- Per-channel FFT analysis with Hann windowing
- SI-prefix-aware controls (n, µ, m, k, M, G) for voltage and time
- Pause/browse mode — scrub through buffered history (up to 50 000 points/channel)
- Auto-scale — sets Y-range and time window from detected signal period
- Save / Load waveforms to/from plain-text files
- Python serial simulator for development without hardware

---

## Requirements

| Component | Version |
|-----------|---------|
| Qt | 5 or 6 (Widgets + Charts + SerialPort modules) |
| CMake | ≥ 3.5 |
| C++ | 17 |
| OS | Linux / macOS / Windows |

On Ubuntu/Debian:
```bash
sudo apt install qt6-base-dev qt6-charts-dev libqt6serialport6-dev cmake build-essential
```

---

## Build

```bash
cd Qt-app
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Binary: `build/miniscope`

---

## Running

### With real hardware (STM32)

1. Flash the STM32 firmware (see `STM32/` directory).
2. Connect the board via USB.
3. Launch `./build/miniscope`.
4. Enter the serial port (e.g. `/dev/ttyUSB0` or `/dev/ttyACM0`) in the port field.
5. Click **Play**.

### With the Python simulator (no hardware needed)

The simulator creates a virtual serial port and streams two test signals:

- **CH0**: 2 V × sin(2π × 1 Hz × t)
- **CH1**: 0.01 V × cos(2π × 5 Hz × t)

```bash
# In one terminal — creates /dev/pts/N (prints N on stdout)
python3 miniscope-sender-simul.py <pts_number>

# Example: open /dev/pts/2 in the app
python3 miniscope-sender-simul.py 2
```

Then set the serial port field to `/dev/pts/2` and press **Play**.

> The simulator requires `pyserial`: `pip install pyserial`

---

## UI Overview

```
┌──────────────────────────┬────────────────────────┐
│                          │ /dev/ttyUSB0           │
│                          │ [▶] [⏸] [⟳] [💾] [📂] │
│      Waveform Display    │ Time Range: 100 ms     │
│      (QChartView)        │ Time Pos:   0 s        │
│                          │ ┌──CH1──┬──CH2──┐      │
│                          │ │ Rng   │ Rng   │      │
│                          │ │ Pos   │ Pos   │      │
│                          │ │ ──    │ ──    │      │
│                          │ │ Amp   │ Amp   │      │
│                          │ │ DC    │ DC    │      │
│                          │ │ Freq  │ Freq  │      │
│                          │ │ Per   │ Per   │      │
│                          │ └───────┴───────┘      │
└──────────────────────────┴────────────────────────┘
```

**Buttons:**
| Button | Shortcut | Action |
|--------|----------|--------|
| Play / Stop | — | Start or stop serial acquisition |
| Pause / Resume | — | Freeze/unfreeze the display |
| Refresh | — | Redraw plot while paused |
| Auto | — | Auto-scale both channels |
| Save | Ctrl+S | Save recorded data to file |
| Load | Ctrl+O | Load data from file |

---

## Serial Protocol

The STM32 firmware (and simulator) emits newline-delimited ASCII:

```
<channel> <timestamp_seconds> <voltage>\n
```

Example:
```
0 1.234567 0.512345
1 1.234567 -0.001234
0 1.235567 1.523456
```

- `channel`: 0 or 1
- `timestamp_seconds`: floating-point seconds since reset
- `voltage`: floating-point volts

The app sends `reset\n` to the device on connection; the device resets its timestamp counter in response.

---

## Save / Load File Format

Same format as the serial protocol:

```
<channel> <voltage> <timestamp>
```

Note: column order is `channel value time` (voltage before timestamp, unlike the wire format which is `channel time value`).

---

## Project Structure

```
miniscope/
├── Qt-app/
│   ├── main.cpp                    # Entry point
│   ├── mainwindow.h/cpp            # Main window, state machine, orchestration
│   ├── channel.h/cpp               # Per-channel data, FFT, display
│   ├── serialReader.h/cpp          # Threaded USB serial I/O
│   ├── qLabeledUnitedSpinBox.h/cpp # Custom SI-prefix spinbox widget
│   ├── qUnitedAxis.h/cpp           # Custom Qt Charts axis with units
│   ├── resources.qrc               # Bundled icons
│   └── CMakeLists.txt
├── STM32/                          # Embedded firmware (STM32F1 HAL + USB CDC)
└── miniscope-sender-simul.py       # Python test signal generator
```

---

## Known Issues / Notes

- Enum typo: `MainWindow::stoped` (one `p`) — should be `stopped` in a future cleanup.
- `dialog.ui` is a legacy Qt Designer file not used by the current build.
- The Qt5 CMake branch references `qLabeledUnitedDial` and `serialthread` (old names); only the Qt6 branch is actively maintained.
- Serial port field defaults to `/dev/pts/2` in `SerialReader`; change before building for production hardware use.
