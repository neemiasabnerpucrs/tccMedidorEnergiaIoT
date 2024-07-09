
#ifndef APP_WEBSERVER_H
#define APP_WEBSERVER_H

#include <esp_http_server.h>

void inicia_server(void);
void fecha_server(void);

void chavear_log(void);

#endif /* APP_WEBSERVER_H */