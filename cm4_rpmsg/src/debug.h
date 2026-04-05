#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/* Module indices - add new modules here */
#define DBG_MOD_MAIN     0
#define DBG_MOD_IPC      1
#define DBG_MOD_APP      2
/* Add more as needed, max 32 */
#define DBG_MOD_COUNT    3

typedef struct {
    uint32_t    dbgBit;
    const char *name;
} debug_bits_t;

extern uint32_t        DebugMask;
extern uint32_t        DebugLevel;
extern debug_bits_t    DebugBits[DBG_MOD_COUNT];

/* Logging macros - output goes via IPC to CM7 once IPC is ready,
 * falls back to printk before IPC is up */
#define DBG_MSK_LVL_BEGIN(m, l) \
    if ((DebugLevel >= (uint32_t)(l)) && (DebugMask & DebugBits[m].dbgBit)) {
#define DBG_END }

void ipc_log_send(const char *fmt, ...);

/* Convenience wrapper keeping the [CM4][ts] prefix */
#define CM4_LOG(m, l, fmt, ...) \
    DBG_MSK_LVL_BEGIN(m, l) \
        ipc_log_send("[CM4][%u] " fmt, k_uptime_get_32(), ##__VA_ARGS__); \
    DBG_END

#endif /* DEBUG_H */
