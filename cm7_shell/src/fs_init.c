/*
 * fs_init.c - SD card auto-mount at boot
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <ff.h>

LOG_MODULE_REGISTER(fs_init, LOG_LEVEL_INF);

static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

/* Mount point */
static const char *sd_mount_point = "/SD:";

int fs_init(void)
{
    static const char *disk_name = "SD";
    int ret;

    LOG_INF("Initializing SD card file system...");

    /* Wait for SD card to be ready */
    ret = disk_access_init(disk_name);
    if (ret != 0) {
        LOG_ERR("SD card init failed: %d", ret);
        return ret;
    }

    mp.mnt_point = sd_mount_point;

    ret = fs_mount(&mp);
    if (ret != 0) {
        LOG_ERR("SD card mount failed: %d", ret);
        return ret;
    }

    LOG_INF("SD card mounted at %s", sd_mount_point);

    /* Create /SD:/recipes directory if it doesn't exist */
    ret = fs_mkdir("/SD:/recipes");
    if (ret == 0) {
        LOG_INF("Created /SD:/recipes directory");
    } else if (ret == -EEXIST) {
        LOG_INF("/SD:/recipes directory already exists");
    } else {
        LOG_WRN("Failed to create /SD:/recipes: %d", ret);
    }

    return 0;
}

/* Auto-initialize at boot */
SYS_INIT(fs_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);