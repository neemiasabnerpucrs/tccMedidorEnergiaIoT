#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "esp_tls_crypto.h"
#include "esp_tls.h"
#include "cJSON.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "App/APP_MQTT.h"
#include "App/APP_Atuador.h"
#include "App/APP_LED.h"

#include "Memory/Memory_Flash.h"
#include "Memory/Memory_Med.h"

#include "Config/defines.h"

TaskHandle_t TaskHandle_MQTT = NULL;

esp_mqtt_client_handle_t client;
#define TAMANHO_PAYLOAD 512
static char mensagem[TAMANHO_PAYLOAD];
static bool conectado = false;
static char *TAG = "MQTT";

bool monta_json(cJSON *corpo,bool memoria){
    cJSON *payload = cJSON_CreateObject();
    uint16_t *tensao = retorna_tensoes();
    uint32_t *corrente = retorna_correntes();
    uint32_t *potencia = retorna_potencias();
    uint32_t *energia = retorna_energias();
    uint16_t *frequencia = retorna_frequencias();
    uint16_t *power = retorna_power();
    uint32_t *tempos = retorna_tempos();
    
    if(!memoria){
        cJSON_AddItemToObject(payload,"tensao",cJSON_CreateNumber(tensao[0]/10.0));
        cJSON_AddItemToObject(payload,"corrente",cJSON_CreateNumber(corrente[0]/1000.0));
        cJSON_AddItemToObject(payload,"potencia",cJSON_CreateNumber(potencia[0]/10.0));
        cJSON_AddItemToObject(payload,"energia",cJSON_CreateNumber(energia[0]));
        cJSON_AddItemToObject(payload,"freq",cJSON_CreateNumber(frequencia[0]/10.0));
        cJSON_AddItemToObject(payload,"fp",cJSON_CreateNumber(power[0]/100.0));
        cJSON_AddItemToObject(corpo,"data",payload);
        char tempo[11];
        snprintf(tempo,11,"%d",tempos[0]);
        tempo[10]='\0';
        cJSON_AddItemToObject(corpo,"time",cJSON_CreateString(tempo));
    }else{
        ESP_LOGI("NVS","ABRIR BLOB");
        uint16_t blob[11];
        recupera_blob(blob);
        cJSON_AddItemToObject(payload,"tensao",cJSON_CreateNumber(blob[0]/10.0));
        cJSON_AddItemToObject(payload,"corrente",cJSON_CreateNumber((blob[2] + (blob[1]<<16))/1000.0));
        cJSON_AddItemToObject(payload,"potencia",cJSON_CreateNumber((blob[4] + (blob[3]<<16))/10.0));
        cJSON_AddItemToObject(payload,"energia",cJSON_CreateNumber((blob[6] + (blob[5]<<16))));
        cJSON_AddItemToObject(payload,"freq",cJSON_CreateNumber(blob[7]/10.0));
        cJSON_AddItemToObject(payload,"fp",cJSON_CreateNumber(blob[8]/100.0));
        cJSON_AddItemToObject(corpo,"data",payload);
        char tempo[11];
        snprintf(tempo,11,"%d",blob[10] + (blob[9]<<16));
        tempo[10]='\0';
        cJSON_AddItemToObject(corpo,"time",cJSON_CreateString(tempo));
    }

    return 1;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
            int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG," EVENTO %d", event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    
    case MQTT_EVENT_CONNECTED:
        conectado=true;
        if(strcmp(memory.filtrado.mqtt_topic_recv,"")!=0)
            esp_mqtt_client_subscribe(client,memory.filtrado.mqtt_topic_recv,0);
        xTaskNotify( xLEDHandlingTask, LED_CON_LIGADO, eSetValueWithOverwrite);
        break;
    case MQTT_EVENT_DISCONNECTED:
        conectado=false;
        if(strcmp(memory.filtrado.mqtt_topic_recv,"")!=0)
            esp_mqtt_client_unsubscribe(client,memory.filtrado.mqtt_topic_recv);
        xTaskNotify( xLEDHandlingTask, LED_CON_PISCANDO, eSetValueWithOverwrite);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        xTaskNotify( xLEDHandlingTask, LED_CON_LIGADO, eSetValueWithOverwrite);
        break;
    case MQTT_EVENT_DATA:
        vTaskDelay(10/portTICK_PERIOD_MS);
        char datinha[256];
        snprintf(datinha,256,"%.*s",event->data_len,event->data);
        ESP_LOGI(TAG, "%s",datinha);
        comandos recebido;

        cJSON *novo = cJSON_Parse(datinha);
        cJSON *payload = cJSON_GetObjectItem(novo,"payload");
        cJSON *cport = cJSON_GetObjectItem(payload,"rele1");
        if(cport!=NULL){
            ESP_LOGI(TAG, "RECEBEU 1");
            uint16_t porta = 4;
            porta = cJSON_GetNumberValue(cport);
            switch (porta)
            {
            case 0: ESP_LOGI(TAG, "ZERO");
                    recebido.rele1 = 0;
            break;
            case 1: ESP_LOGI(TAG, "UM");
                    recebido.rele1 = 1;
            ;break;
            case 2: ESP_LOGI(TAG, "DOIS");
                    recebido.rele1 = 2;
            break;
            default:break;
            }
        }
        cJSON *cport2 = cJSON_GetObjectItem(payload,"rele2");
        if(cport2!=NULL){
            ESP_LOGI(TAG, "RECEBEU 2");
            uint16_t porta = 4;
            porta = cJSON_GetNumberValue(cport2);
            switch (porta)
            {
            case 0: ESP_LOGI(TAG, "ZERO");
                    recebido.rele2 = 0;
            break;
            case 1: ESP_LOGI(TAG, "UM");
                    recebido.rele2 = 1;
            ;break;
            case 2: ESP_LOGI(TAG, "DOIS");
                    recebido.rele2 = 2;
            break;
            default:break;
            }
        }
        xQueueSend(QueueComandos, &recebido, portMAX_DELAY);
        cJSON_Delete(novo);

        break;
    case MQTT_EVENT_ERROR:
        xTaskNotify( xLEDHandlingTask, LED_CON_PISCANDO, eSetValueWithOverwrite);
        break;
    default:
        break;
    }
}

void task_MQTT_TLS( void *pvParameter )
{
    char  stringBroker[60];
    if(memory.filtrado.TLS){
        strcpy(stringBroker,"mqtts://");
    }else{
        strcpy(stringBroker,"mqtt://");
    }
    strcat(stringBroker,memory.filtrado.mqtt_broker);
    esp_mqtt_client_config_t mqtt_config = {
        .client_id = memory.filtrado.mqtt_id,
        .uri=stringBroker,
        .username=memory.filtrado.mqtt_user,
        .password=memory.filtrado.mqtt_pass,
        .port = memory.filtrado.mqtt_port,
        .keepalive = 60,
        .reconnect_timeout_ms = 40000,
        .network_timeout_ms = 40000,
    };



    ESP_LOGI(TAG,"URL = %s:%d", mqtt_config.uri,mqtt_config.port);
    ESP_LOGI(TAG,"user = %s@%s", mqtt_config.username,mqtt_config.password);
    
    ESP_LOGI(TAG,"ID = %s:%d", mqtt_config.client_id,mqtt_config.port);


    mqtt_config.task_stack = 5 * STACK_DEFAULT_SIZE;
    client = esp_mqtt_client_init(&mqtt_config);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    vTaskDelay(30000/portTICK_PERIOD_MS);
    esp_mqtt_client_start(client);
    time_t t_send,now;
    time(&now);
    time(&t_send);
    verifica_blob();
    ESP_LOGI(TAG,"Intervalo: %d",((uint16_t) (memory.filtrado.intervalo[0]) + (uint16_t) (memory.filtrado.intervalo[1]<<8)));
    for(;;)
    {
        if(difftime(now,t_send) >= (((uint16_t) (memory.filtrado.intervalo[0]) + (uint16_t) (memory.filtrado.intervalo[1]<<8)) * 60)){
            if(conectado){
                cJSON *root = cJSON_CreateObject();
                monta_json(root, 0);
                cJSON_PrintPreallocated(root,mensagem,TAMANHO_PAYLOAD,0);
                if(esp_mqtt_client_publish(client,memory.filtrado.mqtt_topic,mensagem,0,0,0)==-1){
                    salva_blob();
                }
                cJSON_Delete(root);
                memset(mensagem,0,TAMANHO_PAYLOAD);
            }else{
                salva_blob();
            }
            time(&t_send);
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
        if(conectado){
            verifica_blob();
            if(return_blob_quantidade()>0){
                ESP_LOGI(TAG,"IMPRIMINDO MEMORIA");
                cJSON *root = cJSON_CreateObject();
                monta_json(root, 1);
                cJSON_PrintPreallocated(root,mensagem,TAMANHO_PAYLOAD,0);
                if(esp_mqtt_client_publish(client,memory.filtrado.mqtt_topic,mensagem,0,0,0) != -1)
                    limpa_blob();
                cJSON_Delete(root);
                memset(mensagem,0,TAMANHO_PAYLOAD);
            }    
        }
        time(&now);
    }
}

void inicia_MQTT(void){
    if(TaskHandle_MQTT==NULL)
        xTaskCreate(&task_MQTT_TLS, "MQTT_TLS_TASK", 5*STACK_DEFAULT_SIZE, NULL, 5, &TaskHandle_MQTT);
}

void kill_MQTT(void){
    if(TaskHandle_MQTT==NULL){

    }
    else{
        if(client==NULL){

        }else{
            esp_mqtt_client_destroy(client);
        }
        vTaskDelete(TaskHandle_MQTT);
        TaskHandle_MQTT = NULL;
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
    ESP_LOGI(TAG,"FINALIZOU");
}