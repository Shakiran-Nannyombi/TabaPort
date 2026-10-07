#include "sd_log.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT "/sd"

static sdmmc_card_t *card;
static bool mounted;

bool sd_log_init(int sck, int mosi, int miso, int cs)
{
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 4000;
    spi_bus_config_t bus = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    if (spi_bus_initialize(host.slot, &bus, SDSPI_DEFAULT_DMA) != ESP_OK) return false;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = cs;
    slot.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mcfg = {
        .format_if_mount_failed = true,   /* blank card in the simulator: format it */
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    mounted = esp_vfs_fat_sdspi_mount(MOUNT, &host, &slot, &mcfg, &card) == ESP_OK;
    return mounted;
}

bool sd_log_ok(void) { return mounted; }

static void path_of(char *out, size_t n, const char *filename)
{
    snprintf(out, n, MOUNT "/%s", filename);
}

bool sd_log_append(const char *filename, const char *line)
{
    if (!mounted) return false;
    char p[64];
    path_of(p, sizeof p, filename);
    FILE *f = fopen(p, "a");
    if (!f) { mounted = false; return false; }   /* card gone: fall back, retry mount later if wanted */
    bool ok = fputs(line, f) >= 0;
    if (ok && line[strlen(line) - 1] != '\n') fputc('\n', f);
    fclose(f);
    return ok;
}

int sd_queue_count(const char *filename)
{
    if (!mounted) return 0;
    char p[64];
    path_of(p, sizeof p, filename);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    int n = 0, c;
    while ((c = fgetc(f)) != EOF) if (c == '\n') n++;
    fclose(f);
    return n;
}

int sd_queue_flush(const char *filename, bool (*send)(const char *line), int max_lines)
{
    if (!mounted) return 0;
    char p[64], tmp[64];
    path_of(p, sizeof p, filename);
    path_of(tmp, sizeof tmp, "queue.tmp");
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    FILE *rest = fopen(tmp, "w");
    if (!rest) { fclose(f); return 0; }

    char line[320];
    int sent = 0;
    bool stopped = false;
    while (fgets(line, sizeof line, f)) {
        if (!stopped && sent < max_lines && send(line)) {
            sent++;
        } else {
            stopped = true;              /* keep this and everything after it */
            fputs(line, rest);
        }
    }
    fclose(f);
    fclose(rest);
    unlink(p);
    if (stopped) rename(tmp, p); else unlink(tmp);
    return sent;
}
