#include "mblock.h"

#include "net_cfg.h"
#include "dbg.h"
#include "nlist.h"

net_err_t mblock_init(mblock_t* mblock, void* mem, int blk_size, int cnt, nlocker_type_t share_type)
{
    // 链表使用了nlist_node结构，所以大小必须合适
    dbg_assert(blk_size >= sizeof(nlist_node_t), "size error");

    // 将缓存区分割成一块块固定大小内存，插入到队列中
    uint8_t* buf = (uint8_t*)mem;
    nlist_init(&mblock->free_list);
    for (int i = 0; i < cnt; i++, buf += blk_size) {
        nlist_node_t* block = (nlist_node_t*)buf;
        nlist_node_init(block);
        nlist_insert_last(&mblock->free_list, block);
    }

    // 初始化锁
    nlocker_init(&mblock->locker, share_type);

    // 创建分配同步用的信号量，由于线程访问处理
    if (share_type != NLOCKER_NONE) {
        mblock->alloc_sem = sys_sem_create(cnt);
        if (mblock->alloc_sem == SYS_SEM_INVALID) {
            dbg_error(DBG_MBLOCK, "create sem failed.");
            nlocker_destroy(&mblock->locker);
            return NET_ERR_SYS;
        }
    }
    mblock->start = mem;
    return NET_ERR_OK;
}

void* mblock_alloc(mblock_t* mblock, int ms)
{
    if((ms < 0) || (mblock->locker.type == NLOCKER_NONE))
    {
        nlocker_lock(&mblock->locker);
        int count = nlist_count(&mblock->free_list);
        if(count == 0)
        {
            nlocker_unlock(&mblock->locker);
            return (void*)0;
        }
        else
        {
            nlist_node_t* block = nlist_remove_first(&mblock->free_list);
            nlocker_unlock(&mblock->locker);
            return block;
        }
    }
    else
    {
        if(sys_sem_wait(mblock->alloc_sem, ms) < 0)
        {
            return (void*)0;
        }
        else
        {
            nlocker_lock(&mblock->locker);
            nlist_node_t* block = nlist_remove_first(&mblock->free_list);
            nlocker_unlock(&mblock->locker);
            return block;
        }
    }
    return (void*)0;
}

int mblock_free_cnt(mblock_t* mblock)
{
    nlocker_lock(&mblock->locker);
    int count = nlist_count(&mblock->free_list);
    nlocker_unlock(&mblock->locker);
    return count;
}

void mblock_free(mblock_t* mblock, void* block)
{
    nlocker_lock(&mblock->locker);
    nlist_insert_last(&mblock->free_list, (nlist_node_t *)block);
    nlocker_unlock(&mblock->locker);

    if(mblock->locker.type != NLOCKER_NONE)
    {
        sys_sem_notify(mblock->alloc_sem);
    }
}

void mblock_destroy(mblock_t* mblock)
{
    if(mblock->locker.type != NLOCKER_NONE)
    {
        sys_sem_free(mblock->alloc_sem);
        nlocker_destroy(&mblock->locker);
    }
}