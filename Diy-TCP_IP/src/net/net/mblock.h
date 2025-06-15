#ifndef MBLOCK_H
#define MBLOCK_H

#include "nlist.h"
#include "nlocker.h"

/**
 * @brief 瀛樺偍绠″潡绠＄悊鍣?
 */
typedef struct _mblock_t{
    void* start;                        // 鎵€鏈夊瓨鍌ㄧ殑璧峰鍦板潃
    nlist_t free_list;                   // 绌洪棽鐨勬秷鎭槦鍒?
    nlocker_t locker;                   // 璁块棶鐨勯攣
    sys_sem_t alloc_sem;                // 鍒嗛厤鍚屾鐢ㄤ俊鍙烽噺
}mblock_t;

net_err_t mblock_init (mblock_t* mblock, void * mem, int blk_size, int cnt, nlocker_type_t share_type);
void * mblock_alloc(mblock_t * block, int ms);
int mblock_free_cnt(mblock_t* list);
void mblock_free(mblock_t * list, void * block);
void mblock_destroy(mblock_t* block);

#endif