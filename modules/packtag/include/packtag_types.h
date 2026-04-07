/*
 * packtag_types.h
 *
 * PackML/PackTag data structures for the Model 255 labeler
 * Based on:
 *   - Model 255 Interface Specification R2401
 *   - ISA-TR88.00.02-2022
 *
 * Memory layout (shared OCRAM):
 *   PackTag master copy (CM7 owns)        16KB  @ 0x20240000  section PACKTAG_MASTER
 *   PackTag CM4 working copy (PackCmd_t)   4KB  @ 0x20244000  section PACKTAG_CM4WORK
 *   PackTag flag structures                4KB  @ 0x20245000  section PACKTAG_FLAGS
 *
 * These addresses are declared as zephyr,memory-region nodes in both
 * CM7 and CM4 board overlays so the linker places them correctly.
 *
 * Synchronization:
 *   global_flag[CM7_IDX] -- set by CM7 when master copy changes
 *   global_flag[CM4_IDX] -- set by CM4 when working copy changes
 *   CM4 checks CM7 flag at end of each product cycle
 *   CM7 polls CM4 flag and propagates changes to GUI/Ethernet
 */

#ifndef PACKTAG_TYPES_H_
#define PACKTAG_TYPES_H_

#include <zephyr/kernel.h>

/* =========================================================================
 * Core index constants
 * ========================================================================= */
#define CM7_IDX     0
#define CM4_IDX     1
#define NUM_CORES   2

/* =========================================================================
 * Size constants from Model 255 Interface Specification R2401
 * ========================================================================= */
#define PACKML_STRING_MAX           80
#define PACKML_ALARM_MSG_MAX        30
#define PACKML_RECIPE_NAME_MAX      20
#define PACKML_DATETIME_ELEMENTS     7

#define CMD_PARAM_COUNT             31   /* Parameter[0..30] */
#define CMD_PROCVAR_COUNT           83   /* ProcessVariables[0..82] */
#define ADMIN_ALARM_MAX             64

/* =========================================================================
 * Basic types
 * ========================================================================= */
typedef int32_t     PackInt;
typedef float       PackReal;
typedef bool        PackBool;
typedef char        PackString[PACKML_STRING_MAX + 1];

/* =========================================================================
 * DateTimeStructure -- ISO 8601:1988, HH in 24-hour format
 * ========================================================================= */
typedef struct {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t microsecond;
} PackDateTime_t;

/* =========================================================================
 * ParameterStructure
 * ========================================================================= */
typedef struct {
    float value;
} PackParameter_t;

/* =========================================================================
 * AlarmStructure
 * ========================================================================= */
typedef struct {
    int32_t         id;
    bool            trigger;
    char            message[PACKML_ALARM_MSG_MAX + 1];
    int32_t         category;
    PackDateTime_t  dateTime;
    PackDateTime_t  ackDateTime;
} PackAlarm_t;

/* =========================================================================
 * CountStructure
 * ========================================================================= */
typedef struct {
    int32_t count;
    int32_t accCount;
} PackCount_t;

/* =========================================================================
 * Command structure  (read/write)
 * ========================================================================= */
typedef struct {
    PackInt         unitMode;
    PackBool        unitModeChangeRequest;
    PackReal        machSpeed;
    PackInt         cntrlCommand;
    PackBool        cmdChangeRequest;
    PackParameter_t parameter[CMD_PARAM_COUNT];
    struct {
        PackParameter_t processVariables[CMD_PROCVAR_COUNT];
    } product[1];
} PackCmd_t;

/* =========================================================================
 * Status structure  (read-only from external perspective)
 * ========================================================================= */
typedef struct {
    PackInt         unitModeCurrent;
    PackInt         stateCurrent;
    PackReal        machSpeed;
    PackReal        currMachSpeed;
    struct {
        PackBool    blocked;
        PackBool    starved;
    } equipmentInterlock;
    PackParameter_t parameter[CMD_PARAM_COUNT];
    struct {
        PackParameter_t processVariables[CMD_PROCVAR_COUNT];
    } product[1];
} PackStatus_t;

/* =========================================================================
 * Count enum sub-indices
 * ========================================================================= */
typedef enum {
    LABEL_PROCESSED  = 0,
    LABEL_INSPECTED  = 1,
    LABEL_CONSUMED   = 2,
    NBR_LABEL_COUNT_TYPES
} enum_LABEL_COUNT_TYPES;

typedef enum {
    GOOD_PROCESSED   = 0,
    GOOD_INSPECTED   = 1,
    PROD_PASSED      = 2,
    NBR_GOOD_COUNT_TYPES
} enum_GOOD_COUNT_TYPES;

typedef enum {
    REJ_PROCESSED    = 0,
    REJ_INSPECTED    = 1,
    NBR_DEFECTIVE_COUNT_TYPES
} enum_DEFECTIVE_COUNT_TYPES;

/* =========================================================================
 * Admin structure  (read-only; counters may be reset together)
 * ========================================================================= */
typedef struct {
    PackCount_t     prodConsumedCount[NBR_LABEL_COUNT_TYPES];
    PackCount_t     prodProcessedCount[NBR_GOOD_COUNT_TYPES];
    PackCount_t     prodDefectiveCount[NBR_DEFECTIVE_COUNT_TYPES];
    PackDateTime_t  plcDateTime;
    PackReal        machineDesignSpeed;
    PackAlarm_t     stopReason;
    PackAlarm_t     alarm[ADMIN_ALARM_MAX];
    int32_t         alarmExtent;
    PackAlarm_t     warning[ADMIN_ALARM_MAX];
    int32_t         warningExtent;
} PackAdmin_t;

/* =========================================================================
 * Additional tags
 * ========================================================================= */
typedef struct {
    int32_t         remoteStartButton;
    int32_t         remoteStopButton;
    int32_t         remoteResetButton;
    int32_t         productDetect1;
    int32_t         productDetect2;
    int32_t         productDetect3;
    int32_t         printTrigger;
    int32_t         inhibitLabeling;
    PackDateTime_t  hmiNewTime;
    int32_t         remoteUpdateTime;
    PackReal        hmiProductSpeed;
    int32_t         hardwareTestBits;
    int32_t         hmiSystemStatusBits;
    int32_t         hmiPrintCycleStatusBits;
    int32_t         hmiPrintCycleTime;
    int32_t         hmiApplyCycleStatusBits;
    int32_t         hmiApplicator1LastCycle;
    int32_t         hmiApplicator2LastCycle;
    int32_t         hmiApplicator3LastCycle;
    int32_t         recipeNumber;
    char            remoteRecipeName[PACKML_RECIPE_NAME_MAX + 1];
    int32_t         remoteLoadRecipe;
    int32_t         remoteLoadRecipeComplete;
    int32_t         remoteLoadRecipeFailed;
    int32_t         remoteSaveRecipe;
    int32_t         remoteSaveRecipeComplete;
    int32_t         remoteSaveRecipeFailed;
    int32_t         remoteSettingsBackup;
    int32_t         remoteSettingsBackupComplete;
    int32_t         remoteSettingsBackupFailed;
    PackAlarm_t     alarms[ADMIN_ALARM_MAX];
} PackAdditional_t;

/* =========================================================================
 * Top-level PackTag master store
 * ========================================================================= */
typedef struct {
    PackCmd_t        cmd;
    PackStatus_t     status;
    PackAdmin_t      admin;
    PackAdditional_t additional;
} PackTagStore_t;

/* =========================================================================
 * Enum -- Command tag identifiers
 * ========================================================================= */
typedef enum {
    UNIT_MODE                   = 0,
    UNIT_MODE_CHANGE_REQUEST,
    MACHINE_SPEED,
    CONTROL_COMMAND,
    COMMAND_CHANGE_REQUEST,
    COMMAND_PARAMETERS,
    COMMAND_PROCESS_VARIABLES,
    NBR_COMMAND_TAGS
} enum_COMMAND_TAGS;

/* =========================================================================
 * Enum -- Parameter indices  (Command.Parameter[0..30])
 * ========================================================================= */
typedef enum {
    ENCODER_PRESENT                      = 0,
    ENCODER_RATIO                        = 1,
    APPLICATOR_HAND                      = 2,
    CONVEYOR_SPEED                       = 3,
    HEIGHT_SENSOR_CAL_HEIGHT_INPUT_1     = 4,
    HEIGHT_SENSOR_CAL_HEIGHT_INPUT_2     = 5,
    HEIGHT_SENSOR_CAL_HEIGHT_VALUE_1     = 6,
    HEIGHT_SENSOR_CAL_HEIGHT_VALUE_2     = 7,
    POWER_MERGE_DRIVE_RATIO              = 8,
    ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_1  = 9,
    ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_2  = 10,
    ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_3  = 11,
    ZERO_DOWNTIME_OTHER_HEAD_IP_OCTET_4  = 12,
    STATUS_OUTPUT_1_SELECT               = 13,
    STATUS_OUTPUT_2_SELECT               = 14,
    STATUS_OUTPUT_3_SELECT               = 15,
    STATUS_OUTPUT_4_SELECT               = 16,
    PRINTER_MODEL_SELECT                 = 17,
    RESERVED_0                           = 18,
    RFID_ENABLED                         = 19,
    APPLICATOR_TYPE                      = 20,
    SECONDARY_OUTPUT                     = 21,
    HOME_SENSOR_1_PRESENT                = 22,
    HOME_SENSOR_2_PRESENT                = 23,
    DATA_VALID_USED                      = 24,
    REJECT_INSTALLED                     = 25,
    REJECT_BIN_SENSOR_INSTALLED          = 26,
    REJECT_HOME_SENSOR_PRESENT           = 27,
    LABEL_ON_PAD_INPUT                   = 28,
    MISSED_WARNING                       = 29,
    HEIGHT_SENSOR_RISE_TIME              = 30,
    NBR_PARAMETERS                       /* = 31 */
} enum_PARAMETER;

/* =========================================================================
 * Parameter logical type -- used by recipe writer to format INI values.
 * All PackParameter_t.value fields are float in the struct.
 * This enum drives correct formatting only; strtof() handles all on load.
 * ========================================================================= */
typedef enum {
    PARAM_TYPE_FLOAT = 0,   /* real-valued: speeds, delays, ratios, angles */
    PARAM_TYPE_INT,          /* integer-valued: selects, octets, counts     */
    PARAM_TYPE_BOOL,         /* boolean 0/1                                  */
} PackParamType_t;

/* =========================================================================
 * Enum -- ProcessVariable indices  (Command.Product[0].ProcessVariables[0..82])
 * ========================================================================= */
typedef enum {
    PRODUCT_DELAY_1                      = 0,
    SENSOR_1_EDGE_SELECT                 = 1,
    HEIGHT_COMPENSATION_1_ENABLE         = 2,
    SPEED_COMPENSATION_1_ENABLE          = 3,
    HEIGHT_COMPENSATION_1_HEIGHT_1       = 4,
    HEIGHT_COMPENSATION_1_HEIGHT_2       = 5,
    HEIGHT_COMPENSATION_1_DELAY_1        = 6,
    HEIGHT_COMPENSATION_1_DELAY_2        = 7,
    SPEED_COMPENSATION_1_SPEED           = 8,
    ZERO_SPEED_DELAY_1                   = 9,
    PRODUCT_DELAY_2                      = 10,
    SENSOR_2_EDGE_SELECT                 = 11,
    HEIGHT_COMPENSATION_2_ENABLE         = 12,
    SPEED_COMPENSATION_2_ENABLE          = 13,
    HEIGHT_COMPENSATION_2_HEIGHT_1       = 14,
    HEIGHT_COMPENSATION_2_HEIGHT_2       = 15,
    HEIGHT_COMPENSATION_2_DELAY_1        = 16,
    HEIGHT_COMPENSATION_2_DELAY_2        = 17,
    SPEED_COMPENSATION_2_SPEED           = 18,
    ZERO_SPEED_DELAY_2                   = 19,
    PRODUCT_DELAY_3                      = 20,
    SENSOR_3_EDGE_SELECT                 = 21,
    HEIGHT_COMPENSATION_3_ENABLE         = 22,
    SPEED_COMPENSATION_3_ENABLE          = 23,
    HEIGHT_COMPENSATION_3_HEIGHT_1       = 24,
    HEIGHT_COMPENSATION_3_HEIGHT_2       = 25,
    HEIGHT_COMPENSATION_3_DELAY_1        = 26,
    HEIGHT_COMPENSATION_3_DELAY_2        = 27,
    SPEED_COMPENSATION_3_SPEED           = 28,
    ZERO_SPEED_DELAY_3                   = 29,
    PRINT_MODE                           = 30,
    APPLY_MODE                           = 31,
    SECONDARY_WIPE_ENABLE                = 32,
    PRINTER_TIMEOUT_ENABLE               = 33,
    PRINTER_TIMEOUT_PRESET               = 34,
    PRIMARY_PACKAGE_SCALE                = 35,
    ZERO_DOWNTIME                        = 36,
    APPLICATOR_ALARM_ENABLE              = 37,
    VACUUM_DELAY_PRESET                  = 38,
    AIR_ASSIST_DELAY_PRESET              = 39,
    LABEL_SETTLE_TIME_PRESET             = 40,
    TAMP_EXTEND_PRESET                   = 41,
    TAMP_RETRACT_PRESET                  = 42,
    AIR_BLAST_PRESET                     = 43,
    SMART_TAMP_DELAY_PRESET              = 44,
    SMART_TAMP_DELAY_2_PRESET            = 45,
    SECONDARY_EXTEND_PRESET              = 46,
    SECONDARY_RETRACT_PRESET             = 47,
    ROTARY_1_EXTEND_PRESET               = 48,
    ROTARY_1_RETRACT_PRESET              = 49,
    ROTARY_2_EXTEND_PRESET               = 50,
    ROTARY_2_RETRACT_PRESET              = 51,
    INFEED_TYPE                          = 52,
    INSPECTION_TRIGGER                   = 53,
    PIN_STOP_EXIT_PRESET                 = 54,
    CLAMP_DELAY_PRESET                   = 55,
    INSPECTION_TYPE                      = 56,
    INSPECTION_1_ENABLE                  = 57,
    INSPECTION_2_ENABLE                  = 58,
    INSPECTION_TIMEOUT_PRESET            = 59,
    REJECT_ALARMS_ENABLE                 = 60,
    CONSECUTIVE_FAILURE_LIMIT            = 61,
    REJECT_EXTEND_PRESET                 = 62,
    REJECT_RETRACT_PRESET                = 63,
    REJECT_FULL_PRESET                   = 64,
    INSPECTION_DELAY                     = 65,
    CARBONFLEX_FRONT_MOVE_ANGLE          = 66,
    CARBONFLEX_FRONT_MOVE_SPEED          = 67,
    CARBONFLEX_SWING_RETURN_SPEED        = 68,
    CARBONFLEX_FRONT_MOVE_FORCE          = 69,
    CARBONFLEX_FRONT_DWELL_TIME          = 70,
    CARBONFLEX_SIDE_MOVE_ANGLE           = 71,
    CARBONFLEX_SIDE_MOVE_SPEED           = 72,
    CARBONFLEX_SIDE_APPLY_FORCE          = 73,
    CARBONFLEX_SIDE_WIPE_FORCE           = 74,
    CARBONFLEX_SIDE_WIPE_DWELL           = 75,
    CARBONFLEX_CORNER_MOVE_ANGLE         = 76,
    CARBONFLEX_CORNER_WRAP_ANGLE         = 77,
    CARBONFLEX_CORNER_MOVE_SPEED         = 78,
    CARBONFLEX_CORNER_MOVE_FORCE         = 79,
    CARBONFLEX_CORNER_WIPE_FORCE         = 80,
    CARBONFLEX_CORNER_WIPE_DWELL         = 81,
    CARBONFLEX_SWING_RETURN_FORCE        = 82,
    NBR_PROCESS_VARIABLES                /* = 83 */
} enum_PROCESS_VARIABLE;

/* =========================================================================
 * Enum -- Status tag identifiers
 * ========================================================================= */
typedef enum {
    UNIT_MODE_CURRENT       = 0,
    STATE_CURRENT,
    STATUS_MACHINE_SPEED,
    CURR_MACHINE_SPEED,
    EQUIPMENT_INTERLOCK,
    STATUS_PARAMETERS,
    STATUS_PROCESS_VARIABLES,
    NBR_STATUS_TAGS
} enum_STATUS_TAGS;

/* =========================================================================
 * Enum -- Admin tag identifiers
 * ========================================================================= */
typedef enum {
    PRODUCT_CONSUMED_COUNT  = 0,
    PRODUCT_PROCESSED_COUNT,
    PRODUCT_DEFECTIVE_COUNT,
    PLC_DATE_TIME,
    MACHINE_DESIGN_SPEED,
    STOP_REASON,
    ALARM,
    ALARM_EXTENT,
    WARNING,
    WARNING_EXTENT,
    NBR_ADMIN_TAGS
} enum_ADMIN_TAGS;

/* =========================================================================
 * Enum -- Additional tag identifiers
 * ========================================================================= */
typedef enum {
    REMOTE_START_BUTTON             = 0,
    REMOTE_STOP_BUTTON,
    REMOTE_RESET_BUTTON,
    PRODUCT_DETECT_1,
    PRODUCT_DETECT_2,
    PRODUCT_DETECT_3,
    PRINT_TRIGGER,
    INHIBIT_LABELING,
    HMI_NEW_TIME,
    REMOTE_UPDATE_TIME,
    HMI_PRODUCT_SPEED,
    HARDWARE_TEST_BITS,
    HMI_SYSTEM_STATUS_BITS,
    HMI_PRINT_CYCLE_STATUS_BITS,
    HMI_PRINT_CYCLE_TIME,
    HMI_APPLY_CYCLE_STATUS_BITS,
    HMI_APPLICATOR_1_LAST_CYCLE,
    HMI_APPLICATOR_2_LAST_CYCLE,
    HMI_APPLICATOR_3_LAST_CYCLE,
    RECIPE_NUMBER,
    REMOTE_RECIPE_NAME,
    REMOTE_LOAD_RECIPE,
    REMOTE_LOAD_RECIPE_COMPLETE,
    REMOTE_LOAD_RECIPE_FAILED,
    REMOTE_SAVE_RECIPE,
    REMOTE_SAVE_RECIPE_COMPLETE,
    REMOTE_SAVE_RECIPE_FAILED,
    REMOTE_SETTINGS_BACKUP,
    REMOTE_SETTINGS_BACKUP_COMPLETE,
    REMOTE_SETTINGS_BACKUP_FAILED,
    ALARMS,
    NBR_ADDITIONAL_TAGS
} enum_ADDITIONAL_TAGS;

/* =========================================================================
 * PackTag change flag structure
 * ========================================================================= */
typedef struct {
    volatile bool   global_flag[NUM_CORES];
    volatile bool   parameter_flags[NUM_CORES][CMD_PARAM_COUNT];
    volatile bool   process_variable_flags[NUM_CORES][CMD_PROCVAR_COUNT];
    volatile bool   admin_alarm_flags[NUM_CORES][ADMIN_ALARM_MAX];
    volatile bool   admin_warning_flags[NUM_CORES][ADMIN_ALARM_MAX];
    volatile bool   additional_flags[NUM_CORES][NBR_ADDITIONAL_TAGS];
    volatile bool   additional_alarm_flags[NUM_CORES][ADMIN_ALARM_MAX];
    volatile bool   product_cycle_complete;
} PackTagFlags_t;

/* =========================================================================
 * Compile-time size assertions
 * ========================================================================= */
BUILD_ASSERT(NBR_PARAMETERS == CMD_PARAM_COUNT,
    "NBR_PARAMETERS must equal CMD_PARAM_COUNT");
BUILD_ASSERT(NBR_PROCESS_VARIABLES == CMD_PROCVAR_COUNT,
    "NBR_PROCESS_VARIABLES must equal CMD_PROCVAR_COUNT");

/* =========================================================================
 * Shared memory instance declarations
 * Definitions in packtag.c (CM7) -- both cores access via these pointers
 * ========================================================================= */
extern PackTagStore_t  * const pPackTagMaster;
extern PackCmd_t       * const pPackTagCM4Work;
extern PackTagFlags_t  * const pPackTagFlags;

#endif /* PACKTAG_TYPES_H_ */
