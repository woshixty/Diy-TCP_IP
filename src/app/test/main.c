﻿#include <stdio.h>
#include "sys_plat.h"
#include "echo/tcp_echo_client.h"

#define SYS_PLAT_WINDOWS 1

static sys_sem_t sem;
static sys_mutex_t mutex;
static int count;

static char buffer[100];
static sys_sem_t read_sem;      // 读信号量
static sys_sem_t write_sem;     // 写信号量
static int write_idx, read_idx, total;  // 写索引和读索引，总计数

void thread1_entry (void* arg) {
    // count的值大一些，效果才显著
    for (int i = 0; i < 10000; i++) {
        sys_mutex_lock(mutex);
        count++;
        sys_mutex_unlock(mutex);
    }
    plat_printf("thread 1: count = %d\n", count);

    // 不断读取数据
    for (int i = 0; i < 2 * sizeof(buffer); i++) {
        // 等待有数据，即读信号量中的数据资源计数不为0
        sys_sem_wait(read_sem, 0);

        // 取出一个数据
        unsigned char data = buffer[read_idx++];
        if (read_idx >= sizeof(buffer)) {
            read_idx = 0;
        } 

        // 显示当前读的内容
        plat_printf("thread 1 read data=%d\n", data);

        // 调整总计数
        sys_mutex_lock(mutex);
        total--;
        sys_mutex_unlock(mutex);

        // 发信号给写线程，通知可以进行写了
        sys_sem_notify(write_sem);

        // 延时一会儿，让读取的速度慢一些
        sys_sleep(100);
    }

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

    // 连续写200次，由于中间没有printf、延时，所以写的速度是比较快的
    for (int i = 0; i < 2 * sizeof(buffer); i++) {
        // 写之前要等有可用的写空间，看看写信号量中空闲资源的数量不为0
        sys_sem_wait(write_sem, 0);

        // 不为0则将数据写入
        buffer[write_idx++] = i;
        if (write_idx >= sizeof(buffer)) {
            write_idx = 0;
        } 
        // 显示当前写的内容，方便观察
        plat_printf("thread 2 write data=%d\n", i);

        // 上锁，保护total的资源计数
        sys_mutex_lock(mutex);
        total++;
        sys_mutex_unlock(mutex);

        // 通知另一个线程可以进行读取，但具体什么时候读则不确定
        sys_sem_notify(read_sem);
    }
    sys_sleep(100000);      // 加点延时，避免后面的代码干扰

    while (1) {
        sys_sem_wait(sem, 0);
        plat_printf("this is thread 2: %s\n", (char *)arg);
    }
}

#include "netif_pcap.h"
#include "dbg.h"
#include "nlist.h"
#include "mblock.h"
#include "pktbuf.h"
#include "net_err.h"
#include "ipaddr.h"
#include "netif.h"

net_err_t netdev_init(void)
{
    netif_pcap_open();
    return NET_ERR_OK;
}

/**
 * @brief 测试结点
 */
typedef struct _tnode_t {
    int id;
    nlist_node_t node;
}tnode_t;

/**
 * @brief 链表访问测试
 */
void nlist_test (void) {
    #define NODE_CNT        8

    tnode_t node[NODE_CNT];
    nlist_t list;
    nlist_node_t * p;

    nlist_init(&list);

    // 头部插入
    for (int i = 0; i < NODE_CNT; i++) {
        node[i].id = i;
        nlist_insert_first(&list, &node[i].node);
    }

    // 遍历打印
    plat_printf("insert first\n");
    nlist_for_each(p, &list) {
        tnode_t * tnode = nlist_entry(p, tnode_t, node);
        plat_printf("id:%d\n", tnode->id);
    }

    // 头部移除
    plat_printf("remove first\n");
    for (int i = 0; i < NODE_CNT; i++) {
        p = nlist_remove_first(&list);
        plat_printf("id:%d\n", nlist_entry(p, tnode_t, node)->id);
    }

    // 尾部插入
    for (int i = 0; i < NODE_CNT; i++) {
        nlist_insert_last(&list, &node[i].node);
    }

    // 遍历打印
    plat_printf("insert last\n");
    nlist_for_each(p, &list) {
        tnode_t * tnode = nlist_entry(p, tnode_t, node);
        plat_printf("id:%d\n", tnode->id);
    }

    // 尾部移除
    plat_printf("remove last\n");
    for (int i = 0; i < NODE_CNT; i++) {
        p = nlist_remove_last(&list);
        plat_printf("id:%d\n", nlist_entry(p, tnode_t, node)->id);
    }   
    
    // 插入到指定结点之后
    plat_printf("insert after\n");
    for (int i = 0; i < NODE_CNT; i++) {
        nlist_insert_after(&list, nlist_first(&list), &node[i].node);
    }

    // 遍历打印
    nlist_for_each(p, &list) {
        tnode_t * tnode = nlist_entry(p, tnode_t, node);
        plat_printf("id:%d\n", tnode->id);
    }
}

void mblock_test()
{
    mblock_t blist;
    static uint8_t buffer[100][10];
    mblock_init(&blist, buffer, 100, 10, NLOCKER_THREAD);
    
    void* temp[10];
    for (size_t i = 0; i < 10; i++)
    {
        temp[i] = mblock_alloc(&blist, 0);
        plat_printf("block: %p, free_count: %d\n", temp[i], mblock_free_cnt(&blist));
    }
    for (size_t i = 0; i < 10; i++)
    {
        mblock_free(&blist, temp[i]);
        plat_printf("free_count: %d\n", mblock_free_cnt(&blist));
    }

    mblock_destroy(&blist);
}

void pktbuf_test()
{
    pktbuf_init();
    pktbuf_t* buf = pktbuf_alloc(2000);
    pktbuf_free(buf);

    buf = pktbuf_alloc(2000);
    for (int i = 0; i < 16; i++) {
        pktbuf_add_header(buf, 33, 1);
    }
    for (int i = 0; i < 16; i++) {
        pktbuf_remove_header(buf, 33);
    }
    for (int i = 0; i < 16; i++) {
        pktbuf_add_header(buf, 33, 0);
    }
    for (int i = 0; i < 16; i++) {
        pktbuf_remove_header(buf, 33);
    }
    pktbuf_free(buf);

    buf = pktbuf_alloc(8);
    pktbuf_resize(buf, 32);
    pktbuf_resize(buf, 288);
    pktbuf_resize(buf, 4922);
    pktbuf_resize(buf, 1921);
    pktbuf_resize(buf, 288);
    pktbuf_resize(buf, 32);
    pktbuf_resize(buf, 0);
    pktbuf_free(buf);

    buf = pktbuf_alloc(689);
    pktbuf_t* sbuf = pktbuf_alloc(892);
    pktbuf_join(buf, sbuf);
    pktbuf_free(buf);

    buf = pktbuf_alloc(32);
    pktbuf_join(buf, pktbuf_alloc(4));
    pktbuf_join(buf, pktbuf_alloc(16));
    pktbuf_join(buf, pktbuf_alloc(54));
    pktbuf_join(buf, pktbuf_alloc(32));
    pktbuf_join(buf, pktbuf_alloc(38));

    pktbuf_set_cont(buf, 44);
    pktbuf_set_cont(buf, 60);
    pktbuf_set_cont(buf, 44);
    pktbuf_set_cont(buf, 128);
    pktbuf_set_cont(buf, 135);
    pktbuf_free(buf);

    buf = pktbuf_alloc(32);
    pktbuf_join(buf, pktbuf_alloc(4));
    pktbuf_join(buf, pktbuf_alloc(16));
    pktbuf_join(buf, pktbuf_alloc(54));
    pktbuf_join(buf, pktbuf_alloc(32));
    pktbuf_join(buf, pktbuf_alloc(38));
    pktbuf_join(buf, pktbuf_alloc(512));
    pktbuf_reset_acc(buf);

    static uint16_t temp[1000];
    for (int i = 0; i < 1000; i++) {
        temp[i] = i;
    }
    pktbuf_write(buf, (uint8_t*)temp, pktbuf_total(buf));
    
    static uint16_t read_temp[1000];
    plat_memset(read_temp, 0, sizeof(read_temp));

    pktbuf_reset_acc(buf);
    pktbuf_read(buf, (uint8_t*)read_temp, pktbuf_total(buf));
    if(plat_memcmp(temp, read_temp, pktbuf_total(buf)) != 0) {
        plat_printf("not equal");
        return;
    }

    plat_memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(buf, 18 * 2);
    pktbuf_read(buf, (uint8_t*)read_temp, 56);
    if(plat_memcmp(temp + 18, read_temp, 56) != 0) {
        plat_printf("not equal");
        return;
    }

    plat_memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(buf, 85 * 2);
    pktbuf_read(buf, (uint8_t*)read_temp, 256);
    if(plat_memcmp(temp + 85, read_temp, 256) != 0) {
        plat_printf("not equal");
        return;
    }

    pktbuf_t* dest = pktbuf_alloc(1024);
    pktbuf_seek(dest, 600);
    pktbuf_seek(buf, 200);
    pktbuf_copy(dest, buf, 122);

    plat_memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(dest, 600);
    pktbuf_read(dest, (uint8_t*)read_temp, 122);
    if(plat_memcmp(temp + 100, read_temp, 122) != 0) {
        plat_printf("not equal");
        return;
    }

    pktbuf_seek(dest, 0);
    pktbuf_fill(dest, 53, pktbuf_total(dest));
    plat_memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(dest, 0);

    char* ptr = (char*)read_temp;
    pktbuf_read(dest, (uint8_t*)read_temp, pktbuf_total(dest));
    for (int i = 0; i < pktbuf_total(dest); i++) {
        if(*ptr++ != 53) {
            plat_printf("not equal");
            return;
        }
    }
    
    pktbuf_free(dest);
    pktbuf_free(buf);
}

/**
 * @brief 基本测试
 */
void basic_test(void) {
    // nlist_test();
    // mblock_test();
    // pktbuf_test();
    // netif_t* netif = netif_open("pcap");
}

#define DBG_TEST    DBG_LEVEL_INFO

int main (void) {
    // 初始化协议栈
    net_init();

    // 基础测试
    // basic_test();

    // 初始化网络接口
    netdev_init();
    
    // 启动协议栈
    net_start();

    while (1) {
        sys_sleep(10);
    }
}