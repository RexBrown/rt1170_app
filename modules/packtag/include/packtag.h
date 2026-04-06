/*
 * packtag.h
 *
 * Public API for PackTag parameter store -- CM7 side
 * CM4 accesses shared memory directly via pPackTagCM4Work and pPackTagFlags
 *
 * Zephyr port: FreeRTOS mutex replaced with k_mutex
 */

#ifndef PACKTAG_H_
#define PACKTAG_H_

#include "packtag_types.h"

/* Initialization -- call once from CM7 before releasing CM4 */
void PackTag_Init(void);

/* Parameter API */
bool PackTag_GetParameter(enum_PARAMETER param, float *value);
bool PackTag_SetParameter(enum_PARAMETER param, float value);

/* Process Variable API */
bool PackTag_GetProcessVariable(enum_PROCESS_VARIABLE pv, float *value);
bool PackTag_SetProcessVariable(enum_PROCESS_VARIABLE pv, float value);

/* State and mode API */
bool PackTag_GetUnitMode(int32_t *mode);
bool PackTag_SetUnitMode(int32_t mode);
bool PackTag_GetCntrlCommand(int32_t *cmd);
bool PackTag_SetCntrlCommand(int32_t cmd);
bool PackTag_GetStateCurrent(int32_t *state);

/* CM7 sync -- call from CM7 polling task when global_flag[CM4_IDX] is set */
void PackTag_CM7_SyncFromCM4(void);

/* Admin API */
bool PackTag_GetProdConsumedCount(enum_LABEL_COUNT_TYPES idx, PackCount_t *count);
bool PackTag_GetProdProcessedCount(enum_GOOD_COUNT_TYPES idx, PackCount_t *count);
bool PackTag_GetProdDefectiveCount(enum_DEFECTIVE_COUNT_TYPES idx, PackCount_t *count);
bool PackTag_ResetAllCounts(void);
bool PackTag_GetAlarm(uint32_t idx, PackAlarm_t *alarm);
bool PackTag_GetStopReason(PackAlarm_t *alarm);

/* Additional tag API */
bool PackTag_SetRemoteStartButton(int32_t value);
bool PackTag_SetRemoteStopButton(int32_t value);
bool PackTag_SetInhibitLabeling(int32_t value);
bool PackTag_GetHmiSystemStatusBits(int32_t *bits);
bool PackTag_GetHmiPrintCycleStatusBits(int32_t *bits);
bool PackTag_GetHmiApplyCycleStatusBits(int32_t *bits);

#endif /* PACKTAG_H_ */