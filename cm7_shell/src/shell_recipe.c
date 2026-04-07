/*
 * shell_recipe.c - PackTag recipe shell commands
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "packtag_recipe.h"

LOG_MODULE_REGISTER(shell_recipe, LOG_LEVEL_INF);

static int cmd_recipe_save(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_error(sh, "Usage: recipe save <name>");
        return -EINVAL;
    }

    const char *name = argv[1];
    char path[64];
    snprintf(path, sizeof(path), "/SD:/recipes/%s.ini", name);

    shell_print(sh, "Saving recipe to %s...", path);

    PackRecipeResult_t result = PackTag_SaveRecipe(path, name);

    switch (result) {
        case RECIPE_OK:
            shell_print(sh, "Recipe saved successfully");
            return 0;
        case RECIPE_ERR_FILE_OPEN:
            shell_error(sh, "Failed to open file");
            return -EIO;
        case RECIPE_ERR_WRITE:
            shell_error(sh, "Failed to write file");
            return -EIO;
        case RECIPE_ERR_NOT_AVAILABLE:
            shell_error(sh, "File system not available");
            return -ENODEV;
        default:
            shell_error(sh, "Unknown error: %d", result);
            return -EIO;
    }
}

static int cmd_recipe_load(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_error(sh, "Usage: recipe load <name>");
        return -EINVAL;
    }

    const char *name = argv[1];
    char path[64];
    snprintf(path, sizeof(path), "/SD:/recipes/%s.ini", name);

    shell_print(sh, "Loading recipe from %s...", path);

    PackRecipeResult_t result = PackTag_LoadRecipe(path, name);

    switch (result) {
        case RECIPE_OK:
            shell_print(sh, "Recipe loaded successfully");
            return 0;
        case RECIPE_ERR_FILE_OPEN:
            shell_error(sh, "Failed to open file (does it exist?)");
            return -ENOENT;
        case RECIPE_ERR_FORMAT:
            shell_error(sh, "Invalid recipe format");
            return -EINVAL;
        case RECIPE_ERR_CRC_MISSING:
            shell_error(sh, "CRC field missing");
            return -EINVAL;
        case RECIPE_ERR_CRC_FAIL:
            shell_error(sh, "CRC verification failed");
            return -EILSEQ;
        case RECIPE_ERR_UNKNOWN_KEY:
            shell_error(sh, "Unknown parameter key in file");
            return -EINVAL;
        case RECIPE_ERR_NOT_AVAILABLE:
            shell_error(sh, "File system not available");
            return -ENODEV;
        default:
            shell_error(sh, "Unknown error: %d", result);
            return -EIO;
    }
}

static int cmd_recipe_list(const struct shell *sh, size_t argc, char **argv)
{
    struct fs_dir_t dir;
    struct fs_dirent entry;
    int ret;

    fs_dir_t_init(&dir);

    ret = fs_opendir(&dir, "/SD:/recipes");
    if (ret != 0) {
        shell_error(sh, "Failed to open /SD:/recipes: %d", ret);
        return ret;
    }

    shell_print(sh, "Available recipes:");
    shell_print(sh, "% -30s %10s", "Name", "Size");
    shell_print(sh, "----------------------------------------");

    while (1) {
        ret = fs_readdir(&dir, &entry);
        if (ret != 0 || entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_FILE) {
            /* Only show .ini files */
            size_t len = strlen(entry.name);
            if (len > 4 && strcmp(&entry.name[len - 4], ".ini") == 0) {
                shell_print(sh, "%-30s %10zu", entry.name, entry.size);
            }
        }
    }

    fs_closedir(&dir);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(recipe_cmds,
    SHELL_CMD(save, NULL, "Save current parameters to recipe", cmd_recipe_save),
    SHELL_CMD(load, NULL, "Load recipe parameters", cmd_recipe_load),
    SHELL_CMD(list, NULL, "List available recipes", cmd_recipe_list),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(recipe, &recipe_cmds, "PackTag recipe commands", NULL);