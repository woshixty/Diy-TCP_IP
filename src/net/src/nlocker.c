#include "nlocker.h"

net_err_t nlocker_init(nlocker_t* locaker, nlocker_type_t type)
{
    if(type == NLOCKER_THREAD)
    {
        sys_mutex_t mutex = sys_mutex_create();
        if(mutex == SYS_MUTEx_INVALID)
        {
            return NET_ERR_SYS;
        }
        locaker->mutex = mutex;
    }
    locaker->type = type;
    return NET_ERR_OK;
}

void nlocker_destroy(nlocker_t* locaker)
{
    if(locaker->type == NLOCKER_THREAD)
    {
        sys_mutex_free(locaker->mutex);
    }
}

void nlocker_lock(nlocker_t* locaker)
{
    if(locaker->type == NLOCKER_THREAD)
    {
        sys_mutex_lock(locaker->mutex);
    }
}

void nlocker_unlock(nlocker_t* locaker)
{
    if(locaker->type == NLOCKER_THREAD)
    {
        sys_mutex_unlock(locaker->mutex);
    }
}