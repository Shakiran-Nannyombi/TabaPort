idf_component_register(SRCS "main.c" "i2c_devices.c" "sd_log.c" "uplink.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio esp_driver_uart esp_driver_ledc esp_driver_i2c
                                esp_driver_spi esp_driver_sdspi sdmmc fatfs esp_adc esp_timer
                                nvs_flash esp_wifi esp_netif esp_event mqtt)
