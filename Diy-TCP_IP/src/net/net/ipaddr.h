#ifndef IPADDR_H
#define IPADDR_H

#include <stdint.h>
#include "net_err.h"

#define IPV4_ADDR_SIZE             4            // IPv4鍦板潃闀垮害

/**
 * @brief IP鍦板潃
 */
typedef struct _ipaddr_t {
    enum {
        IPADDR_V4,
    }type;              // 鍦板潃绫诲瀷

    union {
        // 娉ㄦ剰锛孖P鍦板潃鎬绘槸鎸夊ぇ绔瓨鏀?
        uint32_t q_addr;                        // 32浣嶆暣浣撴弿杩?
        uint8_t a_addr[IPV4_ADDR_SIZE];        // 鏁扮粍鎻忚堪
    };
}ipaddr_t;

void ipaddr_set_any(ipaddr_t * ip);
net_err_t ipaddr_from_str(ipaddr_t * dest, const char* str);
ipaddr_t * ipaddr_get_any(void);
void ipaddr_copy(ipaddr_t * dest, const ipaddr_t * src);

#endif