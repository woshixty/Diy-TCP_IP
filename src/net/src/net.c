#include "net.h"
#include "net_plat.h"
#include "exmsg.h"
#include "pktbuf.h"

/**
 * 协议栈初始化
 */
net_err_t net_init(void) {
    net_plat_init();
    exmsg_init();
    pktbuf_init();
    return NET_ERR_OK;
}

/**
 * 启动协议栈
 */
net_err_t net_start(void) {
    // 启动消息传递机制
    exmsg_start();
    return NET_ERR_OK;
}