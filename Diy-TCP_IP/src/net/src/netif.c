﻿#include <string.h>
#include <stdlib.h>
#include "netif.h"
#include "net.h"
#include "dbg.h"
#include "fixq.h"
#include "exmsg.h"
#include "mblock.h"
#include "pktbuf.h"
#include "protocol.h"
#include "ether.h"

static netif_t netif_buffer[NETIF_DEV_CNT];     // 网络接口的数量
static mblock_t netif_mblock;                   // 接口分配结构
static nlist_t netif_list;               // 网络接口列表
static netif_t * netif_default;          // 缺省的网络列表

static const link_layer_t* link_layers[NETIF_TYPE_SIZE];

/**
 * @brief 显示系统中的网卡列表信息
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

        // 队列中包数量的显示
        plat_printf("\n");
    }
}
#else
#define display_netif_list()
#endif // DBG_NETIF

/**
 * @brief 网络接口层初始化
 */
net_err_t netif_init(void) {
    dbg_info(DBG_NETIF, "init netif");

    // 建立接口列表
    nlist_init(&netif_list);
    mblock_init(&netif_mblock, netif_buffer, sizeof(netif_t), NETIF_DEV_CNT, NLOCKER_NONE);

    // 设置缺省接口
    netif_default = (netif_t *)0;
    
    plat_memset((void*)link_layers, 0, sizeof(link_layers));
    dbg_info(DBG_NETIF, "init done.\n");
    return NET_ERR_OK;
}

net_err_t netif_register_layer(int type, const link_layer_t* layer) {
    if(type < 0 || (type >= NETIF_TYPE_SIZE)) {
        dbg_info(DBG_NETIF, "type error");
        return NET_ERR_PARAM;
    }

    if(link_layers[type]) {
        dbg_info(DBG_NETIF, "link layer exist");
        return NET_ERR_EXIST;
    }

    link_layers[type] = layer;
    return NET_ERR_OK;
}

static const link_layer_t* netif_get_layer(int type) {
    if(type < 0 || (type >= NETIF_TYPE_SIZE)) {
        dbg_info(DBG_NETIF, "type error");
        return (link_layer_t*)0;
    }

    return link_layers[type];
}

/**
 * @brief 打开指定的网络接口
 */
netif_t* netif_open(const char* dev_name, const netif_ops_t* ops, void * ops_data) {
    // 分配一个网络接口
    netif_t*  netif = (netif_t *)mblock_alloc(&netif_mblock, -1);
    if (!netif) {
        dbg_error(DBG_NETIF, "no netif");
        return (netif_t*)0;
    }

    // 设置各种缺省值, 这些值有些将被驱动处理，有些将被上层netif_xxx其它函数设置
    ipaddr_set_any(&netif->ipaddr);
    ipaddr_set_any(&netif->netmask);
    ipaddr_set_any(&netif->gateway);
    netif->mtu = 0;                      // 默认为0，即不限制
    netif->type = NETIF_TYPE_NONE;
    nlist_node_init(&netif->node);

    plat_strncpy(netif->name, dev_name, NETIF_NAME_SIZE);
    netif->name[NETIF_NAME_SIZE - 1] = '\0';
    netif->ops = ops;                   // 设置驱动和私有参数
    netif->ops_data = (void *)ops_data;

    // 初始化接收队列
    net_err_t err = fixq_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif in_q init error, err: %d", err);
        return (netif_t *)0;
    }

    // 初始化发送队列
    err = fixq_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif out_q init error, err: %d", err);
        fixq_destroy(&netif->in_q);
        return (netif_t *)0;
    }

    // 打开设备，对硬件做进一步的设置, 在其内部可对netif字段进行设备
    // 特别是要对type和link_layer做设备
    err = ops->open(netif, ops_data);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif ops open error: %d");
        goto free_return;
    }
    netif->state = NETIF_OPENED;        // 切换为opened

    // 驱动初始化完成后，对netif进行进一步检查
    // 做一些必要性的检查，以免驱动没写好
    if (netif->type == NETIF_TYPE_NONE) {
        dbg_error(DBG_NETIF, "netif type unknown");
        goto free_return;
    }

    netif->link_layer = netif_get_layer(netif->type);
    if(!netif->link_layer && (netif->type != NETIF_TYPE_LOOP)) {
        dbg_error(DBG_NETIF, "no link layer, netif name: %s\n", dev_name);
        goto free_return;
    }

    // 插入队列中
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
 * @brief 设置IP地址、掩码、网关等
 * 这里只是简单的设置接口的各个地址，进行写入
 */
net_err_t netif_set_addr(netif_t* netif, ipaddr_t* ip, ipaddr_t* netmask, ipaddr_t* gateway) {
    ipaddr_copy(&netif->ipaddr, ip ? ip : ipaddr_get_any());
    ipaddr_copy(&netif->netmask, netmask ? netmask : ipaddr_get_any());
    ipaddr_copy(&netif->gateway, gateway ? gateway : ipaddr_get_any());
    return NET_ERR_OK;
}

/**
 * @brief 设置硬件地址
 */
net_err_t netif_set_hwaddr(netif_t* netif, const uint8_t* hwaddr, int len) {
    plat_memcpy(netif->hwaddr.addr, hwaddr, len);
    netif->hwaddr.len = len;
    return NET_ERR_OK;
}

/**
 * @brief 激活网络设备
 */
net_err_t netif_set_active(netif_t* netif) {
    // 必须为打开状态地能激活
    if (netif->state != NETIF_OPENED) {
        dbg_error(DBG_NETIF, "netif is not opened");
        return NET_ERR_STATE;
    }

    // 看看是否要添加缺省接口
    // 缺省网络接口用于外网数据收发时的包处理
    if (!netif_default && (netif->type != NETIF_TYPE_LOOP)) {
        netif_set_default(netif);
    }

    if(netif->link_layer) {
        net_err_t err = netif->link_layer->open(netif);
        if(err < 0) {
            dbg_info(DBG_NETIF, "active error");
            return err;
        }
    }

    // 切换为就绪状态
    netif->state = NETIF_ACTIVE;
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 取消设备的激活态
 */
net_err_t netif_set_deactive(netif_t* netif) {
    // 必须已经激活的状态
    if (netif->state != NETIF_ACTIVE) {
        dbg_error(DBG_NETIF, "netif is not actived");
        return NET_ERR_STATE;
    }

    if(netif->link_layer) {
        netif->link_layer->close(netif);
    }

    // 释放掉队列中的所有数据包
    pktbuf_t* buf;
    while ((buf = fixq_recv(&netif->in_q, -1))) {
        pktbuf_free(buf);
    }
    while ((buf = fixq_recv(&netif->out_q, -1))) {
        pktbuf_free(buf);
    }

    // 重设缺省网口
    if (netif_default == netif) {
        netif_default = (netif_t*)0;
    }

    // 切换回打开（非激活状态）
    netif->state = NETIF_OPENED;
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 关闭网络接口
 */
net_err_t netif_close(netif_t* netif) {
    // 需要先取消active状态才能关闭
    if (netif->state == NETIF_ACTIVE) {
        dbg_error(DBG_NETIF, "netif(%s) is active, close failed.", netif->name);
        return NET_ERR_STATE;
    }

    // 先关闭设备
    netif->ops->close(netif);
    netif->state = NETIF_CLOSED;

    // 最后释放netif结构
    nlist_remove(&netif_list, &netif->node);
    mblock_free(&netif_mblock, netif);
    display_netif_list();
    return NET_ERR_OK;
}

/**
 * @brief 设置缺省的网络接口
 * @param netif 缺省的网络接口
 */
void netif_set_default(netif_t* netif) {
    // 纪录新的网卡
    netif_default = netif;
}

/**
 * @brief 将buf加入到网络接口的输入队列中
 */
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 写入接收队列
    net_err_t err = fixq_send(&netif->in_q, buf, tmo);
    if (err < 0) {
        dbg_warning(DBG_NETIF, "netif %s in_q full", netif->name);
        return NET_ERR_FULL;
    }

    // 通知消息处理线程，这里不处理消息是否发送成功等问题
    // 消息满了不要紧，说明网卡正在忙，后续还会处理的
    exmsg_netif_in(netif);
    return NET_ERR_OK;
}

/**
 * @brief 将Buf添加到网络接口的输出队列中
 */
net_err_t netif_put_out(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 写入发送队列
    net_err_t err = fixq_send(&netif->out_q, buf, tmo);
    if (err < 0) {
        dbg_info(DBG_NETIF, "netif %s out_q full", netif->name);
        return err;
    }

    // 只是写入队列，具体的发送会调用ops->xmit来启动发送
    return NET_ERR_OK;
}

/**
 * @brief 从输入队列中取出一个数据包
 */
pktbuf_t* netif_get_in(netif_t* netif, int tmo) {
    // 从接收队列中取数据包
    pktbuf_t* buf = fixq_recv(&netif->in_q, tmo);
    if (buf) {
        // 重新定位，方便进行读写
        pktbuf_reset_acc(buf);
        return buf;
    }

    dbg_info(DBG_NETIF, "netif %s in_q empty", netif->name);
    return (pktbuf_t*)0;
}

/**
 * 从输出队列中取出一个数据包
 */
 pktbuf_t* netif_get_out(netif_t* netif, int tmo) {
    // 从发送队列中取数据包，不需要等待。可能会被中断处理程序中调用
    // 因此，不能因为没有包而挂起程序
    pktbuf_t* buf = fixq_recv(&netif->out_q, tmo);
    if (buf) {
        // 重新定位，方便进行读写
        pktbuf_reset_acc(buf);
        return buf;
    }

    dbg_info(DBG_NETIF, "netif %s out_q empty", netif->name);
    return (pktbuf_t*)0;
}

/**
 * @brief 发送一个网络包到网络接口上, 目标地址为ipaddr
 * 在发送前，先判断驱动是否正在发送，如果是则将其插入到发送队列，等驱动有空后，由驱动自行取出发送。
 * 否则，加入发送队列后，启动驱动发送
 */
net_err_t netif_out(netif_t* netif, ipaddr_t * ipaddr, pktbuf_t* buf) {
    // 缺省情况，将数据包插入就绪队列，然后通知驱动程序开始发送
    // 硬件当前发送如果未进行，则启动发送，否则不处理，等待硬件中断自动触发进行发送
    if(netif->link_layer) {
        net_err_t err = ether_raw_out(netif, NET_PROTOCOL_ARP, ether_broadcast_addr(), buf);
        if(err < 0) {
            dbg_warning(DBG_NETIF, "netif link out err");
            return err;
        }
        return NET_ERR_OK;
    }
    else
    {
        net_err_t err = netif_put_out(netif, buf, -1);
        if (err < 0) {
            dbg_info(DBG_NETIF, "send to netif queue failed. err: %d", err);
            return err;
        }
        
        // 启动发送
        return netif->ops->xmit(netif);
    }
}