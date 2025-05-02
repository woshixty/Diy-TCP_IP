#include <stdio.h>
#include "sys_plat.h"

#define SYS_PLAT_WINDOWS 1

static sys_sem_t sem;
static int count;
static sys_mutex_t mutex;

void thread1_entry (void* arg) {
    // count的值大一些，效果才显著
    for (int i = 0; i < 10000; i++) {
        sys_mutex_lock(mutex);
        count++;
        sys_mutex_unlock(mutex);
    }
    plat_printf("thread 1: count = %d\n", count);

    while (1) {
        plat_printf("this is thread 1: %s\n", (char *)arg);
        sys_sem_notify(sem);
        sys_sleep(1000);
    }
}

void thread2_entry(void* arg) {
    // 注意循环的次数要和上面的一样
    for (int i = 0; i < 10000; i++) {
        sys_mutex_lock(mutex);
        count--;
        sys_mutex_unlock(mutex);
    }
    plat_printf("thread 2: count = %d\n", count);

    while (1) {
        sys_sem_wait(sem, 0);
        plat_printf("this is thread 2: %s\n", (char *)arg);
    }
}

int main (void) {
    // 注意放在线程的创建的前面，以便线程运行前就准备好
    sem = sys_sem_create(0);
    mutex = sys_mutex_create();
    plat_printf("thread main: count = %d\n", count);

    sys_thread_create(thread1_entry, "AAAA");
    sys_thread_create(thread2_entry, "BBBB");
	
	pcap_t* pcap = pcap_device_open(netdev0_phy_ip, netdev0_hwaddr);
	while (pcap)
	{
		static uint8_t buffer[1024];
		static int counter = 0;
		struct pcap_pkthdr* pkthdr;
		const uint8_t* pkt_data;
		
		plat_printf("begin test: %d\n", counter++);
		for (size_t i = 0; i < sizeof(buffer); i++)
		{
			buffer[i] = i;
		}
		
		if(pcap_next_ex(pcap, &pkthdr, &pkt_data) != 1) {
			continue;
		}

		int len = pkthdr->len > sizeof(buffer) ? sizeof(buffer) : pkthdr->len;
		plat_memcpy(buffer, pkt_data, len);
		buffer[0] = 1;
		buffer[1] = 2;

		if(pcap_inject(pcap, buffer, len) == -1)
		{
			plat_printf("pcap send: send packet failed %s\n", pcap_geterr(pcap));
			break;
		}
	}
	

	printf("Hello, world");
	return 0;
}