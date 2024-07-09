#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "esp_log.h"

#include "Drivers/Drivers_Wifi.h"
#include "Config/defines.h"
#include "Memory/Memory_Flash.h"

#include "App/APP_Webserver.h"

static int s_retry_num = 0;
uint8_t WIFI_MAC[8];
bool conectado = false;
static const char *TAG = "WIFI";

static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        conectado = false;
		esp_wifi_connect();
        vTaskDelay(10000/portTICK_PERIOD_MS);
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ESP_LOGI(TAG, "IP CONECTADO.");
        conectado = true;
	} else if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ESP_LOGI(TAG, "AP CONECTADA.");
    } else{
        // ESP_LOGE(TAG, "EVENT BASE %s", event_base);
        // ESP_LOGE(TAG, "EVENT  ID %d", event_id);
    }
}

void wifi_init_apsta(void)
{  

	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set("wifi", ESP_LOG_INFO);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
	assert(ap_netif);
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_log_level_set("wifi",ESP_LOG_ERROR);
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,NULL));

    esp_wifi_get_mac(WIFI_IF_STA,WIFI_MAC);

	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
	ESP_ERROR_CHECK( esp_wifi_start() );

    wifi_config_t ap_config = { 0 };

    char nome[33];

	strcpy((char *)ap_config.ap.ssid,"ESP32_Neemias");
	strcpy((char *)ap_config.ap.password, ESP_AP_WIFI_PASS);
	ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	ap_config.ap.ssid_len = 13;
	ap_config.ap.max_connection = 4;
	ap_config.ap.channel = 1;

	if (strlen(ESP_AP_WIFI_PASS) == 0) {
		ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}


    
	wifi_config_t sta_config = { 0 };

	strcpy((char *)sta_config.sta.ssid, memory.filtrado.wifi_ssid);
	strcpy((char *)sta_config.sta.password, memory.filtrado.wifi_pass);
    ESP_LOGI("WIFI","USER:%s PASS:%s",sta_config.sta.ssid,sta_config.sta.password);

	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );
	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config) );
    ESP_LOGI("WIFI","MAC AP:");
    uint8_t macz[8];
    esp_wifi_get_mac(WIFI_IF_STA,macz);
    ESP_LOG_BUFFER_HEX("STATION",macz,8);


	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI(TAG,"Iniciando AP em: %s com senha: %s",ap_config.ap.ssid,ap_config.ap.password);
	ESP_ERROR_CHECK( esp_wifi_connect() );

}