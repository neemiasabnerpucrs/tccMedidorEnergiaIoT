#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"

#include "mdns.h"
#include "httpd_basic_auth.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "esp_https_ota.h"
#include "esp_http_server.h"

#include "App/APP_Webserver.h"

#include "Config/defines.h"
#include "Memory/Memory_Med.h"
#include "Memory/Memory_Flash.h"


const char *TAG = "WEBS";
const char *TAGO = "OTA";


cJSON *saida_log;

static httpd_handle_t global_server = NULL;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern const uint8_t config_html_start[] asm("_binary_config_html_start");
extern const uint8_t config_html_end[]   asm("_binary_config_html_end");


extern const uint8_t charts_js_start[] asm("_binary_charts_js_start");
extern const uint8_t charts_js_end[]   asm("_binary_charts_js_end");

extern const uint8_t hammer_start[]  asm("_binary_hammer_min_js_start");
extern const uint8_t hammer_end[]    asm("_binary_hammer_min_js_end");

extern const uint8_t zoom_start[]  asm("_binary_zoom_min_js_start");
extern const uint8_t zoom_end[]    asm("_binary_zoom_min_js_end");

static void disconnect_handler(void* arg, esp_event_base_t event_base,
int32_t event_id, void* event_data);
static void connect_handler(void* arg, esp_event_base_t event_base,
int32_t event_id, void* event_data);

void stop_webserver(httpd_handle_t server);

#define SCRATCH_BUFSIZE  8192
#include "esp_vfs.h"

struct file_server_data {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
};

char rcv_buffer[200];

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    
	switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
				strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
    }
    return ESP_OK;
}

uint8_t flag_ota;

void check_update_task(void *pvParameter) {
    vTaskDelay(10000/portTICK_PERIOD_MS);
	flag_ota = 0;
	while(1) {
        int fw_version = FIRMWARE_MAJOR*100 + FIRMWARE_MINOR*10 + FIRMWARE_RELEASE;
		// configure the esp_http_client
		esp_http_client_config_t config = {
        .url = UPDATE_JSON_URL,
        .event_handler = _http_event_handler,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
	
		// downloading the json file
		esp_err_t err = esp_http_client_perform(client);
		if(err == ESP_OK) {
			
			// parse the json file	
			cJSON *json = cJSON_Parse(rcv_buffer);
			if(json == NULL) ESP_LOGI(TAGO,"downloaded file is not a valid json, aborting...\n");
			else {	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
				cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
				
				// check the version
				if(!cJSON_IsNumber(version)) ESP_LOGI(TAGO,"unable to read new version, aborting...\n");
				else {
					
					int new_version = version->valueint;
					if(new_version > fw_version) {
						flag_ota = 1;
						ESP_LOGI(TAGO,"current firmware version (%d) is lower than the available one (%d), upgrading...\n", fw_version, new_version);
						if(cJSON_IsString(file) && (file->valuestring != NULL)) {
							ESP_LOGI(TAGO,"downloading and installing new firmware (%s)...\n", file->valuestring);
							
							esp_http_client_config_t ota_client_config = {
								.url = file->valuestring,
								.cert_pem = ""
							};
							esp_err_t ret = esp_https_ota(&ota_client_config);
							if (ret == ESP_OK) {
								ESP_LOGI(TAGO,"OTA OK, restarting...\n");
								esp_restart();
							} else {
								ESP_LOGI(TAGO,"OTA failed...\n");
                                break;
							}
						}
						else ESP_LOGI(TAGO,"unable to read the new file name, aborting...\n");
                        break;
					}
					else {
                        ESP_LOGI(TAGO,"current firmware version (%d) is greater or equal to the available one (%d), nothing to do...\n", fw_version, new_version);
                    }
				}
			}
		}
		else ESP_LOGI(TAGO,"unable to download the json file, aborting...\n");
		// cleanup
		esp_http_client_cleanup(client);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

esp_err_t get_charts_js( httpd_req_t *req ){
	httpd_resp_send( req, (const char*) charts_js_start, (charts_js_end - charts_js_start)-1);
    return ESP_OK;
}
httpd_uri_t page_charts_js = {
    .uri       = "/charts.js",
    .method    = HTTP_GET,
    .handler   = get_charts_js,
    .user_ctx  = NULL
};

esp_err_t get_hammer_js( httpd_req_t *req ){
	httpd_resp_send( req, (const char*) hammer_start, (hammer_end - hammer_start)-1);
    return ESP_OK;
}
httpd_uri_t page_hammer_js = {
    .uri       = "/hammer.min.js",
    .method    = HTTP_GET,
    .handler   = get_hammer_js,
    .user_ctx  = NULL
};

esp_err_t get_zoom_js( httpd_req_t *req ){
	httpd_resp_send( req, (const char*) zoom_start, (zoom_end - zoom_start)-1);
    return ESP_OK;
}
httpd_uri_t page_zoom_js = {
    .uri       = "/zoom.min.js",
    .method    = HTTP_GET,
    .handler   = get_zoom_js,
    .user_ctx  = NULL
};


esp_err_t webserver_get_handler( httpd_req_t *req ){

    if(httpd_basic_auth(req, "admin", "admin") == ESP_OK) {
        httpd_resp_set_hdr(req, "Cache-Control", "max-age=500");
		httpd_resp_send( req, (const char*) index_html_start, (index_html_end - index_html_start) ); 
	}else{
		httpd_basic_auth_resp_send_401(req);
		httpd_resp_sendstr(req, "Não Autorizado");
		return ESP_FAIL;
	}
    return ESP_OK;
}

httpd_uri_t page_index_html = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = webserver_get_handler,
    .user_ctx  = NULL
};

esp_err_t get_config_js( httpd_req_t *req ){
	httpd_resp_send( req, (const char*) config_html_start, (config_html_end - config_html_start)-1);
    return ESP_OK;
}
httpd_uri_t page_index_js = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = get_config_js,
    .user_ctx  = NULL
};

esp_err_t post_config_js( httpd_req_t *req ){
	char buf[512];
    int ret;
    size_t recv_size = sizeof(buf);
    if ((ret = httpd_req_recv(req, buf,recv_size)) <= 0) {
        return ESP_FAIL;
    }
    ESP_LOG_BUFFER_HEX("BYTE",buf,recv_size);
    ESP_LOGI("SERVER", "TAMANHO %d",recv_size);
    preenche_tudo(buf);
    httpd_resp_send(req, "tudo certo!", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(200/portTICK_PERIOD_MS);
    save_flash();
    return ESP_OK;
}
httpd_uri_t papos_index_js = {
    .uri       = "/config",
    .method    = HTTP_POST,
    .handler   = post_config_js,
    .user_ctx  = NULL
};

esp_err_t log_get( httpd_req_t *req )
{

    if(httpd_basic_auth(req, "admin", "admin") == ESP_OK) {
        
    cJSON *mensagem;
        cJSON_ArrayForEach(mensagem,saida_log){
            char *payload =  cJSON_Print(mensagem);
            httpd_resp_sendstr_chunk(req,payload);
            httpd_resp_sendstr_chunk(req,"\n");
            free(payload);
        };
            httpd_resp_sendstr_chunk(req,NULL);
        return ESP_OK;
	}else{
		httpd_basic_auth_resp_send_401(req);
		httpd_resp_sendstr(req, "Não Autorizado");
		return ESP_FAIL;
	}
    return ESP_FAIL;
}

httpd_uri_t page_log_get = {
    .uri       = "/log",
    .method    = HTTP_GET,
    .handler   = log_get,
    .user_ctx  = NULL
};

esp_err_t json_get( httpd_req_t *req )
{
    cJSON *root = cJSON_CreateObject();

    cJSON *tensoes= cJSON_CreateArray();
    cJSON *correntes= cJSON_CreateArray();
    cJSON *potencias= cJSON_CreateArray();
    cJSON *energias= cJSON_CreateArray();
    cJSON *frequencias= cJSON_CreateArray();
    cJSON *powers= cJSON_CreateArray();
    cJSON *times= cJSON_CreateArray();
    
    uint16_t* valores = retorna_tensoes();
    uint32_t* valores_1 = retorna_correntes();
    uint32_t* valores_2 = retorna_potencias();
    uint32_t* valores_3 = retorna_energias();
    uint16_t* valores_4 = retorna_frequencias();
    uint16_t* valores_5 = retorna_power();
    uint32_t* tempos = retorna_tempos();
    
    

    for(uint8_t i=0;i<AMOSTRAS;i++){
        cJSON_AddItemToArray(tensoes,cJSON_CreateNumber(valores[i]));
        cJSON_AddItemToArray(correntes,cJSON_CreateNumber(valores_1[i]));
        cJSON_AddItemToArray(potencias,cJSON_CreateNumber(valores_2[i]));
        cJSON_AddItemToArray(energias,cJSON_CreateNumber(valores_3[i]));
        cJSON_AddItemToArray(frequencias,cJSON_CreateNumber(valores_4[i]));
        cJSON_AddItemToArray(powers,cJSON_CreateNumber(valores_5[i]));
        char tempo[11];
        snprintf(tempo,11,"%d",tempos[i]);
        cJSON_AddItemToArray(times,cJSON_CreateString(tempo));
    }
    cJSON_AddItemToObject(root, "tensao", tensoes);
    cJSON_AddItemToObject(root, "correntes", correntes);
    cJSON_AddItemToObject(root, "potencias", potencias);
    cJSON_AddItemToObject(root, "energias", energias);
    cJSON_AddItemToObject(root, "frequencias", frequencias);
    cJSON_AddItemToObject(root, "powers", powers);
    cJSON_AddItemToObject(root, "tempos", times);
    
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = NULL;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i){
        netif = esp_netif_next(netif);
        ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
        ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
        break;
    }
    char ipAddr[25],maskAddr[25],gateAddr[25],MACAddr[40];

    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
    snprintf(ipAddr,25," " IPSTR, IP2STR(&ip_info.ip));
    snprintf(maskAddr,25," " IPSTR, IP2STR(&ip_info.netmask));
    snprintf(gateAddr,25," " IPSTR, IP2STR(&ip_info.gw));
    extern uint8_t WIFI_MAC[8];
    snprintf(MACAddr,40,"%02x:%02x:%02x:%02x:%02x:%02x", WIFI_MAC[0],WIFI_MAC[1],WIFI_MAC[2],WIFI_MAC[3],WIFI_MAC[4],WIFI_MAC[5]);
    cJSON_AddItemToObject(root, "ip", cJSON_CreateString(ipAddr));
    cJSON_AddItemToObject(root, "mask", cJSON_CreateString(maskAddr));
    cJSON_AddItemToObject(root, "gw", cJSON_CreateString(gateAddr));
    cJSON_AddItemToObject(root, "mac", cJSON_CreateString(MACAddr));
    extern union vetorzao memory;
    cJSON_AddItemToObject(root, "ssid", cJSON_CreateString(memory.filtrado.wifi_ssid));

    char *nome = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    if(nome!=NULL){
        httpd_resp_send( req, (const char*) nome, strlen(nome));
    }else{
        httpd_resp_send_404(req);
    }
    cJSON_free(nome);
    return ESP_OK;
}

httpd_uri_t page_json_get = {
    .uri       = "/json",
    .method    = HTTP_GET,
    .handler   = json_get,
    .user_ctx  = NULL
};


void start_mdns_service(void)
{
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGI(TAG,"MDNS Init failed");
        return;
    }
    mdns_hostname_set("neemias");
    mdns_instance_name_set("ESP32 - NEEMIAS");
}

static int print_to_webs(const char *fmt, va_list list)
{
    #if DEBUG
        vprintf(fmt,list);
    #endif
    char mensagem[150];
    int res = vsprintf(mensagem,fmt, list);
    cJSON_AddItemToArray(saida_log,cJSON_CreateString(mensagem+12));
    if(cJSON_GetArraySize(saida_log)>75){
        cJSON_DeleteItemFromArray(saida_log,0);
    }
    return res;
}

httpd_handle_t start_webserver(void)
{ 
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_open_sockets = 2;
    config.task_priority = 1;
    httpd_handle_t server = NULL;
    config.stack_size=5*STACK_DEFAULT_SIZE;
    config.max_uri_handlers = 15;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler( server, &page_index_html);
        httpd_register_uri_handler( server, &page_charts_js);

        httpd_register_uri_handler( server, &page_log_get);

        httpd_register_uri_handler( server, &page_hammer_js);
        httpd_register_uri_handler( server, &page_zoom_js);
        
        httpd_register_uri_handler( server, &page_json_get);

        httpd_register_uri_handler( server, &page_index_js);
        httpd_register_uri_handler( server, &papos_index_js);
        


        return server;
    }

    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (global_server) {
        stop_webserver(global_server);
        
        global_server = NULL;
    }
}

void inicia_server(void)
{
    if (global_server == NULL) {
        ESP_LOGI(TAG,"ABRINDO...");
        global_server = start_webserver();
        start_mdns_service();
        xTaskCreate(&check_update_task, "procura_atualizacao", 8*STACK_DEFAULT_SIZE, NULL, 5, NULL);
    }
}

void fecha_server(void)
{
    if (global_server != NULL) {
        ESP_LOGI(TAG,"FECHANDO...");
        mdns_free();
        stop_webserver(global_server);
        global_server = NULL;
    }
}

void chavear_log(void){
    ESP_LOGI(TAG,"Chaveando saida de Log para WebServer!");
    saida_log = cJSON_CreateArray();
    esp_log_set_vprintf(&print_to_webs);
}