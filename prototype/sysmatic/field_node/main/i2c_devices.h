#pragma once
/* I2C peripherals on the field node: 20x4 LCD (PCF8574 backpack), DS1307 RTC, MPU6050 tilt. */
#include <stdbool.h>
#include <time.h>

bool i2c_devices_init(int sda_gpio, int scl_gpio);

/* LCD 20x4 at 0x27 */
bool lcd_init(void);
bool lcd_present(void);
void lcd_print(int row, const char *text);   /* pads or truncates to 20 columns */

/* DS1307 at 0x68 */
bool ds1307_init(void);
bool ds1307_present(void);
bool ds1307_read(struct tm *out);
bool ds1307_write(const struct tm *t);

/* MPU6050 at 0x69 (AD0 tied to 3V3 so it does not clash with the RTC) */
bool imu_init(void);
bool imu_present(void);
bool imu_tilt_deg(float *deg_from_vertical);
