#include "Config/pins.h"
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "freertos/task.h"
#include "App/APP_ATUADOR.h"

static bool rele1, rele2;
static comandos receivedData;

void task_ATUADOR ( void *pvParameter )
{    
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 1000 );
    xRELAYHandlingTask = xTaskGetCurrentTaskHandle();
    QueueComandos = xQueueCreate(10, sizeof(comandos));

    gpio_pad_select_gpio(ATUADOR);
    gpio_set_direction(ATUADOR, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(ATUADOR, true);
    gpio_pad_select_gpio(ATUADOR_1);
    gpio_set_direction(ATUADOR_1, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(ATUADOR_1, true);


    for(;;)
    {

        if (xQueueReceive(QueueComandos, &receivedData, portMAX_DELAY) == pdPASS) {
            
            if(receivedData.rele1 == 1){
                gpio_set_level(ATUADOR, false);
                rele1=false;
            }else{ 
                if(receivedData.rele1 == 0){
                gpio_set_level(ATUADOR, true);
                rele1=true;
                }else{
                    if(receivedData.rele1 == 2){
                        gpio_set_level(ATUADOR, !rele1);
                        rele1=!rele1;
                        }
                }
            }
            if(receivedData.rele2 == 1){
                gpio_set_level(ATUADOR_1, false);
                rele2=false;
            }else{ 
                if(receivedData.rele2 == 0){
                gpio_set_level(ATUADOR_1, true);
                rele2=false;
                }else{
                    if(receivedData.rele1 == 2){
                        gpio_set_level(ATUADOR_1, !rele2);
                        rele2=!rele2;
                    }
                
                }
            }
        }
    }
}

void inicia_automacao(void){
    if(xRELAYHandlingTask==NULL)
        xTaskCreate(&task_ATUADOR, "RELAY_TASK", 6*1024, NULL, 5, &xRELAYHandlingTask);
}

void finaliza_automacao(void){
    #ifdef DEBUG
    ESP_LOGI("RELE","FINALIZANDO ATUADORES");
    #endif
    if(xRELAYHandlingTask==NULL){
        #if DEBUG
            ESP_LOGI("RELE","NAO FOI NECESSARIO");
        #endif
    }
    else{
        vTaskDelete(xRELAYHandlingTask);
        xRELAYHandlingTask = NULL;
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
    ESP_LOGI("RELE","FINALIZOU");
}