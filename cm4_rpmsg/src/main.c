#include <zephyr/kernel.h>
#include "ipc_handler.h"
#include "debug.h"

int main(void)
{
    int ret = ipc_handler_init();
    if (ret < 0) {
        printk("IPC init failed: %d\n", ret);
        return ret;
    }

    /* IPC is up - CM4 logs now route to CM7 console */
    CM4_LOG(DBG_MOD_MAIN, 1, "CM4 started, IPC ready");

    int count = 0;
    while (1) {
        CM4_LOG(DBG_MOD_MAIN, 2, "CM4 tick: %d", count++);
        k_sleep(K_SECONDS(5));
    }
    return 0;
}
