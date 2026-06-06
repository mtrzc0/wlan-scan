# WLAN-scan project & ESP32 Wi-Fi Scanner Monitor

Wi-Fi scanner project for the **ESP32-WROOM** module utilizing the official **Espressif IoT Development Framework (ESP-IDF)**, paired with an independent Python script for monitoring ESP32 Wi-Fi scan results over a serial port.

Does **not** require the ESP-IDF toolchain to run the monitor script.

---

## Architecture

Configure the project target exclusively for the classic ESP32 architecture powering your ESP32-WROOM system module variant:

```bash
idf.py set-target esp3
```

1. **NVS Initialization:** The Wi-Fi driver relies on Non-Volatile Storage (NVS) to load internal RF calibration parameters and save state configurations.
2. **Event Loop:** Spawns the system dispatch engine responsible for handling internal Wi-Fi state machine updates.
3. **Wi-Fi Driver Setup:** Provisions the RF hardware into Station (`WIFI_MODE_STA`) client mode.
4. **Active Channel Sweep:** Sequentially steps through available 2.4GHz channels (`1` to `13` or `14`) using an active scan configuration.
5. **Telemetry Extraction:** Captures the hardware-populated structure arrays into organized json format.

---

## Data Structure & Expected JSON Format

The system relies on parsing the structural telemetry elements provided inside Espressif's native `wifi_ap_record_t` array buffer:

* **`.ssid`**: A character array containing the network name identifier (maximum capacity of 32 octets).
* **`.bssid`**: A 6-byte array holding the unique hardware MAC address of the routing transceiver.
* **`.rssi`**: Received Signal Strength Indicator measured in dBm (values closer to `0` denote increased signal intensity; e.g., `-45 dBm` indicates excellent signal, while `-85 dBm` signals marginal connectivity).
* **`.primary`**: Indicates the specific radio frequency slot channel managing transmission (typically Channels 1–13 within the standard 2.4GHz spectrum rules).
* **`.authmode`**: The internal enumeration defining the target authentication wrapper type (e.g., `WIFI_AUTH_WPA2_PSK`).

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

## Toolchain & Requirements

### ESP32 Application

Make sure that you are using latest version of the ESP-IDF toolchain. Follow installation and initialization procecure on official Espressif webpage.

### Python Monitor

* Python 3.10+
* ESP32 flashed with firmware that outputs scan results as JSON over UART

#### Python Dependencies

```text
pyserial
rich
```

Install Python dependencies:

```bash
pip install -r requirements.txt
```

---

## Monitor script Usage

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
| --- | --- | --- |
| `port` | auto-detect | Serial port (e.g. `COM3`, `/dev/ttyUSB0`) |
| `baud` | `115200` | Baud rate |
| `--refresh / -r` | `1.0` | Table refresh interval in seconds |

---

## Features

* **Live table** - network list updates in place using `rich.live`, no terminal flickering
* **Signal strength** - RSSI displayed as both a numeric value and a visual bar, color-coded by quality
* **Auto-reconnect** - if the serial port disconnects, the monitor clears the network list, shows a warning, and retries every 3 seconds automatically
* **Auto port detection** - if only one serial device is connected, the port is selected without prompting
* **Graceful exit** - press `Ctrl+C` to stop

---

## Signal Quality Reference

| Bar | RSSI range | Quality |
| --- | --- | --- |
| `████` | ≥ -50 dBm | Excellent |
| `███░` | -50 to -65 dBm | Good |
| `██░░` | -65 to -75 dBm | Fair |
| `█░░░` | -75 to -85 dBm | Weak |
| `░░░░` | < -85 dBm | Very weak |
