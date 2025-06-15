#include "nlist.h"

/**
 * 鍒濆鍖栭摼琛?
 * @param list 寰呭垵濮嬪寲鐨勯摼琛?
 */
void nlist_init(nlist_t *list) {
    list->first = list->last = (nlist_node_t *)0;
    list->count = 0;
}

/**
 * 灏嗘寚瀹氳〃椤规彃鍏ュ埌鎸囧畾閾捐〃鐨勫ご閮?
 * @param list 寰呮彃鍏ョ殑閾捐〃
 * @param node 寰呮彃鍏ョ殑缁撶偣
 */
void nlist_insert_first(nlist_t *list, nlist_node_t *node) {
    // 璁剧疆濂藉緟鎻掑叆缁撶偣鐨勫墠鍚庯紝鍓嶉潰涓虹┖
    node->next = list->first;
    node->pre = (nlist_node_t *)0;

    // 濡傛灉涓虹┖锛岄渶瑕佸悓鏃惰缃甪irst鍜宭ast鎸囧悜鑷繁
    if (nlist_is_empty(list)) {
        list->last = list->first = node;
    } else {
        // 鍚﹀垯锛岃缃ソ鍘熸湰绗竴涓粨鐐圭殑pre
        list->first->pre = node;

        // 璋冩暣first鎸囧悜
        list->first = node;
    }

    list->count++;
}

/**
 * 绉婚櫎鎸囧畾閾捐〃鐨勪腑鐨勮〃椤?
 * 涓嶆鏌ode鏄惁鍦ㄧ粨鐐逛腑
 */
nlist_node_t * nlist_remove(nlist_t *list, nlist_node_t *remove_node) {
    // 濡傛灉鏄ご锛屽ご寰€鍓嶇Щ
    if (remove_node == list->first) {
        list->first = remove_node->next;
    }

    // 濡傛灉鏄熬锛屽垯灏惧線鍥炵Щ
    if (remove_node == list->last) {
        list->last = remove_node->pre;
    }

    // 濡傛灉鏈夊墠锛屽垯璋冩暣鍓嶇殑鍚庣户
    if (remove_node->pre) {
        remove_node->pre->next = remove_node->next;
    }

    // 濡傛灉鏈夊悗锛屽垯璋冩暣鍚庡線鍓嶇殑
    if (remove_node->next) {
        remove_node->next->pre = remove_node->pre;
    }

    // 娓呯┖node鎸囧悜
    remove_node->pre = remove_node->next = (nlist_node_t*)0;
    --list->count;
    return remove_node;
}

/**
 * 灏嗘寚瀹氳〃椤规彃鍏ュ埌鎸囧畾閾捐〃鐨勫熬閮?
 * @param list 鎿嶄綔鐨勯摼琛?
 * @param node 寰呮彃鍏ョ殑缁撶偣
 */
void nlist_insert_last(nlist_t *list, nlist_node_t *node) {
    // 璁剧疆濂界粨鐐规湰韬?
    node->pre = list->last;
    node->next = (nlist_node_t*)0;

    // 琛ㄧ┖锛屽垯first/last閮芥寚鍚戝敮涓€鐨刵ode
    if (nlist_is_empty(list)) {
        list->first = list->last = node;
    } else {
        // 鍚﹀垯锛岃皟鏁磍ast缁撶偣鐨勫悜涓€鎸囧悜涓簄ode
        list->last->next = node;

        // node鍙樻垚浜嗘柊鐨勫悗缁х粨鐐?
        list->last = node;
    }

    list->count++;
}

/**
 * 灏哊ode鎻掑叆鎸囧畾缁撶偣涔嬪悗
 * @param 鎿嶄綔鐨勯摼琛?
 * @param pre 鍓嶄竴缁撶偣
 * @param node 寰呮彃鍏ョ殑缁撶偣
 */
void nlist_insert_after(nlist_t* list, nlist_node_t* pre, nlist_node_t* node) {
    // 鍘熼摼琛ㄤ负绌?
    if (nlist_is_empty(list)) {
        nlist_insert_first(list, node);
        return;
    }

    // node鐨勪笅涓€缁撶偣锛屽簲褰撲负pre鐨勪笅涓€缁撶偣
    node->next = pre->next;
    node->pre = pre;

    // 鍏堣皟鏁村悗绮ワ紝鍐嶆洿鏂拌嚜宸?
    if (pre->next) {
        pre->next->pre = node;
    }
    pre->next = node;


    // 濡傛灉pre鎭板ソ浣嶄簬琛ㄥ熬锛屽垯鏂扮殑琛ㄥ熬灏遍渶瑕佹洿鏂版垚node
    if (list->last == pre) {
        list->last = node;
    }

    list->count++;
}