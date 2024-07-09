#ifndef MEMORY_FLASH_H
#define MEMORY_FLASH_H

typedef union Float32
{
    float   f32;
    struct html
    {
        uint8_t Low1;
        uint8_t Low2; 
        uint8_t High1;
        uint8_t High2;
    }word;
}f32word;

#define SOMATORIO_MEMORIA 367

union vetorzao{
    char todo[SOMATORIO_MEMORIA];
    struct
    {
        char wifi_ssid[15];
        char wifi_pass[30];
        char mqtt_user[50];
        char mqtt_pass[70];
        char mqtt_id[30];
        char TLS;
        char mqtt_broker[50];
        uint16_t mqtt_port;
        char mqtt_topic[50];
        char mqtt_topic_recv[50];
        uint8_t qos;
        uint8_t intervalo[2];
        uint8_t limiares[16];
    } filtrado;
};

#define STORAGE_NVS     "storage"
#define STORAGE_PAYLOAD "buffer"

bool return_limiares(float *vetor);
bool get_flash();
bool save_flash();
bool preenche_tudo(char *tudo);
uint16_t return_blob_quantidade(void);
bool verifica_blob(void);
bool salva_blob(void);
bool limpa_blob(void);
bool recupera_blob(uint16_t *blob_rec);
extern union vetorzao memory;

#endif /* MEMORY_FLASH_H */
