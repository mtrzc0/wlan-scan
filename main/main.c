#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_scanner";

// Helper function to convert auth mode enum to a readable string
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
    // 1. Initialize NVS (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize the underlying TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // 3. Create the default system event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 4. Initialize Wi-Fi with default configuration
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Set Wi-Fi mode to Station (Client) and Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized. Starting scan...");

    // 6. Configure Scan parameters
    // Setting show_hidden to true scans for networks not broadcasting SSID
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0, // 0 means scan all channels
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    while (1) {
        // Start the scan. The 'true' argument makes this a blocking call.
        // It will wait until the scan finishes before moving to the next line.
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
        ESP_LOGI(TAG, "Scan finished completely.");

        // Fetch the number of APs found
        uint16_t number = 0;
        esp_wifi_scan_get_ap_num(&number);
        
        if (number == 0) {
            ESP_LOGW(TAG, "No Wi-Fi networks found.");
        } else {
            ESP_LOGI(TAG, "Total APs found: %d", number);

            // Allocate memory to hold the AP records found
            wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * number);
            if (ap_info == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for AP records");
                return;
            }

            // Populate the array with actual AP data
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

            // Print Header
            printf("\n%-32s | %-18s | %-7s | %-4s | %-12s\n", "SSID", "BSSID (MAC)", "RSSI", "CHAN", "AUTH");
            printf("----------------------------------------------------------------------------------------\n");

            // Loop through and print details of each access point
            for (int i = 0; i < number; i++) {
                char bssid_str[18];
                snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                         ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                         ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);

                printf("%-32s | %-18s | %-7d | %-4d | %-12s\n",
                       (char *)ap_info[i].ssid,
                       bssid_str,
                       ap_info[i].rssi,
                       ap_info[i].primary,
                       get_auth_mode_name(ap_info[i].authmode));
            }
            printf("----------------------------------------------------------------------------------------\n\n");

            // Clean up memory
            free(ap_info);
        }

        // Wait 10 seconds before scanning again
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}