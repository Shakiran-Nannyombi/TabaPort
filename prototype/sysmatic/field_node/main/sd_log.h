#pragma once
/* microSD over SPI, FAT. Local log of every reading and a store-and-forward queue for the uplink. */
#include <stdbool.h>

bool sd_log_init(int sck, int mosi, int miso, int cs);
bool sd_log_ok(void);
bool sd_log_append(const char *filename, const char *line);      /* filename relative to card root */
/* Send queued lines with `send`; stop at the first failure. Returns lines sent. Deletes the queue when empty. */
int  sd_queue_flush(const char *filename, bool (*send)(const char *line), int max_lines);
int  sd_queue_count(const char *filename);
