//======================Copyright (c)=====================================
// 文件名称: app_main.c
// 功能描述:
//
// 修改记录:
//           2019-03-12, hzb 创建文件
//
//
//========================================================================
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"

#include <nvs_flash.h>
#include "wifi_config.h"
#include "tcp_service.h"
#include "lan8720.h"
#include "smartConfig.h"



void app_main()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK( nvs_flash_erase() );
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	Master_Wifi_Init(WIFI_AP_SSID, WIFI_AP_PWD);
	TCP_Service_Init();
	device_ethernet_int();

}

