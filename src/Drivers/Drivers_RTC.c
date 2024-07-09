#include <string.h>
#include <time.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "Config/pins.h"
#include "Config/defines.h"

#include "esp_sntp.h"

#include "Drivers/Drivers_RTC.h"

TaskHandle_t TaskHandle_RTC = NULL;

void time_sync_notification_cb(struct timeval *tv)
{
    time_t esp;
    time(&esp);
    struct tm esp_s;
    setenv("TZ", "UTC+3", 1);
    tzset();
    esp+=10800; //Corrigir Fuso Horario
    localtime_r(&esp,&esp_s);
    char timeinfo[64];
    strftime(timeinfo, sizeof(timeinfo), "%c", &esp_s);
    ESP_LOGI("SNTP", "Horario Ajustado para: %s", timeinfo);
    ds1307_set_time(&esp_s);
}

static void configure_rtc(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "a.st1.ntp.br");
    sntp_init();
}

esp_err_t ds1307_set_time(const struct tm *time)
{
    CHECK_ARG(time);

    uint8_t buf[7] = {
        dec2bcd(time->tm_sec),
        dec2bcd(time->tm_min),
        dec2bcd(time->tm_hour),
        dec2bcd(time->tm_wday + 1),
        dec2bcd(time->tm_mday),
        dec2bcd(time->tm_mon + 1),
        dec2bcd(time->tm_year)
    };
    i2c_dev_write_reg(DS1307_ADDR, TIME_REG, buf, sizeof(buf));

    return ESP_OK;
}

esp_err_t ds1307_get_time(struct tm *time)
{
    CHECK_ARG(time);

    uint8_t buf[7];

    i2c_dev_read_reg(DS1307_ADDR, TIME_REG, buf, 7);

    time->tm_sec = bcd2dec(buf[0] & SECONDS_MASK);
    time->tm_min = bcd2dec(buf[1]);
    if (buf[2] & HOUR12_BIT)
    {
        // RTC in 12-hour mode
        time->tm_hour = bcd2dec(buf[2] & HOUR12_MASK) - 1;
        if (buf[2] & PM_BIT)
            time->tm_hour += 12;
    }
    else
        time->tm_hour = bcd2dec(buf[2] & HOUR24_MASK);
    time->tm_wday = bcd2dec(buf[3]) - 1;
    time->tm_mday = bcd2dec(buf[4]);
    time->tm_mon  = bcd2dec(buf[5]) - 1;
    time->tm_year = bcd2dec(buf[6]);
    ESP_LOGI("RTC", "GET - %d",(int)mktime(time));
    return ESP_OK;
}

esp_err_t ds1307_init_desc( void )
{
    return i2c_master_init(I2C_PORT, PIN_RTC_SDA, PIN_RTC_SCL);
}

void task_RTC( void *pvParameter ){
    ds1307_init_desc();
    struct tm timeinfo;
    ds1307_get_time(&timeinfo);
    char strftime_buf[64];

    struct timezone teste;
    teste.tz_minuteswest = 0;
    struct timeval timestamp;
    timestamp.tv_sec= mktime(&timeinfo);

    settimeofday(&timestamp,&teste);
    // adjtime(&timestamp,&teste);

    time_t esp;
    time(&esp);
    struct tm esp_s;
    setenv("TZ", "UTC+3", 1);
    tzset();
    localtime_r(&esp,&esp_s);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &esp_s);
    ESP_LOGI("RTC", "HORARIO PELO RTC: %s", strftime_buf);
    configure_rtc();
    vTaskDelete(NULL);
}

void inicia_RTC(void){
    if(TaskHandle_RTC==NULL)
        xTaskCreate(&task_RTC, "RTC_TASK", 3*STACK_DEFAULT_SIZE, NULL, 5, &TaskHandle_RTC);
    return ESP_OK;
}