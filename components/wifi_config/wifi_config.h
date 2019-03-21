#ifndef _WIFI_CONFIG_H
#define _WIFI_CONFIG_H


#define maxIpLen            16


void Str_Get_IP(const char *ip_str, unsigned char *ip);

void Master_Wifi_Init(char *ssid, char *pwd);


#endif 

