idf_component_register(SRCS "main.c" "wlan.c"
                       PRIV_REQUIRES spi_flash esp_event esp_wifi nvs_flash
                       INCLUDE_DIRS "include")
