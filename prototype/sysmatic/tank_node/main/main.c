/*
 * TabaPort drain node — generation 1 firmware (ESP-IDF)
 * Board: ESP32-WROOM-32 DevKit (38-pin). No Wi-Fi, no sleep, no LoRa.
 * Sensor: JSN-SR04T waterproof ultrasonic (or HC-SR04 backup), Echo through a 1k/2k divider.
 *
 * Output: one JSON line per reading on UART0 (USB) at 115200, 2 Hz.
 *   {"node_id":"drain_01","raw_cm":41.2,"level_cm":8.8,"filtered":false,"state":"green","ok":true}
 *
 * Input: the hub may send one line to set calibration/thresholds at any time:
 *   CFG h_sensor=60.0 yellow=30 red=48
 *
 * The hub (prototype/app) is the source of truth; the LEDs on the node mirror the same
 * thresholds so the camera can see the state change at the drain.
 */

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---------- Pins (match docs/phase3/wiring.md) ---------- */
#define PIN_TRIG   GPIO_NUM_5
#define PIN_ECHO   GPIO_NUM_18   /* through 1k series + 2k to GND */
#define PIN_GREEN  GPIO_NUM_25
#define PIN_YELLOW GPIO_NUM_26
#define PIN_RED    GPIO_NUM_27
#define PIN_BUZZER GPIO_NUM_14

#define NODE_ID    "drain_01"
#define UART       UART_NUM_0
#define BAUD       115200

/* ---------- Calibration (defaults = rain-day tank fallback; hub overrides with CFG) ---------- */
static float h_sensor_cm = 50.0f;   /* probe face -> drain invert, measured dry */
static float yellow_cm   = 8.0f;
static float red_cm      = 15.0f;

/* ---------- Filter ---------- */
#define WIN        5               /* moving median window (7 if the drain is noisy) */
#define SPIKE_CM   10.0f           /* |raw - median| above this = debris, report median */
static float win[WIN];
static int   win_count = 0, win_head = 0;

/* ---------- Fault detection ---------- */
#define MISS_FAULT 5               /* consecutive no-echo readings before blinking all LEDs */
static int miss_count = 0;

/* ---------- Timing ---------- */
#define PERIOD_MS  500             /* 2 Hz */
#define ECHO_TIMEOUT_US 30000      /* ~5 m round trip */

static char prev_state[8] = "";
static char rx_line[128];
static int  rx_len = 0;

/* ------------------------------------------------------------------------- */
static void out(const char *fmt, ...)
{
    char buf[224];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) uart_write_bytes(UART, buf, n > (int)sizeof buf ? (int)sizeof buf : n);
}

static float read_raw_cm(void)
{
    gpio_set_level(PIN_TRIG, 0);
    esp_rom_delay_us(5);
    gpio_set_level(PIN_TRIG, 1);
    esp_rom_delay_us(20);              /* JSN-SR04T wants >= 10 us */
    gpio_set_level(PIN_TRIG, 0);

    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 0) {
        if (esp_timer_get_time() - t0 > ECHO_TIMEOUT_US) return -1.0f;   /* no echo start */
    }
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 1) {
        if (esp_timer_get_time() - start > ECHO_TIMEOUT_US) return -1.0f; /* out of range */
    }
    int64_t dur_us = esp_timer_get_time() - start;
    return (dur_us * 0.0343f) / 2.0f;  /* cm */
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
    for (int i = 1; i < n; i++) {          /* insertion sort, n <= 7 */
        float k = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > k) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = k;
    }
    return tmp[n / 2];
}

static const char *state_for(float level_cm)
{
    if (level_cm >= red_cm)    return "red";
    if (level_cm >= yellow_cm) return "yellow";
    return "green";
}

static void set_leds(bool g, bool y, bool r)
{
    gpio_set_level(PIN_GREEN,  g);
    gpio_set_level(PIN_YELLOW, y);
    gpio_set_level(PIN_RED,    r);
}

static void show_state(const char *s)
{
    if      (!strcmp(s, "red"))    set_leds(false, false, true);
    else if (!strcmp(s, "yellow")) set_leds(false, true,  false);
    else                           set_leds(true,  false, false);
}

/* Buzzer on LEDC so a passive buzzer gets a real tone; an active buzzer just hears 50 % duty. */
static void buzzer_init(void)
{
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 2000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
    ledc_channel_config_t c = {
        .gpio_num   = PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&c);
}

static void beep(uint32_t freq_hz, int ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void print_cfg(const char *tag)
{
    out("{\"node_id\":\"%s\",\"%s\":true,\"h_sensor_cm\":%.1f,\"yellow_cm\":%.1f,\"red_cm\":%.1f}\n",
        NODE_ID, tag, h_sensor_cm, yellow_cm, red_cm);
}

static void apply_cfg_line(char *line)
{
    /* Accept: CFG h_sensor=60.0 yellow=30 red=48   (any subset) */
    if (strncmp(line, "CFG", 3) != 0) return;
    char *p;
    if ((p = strstr(line, "h_sensor="))) h_sensor_cm = strtof(p + 9, NULL);
    if ((p = strstr(line, "yellow=")))   yellow_cm   = strtof(p + 7, NULL);
    if ((p = strstr(line, "red=")))      red_cm      = strtof(p + 4, NULL);
    print_cfg("cfg");
}

static void handle_serial_config(void)
{
    uint8_t b;
    while (uart_read_bytes(UART, &b, 1, 0) == 1) {
        if (b == '\n' || b == '\r') {
            if (rx_len > 0) {
                rx_line[rx_len] = '\0';
                apply_cfg_line(rx_line);
                rx_len = 0;
            }
        } else if (rx_len < (int)sizeof rx_line - 1) {
            rx_line[rx_len++] = (char)b;
        } else {
            rx_len = 0;   /* garbage; drop */
        }
    }
}

static void gpio_init(void)
{
    gpio_config_t outs = {
        .pin_bit_mask = (1ULL << PIN_TRIG) | (1ULL << PIN_GREEN) | (1ULL << PIN_YELLOW) | (1ULL << PIN_RED),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&outs);
    gpio_config_t in = {
        .pin_bit_mask = (1ULL << PIN_ECHO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   /* idle low if the probe is unplugged */
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in);
    gpio_set_level(PIN_TRIG, 0);
}

static void uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART, 1024, 0, 0, NULL, 0);
    uart_param_config(UART, &cfg);
}

/* ------------------------------------------------------------------------- */
void app_main(void)
{
    gpio_init();
    uart_init();
    buzzer_init();

    /* Power-on self test: each LED in turn, then a short beep. */
    set_leds(true, false, false);  vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(false, true, false);  vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(false, false, true);  vTaskDelay(pdMS_TO_TICKS(150));
    set_leds(false, false, false);
    beep(1500, 80);
    print_cfg("boot");

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        handle_serial_config();

        float raw = read_raw_cm();

        if (raw < 0.0f) {
            miss_count++;
            if (miss_count >= MISS_FAULT) {
                /* Sensor fault: blink all three. Hub marks the segment unknown after its own timeout. */
                bool on = (esp_timer_get_time() / 250000) % 2 == 0;
                set_leds(on, on, on);
            }
            out("{\"node_id\":\"%s\",\"raw_cm\":null,\"level_cm\":null,\"filtered\":false,"
                "\"state\":\"unknown\",\"ok\":false,\"miss\":%d}\n", NODE_ID, miss_count);
        } else {
            miss_count = 0;
            push_window(raw);
            float med = median_of_window();
            bool filtered = fabsf(raw - med) > SPIKE_CM;   /* debris spike -> use median */
            float use_dist = filtered ? med : raw;

            float level = h_sensor_cm - use_dist;
            if (level < 0.0f) level = 0.0f;

            const char *st = state_for(level);
            show_state(st);
            if (strcmp(prev_state, "red") != 0 && strcmp(st, "red") == 0) beep(2000, 150);
            strncpy(prev_state, st, sizeof prev_state - 1);

            out("{\"node_id\":\"%s\",\"raw_cm\":%.1f,\"level_cm\":%.1f,\"filtered\":%s,"
                "\"state\":\"%s\",\"ok\":true}\n",
                NODE_ID, raw, level, filtered ? "true" : "false", st);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_MS));
    }
}
