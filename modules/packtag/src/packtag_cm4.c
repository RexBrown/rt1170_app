/*
 * packtag_cm4.c
 *
 * PackTag access for CM4 -- direct shared RAM access, no mutex needed.
 * CM4 owns its working copy (pPackTagCM4Work) during normal operation.
 * CM7 only writes to it during the sync window at product cycle end.
 *
 * Zephyr port: no RTOS changes required -- pure pointer operations only.
 */

#include "packtag_types.h"
#include <zephyr/kernel.h>

/* =========================================================================
 * CM4 read -- direct from working copy
 * ========================================================================= */
float PackTagCM4_GetParameter(enum_PARAMETER param)
{
    if (param >= NBR_PARAMETERS) return 0.0f;
    return pPackTagCM4Work->parameter[param].value;
}

float PackTagCM4_GetProcessVariable(enum_PROCESS_VARIABLE pv)
{
    if (pv >= NBR_PROCESS_VARIABLES) return 0.0f;
    return pPackTagCM4Work->product[0].processVariables[pv].value;
}

/* =========================================================================
 * CM4 write -- updates working copy and sets CM4 dirty flags.
 * CM7 picks these up via PackTag_CM7_SyncFromCM4().
 * ========================================================================= */
bool PackTagCM4_SetParameter(enum_PARAMETER param, float value)
{
    if (param >= NBR_PARAMETERS) return false;
    pPackTagCM4Work->parameter[param].value        = value;
    pPackTagFlags->parameter_flags[CM4_IDX][param] = true;
    pPackTagFlags->global_flag[CM4_IDX]            = true;
    return true;
}

bool PackTagCM4_SetProcessVariable(enum_PROCESS_VARIABLE pv, float value)
{
    if (pv >= NBR_PROCESS_VARIABLES) return false;
    pPackTagCM4Work->product[0].processVariables[pv].value = value;
    pPackTagFlags->process_variable_flags[CM4_IDX][pv]     = true;
    pPackTagFlags->global_flag[CM4_IDX]                    = true;
    return true;
}

/* =========================================================================
 * CM4 product cycle end -- sync CM7 changes into working copy.
 * Call at end of each product cycle.
 * ========================================================================= */
void PackTagCM4_SyncFromCM7(void)
{
    if (!pPackTagFlags->global_flag[CM7_IDX]) return;

    for (int i = 0; i < CMD_PARAM_COUNT; i++) {
        if (pPackTagFlags->parameter_flags[CM7_IDX][i]) {
            pPackTagCM4Work->parameter[i].value =
                pPackTagMaster->cmd.parameter[i].value;
            pPackTagFlags->parameter_flags[CM7_IDX][i] = false;
        }
    }

    for (int i = 0; i < CMD_PROCVAR_COUNT; i++) {
        if (pPackTagFlags->process_variable_flags[CM7_IDX][i]) {
            pPackTagCM4Work->product[0].processVariables[i].value =
                pPackTagMaster->cmd.product[0].processVariables[i].value;
            pPackTagFlags->process_variable_flags[CM7_IDX][i] = false;
        }
    }

    pPackTagFlags->global_flag[CM7_IDX] = false;
}

/* =========================================================================
 * CM4 product cycle complete signal -- sets flag for CM7 to act on
 * ========================================================================= */
void PackTagCM4_SignalProductCycleComplete(void)
{
    pPackTagFlags->product_cycle_complete = true;
}