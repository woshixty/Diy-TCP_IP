#include <string.h>
#include "net_err.h"
#include "pktbuf.h"
#include "mblock.h"
#include "dbg.h"

static nlocker_t locker;                    // 鍒嗛厤涓庡洖鏀剁殑閿?
static mblock_t block_list;                 // 绌洪棽鍖呭垪琛?
static pktblk_t block_buffer[PKTBUF_BLK_CNT];
static mblock_t pktbuf_list;                // 绌洪棽鍖呭垪琛?
static pktbuf_t pktbuf_buffer[PKTBUF_BUF_CNT];

/**
 * 鑾峰彇浠ュ綋鍓嶄綅缃€岃█锛屼綑涓嬪灏戞€诲叡鏈夋晥鐨勬暟鎹┖闂?
 */
static inline int total_blk_remain(pktbuf_t* buf) {
    return buf->total_size - buf->pos;
}

/**
 * 鑾峰彇浠呭湪褰撳墠鐨刡lck涓綑涓嬪灏戞暟鎹┖闂?
 */
static int curr_blk_remain(pktbuf_t* buf) {
    pktblk_t* block = buf->curr_blk;
    if (!block) {
        return 0;
    }

    return (int)(buf->curr_blk->data + block->size - buf->blk_offset);
}

/**
 * 鑾峰彇blk鐨勫墿浣欑┖闂村ぇ灏?
 */
static inline int curr_blk_tail_free(pktblk_t* blk) {
    return PKTBUF_BLK_SIZE - (int)(blk->data - blk->payload) - blk->size;
}

/**
 * 鎵撳嵃缂撳啿琛ㄩ摼銆佸悓鏃舵鏌ヨ〃閾剧殑鏄惁姝ｇ‘閰嶇疆
 *
 * 鍦ㄦ墦鍗扮殑杩囩▼涓紝鍚屾椂瀵规暣涓紦瀛橀摼琛ㄨ繘琛屾鏌ワ紝鐪嬬湅鏄惁瀛樺湪閿欒
 * 涓昏閫氳繃妫€鏌ョ┖闂村拰size鐨勮缃槸鍚︽纭潵鍒ゆ柇缂撳瓨鏄惁姝ｇ‘璁剧疆
 *
 * @param buf 寰呮煡璇㈢殑Buf
 */
#if DBG_DISP_ENABLED(DBG_BUF)
static void display_check_buf(pktbuf_t* buf) {
    if (!buf) {
        dbg_error(DBG_BUF, "invalid buf. buf == 0");
        return;
    }

    plat_printf("check buf %p: size %d\n", buf, buf->total_size);
    pktblk_t* curr;
    int total_size = 0, index = 0;
    for (curr = pktbuf_first_blk(buf); curr; curr = pktbuf_blk_next(curr)) {
        plat_printf("%d: ", index++);

        if ((curr->data < curr->payload) || (curr->data >= curr->payload + PKTBUF_BLK_SIZE)) {
            dbg_error(DBG_BUF, "bad block data. ");
        }

        int pre_size = (int)(curr->data - curr->payload);
        plat_printf("Pre:%d b, ", pre_size);

        // 涓棿瀛樺湪鐨勫凡鐢ㄥ尯鍩?
        int used_size = curr->size;
        plat_printf("Used:%d b, ", used_size);

        // 鏈熬鍙兘瀛樺湪鐨勬湭鐢ㄥ尯鍩?
        int free_size = curr_blk_tail_free(curr);
        plat_printf("Free:%d b, ", free_size);
        plat_printf("\n");

        // 妫€鏌ュ綋鍓嶅寘鐨勫ぇ灏忔槸鍚︿笌璁＄畻鐨勪竴鑷?
        int blk_total = pre_size + used_size + free_size;
        if (blk_total != PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF,"bad block size. %d != %d", blk_total, PKTBUF_BLK_SIZE);
        }

        // 绱鎬荤殑澶у皬
        total_size += used_size;
    }

    // 妫€鏌ユ€荤殑澶у皬鏄惁涓€鑷?
    if (total_size != buf->total_size) {
        dbg_error(DBG_BUF,"bad buf size. %d != %d", total_size, buf->total_size);
    }
}
#else
#define display_check_buf(buf)
#endif

/**
 * 鍑嗗pktbuf鐨勮鍐?
 */
void pktbuf_reset_acc(pktbuf_t* buf) {
    if (buf) {
        buf->pos = 0;
        buf->curr_blk = pktbuf_first_blk(buf);
        buf->blk_offset = buf->curr_blk ? buf->curr_blk->data : (uint8_t*)0;
    }
}

/**
 * 閲婃斁涓€涓猙lock
 */
static void pktblock_free (pktblk_t * block) {
    nlocker_lock(&locker);
    mblock_free(&block_list, block);
    nlocker_unlock(&locker);
}

/**
 * 鍥炴敹block閾?
 */
static void pktblock_free_list(pktblk_t* first) {
    while (first) {
        // 鍏堝彇涓嬩竴涓?
        pktblk_t* next_block = pktbuf_blk_next(first);

        // 鐒跺悗閲婃斁
        mblock_free(&block_list, first);

        // 鐒跺悗璋冩暣褰撳墠鐨勫鐞嗗鍍?
        first = next_block;
    }
}

/**
 * @brief 閲婃斁鏁版嵁鍖?
 */
void pktbuf_free (pktbuf_t * buf) {
    nlocker_lock(&locker);
    if (--buf->ref == 0) {
        pktblock_free_list(pktbuf_first_blk(buf));
        mblock_free(&pktbuf_list, buf);
    }
    nlocker_unlock(&locker);
}

/**
 * @brief 鍒嗛厤涓€涓┖闂茬殑block
 */
static pktblk_t* pktblock_alloc(void) {
    // 涓嶇瓑寰呭垎閰嶏紝鍥犱负浼氬湪涓柇涓皟鐢?
    nlocker_lock(&locker);
    pktblk_t* block = mblock_alloc(&block_list, -1);
    nlocker_unlock(&locker);

    if (block) {
        block->size = 0;
        block->data = (uint8_t *)0;
        nlist_node_init(&block->node);
    }

    return block;
}

/**
 * @brief 鍒嗛厤涓€涓紦鍐查摼
 *
 * 鐢变簬鍒嗛厤鏄互BUF鍧椾负鍗曚綅锛屾墍浠lloc_size鐨勫ぇ灏忓彲鑳戒細灏忎簬瀹為檯鍒嗛厤鐨凚UF鍧楃殑鎬诲ぇ灏?
 * 閭ｄ箞姝ゆ椂灏辨湁涓€閮ㄥ垎绌洪棿鏈敤锛岃繖閮ㄥ垎绌洪棿鍙兘鏀惧湪閾捐〃鐨勬渶寮€濮嬶紝涔熷彲鑳芥斁鍦ㄩ摼琛ㄧ殑缁撳熬澶?
 * 鍏蜂綋瀛樺偍锛屽彇鍐充簬add_front锛宎dd_front=1锛屽皢鏂板垎閰嶇殑鍧楁彃鍏ュ埌琛ㄥご.鍚﹀垯锛屾彃鍏ュ埌琛ㄥ熬
 *
 */
static pktblk_t* pktblock_alloc_list(int size, int add_front) {
    pktblk_t* first_block = (pktblk_t*)0;
    pktblk_t* pre_block = (pktblk_t*)0;

    while (size) {
        // 鍒嗛厤涓€涓猙lock锛屽ぇ灏忎负0
        pktblk_t* new_block = pktblock_alloc();
        if (!new_block) {
            dbg_error(DBG_BUF, "no buffer for alloc(size:%d)", size);
            if (first_block) {
                // 澶辫触锛岃鍥炴敹閲婃斁鏁翠釜閾?
                nlocker_lock(&locker);
                pktblock_free_list(first_block);
                nlocker_unlock(&locker);
            }
            return (pktblk_t*)0;
        }

        int curr_size = 0;
        if (add_front) {
            curr_size = size > PKTBUF_BLK_SIZE ? PKTBUF_BLK_SIZE : size;

            // 鍙嶅悜鍒嗛厤锛屼粠鏈寰€鍓嶅垎閰嶇┖闂?
            new_block->size = curr_size;
            new_block->data = new_block->payload + PKTBUF_BLK_SIZE - curr_size;
            if (first_block) {
                // 灏嗚嚜宸卞姞鍦ㄥご閮?
                nlist_node_set_next(&new_block->node, &first_block->node);
            }

            // 濡傛灉鏄弽鍚戝垎閰嶏紝绗竴涓寘鎬绘槸褰撳墠鍒嗛厤鐨勫寘
            first_block = new_block;
        } else {
            // 濡傛灉鏄鍚戝垎閰嶏紝绗竴涓寘鏄1涓垎閰嶇殑鍖?
            if (!first_block) {
                first_block = new_block;
            }

            curr_size = size > PKTBUF_BLK_SIZE ? PKTBUF_BLK_SIZE : size;

            // 姝ｅ悜鍒嗛厤锛屼粠鍓嶇鍚戞湯绔垎閰嶇┖闂?
            new_block->size = curr_size;
            new_block->data = new_block->payload;
            if (pre_block) {
                // 灏嗚嚜宸辨坊鍔犲埌琛ㄥ熬
                nlist_node_set_next(&pre_block->node, &new_block->node);
            }
        }

        size -= curr_size;
        pre_block = new_block;
    }

    return first_block;
}

/**
 * 灏咮lock閾捐〃鎻掑叆鍒癰uf涓?
 */
static void pktbuf_insert_blk_list(pktbuf_t * buf, pktblk_t * first_blk, int add_last) {
    if (add_last) {
        // 鎻掑叆灏鹃儴
        while (first_blk) {
            // 涓嶆柇浠巉irst_blk涓彇鍧楋紝鐒跺悗鎻掑叆鍒癰uf鐨勫悗闈?
            pktblk_t* next_blk = pktbuf_blk_next(first_blk);

            nlist_insert_last(&buf->blk_list, &first_blk->node);         // 鎻掑叆鍒板悗闈?
            buf->total_size += first_blk->size;

            first_blk = next_blk;
        }
    } else {
        pktblk_t *pre = (pktblk_t*)0;

        // 閫愪釜鍙栧ご閮ㄧ粨鐐逛緷娆℃彃鍏?
        while (first_blk) {
            pktblk_t *next = pktbuf_blk_next(first_blk);

            if (pre) {
                nlist_insert_after(&buf->blk_list, &pre->node, &first_blk->node);
            } else {
                nlist_insert_first(&buf->blk_list, &first_blk->node);
            }
            buf->total_size += first_blk->size;

            pre = first_blk;
            first_blk = next;
        };
    }
}


/**
 * 澧炲姞buf鐨勫紩鐢ㄦ鏁?
 * @param buf
 */
void pktbuf_inc_ref (pktbuf_t * buf) {
    nlocker_lock(&locker);
    buf->ref++;
    nlocker_unlock(&locker);
}

/**
 * @brief 鍒嗛厤鏁版嵁鍖呭潡
 */
pktbuf_t* pktbuf_alloc(int size) {
    // 鍒嗛厤涓€涓粨鏋?
    nlocker_lock(&locker);
    pktbuf_t* buf = mblock_alloc(&pktbuf_list, -1);
    nlocker_unlock(&locker);
    if (!buf) {
        dbg_error(DBG_BUF, "no pktbuf");
        return (pktbuf_t*)0;
    }

    // 瀛楁鍊煎垵濮嬪寲
    buf->ref = 1;
    buf->total_size = 0;
    nlist_init(&buf->blk_list);
    nlist_node_init(&buf->node);

    // 鍒嗛厤鍧楃┖闂?
    if (size) {
        pktblk_t* block = pktblock_alloc_list(size, 0);
        if (!block) {
            mblock_free(&pktbuf_list, buf);
            return (pktbuf_t*)0;
        }
        pktbuf_insert_blk_list(buf, block, 1);
    }

    // 璁剧疆褰撳墠璇诲啓鎯呭喌鍙婂紩鐢ㄦ儏鍐?
    pktbuf_reset_acc(buf);

    // 妫€鏌ヤ竴涓媌uf鐨勫畬鏁存€у拰姝ｇ‘鎬?
    display_check_buf(buf);
    return buf;
}

/**
 * 涓烘暟鎹寘鏂板涓€涓ご
 */
net_err_t pktbuf_add_header(pktbuf_t* buf, int size, int cont) {
    dbg_assert(buf->ref != 0, "buf freed")
    pktblk_t * block = pktbuf_first_blk(buf);

    // 濡傛灉鍧楀墠闈㈡湁瓒冲鐨勭┖闂诧紝鐩存帴鍒嗛厤鎺夊氨鍙互浜?
    int resv_size = (int)(block->data - block->payload);
    if (size <= resv_size) {
        block->size += size;
        block->data -= size;
        buf->total_size += size;

        display_check_buf(buf);
        return NET_ERR_OK;
    }

    // 娌℃湁瓒冲鐨勭┖闂达紝闇€瑕侀澶栧垎閰嶅潡娣诲姞鍒板ご閮? 绗竴涓潡鍗充娇鏈夌┖闂翠篃鐢ㄤ笉浜?
    if (cont) {
        // 瑕佹眰杩炵画鏃讹紝浣嗗ぇ灏忚秴杩?涓潡锛屽惁鍒欒偗瀹氭槸鍒嗛厤涓嶄簡鐨?
        if (size > PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF,"is_contious && size too big %d > %d", size, PKTBUF_BLK_SIZE);
            return NET_ERR_NONE;
        }

        // 涓嶈秴杩囦竴涓潡锛屽垯闇€瑕佸湪鏂板垎閰嶇殑鍧椾腑璁剧疆杩炵画绌洪棿锛屽師淇濈暀鍖虹┖闂翠笉褰卞搷
        block = pktblock_alloc_list(size, 1);
        if (!block) {
            dbg_error(DBG_BUF,"no buffer for alloc(size:%d)", size);
            return NET_ERR_NONE;
        }
    } else {
        // 闈炶繛缁垎閰嶏紝鍙兘鍦ㄥ綋鍓峛uf鍓嶉潰杩樻湁涓€鐐圭┖闂达紝鍙互鍏堝埄鐢ㄨ捣鏉?
        block->data = block->payload;
        block->size += resv_size;
        buf->total_size += resv_size;
        size -= resv_size;

        // 鍐嶅垎閰嶅叾瀹冩湭鐢ㄧ殑绌洪棿
        block = pktblock_alloc_list(size, 1);
        if (!block) {
            dbg_error(DBG_BUF,"no buffer for alloc(size:%d)", size);
            return NET_ERR_NONE;
        }
    }

    // 鍔犲叆鍧楀垪琛ㄧ殑澶撮儴
    pktbuf_insert_blk_list(buf, block, 0);
    display_check_buf(buf);
    return NET_ERR_OK;
}

/**
 * 涓烘暟鎹寘绉诲幓涓€涓寘澶?
 */
net_err_t pktbuf_remove_header(pktbuf_t* buf, int size) {
    dbg_assert(buf->ref != 0, "buf freed")
    pktblk_t * block = pktbuf_first_blk(buf);
    while (size) {
        pktblk_t* next_blk = pktbuf_blk_next(block);

        // 褰撳墠鍖呰冻澶熷噺鍘诲ご锛屽湪褰撳墠鍖呬笂鎿嶄綔鍗冲彲瀹屾垚瑕佹眰鐨勫伐浣?
        if (size < block->size) {
            block->data += size;
            block->size -= size;
            buf->total_size -= size;
            break;
        }

        // 涓嶅锛屽垯鍑忓幓褰撳墠鏁翠釜鍖呭ご
        int curr_size = block->size;

        // 涓嶅锛岀Щ闄ゅ綋鍓嶇殑鍧?
        nlist_remove_first(&buf->blk_list);
        pktblock_free(block);

        size -= curr_size;
        buf->total_size -= curr_size;
        block = next_blk;
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

/**
 * 璋冩暣鏁版嵁鍖呯殑澶у皬
 */
net_err_t pktbuf_resize(pktbuf_t* buf, int to_size) {
    dbg_assert(buf->ref != 0, "buf freed")
    // 澶у皬鐩稿悓锛屾棤闇€璋冩暣
    if (to_size == buf->total_size) {
        return NET_ERR_OK;
    }

    if (buf->total_size == 0) {
        // 鍘熸潵绌洪棿涓?锛岀洿鎺ュ垎閰嶅氨琛?
        pktblk_t* blk = pktblock_alloc_list(to_size, 0);
        if (!blk) {
            dbg_error(DBG_BUF, "not enough blk.");
            return NET_ERR_MEM;
        }

        pktbuf_insert_blk_list(buf, blk, 1);
    } else if (to_size == 0) {
        pktblock_free_list(pktbuf_first_blk(buf));
        buf->total_size = 0;
        nlist_init(&buf->blk_list);
    }  else if (to_size > buf->total_size) {
        // 鎵╁厖灏鹃儴锛屾暣浣撳彉闀? 闇€瑕佺煡閬撹嚜宸辨渶鍚庡墿浣欏灏戯紝濡傛灉涓嶅锛岃繕瑕侀澶栧垎閰?
        // 涓嶈€冭檻鏈€鍚庝竴涓潡澶撮儴鏈夌┖闂寸殑鎯呭喌锛岀畝鍖栧鐞?
        pktblk_t * tail_blk = pktbuf_last_blk(buf);

        // 瑕佸畾浣嶅埌鏈€鍚庯紝鐪嬬┖闂存槸鍚﹁冻澶?
        int inc_size = to_size - buf->total_size;       // 澶у皬鐨勫樊鍊?
        int remain_size = curr_blk_tail_free(tail_blk);
        if (remain_size >= inc_size) {
            // 瓒冲锛屽垯浣跨敤璇ョ┖闂寸洿鎺ュ垎閰嶏紝涓嶉渶瑕侀澶栧垎閰嶆柊鐨刡lock
            tail_blk->size += inc_size;
            buf->total_size += inc_size;
        } else {
            // 涓烘墿鍏呯殑绌洪棿鍒嗛厤buf閾撅紝涓嶇畻鍓╀綑绌洪棿
            pktblk_t * new_blks = pktblock_alloc_list(inc_size - remain_size, 0);
            if (!new_blks) {
                dbg_error(DBG_BUF, "not enough blk.");
                return NET_ERR_NONE;
            }

            // 杩炴帴涓や釜buf閾?
            tail_blk->size += remain_size;      // 鍓╀綑绌洪棿鍏ㄩ儴鍒╃敤璧锋潵
            buf->total_size += remain_size;
            pktbuf_insert_blk_list(buf, new_blks, 1);
        }
    } else {
        // 缂╁噺灏鹃儴锛屾暣浣撳彉鐭?
        int total_size = 0;

        // 閬嶅巻鍒拌揪闇€瑕佷繚鐣欑殑鏈€鍚庝竴涓紦瀛樺潡
        pktblk_t* tail_blk;
        for (tail_blk = pktbuf_first_blk(buf); tail_blk; tail_blk = pktbuf_blk_next(tail_blk)) {
            total_size += tail_blk->size;
            if (total_size >= to_size) {
                break;
            }
        }

        if (tail_blk == (pktblk_t*)0) {
            return NET_ERR_SIZE;
        }

        // 鍑忔帀鍚庣画鎵€鏈夊潡閾句腑鐨勫閲?
        pktblk_t * curr_blk = pktbuf_blk_next(tail_blk);
        total_size = 0;
        while (curr_blk) {
            // 鍏堝彇鍚庣画鐨勭粨鐐?
            pktblk_t * next_blk = pktbuf_blk_next(curr_blk);

            // 鍒犻櫎褰撳墠block
            nlist_remove(&buf->blk_list, &curr_blk->node);
            pktblock_free(curr_blk);

            total_size += curr_blk->size;
            curr_blk = next_blk;
        }

        // 璋冩暣tail_blk鐨勫ぇ灏?
        tail_blk->size -= buf->total_size - total_size - to_size;
        buf->total_size = to_size;
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

/**
 * 灏唖rc鎷煎埌dest鐨勫熬閮紝缁勬垚涓€涓猙uf閾?
 * 涓嶈€冭檻涓ゅ鍖呰繛鎺ュ鐨勫寘鍚堝苟閲婃斁闂锛屼互绠€鍖栧鐞?
 */
net_err_t pktbuf_join(pktbuf_t* dest, pktbuf_t* src) {
    dbg_assert(dest->ref != 0, "buf freed");
    dbg_assert(src->ref != 0, "buf freed");
    // 閫愪釜浠巗rc鐨勫潡鍒楄〃涓彇鍑猴紝鐒跺悗鍔犲叆鍒癲est涓?
    pktblk_t* first;

    while ((first = pktbuf_first_blk(src))) {
        // 浠巗rc涓Щ闄ら鍧?
        nlist_remove_first(&src->blk_list);

        // 鎻掑叆鍒板潡閾捐〃涓?
        pktbuf_insert_blk_list(dest, first, 1);
    }

    pktbuf_free(src);

    dbg_info(DBG_BUF,"join result:");
    display_check_buf(dest);
    return NET_ERR_OK;
}

/**
 * 灏嗗寘鐨勬渶寮€濮媠ize涓瓧鑺傦紝閰嶇疆鎴愯繛缁┖闂?
 * 甯哥敤浜庡鍖呭ご杩涜瑙ｆ瀽鏃讹紝鎴栬€呮湁鍏跺畠閫夐」瀛楄妭鏃?
 */
net_err_t pktbuf_set_cont(pktbuf_t* buf, int size) {
    dbg_assert(buf->ref != 0, "buf freed")
    // 蹇呴』瑕佹湁瓒冲鐨勯暱搴?
    if (size > buf->total_size) {
        dbg_error(DBG_BUF,"size(%d) > total_size(%d)", size, buf->total_size);
        return NET_ERR_SIZE;
    }

    // 瓒呰繃1涓狿OOL鐨勫ぇ灏忥紝杩斿洖閿欒
    if (size > PKTBUF_BLK_SIZE) {
        dbg_error(DBG_BUF,"size too big > %d", PKTBUF_BLK_SIZE);
        return NET_ERR_SIZE;
    }

    // 宸茬粡澶勪簬杩炵画绌洪棿锛屼笉鐢ㄥ鐞?
    pktblk_t * first_blk = pktbuf_first_blk(buf);
    if (size <= first_blk->size) {
        display_check_buf(buf);
        return NET_ERR_OK;
    }

    // 鍏堝皢绗竴涓猙lk鐨勬暟鎹線鍓嶆尓锛屼互鍦ㄥ熬閮ㄨ吘鍑簊ize绌洪棿
#if 0
    uint8_t * dest = first_blk->payload + PKTBUF_BLK_SIZE - size;
    plat_memmove(dest, first_blk->data, first_blk->size);   // 娉ㄦ剰澶勭悊鍐呭瓨閲嶅彔
    first_blk->data = dest;
    dest += first_blk->size;          // 鎸囧悜涓嬩竴鍧楀鍒剁殑鐩殑鍦?
#else
    uint8_t * dest = first_blk->payload;
    for (int i = 0; i < first_blk->size; i++) {
        *dest++ = first_blk->data[i];
    }
    first_blk->data = first_blk->payload;
#endif

    // 鍐嶄緷娆″皢鍚庣画鐨勭┖闂达紝鎼埌buf涓紝鐩村埌buf涓殑澶у皬杈惧埌size
    int remain_size = size - first_blk->size;
    pktblk_t * curr_blk = pktbuf_blk_next(first_blk);
    while (remain_size && curr_blk) {
        // 璁＄畻鏈绉诲姩鐨勬暟鎹噺
        int curr_size = (curr_blk->size > remain_size) ? remain_size : curr_blk->size;

        // 灏哻urr涓殑鏁版嵁锛岀Щ鍔ㄥ埌buf涓?
        plat_memcpy(dest, curr_blk->data, curr_size);
        dest += curr_size;
        curr_blk->data += curr_size;
        curr_blk->size -= curr_size;
        first_blk->size += curr_size;
        remain_size -= curr_size;

        // 澶嶅埗瀹屽悗锛宑urr_blk鍙兘鏃犳暟鎹紝閲婃斁鎺夛紝浠庡叾鍚庝竴涓寘缁х画澶嶅埗
        if (curr_blk->size == 0) {
            pktblk_t* next_blk = pktbuf_blk_next(curr_blk);

            nlist_remove(&buf->blk_list, &curr_blk->node);
            pktblock_free(curr_blk);

            curr_blk = next_blk;
        }
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

/**
 * 鏁版嵁鍖呯鐞嗗垵濮嬪寲
 */
net_err_t pktbuf_init(void) {
    dbg_info(DBG_BUF,"init pktbuf list.");

    // 寤虹珛绌洪棽閾捐〃. TODO锛氬湪宓屽叆寮忚澶囦腑鏀规垚涓嶅彲鍏变韩
    nlocker_init(&locker, NLOCKER_THREAD);
    mblock_init(&block_list, block_buffer, sizeof(pktblk_t), PKTBUF_BLK_CNT, NLOCKER_NONE);
    mblock_init(&pktbuf_list, pktbuf_buffer, sizeof(pktbuf_t), PKTBUF_BUF_CNT, NLOCKER_NONE);
    dbg_info(DBG_BUF,"init done.");

    return NET_ERR_OK;
}

/**
 * 鍓嶇Щ浣嶇疆. 濡傛灉璺ㄨ秺涓€涓寘锛屽垯浠呯Щ鍔ㄥ埌涓嬩竴涓寘鐨勫紑澶?
 */
static void move_forward(pktbuf_t* buf, int size) {
    pktblk_t* curr = buf->curr_blk;

    // 璋冩暣璇诲啓浣嶇疆
    buf->pos += size;
    buf->blk_offset += size;

    // 鍙兘瓒呭嚭浜嗗綋鍓嶅潡锛屾墍浠ヨ绉诲姩鍒颁笅涓€涓潡
    if (buf->blk_offset >= curr->data + curr->size) {
        buf->curr_blk = pktbuf_blk_next(curr);
        if (buf->curr_blk) {
            buf->blk_offset = buf->curr_blk->data;
        } else {
            // 涓嬩竴涓潡鍙兘涓虹┖
            buf->blk_offset = (uint8_t*)0;
        }
    }
}

/**
 * 灏唖rc涓殑鏁版嵁锛屽啓鍏ュ埌buf涓?
 */
int pktbuf_write(pktbuf_t* buf, uint8_t* src, int size) {
    dbg_assert(buf->ref != 0, "buf freed")
    // 蹇呰鐨勫弬鏁版鏌?
    if (!src || !size) {
        return NET_ERR_PARAM;
    }

    // 璁＄畻瀹為檯搴斿綋鍐欏叆鐨勬暟鎹噺
    int remain_size = total_blk_remain(buf);
    if (remain_size < size) {
        dbg_error(DBG_BUF, "size errorL %d < %d", remain_size, size);
        return NET_ERR_SIZE;
    }

    // 寰幆鍐欏叆鎵€鏈夋暟鎹?
    while (size > 0) {
        int blk_size = curr_blk_remain(buf);

        // 鍒ゆ柇褰撳墠璇诲彇鐨勯噺锛屽苟杩涜澶嶅埗
        int curr_copy = size > blk_size ? blk_size : size;
        plat_memcpy(buf->blk_offset, src, curr_copy);

        // 绉诲姩鎸囬拡
        move_forward(buf, curr_copy);
        src += curr_copy;
        size -= curr_copy;
    }

    return NET_ERR_OK;
}

/**
 * 浠庡綋鍓嶄綅缃鍙栨寚瀹氱殑鏁版嵁閲?
 *
 * @param rw buf璇诲啓鍣?
 * @param read_to 鏁版嵁瀛樺偍鐨勮捣濮嬩綅缃?
 * @param size 鏈€澶氳鍙栫殑瀛楄妭閲?
 */
int pktbuf_read(pktbuf_t* buf, uint8_t* dest, int size) {
    dbg_assert(buf->ref != 0, "buf freed")

    // 浼犲叆涓?锛屾垨鑰呯┖闂翠负0锛岃鍙?涓?
    if (!dest || !size) {
        return NET_ERR_OK;
    }

    // 璁＄畻鍓╀綑鐨勫瓧鑺傞噺锛岃揪涓嶅埌瑕佹眰鐨勯噺鍒欓€€鍑?
    int remain_size = total_blk_remain(buf);
    if (remain_size < size) {
        dbg_error(DBG_BUF, "size errorL %d < %d", remain_size, size);
        return NET_ERR_SIZE;
    }

    // 寰幆璇诲彇鎵€鏈夐噺
    while (size > 0) {
        int blk_size = curr_blk_remain(buf);
        int curr_copy = size > blk_size ? blk_size : size;
        plat_memcpy(dest, buf->blk_offset, curr_copy);

        // 瓒呭嚭褰撳墠buf锛岀Щ鑷充笅涓€涓猙uf
        move_forward(buf, curr_copy);

        dest += curr_copy;
        size -= curr_copy;
    }

    return NET_ERR_OK;
}

/**
 * 绉诲姩褰撳墠璇诲啓浣嶇疆鍒皁ffset鍋忕Щ澶?
 */
net_err_t pktbuf_seek(pktbuf_t* buf, int offset) {
    dbg_assert(buf->ref != 0, "buf freed")

    // 浣嶇疆鐩稿悓锛岀洿鎺ヨ烦杩?
    if (buf->pos == offset) {
        return NET_ERR_OK;
    }

    // 瓒呭嚭浜嗙紦瀛樼殑澶у皬锛岀Щ鍔ㄥけ璐?
    if ((offset < 0) || (offset >= buf->total_size)) {
        return NET_ERR_SIZE;
    }

    int move_bytes;
    if (offset < buf->pos) {
        // 浠庡悗寰€鍓嶇Щ锛屾瘮杈冮夯鐑︼紝鎵€浠ュ畾浣嶅埌buf鐨勬渶寮€澶村啀寮€濮嬬Щ鍔?
        buf->curr_blk = pktbuf_first_blk(buf);
        buf->blk_offset = buf->curr_blk->data;
        buf->pos = 0;

        move_bytes = offset;
    } else {
        // 寰€鍓嶇Щ鍔紝璁＄畻绉诲姩鐨勫瓧鑺傞噺
        move_bytes = offset - buf->pos;
    }

    // 涓嶆柇绉诲姩浣嶇疆,鍦ㄧЩ鍔ㄨ繃绋嬩腑锛屼富瑕佽皟鏁磘otal_offset, buf_offset鍜宑urr
    // offset鐨勫€煎彲鑳藉ぇ浜庝綑涓嬬殑绌洪棿锛屾鏃跺彧绉诲姩閮ㄥ垎锛屼絾鏄粛鐒舵纭?
    while (move_bytes) {
        int remain_size = curr_blk_remain(buf);
        int curr_move = move_bytes > remain_size ? remain_size : move_bytes;

        // 寰€鍓嶇Щ鍔紝鍙兘浼氳秴鍑哄綋鍓嶇殑buf
        move_forward(buf, curr_move);
        move_bytes -= curr_move;
    }

    return NET_ERR_OK;
}

/**
 * 鍦╞uf涔嬮棿澶嶅埗鏁版嵁锛屽鏋滅┖闂村锛屽け璐?
 * @param dest_rw 鐩爣buf璇诲啓鍣?
 * @param src_rw 婧恇uf璇诲啓鍣?
 */
net_err_t pktbuf_copy(pktbuf_t* dest, pktbuf_t* src, int size) {
    dbg_assert(src->ref != 0, "buf freed")
    dbg_assert(dest->ref != 0, "buf freed")

    // 妫€鏌ユ槸鍚﹁兘杩涜鎷疯礉
    if ((total_blk_remain(dest) < size) || (total_blk_remain(src) < size)) {
        return NET_ERR_SIZE;
    }

    // 杩涜瀹為檯鐨勬嫹璐濆伐浣?
    while (size) {
        // 鍦╯ize銆佷袱涓猙uf涓綋鍓嶅潡鍓╀綑澶у皬涓彇鏈€灏忕殑鍊?
        int dest_remain = curr_blk_remain(dest);
        int src_remain = curr_blk_remain(src);
        int copy_size = dest_remain > src_remain ? src_remain : dest_remain;
        copy_size = copy_size > size ? size : copy_size;

        // 澶嶅埗鏁版嵁
        plat_memcpy(dest->blk_offset, src->blk_offset, copy_size);

        move_forward(dest, copy_size);
        move_forward(src, copy_size);
        size -= copy_size;
    }

    return NET_ERR_OK;
}

/**
 * 鍚慴uf涓綋鍓嶄綅缃繛缁～鍏卻ize涓瓧鑺傚€紇
 * 涓婅堪鎿嶄綔锛屽悓pktbuf_write
 */
net_err_t pktbuf_fill(pktbuf_t* buf, uint8_t v, int size) {
    dbg_assert(buf->ref != 0, "buf freed")

    if (!size) {
        return NET_ERR_PARAM;
    }

    // 鐪嬬湅鏄惁澶熷～鍏?
    int remain_size = total_blk_remain(buf);
    if (remain_size < size) {
        dbg_error(DBG_BUF, "size errorL %d < %d", remain_size, size);
        return NET_ERR_SIZE;
    }

    // 寰幆鍐欏叆鎵€鏈夋暟鎹?
    while (size > 0) {
        int blk_size = curr_blk_remain(buf);

        // 鍒ゆ柇褰撳墠鍐欏叆鐨勯噺
        int curr_fill = size > blk_size ? blk_size : size;
        plat_memset(buf->blk_offset, v, curr_fill);

        // 绉诲姩鎸囬拡
        move_forward(buf, curr_fill);
        size -= curr_fill;
    }

    return NET_ERR_OK;
}