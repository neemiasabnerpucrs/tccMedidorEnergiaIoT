#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_app_trace.h"

#include "Drivers/Drivers_Wifi.h"
#include "Drivers/Drivers_RTC.h"

#include "App/APP_MQTT.h"
#include "Memory/Memory_Med.h"
#include "Memory/Memory_Flash.h"
#include "App/APP_Webserver.h"
#include "App/APP_Atuador.h"
#include "App/APP_LED.h"

void monitoring_task(void *pvParameter)
{
    for(;;)
    {
        ESP_LOGI( "MAIN","free heap: %d",esp_get_free_heap_size() );
        vTaskDelay( 5000 / portTICK_PERIOD_MS );
    }
}

void app_main() {
    //xTaskCreate(&monitoring_task, "task_monitoracao", 2560, NULL, 5, NULL);
    get_flash();
    inicia_RTC();
    chavear_log();
    inicia_led();
    inicia_medicao();
    wifi_init_apsta();
    inicia_server();
    inicia_automacao();
    inicia_MQTT();
}