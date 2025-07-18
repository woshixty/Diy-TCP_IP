﻿#ifndef EXMSG_H
#define EXMSG_H

#include "net_err.h"
#include "nlist.h"
#include "netif.h"

/**
 * 网络接口消息
 */
typedef struct _msg_netif_t {
    netif_t* netif;                 // 消息的来源
}msg_netif_t;

/**
 * @brief 传递给核心线程的消息
 */
typedef struct _exmsg_t {
    // 消息类型
    enum {
        NET_EXMSG_NETIF_IN,             // 网络接口数据消息
    }type;

    // 消息数据
    union {
        msg_netif_t netif;             // 网络接口的消息
    };
}exmsg_t;

net_err_t exmsg_init(void);
net_err_t exmsg_start(void);

net_err_t exmsg_netif_in(netif_t* netif);

#endif