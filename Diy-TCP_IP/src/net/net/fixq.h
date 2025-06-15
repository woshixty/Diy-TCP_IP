#ifndef FIXQ_H
#define FIXQ_H

#include "nlist.h"
#include "nlocker.h"
#include "net_plat.h"

/**
 * 鍥哄畾闀垮害鐨勬秷鎭槦鍒?
 */
typedef struct _fixq_t{
    int size;                               // 娑堟伅瀹归噺
    void** buf;                             // 娑堟伅缂撳瓨
    int in, out, cnt;                       // 璇诲啓浣嶇疆绱㈠紩锛屾秷鎭暟
    nlocker_t locker;                       // 璁块棶鐨勯攣
    sys_sem_t recv_sem;                     // 鎺ユ敹娑堟伅鏃朵娇鐢ㄧ殑淇″彿閲?
    sys_sem_t send_sem;                     // 鍙戦€佹秷鎭椂浣挎湁鐨勪俊鍙烽噺
}fixq_t;

net_err_t fixq_init(fixq_t * q, void ** buf, int size, nlocker_type_t share_type);
net_err_t fixq_send(fixq_t* q, void* msg, int tmo);
void * fixq_recv(fixq_t* q, int tmo);
void fixq_destroy(fixq_t * q);
int fixq_count (fixq_t *q);

#endif