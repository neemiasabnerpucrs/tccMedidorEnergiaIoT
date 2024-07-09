#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Drivers/Drivers_I2C.h"
#include "Drivers/Drivers_RTC.h"


esp_err_t i2c_master_init(i2c_port_t port, int sda, int scl)
{
        i2c_config_t i2c_config = {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = sda,
                .scl_io_num = scl,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master.clk_speed = I2C_FREQ_HZ
        };
        i2c_param_config(port, &i2c_config);
        return i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
}

esp_err_t i2c_dev_read(uint8_t adress, const void *out_data, size_t out_size, 
        void *in_data, size_t in_size)
{
    if (!in_data || !in_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if(cmd==NULL){
        vTaskDelay(4);
        cmd = i2c_cmd_link_create();
    }

    if (out_data && out_size)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (adress << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (void *)out_data, out_size, true);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (adress << 1) | 1, true);
    i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t res = 
        i2c_master_cmd_begin(I2C_PORT, cmd, I2CDEV_TIMEOUT / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return res;
}

esp_err_t i2c_dev_write(uint8_t adress, const void *out_reg, size_t out_reg_size,
         const void *out_data, size_t out_size)
{
    if (!out_data || !out_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (adress << 1) | I2C_MASTER_WRITE, true);
    if (out_reg && out_reg_size)
        i2c_master_write(cmd, (void *)out_reg, out_reg_size, true);
    i2c_master_write(cmd, (void *)out_data, out_size, true);
    i2c_master_stop(cmd);
    esp_err_t res = i2c_master_cmd_begin(I2C_PORT, cmd, I2CDEV_TIMEOUT / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return res;
}

uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

esp_err_t update_register(uint8_t adress, uint8_t reg, uint8_t val)
{
    uint8_t old;

    i2c_dev_read_reg(adress, reg, &old, 1);
    uint8_t buf = old | val;
    esp_err_t res = i2c_dev_write_reg(adress, reg, &buf, 1);
    return res;
}