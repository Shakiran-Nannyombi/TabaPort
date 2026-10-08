/* JSN-SR04T waterproof ultrasonic (Trig/Echo mode).
 * The round probe hangs over the water. This board stays dry.
 * Echo high-time matches the firmware: cm = (microseconds * 0.0343) / 2.
 * Inside the 25 cm blind zone the probe returns no echo.
 */
#include "wokwi-api.h"
#include <stdlib.h>

#define BLIND_CM 25.0f

typedef struct {
    pin_t echo;
    uint32_t distance;
    uint64_t trig_rise_ns;
    timer_t echo_off;
} chip_state_t;

static void echo_off(void *user_data) {
    chip_state_t *chip = user_data;
    pin_write(chip->echo, LOW);
}

/* Walk the air gap so the LED and the road keep changing color.
 * 5 seconds green, 5 seconds yellow, 5 seconds red, then again.
 */
static float scenario_cm(void) {
    uint64_t sec = get_sim_nanos() / 1000000000ULL;
    switch ((sec / 5) % 3) {
        case 0: return 48.0f; /* about 2 cm of water, green  */
        case 1: return 40.0f; /* about 10 cm of water, yellow */
        default: return 30.0f; /* about 20 cm of water, red   */
    }
}

static void on_trig(void *user_data, pin_t pin, uint32_t value) {
    (void)pin;
    chip_state_t *chip = user_data;
    if (value == HIGH) {
        chip->trig_rise_ns = get_sim_nanos();
        return;
    }
    uint64_t width_us = (get_sim_nanos() - chip->trig_rise_ns) / 1000;
    if (width_us < 10) return;

    float cm = scenario_cm();
    (void)chip->distance;
    if (cm < BLIND_CM) {
        pin_write(chip->echo, LOW);
        return;
    }
    pin_write(chip->echo, HIGH);
    uint32_t echo_us = (uint32_t)(cm * 2.0f / 0.0343f);
    if (echo_us < 1) echo_us = 1;
    timer_start(chip->echo_off, echo_us, false);
}

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof *chip);
    pin_init("VCC", INPUT);
    pin_init("GND", INPUT);
    pin_t trig = pin_init("TRIG", INPUT);
    chip->echo = pin_init("ECHO", OUTPUT_LOW);
    chip->distance = attr_init_float("distance", 48.0f);
    chip->trig_rise_ns = 0;

    const timer_config_t timer = { .callback = echo_off, .user_data = chip };
    chip->echo_off = timer_init(&timer);

    const pin_watch_config_t watch = {
        .edge = BOTH,
        .pin_change = on_trig,
        .user_data = chip,
    };
    pin_watch(trig, &watch);
}
