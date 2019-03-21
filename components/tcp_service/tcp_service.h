#ifndef _TCP_SERVICE__
#define _TCP_SERVICE__

#include "smartConfig.h"

typedef struct
{
	int content_size;
	unsigned char content_data[0];
} tcp_packet_t, *p_tcp_packet_t_t;

typedef struct
{
	int fd[WIFI_TCP_SERVER_ACCEPT_MAX];
	char ip_str[WIFI_TCP_SERVER_ACCEPT_MAX][16];
	
}client_info_table_t;

void tcp_client_info_save(char *ip, int clie_fd);

int tcp_lookup_clieInfo_table(char *ip);

tcp_packet_t *tcp_packet_creat(const char *content_data, int content_size);

void tcp_packet_release(tcp_packet_t **tcp_packet_p);

int tcp_lowLevel_sendData(tcp_packet_t *tcp_send_packet_p, const int sock_fd);

int TCP_Service_Init(void);





#endif

