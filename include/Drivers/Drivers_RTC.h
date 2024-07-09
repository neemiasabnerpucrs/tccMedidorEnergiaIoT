#ifndef DRIVERS_DS1307_H__
#define DRIVERS_DS1307_H__

#include "Drivers_I2C.h"
#include <time.h>

#define DS1307_ADDR             0x68 

#define RAM_SIZE                56

#define CHECK_ARG(ARG) do { if (!ARG) return ESP_ERR_INVALID_ARG; } while (0)


#define TIME_REG                0
#define CONTROL_REG             7
#define RAM_REG                 8

#define CH_BIT                  (1 << 7)
#define HOUR12_BIT              (1 << 6)
#define PM_BIT                  (1 << 5)
#define SQWE_BIT                (1 << 4)
#define OUT_BIT                 (1 << 7)

#define CH_MASK                 0x7f
#define SECONDS_MASK            0x7f
#define HOUR12_MASK             0x1f
#define HOUR24_MASK             0x3f
#define SQWEF_MASK              0x03
#define SQWE_MASK               0xef
#define OUT_MASK                0x7

esp_err_t ds1307_set_time(const struct tm *time);
esp_err_t ds1307_get_time(struct tm *time);
esp_err_t ds1307_init_desc( void );

void inicia_RTC(void);

#endif /* DRIVERS_DS1307_H__ */