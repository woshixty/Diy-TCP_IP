﻿#include "net_plat.h"
#include "exmsg.h"
#include "fixq.h"
#include "mblock.h"
#include "dbg.h"
#include "nlocker.h"

static void * msg_tbl[EXMSG_MSG_CNT];  // 消息缓冲区
static fixq_t msg_queue;            // 消息队列
static exmsg_t msg_buffer[EXMSG_MSG_CNT];  // 消息块
static mblock_t msg_block;          // 消息分配器

/**
 * @brief 收到来自网卡的消息
 */
net_err_t exmsg_netif_in(netif_t* netif) {
    // 分配一个消息结构
    exmsg_t* msg = mblock_alloc(&msg_block, -1);
    if (!msg) {
        dbg_warning(DBG_MSG, "no free exmsg");
        return NET_ERR_MEM;
    }

    // 写消息内容
    msg->type = NET_EXMSG_NETIF_IN;
    msg->netif.netif = netif;

    // 发送消息
    net_err_t err = fixq_send(&msg_queue, msg, -1);
    if (err < 0) {
        dbg_warning(DBG_MSG, "fixq full");
        mblock_free(&msg_block, msg);
        return err;
    }

    return NET_ERR_OK;
}

/**
 * @brief 网络接口有数据到达时的消息处理
 */
static net_err_t do_netif_in(exmsg_t* msg) {
    netif_t* netif = msg->netif.netif;

    // 反复从接口中取出包，然后一次性处理
    pktbuf_t* buf;
    while ((buf = netif_get_in(netif, -1))) {
        dbg_info(DBG_MSG, "recv a packet");

        if(netif->link_layer) {
            net_err_t err = netif->link_layer->in(netif, buf);
            if(err < 0) {
                pktbuf_free(buf);
                dbg_warning(DBG_MSG, "netif in failed, error=%d", err);
            }
        } else {
            pktbuf_free(buf);
        }
    }

    return NET_ERR_OK;
}

/**
 * @brief 核心线程通信初始化
 */
net_err_t exmsg_init(void) {
    dbg_info(DBG_MSG, "exmsg init");

    // 初始化消息队列
    net_err_t err = fixq_init(&msg_queue, msg_tbl, EXMSG_MSG_CNT, EXMSG_BLOCKER);
    if (err < 0) {
        dbg_error(DBG_MSG, "fixq init error");
        return err;
    }

    // 初始化消息分配器
    err = mblock_init(&msg_block, msg_buffer, sizeof(exmsg_t), EXMSG_MSG_CNT, EXMSG_BLOCKER);
    if (err < 0) {
        dbg_error(DBG_MSG,  "mblock init error");
        return err;
    }

    // 初始化完成
    dbg_info(DBG_MSG, "init done.");
    return NET_ERR_OK;
}

/**
 * @brief 核心线程功能体
 */
static void work_thread (void * arg) {
    // 注意要加上\n。否则由于C库缓存的关系，字符串会被暂时缓存而不输出显示
    dbg_info(DBG_MSG, "exmsg is running....\n");

    while (1) {
        // 接受消息
        exmsg_t* msg = (exmsg_t*)fixq_recv(&msg_queue, 0);
        if (msg == (exmsg_t*)0) {
            continue;
        }

        // 消息到了，打印提示
        dbg_info(DBG_MSG, "recieve a msg(%p): %d", msg, msg->type);
        switch (msg->type) {
        case NET_EXMSG_NETIF_IN:          // 网络接口消息
            do_netif_in(msg);
            break;
        }

        // 释放消息
        mblock_free(&msg_block, msg);
    }
}

/**
 * @brief 启动核心线程通信机制
 */
net_err_t exmsg_start(void) {
    // 创建核心线程
    sys_thread_t thread = sys_thread_create(work_thread, (void *)0);
    if (thread == SYS_THREAD_INVALID) {
        return NET_ERR_SYS;
    }

    return NET_ERR_OK;
}