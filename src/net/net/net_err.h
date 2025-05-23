#ifndef NET_ERR_H
#define NET_ERR_H

/**
 * 错误码及其类型
 */
typedef enum _net_err_t {
    NET_ERR_OK = 0,                         // 没有错误
    NET_ERR_SYS = -1,                       // 操作系统错误
    NET_ERR_MEM = -2,                       // 存储错误
    NET_ERR_FULL = -3,                      // 缓存满
    NET_ERR_TMO = -4,                       // 超时
    NET_ERR_SIZE = -5,
    NET_ERR_NONE = -6,
}net_err_t;

#endif // NET_ERR_H
