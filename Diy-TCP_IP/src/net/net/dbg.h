#ifndef DBG_H
#define DBG_H

#include "net_cfg.h"
#include "net_plat.h"

// 璋冭瘯淇℃伅鐨勬樉绀烘牱寮忚缃?
#define DBG_STYLE_RESET       "\033[0m"       // 澶嶄綅鏄剧ず
#define DBG_STYLE_ERROR       "\033[31m"      // 绾㈣壊鏄剧ず
#define DBG_STYLE_WARNING     "\033[33m"      // 榛勮壊鏄剧ず

// 寮€鍚殑淇℃伅杈撳嚭閰嶇疆锛屽€艰秺澶э紝杈撳嚭鐨勮皟璇曚俊鎭秺澶?
#define DBG_LEVEL_NONE           0         // 涓嶅紑鍚换浣曡緭鍑?
#define DBG_LEVEL_ERROR          1         // 鍙紑鍚敊璇俊鎭緭鍑?
#define DBG_LEVEL_WARNING        2         // 寮€鍚敊璇拰璀﹀憡淇℃伅杈撳嚭
#define DBG_LEVEL_INFO           3         // 寮€鍚敊璇€佽鍛娿€佷竴鑸俊鎭緭鍑?

/**
 * @brief 鎵撳嵃璋冭瘯淇℃伅
 */
void dbg_print(int m_level, int s_level, const char* file, const char* func, int line, const char* fmt, ...);

void dump_mac(const char* msg, const uint8_t* mac);
void dump_ip_buf(const char* msg, const uint8_t* ip);

/**
 * @brief 涓嶅悓鐨勮皟璇曡緭鍑哄畯
 * __FILE__ __FUNCTION__, __LINE__涓篊璇█鍐呯疆鐨勫畯
*/
#define dbg_info(module, fmt, ...)  dbg_print(module, DBG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define dbg_error(module, fmt, ...)  dbg_print(module, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define dbg_warning(module, fmt, ...) dbg_print(module, DBG_LEVEL_WARNING, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief 鏂█鍒ゆ柇
 * 涓嬮潰鐢ㄥぇ鎷彿鍖呭惈锛岃繖鏍烽伩鍏峣f鍙兘涓庡悗闈㈠彲鑳界殑else鐩歌竟锛屼緥濡傦細
 * if (xxx)
 *    dbg_assert   澶氫釜浜唅f (xxx) 
 * else
 *    xxxx
 * 涓嶈繃涓€鑸垜浼氱敤澶ф嫭鍙峰寘鍚玦f涓嬮潰鐨勮鍙ワ紝鎵€浠ュ嚭閿欏彲鑳芥€т笉澶?
 */
#define dbg_assert(expr, msg)   {\
    if (!(expr)) {\
        dbg_print(DBG_LEVEL_ERROR, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "assert failed:"#expr","msg); \
        while(1);   \
    }   \
}

#define dbg_dump_ip_buf(module, msg, ip)   {if (module >= DBG_LEVEL_INFO) dump_ip_buf(msg, ip); }
#define dbg_dump_ip(module, msg, ip)   {if (module >= DBG_LEVEL_INFO) dump_ip_buf(msg, (ip)->a_addr); }

#define DBG_DISP_ENABLED(module)  (module >= DBG_LEVEL_INFO)

#endif