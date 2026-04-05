#include <zephyr/shell/shell.h>
#include <string.h>
#include "ipc_handler.h"

static int cmd_cm4(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(sh, "Usage: cm4 <command> [args...]");
        return 0;
    }

    /* Reconstruct command string from argv[1..] */
    char cmd_buf[IPC_MAX_PAYLOAD];
    size_t offset = 0;

    for (size_t i = 1; i < argc && offset < sizeof(cmd_buf) - 1; i++) {
        if (i > 1) {
            cmd_buf[offset++] = ' ';
        }
        size_t arglen = strlen(argv[i]);
        if (offset + arglen >= sizeof(cmd_buf) - 1) {
            arglen = sizeof(cmd_buf) - 1 - offset;
        }
        memcpy(cmd_buf + offset, argv[i], arglen);
        offset += arglen;
    }
    cmd_buf[offset] = '\0';

    ipc_handler_set_shell(sh);
    return ipc_send_cmd(cmd_buf, offset);
}

SHELL_CMD_ARG_REGISTER(cm4, NULL,
    "Send command to CM4 core\nUsage: cm4 <command> [args...]",
    cmd_cm4, 2, 8);
    