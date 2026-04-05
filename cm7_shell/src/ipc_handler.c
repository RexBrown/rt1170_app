#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include "ipc_handler.h"

LOG_MODULE_REGISTER(ipc_handler, LOG_LEVEL_DBG);

static struct ipc_ept ep;
static K_SEM_DEFINE(bound_sem, 0, 1);
static bool bound_once = false;

/* Shell pointer for printing CM4 responses */
static const struct shell *active_shell = NULL;
static K_SEM_DEFINE(resp_sem, 0, 1);

void ipc_handler_set_shell(const struct shell *sh)
{
    active_shell = sh;
}

static void ep_bound(void *priv)
{
    /* RPMsg backend fires bound twice (once per side) - only act on first */
    if (!bound_once) {
        bound_once = true;
        k_sem_give(&bound_sem);
    }
}

static void ep_recv(const void *data, size_t len, void *priv)
{
    if (len < sizeof(ipc_message_t)) {
        LOG_WRN("Short IPC message: %zu bytes", len);
        return;
    }

    const ipc_message_t *msg = (const ipc_message_t *)data;

    switch (msg->msg_type) {
    case IPC_MSG_TYPE_LOG:
        /* CM4 log message - print to CM7 console */
        LOG_INF("%s", msg->payload);
        break;

    case IPC_MSG_TYPE_RESP:
        /* Response to a shell command */
        if (active_shell) {
            shell_print(active_shell, "%s", msg->payload);
        } else {
            LOG_INF("CM4 resp: %s", msg->payload);
        }
        k_sem_give(&resp_sem);
        break;

    default:
        LOG_WRN("Unknown IPC msg type: 0x%02x", msg->msg_type);
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
        LOG_ERR("ipc_service_open_instance failed: %d", ret);
        return ret;
    }

    ret = ipc_service_register_endpoint(ipc_dev, &ep, &ep_cfg);
    if (ret < 0) {
        LOG_ERR("ipc_service_register_endpoint failed: %d", ret);
        return ret;
    }

    LOG_INF("Waiting for CM4...");
    k_sem_take(&bound_sem, K_FOREVER);
    LOG_INF("IPC ready - CM4 is up");

    return 0;
}

int ipc_send_cmd(const char *cmd, size_t len)
{
    ipc_message_t msg = {0};

    msg.msg_type     = IPC_MSG_TYPE_CMD;
    msg.timestamp_ms = k_uptime_get_32();

    if (len >= sizeof(msg.payload)) {
        len = sizeof(msg.payload) - 1;
    }
    memcpy(msg.payload, cmd, len);
    msg.payload[len] = '\0';

    /* Clear any stale response semaphore */
    k_sem_reset(&resp_sem);

    int ret = ipc_service_send(&ep, &msg, sizeof(msg));
    if (ret < 0) {
        LOG_ERR("ipc_service_send failed: %d", ret);
        return ret;
    }

    /* Wait up to 2s for CM4 response */
    ret = k_sem_take(&resp_sem, K_MSEC(2000));
    if (ret == -EAGAIN) {
        if (active_shell) {
            shell_print(active_shell, "CM4 command timed out");
        }
        LOG_WRN("CM4 command response timeout");
    }

    return ret;
}