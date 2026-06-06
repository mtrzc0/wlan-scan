#ifndef WLAN_H
#define WLAN_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "config.h"

ESP_EVENT_DECLARE_BASE(APP_EVENT);

typedef enum {
    APP_SCAN_READY_ID,
} app_event_id;

const char* app_get_auth_mode_name(wifi_auth_mode_t auth_mode);
void app_wifi_init(void);
void app_show_scanned(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data);
void app_start_scanning(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data);

#endif /* WLAN_H */