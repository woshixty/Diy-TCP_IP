#include <string.h>
#include <stdlib.h>
#include "netif.h"
#include "net.h"
#include "dbg.h"
#include "fixq.h"
#include "exmsg.h"
#include "mblock.h"
#include "pktbuf.h"

static netif_t netif_buffer[NETIF_DEV_CNT];     // 缃戠粶鎺ュ彛鐨勬暟閲?
static mblock_t netif_mblock;                   // 鎺ュ彛鍒嗛厤缁撴瀯
static nlist_t netif_list;               // 缃戠粶鎺ュ彛鍒楄〃
static netif_t * netif_default;          // 缂虹渷鐨勭綉缁滃垪琛?

/**
 * @brief 鏄剧ず绯荤粺涓殑缃戝崱鍒楄〃淇℃伅
 */
#if DBG_DISP_ENABLED(DBG_NETIF)
void display_netif_list (void) {
    nlist_node_t * node;

    plat_printf("netif list:\n");
    nlist_for_each(node, &netif_list) {
        netif_t * netif = nlist_entry(node, netif_t, node);
        plat_printf("%s:", netif->name);
        switch (netif->state) {
            case NETIF_CLOSED:
                plat_printf(" %s ", "closed");
                break;
            case NETIF_OPENED:
                plat_printf(" %s ", "opened");
                break;
            case NETIF_ACTIVE:
                plat_printf(" %s ", "active");
                break;
            default:
                break;
        }
        switch (netif->type) {
            case NETIF_TYPE_ETHER:
                plat_printf(" %s ", "ether");
                break;
            case NETIF_TYPE_LOOP:
                plat_printf(" %s ", "loop");
                break;
            default:
                break;
        }
        plat_printf(" mtu=%d ", netif->mtu);
        plat_printf("\n");
        dump_mac("\tmac:", netif->hwaddr.addr);
        dump_ip_buf(" ip:", netif->ipaddr.a_addr);
        dump_ip_buf(" netmask:", netif->netmask.a_addr);
        dump_ip_buf(" gateway:", netif->gateway.a_addr);

        // 闃熷垪涓寘鏁伴噺鐨勬樉绀?
        plat_printf("\n");
    }
}
#else
#define display_netif_list()
#endif // DBG_NETIF

/**
 * @brief 缃戠粶鎺ュ彛灞傚垵濮嬪寲
 */
net_err_t netif_init(void) {
    dbg_info(DBG_NETIF, "init netif");

    // 寤虹珛鎺ュ彛鍒楄〃
    nlist_init(&netif_list);
    mblock_init(&netif_mblock, netif_buffer, sizeof(netif_t), NETIF_DEV_CNT, NLOCKER_NONE);

    // 璁剧疆缂虹渷鎺ュ彛
    netif_default = (netif_t *)0;
    
    dbg_info(DBG_NETIF, "init done.\n");
    return NET_ERR_OK;
}

/**
 * @brief 鎵撳紑鎸囧畾鐨勭綉缁滄帴鍙?
 */
netif_t* netif_open(const char* dev_name, const netif_ops_t* ops, void * ops_data) {
    // 鍒嗛厤涓€涓綉缁滄帴鍙?
    netif_t*  netif = (netif_t *)mblock_alloc(&netif_mblock, -1);
    if (!netif) {
        dbg_error(DBG_NETIF, "no netif");
        return (netif_t*)0;
    }

    // 璁剧疆鍚勭缂虹渷鍊? 杩欎簺鍊兼湁浜涘皢琚┍鍔ㄥ鐞嗭紝鏈変簺灏嗚涓婂眰netif_xxx鍏跺畠鍑芥暟璁剧疆
    ipaddr_set_any(&netif->ipaddr);
    ipaddr_set_any(&netif->netmask);
    ipaddr_set_any(&netif->gateway);
    netif->mtu = 0;                      // 榛樿涓?锛屽嵆涓嶉檺鍒?
    netif->type = NETIF_TYPE_NONE;
    nlist_node_init(&netif->node);

    plat_strncpy(netif->name, dev_name, NETIF_NAME_SIZE);
    netif->name[NETIF_NAME_SIZE - 1] = '\0';
    netif->ops = ops;                   // 璁剧疆椹卞姩鍜岀鏈夊弬鏁?
    netif->ops_data = (void *)ops_data;

    // 鍒濆鍖栨帴鏀堕槦鍒?
    net_err_t err = fixq_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif in_q init error, err: %d", err);
        return (netif_t *)0;
    }

    // 鍒濆鍖栧彂閫侀槦鍒?
    err = fixq_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif out_q init error, err: %d", err);
        fixq_destroy(&netif->in_q);
        return (netif_t *)0;
    }

    // 鎵撳紑璁惧锛屽纭欢鍋氳繘涓€姝ョ殑璁剧疆, 鍦ㄥ叾鍐呴儴鍙netif瀛楁杩涜璁惧
    // 鐗瑰埆鏄瀵箃ype鍜宭ink_layer鍋氳澶?
    err = ops->open(netif, ops_data);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif ops open error: %d");
        goto free_return;
    }
    netif->state = NETIF_OPENED;        // 鍒囨崲涓簅pened

    // 椹卞姩鍒濆鍖栧畬鎴愬悗锛屽netif杩涜杩涗竴姝ユ鏌?
    // 鍋氫竴浜涘繀瑕佹€х殑妫€鏌ワ紝浠ュ厤椹卞姩娌″啓濂?
    if (netif->type == NETIF_TYPE_NONE) {
        dbg_error(DBG_NETIF, "netif type unknown");
        goto free_return;
    }

    // 鎻掑叆闃熷垪涓?
    nlist_insert_last(&netif_list, &netif->node);
    display_netif_list();
    return netif;

free_return:
    if (netif->state == NETIF_OPENED) {
        netif->ops->close(netif);
    }

    fixq_destroy(&netif->in_q);
    fixq_destroy(&netif->out_q);
    mblock_free(&netif_mblock, netif);
    return (netif_t*)0;
}

/**
 * @brief 璁剧疆IP鍦板潃銆佹帺鐮併€佺綉鍏崇瓑
 * 杩欓噷鍙槸绠€鍗曠殑璁剧疆鎺ュ彛鐨勫悇涓湴鍧€锛岃繘琛屽啓鍏?
 */
net_err_t netif_set_addr(netif_t* netif, ipaddr_t* ip, ipaddr_t* netmask, ipaddr_t* gateway) {
    ipaddr_copy(&netif->ipaddr, ip ? ip : ipaddr_get_any());
    ipaddr_copy(&netif->netmask, netmask ? netmask : ipaddr_get_any());
    ipaddr_copy(&netif->gateway, gateway ? gateway : ipaddr_get_any());
    return NET_ERR_OK;
}

/**
 * @brief 璁剧疆纭欢鍦板潃
 */
net_err_t netif_set_hwaddr(netif_t* netif, const uint8_t* hwaddr, int len) {
    plat_memcpy(netif->hwaddr.addr, hwaddr, len);
    netif->hwaddr.len = len;
    return NET_ERR_OK;
}

/**
 * @brief 婵€娲荤綉缁滆澶?
 */
net_err_t netif_set_active(netif_t* netif) {
    // 蹇呴』涓烘墦寮€鐘舵€佸湴鑳芥縺娲?
    if (netif->state != NETIF_OPENED) {
        dbg_error(DBG_NETIF, "netif is not opened");
        return NET_ERR_STATE;
    }

    // 鐪嬬湅鏄惁瑕佹坊鍔犵己鐪佹帴鍙?
    // 缂虹渷缃戠粶鎺ュ彛鐢ㄤ簬澶栫綉鏁版嵁鏀跺彂鏃剁殑鍖呭鐞?
    if (!netif_default && (netif->type != NETIF_TYPE_LOOP)) {
        netif_set_default(netif);
    }

    // 鍒囨崲涓哄氨缁姸鎬?
    netif->state = NETIF_ACTIVE;
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 鍙栨秷璁惧鐨勬縺娲绘€?
 */
net_err_t netif_set_deactive(netif_t* netif) {
    // 蹇呴』宸茬粡婵€娲荤殑鐘舵€?
    if (netif->state != NETIF_ACTIVE) {
        dbg_error(DBG_NETIF, "netif is not actived");
        return NET_ERR_STATE;
    }

    // 閲婃斁鎺夐槦鍒椾腑鐨勬墍鏈夋暟鎹寘
    pktbuf_t* buf;
    while ((buf = fixq_recv(&netif->in_q, -1))) {
        pktbuf_free(buf);
    }
    while ((buf = fixq_recv(&netif->out_q, -1))) {
        pktbuf_free(buf);
    }

    // 閲嶈缂虹渷缃戝彛
    if (netif_default == netif) {
        netif_default = (netif_t*)0;
    }

    // 鍒囨崲鍥炴墦寮€锛堥潪婵€娲荤姸鎬侊級
    netif->state = NETIF_OPENED;
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 鍏抽棴缃戠粶鎺ュ彛
 */
net_err_t netif_close(netif_t* netif) {
    // 闇€瑕佸厛鍙栨秷active鐘舵€佹墠鑳藉叧闂?
    if (netif->state == NETIF_ACTIVE) {
        dbg_error(DBG_NETIF, "netif(%s) is active, close failed.", netif->name);
        return NET_ERR_STATE;
    }

    // 鍏堝叧闂澶?
    netif->ops->close(netif);
    netif->state = NETIF_CLOSED;

    // 鏈€鍚庨噴鏀緉etif缁撴瀯
    nlist_remove(&netif_list, &netif->node);
    mblock_free(&netif_mblock, netif);
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 璁剧疆缂虹渷鐨勭綉缁滄帴鍙?
 * @param netif 缂虹渷鐨勭綉缁滄帴鍙?
 */
void netif_set_default(netif_t* netif) {
    // 绾綍鏂扮殑缃戝崱
    netif_default = netif;
}

/**
 * @brief 灏哹uf鍔犲叆鍒扮綉缁滄帴鍙ｇ殑杈撳叆闃熷垪涓?
 */
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 鍐欏叆鎺ユ敹闃熷垪
    net_err_t err = fixq_send(&netif->in_q, buf, tmo);
    if (err < 0) {
        dbg_warning(DBG_NETIF, "netif %s in_q full", netif->name);
        return NET_ERR_FULL;
    }

    // 閫氱煡娑堟伅澶勭悊绾跨▼锛岃繖閲屼笉澶勭悊娑堟伅鏄惁鍙戦€佹垚鍔熺瓑闂
    // 娑堟伅婊′簡涓嶈绱э紝璇存槑缃戝崱姝ｅ湪蹇欙紝鍚庣画杩樹細澶勭悊鐨?
    exmsg_netif_in(netif);
    return NET_ERR_OK;
}

/**
 * @brief 灏咮uf娣诲姞鍒扮綉缁滄帴鍙ｇ殑杈撳嚭闃熷垪涓?
 */
net_err_t netif_put_out(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 鍐欏叆鍙戦€侀槦鍒?
    net_err_t err = fixq_send(&netif->out_q, buf, tmo);
    if (err < 0) {
        dbg_info(DBG_NETIF, "netif %s out_q full", netif->name);
        return err;
    }

    // 鍙槸鍐欏叆闃熷垪锛屽叿浣撶殑鍙戦€佷細璋冪敤ops->xmit鏉ュ惎鍔ㄥ彂閫?
    return NET_ERR_OK;
}

/**
 * @brief 浠庤緭鍏ラ槦鍒椾腑鍙栧嚭涓€涓暟鎹寘
 */
pktbuf_t* netif_get_in(netif_t* netif, int tmo) {
    // 浠庢帴鏀堕槦鍒椾腑鍙栨暟鎹寘
    pktbuf_t* buf = fixq_recv(&netif->in_q, tmo);
    if (buf) {
        // 閲嶆柊瀹氫綅锛屾柟渚胯繘琛岃鍐?
        pktbuf_reset_acc(buf);
        return buf;
    }

    dbg_info(DBG_NETIF, "netif %s in_q empty", netif->name);
    return (pktbuf_t*)0;
}

/**
 * 浠庤緭鍑洪槦鍒椾腑鍙栧嚭涓€涓暟鎹寘
 */
 pktbuf_t* netif_get_out(netif_t* netif, int tmo) {
    // 浠庡彂閫侀槦鍒椾腑鍙栨暟鎹寘锛屼笉闇€瑕佺瓑寰呫€傚彲鑳戒細琚腑鏂鐞嗙▼搴忎腑璋冪敤
    // 鍥犳锛屼笉鑳藉洜涓烘病鏈夊寘鑰屾寕璧风▼搴?
    pktbuf_t* buf = fixq_recv(&netif->out_q, tmo);
    if (buf) {
        // 閲嶆柊瀹氫綅锛屾柟渚胯繘琛岃鍐?
        pktbuf_reset_acc(buf);
        return buf;
    }

    dbg_info(DBG_NETIF, "netif %s out_q empty", netif->name);
    return (pktbuf_t*)0;
}

/**
 * @brief 鍙戦€佷竴涓綉缁滃寘鍒扮綉缁滄帴鍙ｄ笂, 鐩爣鍦板潃涓篿paddr
 * 鍦ㄥ彂閫佸墠锛屽厛鍒ゆ柇椹卞姩鏄惁姝ｅ湪鍙戦€侊紝濡傛灉鏄垯灏嗗叾鎻掑叆鍒板彂閫侀槦鍒楋紝绛夐┍鍔ㄦ湁绌哄悗锛岀敱椹卞姩鑷鍙栧嚭鍙戦€併€?
 * 鍚﹀垯锛屽姞鍏ュ彂閫侀槦鍒楀悗锛屽惎鍔ㄩ┍鍔ㄥ彂閫?
 */
net_err_t netif_out(netif_t* netif, ipaddr_t * ipaddr, pktbuf_t* buf) {
    // 缂虹渷鎯呭喌锛屽皢鏁版嵁鍖呮彃鍏ュ氨缁槦鍒楋紝鐒跺悗閫氱煡椹卞姩绋嬪簭寮€濮嬪彂閫?
    // 纭欢褰撳墠鍙戦€佸鏋滄湭杩涜锛屽垯鍚姩鍙戦€侊紝鍚﹀垯涓嶅鐞嗭紝绛夊緟纭欢涓柇鑷姩瑙﹀彂杩涜鍙戦€?
    net_err_t err = netif_put_out(netif, buf, -1);
    if (err < 0) {
        dbg_info(DBG_NETIF, "send to netif queue failed. err: %d", err);
        return err;
    }

    // 鍚姩鍙戦€?
    return netif->ops->xmit(netif);
}