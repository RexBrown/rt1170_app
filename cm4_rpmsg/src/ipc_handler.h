#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include <stdint.h>
#include <stddef.h>

/* Must match cm7_shell/src/ipc_handler.h */
#define IPC_MSG_TYPE_LOG     0x01
#define IPC_MSG_TYPE_CMD     0x02
#define IPC_MSG_TYPE_RESP    0x03

#define IPC_MAX_PAYLOAD      220

typedef struct {
    uint8_t  msg_type;
    uint8_t  reserved[3];
    uint32_t timestamp_ms;
    char     payload[IPC_MAX_PAYLOAD];
} ipc_message_t;

/* CM4-side API */
int  ipc_handler_init(void);
int  ipc_send_log(const char *msg);
int  ipc_send_resp(const char *resp);

#endif /* IPC_HANDLER_H */
