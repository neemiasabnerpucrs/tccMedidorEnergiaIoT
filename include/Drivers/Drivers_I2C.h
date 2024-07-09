#ifndef DRIVER_I2C_H_
#define DRIVER_I2C_H_

#include "driver/i2c.h"

#define I2C_FREQ_HZ             100000
#define I2CDEV_TIMEOUT          1000
#define I2C_PORT                I2C_NUM_0

typedef struct {
    i2c_port_t port;            
    uint8_t addr;               
    gpio_num_t sda_io_num;      
    gpio_num_t scl_io_num;     
    uint32_t clk_speed;
} i2c_dev_t;

esp_err_t i2c_master_init(i2c_port_t port, int sda, int scl);
esp_err_t i2c_dev_read(uint8_t adress, const void *out_data, size_t out_size, void *in_data, size_t in_size);
esp_err_t i2c_dev_write(uint8_t adress, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size);

inline esp_err_t i2c_dev_read_reg(uint8_t adress, uint8_t reg, void *in_data, size_t in_size)
{
    return i2c_dev_read(adress ,&reg, 1, in_data, in_size);
}

inline esp_err_t i2c_dev_write_reg(uint8_t adress, uint8_t reg, const void *out_data, size_t out_size)
{
    return i2c_dev_write(adress, &reg, 1, out_data, out_size);
}

uint8_t bcd2dec(uint8_t val);
uint8_t dec2bcd(uint8_t val);
esp_err_t update_register(uint8_t adress, uint8_t reg, uint8_t val);
#endif /* DRIVER_I2C_H_ */