#include "netif.h"
#include "dbg.h"
#include "exmsg.h"

/**
 * @brief 鎵撳紑鐜洖鎺ュ彛
 */
static net_err_t loop_open(netif_t* netif, void* ops_data) {
    netif->type = NETIF_TYPE_LOOP;
    return NET_ERR_OK;
}

/**
 * @brief 鍏抽棴鐜洖鎺ュ彛
 */
static void loop_close (struct _netif_t* netif) {
}

/**
 * @brief 鍚姩鏁版嵁鍖呯殑鍙戦€?
 */
static net_err_t loop_xmit (struct _netif_t* netif) {
    // 浠庢帴鍙ｅ彇鏀跺埌鐨勬暟鎹寘锛岀劧鍚庡啓鍏ユ帴鏀堕槦鍒楋紝骞堕€氱煡涓荤嚎绋嬪鐞?
    pktbuf_t * pktbuf = netif_get_out(netif, -1);
    if (pktbuf) {
        // 鍐欏叆鎺ユ敹闃熷垪
        net_err_t err = netif_put_in(netif, pktbuf, -1);
        if (err < 0) {
            dbg_warning(DBG_NETIF, "netif full");
            pktbuf_free(pktbuf);
            return err;
        }
    }

    return NET_ERR_OK;
}

/**
 * @brief 鐜洖鎺ュ彛椹卞姩
 */
static const netif_ops_t loop_driver = {
    .open = loop_open,
    .close = loop_close,
    .xmit = loop_xmit,
};

/**
 * @brief 鍒濆鍖栫幆鍥炴帴鍙?
 */
net_err_t loop_init(void) {
    dbg_info(DBG_NETIF, "init loop");

    // 鎵撳紑鎺ュ彛
    netif_t * netif = netif_open("loop", &loop_driver, (void*)0);
    if (!netif) {
        dbg_error(DBG_NETIF, "open loop error");
        return NET_ERR_NONE;
    }

     // 鐢熸垚鐩稿簲鐨勫湴鍧€
    ipaddr_t ip, mask;
    ipaddr_from_str(&ip, "127.0.0.1");
    ipaddr_from_str(&mask, "255.0.0.0");
    netif_set_addr(netif, &ip, &mask, (ipaddr_t*)0);
    netif_set_active(netif);

    // 娴嬭瘯绋嬪簭
    pktbuf_t * buf = pktbuf_alloc(100);
    netif_out(netif, NULL, buf);

    dbg_info(DBG_NETIF, "init done");
    return NET_ERR_OK;;
}