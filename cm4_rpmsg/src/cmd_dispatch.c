#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include "cmd_dispatch.h"
#include "ipc_handler.h"

#define CM4_FW_VERSION  "0.1.0"
#define CM4_BOARD_NAME  "MIMXRT1170-EVK Rev B"

static void cmd_ping(const char *args)
{
    ipc_send_resp("pong");
}

static void cmd_status(const char *args)
{
    char buf[IPC_MAX_PAYLOAD];
    snprintf(buf, sizeof(buf), "CM4 uptime: %u ms", k_uptime_get_32());
    ipc_send_resp(buf);
}

static void cmd_ver(const char *args)
{
    char buf[IPC_MAX_PAYLOAD];
    snprintf(buf, sizeof(buf),
             "Board:   %s\nFW Ver:  %s\nBuilt:   %s %s\nCM4 uptime: %u ms",
             CM4_BOARD_NAME, CM4_FW_VERSION, __DATE__, __TIME__,
             k_uptime_get_32());
    ipc_send_resp(buf);
}

static void cmd_log(const char *args)
{
    if (args && *args) {
        ipc_send_log(args);
        ipc_send_resp("log sent");
    } else {
        ipc_send_resp("Usage: log <message>");
    }
}

static void cmd_echo(const char *args)
{
    char buf[IPC_MAX_PAYLOAD];
    snprintf(buf, sizeof(buf), "echo: %s", args ? args : "");
    ipc_send_resp(buf);
}

static void cmd_shendpts(const char *args)
{
    char buf[IPC_MAX_PAYLOAD];
    snprintf(buf, sizeof(buf),
             "CM4 IPC ep:  cm4_ep\n"
             "OCRAM2_IPC0: 0x202C0000 (32KB) role=remote\n"
             "OCRAM2_IPC1: 0x202C8000 (32KB) role=remote");
    ipc_send_resp(buf);
}

typedef struct {
    const char *name;
    void (*handler)(const char *args);
} cmd_entry_t;

static const cmd_entry_t cmd_table[] = {
    { "ping",     cmd_ping     },
    { "status",   cmd_status   },
    { "ver",      cmd_ver      },
    { "log",      cmd_log      },
    { "echo",     cmd_echo     },
    { "shendpts", cmd_shendpts },
    /* Add new commands here */
};

void cmd_dispatch(const char *cmd)
{
    char name[32] = {0};
    const char *args = "";

    const char *space = strchr(cmd, ' ');
    if (space) {
        size_t nlen = (size_t)(space - cmd);
        if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
        memcpy(name, cmd, nlen);
        args = space + 1;
    } else {
        strncpy(name, cmd, sizeof(name) - 1);
    }

    for (size_t i = 0; i < ARRAY_SIZE(cmd_table); i++) {
        if (strcmp(name, cmd_table[i].name) == 0) {
            cmd_table[i].handler(args);
            return;
        }
    }

    char resp[IPC_MAX_PAYLOAD];
    snprintf(resp, sizeof(resp), "Unknown command: %s", name);
    ipc_send_resp(resp);
}