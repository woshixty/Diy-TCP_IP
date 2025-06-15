#include "nlocker.h"

/**
 * 鍒濆鍖栬祫婧愰攣
 */
net_err_t nlocker_init(nlocker_t * locker, nlocker_type_t type) {
    if (type == NLOCKER_THREAD) {
        sys_mutex_t mutex = sys_mutex_create();
        if (mutex == SYS_MUTEx_INVALID) {
            return NET_ERR_SYS;
        }
        locker->mutex = mutex;
    }

    locker->type = type;
    return NET_ERR_OK;
}

/**
 * @brief 閿€姣侀攣
 */
void nlocker_destroy(nlocker_t * locker) {
    if (locker->type == NLOCKER_THREAD) {
        sys_mutex_free(locker->mutex);
    }
}

/**
 * @brief 涓婇攣
 */
void nlocker_lock(nlocker_t * locker) {
    if (locker->type == NLOCKER_THREAD) {
        sys_mutex_lock(locker->mutex);
    }
}

/**
 * @brief 瑙ｉ攣
 */
void nlocker_unlock(nlocker_t * locker) {
    if (locker->type == NLOCKER_THREAD) {
        sys_mutex_unlock(locker->mutex);
    } 
}