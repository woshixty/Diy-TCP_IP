#include <string.h>
#include <stdarg.h>
#include "dbg.h"
#include "net_plat.h"

/**
 * @brief 璋冭瘯淇℃伅杈撳嚭
 * 杈撳嚭鏂囦欢鍚嶃€佸嚱鏁板悕銆佸嚱鏁颁互鍙婅皟璇曚俊鎭?
 */
void dbg_print(int m_level, int s_level, const char* file, const char* func, int line, const char* fmt, ...) {
    static const char* title[] = {
        [DBG_LEVEL_ERROR] = DBG_STYLE_ERROR"error",
        [DBG_LEVEL_WARNING] = DBG_STYLE_WARNING"warning",
        [DBG_LEVEL_INFO] = "info",
        [DBG_LEVEL_NONE] = "none"
    };

    // 褰撲粎褰撳墠绛夌骇鏁板€兼瘮杈冨ぇ鏃舵墠杈撳嚭
    if (m_level >= s_level) {
        // 瀹氫綅鍒版枃浠跺悕閮ㄥ垎锛屽洜涓篺ile浼犺緭鐨勬槸瀹屾暣鐨勬枃浠惰矾寰勶紝澶暱浜?
        const char * end = file + plat_strlen(file);
        while (end >= file) {
            if ((*end == '\\') || (*end == '/')) {
                break;
            }
            end--;
        }
        end++;

        // 姣忚淇℃伅鎻愮ず鐨勫紑澶?
        plat_printf("%s(%s-%s-%d):", title[s_level], end, func, line);

        char str_buf[128];
        va_list args;

        // 鍏蜂綋鐨勪俊鎭?
        va_start(args, fmt);
        plat_vsprintf(str_buf, fmt, args);
        plat_printf("%s\n"DBG_STYLE_RESET, str_buf);
        va_end(args);
    }
}

/**
 * 鎵撳嵃mac鍦板潃
 */
void dump_mac(const char * msg, const uint8_t * mac) {
    if (msg) {
        plat_printf("%s", msg);
    }

    plat_printf("%02x-%02x-%02x-%02x-%02x-%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * 鎵撳嵃IP鍦板潃
 * @param ip ip鍦板潃
 */
void dump_ip_buf(const char* msg, const uint8_t* ip) {
    if (msg) {
        plat_printf("%s", msg);
    }

    if (ip) {
        plat_printf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    } else {
        plat_printf("0.0.0.0\n");
    }
}