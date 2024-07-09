#ifndef APP_LED_H_
#define APP_LED_H_

TaskHandle_t xLEDHandlingTask;

#define LED_CON_LIGADO          0x01
#define LED_CON_DESLIGADO       0x02
#define LED_CON_PISCANDO        0x03

void inicia_led ( void );
#endif /* APP_LED_H_ */