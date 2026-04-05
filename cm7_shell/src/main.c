#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ipc_handler.h"

LOG_MODULE_REGISTER(cm7_main, LOG_LEVEL_DBG);

static K_THREAD_STACK_DEFINE(ipc_stack, 2048);
static struct k_thread ipc_thread;

static void ipc_thread_entry(void *a, void *b, void *c)
{
    ipc_handler_init();
}

int main(void)
{
    LOG_INF("CM7 shell started on LPUART1");

    k_thread_create(&ipc_thread, ipc_stack,
                    K_THREAD_STACK_SIZEOF(ipc_stack),
                    ipc_thread_entry,
                    NULL, NULL, NULL,
                    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

    return 0;
}