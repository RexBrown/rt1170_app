#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <string.h>
#include "ipc_handler.h"
#include "cmd_dispatch.h"

static struct ipc_ept ep;
static K_SEM_DEFINE(bound_sem, 0, 1);
static bool bound_once = false;
static bool ipc_ready  = false;

static void ep_bound(void *priv)
{
    if (!bound_once) {
        bound_once = true;
        ipc_ready  = true;
        k_sem_give(&bound_sem);
    }
}

static void ep_recv(const void *data, size_t len, void *priv)
{
    if (len < sizeof(ipc_message_t)) {
        return;
    }

    const ipc_message_t *msg = (const ipc_message_t *)data;

    switch (msg->msg_type) {
    case IPC_MSG_TYPE_CMD:
        cmd_dispatch(msg->payload);
        break;
    default:
        break;
    }
}

static struct ipc_ept_cfg ep_cfg = {
    .name = "cm4_ep",
    .cb = {
        .bound    = ep_bound,
        .received = ep_recv,
    },
};

int ipc_handler_init(void)
{
    const struct device *ipc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
    int ret;

    ret = ipc_service_open_instance(ipc_dev);
    if (ret < 0 && ret != -EALREADY) {
        return ret;
    }

    ret = ipc_service_register_endpoint(ipc_dev, &ep, &ep_cfg);
    if (ret < 0) {
        return ret;
    }

    k_sem_take(&bound_sem, K_FOREVER);
    return 0;
}

int ipc_send_log(const char *msg)
{
    if (!ipc_ready) {
        return -EAGAIN;
    }

    ipc_message_t out = {0};
    out.msg_type     = IPC_MSG_TYPE_LOG;
    out.timestamp_ms = k_uptime_get_32();

    strncpy(out.payload, msg, sizeof(out.payload) - 1);

    return ipc_service_send(&ep, &out, sizeof(out));
}

int ipc_send_resp(const char *resp)
{
    if (!ipc_ready) {
        return -EAGAIN;
    }

    ipc_message_t out = {0};
    out.msg_type     = IPC_MSG_TYPE_RESP;
    out.timestamp_ms = k_uptime_get_32();

    strncpy(out.payload, resp, sizeof(out.payload) - 1);

    return ipc_service_send(&ep, &out, sizeof(out));
}
