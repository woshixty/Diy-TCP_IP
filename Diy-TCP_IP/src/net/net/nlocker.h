#ifndef NLOCKER_H
#define NLOCKER_H

#include "sys.h"

/**
 * @brief 閿佺殑绫诲瀷
 * 
 */
typedef enum _nlocker_type_t {
    NLOCKER_NONE,                   // 涓嶉渶瑕侀攣
    NLOCKER_THREAD,                 // 鐢ㄤ簬绾跨▼鍏变韩鐨勯攣
}nlocker_type_t;

typedef struct _nlocker_t {
    nlocker_type_t type;                // 閿佺殑绫诲瀷

    // 鏍规嵁涓嶅悓鐨勯攣绫诲瀷锛屾斁缃笉鍚岀殑缁撴瀯
    union {
        sys_mutex_t mutex;           // 鐢ㄤ簬绾跨▼涔嬮棿璁块棶鐨勪簰鏂ラ攣
    };
}nlocker_t;

net_err_t nlocker_init(nlocker_t * locker, nlocker_type_t type);
void nlocker_destroy(nlocker_t * locker);
void nlocker_lock(nlocker_t * locker);
void nlocker_unlock(nlocker_t * locker);

#endif