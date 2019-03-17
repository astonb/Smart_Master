#ifndef _TCP_SERVICE__
#define _TCP_SERVICE__

#define TCP_SERVER_IP 		"192.168.2.1"
#define TCP_SERVER_PORT 		8000

#define TCP_SERVER_ACCEPT_MAX		8
#define TCP_SERVER_LISTEN_BACKLOG	5
#define TCP_RECV_BUF_SIZE			512
#define TCP_SEND_BUF_SIZE			512
#define TCP_SELECT_TIMEVAL_SEC		5
#define TCP_SELECT_TIMEVAL_uSEC		0

#define TCP_SEND_QUEUE_MAX		20
#define TCP_RECV_QUEUE_MAX		20





typedef struct
{
	int fd;
	int content_size;
	unsigned char content_data[0];
} tcp_packet_t;




int TCP_Service_Init(void);







#endif

