#include "ipaddr.h"

/**
 * 璁剧疆ip涓轰换鎰廼p鍦板潃
 * @param ip 寰呰缃殑ip鍦板潃
 */
void ipaddr_set_any(ipaddr_t * ip) {
    ip->q_addr = 0;
}

/**
 * @brief 灏唖tr瀛楃涓茶浆绉讳负ipaddr_t鏍煎紡
 */
net_err_t ipaddr_from_str(ipaddr_t * dest, const char* str) {
    // 蹇呰鐨勫弬鏁版鏌?
    if (!dest || !str) {
        return NET_ERR_PARAM;
    }

    // 鍒濆鍊兼竻绌?
    dest->q_addr = 0;

    // 涓嶆柇鎵弿鍚勫瓧鑺備覆锛岀洿鍒板瓧绗︿覆缁撴潫
    char c;
    uint8_t * p = dest->a_addr;
    uint8_t sub_addr = 0;
    while ((c = *str++) != '\0') {
        // 濡傛灉鏄暟瀛楀瓧绗︼紝杞崲鎴愭暟瀛楀苟鍚堝苟杩涘幓
        if ((c >= '0') && (c <= '9')) {
            // 鏁板瓧瀛楃杞崲涓烘暟瀛楋紝鍐嶅苟鍏ヨ绠?
            sub_addr = sub_addr * 10 + c - '0';
        } else if (c == '.') {
            // . 鍒嗛殧绗︼紝杩涘叆涓嬩竴涓湴鍧€绗︼紝骞堕噸鏂拌绠?
            *p++ = sub_addr;
            sub_addr = 0;
        } else {
            // 鏍煎紡閿欒锛屽寘鍚潪鏁板瓧鍜?瀛楃
            return NET_ERR_PARAM;
        }
    }

    // 鍐欏叆鏈€鍚庤绠楃殑鍊?
    *p++ = sub_addr;
    return NET_ERR_OK;
}

/**
 * 鑾峰彇缂虹渷鍦板潃
 */
ipaddr_t * ipaddr_get_any(void) {
    static ipaddr_t ipaddr_any = { .q_addr = 0 };
    return &ipaddr_any;
}

/**
 * @brief 澶嶅埗ip鍦板潃
 */
void ipaddr_copy(ipaddr_t * dest, const ipaddr_t * src) {
    if (!dest || !src) {
        return;
    }

    dest->q_addr = src->q_addr;
}