/*
**
** Copyright 2017, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraVendorUtils"
#include <log/log.h>

#include "ExynosCameraVendorUtils.h"
#include <utils/CallStack.h>

namespace android {

void setVendorMetaLuxStd(struct camera2_shot_ext *shot_ext, float value)
{
    shot_ext->shot.dm.aa.vendor_luxStandard = value;
}

float getVendorMetaLuxStd(struct camera2_shot_ext *shot_ext)
{
    float ret = 0.0f;
    ret = shot_ext->shot.dm.aa.vendor_luxStandard;
    return ret;
}

void setVendorMetaLuxIdx(struct camera2_shot_ext *shot_ext, float value)
{
    shot_ext->shot.dm.aa.vendor_luxIndex = (uint32_t)value;
}

float getVendorMetaLuxIdx(struct camera2_shot_ext *shot_ext)
{
    float ret = 0.0f;
    ret = (float)shot_ext->shot.dm.aa.vendor_luxIndex;
    return ret;
}

void setVendorMetaAnalogGain(struct camera2_shot_ext *shot_ext, float value)
{
    shot_ext->shot.udm.sensor.analogGain = (uint32_t)value;
}

float getVendorMetaAnalogGain(struct camera2_shot_ext *shot_ext)
{
    float ret = 0.0f;
    ret = shot_ext->shot.udm.sensor.analogGain;
    return ret;
}

void setVendorMetaDigitalGain(struct camera2_shot_ext *shot_ext, float value)
{
    shot_ext->shot.udm.sensor.digitalGain = (uint32_t)value;
}

float getVendorMetaDigitalGain(struct camera2_shot_ext *shot_ext)
{
    float ret = 0.0f;
    ret = shot_ext->shot.udm.sensor.digitalGain;
    return ret;
}

enum camera_thermal_mode getVendorMetaThermalLevel(struct camera2_shot_ext *shot_ext)
{
    enum camera_thermal_mode ret = CAM_THERMAL_NORMAL;
#ifndef DISABLE_THERMAL_META
    ret = shot_ext->thermal;
#endif
    return ret;
}

void setVendorMetaAwbCct(struct camera2_shot_ext *shot_ext, int32_t value)
{
    shot_ext->shot.dm.aa.vendor_colorTempKelvin = value;
}

int32_t getVendorMetaAwbCct(struct camera2_shot_ext *shot_ext)
{
    int32_t ret = 0;
    ret = (int32_t)shot_ext->shot.dm.aa.vendor_colorTempKelvin;
    return ret;
}

void setVendorMetaAwbDec(struct camera2_shot_ext *shot_ext, int32_t value)
{
    shot_ext->shot.dm.aa.vendor_colorTempIndex = (uint32_t)value;
}

int32_t getVendorMetaAwbDec(struct camera2_shot_ext *shot_ext)
{
    int32_t ret = 0;
    ret = (int32_t)shot_ext->shot.dm.aa.vendor_colorTempIndex;
    return ret;
}

int32_t getVendorMetaLensPos(struct camera2_shot_ext *shot_ext)
{
    int32_t ret = 0;
    ret = (int32_t)shot_ext->shot.udm.lens.pos;
    return ret;
}

int getVendorMetaAfdSubmode(struct camera2_shot_ext *shot_ext)
{
    int ret = 0;
    ret = (int32_t)shot_ext->shot.dm.aa.aeAntibandingMode;
    return ret;
}

int32_t getVendorMetaFlickerDetect(struct camera2_shot_ext *shot_ext)
{
    int32_t ret = 0;
    ret = (int32_t)shot_ext->shot.udm.flicker_detect;
    return ret;
}

void setVendorMetaFocusTargetPositionSupported(struct camera2_shot_ext *shot_ext, int value)
{
}

int32_t getVendorMetaFocusTargetPositionSupported(struct camera2_shot_ext *shot_ext)
{
    return 1;
}

void setVendorMetaFocusTargetPosition(struct camera2_shot_ext *shot_ext, int value)
{
#ifndef DISABLE_USER_META
    shot_ext->user.focus_target_pos = (int32_t)value;
#endif
}

int32_t getVendorMetaFocusTargetPosition(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return (int32_t)shot_ext->user.focus_target_pos;
#else
    return 0;
#endif
}

void setVendorMetaFocusActualPositionSupported(struct camera2_shot_ext *shot_ext, int value)
{
}

int32_t getVendorMetaFocusActualPositionSupported(struct camera2_shot_ext *shot_ext)
{
    return 1;
}

void setVendorMetaFocusActualPosition(struct camera2_shot_ext *shot_ext, int value)
{
#ifndef DISABLE_USER_META
    shot_ext->user.focus_actual_pos = (int32_t)value;
#endif
}

int32_t getVendorMetaFocusActualPosition(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return (int32_t)shot_ext->user.focus_actual_pos;
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusCount(struct camera2_shot_ext *shot_ext)
{
    return CAMERA_CRC_INDEX_MAX;
}

int32_t getVendorMetaCalibStatus(struct camera2_shot_ext *shot_ext, int index)
{
    if (index < 0 || CAMERA_CRC_INDEX_MAX <= index) {
        ALOGW("Invalid calibration index(%d). so, fail", index);
        return -1;
    }

#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[index];
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusMnf(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[CAMERA_CRC_INDEX_MNF];
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusDual(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[CAMERA_CRC_INDEX_DUAL];
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusPdaf(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[CAMERA_CRC_INDEX_PDAF];
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusAwb(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[CAMERA_CRC_INDEX_AWB];
#else
    return 0;
#endif
}

int32_t getVendorMetaCalibStatusAf(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_USER_META
    return shot_ext->user.crc_result[CAMERA_CRC_INDEX_AF];
#else
    return 0;
#endif
}

int32_t getVendorMetaEv(struct camera2_shot_ext *shot_ext)
{
    return shot_ext->shot.dm.aa.vendor_exposureValue;
}

uint32_t getVendorMetaVendorIsoValue(struct camera2_shot_ext *shot_ext)
{
    return shot_ext->shot.dm.aa.vendor_isoValue;
}

float getVendorMetaVendorObjectDistanceCm(struct camera2_shot_ext *shot_ext)
{
    return shot_ext->shot.dm.aa.vendor_objectDistanceCm;
}

int32_t getVendorMetaAecAecStatus(struct camera2_shot_ext *shot_ext)
{
    int32_t AecAecStatus = 0; // 1 : AECSettled  0 : AECUnSettled

    enum ae_state aeState = (enum ae_state)getMetaDmAeState(shot_ext);

    switch (aeState) {
    case AE_STATE_INACTIVE:
        AecAecStatus = 1;
        break;
    case AE_STATE_SEARCHING:
        AecAecStatus = 0;
        break;
    case AE_STATE_CONVERGED:
        AecAecStatus = 1;
        break;
    case AE_STATE_LOCKED:
        AecAecStatus = 1;
        break;
    case AE_STATE_FLASH_REQUIRED:
        AecAecStatus = 1;
        break;
    case AE_STATE_PRECAPTURE:
        AecAecStatus = 1;
        break;
    case AE_STATE_LOCKED_CONVERGED:
        AecAecStatus = 1;
        break;
    case AE_STATE_LOCKED_FLASH_REQUIRED:
        AecAecStatus = 1;
        break;
    case AE_STATE_SEARCHING_FLASH_REQUIRED:
        AecAecStatus = 0;
        break;
    default:
        AecAecStatus = 0;
        break;
    }

    return AecAecStatus;
}

int32_t getVendorMetaAecSettled(struct camera2_shot_ext *shot_ext)
{
    int32_t aecSettled = 0; // 1 : AECSettled  0 : AECUnSettled

    enum ae_state aeState = (enum ae_state)getMetaDmAeState(shot_ext);

    switch (aeState) {
    case AE_STATE_INACTIVE:
        aecSettled = 0;
        break;
    case AE_STATE_SEARCHING:
        aecSettled = 0;
        break;
    case AE_STATE_CONVERGED:
        aecSettled = 1;
        break;
    case AE_STATE_LOCKED:
        aecSettled = 1;
        break;
    case AE_STATE_FLASH_REQUIRED:
        aecSettled = 0;
        break;
    case AE_STATE_PRECAPTURE:
        aecSettled = 0;
        break;
    case AE_STATE_LOCKED_CONVERGED:
        aecSettled = 1;
        break;
    case AE_STATE_LOCKED_FLASH_REQUIRED:
        aecSettled = 1;
        break;
    case AE_STATE_SEARCHING_FLASH_REQUIRED:
        aecSettled = 0;
        break;
    default:
        aecSettled = 0;
        break;
    }

    return aecSettled;
}

int32_t getVendorMetaAfStatus(struct camera2_shot_ext *shot_ext)
{
    enum VENDOR_AF_STATUS {
        VENDOR_AF_STATUS_INVALID = -1,
        VENDOR_AF_STATUS_INIT = 0,
        VENDOR_AF_STATUS_FOCUSED,
        VENDOR_AF_STATUS_UNKNOWN,
        VENDOR_AF_STATUS_FOCUSING,
        VENDOR_AF_STATUS_CUSTOM,
    };

    // vendor af status
    enum VENDOR_AF_STATUS afStatus = VENDOR_AF_STATUS_INVALID;

    // exynos af status
    enum aa_afstate afState = (enum aa_afstate)getMetaDmAfState(shot_ext);

    switch (afState) {
    case AA_AFSTATE_INACTIVE:
        afStatus = VENDOR_AF_STATUS_INIT;
        break;
    case AA_AFSTATE_PASSIVE_SCAN:
        afStatus = VENDOR_AF_STATUS_FOCUSING;
        break;
    case AA_AFSTATE_PASSIVE_FOCUSED:
        afStatus = VENDOR_AF_STATUS_FOCUSED;
        break;
    case AA_AFSTATE_ACTIVE_SCAN:
        afStatus = VENDOR_AF_STATUS_FOCUSING;
        break;
    case AA_AFSTATE_FOCUSED_LOCKED:
        afStatus = VENDOR_AF_STATUS_FOCUSED;
        break;
    case AA_AFSTATE_NOT_FOCUSED_LOCKED:
        afStatus = VENDOR_AF_STATUS_UNKNOWN;
        break;
    case AA_AFSTATE_PASSIVE_UNFOCUSED:
        afStatus = VENDOR_AF_STATUS_UNKNOWN;
        break;
    default:
        afStatus = VENDOR_AF_STATUS_INIT;
        break;
    }

    return afStatus;
}

struct facial_score *getVendorMetaFacialScore(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_VRA_EXT_META
    return shot_ext->vra_ext.facialScore;
#else
    return 0;
#endif
}

struct facial_angle *getVendorMetaFacialAngle(struct camera2_shot_ext *shot_ext)
{
#ifndef DISABLE_VRA_EXT_META
    return shot_ext->vra_ext.facialAngle;
#else
    return 0;
#endif
}

int getVendorMetaNumOfDetectedFaces(struct camera2_shot_ext *shot_ext)
{
    int numOfDetectedFaces = 0;

    for (int i = 0; i < CAMERA2_MAX_FACES; i++) {
        if (0 < shot_ext->shot.dm.stats.faceIds[i]) {
            numOfDetectedFaces++;
        }
    }

    if (CAMERA2_MAX_FACES < numOfDetectedFaces) {
        ALOGW("Weird... CAMERA2_MAX_FACES(%d) < numOfDetectedFaces(%d). please check", CAMERA2_MAX_FACES, numOfDetectedFaces);
        numOfDetectedFaces = CAMERA2_MAX_FACES;
    }

    return numOfDetectedFaces;
}

int getLowlightMergeNum(void)
{
    return 3;
}

int getSuperNightShotBayerMergeNum(void)
{
    return 16;
}

int getHdrBayerMergeNum(void)
{
    return 7;
}

int getHdrYuvMergeNum(void)
{
    return 6;
}

int getFlashMultiFrameDenoiseYuvMergeNum(void)
{
    return 4;
}

int getBeautyFaceMergeNum(void)
{
    return 1;
}

int getSuperResolutionMergeNum(void)
{
    return 1;
}

int getOisDenoiseYuvMergeNum(void)
{
    return 6;
}

int getSportsYuvMergeNum(void)
{
    return 4;
}

int getBokehPrepareAnchorNum(void)
{
    return 6;
}

}; /* namespace android */
