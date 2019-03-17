#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_log.h"
#include "wifi_config.h"

//config  ssid,password
//char master_ssid[64]="Smart-Master";
//char master_pwd[64]="9876543210";

static EventGroupHandle_t wifi_event_group;

xQueueHandle xQueueConntecdWifi = NULL;
static const int CONNECTED_BIT = BIT0;
static const char *TAG = "[WIFI]";
static uint8_t g_ReConnect_Cnt = 0;


//wifi事件处理
static esp_err_t Wifi_EventHandler(void *ctx, system_event_t *event)
{
	char localIp[maxIpLen];

	ESP_LOGI(TAG, "event_id:%d\n", event->event_id);

	switch (event->event_id)
	{
		case SYSTEM_EVENT_STA_START:
			ESP_LOGE(TAG, "Wifi Connect start");
			break;
		case SYSTEM_EVENT_AP_START:
			ESP_LOGE(TAG, "ESP Wifi-AP start");
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			break;
		case SYSTEM_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "Wifi Connected:Success");
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			//得到sta成功连接AP时，ip地址
			strncpy(localIp, ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip), maxIpLen);//
			ESP_LOGI(TAG, "Wifi IP:%s", localIp);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			if(++g_ReConnect_Cnt>3)
			{
				ESP_LOGI(TAG, "ERROR:Wifi DisConnected");
			}else
			{
				ESP_LOGI(TAG, "Wifi DisConnected,ReConnect[%d]", g_ReConnect_Cnt);
				ESP_ERROR_CHECK(esp_wifi_connect());
			}
			break;
		case SYSTEM_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
			wifi_sta_list_t sta_list;
			esp_wifi_ap_get_sta_list(&sta_list);	//station list
			if (sta_list.num > 3)
			{
				esp_wifi_deauth_sta(0);
				ESP_LOGI(TAG, "clear %d",sta_list.num);
			}
			
			ESP_LOGI(TAG, "NUM %d",sta_list.num);	//打印是哪个station
			break;
		case SYSTEM_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED");
			break;
		default:
			ESP_LOGI(TAG, "Wifi event->event_id %d",event->event_id);
			break;
	}

	return ESP_OK;
}


void Wifi_SoftAp_Start(char *ssid, char *password)
{
    wifi_config_t wifi_config =
    {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .max_connection = 4,
            .channel = 12,
            .password = "",
            .authmode = WIFI_AUTH_OPEN
        },
    };

    g_ReConnect_Cnt = 0;
	

	strlcpy((char*) wifi_config.ap.ssid,	  ssid,    sizeof(wifi_config.ap.ssid));
	strncpy((char*) wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
	ESP_LOGI(TAG,"Open Wifi Hotspot: %s......, key %s",  wifi_config.ap.ssid, wifi_config.ap.password);
	wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK ;//WIFI_AUTH_OPEN;

	ESP_ERROR_CHECK(esp_wifi_stop());
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGE(TAG, "Connect to Wifi ! Start to Connect to Server....");

	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

}





//wifi初始化
void Master_Wifi_Init(char *ssid, char *pwd)
{
    //初始化wifi
    tcpip_adapter_init();
    //设置AP的地址为10.10.2.1
    tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
    tcpip_adapter_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 1); //10.10.2.1
    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(Wifi_EventHandler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	Wifi_SoftAp_Start(ssid, pwd);


}



