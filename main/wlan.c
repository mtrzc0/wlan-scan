#include "wlan.h"

static const char *TAG = "app-wlan";

ESP_EVENT_DEFINE_BASE(APP_EVENT);

// Helper function to convert auth mode enum to a readable string
const char* app_get_auth_mode_name(wifi_auth_mode_t auth_mode) {
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

void app_wifi_init(void) {
    // Initialize Wi-Fi with default configuration
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   
    // Set Wi-Fi mode to Station (Client) and Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
   
    ESP_LOGD(TAG, "Wi-Fi init DONE");

    // Register App SCAN READY event to system default event loop
    ESP_ERROR_CHECK(esp_event_handler_instance_register(APP_EVENT, APP_SCAN_READY_ID, app_start_scanning, NULL, NULL));
   
    // Register Wi-Fi SCAN DONE event to system default event loop
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, app_show_scanned, NULL, NULL));
    
    // Post event for start scanning 
    ESP_ERROR_CHECK(esp_event_post(APP_EVENT, APP_SCAN_READY_ID, NULL, 0, pdMS_TO_TICKS(SCAN_WAIT_PERIOD)));
}

void app_start_scanning(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    // Configure Scan parameters
    wifi_scan_config_t scan_config = {
       .ssid = NULL,
       .bssid = NULL,
       .channel = 0, // 0 means scan all channels
       .show_hidden = true, // scan for network not broadcasting SSID
       .scan_type = WIFI_SCAN_TYPE_ACTIVE,
       .scan_time.active.min = 100,
       .scan_time.active.max = 300
    };

    // Start scanning for wifi networks
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));
}

void app_show_scanned(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    // Fetch the number of APs found
    uint16_t number = 0;
    esp_wifi_scan_get_ap_num(&number);
    
    if (number == 0) {
        ESP_LOGW(TAG, "Wi-Fi networks NOT FOUND");
    } else {
        ESP_LOGI(TAG, "Wi-Fi scan done. Total APs found: %d", number);

        // Allocate memory to hold the AP records found
        wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * number);
        if (ap_info == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for AP records");
            return;
        }

        // Populate the array with actual AP data
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

        // Print details of each access point
        for (int i = 0; i < number; i++) {
            char bssid_str[18];
            snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                     ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);

            printf("{\"ssid\":\"%s\",\"bssid\":\"%s\",\"rssi\":%d,\"chan\":%d,\"auth\":\"%s\"}\n", 
            (char *)ap_info[i].ssid, 
            bssid_str, 
            ap_info[i].rssi, 
            ap_info[i].primary, 
            app_get_auth_mode_name(ap_info[i].authmode));
        }

        // Clean up memory
        free(ap_info);
        
        // Post event for start scanning 
        ESP_ERROR_CHECK(esp_event_post(APP_EVENT, APP_SCAN_READY_ID, NULL, 0, pdMS_TO_TICKS(SCAN_WAIT_PERIOD)));
    }
}
