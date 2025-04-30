/**
 * @file main.c
 * @author lishutong (527676163@qq.com)
 * @brief 测试主程序，完成一些简单的测试主程序
 * @version 0.1
 * @date 2022-10-23
 *
 * @copyright Copyright (c) 2022
 * @note 该源码配套相应的视频课程，请见源码仓库下面的README.md
 */
#include <stdio.h>
#include "sys_plat.h"

#define SYS_PLAT_WINDOWS 1

int main (void) {
	// 以下是测试代码，可以删掉
	// 打开物理网卡，设置好硬件地址
	static const uint8_t netdev0_hwaddr[] = { 0x00, 0x50, 0x56, 0xc0, 0x00, 0x11 };
	pcap_t* pcap = pcap_device_open("192.168.74.1", netdev0_hwaddr);
	while (pcap)
	{
		static uint8_t buffer[1024];
		memset(buffer, 0, sizeof(buffer));

		static int counter = 0;
		plat_printf("begin test: %d\n", counter++);
		
		if(pcap_inject(pcap, buffer, sizeof(buffer)) == -1)
		{
			plat_printf("pcap send: send packet failed %s\n", pcap_geterr(pcap));
			break;
		}

		sys_sleep(10);
	}
	

	printf("Hello, world");
	return 0;
}