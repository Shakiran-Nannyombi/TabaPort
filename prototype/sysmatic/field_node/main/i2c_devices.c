#include "i2c_devices.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_ADDR 0x27
#define RTC_ADDR 0x68
#define IMU_ADDR 0x69
#define I2C_TIMEOUT_MS 50

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t lcd, rtc, imu;
static bool have_lcd, have_rtc, have_imu;

bool i2c_devices_init(int sda_gpio, int scl_gpio)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&cfg, &bus) == ESP_OK;
}

static bool add_dev(uint8_t addr, i2c_master_dev_handle_t *h)
{
    if (!bus || i2c_master_probe(bus, addr, I2C_TIMEOUT_MS) != ESP_OK) return false;
    i2c_device_config_t d = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    return i2c_master_bus_add_device(bus, &d, h) == ESP_OK;
}

/* ---------------- LCD 20x4 via PCF8574: P0=RS P1=RW P2=E P3=backlight P4..7=D4..7 ---------------- */
#define LCD_BL 0x08
#define LCD_EN 0x04

static void pcf_write(uint8_t v)
{
    i2c_master_transmit(lcd, &v, 1, I2C_TIMEOUT_MS);
}

static void lcd_nibble(uint8_t high_nibble, bool rs)
{
    uint8_t v = (high_nibble & 0xF0) | LCD_BL | (rs ? 0x01 : 0x00);
    pcf_write(v | LCD_EN);
    esp_rom_delay_us(2);
    pcf_write(v & (uint8_t)~LCD_EN);
    esp_rom_delay_us(50);
}

static void lcd_send(uint8_t b, bool rs)
{
    lcd_nibble(b & 0xF0, rs);
    lcd_nibble((uint8_t)(b << 4), rs);
}

static void lcd_cmd(uint8_t c)
{
    lcd_send(c, false);
    if (c == 0x01 || c == 0x02) vTaskDelay(pdMS_TO_TICKS(2));
}

bool lcd_init(void)
{
    if (!add_dev(LCD_ADDR, &lcd)) return false;
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_nibble(0x30, false); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_nibble(0x30, false); esp_rom_delay_us(150);
    lcd_nibble(0x30, false); esp_rom_delay_us(150);
    lcd_nibble(0x20, false);              /* 4-bit */
    lcd_cmd(0x28);                        /* 2 lines, 5x8 */
    lcd_cmd(0x08);                        /* display off */
    lcd_cmd(0x01);                        /* clear */
    lcd_cmd(0x06);                        /* entry mode */
    lcd_cmd(0x0C);                        /* display on, no cursor */
    have_lcd = true;
    return true;
}

bool lcd_present(void) { return have_lcd; }

void lcd_print(int row, const char *text)
{
    static const uint8_t row_addr[4] = {0x00, 0x40, 0x14, 0x54};
    if (!have_lcd) return;
    lcd_cmd(0x80 | row_addr[row & 3]);
    char buf[21];
    snprintf(buf, sizeof buf, "%-20.20s", text);
    for (int i = 0; i < 20; i++) lcd_send((uint8_t)buf[i], true);
}

/* ---------------- DS1307 ---------------- */
static uint8_t bcd2bin(uint8_t b) { return (uint8_t)((b >> 4) * 10 + (b & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

bool ds1307_init(void)
{
    have_rtc = add_dev(RTC_ADDR, &rtc);
    return have_rtc;
}

bool ds1307_present(void) { return have_rtc; }

bool ds1307_read(struct tm *t)
{
    if (!have_rtc) return false;
    uint8_t reg = 0x00, d[7];
    if (i2c_master_transmit_receive(rtc, &reg, 1, d, 7, I2C_TIMEOUT_MS) != ESP_OK) return false;
    if (d[0] & 0x80) return false;        /* CH bit: oscillator halted, time invalid */
    memset(t, 0, sizeof *t);
    t->tm_sec = bcd2bin(d[0] & 0x7F);
    t->tm_min = bcd2bin(d[1] & 0x7F);
    if (d[2] & 0x40) {                    /* 12 h mode */
        int h = bcd2bin(d[2] & 0x1F) % 12;
        t->tm_hour = h + ((d[2] & 0x20) ? 12 : 0);
    } else {
        t->tm_hour = bcd2bin(d[2] & 0x3F);
    }
    t->tm_wday = (d[3] & 0x07) - 1;
    t->tm_mday = bcd2bin(d[4] & 0x3F);
    t->tm_mon = bcd2bin(d[5] & 0x1F) - 1;
    t->tm_year = bcd2bin(d[6]) + 100;     /* 20xx */
    return t->tm_mday >= 1 && t->tm_mon >= 0 && t->tm_mon < 12;
}

bool ds1307_write(const struct tm *t)
{
    if (!have_rtc) return false;
    uint8_t w[8] = {
        0x00,
        bin2bcd((uint8_t)t->tm_sec),       /* CH = 0: running */
        bin2bcd((uint8_t)t->tm_min),
        bin2bcd((uint8_t)t->tm_hour),      /* 24 h mode */
        bin2bcd((uint8_t)(t->tm_wday + 1)),
        bin2bcd((uint8_t)t->tm_mday),
        bin2bcd((uint8_t)(t->tm_mon + 1)),
        bin2bcd((uint8_t)(t->tm_year - 100)),
    };
    return i2c_master_transmit(rtc, w, sizeof w, I2C_TIMEOUT_MS) == ESP_OK;
}

/* ---------------- MPU6050 ---------------- */
bool imu_init(void)
{
    if (!add_dev(IMU_ADDR, &imu)) return false;
    uint8_t wake[2] = {0x6B, 0x00};       /* PWR_MGMT_1: clear sleep */
    have_imu = i2c_master_transmit(imu, wake, 2, I2C_TIMEOUT_MS) == ESP_OK;
    return have_imu;
}

bool imu_present(void) { return have_imu; }

bool imu_tilt_deg(float *deg)
{
    if (!have_imu) return false;
    uint8_t reg = 0x3B, d[6];
    if (i2c_master_transmit_receive(imu, &reg, 1, d, 6, I2C_TIMEOUT_MS) != ESP_OK) return false;
    int16_t ax = (int16_t)((d[0] << 8) | d[1]);
    int16_t ay = (int16_t)((d[2] << 8) | d[3]);
    int16_t az = (int16_t)((d[4] << 8) | d[5]);
    float x = ax / 16384.0f, y = ay / 16384.0f, z = az / 16384.0f;   /* +-2 g range */
    float mag = sqrtf(x * x + y * y + z * z);
    if (mag < 0.1f) return false;
    float c = z / mag;
    if (c > 1.0f) c = 1.0f;
    if (c < -1.0f) c = -1.0f;
    *deg = acosf(c) * 57.2957795f;        /* 0 = probe hanging straight down */
    return true;
}
