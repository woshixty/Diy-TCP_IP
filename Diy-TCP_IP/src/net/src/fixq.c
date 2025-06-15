#include "fixq.h"
#include "nlocker.h"
#include "dbg.h"
#include "sys.h"

/**
 * @brief 鍒濆鍖栧畾闀挎秷鎭槦鍒?
 */
net_err_t fixq_init(fixq_t *q, void **buf, int size, nlocker_type_t type) {
    q->size = size;
    q->in = q->out = q->cnt = 0;
    q->buf = (void **)0;
    q->recv_sem = SYS_SEM_INVALID;
    q->send_sem = SYS_SEM_INVALID;

    // 鍒涘缓閿?
    net_err_t err = nlocker_init(&q->locker, type);
    if (err < 0) {
        dbg_error(DBG_QUEUE, "init locker failed!");
        return err;
    }

    // 鍒涘缓鍙戦€佷俊鍙烽噺
    q->send_sem = sys_sem_create(size);
    if (q->send_sem == SYS_SEM_INVALID)  {
        dbg_error(DBG_QUEUE, "create sem failed!");
        err = NET_ERR_SYS;
        goto init_failed;
    }

    q->recv_sem = sys_sem_create(0);
    if (q->recv_sem == SYS_SEM_INVALID) {
        dbg_error(DBG_QUEUE, "create sem failed!");
        err = NET_ERR_SYS;
        goto init_failed;
    }

    q->buf = buf;
    return NET_ERR_OK;
init_failed:
    if (q->recv_sem != SYS_SEM_INVALID) {
        sys_sem_free(q->recv_sem);
    }

    if (q->send_sem != SYS_SEM_INVALID) {
        sys_sem_free(q->send_sem);
    }

    nlocker_destroy(&q->locker);
    return err;
}

/**
 * @brief 鍚戞秷鎭槦鍒楀啓鍏ヤ竴涓秷鎭?
 * 濡傛灉娑堟伅闃熷垪婊★紝鍒欑湅涓媡mo锛屽鏋渢mo < 0鍒欎笉绛夊緟
 */
net_err_t fixq_send(fixq_t *q, void *msg, int tmo) {
    nlocker_lock(&q->locker);
    if ((q->cnt >= q->size) && (tmo < 0)) {
        // 濡傛灉缂撳瓨宸叉弧锛屽苟涓斾笉闇€瑕佺瓑寰咃紝鍒欑珛鍗抽€€鍑?
        nlocker_unlock(&q->locker);
        return NET_ERR_FULL;
    }
    nlocker_unlock(&q->locker);

    // 娑堣€楁帀涓€涓┖闂茶祫婧愶紝濡傛灉涓虹┖鍒欎細绛夊緟
    if (sys_sem_wait(q->send_sem, tmo) < 0) {
        return NET_ERR_TMO;
    }

    // 鏈夌┖闂插崟鍏冨啓鍏ョ紦瀛?
    nlocker_lock(&q->locker);
    q->buf[q->in++] = msg;
    if (q->in >= q->size) {
        q->in = 0;
    }
    q->cnt++;
    nlocker_unlock(&q->locker);

    // 閫氱煡鍏跺畠杩涚▼鏈夋秷鎭彲鐢?
    sys_sem_notify(q->recv_sem);
    return NET_ERR_OK;
}

/**
 * @brief 浠庢暟鎹寘闃熷垪涓彇涓€涓秷鎭紝濡傛灉鏃狅紝鍒欑瓑寰?
 */
void *fixq_recv(fixq_t *q, int tmo) {
    // 濡傛灉缂撳瓨涓虹┖涓斾笉闇€瑕佺瓑锛屽垯绔嬪嵆閫€鍑?
    nlocker_lock(&q->locker);
    if (!q->cnt && (tmo < 0)) {
        nlocker_unlock(&q->locker);
        return (void *)0;
    }
    nlocker_unlock(&q->locker);

    // 鍦ㄤ俊鍙烽噺涓婄瓑寰呮暟鎹寘鍙敤
    if (sys_sem_wait(q->recv_sem, tmo) < 0) {
        return (void *)0;
    }

    // 鍙栨秷鎭?
    nlocker_lock(&q->locker);
    void *msg = q->buf[q->out++];
    if (q->out >= q->size) {
        q->out = 0;
    }
    q->cnt--;
    nlocker_unlock(&q->locker);

    // 閫氱煡鏈夌┖闂茬┖闂村彲鐢?
    sys_sem_notify(q->send_sem);
    return msg;
}

/**
 * 閿€姣侀槦鍒?
 * @param list 寰呴攢姣佺殑闃熷垪
 */
void fixq_destroy(fixq_t *q) {
    nlocker_destroy(&q->locker);
    sys_sem_free(q->send_sem);
    sys_sem_free(q->recv_sem);
}

/**
 * @brief 鍙栫紦鍐蹭腑娑堟伅鐨勬暟閲?
 */
int fixq_count (fixq_t *q) {
    nlocker_lock(&q->locker);
    int count = q->cnt;
    nlocker_unlock(&q->locker);
    return count;
}