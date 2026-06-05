# WLAN-scan project

Wi-Fi scanner project for the **ESP32-WROOM** module utilizing the official **Espressif IoT Development Framework (ESP-IDF)**.

## Architecture

1. **NVS Initialization:** The Wi-Fi driver relies on Non-Volatile Storage (NVS) to load internal RF calibration parameters and save state configurations.
2. **Netif Initialization:** Configures the underlying network interface and initializes the Espressif TCP/IP stack instance.
3. **Event Loop:** Spawns the system dispatch engine responsible for handling internal Wi-Fi state machine updates.
4. **Wi-Fi Driver Setup:** Provisions the RF hardware into Station (`WIFI_MODE_STA`) client mode.
5. **Active Channel Sweep:** Sequentially steps through available 2.4GHz channels (`1` to `13` or `14`) using an active scan configuration.
6. **Telemetry Extraction:** Captures the hardware-populated structure arrays into organized matrix formats for downstream applications.

---

## Data Structure 

The system relies on parsing the structural telemetry elements provided inside Espressif's native `wifi_ap_record_t` array buffer:

* **`.ssid`**: A character array containing the network name identifier (maximum capacity of 32 octets).
* **`.bssid`**: A 6-byte array holding the unique hardware MAC address of the routing transceiver.
* **`.rssi`**: Received Signal Strength Indicator measured in dBm (values closer to `0` denote increased signal intensity; e.g., `-45 dBm` indicates excellent signal, while `-85 dBm` signals marginal connectivity).
* **`.primary`**: Indicates the specific radio frequency slot channel managing transmission (typically Channels 1–13 within the standard 2.4GHz spectrum rules).
* **`.authmode`**: The internal enumeration defining the target authentication wrapper type (e.g., `WIFI_AUTH_WPA2_PSK`).

---

## Implementation Roadmap (TODOs)

The source files in this repository require implementation for the following segments:

### 💡 TODO 1: Core Subsystem Initialization & Blocking Scan
* Initialize the NVS flash partition handling.
* Instantiate the system event loop.
* Configure `wifi_scan_config_t` with `.show_hidden = true` to unmask hidden networks.
* Call `esp_wifi_scan_start()` in synchronous (blocking) mode to halt the task until the hardware sweep is complete.

### 💡 TODO 2: Logging to Serial Port
* Extract the AP array data via `esp_wifi_scan_get_ap_records()`.
* Implement clean formatting using the `ESP_LOGI` macro to stream the raw Wi-Fi metrics (SSID, BSSID, RSSI, Channel, Auth Mode) to the UART0 serial bus.

### 💡 TODO 3: External Python Script for Serial Port Monitoring
* Develop an independent Python script (using libraries such as `pyserial`) to capture and read communication telemetry directly from the ESP32's COM/TTY port.
* This script must operate **fully independent of the ESP-IDF toolchain**, allowing third-party desktop tools to display or log the scanned access points without needing `idf.py monitor` running.

---

## Toolchain

Make sure that you are using latest version of the ESP-IDF toolchain. Follow installation and initialization procecure on official Espressif webpage.

### Hardware Architecture Target
Configure the project target exclusively for the classic ESP32 architecture powering your ESP32-WROOM system module variant:
```bash
idf.py set-target esp32