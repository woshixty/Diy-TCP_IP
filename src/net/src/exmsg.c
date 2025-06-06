﻿#include "net_plat.h"
#include "exmsg.h"
#include "mblock.h"
#include "dbg.h"
#include "net_cfg.h"
#include "fixq.h"

static void * msg_tbl[EXMSG_MSG_CNT];       // 消息缓冲区
static fixq_t msg_queue;                    // 消息队列
static exmsg_t msg_buffer[EXMSG_MSG_CNT];   // 消息块
static mblock_t msg_block;                  // 消息分配器

/**
 * @brief 核心线程通信初始化
 */
net_err_t exmsg_init(void) {
    dbg_info(DBG_MSG, "exmsg init");

    net_err_t err = fixq_init(&msg_queue, msg_tbl, EXMSG_MSG_CNT, EXMSG_LOCKER);
    if(err < 0)
    {
        dbg_error(DBG_MSG, "fix init failed.");
    }

    err = mblock_init(&msg_block, msg_buffer, sizeof(exmsg_t), EXMSG_MSG_CNT, EXMSG_LOCKER);
    if(err < 0) {
        dbg_error(DBG_MSG, "mblock init failed.");
        return err;
    }

    dbg_info(DBG_MSG, "exmsg done");
    return NET_ERR_OK;
}

net_err_t exmsg_netif_in(netif_t* netif)
{
    exmsg_t* msg = mblock_alloc(&msg_block, -1);
    if(!msg) {
        dbg_warning(DBG_MSG, "no free msg");
        return NET_ERR_MEM;
    }
    
    static int id = 0;
    msg->type = NET_EXMSG_NETIF_IN;
    msg->id = id++;

    net_err_t err = fixq_send(&msg_queue, msg, -1);
    if(err < 0)
    {
        dbg_error(DBG_MSG, "fixq full");
        mblock_free(&msg_block, msg);
        return err;
    }
    return err;
}

/**
 * @brief 核心线程功能体
 */
static void work_thread (void * arg) {
    // 注意要加上\n。否则由于C库缓存的关系，字符串会被暂时缓存而不输出显示
    dbg_info(DBG_MSG, "exmsg is running....");

    while (1) {
        exmsg_t* msg = (exmsg_t*)fixq_recv(&msg_queue, 0);

        plat_printf("recv a msg, type: %d, id: %d\n", msg->type, msg->id);

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