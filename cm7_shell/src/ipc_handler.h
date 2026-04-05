#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/shell/shell.h>

/* Message types */
#define IPC_MSG_TYPE_LOG     0x01   /* CM4 -> CM7 */
#define IPC_MSG_TYPE_CMD     0x02   /* CM7 -> CM4 */
#define IPC_MSG_TYPE_RESP    0x03   /* CM4 -> CM7 */

#define IPC_MAX_PAYLOAD      220

typedef struct {
    uint8_t  msg_type;
    uint8_t  reserved[3];
    uint32_t timestamp_ms;
    char     payload[IPC_MAX_PAYLOAD];
} ipc_message_t;

/* CM7-side API */
int  ipc_handler_init(void);
int  ipc_send_cmd(const char *cmd, size_t len);
void ipc_handler_set_shell(const struct shell *sh);

#endif /* IPC_HANDLER_H */