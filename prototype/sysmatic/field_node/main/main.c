/*
 * TabaPort field node — generation 2 reference design (ESP-IDF, simulation first)
 *
 * This is the node we would mount at a Kampala blackspot. It exists to show every
 * component a real installation needs, so the parts the simulator cannot model
 * (radio, solar, enclosure, probe survival) become concrete questions for review.
 *
 * Components (see diagram.json and docs/phase3/field-node-brief.md):
 *   ultrasonic probe        water level              GPIO16 trig, GPIO17 echo (1k/2k divider)
 *   NTC thermistor          speed-of-sound correction GPIO35 (ADC1 ch7)
 *   battery divider         node health              GPIO34 (ADC1 ch6), x2 divider
 *   MPU6050                 probe tilt / tamper      I2C 0x69
 *   DS1307                  offline timestamps       I2C 0x68
 *   20x4 LCD                on-site readout          I2C 0x27
 *   microSD                 local log + send queue   SPI: SCK18 MOSI23 MISO19 CS5
 *   button                  set dry zero on site     GPIO4 (to GND)
 *   relay -> strobe         road warning on red      GPIO13
 *   LEDs green/yellow/red   state                    GPIO25/26/27
 *   buzzer                  one beep on red          GPIO14
 *   Wi-Fi + MQTT            uplink stand-in for LoRa/GSM
 *
 * Output: one JSON line per reading on UART0 at 115200. The hub (prototype/app) reads it.
 */

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "i2c_devices.h"
#include "sd_log.h"
#include "uplink.h"

/* ---------- identity ---------- */
#define NODE_ID     "field_01"
#define SEGMENT_ID  "mulago_rbt_01"

/* ---------- pins ---------- */
#define PIN_TRIG    GPIO_NUM_16
#define PIN_ECHO    GPIO_NUM_17
#define PIN_SDA     GPIO_NUM_21
#define PIN_SCL     GPIO_NUM_22
#define PIN_SD_SCK  18
#define PIN_SD_MOSI 23
#define PIN_SD_MISO 19
#define PIN_SD_CS   5
#define PIN_BUTTON  GPIO_NUM_4
#define PIN_RELAY   GPIO_NUM_13
#define PIN_GREEN   GPIO_NUM_25
#define PIN_YELLOW  GPIO_NUM_26
#define PIN_RED     GPIO_NUM_27
#define PIN_BUZZER  GPIO_NUM_14
#define ADC_BATT    ADC_CHANNEL_6        /* GPIO34 */
#define ADC_NTC     ADC_CHANNEL_7        /* GPIO35 */

/* ---------- uplink (Wokwi guest network, public test broker) ---------- */
#define WIFI_SSID   "Wokwi-GUEST"
#define WIFI_PASS   ""
#define MQTT_URI    "mqtt://test.mosquitto.org"
#define MQTT_TOPIC  "tabaport/" NODE_ID "/reading"

/* ---------- behaviour ---------- */
#define PERIOD_MS        500
#define ECHO_TIMEOUT_US  30000
#define WIN              5
#define SPIKE_CM         10.0f
#define MISS_FAULT       5
#define TILT_MAX_DEG     10.0f
#define BATT_LOW_V       3.5f
#define NTC_BETA         3950.0f
#define LOG_EVERY        4               /* readings per SD log line (2 s) */
#define PUBLISH_EVERY    10              /* readings per uplink message (5 s) */
#define BUTTON_HOLD_MS   1000

#define LOG_FILE   "log.jsonl"
#define QUEUE_FILE "queue.jsonl"

/* ---------- calibration (NVS, defaults = 60 cm drain, probe 50 cm above invert) ---------- */
static float h_sensor_cm = 50.0f;
static float h_road_cm   = 60.0f;
static float yellow_cm   = 30.0f;
static float red_cm      = 48.0f;

/* ---------- state ---------- */
static adc_oneshot_unit_handle_t adc;
static float win[WIN];
static int   win_count, win_head;
static int   miss_count;
static char  prev_state[8] = "";
static char  rx_line[128];
static int   rx_len;
static uint32_t n_reading;
static int64_t  button_down_at = -1;

/* ------------------------------------------------------------------------- */
static void out(const char *fmt, ...)
{
    char buf[400];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) uart_write_bytes(UART_NUM_0, buf, n > (int)sizeof buf ? (int)sizeof buf : n);
}

/* ---------- calibration persistence ---------- */
static void cal_load(void)
{
    nvs_handle_t h;
    if (nvs_open("cal", NVS_READONLY, &h) != ESP_OK) return;
    size_t sz = sizeof(float);
    nvs_get_blob(h, "h_sensor", &h_sensor_cm, &sz);
    nvs_get_blob(h, "h_road",   &h_road_cm,   &sz);
    nvs_get_blob(h, "yellow",   &yellow_cm,   &sz);
    nvs_get_blob(h, "red",      &red_cm,      &sz);
    nvs_close(h);
}

static void cal_save(void)
{
    nvs_handle_t h;
    if (nvs_open("cal", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "h_sensor", &h_sensor_cm, sizeof(float));
    nvs_set_blob(h, "h_road",   &h_road_cm,   sizeof(float));
    nvs_set_blob(h, "yellow",   &yellow_cm,   sizeof(float));
    nvs_set_blob(h, "red",      &red_cm,      sizeof(float));
    nvs_commit(h);
    nvs_close(h);
}

static void print_cfg(const char *tag)
{
    out("{\"node_id\":\"%s\",\"%s\":true,\"segment_id\":\"%s\",\"h_sensor_cm\":%.1f,\"h_road_cm\":%.1f,"
        "\"yellow_cm\":%.1f,\"red_cm\":%.1f,\"lcd\":%s,\"rtc\":%s,\"imu\":%s,\"sd\":%s}\n",
        NODE_ID, tag, SEGMENT_ID, h_sensor_cm, h_road_cm, yellow_cm, red_cm,
        lcd_present() ? "true" : "false", ds1307_present() ? "true" : "false",
        imu_present() ? "true" : "false", sd_log_ok() ? "true" : "false");
}

static void apply_cfg_line(char *line)
{
    if (strncmp(line, "CFG", 3) != 0) return;
    char *p;
    if ((p = strstr(line, "h_sensor="))) h_sensor_cm = strtof(p + 9, NULL);
    if ((p = strstr(line, "h_road=")))   h_road_cm   = strtof(p + 7, NULL);
    if ((p = strstr(line, "yellow=")))   yellow_cm   = strtof(p + 7, NULL);
    if ((p = strstr(line, "red=")))      red_cm      = strtof(p + 4, NULL);
    cal_save();
    print_cfg("cfg");
}

static void handle_serial_config(void)
{
    uint8_t b;
    while (uart_read_bytes(UART_NUM_0, &b, 1, 0) == 1) {
        if (b == '\n' || b == '\r') {
            if (rx_len > 0) { rx_line[rx_len] = '\0'; apply_cfg_line(rx_line); rx_len = 0; }
        } else if (rx_len < (int)sizeof rx_line - 1) {
            rx_line[rx_len++] = (char)b;
        } else {
            rx_len = 0;
        }
    }
}

/* ---------- sensors ---------- */
static float adc_volts(adc_channel_t ch)
{
    int raw = 0;
    adc_oneshot_read(adc, ch, &raw);
    return raw * 3.3f / 4095.0f;          /* close enough for a health readout; calibrate for real */
}

static float read_temp_c(void)
{
    int raw = 0;
    adc_oneshot_read(adc, ADC_NTC, &raw);
    if (raw <= 0 || raw >= 4095) return NAN;
    /* Module: 10k NTC to GND, 10k series to VCC, OUT across the NTC. */
    float ratio = raw / (4095.0f - raw);                 /* R_ntc / R_series */
    float inv_t = logf(ratio) / NTC_BETA + 1.0f / 298.15f;
    return 1.0f / inv_t - 273.15f;
}

static float read_batt_v(void)
{
    return adc_volts(ADC_BATT) * 2.0f;    /* 1:1 divider on the real board */
}

static float read_raw_cm(float temp_c)
{
    gpio_set_level(PIN_TRIG, 0); esp_rom_delay_us(5);
    gpio_set_level(PIN_TRIG, 1); esp_rom_delay_us(20);
    gpio_set_level(PIN_TRIG, 0);

    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 0)
        if (esp_timer_get_time() - t0 > ECHO_TIMEOUT_US) return -1.0f;
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 1)
        if (esp_timer_get_time() - start > ECHO_TIMEOUT_US) return -1.0f;
    int64_t dur_us = esp_timer_get_time() - start;

    /* speed of sound: 331.3 + 0.606 T  (m/s). Default 20 C if the thermistor is missing. */
    float t = isnan(temp_c) ? 20.0f : temp_c;
    float c_cm_per_us = (331.3f + 0.606f * t) / 10000.0f;
    return dur_us * c_cm_per_us / 2.0f;
}

static void push_window(float v)
{
    win[win_head] = v;
    win_head = (win_head + 1) % WIN;
    if (win_count < WIN) win_count++;
}

static float median_of_window(void)
{
    float tmp[WIN];
    int n = win_count;
    memcpy(tmp, win, n * sizeof(float));
    for (int i = 1; i < n; i++) {
        float k = tmp[i]; int j = i - 1;
        while (j >= 0 && tmp[j] > k) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = k;
    }
    return tmp[n / 2];
}

/* ---------- outputs ---------- */
static void set_leds(bool g, bool y, bool r)
{
    gpio_set_level(PIN_GREEN, g); gpio_set_level(PIN_YELLOW, y); gpio_set_level(PIN_RED, r);
}

static void buzzer_init(void)
{
    ledc_timer_config_t t = { .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
                              .timer_num = LEDC_TIMER_0, .freq_hz = 2000, .clk_cfg = LEDC_AUTO_CLK };
    ledc_timer_config(&t);
    ledc_channel_config_t c = { .gpio_num = PIN_BUZZER, .speed_mode = LEDC_LOW_SPEED_MODE,
                                .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0 };
    ledc_channel_config(&c);
}

static void beep(uint32_t hz, int ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static const char *state_for(float level, bool tilted, bool fault)
{
    if (fault || tilted) return "unknown";
    if (level >= red_cm || level >= h_road_cm - 10.0f) return "red";
    if (level >= yellow_cm) return "yellow";
    return "green";
}

static void show_state(const char *s, int64_t now_us)
{
    if (!strcmp(s, "red"))         { set_leds(0, 0, 1); gpio_set_level(PIN_RELAY, 1); }
    else if (!strcmp(s, "yellow")) { set_leds(0, 1, 0); gpio_set_level(PIN_RELAY, 0); }
    else if (!strcmp(s, "green"))  { set_leds(1, 0, 0); gpio_set_level(PIN_RELAY, 0); }
    else {                                                /* unknown: blink all, strobe off */
        bool on = (now_us / 250000) % 2 == 0;
        set_leds(on, on, on); gpio_set_level(PIN_RELAY, 0);
    }
}

/* ---------- time ---------- */
static void ds1307_seed_if_needed(void)
{
    struct tm t;
    if (!ds1307_present() || ds1307_read(&t)) return;
    /* Clock stopped or never set: seed with the build time so timestamps are at least ordered. */
    static const char mon[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char m[4] = {0}; int d, y, H, M, S;
    sscanf(__DATE__, "%3s %d %d", m, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &H, &M, &S);
    memset(&t, 0, sizeof t);
    t.tm_mon = (int)((strstr(mon, m) - mon) / 3);
    t.tm_mday = d; t.tm_year = y - 1900; t.tm_hour = H; t.tm_min = M; t.tm_sec = S;
    ds1307_write(&t);
}

static void timestamp(char *buf, size_t n, int64_t uptime_us)
{
    struct tm t;
    if (ds1307_read(&t)) strftime(buf, n, "%Y-%m-%dT%H:%M:%S", &t);
    else snprintf(buf, n, "uptime+%llds", (long long)(uptime_us / 1000000));
}

/* ---------- init ---------- */
static void hw_init(void)
{
    gpio_config_t outs = {
        .pin_bit_mask = (1ULL << PIN_TRIG) | (1ULL << PIN_GREEN) | (1ULL << PIN_YELLOW) |
                        (1ULL << PIN_RED) | (1ULL << PIN_RELAY),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&outs);
    gpio_config_t echo = { .pin_bit_mask = 1ULL << PIN_ECHO, .mode = GPIO_MODE_INPUT,
                           .pull_down_en = GPIO_PULLDOWN_ENABLE };
    gpio_config(&echo);
    gpio_config_t btn = { .pin_bit_mask = 1ULL << PIN_BUTTON, .mode = GPIO_MODE_INPUT,
                          .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&btn);

    uart_config_t ucfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
                           .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                           .source_clk = UART_SCLK_DEFAULT };
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &ucfg);

    adc_oneshot_unit_init_cfg_t ucfg_adc = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&ucfg_adc, &adc);
    adc_oneshot_chan_cfg_t ch = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    adc_oneshot_config_channel(adc, ADC_BATT, &ch);
    adc_oneshot_config_channel(adc, ADC_NTC, &ch);

    buzzer_init();
}

void app_main(void)
{
    hw_init();

    /* NVS before Wi-Fi and calibration */
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) { nvs_flash_erase(); nvs_flash_init(); }
    cal_load();

    i2c_devices_init(PIN_SDA, PIN_SCL);
    lcd_init();
    ds1307_init();
    imu_init();
    ds1307_seed_if_needed();
    sd_log_init(PIN_SD_SCK, PIN_SD_MOSI, PIN_SD_MISO, PIN_SD_CS);

    lcd_print(0, "TabaPort " NODE_ID);
    lcd_print(1, "starting...");
    lcd_print(2, sd_log_ok() ? "SD ok" : "SD missing");
    lcd_print(3, "uplink: connecting");

    set_leds(1, 0, 0); vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(0, 1, 0); vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(0, 0, 1); vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(0, 0, 0);
    beep(1500, 80);
    print_cfg("boot");

    uplink_init(WIFI_SSID, WIFI_PASS, MQTT_URI, MQTT_TOPIC);

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        handle_serial_config();
        int64_t now = esp_timer_get_time();
        n_reading++;

        /* ----- health inputs ----- */
        float temp_c = read_temp_c();
        float batt_v = read_batt_v();
        float tilt = 0.0f;
        bool  have_tilt = imu_tilt_deg(&tilt);
        bool  tilted = have_tilt && tilt > TILT_MAX_DEG;

        /* ----- dry-zero button: hold 1 s ----- */
        if (gpio_get_level(PIN_BUTTON) == 0) {
            if (button_down_at < 0) button_down_at = now;
            else if (now - button_down_at > BUTTON_HOLD_MS * 1000 && win_count > 0) {
                h_sensor_cm = median_of_window();
                cal_save();
                beep(2500, 60); vTaskDelay(pdMS_TO_TICKS(60)); beep(2500, 60);
                print_cfg("dry_zero");
                lcd_print(1, "dry zero set");
                button_down_at = INT64_MAX / 2;        /* fire once per press */
            }
        } else {
            button_down_at = -1;
        }

        /* ----- level ----- */
        float raw = read_raw_cm(temp_c);
        bool fault = raw < 0.0f;
        float level = 0.0f, med = 0.0f;
        bool filtered = false;
        if (fault) {
            miss_count++;
        } else {
            miss_count = 0;
            push_window(raw);
            med = median_of_window();
            filtered = fabsf(raw - med) > SPIKE_CM;
            level = h_sensor_cm - (filtered ? med : raw);
            if (level < 0.0f) level = 0.0f;
        }
        bool sensor_fault = miss_count >= MISS_FAULT;

        const char *st = state_for(level, tilted, sensor_fault);
        show_state(st, now);
        if (strcmp(prev_state, "red") != 0 && strcmp(st, "red") == 0) beep(2000, 150);
        strncpy(prev_state, st, sizeof prev_state - 1);

        /* ----- JSON line ----- */
        char ts[32];
        timestamp(ts, sizeof ts, now);
        char line[400];
        if (fault) {
            snprintf(line, sizeof line,
                "{\"node_id\":\"%s\",\"ts\":\"%s\",\"raw_cm\":null,\"level_cm\":null,\"filtered\":false,"
                "\"state\":\"%s\",\"ok\":false,\"miss\":%d,\"temp_c\":%.1f,\"batt_v\":%.2f,\"batt_low\":%s,"
                "\"tilt_deg\":%.1f,\"tilted\":%s,\"sd\":%s,\"queued\":%d,\"link\":\"%s\"}",
                NODE_ID, ts, st, miss_count, temp_c, batt_v, batt_v < BATT_LOW_V ? "true" : "false",
                tilt, tilted ? "true" : "false", sd_log_ok() ? "true" : "false",
                sd_queue_count(QUEUE_FILE), uplink_connected() ? "mqtt" : (uplink_wifi_up() ? "wifi" : "none"));
        } else {
            snprintf(line, sizeof line,
                "{\"node_id\":\"%s\",\"ts\":\"%s\",\"raw_cm\":%.1f,\"level_cm\":%.1f,\"filtered\":%s,"
                "\"state\":\"%s\",\"ok\":true,\"temp_c\":%.1f,\"batt_v\":%.2f,\"batt_low\":%s,"
                "\"tilt_deg\":%.1f,\"tilted\":%s,\"sd\":%s,\"queued\":%d,\"link\":\"%s\"}",
                NODE_ID, ts, raw, level, filtered ? "true" : "false", st, temp_c, batt_v,
                batt_v < BATT_LOW_V ? "true" : "false", tilt, tilted ? "true" : "false",
                sd_log_ok() ? "true" : "false", sd_queue_count(QUEUE_FILE),
                uplink_connected() ? "mqtt" : (uplink_wifi_up() ? "wifi" : "none"));
        }
        out("%s\n", line);

        /* ----- local log, store and forward ----- */
        if (n_reading % LOG_EVERY == 0) sd_log_append(LOG_FILE, line);
        if (n_reading % PUBLISH_EVERY == 0) {
            if (!uplink_publish(line)) sd_log_append(QUEUE_FILE, line);   /* offline: queue it */
            else sd_queue_flush(QUEUE_FILE, uplink_publish, 20);           /* online: drain backlog */
        }

        /* ----- LCD ----- */
        char l[21];
        snprintf(l, sizeof l, "%-8s %11s", NODE_ID, st);
        lcd_print(0, l);
        if (fault) snprintf(l, sizeof l, "Depth  ---   T %4.1fC", temp_c);
        else       snprintf(l, sizeof l, "Depth %5.1fcm T%4.1fC", level, temp_c);
        lcd_print(1, l);
        snprintf(l, sizeof l, "Batt %4.2fV %s%4.1f%c", batt_v, tilted ? "TILT" : "tilt", tilt, 0xDF);
        lcd_print(2, l);
        snprintf(l, sizeof l, "SD %s Q%-3d %s", sd_log_ok() ? "ok" : "--", sd_queue_count(QUEUE_FILE),
                 uplink_connected() ? "MQTT" : uplink_wifi_up() ? "WIFI" : "OFFL");
        lcd_print(3, l);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_MS));
    }
}
