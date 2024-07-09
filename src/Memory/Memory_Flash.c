#include <stdio.h>
#include <string.h>
#include "esp_types.h"
#include "esp_log.h"
#include "esp_system.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "Memory/Memory_Flash.h"
#include "Memory/Memory_Med.h"

union vetorzao memory;
static uint16_t blob_quantidade=0;

bool get_flash(){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle_t my_handle;
    err = nvs_open(STORAGE_NVS, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        if(err == ESP_ERR_NVS_NOT_FOUND){
                err = nvs_open(STORAGE_NVS, NVS_READWRITE, &my_handle);
                if (err != ESP_OK) {
                    ESP_LOGE("NVSSECURITY","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                }else{
                    ESP_LOGI("NVSECURITY","CRIANDO PARTICAO...");
                    nvs_set_u8(my_handle,"first",1);
                    nvs_commit(my_handle);
                    esp_restart();
                }
        }
    } else {
        ESP_LOGI("NVS","LENDO FLASH...");

        size_t required_size = 0;
        err = nvs_get_blob(my_handle, "valores", NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
        err = nvs_get_blob(my_handle, "valores", memory.todo,&required_size);
        switch (err) {
            case ESP_OK:
                ESP_LOGI("NVS","VALORES RESGATADOS");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE("NVS","VALORES NÃO ENCONTRADOS. DEFININDO VALORES DE FABRICA");
                snprintf(memory.filtrado.wifi_ssid,15,"Galaxy");
                snprintf(memory.filtrado.wifi_pass,30,"0102030405");
                memset(memory.filtrado.mqtt_user,0,30);
                memset(memory.filtrado.mqtt_pass,0,30);
                snprintf(memory.filtrado.mqtt_id,30,"esptcc");
                memory.filtrado.TLS = false;
                snprintf(memory.filtrado.mqtt_broker,50,"broker.hivemq.com");
                memory.filtrado.mqtt_port = 1883;
                snprintf(memory.filtrado.mqtt_topic,50,"PUCRS/medicao/state");
                snprintf(memory.filtrado.mqtt_topic_recv,50,"PUCRS/medicao/command");
                memory.filtrado.qos = 0;
                memory.filtrado.intervalo[0] = 5;
                memory.filtrado.intervalo[1] = 0;
                
                memset(memory.filtrado.limiares,0,16);

                nvs_close(my_handle);
                save_flash();
                esp_restart();
                break;
            default :
                ESP_LOGE("NVS","Error (%s) reading!\n", esp_err_to_name(err));
            }
    }
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    ESP_LOGI("NVS", "MEMORIAS %d %d %d",nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    nvs_close(my_handle);
    return true;
}


bool save_flash(){
    nvs_handle_t my_handle;
    extern union vetorzao memory;
    esp_err_t err = nvs_open(STORAGE_NVS, NVS_READWRITE, &my_handle);
    if(nvs_set_blob(my_handle, "valores", memory.todo,SOMATORIO_MEMORIA) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO SALVAR FLASH");
    if(nvs_commit(my_handle) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO COMMITAR FLASH");
    nvs_close(my_handle);
    ESP_LOGE("NVS","Gravando na Flash");
    esp_restart();
    return true;
}

bool update_wifi(char* iwifi_ssid, char* iwifi_pass){
    snprintf(memory.filtrado.wifi_ssid,15,"%s",iwifi_ssid);
    snprintf(memory.filtrado.wifi_pass,30,"%s",iwifi_pass);
    return true;
}

bool update_mqtt_auth(char* imqtt_user, char* imqtt_pass, char* imqtt_id){
    snprintf(memory.filtrado.mqtt_user,30,"%s",imqtt_user);
    snprintf(memory.filtrado.mqtt_pass,30,"%s",imqtt_pass);
    snprintf(memory.filtrado.mqtt_id,30,"%s",imqtt_id);
    return true;
}

bool update_mqtt(char* imqtt_broker, char* imqtt_topic, uint16_t iport, bool iTLS, uint8_t iqos){
    snprintf(memory.filtrado.mqtt_broker,30,"%s",imqtt_broker);
    snprintf(memory.filtrado.mqtt_topic,30,"%s",imqtt_topic);
    memory.filtrado.mqtt_port = iport;
    memory.filtrado.TLS = iTLS;
    memory.filtrado.qos = iqos;
    return true;
}

bool return_limiares(float *vetor){

    f32word aux;
    aux.f32 = 84.5;
    for(int i=0;i<16;i=i+4){
        aux.word.High1 = memory.filtrado.limiares[i];
        aux.word.High2 = memory.filtrado.limiares[i+1];
        aux.word.Low1 = memory.filtrado.limiares[i+2];
        aux.word.Low2 = memory.filtrado.limiares[i+3];
        vetor[(int)i/4] = aux.f32;
    }

    return true;
}

bool preenche_tudo(char *tudo){
    for(uint16_t i=0;i<SOMATORIO_MEMORIA;i++){
        memory.todo[i] = tudo[i];
    }

    ESP_LOG_BUFFER_HEX("BYTES",memory.filtrado.limiares,16);
    float limites[4];
    return_limiares(limites);
    ESP_LOGI("HEXA","%f",limites[0]);

    return true;
}

bool salva_blob(void){
    
    char nome[30];
    uint16_t blob[11];
    blob_quantidade++;
    snprintf(nome,30,"blob_%d",blob_quantidade);
    ESP_LOGI("NVS", "SALANDO %s", nome);
    uint16_t *tensao = retorna_tensoes();
    uint32_t *corrente = retorna_correntes();
    uint32_t *potencia = retorna_potencias();
    uint32_t *energia = retorna_energias();
    uint16_t *frequencia = retorna_frequencias();
    uint16_t *power = retorna_power();
    uint32_t *tempos = retorna_tempos();
    blob[0] = tensao[0];
    blob[1] = (corrente[0] >> 16) & 0xFFFF;
    blob[2] = corrente[0] & 0xFFFF;
    blob[3] = (potencia[0] >> 16) & 0xFFFF;
    blob[4] = potencia[0] & 0xFFFF;
    blob[5] = (energia[0] >> 16) & 0xFFFF;
    blob[6] = energia[0] & 0xFFFF;
    blob[7] = frequencia[0];
    blob[8] = power[0];
    blob[9] = (tempos[0] >> 16) & 0xFFFF;
    blob[10] = tempos[0] & 0xFFFF;

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_PAYLOAD, NVS_READWRITE, &my_handle);
    if(nvs_set_blob(my_handle, nome, &blob,sizeof(uint16_t)*11) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO SALVAR BLOB");
    if(nvs_set_u16(my_handle, "quantidade", blob_quantidade) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO SALVAR BLOBQ");
    if(nvs_commit(my_handle) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO COMMITAR FLASH");
    nvs_close(my_handle);
    ESP_LOGE("NVS","Gravando na Flash");
    return true;
}

bool recupera_blob(uint16_t *blob_rec){
    
    char nome[30];
    snprintf(nome,30,"blob_%d",blob_quantidade);
 
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_PAYLOAD, NVS_READWRITE, &my_handle);
    
    size_t required_size = 0;
    err = nvs_get_blob(my_handle, nome, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    err = nvs_get_blob(my_handle, nome, blob_rec,&required_size);

    if(nvs_set_u16(my_handle, "quantidade", blob_quantidade) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO SALVAR BLOBQ");
    if(nvs_commit(my_handle) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO COMMITAR FLASH");
    nvs_close(my_handle);
    ESP_LOGE("NVS","Gravando na Flash");
    return true;
}

bool limpa_blob(void){
    char nome[30];
    snprintf(nome,30,"blob_%d",blob_quantidade);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_PAYLOAD, NVS_READWRITE, &my_handle);
    if(nvs_erase_key(my_handle,nome) != ESP_OK)
        ESP_LOGE("NVS","LIMPAR BLOB %s",nome);
    blob_quantidade--;
    if(nvs_set_u16(my_handle, "quantidade", blob_quantidade) != ESP_OK)
            ESP_LOGE("NVS","ERRO AO SALVAR BLOBQ");
    if(nvs_commit(my_handle) != ESP_OK)
        ESP_LOGE("NVS","ERRO AO COMMITAR FLASH");
    nvs_close(my_handle);
    return true;
}

uint16_t return_blob_quantidade(){
    return blob_quantidade;
}

bool verifica_blob(void){
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(STORAGE_PAYLOAD, NVS_READWRITE, &my_handle);
    if(nvs_get_u16(my_handle, "quantidade", &blob_quantidade) != ESP_OK){
        ESP_LOGE("NVS","BLOBS NÃO ENCONTRADOS");
        blob_quantidade=0;
        if(nvs_set_u16(my_handle, "quantidade", blob_quantidade) != ESP_OK)
            ESP_LOGE("NVS","ERRO AO SALVAR BLOBQ");
        if(nvs_commit(my_handle) != ESP_OK)
            ESP_LOGE("NVS","ERRO AO COMMITAR FLASH");
    }
    nvs_close(my_handle);
    if(blob_quantidade>0)
        ESP_LOGI("NVS", "TEM %d BLOBS", blob_quantidade);
    return 1;
}