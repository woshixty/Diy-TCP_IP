#ifndef NLIST_H
#define NLIST_H

typedef struct _nlist_node_t
{
    struct _nlist_node_t* pre;
    struct _nlist_node_t* next;
} nlist_node_t;

static inline void nlist_node_init(nlist_node_t* node) {
    node->next = node->pre = (nlist_node_t*) 0;
}

static inline nlist_node_t* nlist_node_next(nlist_node_t* node) {
    return node->next;
}

static inline nlist_node_t* nlist_node_pre(nlist_node_t* node) {
    return node->pre;
}

static inline void nlist_node_set_next(nlist_node_t* node, nlist_node_t* next) {
    node->next = next;
}

typedef struct _nlist_t {
    nlist_node_t* first;
    nlist_node_t* last;
    int count;
} nlist_t;

void nlist_init(nlist_t* list);

static inline int nlist_is_empty(nlist_t* list) {
    return list->count == 0;
}

static inline int nlist_count(nlist_t* list) {
    return list->count;
}

static inline nlist_node_t* nlist_first(nlist_t* list) {
    return list->first;
}

static inline nlist_node_t* nlist_last(nlist_t* list) {
    return list->last;
}

void nlist_insert_first(nlist_t* list, nlist_node_t* node);

/**
 * @brief 将结构体中某个字段的地址转换为所在结构体的指针
 * 例如：
 * struct aa{
 *  .....
 *  int node;
 *  .....
 * };
 * struct aa a;
 * 1.求结点在所在结构中的偏移:定义一个指向0的指针，用(struct aa *)&0->node，所得即为node字段在整个结构体的偏移
 */
#define noffset_in_parent(parent_type, node_name)    \
    ((char *)&(((parent_type*)0)->node_name))

// 2.求node所在的结构体首址：node的地址 - node的偏移
// 即已知a->node的地址，求a的地址
#define noffset_to_parent(node, parent_type, node_name)   \
    ((char *)node - noffset_in_parent(parent_type, node_name))

// 3. 进行转换: (struct aa *)addr
// 使用方式：net_node_to_parent(node_addr, struct aa, node)
#define nlist_entry(node, parent_type, node_name)   \
        ((parent_type *)(node ? noffset_to_parent((node), parent_type, node_name) : 0))

#define nlist_for_each(node, list) for ((node) = (list)->first; (node); (node) = (node)->next)

nlist_node_t* nlist_remove(nlist_t* list, nlist_node_t* node);

static inline nlist_node_t* nlist_remove_first(nlist_t* list)
{
    nlist_node_t* first = nlist_first(list);
    if(first) {
        nlist_remove(list, first);
    }
    return first;
}

#endif