# ESP32 Wi-Fi Scanner Monitor

An independent Python script for monitoring ESP32 Wi-Fi scan results over a serial port.
Does **not** require the ESP-IDF toolchain or `idf.py monitor` to run.

---

## Requirements

- Python 3.10+
- ESP32 flashed with firmware that outputs scan results as JSON over UART

### Python Dependencies 

```text
pyserial
rich
```

Install Python dependencies:
```bash
pip install -r requirements.txt
```

---

## Expected JSON Format

The monitor expects one JSON object per line from the ESP32, in the following format:

```json
{"ssid":"MyNetwork","bssid":"aa:bb:cc:dd:ee:ff","rssi":-67,"chan":6,"auth":"WPA2"}
```

Matching `printf` on the ESP32 side:
```c
printf("{\"ssid\":\"%s\",\"bssid\":\"%s\",\"rssi\":%d,\"chan\":%d,\"auth\":\"%s\"}\n",
        ssid, bssid_str, rssi, chan, auth);
```

Lines that are not valid JSON are silently ignored, so debug output from ESP-IDF will not cause any issues.

---

## Usage

```bash
# Auto-detect port (works if only one device is connected)
python wifi_monitor.py

# Specify port explicitly
python wifi_monitor.py COM3
python wifi_monitor.py /dev/ttyUSB0

# Specify port and baudrate
python wifi_monitor.py COM3 115200

# Set a custom refresh interval (in seconds)
python wifi_monitor.py COM3 115200 --refresh 0.5
python wifi_monitor.py COM3 115200 -r 2.0
```

### Arguments

| Argument | Default | Description |
|---|---|---|
| `port` | auto-detect | Serial port (e.g. `COM3`, `/dev/ttyUSB0`) |
| `baud` | `115200` | Baud rate |
| `--refresh / -r` | `1.0` | Table refresh interval in seconds |

---

## Features

- **Live table** - network list updates in place using `rich.live`, no terminal flickering
- **Signal strength** - RSSI displayed as both a numeric value and a visual bar, color-coded by quality
- **Auto-reconnect** - if the serial port disconnects, the monitor clears the network list, shows a warning, and retries every 3 seconds automatically
- **Auto port detection** - if only one serial device is connected, the port is selected without prompting
- **Graceful exit** - press `Ctrl+C` to stop

---

## Signal Quality Reference
| Bar | RSSI range | Quality |
|---|---|---|
| `████` | ≥ -50 dBm | Excellent |
| `███░` | -50 to -65 dBm | Good |
| `██░░` | -65 to -75 dBm | Fair |
| `█░░░` | -75 to -85 dBm | Weak |
| `░░░░` | < -85 dBm | Very weak |