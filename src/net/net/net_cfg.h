#ifndef NET_CFG_H
#define NET_CFG_H

#define DBG_MBLOCK          DBG_LEVEL_INFO
#define DBG_QUEUE           DBG_LEVEL_INFO
#define DBG_MSG             DBG_LEVEL_INFO
#define DBG_BUF             DBG_LEVEL_INFO
#define DBG_INIT            DBG_LEVEL_INFO
#define DBG_PLAT            DBG_LEVEL_INFO
#define DBG_NETIF           DBG_LEVEL_INFO

#define EXMSG_MSG_CNT       10
#define EXMSG_LOCKER        NLOCKER_THREAD

#define PKTBUF_BLK_SIZE     128
#define PKTBUF_BLK_CNT      100
#define PKTBUF_BUF_CNT      100

#define NETIF_HWADDR_SIZE   10
#define NETIF_NAME_SIZE     10
#define NETIF_INQ_SIZE      50
#define NETIF_OUTQ_SIZE     50

#define NETIF_DEV_CNT       10

#endif