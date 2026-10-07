#include "uplink.h"

#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

static const char *s_topic;
static esp_mqtt_client_handle_t s_mqtt;
static volatile bool s_wifi_up, s_mqtt_up;

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_up = false;
        esp_wifi_connect();               /* keep trying; the node works offline meanwhile */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_wifi_up = true;
        if (s_mqtt) esp_mqtt_client_start(s_mqtt);
    }
}

static void mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:    s_mqtt_up = true;  break;
    case MQTT_EVENT_DISCONNECTED: s_mqtt_up = false; break;
    default: break;
    }
}

void uplink_init(const char *ssid, const char *password, const char *broker_uri, const char *topic)
{
    s_topic = topic;

    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wcfg);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event, NULL);

    wifi_config_t sta = {0};
    strncpy((char *)sta.sta.ssid, ssid, sizeof sta.sta.ssid - 1);
    strncpy((char *)sta.sta.password, password, sizeof sta.sta.password - 1);
    sta.sta.threshold.authmode = password[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta);
    esp_wifi_start();

    esp_mqtt_client_config_t mcfg = {
        .broker.address.uri = broker_uri,
        .network.disable_auto_reconnect = false,
    };
    s_mqtt = esp_mqtt_client_init(&mcfg);
    esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, &mqtt_event, NULL);
}

bool uplink_wifi_up(void)   { return s_wifi_up; }
bool uplink_connected(void) { return s_wifi_up && s_mqtt_up; }

bool uplink_publish(const char *json_line)
{
    if (!uplink_connected()) return false;
    return esp_mqtt_client_publish(s_mqtt, s_topic, json_line, 0, 0, 0) >= 0;
}
