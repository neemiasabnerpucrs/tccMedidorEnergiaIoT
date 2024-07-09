#ifndef MEMORY_MED_H
#define MEMORY_MED_H

void inicia_medicao(void);

uint16_t* retorna_tensoes();
uint32_t* retorna_correntes();
uint32_t* retorna_potencias();
uint32_t* retorna_energias();
uint16_t* retorna_frequencias();
uint16_t* retorna_power();

uint32_t* retorna_tempos();

#endif