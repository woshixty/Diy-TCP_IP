﻿#include "net.h"
#include "net_plat.h"
#include "exmsg.h"
#include "pktbuf.h"
#include "dbg.h"
#include "netif.h"
#include "loop.h"
#include "ether.h"
#include "tools.h"

/**
 * 协议栈初始化
 */
net_err_t net_init(void) {
    dbg_info(DBG_INIT, "init net...");

    // 各模块初始化
    net_plat_init();
    tools_init();
    exmsg_init();
    pktbuf_init();
    netif_init();

    // 环回接口初始化
    loop_init();
    ether_init();
    return NET_ERR_OK;
}

/**
 * 启动协议栈
 */
net_err_t net_start(void) {
    // 启动消息传递机制
    exmsg_start();

    dbg_info(DBG_INIT, "net is running.");
    return NET_ERR_OK;
}