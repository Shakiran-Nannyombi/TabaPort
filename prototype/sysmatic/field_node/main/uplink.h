#pragma once
/* Uplink stand-in: Wi-Fi station + MQTT. In the field this slot is LoRa or GSM; the
 * firmware contract is the same: publish(line) succeeds or the line is queued on the SD card. */
#include <stdbool.h>

void uplink_init(const char *ssid, const char *password, const char *broker_uri, const char *topic);
bool uplink_wifi_up(void);
bool uplink_connected(void);              /* MQTT session established */
bool uplink_publish(const char *json_line);
