#include "nlist.h"

void nlist_init(nlist_t* list)
{
    list->first = list->last = nullptr;
    list->count = 0;
}