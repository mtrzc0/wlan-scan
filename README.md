# WLAN-scan project

Wi-Fi scanner project for the **ESP32-WROOM** module utilizing the official **Espressif IoT Development Framework (ESP-IDF)**.

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

## Data Structure 

The system relies on parsing the structural telemetry elements provided inside Espressif's native `wifi_ap_record_t` array buffer:

* **`.ssid`**: A character array containing the network name identifier (maximum capacity of 32 octets).
* **`.bssid`**: A 6-byte array holding the unique hardware MAC address of the routing transceiver.
* **`.rssi`**: Received Signal Strength Indicator measured in dBm (values closer to `0` denote increased signal intensity; e.g., `-45 dBm` indicates excellent signal, while `-85 dBm` signals marginal connectivity).
* **`.primary`**: Indicates the specific radio frequency slot channel managing transmission (typically Channels 1–13 within the standard 2.4GHz spectrum rules).
* **`.authmode`**: The internal enumeration defining the target authentication wrapper type (e.g., `WIFI_AUTH_WPA2_PSK`).

---

## Toolchain

Make sure that you are using latest version of the ESP-IDF toolchain. Follow installation and initialization procecure on official Espressif webpage.