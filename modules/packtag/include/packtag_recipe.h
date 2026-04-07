/*
 * packtag_recipe.h
 *
 * Recipe save/load API -- INI file format on filesystem
 *
 * NOTE: File I/O is stubbed (returns RECIPE_ERR_NOT_AVAILABLE) pending
 * Phase 4 (FatFS on SD card). CRC16 and all helper logic is compiled.
 */

#ifndef PACKTAG_RECIPE_H_
#define PACKTAG_RECIPE_H_

#include <stdint.h>

typedef enum {
    RECIPE_OK                = 0,
    RECIPE_ERR_FILE_OPEN     = -1,
    RECIPE_ERR_WRITE         = -2,
    RECIPE_ERR_FORMAT        = -3,
    RECIPE_ERR_CRC_MISSING   = -4,
    RECIPE_ERR_CRC_FAIL      = -5,
    RECIPE_ERR_UNKNOWN_KEY   = -6,
    RECIPE_ERR_NOT_AVAILABLE = -7,
} PackRecipeResult_t;

PackRecipeResult_t PackTag_SaveRecipe(const char *path, const char *recipeName);
PackRecipeResult_t PackTag_LoadRecipe(const char *path, const char *recipeName);
uint16_t CRC16_buf(uint8_t *buf_start, uint8_t *buf_end, uint16_t offset);

#endif /* PACKTAG_RECIPE_H_ */