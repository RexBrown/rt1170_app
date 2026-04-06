/*
 * packtag_recipe.c
 *
 * Recipe save/load -- INI-style ASCII file format
 *
 * Zephyr port status:
 *   FULLY IMPLEMENTED: CRC16, name tables, type tables, all helper functions
 *   STUBBED (Phase 4): PackTag_SaveRecipe / PackTag_LoadRecipe
 *     -> return RECIPE_ERR_NOT_AVAILABLE until filesystem is available
 *     -> TODO Phase 4: replace stubs with Zephyr fs.h implementation
 *
 * Type-aware formatting:
 *   All PackParameter_t.value fields are stored as float in the struct.
 *   s_paramTypes[] and s_procVarTypes[] classify each parameter so the
 *   recipe writer formats values correctly in the INI file:
 *     PARAM_TYPE_FLOAT -> "%g"   e.g. 100.5
 *     PARAM_TYPE_INT   -> "%d"   e.g. 4
 *     PARAM_TYPE_BOOL  -> "%d"   e.g. 0 or 1
 *   On load, strtof() handles all three correctly.
 *   IEEE 754 float exactly represents all integers used here (all < 2^24).
 *
 * INI file format summary:
 *   # comment
 *   [SECTION_NAME]
 *   KEY = VALUE
 *   END
 *   [CRC]
 *   XXXX        <- 16-bit hex CRC (seed 0xA596) of all content before [CRC]
 */

#include "packtag_recipe.h"
#include "packtag.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

LOG_MODULE_REGISTER(packtag_recipe, LOG_LEVEL_INF);

/* =========================================================================
 * CRC16 lookup table (feedback polynomial 0xA001, seed 0xA596)
 * ========================================================================= */
#define CRC16_SEED  0xA596U

static const uint16_t crc16revtab[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

uint16_t CRC16_buf(uint8_t *buf_start, uint8_t *buf_end, uint16_t offset)
{
    uint16_t accum;
    for (accum = offset; buf_start <= buf_end; buf_start++) {
        accum = (uint16_t)((accum >> 8) ^
                crc16revtab[(accum ^ (*buf_start)) & 0x00ff]);
    }
    return (uint16_t)(~accum);
}

/* =========================================================================
 * Parameter name tables
 * ========================================================================= */
static const char * const s_paramNames[NBR_PARAMETERS] = {
    "ENCODER_PRESENT",
    "ENCODER_RATIO",
    "APPLICATOR_HAND",
    "CONVEYOR_SPEED",
    "HEIGHT_SENSOR_CAL_HEIGHT_INPUT_1",
    "HEIGHT_SENSOR_CAL_HEIGHT_INPUT_2",
    "HEIGHT_SENSOR_CAL_HEIGHT_VALUE_1",
    "HEIGHT_SENSOR_CAL_HEIGHT_VALUE_2",
    "POWER_MERGE_DRIVE_RATIO",
    "ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_1",
    "ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_2",
    "ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_3",
    "ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_4",
    "STATUS_OUTPUT_1_SELECT",
    "STATUS_OUTPUT_2_SELECT",
    "STATUS_OUTPUT_3_SELECT",
    "STATUS_OUTPUT_4_SELECT",
    "PRINTER_MODEL_SELECT",
    "RESERVED_0",
    "RFID_ENABLED",
    "APPLICATOR_TYPE",
    "SECONDARY_OUTPUT",
    "HOME_SENSOR_1_PRESENT",
    "HOME_SENSOR_2_PRESENT",
    "DATA_VALID_USED",
    "REJECT_INSTALLED",
    "REJECT_BIN_SENSOR_INSTALLED",
    "REJECT_HOME_SENSOR_PRESENT",
    "LABEL_ON_PAD_INPUT",
    "MISSED_WARNING",
    "HEIGHT_SENSOR_RISE_TIME"
};

static const char * const s_procVarNames[NBR_PROCESS_VARIABLES] = {
    "PRODUCT_DELAY_1",
    "SENSOR_1_EDGE_SELECT",
    "HEIGHT_COMPENSATION_1_ENABLE",
    "SPEED_COMPENSATION_1_ENABLE",
    "HEIGHT_COMPENSATION_1_HEIGHT_1",
    "HEIGHT_COMPENSATION_1_HEIGHT_2",
    "HEIGHT_COMPENSATION_1_DELAY_1",
    "HEIGHT_COMPENSATION_1_DELAY_2",
    "SPEED_COMPENSATION_1_SPEED",
    "ZERO_SPEED_DELAY_1",
    "PRODUCT_DELAY_2",
    "SENSOR_2_EDGE_SELECT",
    "HEIGHT_COMPENSATION_2_ENABLE",
    "SPEED_COMPENSATION_2_ENABLE",
    "HEIGHT_COMPENSATION_2_HEIGHT_1",
    "HEIGHT_COMPENSATION_2_HEIGHT_2",
    "HEIGHT_COMPENSATION_2_DELAY_1",
    "HEIGHT_COMPENSATION_2_DELAY_2",
    "SPEED_COMPENSATION_2_SPEED",
    "ZERO_SPEED_DELAY_2",
    "PRODUCT_DELAY_3",
    "SENSOR_3_EDGE_SELECT",
    "HEIGHT_COMPENSATION_3_ENABLE",
    "SPEED_COMPENSATION_3_ENABLE",
    "HEIGHT_COMPENSATION_3_HEIGHT_1",
    "HEIGHT_COMPENSATION_3_HEIGHT_2",
    "HEIGHT_COMPENSATION_3_DELAY_1",
    "HEIGHT_COMPENSATION_3_DELAY_2",
    "SPEED_COMPENSATION_3_SPEED",
    "ZERO_SPEED_DELAY_3",
    "PRINT_MODE",
    "APPLY_MODE",
    "SECONDARY_WIPE_ENABLE",
    "PRINTER_TIMEOUT_ENABLE",
    "PRINTER_TIMEOUT_PRESET",
    "PRIMARY_PACKAGE_SCALE",
    "ZERO_DOWNTIME",
    "APPLICATOR_ALARM_ENABLE",
    "VACUUM_DELAY_PRESET",
    "AIR_ASSIST_DELAY_PRESET",
    "LABEL_SETTLE_TIME_PRESET",
    "TAMP_EXTEND_PRESET",
    "TAMP_RETRACT_PRESET",
    "AIR_BLAST_PRESET",
    "SMART_TAMP_DELAY_PRESET",
    "SMART_TAMP_DELAY_2_PRESET",
    "SECONDARY_EXTEND_PRESET",
    "SECONDARY_RETRACT_PRESET",
    "ROTARY_1_EXTEND_PRESET",
    "ROTARY_1_RETRACT_PRESET",
    "ROTARY_2_EXTEND_PRESET",
    "ROTARY_2_RETRACT_PRESET",
    "INFEED_TYPE",
    "INSPECTION_TRIGGER",
    "PIN_STOP_EXIT_PRESET",
    "CLAMP_DELAY_PRESET",
    "INSPECTION_TYPE",
    "INSPECTION_1_ENABLE",
    "INSPECTION_2_ENABLE",
    "INSPECTION_TIMEOUT_PRESET",
    "REJECT_ALARMS_ENABLE",
    "CONSECUTIVE_FAILURE_LIMIT",
    "REJECT_EXTEND_PRESET",
    "REJECT_RETRACT_PRESET",
    "REJECT_FULL_PRESET",
    "INSPECTION_DELAY",
    "CARBONFLEX_FRONT_MOVE_ANGLE",
    "CARBONFLEX_FRONT_MOVE_SPEED",
    "CARBONFLEX_SWING_RETURN_SPEED",
    "CARBONFLEX_FRONT_MOVE_FORCE",
    "CARBONFLEX_FRONT_DWELL_TIME",
    "CARBONFLEX_SIDE_MOVE_ANGLE",
    "CARBONFLEX_SIDE_MOVE_SPEED",
    "CARBONFLEX_SIDE_APPLY_FORCE",
    "CARBONFLEX_SIDE_WIPE_FORCE",
    "CARBONFLEX_SIDE_WIPE_DWELL",
    "CARBONFLEX_CORNER_MOVE_ANGLE",
    "CARBONFLEX_CORNER_WRAP_ANGLE",
    "CARBONFLEX_CORNER_MOVE_SPEED",
    "CARBONFLEX_CORNER_MOVE_FORCE",
    "CARBONFLEX_CORNER_WIPE_FORCE",
    "CARBONFLEX_CORNER_WIPE_DWELL",
    "CARBONFLEX_SWING_RETURN_FORCE"
};

/* =========================================================================
 * Type tables -- drive INI value formatting
 * PARAM_TYPE_FLOAT -> "%g"  PARAM_TYPE_INT -> "%d"  PARAM_TYPE_BOOL -> "%d"
 * ========================================================================= */
static const PackParamType_t s_paramTypes[NBR_PARAMETERS] = {
    PARAM_TYPE_BOOL,    /* ENCODER_PRESENT */
    PARAM_TYPE_FLOAT,   /* ENCODER_RATIO */
    PARAM_TYPE_INT,     /* APPLICATOR_HAND */
    PARAM_TYPE_FLOAT,   /* CONVEYOR_SPEED */
    PARAM_TYPE_FLOAT,   /* HEIGHT_SENSOR_CAL_HEIGHT_INPUT_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_SENSOR_CAL_HEIGHT_INPUT_2 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_SENSOR_CAL_HEIGHT_VALUE_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_SENSOR_CAL_HEIGHT_VALUE_2 */
    PARAM_TYPE_FLOAT,   /* POWER_MERGE_DRIVE_RATIO */
    PARAM_TYPE_INT,     /* ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_1 */
    PARAM_TYPE_INT,     /* ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_2 */
    PARAM_TYPE_INT,     /* ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_3 */
    PARAM_TYPE_INT,     /* ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_4 */
    PARAM_TYPE_INT,     /* STATUS_OUTPUT_1_SELECT */
    PARAM_TYPE_INT,     /* STATUS_OUTPUT_2_SELECT */
    PARAM_TYPE_INT,     /* STATUS_OUTPUT_3_SELECT */
    PARAM_TYPE_INT,     /* STATUS_OUTPUT_4_SELECT */
    PARAM_TYPE_INT,     /* PRINTER_MODEL_SELECT */
    PARAM_TYPE_INT,     /* RESERVED_0 */
    PARAM_TYPE_BOOL,    /* RFID_ENABLED */
    PARAM_TYPE_INT,     /* APPLICATOR_TYPE */
    PARAM_TYPE_INT,     /* SECONDARY_OUTPUT */
    PARAM_TYPE_BOOL,    /* HOME_SENSOR_1_PRESENT */
    PARAM_TYPE_BOOL,    /* HOME_SENSOR_2_PRESENT */
    PARAM_TYPE_BOOL,    /* DATA_VALID_USED */
    PARAM_TYPE_BOOL,    /* REJECT_INSTALLED */
    PARAM_TYPE_BOOL,    /* REJECT_BIN_SENSOR_INSTALLED */
    PARAM_TYPE_BOOL,    /* REJECT_HOME_SENSOR_PRESENT */
    PARAM_TYPE_BOOL,    /* LABEL_ON_PAD_INPUT */
    PARAM_TYPE_BOOL,    /* MISSED_WARNING */
    PARAM_TYPE_FLOAT,   /* HEIGHT_SENSOR_RISE_TIME */
};

static const PackParamType_t s_procVarTypes[NBR_PROCESS_VARIABLES] = {
    PARAM_TYPE_FLOAT,   /* PRODUCT_DELAY_1 */
    PARAM_TYPE_INT,     /* SENSOR_1_EDGE_SELECT */
    PARAM_TYPE_BOOL,    /* HEIGHT_COMPENSATION_1_ENABLE */
    PARAM_TYPE_BOOL,    /* SPEED_COMPENSATION_1_ENABLE */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_1_HEIGHT_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_1_HEIGHT_2 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_1_DELAY_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_1_DELAY_2 */
    PARAM_TYPE_FLOAT,   /* SPEED_COMPENSATION_1_SPEED */
    PARAM_TYPE_FLOAT,   /* ZERO_SPEED_DELAY_1 */
    PARAM_TYPE_FLOAT,   /* PRODUCT_DELAY_2 */
    PARAM_TYPE_INT,     /* SENSOR_2_EDGE_SELECT */
    PARAM_TYPE_BOOL,    /* HEIGHT_COMPENSATION_2_ENABLE */
    PARAM_TYPE_BOOL,    /* SPEED_COMPENSATION_2_ENABLE */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_2_HEIGHT_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_2_HEIGHT_2 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_2_DELAY_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_2_DELAY_2 */
    PARAM_TYPE_FLOAT,   /* SPEED_COMPENSATION_2_SPEED */
    PARAM_TYPE_FLOAT,   /* ZERO_SPEED_DELAY_2 */
    PARAM_TYPE_FLOAT,   /* PRODUCT_DELAY_3 */
    PARAM_TYPE_INT,     /* SENSOR_3_EDGE_SELECT */
    PARAM_TYPE_BOOL,    /* HEIGHT_COMPENSATION_3_ENABLE */
    PARAM_TYPE_BOOL,    /* SPEED_COMPENSATION_3_ENABLE */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_3_HEIGHT_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_3_HEIGHT_2 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_3_DELAY_1 */
    PARAM_TYPE_FLOAT,   /* HEIGHT_COMPENSATION_3_DELAY_2 */
    PARAM_TYPE_FLOAT,   /* SPEED_COMPENSATION_3_SPEED */
    PARAM_TYPE_FLOAT,   /* ZERO_SPEED_DELAY_3 */
    PARAM_TYPE_INT,     /* PRINT_MODE */
    PARAM_TYPE_INT,     /* APPLY_MODE */
    PARAM_TYPE_BOOL,    /* SECONDARY_WIPE_ENABLE */
    PARAM_TYPE_BOOL,    /* PRINTER_TIMEOUT_ENABLE */
    PARAM_TYPE_FLOAT,   /* PRINTER_TIMEOUT_PRESET */
    PARAM_TYPE_FLOAT,   /* PRIMARY_PACKAGE_SCALE */
    PARAM_TYPE_BOOL,    /* ZERO_DOWNTIME */
    PARAM_TYPE_BOOL,    /* APPLICATOR_ALARM_ENABLE */
    PARAM_TYPE_FLOAT,   /* VACUUM_DELAY_PRESET */
    PARAM_TYPE_FLOAT,   /* AIR_ASSIST_DELAY_PRESET */
    PARAM_TYPE_FLOAT,   /* LABEL_SETTLE_TIME_PRESET */
    PARAM_TYPE_FLOAT,   /* TAMP_EXTEND_PRESET */
    PARAM_TYPE_FLOAT,   /* TAMP_RETRACT_PRESET */
    PARAM_TYPE_FLOAT,   /* AIR_BLAST_PRESET */
    PARAM_TYPE_FLOAT,   /* SMART_TAMP_DELAY_PRESET */
    PARAM_TYPE_FLOAT,   /* SMART_TAMP_DELAY_2_PRESET */
    PARAM_TYPE_FLOAT,   /* SECONDARY_EXTEND_PRESET */
    PARAM_TYPE_FLOAT,   /* SECONDARY_RETRACT_PRESET */
    PARAM_TYPE_FLOAT,   /* ROTARY_1_EXTEND_PRESET */
    PARAM_TYPE_FLOAT,   /* ROTARY_1_RETRACT_PRESET */
    PARAM_TYPE_FLOAT,   /* ROTARY_2_EXTEND_PRESET */
    PARAM_TYPE_FLOAT,   /* ROTARY_2_RETRACT_PRESET */
    PARAM_TYPE_INT,     /* INFEED_TYPE */
    PARAM_TYPE_INT,     /* INSPECTION_TRIGGER */
    PARAM_TYPE_FLOAT,   /* PIN_STOP_EXIT_PRESET */
    PARAM_TYPE_FLOAT,   /* CLAMP_DELAY_PRESET */
    PARAM_TYPE_INT,     /* INSPECTION_TYPE */
    PARAM_TYPE_BOOL,    /* INSPECTION_1_ENABLE */
    PARAM_TYPE_BOOL,    /* INSPECTION_2_ENABLE */
    PARAM_TYPE_FLOAT,   /* INSPECTION_TIMEOUT_PRESET */
    PARAM_TYPE_BOOL,    /* REJECT_ALARMS_ENABLE */
    PARAM_TYPE_INT,     /* CONSECUTIVE_FAILURE_LIMIT */
    PARAM_TYPE_FLOAT,   /* REJECT_EXTEND_PRESET */
    PARAM_TYPE_FLOAT,   /* REJECT_RETRACT_PRESET */
    PARAM_TYPE_FLOAT,   /* REJECT_FULL_PRESET */
    PARAM_TYPE_FLOAT,   /* INSPECTION_DELAY */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_FRONT_MOVE_ANGLE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_FRONT_MOVE_SPEED */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SWING_RETURN_SPEED */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_FRONT_MOVE_FORCE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_FRONT_DWELL_TIME */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SIDE_MOVE_ANGLE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SIDE_MOVE_SPEED */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SIDE_APPLY_FORCE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SIDE_WIPE_FORCE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SIDE_WIPE_DWELL */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_MOVE_ANGLE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_WRAP_ANGLE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_MOVE_SPEED */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_MOVE_FORCE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_WIPE_FORCE */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_CORNER_WIPE_DWELL */
    PARAM_TYPE_FLOAT,   /* CARBONFLEX_SWING_RETURN_FORCE */
};

/* =========================================================================
 * Helper functions (fully implemented, used by Phase 4 FS implementation)
 * ========================================================================= */
static char *prv_Trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

static int prv_FindParam(const char *name)
{
    for (int i = 0; i < NBR_PARAMETERS; i++)
        if (strcmp(name, s_paramNames[i]) == 0) return i;
    return -1;
}

static int prv_FindProcVar(const char *name)
{
    for (int i = 0; i < NBR_PROCESS_VARIABLES; i++)
        if (strcmp(name, s_procVarNames[i]) == 0) return i;
    return -1;
}

/**
 * @brief Format a float value according to its logical type for INI output
 */
static void prv_FormatValue(char *buf, size_t buflen,
                             float val, PackParamType_t type)
{
    switch (type) {
    case PARAM_TYPE_INT:
        snprintf(buf, buflen, "%d", (int)val);
        break;
    case PARAM_TYPE_BOOL:
        snprintf(buf, buflen, "%d", val != 0.0f ? 1 : 0);
        break;
    case PARAM_TYPE_FLOAT:
    default:
        snprintf(buf, buflen, "%g", (double)val);
        break;
    }
}

/* Suppress unused-function warnings until Phase 4 activates these */
static void prv_suppress_unused(void) __attribute__((unused));
static void prv_suppress_unused(void)
{
    (void)prv_Trim(NULL);
    (void)prv_FindParam(NULL);
    (void)prv_FindProcVar(NULL);
    char buf[32];
    prv_FormatValue(buf, sizeof(buf), 0.0f, PARAM_TYPE_FLOAT);
}

/* =========================================================================
 * PackTag_SaveRecipe -- STUBBED pending Phase 4 (FatFS / Zephyr fs.h)
 *
 * TODO Phase 4: implement using Zephyr fs.h:
 *   struct fs_file_t fp;
 *   fs_open(&fp, fullpath, FS_O_CREATE | FS_O_WRITE);
 *   fs_write(&fp, buf, len);
 *   fs_close(&fp);
 * Use prv_FormatValue() with s_paramTypes[] / s_procVarTypes[] for output.
 * Accumulate CRC16 (seed CRC16_SEED) over all content before [CRC] section.
 * ========================================================================= */
PackRecipeResult_t PackTag_SaveRecipe(const char *path, const char *recipeName)
{
    ARG_UNUSED(path);
    ARG_UNUSED(recipeName);
    LOG_WRN("PackTag_SaveRecipe: filesystem not yet available (Phase 4)");
    return RECIPE_ERR_NOT_AVAILABLE;
}

/* =========================================================================
 * PackTag_LoadRecipe -- STUBBED pending Phase 4 (FatFS / Zephyr fs.h)
 *
 * TODO Phase 4: implement two-pass parse:
 *   Pass 1: accumulate CRC, locate [CRC] value, verify
 *   Pass 2: parse KEY=VALUE pairs, call PackTag_SetParameter /
 *           PackTag_SetProcessVariable with strtof(value)
 * ========================================================================= */
PackRecipeResult_t PackTag_LoadRecipe(const char *path, const char *recipeName)
{
    ARG_UNUSED(path);
    ARG_UNUSED(recipeName);
    LOG_WRN("PackTag_LoadRecipe: filesystem not yet available (Phase 4)");
    return RECIPE_ERR_NOT_AVAILABLE;
}