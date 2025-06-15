#ifndef NETIF_H
#define NETIF_H

#include "ipaddr.h"
#include "fixq.h"
#include "net_cfg.h"
#include "pktbuf.h"

/**
 * @brief 纭欢鍦板潃
 */
typedef struct _netif_hwaddr_t {
    uint8_t len;                            // 鍦板潃闀垮害
    uint8_t addr[NETIF_HWADDR_SIZE];        // 鍦板潃绌洪棿
}netif_hwaddr_t;

/**
 * @brief 缃戠粶鎺ュ彛鏀寔鐨勬搷浣?
 */
struct _netif_t;       // 鍓嶇疆澹版槑锛岄伩鍏嶇紪璇戝嚭閿?
typedef struct _netif_ops_t {
    net_err_t(*open) (struct _netif_t* netif, void * data);
    void (*close) (struct _netif_t* netif);

    net_err_t (*xmit)(struct _netif_t* netif);
}netif_ops_t;

/**
 * @brief 缃戠粶鎺ュ彛绫诲瀷
 */
typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,                // 鏃犵被鍨嬬綉缁滄帴鍙?
    NETIF_TYPE_ETHER,                   // 浠ュお缃?
    NETIF_TYPE_LOOP,                    // 鍥炵幆鎺ュ彛

    NETIF_TYPE_SIZE,
}netif_type_t;

/**
 * @brief 缃戠粶鎺ュ彛灞備唬鐮?
 */
typedef struct _netif_t {
    char name[NETIF_NAME_SIZE];             // 缃戠粶鎺ュ彛鍚嶅瓧

    netif_hwaddr_t hwaddr;                  // 纭欢鍦板潃
    ipaddr_t ipaddr;                        // ip鍦板潃
    ipaddr_t netmask;                       // 鎺╃爜
    ipaddr_t gateway;                       // 缃戝叧

    enum {                                  // 鎺ュ彛鐘舵€?
        NETIF_CLOSED,                       // 宸插叧娉?
        NETIF_OPENED,                       // 宸茬粡鎵撳紑
        NETIF_ACTIVE,                       // 婵€娲荤姸鎬?
    }state;

    netif_type_t type;                      // 缃戠粶鎺ュ彛绫诲瀷
    int mtu;                                // 鏈€澶т紶杈撳崟鍏?

    const netif_ops_t* ops;                 // 椹卞姩绫诲瀷
    void* ops_data;                         // 搴曞眰绉佹湁鏁版嵁

    nlist_node_t node;                       // 閾炬帴缁撶偣锛岀敤浜庡涓摼鎺ョ綉缁滄帴鍙?
    
    fixq_t in_q;                            // 鏁版嵁鍖呰緭鍏ラ槦鍒?
    void * in_q_buf[NETIF_INQ_SIZE];
    fixq_t out_q;                           // 鏁版嵁鍖呭彂閫侀槦鍒?
    void * out_q_buf[NETIF_OUTQ_SIZE];

    // 鍙互鍦ㄨ繖閲屽姞鍏ヤ竴浜涚粺璁℃€х殑鍙橀噺
}netif_t;

net_err_t netif_init(void);
netif_t* netif_open(const char* dev_name, const netif_ops_t* driver, void* driver_data);
net_err_t netif_set_addr(netif_t* netif, ipaddr_t* ip, ipaddr_t* netmask, ipaddr_t* gateway);
net_err_t netif_set_hwaddr(netif_t* netif, const uint8_t* hwaddr, int len);
net_err_t netif_set_active(netif_t* netif);
net_err_t netif_set_deactive(netif_t* netif);
void netif_set_default (netif_t * netif);
net_err_t netif_close(netif_t* netif);

// 鏁版嵁鍖呰緭鍏ヨ緭鍑虹鐞?
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo);
net_err_t netif_put_out(netif_t * netif, pktbuf_t * buf, int tmo);
pktbuf_t* netif_get_in(netif_t* netif, int tmo);
pktbuf_t* netif_get_out(netif_t * netif, int tmo);
net_err_t netif_out(netif_t* netif, ipaddr_t* ipaddr, pktbuf_t* buf);

#endif