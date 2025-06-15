#ifndef NET_CFG_H
#define NET_CFG_H

// 璋冭瘯淇℃伅杈撳嚭
#define DBG_MBLOCK		    DBG_LEVEL_INFO			// 鍐呭瓨鍧楃鐞嗗櫒
#define DBG_QUEUE           DBG_LEVEL_INFO          // 瀹氶暱瀛樺偍鍧?
#define DBG_MSG             DBG_LEVEL_INFO          // 娑堟伅閫氫俊
#define DBG_BUF             DBG_LEVEL_INFO          // 鏁版嵁鍖呯鐞嗗櫒
#define DBG_PLAT            DBG_LEVEL_INFO          // 绯荤粺骞冲彴
#define DBG_INIT            DBG_LEVEL_INFO          // 绯荤粺鍒濆鍖?
#define DBG_NETIF           DBG_LEVEL_INFO          // 缃戠粶鎺ュ彛灞?

#define EXMSG_MSG_CNT          10                 // 娑堟伅缂撳啿鍖哄ぇ灏?
#define EXMSG_BLOCKER        NLOCKER_THREAD      // 鏍稿績绾跨▼鐨勯攣绫诲瀷

#define PKTBUF_BLK_SIZE         128                 // 鏁版嵁鍖呭潡澶у皬
#define PKTBUF_BLK_CNT          100                 // 鏁版嵁鍖呭潡鐨勬暟閲?
#define PKTBUF_BUF_CNT          100                 // 鏁版嵁鍖呯殑鏁伴噺

#define NETIF_HWADDR_SIZE           10                  // 纭欢鍦板潃闀垮害锛宮ac鍦板潃鏈€灏?涓瓧鑺?
#define NETIF_NAME_SIZE             10                  // 缃戠粶鎺ュ彛鍚嶇О澶у皬
#define NETIF_DEV_CNT               4                   // 缃戠粶鎺ュ彛鐨勬暟閲?
#define NETIF_INQ_SIZE             50                  // 缃戝崱杈撳叆闃熷垪鏈€澶у閲?
#define NETIF_OUTQ_SIZE            50                  // 缃戝崱杈撳嚭闃熷垪鏈€澶у閲?

#endif