#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_err.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include <string.h>
#include "tcp_service.h"

static const char *TAG = "[TCP]";


QueueHandle_t tcp_msg_send_queue;
QueueHandle_t tcp_msg_recv_queue;


int fdArray[sizeof(fd_set)*TCP_SERVER_ACCEPT_MAX];//第三方数组

tcp_packet_t *tcp_packet_creat(int fd, const char *content_data, int content_size)
{
	tcp_packet_t *tcp_packet = NULL;

	if(fd < 0)
		return NULL;
	
	if(content_size > TCP_RECV_BUF_SIZE)
		return NULL;
	
	tcp_packet->fd = fd;
	tcp_packet = (tcp_packet_t *)malloc(sizeof(tcp_packet_t) + content_size + 1);
	if(tcp_packet != NULL)
	{
		tcp_packet->content_size = content_size;
		if(content_size > 0)
		{
			memcpy(tcp_packet->content_data, content_data, content_size);
			tcp_packet->content_data[content_size] = '\0';
		}
	}
	return tcp_packet;
}


void tcp_packet_release(tcp_packet_t *tcp_packet_p)
{
	if(tcp_packet_p != NULL)
		free(tcp_packet_p);
}

int tcp_lowLevel_sendData(tcp_packet_t *tcp_send_packet_p)
{	
	int err = ESP_OK;
	int bytes = 0;
	int errcnt =0;
	int sendLen;
	char *sendBuf = NULL;
	
	sendBuf = malloc(TCP_SEND_BUF_SIZE);
	if(sendBuf == NULL)
	{
		ESP_LOGE(TAG, "sendbuf malloc failed");
		return ESP_FAIL;	
	}
	memset(sendBuf, 0x0, TCP_SEND_BUF_SIZE);

	sendLen = strlen((char *)tcp_send_packet_p->content_data);
	memcpy(sendBuf, tcp_send_packet_p->content_data, sendLen);
	
	while(sendLen > 0)
	{
		bytes = send( tcp_send_packet_p->fd, sendBuf, sendLen , 0);
		if(bytes < 0)
		{
			errcnt++;
			if(errcnt == 3)
			{
				ESP_LOGE(TAG, "DeviceStatus_Report failed");
				close(tcp_send_packet_p->fd);
				err = ESP_FAIL;
				goto exit;
			}
			vTaskDelay(100); 
		}
		else
		{
			sendLen -= bytes;
       		sendBuf += bytes;
		}
	}
exit:
	return err;
}


int TCP_Service_StartUp( int port )
{   
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if( sock < 0 )
    {   
    	ESP_LOGE(TAG, "socket fail...");
        return -1;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);

    if( bind(sock,(struct sockaddr *)&local,sizeof(local)) < 0 )
    {
    	ESP_LOGE(TAG, "bind fail...");
       	return -1;
    }   

    if( listen(sock, TCP_LISTEN_BACKLOG) < 0 )
    {
    	ESP_LOGE(TAG, "listen fail...");
        return -1;
    } 
	
    return sock;
}   

static void TCP_Service_Thread(void *pvParameters)
{
	fd_set rfds;
	int i = 0;
	tcp_packet_t *tcp_packet_recv_p = NULL;
	tcp_packet_t *tcp_packet_send_p = NULL;
	
    int listen_sock = TCP_Service_StartUp(TCP_SERVER_PORT);
	if(listen_sock < 0)
	{
		ESP_LOGI(TAG, "creat socket failed");
		vTaskDelete(NULL);
		return;
	}

    fdArray[0] = listen_sock ; 
    int num = sizeof(fdArray)/sizeof(fdArray[0]);
    ESP_LOGI(TAG, "At The Same Time Keep Connect Max = %d\n",num);
                       
	//init fd array
    for(i = 1 ; i < num; i++  )
    {
        fdArray[i] = -1;
    }

    while(1)
    {
        FD_ZERO(&rfds);
        int max_fd = fdArray[0];

        for(i = 0; i < num; i++ )
        {
            if( fdArray[i] >= 0 )//遍历数组，遇到一个不是-1的数组元素，将该元素表示的文件描述符添加进文件描述符集中
            {
                FD_SET(fdArray[i],&rfds);
                if( fdArray[i] > max_fd )
                {
                    max_fd = fdArray[i];// 不断的更新，找到所关心的最大的描述符，是为了填写select的第一个参数
                }
            }
        }

       struct timeval timeout = {TCP_SELECT_TIMEVAL_SEC, TCP_SELECT_TIMEVAL_uSEC};

       switch( select(max_fd+1,&rfds,NULL,NULL,&timeout) )// 表示只监视该文件描述符的读事件
       {
           case 0://timeout
               ESP_LOGI(TAG, "select timeout...");
			   break;
           case -1:// failed
               ESP_LOGI(TAG, "select fail...");
			   break;
           default: //successful
           {
               // 6. 根据数组中记录的所关心的文件描述符集先判断哪个文件描述符就绪
               //    如果是监听文件描述符，则调用accept接受新连接
               //   如果是普通文件描述符，则调用read读取数据
               for(i = 0 ;i < num; i++ )
	           {
	               if( fdArray[i] == -1 )
	               {
	                   continue;
	               }
	               if( fdArray[i] == listen_sock && FD_ISSET( fdArray[i],&rfds ) )
	               {
	                   // 1. 如果监听套接字上读就绪,此时提供接受连接服务
	                   struct sockaddr_in client;
	                   socklen_t len = sizeof(client);
	                   int new_sock = accept(listen_sock,(struct sockaddr *)&client,&len);
	                   if(new_sock < 0)
	                   {
	                       ESP_LOGE(TAG, "accept fail...");
	                       continue;
	                   }
	                   //获得新的文件描述符之后，将该文件描述符添加进数组中，以供下一次关心该文件描述符
	                   for(i = 0 ; i < num; i++ )
	                   {
	                       if( fdArray[i] == -1 )//放到数组中第一个值为-1的位置
	                           break;
	                   }
	                   if( i < num )
	                   {
	                       fdArray[i] = new_sock;
	                   }
	                   else
	                   {
	                       close(new_sock);
	                   }
	                   ESP_LOGI(TAG, "get a new link!,[%s:%d]",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
	                   continue;
	               }

	                //2. 此时关心的是普通文件描述符
	                //   此时提供读取数据的服务
	                if( FD_ISSET( fdArray[i],&rfds ) )
	                {
	                    char recvBuf[TCP_RECV_BUF_SIZE];
	                    ssize_t readLen = recv(fdArray[i], recvBuf, TCP_RECV_BUF_SIZE, 0);
	                    if( readLen < 0 )
	                    {
	                        ESP_LOGE(TAG, "read fail...");
	                        close(fdArray[i]);
	                        fdArray[i] = -1;
	                    }
	                    else if( readLen == 0 )
	                    {
	                        ESP_LOGI(TAG, "client quit...");
	                        close(fdArray[i]);
	                        fdArray[i] = -1;
	                    }
	                    else
	                    {
	                        recvBuf[readLen] = 0;
	                        ESP_LOGI(TAG, "client# %s", recvBuf);
							int err = send(fdArray[i], recvBuf, readLen, 0);
							if (err < 0) {
	            				ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
	            				break;
	        				}
	        			
	        				/*
							printf("client# %s\n", recvBuf);
							tcp_packet_recv_p = tcp_packet_creat(fdArray[i], recvBuf, readLen);
							int err = xQueueSend(tcp_msg_recv_queue, &tcp_packet_recv_p, 200);
							if(err != pdPASS)
							{
								tcp_packet_release(tcp_packet_recv_p);
								tcp_packet_recv_p = NULL;
								ESP_LOGE(TAG, "push to queue error");
							}
							*/
	             		}
	           		}
	        	}
           	}
    		break;
       	}
	   //处理发送队列消息  	
    }
    vTaskDelete(NULL);
}   


void TCP_Recv_Thread(void *pvParameters)
{
	(void)pvParameters;

	
}


int TCP_Service_Init(void)
{
	int ret = ESP_OK;

	ESP_LOGI(TAG, "TCP_Service_Init");
	
	tcp_msg_send_queue = xQueueCreate(TCP_SEND_QUEUE_MAX, sizeof(tcp_packet_t));

	tcp_msg_recv_queue = xQueueCreate(TCP_RECV_QUEUE_MAX, sizeof(tcp_packet_t));

	ret = xTaskCreate(TCP_Service_Thread, "tcp_server", 20480, NULL, 5, NULL);
	if(ret != pdPASS)
	{
		ESP_LOGE(TAG, "creat TCP_Service_Thread failed");
		goto ERROR;
	}

/*	
	ret = xTaskCreate(TCP_Recv_Thread, "TCP_Recv_Thread", 4096, NULL, 9, &tcp_recv_threadHandle); 
	if(ret != pdPASS)
	{
		ESP_LOGE(TAG, "creat TCP_Service_Thread failed");
		goto exit;
	}

*/

	return 0;

ERROR:
	return -1;
}



















































