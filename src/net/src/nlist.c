#include "nlist.h"

void nlist_init(nlist_t* list)
{
    list->first = list->last = (nlist_node_t*) 0;
    list->count = 0;
}

void nlist_insert_first(nlist_t* list, nlist_node_t* node)
{
    node->pre = (nlist_node_t*) 0;
    node->next = list->first;
    if(nlist_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->first->pre = node;
        list->first = node;
    }
    list->count++;
}

void nlist_insert_last(nlist_t* list, nlist_node_t* node)
{
    node->next = (nlist_node_t*)0;
    node->pre = list->last;
    if(nlist_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }
    list->count++;
}

nlist_node_t* nlist_remove(nlist_t* list, nlist_node_t* node)
{
    if(list->first == node)
    {
        list->first = node->next;
    }
    if(list->last == node)
    {
        list->last = node->pre;
    }
    if(node->pre)
    {
        node->pre->next = node->next;
    }
    if(node->next)
    {
        node->next->pre = node->pre;
    }
    node->pre = (nlist_node_t*)0;
    node->next = (nlist_node_t*)0;
    list->count--;
    return node;
}