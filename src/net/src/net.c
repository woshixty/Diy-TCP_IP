#include "net.h"
#include "net_err.h"
#include "net_plat.h"
#include "exmsg.h"
#include "pktbuf.h"
#include "dbg.h"
#include "netif.h"

/**
 * 协议栈初始化
 */
net_err_t net_init(void) {
    dbg_info(DBG_INIT, "init net");
    net_plat_init();
    exmsg_init();
    pktbuf_init();
    netif_init();
    return NET_ERR_OK;
}

/**
 * 启动协议栈
 */
net_err_t net_start(void) {
    // 启动消息传递机制
    exmsg_start();
    dbg_info(DBG_INIT, "net is running");
    return NET_ERR_OK;
}