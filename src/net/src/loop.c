#include "loop.h"
#include "dbg.h"

static net_err_t loop_open (struct _netif_t* netif, void* data) {
    netif->type = NETIF_TYPE_LOOP;

    return NET_ERR_OK;
}

static void loop_close (struct _netif_t* netif) {}

static net_err_t loop_xmit (struct _netif_t* netif) {
    return NET_ERR_OK;
}

static const netif_ops_t loop_ops = {
    .open = loop_open,
    .close = loop_close,
    .xmit = loop_xmit,
};

net_err_t loop_init(void) {
    dbg_info(DBG_NETIF, "init loop");

    netif_t* netif = netif_open("loop", &loop_ops, (void*)0);
    if(!netif) {
        dbg_error(DBG_NETIF, "open loop error");
    }

    ipaddr_t ip, mask;
    ipaddr_from_str(&ip, "127.0.0.1");
    ipaddr_from_str(&mask, "255.0.0.0");

    netif_set_addr(netif, &ip, &mask, (ipaddr_t*)0);

    netif_set_active(netif);

    dbg_info(DBG_NETIF, "init done");
    return NET_ERR_OK;
}