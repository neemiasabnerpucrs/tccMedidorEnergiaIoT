#ifndef APP_ATUADOR_H
#define APP_ATUADOR_H

typedef struct comandos
{
    uint8_t rele1;
    uint8_t rele2;
} comandos;

TaskHandle_t xRELAYHandlingTask;
QueueHandle_t QueueComandos;

void inicia_automacao(void);
void finaliza_automacao(void);

#endif /* APP_ATUADOR_H_ */