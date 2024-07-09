#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_types.h"
#include "esp_log.h"
#include "mbcontroller.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/uart_select.h"

#include "Config/defines.h"
#include "Config/pins.h"

#include "esp_system.h"
#include <time.h>
#include <sys/time.h>

#include "App/APP_Atuador.h"
#include "Memory/Memory_Flash.h"

TaskHandle_t TaskHandle_MEDICAO;

static uint16_t voltage[AMOSTRAS];
static uint32_t current[AMOSTRAS];
static uint32_t power[AMOSTRAS];
static uint32_t energy[AMOSTRAS];
static uint16_t frequency[AMOSTRAS];
static uint16_t powerfactory[AMOSTRAS];

static uint32_t tempos[AMOSTRAS];

static float limitesf[4];
static uint16_t limites[4];

bool atualiza_medidas(uint8_t *vetor);

#include "mbcontroller.h"

#define STR(fieldname) ((const char*)( fieldname ))
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

enum {
    MB_DEVICE_ADDR1 = 1,
    MB_SLAVE_COUNT
};

mb_parameter_descriptor_t device_parameters[] = {
      { 0, STR("Valores"), STR("--"), 1, MB_PARAM_INPUT, 0, 9,
                    0, PARAM_TYPE_U32, 4, OPTS( 0,0,0 ), PAR_PERMS_READ_WRITE_TRIGGER }
};

uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters[0]));

void task_MEDICAO( void *pvParameter )
{
    return_limiares(limitesf);
    ESP_LOGI("TAG","%.2f - %.2f - %.2f - %.2f", limitesf[0],limitesf[1],limitesf[2],limitesf[3]);
    for(int i=0;i<4;i++)
        limites[i] = (uint16_t) (limitesf[i]*10.0);
    
    ESP_LOGI("TAG","%d - %d - %d - %d", limites[0],limites[1],limites[2],limites[3]);
    

    vTaskDelay(2000/portTICK_PERIOD_MS);
    mb_communication_info_t comm = {
            .port = UART_NUM_2,
            .mode = MB_MODE_RTU,
            .baudrate = 9600,
            .parity = MB_PARITY_NONE
    };
    void* master_handler = NULL;

    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    err = mbc_master_setup((void*)&comm);
    err = uart_set_pin(UART_NUM_2, RS_TXD, RS_RXD,UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(mbc_master_set_descriptor(&device_parameters[0], num_device_parameters));
    
    err = mbc_master_start();
    if (err != ESP_OK) {
        ESP_LOGE("TAG", "mb controller start fail, err=%x.", err);
    }
    for(;;)
    {
        const mb_parameter_descriptor_t* param_descriptor = NULL;
        uint8_t val_data[50] = {0};
        uint8_t type = 0;

        // Get the information for characteristic cid from data dictionary
        esp_err_t err = mbc_master_get_cid_info(0, &param_descriptor);
        if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
            err = mbc_master_get_parameter(param_descriptor->cid, (char*)param_descriptor->param_key, (uint8_t*)val_data, &type);
            if (err == ESP_OK) {
                //ESP_LOG_BUFFER_HEX("RECEBIDO",val_data,18);
                atualiza_medidas(val_data);

            } else {
                ESP_LOGE("TAG", "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
                                param_descriptor->cid,
                                (char*)param_descriptor->param_key,
                                (int)err,
                                (char*)esp_err_to_name(err));
            }
        } else {
            ESP_LOGE("TAG", "Could not get information for characteristic %d.", 5);
        }

    vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

bool regime_normal = true;


bool atualiza_medidas(uint8_t *vetor){
    for(uint8_t i=1;i<AMOSTRAS;i++){
        tempos[AMOSTRAS-i] = tempos[AMOSTRAS-i-1];
        voltage[AMOSTRAS-i] = voltage[AMOSTRAS-i-1];
        current[AMOSTRAS-i] = current[AMOSTRAS-i-1];
        power[AMOSTRAS-i] = power[AMOSTRAS-i-1];
        energy[AMOSTRAS-i] = energy[AMOSTRAS-i-1];
        frequency[AMOSTRAS-i] = frequency[AMOSTRAS-i-1];
        powerfactory[AMOSTRAS-i] = powerfactory[AMOSTRAS-i-1];
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    tempos[0] = (uint32_t)tv_now.tv_sec;

    voltage[0] = (uint16_t)(vetor[0] + (vetor[1]<<8));
    current[0] = (uint32_t)(vetor[2] + (vetor[3]<<8) + (vetor[4]<<16) +(vetor[5]<<24));
    current[0]*=2;
    power[0] = (uint32_t)(vetor[6] + (vetor[7]<<8) + (vetor[8]<<16) +(vetor[9]<<24));
    power[0]*=2;
    energy[0] = (uint32_t)(vetor[10] + (vetor[11]<<8) + (vetor[12]<<16) +(vetor[13]<<24));
    energy[0]*=2;
    frequency[0] = (uint16_t)(vetor[14] + (vetor[15]<<8));
    powerfactory[0] = (uint16_t)(vetor[16] + (vetor[17]<<8));

    if(voltage[0] > limites[3] || voltage[0] < limites[0]){
        if(regime_normal){
            regime_normal = false;
            comandos recebido;
            recebido.rele1 = 0;
            xQueueSend(QueueComandos, &recebido, portMAX_DELAY);
        }
    }

    if(voltage[0] < limites[2] && voltage[0] > limites[1]){
        if(!regime_normal){
            regime_normal = true;
            comandos recebido;
            recebido.rele1 = 1;
            xQueueSend(QueueComandos, &recebido, portMAX_DELAY);
        }
    }
    return 1;
}

void inicia_medicao(void){
    if(TaskHandle_MEDICAO==NULL)
        xTaskCreate(&task_MEDICAO, "MEDICAO_TASK", 6*STACK_DEFAULT_SIZE, NULL, 5, &TaskHandle_MEDICAO);
    return ESP_OK;
}

uint16_t* retorna_tensoes(){
    return voltage;
}
uint32_t* retorna_correntes(){
    return current;
}
uint32_t* retorna_potencias(){
    return power;
}
uint32_t* retorna_energias(){
    return energy;
}

uint16_t* retorna_frequencias(){
    return frequency;
}
uint16_t* retorna_power(){
    return powerfactory;
}

uint32_t* retorna_tempos(){
    return tempos;
}