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

#ifndef EXYNOS_CAMERA_VENDOR_UTILS_H
#define EXYNOS_CAMERA_VENDOR_UTILS_H

#include <cutils/properties.h>
#include <utils/threads.h>
#include <utils/String8.h>
#include <arm_neon.h>

#include "exynos_format.h"
#include "ExynosRect.h"

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSensorInfo.h"
#include "videodev2_exynos_media.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraUtils.h"

namespace android {

    void  setVendorMetaLuxStd(struct camera2_shot_ext *shot_ext, float value);
float getVendorMetaLuxStd(struct camera2_shot_ext *shot_ext);

void  setVendorMetaLuxIdx(struct camera2_shot_ext *shot_ext, float value);
float getVendorMetaLuxIdx(struct camera2_shot_ext *shot_ext);

void  setVendorMetaAnalogGain(struct camera2_shot_ext *shot_ext, float value);
float getVendorMetaAnalogGain(struct camera2_shot_ext *shot_ext);

void  setVendorMetaDigitalGain(struct camera2_shot_ext *shot_ext, float value);
float getVendorMetaDigitalGain(struct camera2_shot_ext *shot_ext);

enum camera_thermal_mode getVendorMetaThermalLevel(struct camera2_shot_ext *shot_ext);

void    setVendorMetaAwbCct(struct camera2_shot_ext *shot_ext, int32_t value);
int32_t getVendorMetaAwbCct(struct camera2_shot_ext *shot_ext);

void    setVendorMetaAwbDec(struct camera2_shot_ext *shot_ext, int32_t value);
int32_t getVendorMetaAwbDec(struct camera2_shot_ext *shot_ext);

int32_t getVendorMetaLensPos(struct camera2_shot_ext *shot_ext);
int   getVendorMetaAfdSubmode(struct camera2_shot_ext *shot_ext);

int32_t getVendorMetaFlickerDetect(struct camera2_shot_ext *shot_ext);

void    setVendorMetaFocusTargetPositionSupported(struct camera2_shot_ext *shot_ext, int value);
int32_t getVendorMetaFocusTargetPositionSupported(struct camera2_shot_ext *shot_ext);

void    setVendorMetaFocusTargetPosition(struct camera2_shot_ext *shot_ext, int value);
int32_t getVendorMetaFocusTargetPosition(struct camera2_shot_ext *shot_ext);

void    setVendorMetaFocusActualPositionSupported(struct camera2_shot_ext *shot_ext, int value);
int32_t getVendorMetaFocusActualPositionSupported(struct camera2_shot_ext *shot_ext);

void    setVendorMetaFocusActualPosition(struct camera2_shot_ext *shot_ext, int value);
int32_t getVendorMetaFocusActualPosition(struct camera2_shot_ext *shot_ext);

int32_t getVendorMetaCalibStatusCount(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaCalibStatus(struct camera2_shot_ext *shot_ext, int index);
int32_t getVendorMetaCalibStatusMnf(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaCalibStatusDual(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaCalibStatusPdaf(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaCalibStatusAwb(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaCalibStatusAf(struct camera2_shot_ext *shot_ext);

int32_t getVendorMetaEv(struct camera2_shot_ext *shot_ext);
uint32_t getVendorMetaVendorIsoValue(struct camera2_shot_ext *shot_ext);
float   getVendorMetaVendorObjectDistanceCm(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaAecAecStatus(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaAecSettled(struct camera2_shot_ext *shot_ext);
int32_t getVendorMetaAfStatus(struct camera2_shot_ext *shot_ext);

struct facial_score *getVendorMetaFacialScore(struct camera2_shot_ext *shot_ext);
struct facial_angle *getVendorMetaFacialAngle(struct camera2_shot_ext *shot_ext);
int   getVendorMetaNumOfDetectedFaces(struct camera2_shot_ext *shot_ext);

int   getLowlightMergeNum(void);
int   getSuperNightShotBayerMergeNum(void);
int   getHdrBayerMergeNum(void);
int   getHdrYuvMergeNum(void);
int   getFlashMultiFrameDenoiseYuvMergeNum(void);
int   getBeautyFaceMergeNum(void);
int   getSuperResolutionMergeNum(void);
int   getOisDenoiseYuvMergeNum(void);
int   getSportsYuvMergeNum(void);
int   getBokehPrepareAnchorNum(void);

}; /* namespace android */

#endif

