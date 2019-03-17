#Smart_Master

content: wifi-ethernet communication
author : 黄志宝
date   : 2019/03/17 create file
contact: qq:408065306   cellphone: 15757821734

版本更新说明：


修改日期：2019/03/17
修改效果：初步完成wifi，lan8720以太网通讯
修改内容：
	1）在void tcpip_adapter_init(void)增加eth默认ip信息
	2）在system_event_eth_connected_handle_default关闭DHCP-client,直接使用静态IP
	3）在tcpip_adapter_start打开eth_netif(并设置成默认netif)
