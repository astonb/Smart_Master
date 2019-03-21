#ifndef _SMARTCONFIG__
#define _SMARTCONFIG__

/********************WIFI AP SSID_PWD CONFIG***********************/
#define WIFI_AP_SSID	"Smart-Master"
#define WIFI_AP_PWD		"9876543210"



/***********WIFI AP NET CONFIG( dotted decimal !!!)***************/
#define WIFI_AP_IP		"192.168.1.30"
#define WIFI_AP_GW		"192.168.1.1"
#define WIFI_AP_NETMASK	"255.255.255.0"


/********************WIFI STATION CONNECT MAX CONFIG***************/
#define WIFI_AP_STA_MAX		10


/********************WIFI TCP SERVER  CONFIG***********************/
#define WIFI_TCP_SERVER_PORT 			8000
//wifi service accept and listen max
#define WIFI_TCP_SERVER_ACCEPT_MAX		WIFI_AP_STA_MAX
#define WIFI_TCP_SERVER_LISTEN_BACKLOG	5
//wifi select time 
#define WIFI_TCP_SELECT_TIMEVAL_SEC		0
#define WIFI_TCP_SELECT_TIMEVAL_uSEC	500000L
//wifi send recv queue max
#define WIFI_TCP_SEND_QUEUE_MAX		20
#define WIFI_TCP_RECV_QUEUE_MAX		20

/********************ETH TCP CLIENT CONFIG***********************/
//pc server config
#define ETH_REMOTE_SERVER_IP		"192.168.1.110"
#define ETH_REMOTE_SERVER_PORT		8000
//eth select time 
#define ETH_SELECT_TIMEOUT_SEC		0L
#define ETH_SELECT_TIMEOUT_uSEC		500000L


/********************TCP BUFF SIZE CONFIG***********************/
#define TCP_RECV_BUF_SIZE			512
#define TCP_SEND_BUF_SIZE			512


#endif

