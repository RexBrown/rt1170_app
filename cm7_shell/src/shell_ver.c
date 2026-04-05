#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include "ipc_handler.h"

#define FW_VERSION      "0.1.0"
#define BOARD_NAME      "MIMXRT1170-EVK Rev B"

/* ver - show CM7 firmware info */
static int cmd_ver(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Board:   %s", BOARD_NAME);
    shell_print(sh, "FW Ver:  %s", FW_VERSION);
    shell_print(sh, "Built:   %s %s", __DATE__, __TIME__);
    shell_print(sh, "CM7 uptime: %u ms", k_uptime_get_32());
    return 0;
}

/* shendpts - show IPC endpoint info */
static int cmd_shendpts(const struct shell *sh, size_t argc, char **argv)
{
    const struct device *ipc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
    shell_print(sh, "CM7 IPC device: %s", ipc_dev->name);
    shell_print(sh, "IPC shared mem: OCRAM2_IPC0 @ 0x202C0000 (32KB)");
    shell_print(sh, "IPC shared mem: OCRAM2_IPC1 @ 0x202C8000 (32KB)");
    shell_print(sh, "Use 'cm4 shendpts' to show CM4-side endpoint info");
    return 0;
}

/* ipcecho - send a message to CM4 and get it echoed back */
static int cmd_ipcecho(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(sh, "Usage: ipcecho <message>");
        return 0;
    }

    /* Build message string */
    char buf[IPC_MAX_PAYLOAD];
    size_t offset = 0;

    /* Prefix with "echo " so CM4 cmd_dispatch routes it */
    offset += snprintf(buf, sizeof(buf), "echo ");

    for (size_t i = 1; i < argc && offset < sizeof(buf) - 1; i++) {
        if (i > 1) {
            buf[offset++] = ' ';
        }
        size_t arglen = strlen(argv[i]);
        if (offset + arglen >= sizeof(buf) - 1) {
            arglen = sizeof(buf) - 1 - offset;
        }
        memcpy(buf + offset, argv[i], arglen);
        offset += arglen;
    }
    buf[offset] = '\0';

    ipc_handler_set_shell(sh);
    return ipc_send_cmd(buf, offset);
}

SHELL_CMD_ARG_REGISTER(ver,      NULL, "Display firmware version info",           cmd_ver,      1, 0);
SHELL_CMD_ARG_REGISTER(shendpts, NULL, "Show IPC endpoint addresses",             cmd_shendpts, 1, 0);
SHELL_CMD_ARG_REGISTER(ipcecho,  NULL, "Echo a message via CM4 IPC\n"
                                       "Usage: ipcecho <message>",                cmd_ipcecho,  2, 8);