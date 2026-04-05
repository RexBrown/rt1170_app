#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "debug.h"
#include "ipc_handler.h"

/* Default: all modules enabled, level 1 */
uint32_t     DebugMask  = 0xFFFFFFFF;
uint32_t     DebugLevel = 1;

debug_bits_t DebugBits[DBG_MOD_COUNT] = {
    [DBG_MOD_MAIN] = { .dbgBit = (1u << DBG_MOD_MAIN), .name = "MAIN" },
    [DBG_MOD_IPC]  = { .dbgBit = (1u << DBG_MOD_IPC),  .name = "IPC"  },
    [DBG_MOD_APP]  = { .dbgBit = (1u << DBG_MOD_APP),  .name = "APP"  },
};

void ipc_log_send(const char *fmt, ...)
{
    char buf[220];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Try IPC first; fall back to printk if IPC not ready */
    if (ipc_send_log(buf) < 0) {
        printk("%s\n", buf);
    }
}
