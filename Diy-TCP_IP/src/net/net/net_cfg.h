﻿#ifndef NET_CFG_H
#define NET_CFG_H

// 调试信息输出
#define DBG_MBLOCK		    DBG_LEVEL_INFO			// 内存块管理器
#define DBG_QUEUE           DBG_LEVEL_INFO          // 定长存储块
#define DBG_MSG             DBG_LEVEL_INFO          // 消息通信
#define DBG_BUF             DBG_LEVEL_INFO          // 数据包管理器
#define DBG_PLAT            DBG_LEVEL_INFO          // 系统平台
#define DBG_INIT            DBG_LEVEL_INFO          // 系统初始化
#define DBG_NETIF           DBG_LEVEL_INFO          // 网络接口层
#define DBG_ETHER           DBG_LEVEL_INFO
#define DBG_TOOLS           DBG_LEVEL_INFO

#define NET_ENDIAN_LITTLE       1

#define EXMSG_MSG_CNT          10                 // 消息缓冲区大小
#define EXMSG_BLOCKER        NLOCKER_THREAD      // 核心线程的锁类型

#define PKTBUF_BLK_SIZE         128                 // 数据包块大小
#define PKTBUF_BLK_CNT          100                 // 数据包块的数量
#define PKTBUF_BUF_CNT          100                 // 数据包的数量

#define NETIF_HWADDR_SIZE           10                  // 硬件地址长度，mac地址最少6个字节
#define NETIF_NAME_SIZE             10                  // 网络接口名称大小
#define NETIF_DEV_CNT               4                   // 网络接口的数量
#define NETIF_INQ_SIZE             50                  // 网卡输入队列最大容量
#define NETIF_OUTQ_SIZE            50                  // 网卡输出队列最大容量

#endif