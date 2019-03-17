#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "rom/gpio.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "driver/periph_ctrl.h"
#include "eth_phy/phy_lan8720.h"
#include "lan8720.h"


#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config

//#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO CONFIG_PHY_SMI_MDIO_PIN

static const char *TAG = "EtherNet";
static const char *payload = "Message from smart_eth_client";


extern EventGroupHandle_t wifi_eth_event_group;
extern const int ETH_LINKUP_BIT;

/**
 * @brief gpio specific init
 *
 * @note RMII data pins are fixed in esp32:
 * TXD0 <=> GPIO19
 * TXD1 <=> GPIO22
 * TX_EN <=> GPIO21
 * RXD0 <=> GPIO25
 * RXD1 <=> GPIO26
 * CLK <=> GPIO0
 *
 */
static void eth_gpio_config_rmii(void)
{
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}



static void test_thread(void *pvParameters)
{
	while(1)
	{
		ESP_LOGI(TAG, "*****Ethernet OK******");
		vTaskDelay(1000);//10s
	}
}


static void eth_tcp_client_thread(void *pvParameters)
{
	fd_set readfds;
	int clie_sock = -1;
	int ret = 0;
	int len = 0;
	char rx_buffer[ETH_TCP_RCVE_BUF] = {0};
	char addr_str[128] = {0};
	struct sockaddr_in destAddr;

	 ESP_LOGE(TAG, "*****eth_tcp_client  Start Up*****");
	
Connect_Start:

    destAddr.sin_addr.s_addr = inet_addr(REMOTE_ETH_SERVER_IP);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(REMOTE_ETH_SERVER_PORT);

	inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
	
	clie_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (clie_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        goto exit;
    }
    ESP_LOGI(TAG, "Socket created");

    ret = connect(clie_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (ret != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        goto exit;
    }
    ESP_LOGI(TAG, "Successfully connected");
	//连接成功主动上报客户端信息
	ret = send(clie_sock, payload, strlen(payload), 0);
	if (ret < 0) {
		ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
        goto exit;
	}

	struct timeval timeout = {ETH_SELECT_TIMEOUT_SEC, ETH_SELECT_TIMEOUT_uSEC};

	while (1) 
	{
		FD_ZERO(&readfds);
		FD_SET(clie_sock, &readfds);

		if( select(clie_sock+1, &readfds, NULL, NULL, &timeout) >= 0)
		{
			if(FD_ISSET(clie_sock, &readfds))
			{
		        len = recv(clie_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		        // Error occured during receiving
		        if (len < 0) {
		            ESP_LOGE(TAG, "recv failed: errno %d", errno);
		            break;
		        }
		        // Data received
		        else {
		            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
		            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
		            ESP_LOGI(TAG, "%s", rx_buffer);
		        }
			}
		}
    }
exit:
    if (clie_sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(clie_sock, 0);
        close(clie_sock);
		vTaskDelay(1000);//10s
		goto Connect_Start;
    }
	vTaskDelete(NULL);
}



void device_ethernet_int(void)
{
	int ret = 0;
	uint8_t eth_mac[6] = {0xdf, 0x20, 0xef, 0x04, 0xff, 0x18};
	uint8_t get_eth_mac[6];
	tcpip_adapter_ip_info_t eth_ip;
	
//   tcpip_adapter_init();
/*
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	tcpip_adapter_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 100); //10.10.2.1
    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ip_info);
	tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
*/
    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;
 
    ESP_ERROR_CHECK(esp_eth_init(&config));
//设置eth的mac
//	esp_eth_set_mac(eth_mac);
//获取eth_mac
	esp_eth_get_mac(get_eth_mac);
	ESP_LOGI(TAG,"eth_mac: %02x : %02x : %02x : %02x : %02x : %02x", get_eth_mac[0],get_eth_mac[1],get_eth_mac[2],get_eth_mac[3],get_eth_mac[4],get_eth_mac[5]);
//获取eth_ip
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &eth_ip);
	ESP_LOGI(TAG,"eth_ip: %s", inet_ntoa(eth_ip.ip));

    ESP_ERROR_CHECK(esp_eth_enable()) ;

	xEventGroupWaitBits(wifi_eth_event_group, ETH_LINKUP_BIT, false, true, portMAX_DELAY);
/*
	ret = xTaskCreate(test_thread, "test_thread", 4096, NULL, 5, NULL);
	if(ret != pdPASS)
	{
		ESP_LOGE(TAG, "creat test_thread failed");
	}
*/
	ret = xTaskCreate(eth_tcp_client_thread, "tcp_server", 8192, NULL, 5, NULL);
	if(ret != pdPASS)
	{
		ESP_LOGE(TAG, "creat eth_tcp_client_thread failed");
	}
}



































