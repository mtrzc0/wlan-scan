# ESP32-WROOM Wi-Fi Scanner using ESP-IDF

A production-ready, robust Wi-Fi scanner project for the **ESP32-WROOM** module utilizing the official **Espressif IoT Development Framework (ESP-IDF)**. This project demonstrates how to initialize the underlying network layers, perform a hardware-level active scan, and parse essential Access Point (AP) metadata such as SSID, BSSID (MAC Address), RSSI (Signal Strength), channel, and encryption scheme.

## Architecture & Flow

Unlike basic wrapper frameworks, ESP-IDF requires explicit configuration of the system layers. The scanning pipeline executes through the following sequence:

1. **NVS Initialization:** The Wi-Fi stack uses Non-Volatile Storage to cache calibration parameters and internal radio data.
2. **Netif Initialization:** Configures the underlying network interface and TCP/IP stack instance.
3. **Event Loop:** Spawns the central system dispatch engine handling internal driver state shifts.
4. **Wi-Fi Driver Configuration:** Provisions the RF hardware into Station (`WIFI_MODE_STA`) mode.
5. **Synchronous Active Scan:** Interrogates all available 2.4GHz channels (`1` to `13`/`14`) sequentially.
6. **Data Processing:** Extracts the hardware-populated structure arrays into readable matrix formats via standard streams.

---

## Code Implementation (`main.c`)

Create an application entry point in your project (typically under `main/main.c`) and include the following implementation:

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_scanner";

// Helper function to map auth mode enums to string representation
static const char* get_auth_mode_name(wifi_auth_mode_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        default: return "UNKNOWN";
    }
}

void app_main(void)
{
    // 1. Initialize NVS (Mandatory for Wi-Fi subsystem calibration)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize the underlying network interface stack
    ESP_ERROR_CHECK(esp_netif_init());

    // 3. Create the default system event loop execution context
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 4. Instantiate default Wi-Fi Station instance and default configuration
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Configure Wi-Fi operating mode as Station and start the RF driver
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi subsystem initialized. Commencing periodic scan loop...");

    // 6. Define Scan Parameter configurations
    wifi_scan_config_t scan_config = {
        .ssid = NULL,            // Fetch all available SSIDs
        .bssid = NULL,           // Fetch all target MACs 
        .channel = 0,            // 0 forces scanning across all valid RF channels
        .show_hidden = true,     // Unmask hidden/non-broadcast SSIDs
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    while (1) {
        // Run blocking scan: Yields task execution until channel sweeps complete
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
        ESP_LOGI(TAG, "Scan sweep completed successfully.");

        // Read out the count of found Access Points
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        
        if (ap_count == 0) {
            ESP_LOGW(TAG, "No networks detected within range.");
        } else {
            ESP_LOGI(TAG, "Total networks discovered: %d", ap_count);

            // Dynamically allocate memory buffer for record telemetry
            wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_count);
            if (ap_info == NULL) {
                ESP_LOGE(TAG, "Heap allocation failed for AP records buffer.");
                return;
            }

            // Retrieve structural payloads into our allocated buffer
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

            // Render data table header
            printf("
%-32s | %-18s | %-7s | %-4s | %-12s
", "SSID", "BSSID (MAC)", "RSSI", "CHAN", "AUTH");
            printf("----------------------------------------------------------------------------------------
");

            // Format and print each distinct record
            for (int i = 0; i < ap_count; i++) {
                char bssid_str[18];
                snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                         ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                         ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);

                printf("%-32s | %-18s | %-7d | %-4d | %-12s
",
                       (char *)ap_info[i].ssid,
                       bssid_str,
                       ap_info[i].rssi,
                       ap_info[i].primary,
                       get_auth_mode_name(ap_info[i].authmode));
            }
            printf("----------------------------------------------------------------------------------------

");

            // Free dynamic memory allocation
            free(ap_info);
        }

        // Delay task execution context for 10,000ms prior to the next cycle
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## Detailed Data Structure Breakdown

The data processing logic revolves around parsing structural telemetry items provided within Espressif's `wifi_ap_record_t` definitions:

*   **`.ssid`**: A character array containing the broadcasted network label identifier (maximum capacity of 32 octets).
*   **`.bssid`**: A 6-byte unique hardware MAC address of the routing transceiver.
*   **`.rssi`**: Received Signal Strength Indicator measured in dBm (values closer to `0` designate increased signal intensity; e.g., `-45 dBm` indicates excellent signal, while `-85 dBm` signals marginal connectivity limit thresholds).
*   **`.primary`**: Indicates the exact radio frequency slot channel assignment managing transmission (typically Channels 1–13 within standard 2.4GHz spectrum rules).
*   **`.authmode`**: The internal enumeration setting defining the target authentication wrapper type (e.g., `WIFI_AUTH_WPA2_PSK`).

---

## Toolchain Compilation and Execution

Ensure that your native machine terminal environment is fully initialized with the required Espressif export scripts (`. ./export.sh` or `export.bat`).

### 1. Set the Hardware Architecture Target
Configure the project target exclusively for the classic ESP32 architecture powering your ESP32-WROOM system module variant:
```bash
idf.py set-target esp32
```

### 2. Compile Project Binaries
Trigger the compilation processes to link your binaries:
```bash
idf.py build
```

### 3. Flash and Monitor Telemetry Output
Upload the completed payload onto your physical microcontroller. Replace `PORT` with your operational communications node path (e.g., `COM3` inside Windows layouts or `/dev/ttyUSB0` under typical Linux configurations):
```bash
idf.py -p PORT flash monitor
```

*Note: To break out or stop running active serial interface monitoring dashboards within target panels, press the shortcut sequence keys **`Ctrl + ]`**.*
