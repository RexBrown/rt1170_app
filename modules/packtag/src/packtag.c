/*
 * packtag.c
 *
 * PackTag store implementation -- CM7 side (Zephyr port)
 *
 * FreeRTOS -> Zephyr changes:
 *   SemaphoreHandle_t / xSemaphoreCreateMutex -> K_MUTEX_DEFINE
 *   xSemaphoreTake(mutex, pdMS_TO_TICKS(10))  -> k_mutex_lock(&m, K_MSEC(10))
 *   xSemaphoreGive                            -> k_mutex_unlock
 *   configASSERT                              -> removed (K_MUTEX_DEFINE is static)
 *   FreeRTOS.h / semphr.h                     -> zephyr/kernel.h
 */

#include "packtag.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(packtag, LOG_LEVEL_INF);

/* =========================================================================
 * Shared memory -- section names match zephyr,memory-region in overlays.
 * Both cores access these at the same physical address.
 * ========================================================================= */
__attribute__((section("PACKTAG_MASTER")))
static PackTagStore_t  s_packTagMaster;

__attribute__((section("PACKTAG_CM4WORK")))
static PackCmd_t       s_packTagCM4Work;

__attribute__((section("PACKTAG_FLAGS")))
static PackTagFlags_t  s_packTagFlags;

/* Public const pointers -- used by CM7 directly and by CM4 via extern */
PackTagStore_t  * const pPackTagMaster  = &s_packTagMaster;
PackCmd_t       * const pPackTagCM4Work = &s_packTagCM4Work;
PackTagFlags_t  * const pPackTagFlags   = &s_packTagFlags;

/* =========================================================================
 * Zephyr mutex -- protects master copy on CM7 side
 * CM4 accesses its working copy directly (no mutex needed on CM4 side)
 * ========================================================================= */
static K_MUTEX_DEFINE(s_packtag_mutex);

/* =========================================================================
 * Internal helpers
 * ========================================================================= */
static inline void prv_SetCM7ParamFlag(enum_PARAMETER param)
{
    pPackTagFlags->parameter_flags[CM7_IDX][param] = true;
    pPackTagFlags->global_flag[CM7_IDX]            = true;
}

static inline void prv_SetCM7ProcVarFlag(enum_PROCESS_VARIABLE pv)
{
    pPackTagFlags->process_variable_flags[CM7_IDX][pv] = true;
    pPackTagFlags->global_flag[CM7_IDX]                = true;
}

static bool prv_Lock(void)
{
    return (k_mutex_lock(&s_packtag_mutex, K_MSEC(10)) == 0);
}

static void prv_Unlock(void)
{
    k_mutex_unlock(&s_packtag_mutex);
}

/* =========================================================================
 * Initialization -- call once from CM7 before releasing CM4
 * ========================================================================= */
void PackTag_Init(void)
{
    memset(&s_packTagMaster,  0, sizeof(s_packTagMaster));
    memset(&s_packTagCM4Work, 0, sizeof(s_packTagCM4Work));
    memset(&s_packTagFlags,   0, sizeof(s_packTagFlags));
    /* K_MUTEX_DEFINE initializes statically -- no runtime init needed */
    LOG_INF("PackTag initialized");
}

/* =========================================================================
 * Parameter read/write
 * ========================================================================= */
bool PackTag_GetParameter(enum_PARAMETER param, float *value)
{
    if (param >= NBR_PARAMETERS || value == NULL) return false;
    if (!prv_Lock()) return false;
    *value = pPackTagMaster->cmd.parameter[param].value;
    prv_Unlock();
    return true;
}

bool PackTag_SetParameter(enum_PARAMETER param, float value)
{
    if (param >= NBR_PARAMETERS) return false;
    if (!prv_Lock()) return false;
    pPackTagMaster->cmd.parameter[param].value = value;
    prv_SetCM7ParamFlag(param);
    prv_Unlock();
    return true;
}

/* =========================================================================
 * Process variable read/write
 * ========================================================================= */
bool PackTag_GetProcessVariable(enum_PROCESS_VARIABLE pv, float *value)
{
    if (pv >= NBR_PROCESS_VARIABLES || value == NULL) return false;
    if (!prv_Lock()) return false;
    *value = pPackTagMaster->cmd.product[0].processVariables[pv].value;
    prv_Unlock();
    return true;
}

bool PackTag_SetProcessVariable(enum_PROCESS_VARIABLE pv, float value)
{
    if (pv >= NBR_PROCESS_VARIABLES) return false;
    if (!prv_Lock()) return false;
    pPackTagMaster->cmd.product[0].processVariables[pv].value = value;
    prv_SetCM7ProcVarFlag(pv);
    prv_Unlock();
    return true;
}

/* =========================================================================
 * State and mode
 * ========================================================================= */
bool PackTag_GetUnitMode(int32_t *mode)
{
    if (mode == NULL) return false;
    if (!prv_Lock()) return false;
    *mode = pPackTagMaster->cmd.unitMode;
    prv_Unlock();
    return true;
}

bool PackTag_SetUnitMode(int32_t mode)
{
    if (!prv_Lock()) return false;
    pPackTagMaster->cmd.unitMode              = mode;
    pPackTagMaster->cmd.unitModeChangeRequest = true;
    pPackTagFlags->global_flag[CM7_IDX]       = true;
    prv_Unlock();
    return true;
}

bool PackTag_GetCntrlCommand(int32_t *cmd)
{
    if (cmd == NULL) return false;
    if (!prv_Lock()) return false;
    *cmd = pPackTagMaster->cmd.cntrlCommand;
    prv_Unlock();
    return true;
}

bool PackTag_SetCntrlCommand(int32_t cmd)
{
    if (!prv_Lock()) return false;
    pPackTagMaster->cmd.cntrlCommand     = cmd;
    pPackTagMaster->cmd.cmdChangeRequest = true;
    pPackTagFlags->global_flag[CM7_IDX]  = true;
    prv_Unlock();
    return true;
}

bool PackTag_GetStateCurrent(int32_t *state)
{
    if (state == NULL) return false;
    if (!prv_Lock()) return false;
    *state = pPackTagMaster->status.stateCurrent;
    prv_Unlock();
    return true;
}

/* =========================================================================
 * CM7 sync: propagate CM4 working copy changes into master copy
 * Call from CM7 polling task when pPackTagFlags->global_flag[CM4_IDX] is set
 * ========================================================================= */
void PackTag_CM7_SyncFromCM4(void)
{
    if (!pPackTagFlags->global_flag[CM4_IDX]) return;
    if (!prv_Lock()) return;

    for (int i = 0; i < CMD_PARAM_COUNT; i++) {
        if (pPackTagFlags->parameter_flags[CM4_IDX][i]) {
            pPackTagMaster->cmd.parameter[i].value =
                pPackTagCM4Work->parameter[i].value;
            pPackTagFlags->parameter_flags[CM4_IDX][i] = false;
        }
    }

    for (int i = 0; i < CMD_PROCVAR_COUNT; i++) {
        if (pPackTagFlags->process_variable_flags[CM4_IDX][i]) {
            pPackTagMaster->cmd.product[0].processVariables[i].value =
                pPackTagCM4Work->product[0].processVariables[i].value;
            pPackTagFlags->process_variable_flags[CM4_IDX][i] = false;
        }
    }

    pPackTagFlags->global_flag[CM4_IDX] = false;
    prv_Unlock();

    /* TODO: notify GUI/Ethernet of changes */
}

/* =========================================================================
 * Admin API
 * ========================================================================= */
bool PackTag_GetProdConsumedCount(enum_LABEL_COUNT_TYPES idx, PackCount_t *count)
{
    if (idx >= NBR_LABEL_COUNT_TYPES || count == NULL) return false;
    if (!prv_Lock()) return false;
    *count = pPackTagMaster->admin.prodConsumedCount[idx];
    prv_Unlock();
    return true;
}

bool PackTag_GetProdProcessedCount(enum_GOOD_COUNT_TYPES idx, PackCount_t *count)
{
    if (idx >= NBR_GOOD_COUNT_TYPES || count == NULL) return false;
    if (!prv_Lock()) return false;
    *count = pPackTagMaster->admin.prodProcessedCount[idx];
    prv_Unlock();
    return true;
}

bool PackTag_GetProdDefectiveCount(enum_DEFECTIVE_COUNT_TYPES idx, PackCount_t *count)
{
    if (idx >= NBR_DEFECTIVE_COUNT_TYPES || count == NULL) return false;
    if (!prv_Lock()) return false;
    *count = pPackTagMaster->admin.prodDefectiveCount[idx];
    prv_Unlock();
    return true;
}

bool PackTag_ResetAllCounts(void)
{
    if (!prv_Lock()) return false;
    memset(pPackTagMaster->admin.prodConsumedCount,  0,
           sizeof(pPackTagMaster->admin.prodConsumedCount));
    memset(pPackTagMaster->admin.prodProcessedCount, 0,
           sizeof(pPackTagMaster->admin.prodProcessedCount));
    memset(pPackTagMaster->admin.prodDefectiveCount, 0,
           sizeof(pPackTagMaster->admin.prodDefectiveCount));
    prv_Unlock();
    return true;
}

bool PackTag_GetAlarm(uint32_t idx, PackAlarm_t *alarm)
{
    if (idx >= ADMIN_ALARM_MAX || alarm == NULL) return false;
    if (!prv_Lock()) return false;
    *alarm = pPackTagMaster->admin.alarm[idx];
    prv_Unlock();
    return true;
}

bool PackTag_GetStopReason(PackAlarm_t *alarm)
{
    if (alarm == NULL) return false;
    if (!prv_Lock()) return false;
    *alarm = pPackTagMaster->admin.stopReason;
    prv_Unlock();
    return true;
}

/* =========================================================================
 * Additional tag API
 * ========================================================================= */
bool PackTag_SetRemoteStartButton(int32_t value)
{
    if (!prv_Lock()) return false;
    pPackTagMaster->additional.remoteStartButton                   = value;
    pPackTagFlags->additional_flags[CM7_IDX][REMOTE_START_BUTTON] = true;
    pPackTagFlags->global_flag[CM7_IDX]                            = true;
    prv_Unlock();
    return true;
}

bool PackTag_SetRemoteStopButton(int32_t value)
{
    if (!prv_Lock()) return false;
    pPackTagMaster->additional.remoteStopButton                   = value;
    pPackTagFlags->additional_flags[CM7_IDX][REMOTE_STOP_BUTTON]  = true;
    pPackTagFlags->global_flag[CM7_IDX]                            = true;
    prv_Unlock();
    return true;
}

bool PackTag_SetInhibitLabeling(int32_t value)
{
    if (!prv_Lock()) return false;
    pPackTagMaster->additional.inhibitLabeling                    = value;
    pPackTagFlags->additional_flags[CM7_IDX][INHIBIT_LABELING]    = true;
    pPackTagFlags->global_flag[CM7_IDX]                            = true;
    prv_Unlock();
    return true;
}

bool PackTag_GetHmiSystemStatusBits(int32_t *bits)
{
    if (bits == NULL) return false;
    if (!prv_Lock()) return false;
    *bits = pPackTagMaster->additional.hmiSystemStatusBits;
    prv_Unlock();
    return true;
}

bool PackTag_GetHmiPrintCycleStatusBits(int32_t *bits)
{
    if (bits == NULL) return false;
    if (!prv_Lock()) return false;
    *bits = pPackTagMaster->additional.hmiPrintCycleStatusBits;
    prv_Unlock();
    return true;
}

bool PackTag_GetHmiApplyCycleStatusBits(int32_t *bits)
{
    if (bits == NULL) return false;
    if (!prv_Lock()) return false;
    *bits = pPackTagMaster->additional.hmiApplyCycleStatusBits;
    prv_Unlock();
    return true;
}
