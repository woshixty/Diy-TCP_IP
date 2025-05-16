#ifndef EXMSG_H
#define EXMSG_H

#include "net_err.h"
#include "nlist.h"

/**
 * @brief 传递给核心线程的消息
 */
typedef struct _exmsg_t {
    // 消息类型
    enum {
        NET_EXMSG_NETIF_IN,     // 网络接口数据消息
    } type;

    int id;
    nlist_node_t temp;          // 临时使用
}exmsg_t;

net_err_t exmsg_init(void);
net_err_t exmsg_start(void);
net_err_t exmsg_netif_in(void);

#endif // EXMSG_H