#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "driver/uart.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "App/APP_LED.h"

#include "Config/pins.h"
#include "Config/defines.h"

void task_LED ( void *pvParameter )
{    
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 1750 );

    xLEDHandlingTask = NULL;

    xLEDHandlingTask = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;

    gpio_pad_select_gpio(LED0);

    gpio_set_direction(LED0, GPIO_MODE_OUTPUT);
    gpio_set_level(LED0, true);

    uint8_t led_0 = 2; //Comportamento do LED, 0 - DESLIGADO 1 - LIGADO 2 - PISCANDO
    bool b_led_0=0;
    
    for(;;)
    {

        xTaskNotifyWait( pdFALSE,    
                        ULONG_MAX,
                        &ulNotifiedValue,
                        xMaxBlockTime);

        switch (ulNotifiedValue)
        {
            case LED_CON_LIGADO:
                led_0=0;break;
            case LED_CON_DESLIGADO:
                led_0=1;break;
            case LED_CON_PISCANDO:
                led_0=2;break;

            default:
                break;
        }
        switch (led_0){
            case 0: b_led_0=0;break;
            case 1: b_led_0=1;break;
            case 2: b_led_0=!b_led_0;break;
            default:break;}

        gpio_set_level(LED0, !b_led_0);
    }
}

void inicia_led(void){
    if(xLEDHandlingTask==NULL)
        xTaskCreate(&task_LED, "LED_TASK", 6*1024, NULL, 5, &xLEDHandlingTask);
}