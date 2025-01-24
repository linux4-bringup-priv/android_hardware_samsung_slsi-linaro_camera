
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
#define LOG_TAG "ExynosCameraParameters"
#include <log/log.h>

#include "ExynosCameraParameters.h"
#include "ExynosCameraRequestManager.h"

namespace android {

ExynosCameraParameters::ExynosCameraParameters(cameraId_Info *camIdInfo,
                                               int camType,
                                               ExynosCameraConfigurations *configurations)
{
    m_camIdInfo = camIdInfo;
    m_camType = camType;
    m_cameraId = camIdInfo->cameraId[camType];
    m_scenario = camIdInfo->scenario;
    m_configurations = configurations;
    m_configurations->setParameters(m_cameraId, this);

    if (getCamName(m_cameraId, m_name, sizeof(m_name)) != NO_ERROR) {
        memset(m_name, 0x00, sizeof(m_name));
        CLOGE("Invalid camera ID(%d)", m_cameraId);
    }

    m_staticInfo = createExynosCameraSensorInfo(m_cameraId, camIdInfo->serviceCameraId);
    m_useSizeTable = (m_staticInfo->sizeTableSupport) ? USE_CAMERA_SIZE_TABLE : false;

    memset(&m_cameraInfo, 0, sizeof(struct exynos_camera_info));
    memset(&m_exifInfo, 0, sizeof(m_exifInfo));
    memset(&m_metadata, 0, sizeof(m_metadata));

    m_activityControl = new ExynosCameraActivityControl(m_cameraId, m_camIdInfo, m_staticInfo);
    ExynosCameraActivityUCTL* uctlMgr = m_activityControl->getUCTLMgr();
    uctlMgr->setMetadata(&m_metadata);

    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START

    m_LLSOn = false;
    m_videoStreamExist = false;

    m_useSensorPackedBayer = false;
    m_usePureBayerReprocessing = (isBackCamera(m_cameraId)) ? USE_PURE_BAYER_REPROCESSING : USE_PURE_BAYER_REPROCESSING_FRONT;
    m_isUniqueIdRead = false;
    m_fastenAeStableOn = false;
    m_isLogicalCam = isLogicalCam(camIdInfo->serviceCameraId);

    memset(m_width, 0, sizeof(m_width));
    memset(m_height, 0, sizeof(m_height));
    if (HW_INFO_MODE_MAX > 0)
        memset(m_mode, 0, sizeof(m_mode));
    memset(m_hwYuvWidth, 0, sizeof(m_hwYuvWidth));
    memset(m_hwYuvHeight, 0, sizeof(m_hwYuvHeight));
    m_hwYuvInputWidth = 0;
    m_hwYuvInputHeight = 0;

    m_previewDsInputPortId = MCSC_PORT_NONE;
    m_captureDsInputPortId = MCSC_PORT_NONE;

    vendorSpecificConstructor(m_cameraId);

    m_brightnessValue = 0;
    m_hifillsOn = false;

#ifdef USE_DUAL_CAMERA
    m_standbyState = DUAL_STANDBY_STATE_ON; /* default */
    initDisplayStatRoi();
#endif

    m_setfile = 0;
    m_yuvRange = 0;
    m_setfileReprocessing = 0;
    m_yuvRangeReprocessing = 0;
    //m_firing_flash_marking = 0;

    m_previewPortId = -1;
    m_recordingPortId = -1;
//    m_yuvStallPort = -1;

    m_bayerFrameLock = false;
    m_oldBayerFrameLockCount = 0;
    m_newBayerFrameLockCount = 0;

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    m_sensorModeTransition = false;
    m_sensorModeTransitionFrameCount = SENSOR_MOODE_TRANSITION_FRAME_COUNT;
#endif //SUPPORT_SENSOR_MODE_CHANGE

    // Initial Values : END
    setDefaultCameraInfo();

    m_setExifFixedAttribute();
}

ExynosCameraParameters::~ExynosCameraParameters()
{
    if (m_staticInfo != NULL) {
        delete m_staticInfo;
        m_staticInfo = NULL;
    }

    if (m_activityControl != NULL) {
        delete m_activityControl;
        m_activityControl = NULL;
    }

    if (m_exifInfo.maker_note) {
        delete[] m_exifInfo.maker_note;
        m_exifInfo.maker_note = NULL;
    }

    if (m_exifInfo.user_comment) {
        delete[] m_exifInfo.user_comment;
        m_exifInfo.user_comment = NULL;
    }

    m_vendorSpecificDestructor();
}

void ExynosCameraParameters::setDefaultCameraInfo(void)
{
    CLOGI("");

    for (int i = 0; i < this->getYuvStreamMaxNum(); i++) {
        /* YUV */
        m_configurations->setSize(CONFIGURATION_YUV_SIZE, m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH, i);

        /* YUV_STALL */
        m_configurations->setSize(CONFIGURATION_YUV_SIZE, m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH, i + YUV_MAX);
    }

    setSize(HW_INFO_HW_SENSOR_SIZE, m_staticInfo->maxSensorW, m_staticInfo->maxSensorH);
    setSize(HW_INFO_HW_MAX_PICTURE_SIZE, m_staticInfo->maxPictureW, m_staticInfo->maxPictureH);
    setSize(HW_INFO_HW_PICTURE_SIZE, m_staticInfo->maxPictureW, m_staticInfo->maxPictureH);
    m_setHwPreviewFormat(PREVIEW_OUTPUT_COLOR_FMT);
    m_setHwPictureFormat(SCC_OUTPUT_COLOR_FMT);
    m_setHwPicturePixelSize(CAMERA_PIXEL_SIZE_8BIT);
    m_configurations->setSize(CONFIGURATION_PICTURE_SIZE, m_staticInfo->maxPictureW, m_staticInfo->maxPictureH);
    m_configurations->setSize(CONFIGURATION_THUMBNAIL_SIZE, m_staticInfo->maxThumbnailW, m_staticInfo->maxThumbnailH);
}

uint32_t ExynosCameraParameters::getSensorStandbyDelay(void)
{
#ifdef SENSOR_STANDBY_DELAY
    return SENSOR_STANDBY_DELAY;
#else
    return 2;
#endif
}

bool ExynosCameraParameters::getHWVdisMode(void)
{
    bool ret = m_configurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE);

    /*
     * Only true case,
     * we will test whether support or not.
     */
    if (ret == true) {
        if (isBackCamera(getCameraId())) {
#ifdef SUPPORT_BACK_HW_VDIS
            ret = SUPPORT_BACK_HW_VDIS;
#else
            ret = false;
#endif
        } else if (isFrontCamera(getCameraId())) {
#ifdef SUPPORT_FRONT_HW_VDIS
            ret = SUPPORT_FRONT_HW_VDIS;
#else
            ret = false;
#endif
        } else {
            ret = false;
        }
    }

    m_vendorSWVdisMode(&ret);

    return ret;
}

int ExynosCameraParameters::getHWVdisFormat(void)
{
    return V4L2_PIX_FMT_YUYV;
}

int ExynosCameraParameters::getHW3AFdFormat(void)
{
#if defined(CAMERA_VRA_INPUT_FORMAT)
    return CAMERA_VRA_INPUT_FORMAT;
#else
    return CAMERA_3AF_OUTPUT_FORMAT;
#endif
}

#ifdef SUPPORT_ME
int ExynosCameraParameters::getMeFormat(void)
{
#ifdef CAMERA_ME_FORMAT
    return CAMERA_ME_FORMAT;
#else
    return V4L2_PIX_FMT_Y12;
#endif
}

void ExynosCameraParameters::getMeSize(int *meWidth, int *meHeight)
{
    *meWidth = CAMERA_ME_WIDTH;
    *meHeight = CAMERA_ME_HEIGHT;
}

int ExynosCameraParameters::getLeaderPipeOfMe()
{
#ifndef SUPPORT_ME
    CLOGE("Not supported ME");
    return -1;
#endif

#ifdef LEADER_PIPE_OF_ME
    return LEADER_PIPE_OF_ME;
#else
    CLOGE("Not defined LEADER_PIPE_OF_ME");
    return -1;
#endif
}
#endif //SUPPORT_ME

void ExynosCameraParameters::getHw3aaVraInputSize(int dsInW, int dsInH, ExynosRect *dsOutRect)
{
    status_t ret = NO_ERROR;
    int vraWidth = MAX_VRA_INPUT_WIDTH;
    int vraHeight = MAX_VRA_INPUT_HEIGHT;

    if (dsInW <= vraWidth && dsInH <= vraHeight) {
        dsOutRect->x = 0;
        dsOutRect->y = 0;
        dsOutRect->w = dsInW;
        dsOutRect->h = dsInH;
    } else {
        ret = getCropRectAlign(
               vraWidth, vraHeight, dsInW, dsInH,
               &dsOutRect->x, &dsOutRect->y, &dsOutRect->w, &dsOutRect->h,
               CAMERA_3AA_DS_ALIGN_W, 1, 1.0f);
        if (ret != NO_ERROR) {
           CLOGE("Failed to getCropRectAlign. %dx%d -> %dx%d ret %d",
                   vraWidth, vraHeight, dsInW, dsInH, ret);

            dsOutRect->x = 0;
            dsOutRect->y = 0;
            dsOutRect->w = dsInW;
            dsOutRect->h = dsInH;
            return;
        }
    }

    CLOGV("dsInW %d dsInH %d  dsOutRect[%d %d %d %d]",
           dsInW, dsInH, dsOutRect->x, dsOutRect->y, dsOutRect->w, dsOutRect->h);
    return;
}

void ExynosCameraParameters::getHwVraInputSize(int *w, int *h, camera2_shot* shot, mcsc_port dsInputPortId)
{
    status_t ret = NO_ERROR;
    int vraWidth = MAX_VRA_INPUT_WIDTH;
    int vraHeight = MAX_VRA_INPUT_HEIGHT;
    int dsInputWidth = 0;
    int dsInputHeight = 0;
    ExynosRect dummyIspSize;
    ExynosRect dsOutputSize;
    int adjustDsInputPortId = m_adjustDsInputPortId((dsInputPortId > MCSC_PORT_NONE) ? dsInputPortId :
                                                        shot->uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS]);

    switch (adjustDsInputPortId) {
    case MCSC_PORT_0:
    case MCSC_PORT_1:
    case MCSC_PORT_2:
        getYuvVendorSize(&dsInputWidth, &dsInputHeight, adjustDsInputPortId, dummyIspSize);
        break;
    case MCSC_PORT_3:
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&dsInputWidth, (uint32_t *)&dsInputHeight);
        break;
    case MCSC_PORT_4:
        if (shot->ctl.jpeg.thumbnailSize[0] <= 0 || shot->ctl.jpeg.thumbnailSize[1] <= 0) {
            m_configurations->getSize(CONFIGURATION_THUMBNAIL_SIZE, (uint32_t *)&dsInputWidth, (uint32_t *)&dsInputHeight);
        } else {
            dsInputWidth = shot->ctl.jpeg.thumbnailSize[0];
            dsInputHeight = shot->ctl.jpeg.thumbnailSize[1];
        }
        break;
    default:
        CLOGE("Unsupported dsInputPortId %d", adjustDsInputPortId);
        *w = vraWidth;
        *h = vraHeight;
        return;
    }

    if (dsInputWidth < vraWidth && dsInputHeight < vraHeight) {
        dsOutputSize.w = dsInputWidth;
        dsOutputSize.h = dsInputHeight;
    } else {
        ret = getCropRectAlign(
                vraWidth, vraHeight, dsInputWidth, dsInputHeight,
                &dsOutputSize.x, &dsOutputSize.y, &dsOutputSize.w, &dsOutputSize.h,
                2, 2, 1.0f);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getCropRectAlign. %dx%d -> %dx%d ret %d",
                    vraWidth, vraHeight, dsInputWidth, dsInputHeight, ret);

            *w = vraWidth;
            *h = vraHeight;
            return;
        }
    }

    *w = dsOutputSize.w;
    *h = dsOutputSize.h;

    CLOGV("dsInputPortId %d DSsize %dx%d->%dx%d",
            adjustDsInputPortId, dsInputWidth, dsInputHeight, *w, *h);
    return;
}

int ExynosCameraParameters::getHwVraInputFormat(void)
{
#if defined(CAMERA_VRA_INPUT_FORMAT)
    return CAMERA_VRA_INPUT_FORMAT;
#else
    return V4L2_PIX_FMT_NV16;
#endif
}

void ExynosCameraParameters::setDsInputPortId(int dsInputPortId, bool isReprocessing)
{
    if (isReprocessing == false) {
        m_previewDsInputPortId = dsInputPortId;
    } else {
        m_captureDsInputPortId = dsInputPortId;
    }
}

int ExynosCameraParameters::getDsInputPortId(bool isReprocessing)
{
    int dsInputPortId = MCSC_PORT_NONE;

    if (isReprocessing == false) {
        dsInputPortId = m_previewDsInputPortId;
    } else {
        dsInputPortId = m_captureDsInputPortId;
    }

    return dsInputPortId;
}

void ExynosCameraParameters::setYsumPordId(int ysumPortId, struct camera2_shot_ext *shot_ext)
{
    if (m_configurations->getMode(CONFIGURATION_YSUM_RECORDING_MODE) == true) {
        m_ysumPortId = ysumPortId;
        setMetaUctlYsumPort(shot_ext, (enum mcsc_port)ysumPortId);
    }
    CLOGV("ysumPortId(%d)", m_ysumPortId);
}

int ExynosCameraParameters::getYsumPordId(void)
{
    if (m_configurations->getMode(CONFIGURATION_YSUM_RECORDING_MODE) == false) {
        return -1;
    }

    return m_ysumPortId;
}

int ExynosCameraParameters::getYuvSizeRatioId(void)
{
    return m_cameraInfo.yuvSizeRatioId;
}

#ifdef HAL3_YUVSIZE_BASED_BDS
/*
   Make the all YUV output size as smallest preview size.
   Format will be set to NV21
*/
status_t ExynosCameraParameters::initYuvSizes() {
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];
    size_t lastIdx = 0;

    lastIdx = m_staticInfo->yuvListMax - 1;
    sizeList = m_staticInfo->yuvList;

    for(int outputPort = 0; outputPort < getYuvStreamMaxNum(); outputPort++) {
        CLOGV("Port[%d] Idx[%d], Size(%d x %d) / true"
            , outputPort, lastIdx, sizeList[lastIdx][0], sizeList[lastIdx][1]);
        /* YUV */
        m_configurations->setSize(CONFIGURATION_YUV_SIZE, sizeList[lastIdx][0], sizeList[lastIdx][1], outputPort);
        m_configurations->setYuvFormat(V4L2_PIX_FMT_NV21, outputPort);
        m_configurations->setYuvPixelSize(CAMERA_PIXEL_SIZE_8BIT, outputPort);

        /* YUV_STALL */
        m_configurations->setSize(CONFIGURATION_YUV_SIZE, sizeList[lastIdx][0], sizeList[lastIdx][1], outputPort, YUV_MAX);
        m_configurations->setYuvFormat(V4L2_PIX_FMT_NV21, outputPort + YUV_MAX);
        m_configurations->setYuvPixelSize(CAMERA_PIXEL_SIZE_8BIT, outputPort + YUV_MAX);
    }

    return NO_ERROR;
}
#endif

status_t ExynosCameraParameters::resetYuvSizeRatioId(void)
{
    m_cameraInfo.yuvSizeRatioId = m_staticInfo->sensorArrayRatio;

    return NO_ERROR;
}

bool ExynosCameraParameters::m_isSupportedYuvSize(const int width,
                                                   const int height,
                                                   __unused const int outputPortId,
                                                   int *ratio)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    getSize(HW_INFO_MAX_PREVIEW_SIZE, (uint32_t *)&maxWidth, (uint32_t *)&maxHeight);

    if (maxWidth*maxHeight < width*height) {
        CLOGE("invalid PreviewSize(maxSize(%d/%d) size(%d/%d)", maxWidth, maxHeight, width, height);
        return false;
    }

    sizeList = m_staticInfo->yuvList;
    for (int i = 0; i < m_staticInfo->yuvListMax; i++) {
        if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
            continue;

        if (sizeList[i][0] == width && sizeList[i][1] == height) {
            *ratio = sizeList[i][3];
            return true;
        }
    }

    if (m_staticInfo->hiddenPreviewList != NULL) {
        sizeList = m_staticInfo->hiddenPreviewList;
        for (int i = 0; i < m_staticInfo->hiddenPreviewListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;

            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                *ratio = sizeList[i][3];
                return true;
            }
        }
    }

    CLOGE("Invalid preview size(%dx%d)", width, height);

    return false;
}

#ifdef USE_BINNING_MODE
int *ExynosCameraParameters::getBinningSizeTable(void) {
    int *sizeList = NULL;
    int index = 0;

    if (m_staticInfo->vtcallSizeLut == NULL
        || m_staticInfo->vtcallSizeLutMax == 0) {
        CLOGE("vtcallSizeLut is NULL");
        return sizeList;
    }

    for (index = 0; index < m_staticInfo->vtcallSizeLutMax; index++) {
        if (m_staticInfo->vtcallSizeLut[index][0] == m_cameraInfo.yuvSizeRatioId)
        break;
    }

    if (m_staticInfo->vtcallSizeLutMax <= index)
        index = 0;

    sizeList = m_staticInfo->vtcallSizeLut[index];

    return sizeList;
}
#endif

void ExynosCameraParameters::setPreviewPortId(int outputPortId)
{
    m_previewPortId = outputPortId;
}

bool ExynosCameraParameters::isPreviewPortId(int outputPortId)
{
    bool result = false;

    if (m_previewPortId >= YUV_0 && m_previewPortId < YUV_MAX
        && outputPortId == m_previewPortId)
        result = true;
    else
        result = false;

    return result;
}

int ExynosCameraParameters::getPreviewPortId(void)
{
    return m_previewPortId;
}

void ExynosCameraParameters::setPreviewCbPortId(int outputPortId)
{
    m_previewCbPortId = outputPortId;
}

bool ExynosCameraParameters::isPreviewCbPortId(int outputPortId)
{
    bool result = false;

    if (outputPortId >= YUV_0 && outputPortId < YUV_MAX && outputPortId == m_previewCbPortId)
        result = true;
    else
        result = false;

    return result;
}

int ExynosCameraParameters::getPreviewCbPortId(void)
{
    return m_previewCbPortId;
}

void ExynosCameraParameters::setRecordingPortId(int outputPortId)
{
    ExynosCameraActivityFlash *flashMgr = m_activityControl->getFlashMgr();

    m_recordingPortId = outputPortId;

    m_configurations->setMode(CONFIGURATION_RECORDING_MODE, true);

    flashMgr->setRecordingHint(true);
}

bool ExynosCameraParameters::isRecordingPortId(int outputPortId)
{
    bool result = false;

    if (m_recordingPortId < 0 || outputPortId < 0)
        return false;

    if (outputPortId == m_recordingPortId)
        result = true;
    else
        result = false;

    return result;
}

int ExynosCameraParameters::getRecordingPortId(void)
{
    return m_recordingPortId;
}

void ExynosCameraParameters::setYuvOutPortId(enum pipeline pipeId, int outputPortId)
{
    if (pipeId >= sizeof(m_yuvOutPortId) / sizeof(m_yuvOutPortId[0])) {
        CLOGE("Invalid pipeId %d", pipeId);
        return;
    }

    m_yuvOutPortId[pipeId] = outputPortId;
}

int ExynosCameraParameters::getYuvOutPortId(enum pipeline pipeId)
{
    if (pipeId >= sizeof(m_yuvOutPortId) / sizeof(m_yuvOutPortId[0])) {
        CLOGE("Invalid pipeId %d", pipeId);
        return -1;
    }

    return m_yuvOutPortId[pipeId];
}

void ExynosCameraParameters::m_adjustSensorMargin(int *sensorMarginW, int *sensorMarginH)
{
    float bnsRatio = 1.00f;
    float binningRatio = 1.00f;
    float sensorMarginRatio = 1.00f;

    binningRatio = (float)m_configurations->getModeValue(CONFIGURATION_BINNING_RATIO) / 1000.00f;
    sensorMarginRatio = bnsRatio * binningRatio;
    if ((int)sensorMarginRatio < 1) {
        CLOGW("Invalid sensor margin ratio(%f), bnsRatio(%f), binningRatio(%f)",
                 sensorMarginRatio, bnsRatio, binningRatio);
        sensorMarginRatio = 1.00f;
    }

    int leftMargin = 0, rightMargin = 0, topMargin = 0, bottomMargin = 0;

    rightMargin = ALIGN_DOWN((int)(m_staticInfo->sensorMarginBase[WIDTH_BASE] / sensorMarginRatio), 2);
    leftMargin = m_staticInfo->sensorMarginBase[LEFT_BASE] + rightMargin;
    bottomMargin = ALIGN_DOWN((int)(m_staticInfo->sensorMarginBase[HEIGHT_BASE] / sensorMarginRatio), 2);
    topMargin = m_staticInfo->sensorMarginBase[TOP_BASE] + bottomMargin;

    *sensorMarginW = leftMargin + rightMargin;
    *sensorMarginH = topMargin + bottomMargin;
}

camera_pixel_comp_info  ExynosCameraParameters::getPixelCompInfo(int pipeID)
{
    camera_pixel_comp_info pixelCompInfo = NO_COMP;

#ifdef USE_SBWC
    ExynosRect bdsRect;

    getPreviewBdsSize(&bdsRect, false);
    if (bdsRect.h > SBWC_MAX_HEIGHT) {
        return NO_COMP;
    }

    uint32_t fps = m_configurations->getModeValue(CONFIGURATION_RECORDING_FPS);
    if (fps == 120 || fps == 240) {
        CLOGD("SBWC is not supported in HIGH_SPEED_MODE(%d)", fps);
        return NO_COMP;
    }

    switch (pipeID) {
        case PIPE_3AP:
        case PIPE_ISP:
#ifdef SBWC_COMP_TYPE
            pixelCompInfo = SBWC_COMP_TYPE;
#else
            pixelCompInfo = COMP;
#endif
            break;

        default:
            break;
    }
#endif

    return pixelCompInfo;
}


int ExynosCameraParameters::getBayerFormat(int pipeId)
{
    int bayerFormat = V4L2_PIX_FMT_SBGGR16;

    switch (pipeId) {
    case PIPE_FLITE:
    case PIPE_VC0:
    case PIPE_3AA:
    case PIPE_FLITE_REPROCESSING:
    case PIPE_3AA_REPROCESSING:
#ifdef SUPPORT_SESSION_PARAMETERS
    if (m_configurations->getModeMultiValue(CONFIGURATION_MULTI_SESSION_MODE_VALUE, EXYNOS_SESSION_MODE_LED_CAL) == true) {
        bayerFormat = CAMERA_FLITE_BAYER_FORMAT_LED_CAL;
    } else
#endif
    {
        bayerFormat = CAMERA_FLITE_BAYER_FORMAT;
    }
        break;
    case PIPE_3AP:
    case PIPE_ISP:
        bayerFormat = CAMERA_3AP_BAYER_FORMAT;
        break;
    case PIPE_3AP_REPROCESSING:
        bayerFormat = CAMERA_3AP_REPROCESSING_BAYER_FORMAT;
        break;
    case PIPE_ISP_REPROCESSING:
        if (getUsePureBayerReprocessing() == true) {
            bayerFormat = CAMERA_3AP_REPROCESSING_BAYER_FORMAT;
        } else {
            bayerFormat = CAMERA_3AC_BAYER_FORMAT;
        }
        break;
    case PIPE_3AC:
        bayerFormat = CAMERA_3AC_BAYER_FORMAT;
        break;
    case PIPE_3AC_REPROCESSING:
        /* only RAW(DNG) format */
        if (isUseRawReverseReprocessing() == true) {
            bayerFormat = CAMERA_3AC_REPROCESSING_BAYER_FORMAT;
        } else {
            bayerFormat = (m_useSensorPackedBayer == true) ? CAMERA_3AC_REPROCESSING_BAYER_FORMAT : V4L2_PIX_FMT_SBGGR16;
        }
        break;
    case PIPE_3AG_REPROCESSING:
        bayerFormat = CAMERA_3AG_REPROCESSING_BAYER_FORMAT;
        break;
    default:
        CLOGW("Invalid pipeId(%d)", pipeId);
        break;
    }

    return bayerFormat;
}

void ExynosCameraParameters::m_setHwPreviewFormat(int colorFormat)
{
    m_cameraInfo.hwPreviewFormat = colorFormat;
}

int ExynosCameraParameters::getHwPreviewFormat(void)
{
    CLOGV("m_cameraInfo.hwPreviewFormat(%d)", m_cameraInfo.hwPreviewFormat);

    return m_cameraInfo.hwPreviewFormat;
}

void ExynosCameraParameters::updateHwSensorSize(void)
{
    int curHwSensorW = 0;
    int curHwSensorH = 0;
    int newHwSensorW = 0;
    int newHwSensorH = 0;
    int maxHwSensorW = 0;
    int maxHwSensorH = 0;

    getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxHwSensorW, (uint32_t *)&maxHwSensorH);
    getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&newHwSensorW, (uint32_t *)&newHwSensorH);

    if (newHwSensorW > maxHwSensorW || newHwSensorH > maxHwSensorH) {
        CLOGE("Invalid sensor size (maxSize(%d/%d) size(%d/%d)",
            maxHwSensorW, maxHwSensorH, newHwSensorW, newHwSensorH);
    }

    if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE) == true) {
#if 0
        int sizeList[SIZE_LUT_INDEX_END];
        m_getHighSpeedRecordingSize(sizeList);
        newHwSensorW = sizeList[SENSOR_W];
        newHwSensorH = sizeList[SENSOR_H];
#endif
    } else {
        getSize(HW_INFO_HW_BNS_SIZE, (uint32_t *)&newHwSensorW, (uint32_t *)&newHwSensorH);
    }

    getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&curHwSensorW, (uint32_t *)&curHwSensorH);
    CLOGI("curHwSensor size(%dx%d) newHwSensor size(%dx%d)", curHwSensorW, curHwSensorH, newHwSensorW, newHwSensorH);
    if (curHwSensorW != newHwSensorW || curHwSensorH != newHwSensorH) {
        setSize(HW_INFO_HW_SENSOR_SIZE, newHwSensorW, newHwSensorH);
        CLOGI("newHwSensor size(%dx%d)", newHwSensorW, newHwSensorH);
    }
}

void ExynosCameraParameters::updateBinningScaleRatio(void)
{
    int ret = 0;
    int32_t binningRatio = DEFAULT_BINNING_RATIO * 1000;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true
        && m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE) == true) {
        int configMode = m_configurations->getConfigMode();
        switch (configMode) {
        case CONFIG_MODE::HIGHSPEED_120:
        case CONFIG_MODE::HIGHSPEED_240:
            binningRatio = 2000;
            break;
        case CONFIG_MODE::HIGHSPEED_480:
            binningRatio = 4000;
            break;
        default:
            CLOGE("Invalide configMode(%d)", configMode);
        }
    }
#ifdef USE_BINNING_MODE
    else if (getBinningMode() == true) {
        binningRatio = 2000;
    }
#endif

    if (binningRatio != m_configurations->getModeValue(CONFIGURATION_BINNING_RATIO)) {
        CLOGI("New sensor binning ratio(%d)", binningRatio);
        ret = m_configurations->setModeValue(CONFIGURATION_BINNING_RATIO, binningRatio);
    }

    if (ret < 0)
        CLOGE(" Cannot update BNS scale ratio(%d)", binningRatio);
}

int ExynosCameraParameters::getBatchSize(__unused enum pipeline pipeId)
{
#ifndef SUPPORT_HFR_BATCH_MODE
    UNUSED_VARIABLE(pipeId);
#endif

    int batchSize = 1;
#ifdef SUPPORT_HFR_BATCH_MODE
    uint32_t minFps = 0, maxFps = 0;
    int yuvPortId = -1;
    ExynosConfigInfo *config = m_configurations->getConfig();
    m_configurations->getPreviewFpsRange(&minFps, &maxFps);

    /*
     * Default batchSize is MAX(1, maxFps/MULTI_BUFFER_BASE_FPS).
     * If specific pipe has different batchSize,
     * add case with pipeId.
     */
    switch (pipeId) {
    case PIPE_MCSC0:
    case PIPE_MCSC1:
    case PIPE_MCSC2:
        yuvPortId = pipeId - PIPE_MCSC0;
        if (this->useServiceBatchMode() == true) {
            batchSize = 1;
            break;
        } else if (this->isPreviewPortId(yuvPortId) == true) {
            /* Preview stream buffer is not delivered through every request */
            batchSize = 1;
            break;
        }
    default:
        batchSize = config->current->bufInfo.num_batch_buffers;
        break;
    }

    if (pipeId >= PIPE_FLITE_REPROCESSING) {
        /* Reprocessing stream always uses single buffer scheme */
        batchSize = 1;
    }
#endif

    return batchSize;
}

bool ExynosCameraParameters::useServiceBatchMode(void)
{
#ifdef USE_SERVICE_BATCH_MODE
    return true;
#else
    return false;
#endif
}

bool ExynosCameraParameters::isDebugInfoPlane(__unused enum pipeline pipeId, int *planeCount)
{
    bool flag = false;

    switch (pipeId) {
    //case PIPE_3AP:
    //case PIPE_ISP:
    case PIPE_3AC:
        if (getUsePureBayerReprocessing())
            flag = false;
        else
            flag = true;
        break;
    case PIPE_3AP_REPROCESSING:
    case PIPE_ISP_REPROCESSING:
        if (getUsePureBayerReprocessing())
            flag = true;
        else
            flag = false;
        break;
    default:
        flag = false;
        break;
    }

    if (flag == true)
        *planeCount +=1;

    return flag;
}

bool ExynosCameraParameters::isCriticalSection(enum pipeline pipeId, enum critical_section_type type)
{
    bool flag = false;

    switch (type) {
    case CRITICAL_SECTION_TYPE_HWFC:
        if (m_getReprocessing3aaIspOtf() != HW_CONNECTION_MODE_M2M
            && pipeId == PIPE_3AA_REPROCESSING) {
            flag = true;
        } else if (pipeId == PIPE_ISP_REPROCESSING) {
            flag = true;
        } else if (pipeId == PIPE_HWFC_JPEG_DST_REPROCESSING) {
            flag = true;
        }
        break;
    case CRITICAL_SECTION_TYPE_VOTF:
        break;
    default:
        break;
    }

    return flag;
}

status_t ExynosCameraParameters::checkPictureSize(int pictureW, int pictureH, bool isSameSensorSize)
{
    int curPictureW = 0;
    int curPictureH = 0;
    int curHwPictureW = 0;
    int curHwPictureH = 0;
    int newHwPictureW = 0;
    int newHwPictureH = 0;
    bool forceChangeHwPictureSize = false;

    if (pictureW < 0 || pictureH < 0) {
        return BAD_VALUE;
    }

    if (m_adjustPictureSize(&pictureW, &pictureH, &newHwPictureW, &newHwPictureH) != NO_ERROR) {
        return BAD_VALUE;
    }

    int camIdx = m_getSensorInfoCamIdx();
    bool supported = m_isSupportedPictureSize(pictureW, pictureH);
    if (camIdx == m_camType && supported == false) {
        int maxHwPictureW =0;
        int maxHwPictureH = 0;

        CLOGE("Invalid picture size(%dx%d)", pictureW, pictureH);

        /* prevent wrong size setting */
        getSize(HW_INFO_MAX_PREVIEW_SIZE, (uint32_t *)&maxHwPictureW, (uint32_t *)&maxHwPictureH);
        m_configurations->setSize(CONFIGURATION_PICTURE_SIZE, maxHwPictureW, maxHwPictureH);
        setSize(HW_INFO_HW_MAX_PICTURE_SIZE, maxHwPictureW, maxHwPictureH);

        CLOGE("changed picture size to MAX(%dx%d)", maxHwPictureW, maxHwPictureH);

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
        return INVALID_OPERATION;
    }
    CLOGI("[setParameters]newPicture Size (%dx%d), ratioId(%d)",
        pictureW, pictureH, m_cameraInfo.pictureSizeRatioId);

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_staticInfo->useSensorCrop[m_scenario][m_camType] == true
        && m_staticInfo->pictureCropFullSizeLut != NULL) {
        forceChangeHwPictureSize = true;
    }
#endif

    m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&curPictureW, (uint32_t *)&curPictureH);
    getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&curHwPictureW, (uint32_t *)&curHwPictureH);

    CLOGI("camType(%d) cur PictureSize(%dx%d) new PictureSize(%dx%d)",
                m_camType, curPictureW, curPictureH, pictureW, pictureH);

    CLOGI("camType(%d) cur HwPictureSize(%dx%d) new HwPictureSize(%dx%d), forceHwPictureSize(%d)",
                m_camType, curHwPictureW, curHwPictureH, newHwPictureW, newHwPictureH, forceChangeHwPictureSize);

    if (curPictureW != pictureW || curPictureH != pictureH ||
        curHwPictureW != newHwPictureW || curHwPictureH != newHwPictureH
        || forceChangeHwPictureSize == true) {

        CLOGI("[setParameters]Picture size changed: cur(%dx%d) -> new(%dx%d)",
                curPictureW, curPictureH, pictureW, pictureH);
        CLOGI("[setParameters]HwPicture size changed: cur(%dx%d) -> new(%dx%d)",
                curHwPictureW, curHwPictureH, newHwPictureW, newHwPictureH);

        if ((m_camType == camIdx) || (curPictureW <= 0 && curPictureH <= 0)) {
            m_configurations->setSize(CONFIGURATION_PICTURE_SIZE, pictureW, pictureH);
        }
        setSize(HW_INFO_HW_MAX_PICTURE_SIZE, newHwPictureW, newHwPictureH);

        switch (m_scenario) {
        case SCENARIO_DUAL_REAR_PORTRAIT:
            if (m_camType == camIdx)
                isSameSensorSize = true;
            //fallthrough
        case SCENARIO_DUAL_REAR_ZOOM:
            if (isSameSensorSize == true) {
                setSize(HW_INFO_HW_PICTURE_SIZE, pictureW, pictureH);
            } else {
                int sizeList[SIZE_LUT_INDEX_END];
                if (m_getPictureSizeList(sizeList) != NO_ERROR) {
                    CLOGE("Fail m_getPictureSizeList()!!");
                } else {
                    CLOGI("[setParameters] Camera Pictursize (%d %d)",
                        sizeList[TARGET_W], sizeList[TARGET_H]);
                    setSize(HW_INFO_HW_PICTURE_SIZE, sizeList[TARGET_W], sizeList[TARGET_H]);
                }
            }
            break;
        default:
            setSize(HW_INFO_HW_PICTURE_SIZE, pictureW, pictureH);
            break;
        }

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
    }

    return NO_ERROR;
}

status_t ExynosCameraParameters::m_adjustPictureSize(int *newPictureW, int *newPictureH,
                                                 int *newHwPictureW, int *newHwPictureH)
{
    int ret = 0;
    int newX = 0, newY = 0, newW = 0, newH = 0;
    float zoomRatio = 1.0f;

    if ((m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true
        && m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE) == true
        && m_configurations->getConfigMode() != CONFIG_MODE::HIGHSPEED_60)
#ifdef USE_BINNING_MODE
        || getBinningMode()
#endif
        ) {
        int sizeList[SIZE_LUT_INDEX_END];
        if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
            *newPictureW = sizeList[TARGET_W];
            *newPictureH = sizeList[TARGET_H];
            *newHwPictureW = *newPictureW;
            *newHwPictureH = *newPictureH;

            return NO_ERROR;
        } else {
            CLOGE("m_getPreviewSizeList() fail");
            return BAD_VALUE;
        }
    }

    getSize(HW_INFO_MAX_PICTURE_SIZE, (uint32_t *)newHwPictureW, (uint32_t *)newHwPictureH);

    if ((newHwPictureW > newPictureW) || (newHwPictureH > newPictureH)) {
        newW = *newPictureW;
        newH = *newPictureH;
    } else {
        ret = getCropRectAlign(*newHwPictureW, *newHwPictureH,
                            *newPictureW, *newPictureH,
                            &newX, &newY, &newW, &newH,
                            CAMERA_BCROP_ALIGN, 2, zoomRatio);
        if (ret < 0) {
            CLOGE("getCropRectAlign(%d, %d, %d, %d) fail",
                    *newHwPictureW, *newHwPictureH, *newPictureW, *newPictureH);
            return BAD_VALUE;
        }
    }

    *newHwPictureW = newW;
    *newHwPictureH = newH;

    return NO_ERROR;
}

bool ExynosCameraParameters::m_isSupportedPictureSize(const int width,
                                                     const int height)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

#ifdef USE_BINNING_MODE
    if (m_binningProperty) {
        CLOGD("Do not check supported picture size at binning mode");
        return true;
    };
#endif

    int camIdx = m_getSensorInfoCamIdx();
    if (m_camType != camIdx && m_configurations->getPictureSizeRatioId() >= 0) {
        m_cameraInfo.pictureSizeRatioId = m_configurations->getPictureSizeRatioId();
        return true;
    }

    getSize(HW_INFO_MAX_PICTURE_SIZE, (uint32_t *)&maxWidth, (uint32_t *)&maxHeight);

    if (maxWidth < width || maxHeight < height) {
        CLOGE("invalid picture Size(maxSize(%d/%d) size(%d/%d)",
            maxWidth, maxHeight, width, height);
        return false;
    }

    sizeList = m_staticInfo->jpegList;
    for (int i = 0; i < m_staticInfo->jpegListMax; i++) {
        if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
            continue;
        if (sizeList[i][0] == width && sizeList[i][1] == height) {
            m_cameraInfo.pictureSizeRatioId = sizeList[i][3];
            m_configurations->setPictureSizeRatioId(m_cameraInfo.pictureSizeRatioId);
            return true;
        }
    }

    if (m_staticInfo->hiddenPictureList != NULL) {
        sizeList = m_staticInfo->hiddenPictureList;
        for (int i = 0; i < m_staticInfo->hiddenPictureListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.pictureSizeRatioId = sizeList[i][3];
                m_configurations->setPictureSizeRatioId(m_cameraInfo.pictureSizeRatioId);
                return true;
            }
        }
    }

    CLOGE("Invalid picture size(%dx%d)", width, height);

    return false;
}

void ExynosCameraParameters::m_setHwBayerCropRegion(int w, int h, int x, int y)
{
    Mutex::Autolock lock(m_parameterLock);

    m_cameraInfo.hwBayerCropW = w;
    m_cameraInfo.hwBayerCropH = h;
    m_cameraInfo.hwBayerCropX = x;
    m_cameraInfo.hwBayerCropY = y;
}

void ExynosCameraParameters::getHwBayerCropRegion(int *w, int *h, int *x, int *y)
{
    Mutex::Autolock lock(m_parameterLock);

    *w = m_cameraInfo.hwBayerCropW;
    *h = m_cameraInfo.hwBayerCropH;
    *x = m_cameraInfo.hwBayerCropX;
    *y = m_cameraInfo.hwBayerCropY;
}

void ExynosCameraParameters::m_setHwPictureFormat(int fmt)
{
    m_cameraInfo.hwPictureFormat = fmt;
}

int ExynosCameraParameters::getHwPictureFormat(void)
{
    return m_cameraInfo.hwPictureFormat;
}

void ExynosCameraParameters::m_setHwPicturePixelSize(camera_pixel_size pixelSize)
{
    m_cameraInfo.hwPicturePixelSize = pixelSize;
}

camera_pixel_size ExynosCameraParameters::getHwPicturePixelSize(void)
{
    return m_cameraInfo.hwPicturePixelSize;
}

status_t ExynosCameraParameters::checkThumbnailSize(int thumbnailW, int thumbnailH)
{
    int curThumbnailW = -1, curThumbnailH = -1;

    if (thumbnailW < 0 || thumbnailH < 0
        || thumbnailW > m_staticInfo->maxThumbnailW
        || thumbnailH > m_staticInfo->maxThumbnailH) {
        CLOGE("Invalide thumbnail size %dx%d", thumbnailW, thumbnailH);
        return BAD_VALUE;
    }

    m_configurations->getSize(CONFIGURATION_THUMBNAIL_SIZE, (uint32_t *)&curThumbnailW, (uint32_t *)&curThumbnailH);

    if (curThumbnailW != thumbnailW || curThumbnailH != thumbnailH) {
        CLOGI("curThumbnailSize %dx%d newThumbnailSize %dx%d",
                curThumbnailW, curThumbnailH, thumbnailW, thumbnailH);
        m_configurations->setSize(CONFIGURATION_THUMBNAIL_SIZE, thumbnailW, thumbnailH);
    }

    return NO_ERROR;
}

/*
 * Static flag for LLS
 * Scenario : When Mode Change happens, this flag can be changed by application
 */
bool ExynosCameraParameters::getLLSOn(void)
{
    return m_LLSOn;
}

void ExynosCameraParameters::setLLSOn(bool enable)
{
    m_LLSOn = enable;
}

status_t ExynosCameraParameters::setCropRegion(int x, int y, int w, int h)
{
    status_t ret = NO_ERROR;

    ret = setMetaCtlCropRegion(&m_metadata, x, y, w, h);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setMetaCtlCropRegion(%d, %d, %d, %d)", x, y, w, h);
    }

    return ret;
}

status_t ExynosCameraParameters::setPictureCropRegion(int x, int y, int w, int h)
{
    status_t ret = NO_ERROR;

    ret = setMetaCtlCropRegion(&m_captureMetadata, x, y, w, h);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setMetaCtlCropRegion(%d, %d, %d, %d)", x, y, w, h);
    }

    return ret;
}

void ExynosCameraParameters::setDisplayStatRoi(int32_t dispCameraId, const ExynosRect &dispRect, const FrameSizeInfoMap_t &sizeMap)
{
    if (dispCameraId < 0 || dispCameraId >= CAMERA_ID_MAX) {
        CLOGE("invalid dispCameraId %d", dispCameraId);
        return;
    }

    if (checkValidateRect(dispRect) == false) {
        CLOGE("disp(%d) invalid dispRegion[%d,%d,%d,%d]",
            dispCameraId, dispRect.x, dispRect.y, dispRect.w, dispRect.h);
        return;
    }

    Mutex::Autolock lock(m_parameterLock);

    m_dispCameraId = dispCameraId;
    m_dispRect = dispRect;
    m_dispSizeInfoMap = sizeMap;
}

void ExynosCameraParameters::getDisplayStatRoi(int32_t &dispCameraId, ExynosRect &dispRect)
{
    Mutex::Autolock lock(m_parameterLock);

    dispCameraId = m_dispCameraId;
    dispRect = m_dispRect;
}

void ExynosCameraParameters::getDisplaySizeInfo(FrameSizeInfoMap_t &sizeMap)
{
    Mutex::Autolock lock(m_parameterLock);

    sizeMap = m_dispSizeInfoMap;
}

void ExynosCameraParameters::initDisplayStatRoi(void)
{
    ExynosRect initRect;

    Mutex::Autolock lock(m_parameterLock);

    m_dispCameraId = -1;
    m_dispRect = initRect;
}

void ExynosCameraParameters::m_getCropRegion(int *x, int *y, int *w, int *h)
{
    getMetaCtlCropRegion(&m_metadata, x, y, w, h);
}

void ExynosCameraParameters::m_getPictureCropRegion(int *x, int *y, int *w, int *h)
{
    getMetaCtlCropRegion(&m_captureMetadata, x, y, w, h);
}

status_t ExynosCameraParameters::m_setParamCropRegion(
        int srcW, int srcH,
        int dstW, int dstH)
{
    int newX = 0, newY = 0, newW = 0, newH = 0;
    float zoomRatio = m_configurations->getZoomRatio();

    if (getCropRectAlign(srcW,  srcH,
                         dstW,  dstH,
                         &newX, &newY,
                         &newW, &newH,
                         CAMERA_BCROP_ALIGN, 2,
                         zoomRatio) != NO_ERROR) {
        CLOGE("getCropRectAlign(%d, %d, %d, %d) zoomRatio(%f) fail",
            srcW,  srcH, dstW,  dstH, zoomRatio);
        return BAD_VALUE;
    }

    newX = ALIGN_UP(newX, 2);
    newY = ALIGN_UP(newY, 2);
    newW = srcW - (newX * 2);
    newH = srcH - (newY * 2);

    CLOGI("size0(%d, %d, %d, %d) zoomRatio(%f)",
        srcW, srcH, dstW, dstH, zoomRatio);
    CLOGI("size(%d, %d, %d, %d) zoomRatio(%f)",
        newX, newY, newW, newH, zoomRatio);

    m_setHwBayerCropRegion(newW, newH, newX, newY);

    return NO_ERROR;
}

void ExynosCameraParameters::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    memset(&m_exifInfo, 0, sizeof(m_exifInfo));

    /* 2 0th IFD TIFF Tags */
    /* 3 Maker */
    property_get("ro.product.manufacturer", property, EXIF_DEF_MAKER);
    strncpy((char *)m_exifInfo.maker, property,
                sizeof(m_exifInfo.maker) - 1);
    m_exifInfo.maker[sizeof(EXIF_DEF_MAKER) - 1] = '\0';

    /* 3 Model */
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)m_exifInfo.model, property,
                sizeof(m_exifInfo.model) - 1);
    m_exifInfo.model[sizeof(m_exifInfo.model) - 1] = '\0';
    /* 3 Software */
    if (property_get("ro.build.PDA", property, NULL) == 0)
        if (property_get("ro.build.display.id", property, NULL) == 0)
            property_get("ro.build.description", property, EXIF_DEF_SOFTWARE);

    strncpy((char *)m_exifInfo.software, property,
                sizeof(m_exifInfo.software) - 1);
    m_exifInfo.software[sizeof(m_exifInfo.software) - 1] = '\0';

    /* 3 YCbCr Positioning */
    m_exifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    /*2 0th IFD Exif Private Tags */
    /* 3 Exposure Program */
    m_exifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    /* 3 Exif Version */
    memcpy(m_exifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(m_exifInfo.exif_version));

    /* 3 Aperture */
    m_exifInfo.aperture.num = (uint32_t)(round(m_staticInfo->aperture * COMMON_DENOMINATOR));
    m_exifInfo.aperture.den = COMMON_DENOMINATOR;
    /* 3 F Number */
    m_exifInfo.fnumber.num = (uint32_t)(round(m_staticInfo->fNumber * COMMON_DENOMINATOR));
    m_exifInfo.fnumber.den = COMMON_DENOMINATOR;
    /* 3 Maximum lens aperture */
    uint32_t max_aperture_num = 0;
    max_aperture_num = ((uint32_t) (round(m_staticInfo->aperture * COMMON_DENOMINATOR)));

    m_exifInfo.max_aperture.num = APEX_FNUM_TO_APERTURE((double) (max_aperture_num) / (double) (m_exifInfo.fnumber.den)) * COMMON_DENOMINATOR;
    m_exifInfo.max_aperture.den = COMMON_DENOMINATOR;
    /* 3 Lens Focal Length */
    m_exifInfo.focal_length.num = (uint32_t)(round(m_staticInfo->focalLength * COMMON_DENOMINATOR * 10));
    m_exifInfo.focal_length.den = COMMON_DENOMINATOR * 10;

    /* 3 Maker note */
    if (m_exifInfo.maker_note)
        delete[] m_exifInfo.maker_note;

    ////////////////////////////////////////////////
    // get the new MakersNote object, to alloc m_exifInfo.maker_note
    ExynosCameraMakersNote *makerNote = this->getMakersNote();
    if (makerNote != NULL) {
        m_exifInfo.maker_note_size = makerNote->getSize();
        m_exifInfo.maker_note = new unsigned char[m_exifInfo.maker_note_size];
    } else {
        m_exifInfo.maker_note_size = 0;
        m_exifInfo.maker_note = NULL;
        CLOGE("this->getMakersNote() fail");
    }

    ////////////////////////////////////////////////

    /* 3 User Comments */
    if (m_exifInfo.user_comment)
        delete[] m_exifInfo.user_comment;

    m_exifInfo.user_comment_size = sizeof("user comment");
    m_exifInfo.user_comment = new unsigned char[m_exifInfo.user_comment_size + 8];
    memset((void *)m_exifInfo.user_comment, 0, m_exifInfo.user_comment_size + 8);

    /* 3 Color Space information */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_P3) == true) {
        m_exifInfo.color_space = 0xFFFF;
    } else {
        m_exifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    }

    /* 3 interoperability */
    m_exifInfo.interoperability_index = EXIF_DEF_INTEROPERABILITY;
    /* 3 Exposure Mode */
    m_exifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    /* 2 0th IFD GPS Info Tags */
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(m_exifInfo.gps_version_id, gps_version, sizeof(gps_version));

    /* 2 1th IFD TIFF Tags */
    m_exifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    m_exifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    m_exifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    m_exifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    m_exifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    m_exifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;

    m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 0);
}

void ExynosCameraParameters::setExifChangedAttribute(exif_attribute_t   *exifInfo,
                                                     ExynosRect         *pictureRect,
                                                     ExynosRect         *thumbnailRect,
                                                     camera2_shot_ext   *shot_ext,
                                                     bool               useDebugInfo2)
{
    m_setExifChangedAttribute(exifInfo, pictureRect, thumbnailRect, shot_ext, useDebugInfo2);
}

void ExynosCameraParameters::initThumbNailInfo(void)
{
    memset(m_thumbHistInfo.magicPrefix, 0, sizeof(m_thumbHistInfo.magicPrefix));
    if (m_thumbHistInfo.bufAddr[0] == NULL) {
        m_thumbHistInfo.bufAddr[0] = new char[THUMBNAIL_HISTOGRAM_STAT_PRAT1];
    }
    m_thumbHistInfo.bufSize[0] = 0;

    if (m_thumbHistInfo.bufAddr[1] == NULL) {
        m_thumbHistInfo.bufAddr[1] = new char[THUMBNAIL_HISTOGRAM_STAT_PRAT2];
    }
    m_thumbHistInfo.bufSize[1] = 0;
}

void ExynosCameraParameters::setThumbNailInfo(char *bufAddr)
{
    if (bufAddr != NULL && (m_thumbHistInfo.bufAddr[0] != NULL) && (m_thumbHistInfo.bufAddr[1] != NULL)) {
        memset(m_thumbHistInfo.magicPrefix, 0, sizeof(m_thumbHistInfo.magicPrefix));
        strncpy(m_thumbHistInfo.magicPrefix, (char*)bufAddr, THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX_SIZE-1);

        if (strcmp(m_thumbHistInfo.magicPrefix,THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX) == 0) {
            //Do deep data copy.
            memset(m_thumbHistInfo.bufAddr[0], 0, THUMBNAIL_HISTOGRAM_STAT_PRAT1);
            memcpy(m_thumbHistInfo.bufAddr[0], (void *)bufAddr,THUMBNAIL_HISTOGRAM_STAT_PRAT1);
            m_thumbHistInfo.bufSize[0] = THUMBNAIL_HISTOGRAM_STAT_PRAT1;

            memset(m_thumbHistInfo.bufAddr[1], 0, THUMBNAIL_HISTOGRAM_STAT_PRAT2);
            memcpy(m_thumbHistInfo.bufAddr[1], (void *)(bufAddr+THUMBNAIL_HISTOGRAM_STAT_PRAT1),THUMBNAIL_HISTOGRAM_STAT_PRAT2);
            m_thumbHistInfo.bufSize[1] = THUMBNAIL_HISTOGRAM_STAT_PRAT2;
            CLOGI("m_thumbHistInfo deep copy finished.");
        } else {
            m_thumbHistInfo.bufSize[0] = 0;
            m_thumbHistInfo.bufSize[1] = 0;
            CLOGI("Has no valid thumbnail&histogram data.");
        }
    } else
        CLOGE("Invalid thumbNail&histogram buffer address.");
}

debug_attribute_t *ExynosCameraParameters::getDebugAttribute(void)
{
    return &mDebugInfo;
}

debug_attribute_t *ExynosCameraParameters::getDebug2Attribute(void)
{
    return &mDebugInfo2;
}

status_t ExynosCameraParameters::getFixedExifInfo(exif_attribute_t *exifInfo)
{
    if (exifInfo == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    memcpy(exifInfo, &m_exifInfo, sizeof(exif_attribute_t));

    return NO_ERROR;
}

#ifdef USE_BINNING_MODE
int ExynosCameraParameters::getBinningMode(void)
{
    int ret = 0;

    if (m_staticInfo->vtcallSizeLutMax == 0
        || m_staticInfo->vtcallSizeLut == NULL) {
       CLOGV("vtCallSizeLut is NULL, can't support the binnig mode");
       return ret;
    }

    /* For VT Call with DualCamera Scenario */
    if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true) {
        CLOGV("PIP Mode can't support the binnig mode.(%d,%d)",
                getCameraId(), m_configurations->getMode(CONFIGURATION_PIP_MODE));
        return ret;
    }

    switch (m_configurations->getModeValue(CONFIGURATION_VT_MODE)) {
    case 1:
    case 2:
    case 4:
        ret = 1;
        break;
    default:
        if (m_binningProperty == true) {
            ret = 1;
        }
        break;
    }

    return ret;
}
#endif

const char *ExynosCameraParameters::getImageUniqueId(void)
{
#if defined(SENSOR_FW_GET_FROM_FILE)
    char *sensorfw = NULL;
    int   sensorfwSize = 0;
    char *uniqueid = NULL;
#ifdef FORCE_CAL_RELOAD
    char checkcal[50];

    memset(checkcal, 0, sizeof(checkcal));
#endif

    if (m_isUniqueIdRead == false) {
        sensorfw = (char *)getSensorFWFromFile(m_staticInfo, m_cameraId, &sensorfwSize);
#ifdef FORCE_CAL_RELOAD
        snprintf(checkcal, sizeof(checkcal), "%s", sensorfw);
        m_calValid = m_checkCalibrationDataValid(checkcal);
#endif

        if (isBackCamera(getCameraId())) {
            uniqueid = sensorfw;
        } else {
            {
                uniqueid = strtok(sensorfw, " ");
            }
        }

        if (uniqueid != NULL)
            setImageUniqueId(uniqueid);
        m_isUniqueIdRead = true;
    }

    return (const char *)m_cameraInfo.imageUniqueId;
#else
    return m_cameraInfo.imageUniqueId;
#endif
}

#if defined(SENSOR_FW_GET_FROM_FILE)
ExynosCameraEEPRomMap *ExynosCameraParameters::getEEPRomMap(void)
{
    status_t ret = NO_ERROR;
    char *eepromData = NULL;
    int   eepromSize = 0;

    ////////////////////////////////////////////////
    // once it true, it never check EEPRom, to avoid error message.
    if (m_flagEEPRomMapRead == true) {
        goto done;
    } else {
        m_flagEEPRomMapRead = true;
    }

    ////////////////////////////////////////////////
    // get eeprom stream
    eepromData = (char *)getSensorFWFromFile(m_staticInfo, getCameraId(), &eepromSize);
    if (eepromData == NULL || eepromSize == 0) {
        CLOGE("getSensorFWFromFile() fail, by eepromData == %p || eepromSize == %d", eepromData, eepromSize);
        goto done;
    }

    ////////////////////////////////////////////////
    // get the new EEPRomMap object, and create.
    m_eepromMap = ExynosCameraEEPRomMapFactory::newEEPRomMap(getCameraId());
    if(m_eepromMap == NULL) {
        CLOGE("ExynosCameraEEPRomMapFactory::newEEPRomMap() fail");
        goto done;
    }

    ret = m_eepromMap->create((const char *)eepromData, eepromSize);
    if (ret != NO_ERROR) {
        CLOGE("eepromMap->create(eepromData(%p), eepromSize(%d)) fail", eepromData, eepromSize);
        SAFE_DELETE(m_eepromMap);
        goto done;
    }

    ////////////////////////////////////////////////

done:
    return m_eepromMap;
}
#endif

ExynosCameraMakersNote *ExynosCameraParameters::getMakersNote(void)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // just reuse the current MakersNote object
    if (m_makersNote != NULL) {
        goto done;
    }

    ////////////////////////////////////////////////
    // get the new MakersNote object, and create
    m_makersNote = ExynosCameraMakersNoteFactory::newMakersNote(getCameraId());
    if(m_makersNote == NULL) {
        CLOGE("ExynosCameraMakersNoteFactory::newMakersNote() fail");
        goto done;
    }

    ret = m_makersNote->create();
    if (ret != NO_ERROR) {
        CLOGE("m_makersNote->create() fail");
        SAFE_DELETE(m_makersNote);
        goto done;
    }

    ////////////////////////////////////////////////

done:
    return m_makersNote;
}

int ExynosCameraParameters::getFocalLengthIn35mmFilm(void)
{
    return m_staticInfo->focalLengthIn35mmLength;
}

int ExynosCameraParameters::getLogicalFocalLengthIn35mmFilm(void)
{
    int sensorInfoCamIdx = m_camIdInfo->sensorInfoCamIdx;
    const struct ExynosCameraSensorInfoBase *staticInfo = getOtherSensorStaticInfo(sensorInfoCamIdx);
    return staticInfo->focalLengthIn35mmLength;
}

bool ExynosCameraParameters::getAutoFocusSupported(void)
{
    bool ret = false;

    for (int i = 0; i < m_staticInfo->afModesLength; i++) {
        if (m_staticInfo->afModes[i] == ANDROID_CONTROL_AF_MODE_AUTO) {
            ret = true;
            break;
        }
    }

    return ret;
}

float ExynosCameraParameters::getMaxZoomRatio(void)
{
    return (float)m_staticInfo->maxZoomRatio;
}

status_t ExynosCameraParameters::reInit(void)
{
    status_t ret = OK;
    ExynosCameraActivityFlash *flashMgr = m_activityControl->getFlashMgr();

#ifdef HAL3_YUVSIZE_BASED_BDS
    /* Reset all the YUV output sizes to smallest one
    To make sure the prior output setting do not affect current session.
    */
    ret = initYuvSizes();
    if (ret < 0) {
        CLOGE("clearYuvSizes() failed!! status(%d)", ret);
        return ret;
    }
    /* To find the minimum sized YUV stream in stream configuration, reset the old data */
#endif

    m_cameraInfo.yuvSizeLutIndex = -1;
    m_cameraInfo.pictureSizeLutIndex = -1;

    m_previewPortId = -1;
    m_previewCbPortId = -1;
    m_recordingPortId = -1;
    m_ysumPortId = -1;

    m_bayerFrameLock = false;
    m_oldBayerFrameLockCount = 0;
    m_newBayerFrameLockCount = 0;

    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        m_yuvOutPortId[i] = -1;
    }

    resetSize(HW_INFO_HW_YUV_SIZE);
    resetSize(HW_INFO_MAX_HW_YUV_SIZE);
    resetSize(HW_INFO_HW_YUV_INPUT_SIZE);

    resetYuvSizeRatioId();
    setUseSensorPackedBayer(true);
    flashMgr->setRecordingHint(false);
#ifdef USE_DUAL_CAMERA
    m_standbyState = DUAL_STANDBY_STATE_ON; /* default */
    initDisplayStatRoi();
#endif

    m_vendorReInit();

    return ret;
}

ExynosCameraActivityControl *ExynosCameraParameters::getActivityControl(void)
{
    return m_activityControl;
}

void ExynosCameraParameters::getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    if (flagReprocessing == true) {
        *setfile = m_setfileReprocessing;
        *yuvRange = m_yuvRangeReprocessing;
    } else {
        if (getFastenAeStableOn()) {
            *setfile = (m_setfile & 0xffff0000) | ISS_SUB_SCENARIO_STILL_PREVIEW;
        } else {
            *setfile = m_setfile;
        }
        *yuvRange = m_yuvRange;
    }
}

status_t ExynosCameraParameters::checkSetfileYuvRange(void)
{
    /* general */
    m_getSetfileYuvRange(false, &m_setfile, &m_yuvRange);

    /* reprocessing */
    m_getSetfileYuvRange(true, &m_setfileReprocessing, &m_yuvRangeReprocessing);

    CLOGD("m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

    return NO_ERROR;
}

void ExynosCameraParameters::setSetfileYuvRange(void)
{
    /* reprocessing */
    m_getSetfileYuvRange(true, &m_setfileReprocessing, &m_yuvRangeReprocessing);

    CLOGD("m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

}

void ExynosCameraParameters::setSetfileYuvRange(bool flagReprocessing, int setfile, int yuvRange)
{

    if (flagReprocessing) {
        m_setfileReprocessing = setfile;
        m_yuvRangeReprocessing = yuvRange;
    } else {
        m_setfile = setfile;
        m_yuvRange = yuvRange;
    }

    CLOGD("m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

}

void ExynosCameraParameters::m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    uint32_t currentSetfile = 0;
    uint32_t currentScenario = 0;
    uint32_t stateReg = 0;
    int flagYUVRange = YUV_FULL_RANGE;

    unsigned int minFps = 0;
    unsigned int maxFps = 0;

    m_configurations->getPreviewFpsRange(&minFps, &maxFps);

    if (m_configurations->getMode(CONFIGURATION_FULL_SIZE_LUT_MODE) == true) {
        currentScenario = FIMC_IS_SCENARIO_FULL_SIZE;
    }

#ifdef SUPPORT_VENDOR_TAG_3DHDR
    if (m_configurations->getModeValue(CONFIGURATION_EXTEND_SENSOR_MODE) == EXTEND_SENSOR_MODE_3DHDR) {
        stateReg = STATE_VIDEO_WDR_ON;
    }
#endif

    if (m_configurations->getDynamicMode(DYNAMIC_UHD_RECORDING_MODE) == true)
        stateReg |= STATE_REG_UHD_RECORDING;

    if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true) {
        stateReg |= STATE_REG_DUAL_MODE;
        if (m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE) == true)
            stateReg |= STATE_REG_DUAL_RECORDINGHINT;
    } else {
        if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true) {
            stateReg |= STATE_REG_RECORDINGHINT;
        }
    }

    if (flagReprocessing == true) {
        stateReg |= STATE_REG_FLAG_REPROCESSING;

#ifdef SET_LLS_CAPTURE_SETFILE
        if (m_configurations->getMode(CONFIGURATION_LLS_CAPTURE_MODE) == true)
            stateReg |= STATE_REG_NEED_LLS;
#endif

        if (m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE) == true ||
            m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE) == true) {
            stateReg |= STATE_LLHDR_CAPTURE_WDR_ON;
        }

        if (m_configurations->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == true) {
            stateReg |= STATE_REG_SUPER_NIGHT_SHOT;
        }

#ifdef USES_SUPER_RESOLUTION
        if (m_configurations->getMode(CONFIGURATION_SUPER_RESOLUTION_MODE) == true)
            stateReg |= STATE_REG_SUPER_RESOLUTION;
#endif

        if (m_configurations->getMode(CONFIGURATION_HDR_BAYER_MODE) == true)
            stateReg |= STATE_REG_LLHDR_CAPTURE;

        if (m_configurations->getMode(CONFIGURATION_HDR_YUV_MODE) == true)
            stateReg |= STATE_REG_MFHDR_CAPTURE;

        if (m_configurations->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE) == true)
            stateReg |= STATE_REG_FLASH_MFDN;

        if (m_configurations->getLongExposureShotCount() > 1)
            stateReg |= STATE_STILL_CAPTURE_LONG;
    } else if (flagReprocessing == false) {
        if (stateReg & STATE_REG_RECORDINGHINT
            || stateReg & STATE_REG_UHD_RECORDING
            || stateReg & STATE_REG_DUAL_RECORDINGHINT
            ) {
            flagYUVRange = YUV_LIMITED_RANGE;
        }

#ifdef USE_BINNING_MODE
        if (getBinningMode() == true
#ifndef SET_PREVIEW_BINNING_SETFILE
            && isBackCamera(m_cameraId)
#endif
        ) {
            stateReg |= STATE_REG_BINNING_MODE;
        }
#endif
    }

    if (isFrontCamera(m_cameraId)) {
        int vtMode = m_configurations->getModeValue(CONFIGURATION_VT_MODE);

        if (vtMode > 0) {
            switch (vtMode) {
            case 1:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1;
                if (stateReg == STATE_STILL_CAPTURE)
                    currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1_STILL_CAPTURE;
                break;
            case 2:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            case 4:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT4;
                break;
            default:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            }
        } else {
            switch(stateReg) {
                case STATE_STILL_PREVIEW:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                    break;

                case STATE_STILL_PREVIEW_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                    break;

                case STATE_STILL_PREVIEW_WDR_AUTO:
#if 0 /* BEYOND_BRING_UP */
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_3RD_PARTY_WDR_AUTO;
#endif
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                    break;

                case STATE_VIDEO:
#ifdef MOT_9609_SENSORS_TROIKA
                   if (30 < minFps && 30 < maxFps) {
                       if (300 == minFps && 300 == maxFps) {
                           currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                       } else if (60 == minFps && 60 == maxFps) {
                           currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                       } else if ((240 == minFps && 240 == maxFps) ||
                                  /* HACK: We need 480 setfile index */
                                  (480 == minFps && 480 == maxFps)) {
                           currentSetfile = ISS_SUB_SCENARIO_FHD_240FPS;
                       } else {
                           currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                       }
                    } else
#endif
                    {
                       currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                    }
                    break;

                case STATE_VIDEO_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                    break;

                case STATE_VIDEO_WDR_AUTO:
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_3RD_PARTY_WDR_AUTO;
                    break;

                case STATE_STILL_CAPTURE:
                case STATE_UHD_PREVIEW_CAPTURE:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                    break;

                case STATE_VIDEO_CAPTURE:
#ifdef MOT_9609_SENSORS_TROIKA
                    if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                    }else if(maxFps<=30) {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                    }else
#endif
                    {
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                    }
                    break;

                case STATE_UHD_VIDEO_CAPTURE:
#ifdef MOT_9609_SENSORS_TROIKA
                    currentSetfile =ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
#else
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
#endif
                    break;

                case STATE_STILL_CAPTURE_WDR_ON:
                case STATE_VIDEO_CAPTURE_WDR_ON:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
                case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                    break;

                case STATE_STILL_CAPTURE_WDR_AUTO:
                case STATE_VIDEO_CAPTURE_WDR_AUTO:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
                case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                    break;

                case STATE_DUAL_STILL_PREVIEW:
                case STATE_DUAL_STILL_CAPTURE:
                case STATE_DUAL_VIDEO_CAPTURE:
                case STATE_DUAL_VIDEO:
                    CLOGD("Unused senario of setfile.(0x%4x)", stateReg);
                    break;

                case STATE_UHD_PREVIEW:
                case STATE_UHD_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                    break;

                case STATE_UHD_VIDEO_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON;
                    break;

                case STATE_UHD_VIDEO_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_AUTO;
                    break;

#if defined(USE_BINNING_MODE) && defined(SET_PREVIEW_BINNING_SETFILE)
                case STATE_STILL_BINNING_PREVIEW:
                case STATE_VIDEO_BINNING:
                case STATE_DUAL_VIDEO_BINNING:
                case STATE_DUAL_STILL_BINING_PREVIEW:
                    currentSetfile = ISS_SUB_SCENARIO_FRONT_STILL_PREVIEW_BINNING;
                    break;
#endif /* USE_BINNING_MODE && SET_PREVIEW_BINNING_SETFILE */

#ifdef SET_LLS_CAPTURE_SETFILE
                case STATE_STILL_CAPTURE_LLS:
                    currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE;
                    break;
                case STATE_VIDEO_CAPTURE_WDR_ON_LLS:
                    currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_ON;
                    break;
                case STATE_STILL_CAPTURE_WDR_AUTO_LLS:
                    currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_AUTO;
                    break;
#endif /* SET_LLS_CAPTURE_SETFILE */
#ifdef USES_SUPER_RESOLUTION
                case STATE_STILL_CAPTURE_SUPER_RESOLUTION:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_SUPER_RESOLUTION;
                    break;
#endif
                case STATE_LLHDR_CAPTURE_WDR_ON:
                case STATE_LLHDR_CAPTURE_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS;
                    break;
                case STATE_STILL_CAPTURE_SUPER_NIGHT_SHOT:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_SUPER_NIGHT_SHOT;
                    break;
                case STATE_STILL_CAPTURE_LLHDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLHDR;
                    break;
                case STATE_STILL_CAPTURE_HDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                    break;
                case STATE_STILL_CAPTURE_FLASH_MFDN_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_FLASH_MFDN;
                    break;
                default:
                    CLOGD("[%d] can't define senario of setfile.(0x%4x)", flagReprocessing, stateReg);
                    break;
            }
        }
    } else {
        switch(stateReg) {
            case STATE_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                break;

            case STATE_STILL_PREVIEW_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                break;

            case STATE_STILL_PREVIEW_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                break;

            case STATE_STILL_CAPTURE:
            case STATE_DUAL_STILL_CAPTURE:
            case STATE_DUAL_VIDEO_CAPTURE:
            case STATE_UHD_PREVIEW_CAPTURE:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                break;

           case STATE_VIDEO_CAPTURE:
#ifdef MOT_9609_SENSORS_TROIKA
                if (60 == minFps && 60 == maxFps) {
                    currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                }else if(maxFps<=30) {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                }else
#endif
                {
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                }
                break;

           case STATE_UHD_VIDEO_CAPTURE:
#ifdef MOT_9609_SENSORS_TROIKA
                currentSetfile =ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
#else
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
#endif

            case STATE_STILL_CAPTURE_WDR_ON:
            case STATE_VIDEO_CAPTURE_WDR_ON:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
            case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                break;

            case STATE_STILL_CAPTURE_WDR_AUTO:
            case STATE_VIDEO_CAPTURE_WDR_AUTO:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
            case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                break;

            case STATE_STILL_BINNING_PREVIEW:
            case STATE_STILL_BINNING_PREVIEW_WDR_AUTO:
            case STATE_VIDEO_BINNING:
            case STATE_DUAL_VIDEO_BINNING:
            case STATE_DUAL_STILL_BINING_PREVIEW:
#if defined(USE_BINNING_MODE) && defined(SET_PREVIEW_BINNING_SETFILE)
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_BINNING;
#else
                currentSetfile = BINNING_SETFILE_INDEX;
#endif
                break;

            case STATE_VIDEO:
                if (30 < minFps && 30 < maxFps) {
                    if (300 == minFps && 300 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                    } else if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                    } else if ((240 == minFps && 240 == maxFps) ||
                            /* HACK: We need 480 setfile index */
                            (480 == minFps && 480 == maxFps)) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_240FPS;
                    } else {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                    }
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                }
                break;

            case STATE_VIDEO_WDR_ON:
                if (60 == minFps && 60 == maxFps) {
                    currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS_WDR_ON;
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                }
                break;

            case STATE_VIDEO_WDR_AUTO:
                if (30 < minFps && 30 < maxFps) {
                    if (300 == minFps && 300 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                    } else if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS_WDR_AUTO;
                    } else if ((240 == minFps && 240 == maxFps) ||
                            /* HACK: We need 480 setfile index */
                            (480 == minFps && 480 == maxFps)) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_240FPS;
                    } else {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                    }
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_AUTO;
                }
                break;

            case STATE_DUAL_VIDEO:
            case STATE_DUAL_STILL_PREVIEW:
                CLOGD("[%d] Unused senario of setfile.(0x%4x)", flagReprocessing, stateReg);
                break;

            case STATE_UHD_PREVIEW:
            case STATE_UHD_VIDEO:
                if (60 == minFps && 60 == maxFps) {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_60FPS;
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                }
                break;
            case STATE_UHD_PREVIEW_WDR_AUTO:
            case STATE_UHD_VIDEO_WDR_AUTO:
                if (60 == minFps && 60 == maxFps) {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_60FPS_WDR_AUTO;
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_AUTO;
                }
                break;

            case STATE_UHD_PREVIEW_WDR_ON:
            case STATE_UHD_VIDEO_WDR_ON:
                if (60 == minFps && 60 == maxFps) {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_60FPS_WDR_ON;
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON;
                }
                break;
#ifdef SET_LLS_CAPTURE_SETFILE
            case STATE_STILL_CAPTURE_LLS:
                currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE;
                break;
            case STATE_VIDEO_CAPTURE_WDR_ON_LLS:
                currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_ON;
                break;
            case STATE_STILL_CAPTURE_WDR_AUTO_LLS:
                currentSetfile = ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_AUTO;
                break;
#endif /* SET_LLS_CAPTURE_SETFILE */
#ifdef USES_SUPER_RESOLUTION
            case STATE_STILL_CAPTURE_SUPER_RESOLUTION:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_SUPER_RESOLUTION;
                break;
#endif
            case STATE_LLHDR_CAPTURE_WDR_ON:
            case STATE_LLHDR_CAPTURE_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS;
                break;
            case STATE_STILL_CAPTURE_SUPER_NIGHT_SHOT:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_SUPER_NIGHT_SHOT;
                break;
            case STATE_STILL_CAPTURE_LLHDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLHDR;
                break;
            case STATE_STILL_CAPTURE_HDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                break;
            case STATE_STILL_CAPTURE_FLASH_MFDN_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_FLASH_MFDN;
                break;
            case STATE_STILL_CAPTURE_LONG:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LONG;
                break;
            default:
                CLOGD("[%d] can't define senario of setfile.(0x%4x)", flagReprocessing, stateReg);
                break;
        }
    }

#if 0
    CLOGD("===============================================================================");
    CLOGD("CurrentState(0x%4x)", stateReg);
    CLOGD("getRTHdr()(%d)", m_configurations->getRTHdr());
    CLOGD("CONFIGURATION_RECORDING_MODE(%d)", m_configurations->getMode(CONFIGURATION_RECORDING_MODE));
    CLOGD("DYNAMIC_UHD_RECORDING_MODE(%d)", m_configurations->getDynamicMode(DYNAMIC_UHD_RECORDING_MODE));
    CLOGD("CONFIGURATION_PIP_MODE(%d)", m_configurations->getMode(CONFIGURATION_PIP_MODE));
    CLOGD("CONFIGURATION_PIP_RECORDINGMODE(%d)", m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE));
#ifdef USE_BINNING_MODE
    CLOGD("getBinningMode(%d)", getBinningMode());
#endif
    CLOGD("flagReprocessing(%d)", flagReprocessing);
    CLOGD("===============================================================================");
    CLOGD("currentSetfile(%d)", currentSetfile);
    CLOGD("flagYUVRange(%d)", flagYUVRange);
    CLOGD("===============================================================================");
#else
    CLOGD("[%s] CurrentState (0x%4x), currentSetfile(%d), currentScenario(%d)",
            flagReprocessing ? "Reprocessing" : "Preview",
            stateReg, currentSetfile, currentScenario);
#endif

    *setfile = currentSetfile | (currentScenario << 16);
    *yuvRange = flagYUVRange;
}

void ExynosCameraParameters::setUseSensorPackedBayer(bool enable)
{
#ifdef CAMERA_PACKED_BAYER_ENABLE
    /* NOP */
#else
    enable = false;
#endif
    m_useSensorPackedBayer = enable;
    CLOGD("PackedBayer %s", (m_useSensorPackedBayer == true) ? "ENABLE" : "DISABLE");
}

void ExynosCameraParameters::setFastenAeStableOn(bool enable)
{
    m_fastenAeStableOn = enable;
    CLOGD("m_cameraId(%d) : m_fastenAeStableOn(%d)",
        m_cameraId, m_fastenAeStableOn);
}

bool ExynosCameraParameters::getFastenAeStableOn(void)
{
    return m_fastenAeStableOn;
}

bool ExynosCameraParameters::checkFastenAeStableEnable(void)
{
    bool ret = false;

    if (isFastenAeStableSupported(m_cameraId) == true
        && m_configurations->getMode(CONFIGURATION_PIP_MODE) == false
#ifdef USE_DUAL_CAMERA
        && ((m_configurations->getMode(CONFIGURATION_DUAL_MODE)
            != m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE))
            || m_configurations->getUseFastenAeStable() == true)
#else
        && m_configurations->getUseFastenAeStable() == true
#endif
        && m_configurations->getMode(CONFIGURATION_VISION_MODE) == false
        && m_camType == MAIN_CAM) {
        ret = true;
    } else {
        ret = false;
    }

    return ret;
}

int ExynosCameraParameters::getTnrMode(void)
{
    int mode = 0;

    /* you can fix here, according to scenario */

    return mode;
}

status_t ExynosCameraParameters::m_getSizeListIndex(int (*sizelist)[SIZE_OF_LUT], int listMaxSize, int ratio, int *index)
{
    if (*index == -1) {
        for (int i = 0; i < listMaxSize; i++) {
            if (sizelist[i][RATIO_ID] == ratio) {
                *index = i;
                break;
            }
        }
    }

    if (*index == -1) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ExynosCameraParameters::getPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom)
{
    return getCropSizeImpl(srcRect, dstRect, applyZoom, isUse3aaInputCrop(), true /* update */);
}

status_t ExynosCameraParameters::getStatCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom)
{
    int use3aaInputCrop = isUse3aaInputCrop();

#if !defined(SUPPORT_DISPLAY_REGION)
#ifdef SUPPORT_POST_SCALER_ZOOM
    /* postScalerZoom scenario send stats region same as bcrop */
    use3aaInputCrop = true;
#endif
#endif

    return getCropSizeImpl(srcRect, dstRect, applyZoom, use3aaInputCrop, false);
}

status_t ExynosCameraParameters::getCropSizeImpl(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom, bool use3aaInputCrop, bool update)
{
    int hwBnsW   = 0;
    int hwBnsH   = 0;
    int hwBcropW = 0;
    int hwBcropH = 0;
    int sizeList[SIZE_LUT_INDEX_END];
    int hwSensorMarginW = 0;
    int hwSensorMarginH = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_getPreviewSizeList(sizeList) != NO_ERROR)
        return calcPreviewBayerCropSize(srcRect, dstRect);

    /* use LUT */
    hwBnsW = sizeList[BNS_W];
    hwBnsH = sizeList[BNS_H];
    hwBcropW = sizeList[BCROP_W];
    hwBcropH = sizeList[BCROP_H];

    int curBnsW = 0, curBnsH = 0;
    getSize(HW_INFO_HW_BNS_SIZE, (uint32_t *)&curBnsW, (uint32_t *)&curBnsH);
    if (SIZE_RATIO(curBnsW, curBnsH) != SIZE_RATIO(hwBnsW, hwBnsH)) {
        CLOGW("current BNS size(%dx%d) is NOT same with Hw BNS size(%dx%d)",
                 curBnsW, curBnsH, hwBnsW, hwBnsH);
    }

    /* Re-calculate the BNS size for removing Sensor Margin */
    getSize(HW_INFO_SENSOR_MARGIN_SIZE, (uint32_t *)&hwSensorMarginW, (uint32_t *)&hwSensorMarginH);
    m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    hwBnsW = hwBnsW - hwSensorMarginW;
    hwBnsH = hwBnsH - hwSensorMarginH;

    /* src */
    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwBnsW;
    srcRect->h = hwBnsH;

    ExynosRect activeArraySize;
    ExynosRect cropRegion;
    ExynosRect hwActiveArraySize;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;
    status_t ret = NO_ERROR;

    if (applyZoom == true && use3aaInputCrop) {
        getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
        m_getCropRegion(&cropRegion.x, &cropRegion.y, &cropRegion.w, &cropRegion.h);

        CLOGV("ActiveArraySize %dx%d(%d) CropRegion %d,%d %dx%d(%d) HWSensorSize %dx%d(%d) BcropSize %dx%d(%d)",
                activeArraySize.w, activeArraySize.h, SIZE_RATIO(activeArraySize.w, activeArraySize.h),
                cropRegion.x, cropRegion.y, cropRegion.w, cropRegion.h, SIZE_RATIO(cropRegion.w, cropRegion.h),
                hwBnsW, hwBnsH, SIZE_RATIO(hwBnsW, hwBnsH),
                hwBcropW, hwBcropH, SIZE_RATIO(hwBcropW, hwBcropH));

        /* Calculate H/W active array size for current sensor aspect ratio
           based on active array size
         */
        ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
                               hwBnsW, hwBnsH,
                               &hwActiveArraySize.x, &hwActiveArraySize.y,
                               &hwActiveArraySize.w, &hwActiveArraySize.h,
                               2, 2, 1.0f);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                    activeArraySize.w, activeArraySize.h,
                    hwBnsW, hwBnsH);
            return INVALID_OPERATION;
        }

        /* Scale down the crop region & HW active array size
           to adjust them to the 3AA input size without sensor margin
         */
        scaleRatioW = (float) hwBnsW / (float) hwActiveArraySize.w;
        scaleRatioH = (float) hwBnsH / (float) hwActiveArraySize.h;

        cropRegion.x = ALIGN_DOWN((int) (((float) cropRegion.x) * scaleRatioW), 2);
        cropRegion.y = ALIGN_DOWN((int) (((float) cropRegion.y) * scaleRatioH), 2);
        cropRegion.w = (int) ((float) cropRegion.w) * scaleRatioW;
        cropRegion.h = (int) ((float) cropRegion.h) * scaleRatioH;

        hwActiveArraySize.x = ALIGN_DOWN((int) (((float) hwActiveArraySize.x) * scaleRatioW), 2);
        hwActiveArraySize.y = ALIGN_DOWN((int) (((float) hwActiveArraySize.y) * scaleRatioH), 2);
        hwActiveArraySize.w = (int) (((float) hwActiveArraySize.w) * scaleRatioW);
        hwActiveArraySize.h = (int) (((float) hwActiveArraySize.h) * scaleRatioH);

        if (cropRegion.w < 1 || cropRegion.h < 1) {
#ifdef USE_DUAL_CAMERA
            if (getStandbyState() == DUAL_STANDBY_STATE_OFF
                || getStandbyState() == DUAL_STANDBY_STATE_OFF_READY)
#endif
            {
            CLOGW("Invalid scaleRatio %fx%f, cropRegion' %d,%d %dx%d.",
                    scaleRatioW, scaleRatioH,
                    cropRegion.x, cropRegion.y, cropRegion.w, cropRegion.h);
            }
            cropRegion.x = 0;
            cropRegion.y = 0;
            cropRegion.w = hwBnsW;
            cropRegion.h = hwBnsH;
        }

        /* Calculate HW bcrop size inside crop region' */
        if ((cropRegion.w > hwBcropW) && (cropRegion.h > hwBcropH)) {
            dstRect->x = ALIGN_DOWN(((cropRegion.w - hwBcropW) >> 1), 2);
            dstRect->y = ALIGN_DOWN(((cropRegion.h - hwBcropH) >> 1), 2);
            dstRect->w = hwBcropW;
            dstRect->h = hwBcropH;
        } else {
            ret = getCropRectAlign(cropRegion.w, cropRegion.h,
                    hwBcropW, hwBcropH,
                    &(dstRect->x), &(dstRect->y),
                    &(dstRect->w), &(dstRect->h),
                    CAMERA_BCROP_ALIGN, 2, 1.0f);
            if (ret != NO_ERROR) {
                CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                        cropRegion.w, cropRegion.h,
                        hwBcropW, hwBcropH);
                return INVALID_OPERATION;
            }

            dstRect->x = ALIGN_DOWN(dstRect->x, 2);
            dstRect->y = ALIGN_DOWN(dstRect->y, 2);
        }

        /* Add crop region offset to HW bcrop offset */
        dstRect->x += cropRegion.x;
        dstRect->y += cropRegion.y;

        /* Check the boundary of HW active array size for HW bcrop offset & size */
        if (dstRect->x < hwActiveArraySize.x) dstRect->x = hwActiveArraySize.x;
        if (dstRect->y < hwActiveArraySize.y) dstRect->y = hwActiveArraySize.y;
        if (dstRect->x + dstRect->w > hwActiveArraySize.x + hwBnsW)
            dstRect->x = hwActiveArraySize.x + hwBnsW - dstRect->w;
        if (dstRect->y + dstRect->h > hwActiveArraySize.y + hwBnsH)
            dstRect->y = hwActiveArraySize.y + hwBnsH - dstRect->h;

        /* Remove HW active array size offset */
        dstRect->x -= hwActiveArraySize.x;
        dstRect->y -= hwActiveArraySize.y;
    } else {
        /* Calculate HW bcrop size inside HW sensor output size */
        if ((hwBnsW >= hwBcropW) && (hwBnsH >= hwBcropH)) {
            dstRect->x = ALIGN_DOWN(((hwBnsW - hwBcropW) >> 1), 2);
            dstRect->y = ALIGN_DOWN(((hwBnsH - hwBcropH) >> 1), 2);
            dstRect->w = hwBcropW;
            dstRect->h = hwBcropH;
        } else {
            ret = getCropRectAlign(hwBnsW, hwBnsH,
                                   hwBcropW, hwBcropH,
                                   &(dstRect->x), &(dstRect->y),
                                   &(dstRect->w), &(dstRect->h),
                                   CAMERA_BCROP_ALIGN, 2, 1.0f);
            if (ret != NO_ERROR) {
                CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                        hwBnsW, hwBnsH,
                        hwBcropW, hwBcropH);
                return INVALID_OPERATION;
            }
        }
    }

    CLOGV("HWBcropSize %d,%d %dx%d(%d)",
            dstRect->x, dstRect->y, dstRect->w, dstRect->h, SIZE_RATIO(dstRect->w, dstRect->h));

    /* Compensate the crop size to satisfy Max Scale Up Ratio */
    if (dstRect->w * SCALER_MAX_SCALE_UP_RATIO < hwBnsW
        || dstRect->h * SCALER_MAX_SCALE_UP_RATIO < hwBnsH) {
        dstRect->w = ALIGN_UP((int)ceil((float)hwBnsW / SCALER_MAX_SCALE_UP_RATIO), CAMERA_BCROP_ALIGN);
        dstRect->h = ALIGN_UP((int)ceil((float)hwBnsH / SCALER_MAX_SCALE_UP_RATIO), CAMERA_BCROP_ALIGN);
    }

    /* Compensate the crop size to satisfy Max Scale Up Ratio on reprocessing path */
    if (getUsePureBayerReprocessing() == false
        && m_cameraInfo.pictureSizeRatioId != m_cameraInfo.yuvSizeRatioId) {
        status_t ret = NO_ERROR;

        ret = m_adjustPreviewCropSizeForPicture(srcRect, dstRect);
        if (ret != NO_ERROR) {
            CLOGW("Failed to adjustPreviewCropSizeForPicture. Bcrop %d,%d %dx%d",
                    dstRect->x, dstRect->y, dstRect->w, dstRect->h);
            /* continue */
        }
    }

    if (update)
        m_setHwBayerCropRegion(dstRect->w, dstRect->h, dstRect->x, dstRect->y);

#ifdef DEBUG_PERFRAME
    CLOGD("hwBnsSize %dx%d, hwBcropSize %d,%d %dx%d",
            srcRect->w, srcRect->h,
            dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCameraParameters::m_adjustPreviewCropSizeForPicture(ExynosRect *inputRect, ExynosRect *previewCropRect)
{
    status_t ret = NO_ERROR;
    ExynosRect pictureRect;
    ExynosRect pictureCropRect;
    ExynosRect minPictureCropRect;
    ExynosRect newPreviewCropRect;
    int offsetX = 0, offsetY = 0;
    bool isAdjusted = false;

    // HACK: Need to add elegant variable whether it use HW_PICTURE or PICTURE size.
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureRect.w, (uint32_t *)&pictureRect.h);
    } else {
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureRect.w, (uint32_t *)&pictureRect.h);
    }

    /* Get the expected picture crop size in preview crop rect for processed bayer */
    ret = getCropRectAlign(previewCropRect->w, previewCropRect->h,
                           pictureRect.w, pictureRect.h,
                           &pictureCropRect.x, &pictureCropRect.y,
                           &pictureCropRect.w, &pictureCropRect.h,
                           CAMERA_MCSC_ALIGN, 2,
                           1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. PreviewCrop %dx%d Picture %dx%d",
                previewCropRect->w, previewCropRect->h, pictureRect.w, pictureRect.h);
        return INVALID_OPERATION;
    }

    /* Get the smallest picture crop size */
    ret = getCropRectAlign(pictureRect.w, pictureRect.h,
                           pictureRect.w, pictureRect.h,
                           &minPictureCropRect.x, &minPictureCropRect.y,
                           &minPictureCropRect.w, &minPictureCropRect.h,
                           CAMERA_MCSC_ALIGN, 2,
                           (float) SCALER_MAX_SCALE_UP_RATIO);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAling. Picture %dx%d, ZoomRatio %d",
                pictureRect.w, pictureRect.h, SCALER_MAX_SCALE_UP_RATIO);
        return INVALID_OPERATION;
    }

    /* Adjust preview crop size for valid picture crop size */
    if (pictureCropRect.w < minPictureCropRect.w) {
        newPreviewCropRect.w = ALIGN_UP(((minPictureCropRect.h * previewCropRect->w) / previewCropRect->h), CAMERA_BCROP_ALIGN);
        newPreviewCropRect.h = minPictureCropRect.h;
        isAdjusted = true;
    } else if (pictureCropRect.h < minPictureCropRect.h) {
        newPreviewCropRect.w = minPictureCropRect.w;
        newPreviewCropRect.h = ALIGN_UP(((minPictureCropRect.w * previewCropRect->h) / previewCropRect->w), 2);
        isAdjusted = true;
    }

    if (isAdjusted == true) {
        CLOGW("Zoom ratio is upto x%d. previewCrop %dx%d pictureCrop %dx%d pictureTarget %dx%d",
                SCALER_MAX_SCALE_UP_RATIO,
                previewCropRect->w, previewCropRect->h,
                pictureCropRect.w, pictureCropRect.h,
                pictureRect.w, pictureRect.h);

        /* Adjust preview crop offset for the adjusted preview crop size */
        offsetX = ALIGN_UP(((newPreviewCropRect.w - previewCropRect->w) >> 1), 2);
        offsetY = ALIGN_UP(((newPreviewCropRect.h - previewCropRect->h) >> 1), 2);
        newPreviewCropRect.x = previewCropRect->x - offsetX;
        newPreviewCropRect.y = previewCropRect->y - offsetY;

        /* Check validation of the adjusted preview crop reigon */
        if (newPreviewCropRect.x < 0) newPreviewCropRect.x = 0;
        if (newPreviewCropRect.y < 0) newPreviewCropRect.y = 0;
        if (newPreviewCropRect.x + newPreviewCropRect.w > inputRect->w) newPreviewCropRect.x = inputRect->w - newPreviewCropRect.w;
        if (newPreviewCropRect.y + newPreviewCropRect.h > inputRect->h) newPreviewCropRect.y = inputRect->h - newPreviewCropRect.h;

        *previewCropRect = newPreviewCropRect;
    }

    return ret;
}

status_t ExynosCameraParameters::calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    status_t ret = 0;

    int maxZoomRatio = 0;
    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int bayerFormat = getBayerFormat(PIPE_3AA);

    /* TODO: check state ready for start */
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);

    // HACK: Need to add elegant variable whether it use HW_PICTURE or PICTURE size.
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    } else {
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    }

    getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    m_configurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);
    getSize(HW_INFO_SENSOR_MARGIN_SIZE, (uint32_t *)&hwSensorMarginW, (uint32_t *)&hwSensorMarginH);
    m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
    int maxSensorW = 0, maxSensorH = 0;
    float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

    if (isUse3aaInputCrop() == true)
        m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);

    getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);

    /* 1. Scale down the crop region to adjust with the bcrop input size */
    scaleRatioX = (float) hwSensorW / (float) maxSensorW;
    scaleRatioY = (float) hwSensorH / (float) maxSensorH;
    cropRegionX = (int) (cropRegionX * scaleRatioX);
    cropRegionY = (int) (cropRegionY * scaleRatioY);
    cropRegionW = (int) (cropRegionW * scaleRatioX);
    cropRegionH = (int) (cropRegionH * scaleRatioY);

    if (cropRegionW < 1 || cropRegionH < 1) {
        cropRegionW = hwSensorW;
        cropRegionH = hwSensorH;
    }

    /* 2. Calculate the real crop region with considering the target ratio */
    ret = getCropRectAlign(cropRegionW, cropRegionH,
            previewW, previewH,
            &cropX, &cropY,
            &cropW, &cropH,
            CAMERA_BCROP_ALIGN, 2,
            1.0f);

    cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
    cropY = ALIGN_DOWN((cropRegionY + cropY), 2);

    if (getUsePureBayerReprocessing() == false) {
        int pictureCropX = 0, pictureCropY = 0;
        int pictureCropW = 0, pictureCropH = 0;

        ret = getCropRectAlign(cropW, cropH,
                pictureW, pictureH,
                &pictureCropX, &pictureCropY,
                &pictureCropW, &pictureCropH,
                CAMERA_BCROP_ALIGN, 2,
                1.0f);

        pictureCropX = ALIGN_DOWN(pictureCropX, 2);
        pictureCropY = ALIGN_DOWN(pictureCropY, 2);
        pictureCropW = cropW - (pictureCropX * 2);
        pictureCropH = cropH - (pictureCropY * 2);

        if (pictureCropW < pictureW / maxZoomRatio || pictureCropH < pictureH / maxZoomRatio) {
            CLOGW(" zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", maxZoomRatio, cropW, cropH, pictureW, pictureH);
            float src_ratio = 1.0f;
            float dst_ratio = 1.0f;
            /* ex : 1024 / 768 */
            src_ratio = ROUND_OFF_HALF(((float)cropW / (float)cropH), 2);
            /* ex : 352  / 288 */
            dst_ratio = ROUND_OFF_HALF(((float)pictureW / (float)pictureH), 2);

            if (dst_ratio <= src_ratio) {
                /* shrink w */
                cropX = ALIGN_DOWN(((int)(hwSensorW - ((pictureH / maxZoomRatio) * src_ratio)) >> 1), 2);
                cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            } else {
                /* shrink h */
                cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
                cropY = ALIGN_DOWN(((int)(hwSensorH - ((pictureW / maxZoomRatio) / src_ratio)) >> 1), 2);
            }
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }
    }

#if 0
    CLOGD("hwSensorSize (%dx%d), previewSize (%dx%d)",
            hwSensorW, hwSensorH, previewW, previewH);
    CLOGD("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwSensorW;
    srcRect->h = hwSensorH;
    srcRect->fullW = hwSensorW;
    srcRect->fullH = hwSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;

    m_setHwBayerCropRegion(dstRect->w, dstRect->h, dstRect->x, dstRect->y);
    return NO_ERROR;
}

status_t ExynosCameraParameters::calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int maxSensorW = 0, maxSensorH = 0;
    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int hwSensorCropX = 0, hwSensorCropY = 0;
    int hwSensorCropW = 0, hwSensorCropH = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int maxZoomRatio = 0;
    int bayerFormat = getBayerFormat(PIPE_3AA_REPROCESSING);

    /* TODO: check state ready for start */
    pictureFormat = getHwPictureFormat();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);

    // HACK: Need to add elegant variable whether it use HW_PICTURE or PICTURE size.
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    } else {
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    }

    getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);
    getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    m_configurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);
    getSize(HW_INFO_SENSOR_MARGIN_SIZE, (uint32_t *)&hwSensorMarginW, (uint32_t *)&hwSensorMarginH);

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    if (getUsePureBayerReprocessing() == true) {
        int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

        if (isUseReprocessing3aaInputCrop() == true)
            m_getPictureCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);

        /* 1. Scale down the crop region to adjust with the bcrop input size */
        scaleRatioX = (float) hwSensorW / (float) maxSensorW;
        scaleRatioY = (float) hwSensorH / (float) maxSensorH;
        cropRegionX = (int) (cropRegionX * scaleRatioX);
        cropRegionY = (int) (cropRegionY * scaleRatioY);
        cropRegionW = (int) (cropRegionW * scaleRatioX);
        cropRegionH = (int) (cropRegionH * scaleRatioY);

        if (cropRegionW < 1 || cropRegionH < 1) {
            cropRegionW = hwSensorW;
            cropRegionH = hwSensorH;
        }

        ret = getCropRectAlign(cropRegionW, cropRegionH,
                pictureW, pictureH,
                &cropX, &cropY,
                &cropW, &cropH,
                CAMERA_BCROP_ALIGN, 2,
                1.0f);

        cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
        cropY = ALIGN_DOWN((cropRegionY + cropY), 2);

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            CLOGW(" zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }
    } else {
        getHwBayerCropRegion(&hwSensorCropW, &hwSensorCropH, &hwSensorCropX, &hwSensorCropY);

        ret = getCropRectAlign(hwSensorCropW, hwSensorCropH,
                     pictureW, pictureH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_BCROP_ALIGN, 2,
                     1.0f);

        cropX = ALIGN_DOWN(cropX, 2);
        cropY = ALIGN_DOWN(cropY, 2);
        cropW = hwSensorCropW - (cropX * 2);
        cropH = hwSensorCropH - (cropY * 2);

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            CLOGW(" zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorCropW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorCropH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorCropW - (cropX * 2);
            cropH = hwSensorCropH - (cropY * 2);
        }
    }

#if 1
    CLOGD("maxSensorSize %dx%d, hwSensorSize %dx%d, previewSize %dx%d",
            maxSensorW, maxSensorH, hwSensorW, hwSensorH, previewW, previewH);
    CLOGD("hwPictureSize %dx%d, pictureSize %dx%d",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD("pictureBayerCropSize %d,%d %dx%d",
            cropX, cropY, cropW, cropH);
    CLOGD("pictureFormat 0x%x, JPEG_INPUT_COLOR_FMT 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = maxSensorW;
    srcRect->h = maxSensorH;
    srcRect->fullW = maxSensorW;
    srcRect->fullH = maxSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;
    return NO_ERROR;
}

status_t ExynosCameraParameters::getPreviewBdsSize(ExynosRect *dstRect, bool applyZoom)
{
    status_t ret = NO_ERROR;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
    ExynosRect bdsSize;

    ret = m_getPreviewBdsSize(&bdsSize);
    if (ret != NO_ERROR) {
        CLOGE("Failed to m_getPreviewBdsSize()");
        return ret;
    }

    if (this->getHWVdisMode() == true) {
        int disW = ALIGN_UP((int)(bdsSize.w * HW_VDIS_W_RATIO), CAMERA_BCROP_ALIGN);
        int disH = ALIGN_UP((int)(bdsSize.h * HW_VDIS_H_RATIO), CAMERA_BCROP_ALIGN);

        CLOGV("HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)",
             dstRect->w, dstRect->h, disW, disH);

        bdsSize.w = disW;
        bdsSize.h = disH;
    }

    if (m_configurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE)) {
        int portId = getRecordingPortId();
        uint32_t maxHwYuvW = 0;
        uint32_t maxHwYuvH = 0;
        ret = getSize(HW_INFO_MAX_HW_YUV_SIZE, &maxHwYuvW, &maxHwYuvH, portId);

        if (ALIGN_UP((int)(bdsSize.w * HW_VDIS_W_RATIO), CAMERA_BCROP_ALIGN) > (int)maxHwYuvW || ALIGN_UP((int)(bdsSize.h * HW_VDIS_W_RATIO), CAMERA_BCROP_ALIGN) > (int)maxHwYuvH) {
            bdsSize.w = ALIGN_UP((int)(bdsSize.w * HW_VDIS_W_RATIO), CAMERA_BCROP_ALIGN);
            bdsSize.h = ALIGN_UP((int)(bdsSize.h * HW_VDIS_H_RATIO), CAMERA_BCROP_ALIGN);
        }
    }

    /* Check the invalid BDS size compared to Bcrop size */
    ret = getPreviewBayerCropSize(&bnsSize, &bayerCropSize, applyZoom);
    if (ret != NO_ERROR)
        CLOGE("Failed to getPreviewBayerCropSize()");

    if (bayerCropSize.w < bdsSize.w || bayerCropSize.h < bdsSize.h) {
        CLOGV("bayerCropSize %dx%d is smaller than BDSSize %dx%d. Force bayerCropSize",
                bayerCropSize.w, bayerCropSize.h, bdsSize.w, bdsSize.h);

        bdsSize.w = bayerCropSize.w;
        bdsSize.h = bayerCropSize.h;
    }

#ifdef HAL3_YUVSIZE_BASED_BDS
    /*
       Do not use BDS downscaling if one or more YUV ouput size
       is larger than BDS output size
    */
    for (int i = 0; i < getYuvStreamMaxNum(); i++) {
        int yuvWidth, yuvHeight;
        getSize(HW_INFO_HW_YUV_SIZE, &yuvWidth, &yuvHeight, i);

        if(yuvWidth > bdsSize.w || yuvHeight > bdsSize.h) {
            CLOGV("Expanding BDS(%d x %d) size to BCrop(%d x %d) to handle YUV stream [%d, (%d x %d)]",
                    bdsSize.w, bdsSize.h, bayerCropSize.w, bayerCropSize.h, i, yuvWidth, yuvHeight);
            bdsSize.w = bayerCropSize.w;
            bdsSize.h = bayerCropSize.h;
            break;
        }
    }
#endif

#ifdef USE_SBWC
    if (bdsSize.h > SBWC_MAX_HEIGHT) {
        bdsSize.w = (SBWC_MAX_HEIGHT * bdsSize.w) / bdsSize.h;
        bdsSize.w = ALIGN_DOWN(bdsSize.w, CAMERA_BCROP_ALIGN);
        bdsSize.h = SBWC_MAX_HEIGHT;
    }
#endif

    if (this->isUse3aaBDSOff()) {
        /* It is a BDS OFF scenario */
        bdsSize.w = bayerCropSize.w;
        bdsSize.h = bayerCropSize.h;
    }

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = bdsSize.w;
    dstRect->h = bdsSize.h;

#ifdef DEBUG_PERFRAME
    CLOGD("hwBdsSize %dx%d", dstRect->w, dstRect->h);
#endif

    return ret;
}

status_t ExynosCameraParameters::calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    status_t ret = NO_ERROR;
    int previewW = 0, previewH = 0;
    int bayerFormat = getBayerFormat(PIPE_3AA);
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;

    /* Get preview size info */
    m_configurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);
    ret = getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
    if (ret != NO_ERROR)
        CLOGE("getPreviewBayerCropSize() failed");

    srcRect->x = bayerCropSize.x;
    srcRect->y = bayerCropSize.y;
    srcRect->w = bayerCropSize.w;
    srcRect->h = bayerCropSize.h;
    srcRect->fullW = bnsSize.w;
    srcRect->fullH = bnsSize.h;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;
    dstRect->fullW = previewW;
    dstRect->fullH = previewH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    /* Check the invalid BDS size compared to Bcrop size */
    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

    if (this->isUse3aaBDSOff()) {
        /* It is a BDS OFF scenario */
        dstRect->w = srcRect->w;
        dstRect->h = srcRect->h;
    }

#ifdef DEBUG_PERFRAME
    CLOGE("BDS %dx%d Preview %dx%d", dstRect->w, dstRect->h, previewW, previewH);
#endif

    return NO_ERROR;
}

status_t ExynosCameraParameters::getPictureBdsSize(ExynosRect *dstRect, bool applyZoom)
{
    status_t ret = NO_ERROR;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
    int hwBdsW = 0;
    int hwBdsH = 0;
    int sizeList[SIZE_LUT_INDEX_END];

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_getPictureSizeList(sizeList) != NO_ERROR) {
        ExynosRect rect;
        return calcPictureBDSSize(&rect, dstRect);
    }

    /* use LUT */
    hwBdsW = sizeList[BDS_W];
    hwBdsH = sizeList[BDS_H];

    /* Check the invalid BDS size compared to Bcrop size */
    ret = getPictureBayerCropSize(&bnsSize, &bayerCropSize, applyZoom);
    if (ret != NO_ERROR)
        CLOGE("Failed to getPictureBayerCropSize()");

    if (bayerCropSize.w < hwBdsW || bayerCropSize.h < hwBdsH) {
        CLOGV("bayerCropSize %dx%d is smaller than BDSSize %dx%d. Force bayerCropSize",
                bayerCropSize.w, bayerCropSize.h, hwBdsW, hwBdsH);

        hwBdsW = bayerCropSize.w;
        hwBdsH = bayerCropSize.h;
    }

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = hwBdsW;
    dstRect->h = hwBdsH;

#ifdef DEBUG_PERFRAME
    CLOGD("hwBdsSize %dx%d", dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCameraParameters::getPreviewYuvCropSize(ExynosRect *yuvCropSize)
{
    status_t ret = NO_ERROR;
    ExynosRect previewBdsSize;
    ExynosRect previewYuvCropSize;
    ExynosRect cropRegion;
    int maxSensorW = 0, maxSensorH = 0;
    float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

    /* 1. Check the invalid parameter */
    if (yuvCropSize == NULL) {
        CLOGE("yuvCropSize is NULL");
        return BAD_VALUE;
    }

    /* 2. Get the BDS info & Zoom info */
    ret = this->getPreviewBdsSize(&previewBdsSize);
    if (ret != NO_ERROR) {
        CLOGE("getPreviewBdsSize failed");
        return ret;
    }

    if (isUseIspInputCrop() == true
        || isUseMcscInputCrop() == true)
        m_getCropRegion(&cropRegion.x, &cropRegion.y, &cropRegion.w, &cropRegion.h);

    getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);

    /* 3. Scale down the crop region to adjust with the original yuv size */
    scaleRatioX = (float) previewBdsSize.w / (float) maxSensorW;
    scaleRatioY = (float) previewBdsSize.h / (float) maxSensorH;
    cropRegion.x = (int) (cropRegion.x * scaleRatioX);
    cropRegion.y = (int) (cropRegion.y * scaleRatioY);
    cropRegion.w = (int) (cropRegion.w * scaleRatioX);
    cropRegion.h = (int) (cropRegion.h * scaleRatioY);

    if (cropRegion.w < 1 || cropRegion.h < 1) {
        cropRegion.w = previewBdsSize.w;
        cropRegion.h = previewBdsSize.h;
    }

    /* 4. Calculate the YuvCropSize with ZoomRatio */
#if defined(SCALER_MAX_SCALE_UP_RATIO)
    /*
     * After dividing float & casting int,
     * zoomed size can be smaller too much.
     * so, when zoom until max, ceil up about floating point.
     */
    if (ALIGN_UP(cropRegion.w, CAMERA_BCROP_ALIGN) * SCALER_MAX_SCALE_UP_RATIO < previewBdsSize.w
     || ALIGN_UP(cropRegion.h, 2) * SCALER_MAX_SCALE_UP_RATIO < previewBdsSize.h) {
        previewYuvCropSize.w = ALIGN_UP((int)ceil((float)previewBdsSize.w / SCALER_MAX_SCALE_UP_RATIO), CAMERA_BCROP_ALIGN);
        previewYuvCropSize.h = ALIGN_UP((int)ceil((float)previewBdsSize.h / SCALER_MAX_SCALE_UP_RATIO), 2);
    } else
#endif
    {
        previewYuvCropSize.w = ALIGN_UP(cropRegion.w, CAMERA_BCROP_ALIGN);
        previewYuvCropSize.h = ALIGN_UP(cropRegion.h, 2);
    }

    /* 4. Calculate the YuvCrop X-Y Offset Coordination & Set Result */
    if (previewBdsSize.w > previewYuvCropSize.w) {
        yuvCropSize->x = ALIGN_UP(((previewBdsSize.w - previewYuvCropSize.w) >> 1), CAMERA_MCSC_ALIGN);
        yuvCropSize->w = previewYuvCropSize.w;
    } else {
        yuvCropSize->x = 0;
        yuvCropSize->w = previewBdsSize.w;
    }
    if (previewBdsSize.h > previewYuvCropSize.h) {
        yuvCropSize->y = ALIGN_UP(((previewBdsSize.h - previewYuvCropSize.h) >> 1), 2);
        yuvCropSize->h = previewYuvCropSize.h;
    } else {
        yuvCropSize->y = 0;
        yuvCropSize->h = previewBdsSize.h;
    }

#ifdef DEBUG_PERFRAME
    CLOGD("BDS %dx%d cropRegion %d,%d %dx%d",
            previewBdsSize.w, previewBdsSize.h,
            yuvCropSize->x, yuvCropSize->y, yuvCropSize->w, yuvCropSize->h);
#endif

    return ret;
}

status_t ExynosCameraParameters::getPictureYuvCropSize(ExynosRect *yuvCropSize)
{
    status_t ret = NO_ERROR;
    float zoomRatio = 1.00f;
    ExynosRect bnsSize;
    ExynosRect pictureBayerCropSize;
    ExynosRect pictureBdsSize;
    ExynosRect ispInputSize;
    ExynosRect cropRegion;
    ExynosRect pictureYuvCropSize;

    /* 1. Check the invalid parameter */
    if (yuvCropSize == NULL) {
        CLOGE("yuvCropSize is NULL");
        return BAD_VALUE;
    }

    /* 2. Get the ISP input info & Zoom info */
    if (this->getUsePureBayerReprocessing() == true) {
        ret = this->getPictureBdsSize(&pictureBdsSize);
        if (ret != NO_ERROR) {
            CLOGE("getPictureBdsSize failed");
            return ret;
        }

        ispInputSize.x = 0;
        ispInputSize.y = 0;
        ispInputSize.w = pictureBdsSize.w;
        ispInputSize.h = pictureBdsSize.h;
    } else {
        ret = this->getPictureBayerCropSize(&bnsSize, &pictureBayerCropSize);
        if (ret != NO_ERROR) {
            CLOGE("getPictureBdsSize failed");
            return ret;
        }

        ispInputSize.x = 0;
        ispInputSize.y = 0;
        ispInputSize.w = pictureBayerCropSize.w;
        ispInputSize.h = pictureBayerCropSize.h;
    }

    /* Initialize cropRegion to ispInputSize */
    cropRegion = ispInputSize;

    if (isUseReprocessingIspInputCrop() == true
        || isUseReprocessingMcscInputCrop() == true)
    {
#ifdef SUPPORT_PICTURE_YUV_CROP
        /* Update cropRegion */
        int maxSensorW = 0, maxSensorH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

        getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);
        m_getPictureCropRegion(&cropRegion.x, &cropRegion.y, &cropRegion.w, &cropRegion.h);

        scaleRatioX = (float) ispInputSize.w / (float) maxSensorW;
        scaleRatioY = (float) ispInputSize.h / (float) maxSensorH;
        cropRegion.x = (int) (cropRegion.x * scaleRatioX);
        cropRegion.y = (int) (cropRegion.y * scaleRatioY);
        cropRegion.w = (int) (cropRegion.w * scaleRatioX);
        cropRegion.h = (int) (cropRegion.h * scaleRatioY);

        if (cropRegion.w < 1 || cropRegion.h < 1) {
            cropRegion.w = ispInputSize.w;
            cropRegion.h = ispInputSize.h;
        }

        CLOGE("cropRegion %d %d %d %d, ispInput %d %d %d %d",
                cropRegion.x, cropRegion.y, cropRegion.w, cropRegion.h,
                ispInputSize.x, ispInputSize.y, ispInputSize.w, ispInputSize.h);
#else
        /* TODO: Implement YUV crop for reprocessing */
        CLOGE("Picture YUV crop is NOT supported");
#endif
    }

    /* 3. Calculate the YuvCropSize with ZoomRatio */
#if defined(SCALER_MAX_SCALE_UP_RATIO)
    /*
     * After dividing float & casting int,
     * zoomed size can be smaller too much.
     * so, when zoom until max, ceil up about floating point.
     */
    if (ALIGN_UP((int)((float)cropRegion.w / zoomRatio), CAMERA_BCROP_ALIGN) * SCALER_MAX_SCALE_UP_RATIO < ispInputSize.w
     || ALIGN_UP((int)((float)cropRegion.h / zoomRatio), 2) * SCALER_MAX_SCALE_UP_RATIO < ispInputSize.h) {
        pictureYuvCropSize.w = ALIGN_UP((int)ceil((float)ispInputSize.w / zoomRatio), CAMERA_BCROP_ALIGN);
        pictureYuvCropSize.h = ALIGN_UP((int)ceil((float)ispInputSize.h / zoomRatio), 2);
    } else
#endif
    {
        pictureYuvCropSize.w = ALIGN_UP((int)((float)cropRegion.w / zoomRatio), CAMERA_BCROP_ALIGN);
        pictureYuvCropSize.h = ALIGN_UP((int)((float)cropRegion.h / zoomRatio), 2);
    }

    /* 4. Calculate the YuvCrop X-Y Offset Coordination & Set Result */
    if (ispInputSize.w > pictureYuvCropSize.w) {
        yuvCropSize->x = ALIGN_UP(((ispInputSize.w - pictureYuvCropSize.w) >> 1), CAMERA_MCSC_ALIGN);
        yuvCropSize->w = pictureYuvCropSize.w;
    } else {
        yuvCropSize->x = 0;
        yuvCropSize->w = ispInputSize.w;
    }
    if (ispInputSize.h > pictureYuvCropSize.h) {
        yuvCropSize->y = ALIGN_UP(((ispInputSize.h - pictureYuvCropSize.h) >> 1), 2);
        yuvCropSize->h = pictureYuvCropSize.h;
    } else {
        yuvCropSize->y = 0;
        yuvCropSize->h = ispInputSize.h;
    }

#ifdef DEBUG_PERFRAME
    CLOGD("ISPS %dx%d YuvCrop %d,%d %dx%d zoomRatio %f",
            ispInputSize.w, ispInputSize.h,
            yuvCropSize->x, yuvCropSize->y, yuvCropSize->w, yuvCropSize->h,
            zoomRatio);
#endif

    return ret;
}


void ExynosCameraParameters::CheckAfFovc(ExynosCameraFrameSP_sptr_t frame,int perframeIndex)
{
    struct camera2_node_group nodeGroupInfo;
    struct camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)frame->getConstMeta();

    int pipeId =-1;
    status_t ret = NO_ERROR;

    if (perframeIndex == PERFRAME_INFO_MCSC && isUseMcscInputCrop()== true)
        pipeId = PIPE_MCSC;
    else if (perframeIndex == PERFRAME_INFO_ISP)
        pipeId = PIPE_ISP;
    else
        return;

    if (isUseAfFovCrop((enum pipeline)pipeId, shot_ext) == true){
        ret = frame->getNodeGroupInfo(&nodeGroupInfo, perframeIndex);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getNodeGroupInfo. perframeIndex %d ret %d",
                    frame->getFrameCount(), perframeIndex, ret);
        }
        m_adjustAfFovCrop(this, &nodeGroupInfo, shot_ext);
        frame->storeNodeGroupInfo(&nodeGroupInfo, perframeIndex);
    }
}


bool ExynosCameraParameters::isUseAfFovCrop(enum pipeline pipeId, struct camera2_shot_ext *shot_ext)
{
    status_t ret = NO_ERROR;
    bool doCrop = false;
    bool isFocusable = false;
    bool recordingHint = false;
    bool isValidBdsSize = false;
    int recordingW = 0, recordingH = 0;
    ExynosRect bdsSize;
    int previewW = 0, previewH = 0;

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return false;
    }
    m_configurations->getSize(CONFIGURATION_VIDEO_SIZE, (uint32_t *)&recordingW, (uint32_t *)&recordingH);
    m_configurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);

    if (m_staticInfo->minimumFocusDistance > 0.0f)
        isFocusable = true;

    //Check record mode
    recordingHint = m_configurations->getMode(CONFIGURATION_VIDEO_MODE);

    //Check bds
    switch (pipeId) {
        case PIPE_ISP:
            ret = getPreviewBdsSize(&bdsSize);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Failed to getPreviewBdsSize",
                        __FUNCTION__, __LINE__);
                return false;
            }

            isValidBdsSize = (bdsSize.w >= recordingW && bdsSize.h >= recordingH && bdsSize.w >= previewW && bdsSize.h >= previewH);
            break;

        case PIPE_MCSC:
        default:
            isValidBdsSize = false;
            break;
    }

    doCrop = isFocusable && recordingHint && isValidBdsSize;

#ifdef DEBUG_PERFRAME
    CLOGD("DEBUG(%s[%d]):[Pipe%d] Do %sCrop for conserving AF FOV. "\
            "isFocusable %d recordingHint %d isValidBdsSize %d bdsSize %dx%d, Video: %dx%d",
            __FUNCTION__, __LINE__,
            pipeId,
            (doCrop == true)? "":"NOT ",
            isFocusable,
            recordingHint,
            isValidBdsSize,
            bdsSize.w, bdsSize.h,
            recordingW,
            recordingH);
#endif

    return doCrop;
}

status_t ExynosCameraParameters::getAfFovCropSize(int lensPosition, ExynosRect *cropRect, struct camera2_shot_ext *shot_ext)
{
    int minLensPosition = 0, maxLensPosition = 0;
    float Dist_macro_end = 0.0f;
    float focal_length = 0.0f;
    float fudge_factor = 0.0f;
    float lens_shift_max = 0.0f;
    float mag_factor_max = 0.0f;
    float lens_shift = 0.0f;
    float mag_factor = 0.0f;
    float final_mag_factor = 0.0f;
    ExynosRect bdsSize;
    status_t ret = NO_ERROR;
    int width, height;

    if (cropRect == NULL) {
        CLOGE("ERR(%s[%d]):cropRect is NULL",__FUNCTION__, __LINE__);
        return BAD_VALUE;
    } else if (m_staticInfo->afFovList == NULL) {
        CLOGE("ERR(%s[%d]):afFovList is NULL",__FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    minLensPosition = m_staticInfo->afFovList[0][0];
    maxLensPosition = m_staticInfo->afFovList[0][1];
    Dist_macro_end  = (float)(m_staticInfo->afFovList[0][2]/1000000.0f);
    fudge_factor    = (float)(m_staticInfo->afFovList[0][3]/1000000.0f);

    focal_length = shot_ext->shot.ctl.lens.focalLength;
    lens_shift_max = (float)(m_staticInfo->afFovList[0][4]/1000000.0f);
    mag_factor_max = (float)(m_staticInfo->afFovList[0][5]/1000000.0f);

    if (lensPosition < minLensPosition || lensPosition > maxLensPosition) {
        CLOGE("ERR(%s[%d]):Invalid lensPosition %d [%d, %d]",__FUNCTION__, __LINE__,
                lensPosition, minLensPosition, maxLensPosition);
        return BAD_VALUE;
    }

    lens_shift = ((float)(lensPosition-minLensPosition) / (float)(maxLensPosition-minLensPosition)) * (lens_shift_max - focal_length) + focal_length;
    mag_factor = (lens_shift / focal_length) * fudge_factor;
    if (mag_factor < 0.0f) {
        final_mag_factor = mag_factor_max;
    } else {
        final_mag_factor = mag_factor_max - mag_factor;
    }

    CLOGV("Debug(%s[%d]):Dist_macro_end(%f),focal_length(%f),fudge_factor(%f),lens_shift_max(%f),mag_factor_max(%f),lens_shift(%f),mag_factor(%f),final_mag_factor(%f),lensPosition(%d)",
            __FUNCTION__, __LINE__,Dist_macro_end,focal_length,fudge_factor,lens_shift_max,mag_factor_max,lens_shift,mag_factor,final_mag_factor,lensPosition);

    ret = getPreviewBdsSize(&bdsSize);

    if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to getPreviewBdsSize",
                    __FUNCTION__, __LINE__);
            return false;
        }

    width = bdsSize.w;
    height = bdsSize.h;

    cropRect->x = 0;
    cropRect->y = 0;
    cropRect->w = (int)(width * (1.0f - final_mag_factor));
    cropRect->h = (int)(height * (1.0f - final_mag_factor));

#ifdef DEBUG_PERFRAME
    CLOGD("DEBUG(%s[%d]):lensPosition(%d) BDS(%dx%d), adjustedCropSize %dx%d",
            __FUNCTION__, __LINE__,
            lensPosition,bdsSize.w,bdsSize.h,cropRect->w, cropRect->h);
#endif
    return NO_ERROR;
}

void  ExynosCameraParameters::m_adjustAfFovCrop(ExynosCameraParameters *params,
                                                camera2_node_group *node_group_info,
                                                struct camera2_shot_ext *shot_ext)
{
    if (params == NULL || node_group_info == NULL || shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):Bad parameters. params %p, node_group_info %p,  shot_ext %p . ",
                __FUNCTION__, __LINE__,
                params, node_group_info,shot_ext);
        return;
    }

    ExynosRect cropRegion;
    status_t ret = NO_ERROR;
    int32_t lensPosition;
    uint32_t vidcur, vidRequest, vidMP, vidMV;
    bool skipCrop = false;
    ExynosRect inputSize;

    lensPosition = shot_ext->shot.udm.af.lensPositionCurrent;
    ret = params->getAfFovCropSize(lensPosition, &cropRegion, shot_ext);
    if (ret != NO_ERROR) {
        ALOGV("ERR(%s[%d]):Failed to getAfFovCropSize. Skip to adjust",
                __FUNCTION__, __LINE__);
        skipCrop=true;
    }

    inputSize.x = node_group_info->leader.output.cropRegion[0];
    inputSize.y = node_group_info->leader.output.cropRegion[1];
    inputSize.w = node_group_info->leader.output.cropRegion[2];
    inputSize.h = node_group_info->leader.output.cropRegion[3];

    CLOGV("DEBUG(%s[%d]):leaderInputSize %dx%d , afCropSize %dx%d.",
            __FUNCTION__, __LINE__,
            inputSize.w, inputSize.h, cropRegion.w, cropRegion.h);

    if (inputSize.w < cropRegion.w || inputSize.h < cropRegion.h) {
        CLOGV("DEBUG(%s[%d]):leaderInputSize %dx%d < afCropSize %dx%d. Skip to adjust",
                __FUNCTION__, __LINE__,
                inputSize.w, inputSize.h, cropRegion.w, cropRegion.h);
        skipCrop = true;
    }

    vidMP = FIMC_IS_VIDEO_M0P_NUM+getPreviewPortId();
    vidMV = FIMC_IS_VIDEO_M0P_NUM+getRecordingPortId();

    for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
        vidcur=node_group_info->capture[i].vid + FIMC_IS_VIDEO_BAS_NUM;
        vidRequest = node_group_info->capture[i].request;

        if( (vidcur == vidMP || vidcur == vidMV) && (vidRequest == 1) ) {
            inputSize.x = node_group_info->capture[i].input.cropRegion[0];
            inputSize.y = node_group_info->capture[i].input.cropRegion[1];
            inputSize.w = node_group_info->capture[i].input.cropRegion[2];
            inputSize.h = node_group_info->capture[i].input.cropRegion[3];
            if(skipCrop) {
                cropRegion.x = 0;
                cropRegion.y = 0;
                cropRegion.w = inputSize.w;
                cropRegion.h = inputSize.h;
            } else {
                if (inputSize.w < cropRegion.w ||inputSize.h < cropRegion.h) {
                    CLOGV("DEBUG(%s[%d]):[V%d]inputSize %dx%d < afCropSize %dx%d. Adjust it.",
                            __FUNCTION__, __LINE__,
                            node_group_info->capture[i].vid,
                            inputSize.w, inputSize.h, cropRegion.w, cropRegion.h);
                    if (inputSize.w < cropRegion.w) {
                        cropRegion.w = inputSize.w;
                    }
                    if (inputSize.h < cropRegion.h) {
                        cropRegion.h = inputSize.h;
                    }
                    getCropRectAlign(inputSize.w, inputSize.h, cropRegion.w, cropRegion.h,
                            &cropRegion.x, &cropRegion.y, &cropRegion.w, &cropRegion.h, CAMERA_MCSC_ALIGN, 1, 1.0f);
                    CLOGV("DEBUG(%s[%d]):[V%d] after ratio adjust ( %d, %d ,%dx%d ). ",
                            __FUNCTION__, __LINE__,
                            node_group_info->capture[i].vid,
                            cropRegion.x, cropRegion.y, cropRegion.w, cropRegion.h);
                }

                cropRegion.x = ALIGN_UP((inputSize.w - cropRegion.w) / 2, CAMERA_MCSC_ALIGN);
                cropRegion.y = (inputSize.h - cropRegion.h) / 2;
                cropRegion.w = inputSize.w - (cropRegion.x * 2);
                cropRegion.h = inputSize.h - (cropRegion.y * 2);
            }

            node_group_info->capture[i].input.cropRegion[0] = inputSize.x + cropRegion.x;
            node_group_info->capture[i].input.cropRegion[1] = inputSize.y + cropRegion.y;
            node_group_info->capture[i].input.cropRegion[2] = cropRegion.w;
            node_group_info->capture[i].input.cropRegion[3] = cropRegion.h;

#ifdef DEBUG_PERFRAME_SIZE
            CLOGV("DEBUG(%s[%d]):[V%d]inputCropRegion %d %d %d %d lensPosition %d",
                    __FUNCTION__, __LINE__,
                    node_group_info->capture[i].vid,
                    node_group_info->capture[i].input.cropRegion[0],
                    node_group_info->capture[i].input.cropRegion[1],
                    node_group_info->capture[i].input.cropRegion[2],
                    node_group_info->capture[i].input.cropRegion[3],
                    lensPosition);
#endif
        }
    }

    return;
}

bool ExynosCameraParameters::isStillShotSensorFpsSupported(void)
{
    return m_staticInfo->stillShotSensorFpsSupport;
}

int ExynosCameraParameters::getStillShotSensorFps(void)
{
    return m_staticInfo->stillShotSensorFps;
}

status_t ExynosCameraParameters::getFastenAeStableSensorSize(int *hwSensorW, int *hwSensorH, int index)
{
    *hwSensorW = m_staticInfo->fastAeStableLut[index][SENSOR_W];
    *hwSensorH = m_staticInfo->fastAeStableLut[index][SENSOR_H];

    return NO_ERROR;
}

status_t ExynosCameraParameters::getFastenAeStableBcropSize(int *hwBcropW, int *hwBcropH, int index)
{
    int BcropW = 0;
    int BcropH = 0;
    float zoomRatio = m_configurations->getZoomRatio();

    BcropW = m_staticInfo->fastAeStableLut[index][BCROP_W];
    BcropH = m_staticInfo->fastAeStableLut[index][BCROP_H];

    if (zoomRatio != 1.0f) {
#if defined(SCALER_MAX_SCALE_UP_RATIO)
        /*
         * After dividing float & casting int,
         * zoomed size can be smaller too much.
         * so, when zoom until max, ceil up about floating point.
         */
        if (ALIGN_UP((int)((float)BcropW / zoomRatio), CAMERA_BCROP_ALIGN) * SCALER_MAX_SCALE_UP_RATIO < BcropW ||
            ALIGN_UP((int)((float)BcropH / zoomRatio), 2) * SCALER_MAX_SCALE_UP_RATIO < BcropH) {
            BcropW = ALIGN_UP((int)ceil((float)BcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            BcropH = ALIGN_UP((int)ceil((float)BcropH / zoomRatio), 2);
        } else
#endif
        {
            BcropW = ALIGN_UP((int)((float)BcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            BcropH = ALIGN_UP((int)((float)BcropH / zoomRatio), 2);
        }
    }

    if(BcropW < 320 || BcropH < 240) {
        *hwBcropW = 320;
        *hwBcropH = 240;
    } else {
        if (BcropW > m_staticInfo->fastAeStableLut[index][BCROP_W]) {
            BcropW = m_staticInfo->fastAeStableLut[index][BCROP_W];
        }
        *hwBcropW = BcropW;

        if (BcropH > m_staticInfo->fastAeStableLut[index][BCROP_H]) {
            BcropH = m_staticInfo->fastAeStableLut[index][BCROP_H];
        }
        *hwBcropH = BcropH;
    }

    return NO_ERROR;
}

status_t ExynosCameraParameters::getFastenAeStableBdsSize(int *hwBdsW, int *hwBdsH, int index)
{
    *hwBdsW = m_staticInfo->fastAeStableLut[index][BDS_W];
    *hwBdsH = m_staticInfo->fastAeStableLut[index][BDS_H];

    return NO_ERROR;
}

status_t ExynosCameraParameters::calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    status_t ret = NO_ERROR;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
    int pictureW = 0, pictureH = 0;
    int bayerFormat = getBayerFormat(PIPE_3AA_REPROCESSING);

    // HACK: Need to add elegant variable whether it use HW_PICTURE or PICTURE size.
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    } else {
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
    }
    ret = getPictureBayerCropSize(&bnsSize, &bayerCropSize);
    if (ret != NO_ERROR)
        CLOGE("Failed to getPictureBayerCropSize()");

    srcRect->x = bayerCropSize.x;
    srcRect->y = bayerCropSize.y;
    srcRect->w = bayerCropSize.w;
    srcRect->h = bayerCropSize.h;
    srcRect->fullW = bnsSize.w;
    srcRect->fullH = bnsSize.h;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = pictureW;
    dstRect->h = pictureH;
    dstRect->fullW = pictureW;
    dstRect->fullH = pictureH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    /* Check the invalid BDS size compared to Bcrop size */
    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

#ifdef DEBUG_PERFRAME
    CLOGE("hwBdsSize %dx%d Picture %dx%d", dstRect->w, dstRect->h, pictureW, pictureH);
#endif

    return NO_ERROR;
}

bool ExynosCameraParameters::getUsePureBayerRemosaicReprocessing(void)
{
    bool pure = true;
#ifdef SUPPORT_REMOSAIC_CAPTURE
    pure = USE_PURE_BAYER_REMOSAIC_REPROCESSING;
#endif
    return pure;
}

bool ExynosCameraParameters::getUsePureBayerReprocessing(void)
{
    bool usePureBayerReprocessing = m_usePureBayerReprocessing;
    int cameraId = getCameraId();

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true) {
#ifdef USE_DUAL_CAMERA
        if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
            if (isBackCamera(cameraId)) {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_ON_DUAL_RECORDING;
            } else {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL_RECORDING;
            }
        } else
#endif
        if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
            usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT_ON_PIP_RECORDING;
        } else if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true) {
            usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_ON_PIP_RECORDING;
        } else {
            if (isBackCamera(cameraId)) {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_ON_RECORDING;
            } else {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING;
            }
        }
    } else {
#ifdef USE_DUAL_CAMERA
        if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
            if (isBackCamera(cameraId)) {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_ON_DUAL;
            } else {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL;
            }
        } else
#endif
        if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
            usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT_ON_PIP;
        } else if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true) {
            usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_ON_PIP;
        } else {
            if (isBackCamera(cameraId)) {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING;
            } else {
                usePureBayerReprocessing = USE_PURE_BAYER_REPROCESSING_FRONT;
            }
        }
    }

    m_getVendorUsePureBayerReprocessing(usePureBayerReprocessing);

    if (usePureBayerReprocessing != m_usePureBayerReprocessing) {
        CLOGD("bayer usage is changed (%d -> %d)", m_usePureBayerReprocessing, usePureBayerReprocessing);
        m_usePureBayerReprocessing = usePureBayerReprocessing;
    }

#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (m_usePureBayerReprocessing != true || m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_REMOSAIC)) {
        CLOGV("Please check current capture type is remosaic or not," \
                "remosaic capture is pure bayer reprocessing");
    }
#endif //SUPPORT_REMOSAIC_CAPTURE

    return m_usePureBayerReprocessing;
}

int32_t ExynosCameraParameters::getReprocessingBayerMode(void)
{
    int32_t mode = REPROCESSING_BAYER_MODE_NONE;
    bool useDynamicBayer = m_configurations->getMode(CONFIGURATION_DYNAMIC_BAYER_MODE);

    if (isReprocessing() == false)
        return mode;

    if (useDynamicBayer == true) {
        if (getUsePureBayerReprocessing() == true)
            mode = REPROCESSING_BAYER_MODE_PURE_DYNAMIC;
        else
            mode = REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC;
    } else {
        if (getUsePureBayerReprocessing() == true)
            mode = REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON;
        else
            mode = REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON;
    }

    return mode;
}

bool ExynosCameraParameters::getSensorOTFSupported(void)
{
    return m_staticInfo->flite3aaOtfSupport;
}

bool ExynosCameraParameters::isReprocessing(void)
{
    bool reprocessing = false;
    int cameraId = getCameraId();
    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
    bool flagDual = false;
#ifdef USE_DUAL_CAMERA
    flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
#endif

    /* No reprocessing when BDS off scenario */
    if (isUseVideoHQISP() == true)
        return false;

    if (flagPIP) {
        if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#if defined(FRONT_CAMERA_DUAL_REPROCESSING) //PIP SUB camera
            reprocessing = FRONT_CAMERA_DUAL_REPROCESSING;
#else
            CLOGW("PIP: FRONT_CAMERA_DUAL_REPROCESSING is not defined");
#endif
        } else {
#if defined(MAIN_CAMERA_DUAL_REPROCESSING)
            reprocessing = MAIN_CAMERA_DUAL_REPROCESSING;
#else
            CLOGW("PIP: MAIN_CAMERA_DUAL_REPROCESSING is not defined");
#endif
        }
        return reprocessing;
    }

    if (flagDual) {
        //TODO: is it back/front or main/sub????
        if(isBackCamera(cameraId)) {
#if defined(MAIN_CAMERA_DUAL_REPROCESSING)
            reprocessing = MAIN_CAMERA_DUAL_REPROCESSING;
#else
            CLOGW("DUAL: MAIN_CAMERA_DUAL_REPROCESSING is not defined");
#endif
        } else {
#if defined(FRONT_CAMERA_DUAL_REPROCESSING) //front or SUB camera ???
            reprocessing = FRONT_CAMERA_DUAL_REPROCESSING;
#else
            CLOGW("PIP: FRONT_CAMERA_DUAL_REPROCESSING is not defined");
#endif
        }
        return reprocessing;
    }

    if (!flagPIP && !flagDual) { //this check is for readability
        if (isBackCamera(cameraId)) {
#if defined(MAIN_CAMERA_SINGLE_REPROCESSING)
        reprocessing = MAIN_CAMERA_SINGLE_REPROCESSING;
#else
        CLOGW(" MAIN_CAMERA_SINGLE_REPROCESSING is not defined");
#endif
        } else {
#if defined(FRONT_CAMERA_SINGLE_REPROCESSING)
            reprocessing = FRONT_CAMERA_SINGLE_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_REPROCESSING is not defined");
#endif
        }
        return reprocessing;
    }

    return reprocessing;
}

bool ExynosCameraParameters::isSccCapture(void)
{
    bool sccCapture = false;
    int cameraId = getCameraId();
    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
    bool flagDual = false;

#ifdef USE_DUAL_CAMERA
    flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
#endif

    if(flagPIP) {
        if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#if defined(FRONT_CAMERA_DUAL_SCC_CAPTURE) //PIP SUB camera
            sccCapture = FRONT_CAMERA_DUAL_SCC_CAPTURE;
#else
            CLOGW("PIP: FRONT_CAMERA_DUAL_SCC_CAPTURE is not defined");
#endif
        } else {
#if defined(MAIN_CAMERA_DUAL_SCC_CAPTURE)
            sccCapture = MAIN_CAMERA_DUAL_SCC_CAPTURE;
#else
            CLOGW("PIP: MAIN_CAMERA_DUAL_SCC_CAPTURE is not defined");
#endif
        }
        return sccCapture;
    }

    if (flagDual) {
    //TODO: is it back/front or main/sub????
        if(isBackCamera(cameraId)) {
#if defined(MAIN_CAMERA_DUAL_SCC_CAPTURE)
            sccCapture = MAIN_CAMERA_DUAL_SCC_CAPTURE;
#else
            CLOGW("DUAL: MAIN_CAMERA_DUAL_SCC_CAPTURE is not defined");
#endif
        } else {
#if defined(FRONT_CAMERA_DUAL_SCC_CAPTURE) //front or SUB camera ???
            sccCapture = FRONT_CAMERA_DUAL_SCC_CAPTURE;
#else
            CLOGW("PIP: FRONT_CAMERA_DUAL_SCC_CAPTURE is not defined");
#endif
        }
        return sccCapture;
    }

    if (!flagPIP && !flagDual) { //this check is for readability
        if (isBackCamera(cameraId)) {
#if defined(MAIN_CAMERA_SINGLE_SCC_CAPTURE)
            sccCapture = MAIN_CAMERA_SINGLE_SCC_CAPTURE;
#else
            CLOGW(" MAIN_CAMERA_SINGLE_SCC_CAPTURE is not defined");
#endif
        } else {
#if defined(FRONT_CAMERA_SINGLE_SCC_CAPTURE)
            sccCapture = FRONT_CAMERA_SINGLE_SCC_CAPTURE;
#else
            CLOGW("FRONT_CAMERA_SINGLE_SCC_CAPTURE is not defined");
#endif
        }
        return sccCapture;
    }

    return sccCapture;
}


/* True if private reprocessing or YUV reprocessing is supported */
bool ExynosCameraParameters::isSupportZSLInput(void) {
    if (m_staticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING
        || m_staticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING) {
        return true;
    }

    return false;
}

enum HW_CHAIN_TYPE ExynosCameraParameters::getHwChainType(void)
{
#ifdef TYPE_OF_HW_CHAINS
    return TYPE_OF_HW_CHAINS;
#else
    return HW_CHAIN_TYPE_DUAL_CHAIN; /* default */
#endif
}

uint32_t ExynosCameraParameters::getNumOfMcscInputPorts(void)
{
#ifdef NUM_OF_MCSC_INPUT_PORTS
    return NUM_OF_MCSC_INPUT_PORTS;
#else
    /* default : 2 input */
    return 2;
#endif
}

uint32_t ExynosCameraParameters::getNumOfMcscOutputPorts(void)
{
#ifdef NUM_OF_MCSC_OUTPUT_PORTS
    return NUM_OF_MCSC_OUTPUT_PORTS;
#else
    /* default : 5 output */
    return 5;
#endif
}

enum HW_CONNECTION_MODE ExynosCameraParameters::getHwConnectionMode(
        enum pipeline srcPipeId,
        enum pipeline dstPipeId)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

        switch (srcPipeId) {
        case PIPE_FLITE:
            if (dstPipeId != PIPE_3AA) {
                goto INVALID_PIPE_ID;
            }

            hwConnectionMode = m_getFlite3aaOtf();
            break;
#ifdef USE_PAF
        case PIPE_PAF:
            if (dstPipeId != PIPE_3AA) {
                goto INVALID_PIPE_ID;
            }

            hwConnectionMode = m_getPaf3aaOtf();
            break;
#endif
        case PIPE_3AA:
            if ((dstPipeId != PIPE_ISP) && (dstPipeId != PIPE_VRA)) {
                goto INVALID_PIPE_ID;
            }

            if (dstPipeId == PIPE_ISP) {
                hwConnectionMode = m_get3aaIspOtf();
            } else if (dstPipeId == PIPE_VRA) {
                hwConnectionMode = m_get3aaVraOtf();
            }
            break;
        case PIPE_ISP:
            if (dstPipeId != PIPE_MCSC) {
                goto INVALID_PIPE_ID;
            }

            hwConnectionMode = m_getIspMcscOtf();
            break;
        case PIPE_MCSC:
            if ((dstPipeId != PIPE_VRA)
#ifdef USE_CLAHE_PREVIEW
                && (dstPipeId != PIPE_CLAHE)
#endif
                ) {
                goto INVALID_PIPE_ID;
            }

            if (dstPipeId == PIPE_VRA) {
                hwConnectionMode = m_getMcscVraOtf();
            }
#ifdef USE_CLAHE_PREVIEW
            else if (dstPipeId == PIPE_CLAHE) {
                hwConnectionMode = HW_CONNECTION_MODE_M2M;
            }
#endif
            break;

#ifdef USE_PAF
        case PIPE_PAF_REPROCESSING:
            if (dstPipeId != PIPE_3AA_REPROCESSING) {
                goto INVALID_PIPE_ID;
            }
            hwConnectionMode = m_getReprocessingPaf3AAOtf();
            break;
#endif

        case PIPE_3AA_REPROCESSING:
            if ((dstPipeId != PIPE_ISP_REPROCESSING) && (dstPipeId != PIPE_VRA_REPROCESSING)) {
                goto INVALID_PIPE_ID;
            }

            if (dstPipeId == PIPE_ISP_REPROCESSING) {
                hwConnectionMode = m_getReprocessing3aaIspOtf();
            } else if (dstPipeId == PIPE_VRA_REPROCESSING) {
                hwConnectionMode = m_getReprocessing3aaVraOtf();
            }
            break;
        case PIPE_ISP_REPROCESSING:
            if (dstPipeId != PIPE_MCSC_REPROCESSING) {
                goto INVALID_PIPE_ID;
            }

            hwConnectionMode = m_getReprocessingIspMcscOtf();
            break;
        case PIPE_MCSC_REPROCESSING:
            if ((dstPipeId != PIPE_VRA_REPROCESSING)
#ifdef USE_CLAHE_REPROCESSING
                && (dstPipeId != PIPE_CLAHE_REPROCESSING)
#endif
                ) {
                goto INVALID_PIPE_ID;
            }

            if (dstPipeId == PIPE_VRA_REPROCESSING) {
                hwConnectionMode = m_getReprocessingMcscVraOtf();
            }
#ifdef USE_CLAHE_REPROCESSING
            else if (dstPipeId == PIPE_CLAHE_REPROCESSING) {
                hwConnectionMode = HW_CONNECTION_MODE_M2M/*m_getReprocessingMcscClaheOtf()*/;
            }
#endif
            break;
        default:
            goto INVALID_PIPE_ID;
            break;
        }

    return hwConnectionMode;

INVALID_PIPE_ID:
    CLOGE("Invalid pipe ID src(%d), dst(%d)", srcPipeId, dstPipeId);
    hwConnectionMode = HW_CONNECTION_MODE_NONE;
    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getFlite3aaOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif


#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {

    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_FLITE_3AA_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_FLITE_3AA_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_FLITE_3AA_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_FLITE_3AA_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_FLITE_3AA_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_FLITE_3AA_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_FLITE_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_FLITE_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_FLITE_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_FLITE_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
        break;

    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_FLITE_3AA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_FLITE_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_FLITE_3AA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_FLITE_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getPaf3aaOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

#if 0
    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

#if 0
    if (m_scenario == SCENARIO_SECURE_VISION) {
        hwConnectionMode = HW_CONNECTION_MODE_M2M;
        return hwConnectionMode;
    }
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {

    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_PAF_3AA_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_PAF_3AA_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_PAF_3AA_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_PAF_3AA_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_PAF_3AA_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_PAF_3AA_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_PAF_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_PAF_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_PAF_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_PAF_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_PAF_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_PAF_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_PAF_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_PAF_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_PAF_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_PAF_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_PAF_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_PAF_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_PAF_3AA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_PAF_3AA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_PAF_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_PAF_3AA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_PAF_3AA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_PAF_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_PAF_3AA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_PAF_3AA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_PAF_3AA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_PAF_3AA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_PAF_3AA_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_PAF_3AA_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }
#endif

    return hwConnectionMode;
}


enum HW_CONNECTION_MODE ExynosCameraParameters::m_get3aaVraOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

#ifdef USE_ALWAYS_FD_OFF
    return HW_CONNECTION_MODE_NONE;
#endif

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {

    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_3AA_VRA_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_3AA_VRA_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_3AA_VRA_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_3AA_VRA_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_3AA_VRA_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_3AA_VRA_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_3AA_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_3AA_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_3AA_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_3AA_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_3AA_VRA_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_3AA_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_VRA_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_3AA_PIP_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_VRA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_VRA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_VRA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_VRA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_VRA_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_3AA_PIP_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_get3aaIspOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_3AA_ISP_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_3AA_ISP_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_3AA_ISP_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_3AA_ISP_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_3AA_ISP_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_3AA_ISP_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }


    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF
            if (m_configurations->getConfigMode() > CONFIG_MODE::HIGHSPEED_60) {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_FULLOTF
                hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_FULLOTF;
#else
                hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_OTF;
#endif
            } else {
                hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_OTF;
            }
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_ISP_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_ISP_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getIspMcscOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_ISP_MCSC_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_ISP_MCSC_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_ISP_MCSC_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_ISP_MCSC_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_ISP_MCSC_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_ISP_MCSC_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }
    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_ISP_MCSC_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_ISP_MCSC_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_ISP_MCSC_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_ISP_MCSC_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_ISP_MCSC_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_ISP_MCSC_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_ISP_MCSC_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_ISP_MCSC_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_ISP_MCSC_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_ISP_MCSC_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_ISP_MCSC_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_ISP_MCSC_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_ISP_MCSC_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_ISP_MCSC_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_ISP_MCSC_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_ISP_MCSC_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_ISP_MCSC_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF;
#else
            CLOGW("FRONT_CAMERA_SINGLE_ISP_MCSC_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getMcscVraOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

#ifdef USE_ALWAYS_FD_OFF
    return HW_CONNECTION_MODE_NONE;
#endif

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_MCSC_VRA_OTF  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_MCSC_VRA_OTF;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_MCSC_VRA_OTF is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_MCSC_VRA_OTF
                hwConnectionMode = MAIN_CAMERA_PIP_MCSC_VRA_OTF;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_MCSC_VRA_OTF is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }
    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_MCSC_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_DUAL_MCSC_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_DUAL_MCSC_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_MCSC_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_MCSC_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_TPU_MCSC_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_MCSC_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_DUAL_MCSC_VRA_OTF;
#else
            CLOGW("FRONT_CAMERA_DUAL_MCSC_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_MCSC_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_MCSC_VRA_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_MCSC_PIP_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_MCSC_VRA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_MCSC_VRA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_MCSC_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_MCSC_VRA_OTF
            hwConnectionMode = MAIN_CAMERA_SINGLE_MCSC_VRA_OTF;
#else
            CLOGW("MAIN_CAMERA_SINGLE_TPU_MCSC_OTF is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true && dualPreviewMode != DUAL_PREVIEW_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_MCSC_VRA_OTF
            hwConnectionMode = SUB_CAMERA_DUAL_MCSC_VRA_OTF;
#else
            CLOGW("SUB_CAMERA_DUAL_MCSC_VRA_OTF is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_MCSC_VRA_OTF
            hwConnectionMode = FRONT_CAMERA_SINGLE_MCSC_VRA_OTF;
#else
            CLOGW(" FRONT_CAMERA_SINGLE_MCSC_PIP_OTF is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getReprocessingPaf3AAOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_REPROCESSING_MODE dualReprocessingMode = m_configurations->getDualReprocessingMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING
                hwConnectionMode = MAIN_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }
    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getReprocessing3aaIspOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_REPROCESSING_MODE dualReprocessingMode = m_configurations->getDualReprocessingMode();
#endif
#ifdef SUPPORT_REMOSAIC_CAPTURE
    bool flagRemosaic = m_configurations->getMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE);
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING
                hwConnectionMode = MAIN_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef SUPPORT_REMOSAIC_CAPTURE
        if (flagRemosaic == true) {
#ifdef MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef SUPPORT_REMOSAIC_CAPTURE
        if (flagRemosaic == true) {
#ifdef FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
        break;

    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef SUPPORT_REMOSAIC_CAPTURE
        if (flagRemosaic == true) {
#ifdef MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef SUPPORT_REMOSAIC_CAPTURE
        if (flagRemosaic == true) {
#ifdef FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    if (hwConnectionMode != HW_CONNECTION_MODE_M2M
        && getUsePureBayerReprocessing() == false) {
        CLOGW("Processed bayer must using 3AA-ISP M2M. but, Current mode(%d)",
                (int)hwConnectionMode);

        hwConnectionMode = HW_CONNECTION_MODE_M2M;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getReprocessing3aaVraOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

#if defined(USE_ALWAYS_FD_OFF) || !defined(USE_EARLY_FD_REPROCES)
    return HW_CONNECTION_MODE_NONE;
#endif

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_REPROCESSING_MODE dualReprocessingMode = m_configurations->getDualReprocessingMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING
                hwConnectionMode = MAIN_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }
    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;

    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getReprocessingIspMcscOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_REPROCESSING_MODE dualReprocessingMode = m_configurations->getDualReprocessingMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {

    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING
                hwConnectionMode = MAIN_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        }
        break;

    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

enum HW_CONNECTION_MODE ExynosCameraParameters::m_getReprocessingMcscVraOtf(void)
{
    enum HW_CONNECTION_MODE hwConnectionMode = HW_CONNECTION_MODE_NONE;

#ifndef USE_VRA_FD
    return  HW_CONNECTION_MODE_NONE;
#endif

    bool flagPIP = m_configurations->getMode(CONFIGURATION_PIP_MODE);
#ifdef USE_DUAL_CAMERA
    bool flagDual = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_REPROCESSING_MODE dualReprocessingMode = m_configurations->getDualReprocessingMode();
#endif

#ifdef USE_DUAL_CAMERA
    if (flagDual == true && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {

    } else
#endif
    {
        if (flagPIP == true) {
            if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
#ifdef FRONT_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING  //PIP SUB Cameera
                hwConnectionMode = FRONT_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING;
#else
                CLOGW("PIP: FRONT_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
            } else {
#ifdef MAIN_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING
                hwConnectionMode = MAIN_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING;
#else
                CLOGW("PIP: MAIN_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
            }
            return hwConnectionMode;
        }
    }

    switch (getCameraId()) {
    case CAMERA_ID_BACK:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef MAIN_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef FRONT_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;

    case CAMERA_ID_BACK_2:
    case CAMERA_ID_BACK_3:
    case CAMERA_ID_BACK_4:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    case CAMERA_ID_FRONT_2:
#ifdef USE_DUAL_CAMERA
        if (flagDual == true
            && dualReprocessingMode != DUAL_REPROCESSING_MODE_OFF) {
#ifdef SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        } else
#endif /* USE_DUAL_CAMERA */
        {
#ifdef FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING
            hwConnectionMode = FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING;
#else
            CLOGW("FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING is not defined");
#endif
        }
        break;
    default:
        CLOGE("Invalid camera ID(%d)", getCameraId());
        hwConnectionMode = HW_CONNECTION_MODE_NONE;
        break;
    }

    return hwConnectionMode;
}

bool ExynosCameraParameters::isUse3aaInputCrop(void)
{
#ifdef USE_3AA_INPUT_CROP
    return USE_3AA_INPUT_CROP;
#else
    return true;
#endif
}

bool ExynosCameraParameters::isUseBayerCompression(void)
{
    bool enableBayerCompression = false;

#ifdef BOARD_CAMERA_USES_BAYER_COMPRESSION
    if (isUseVideoHQISP() == true)
        enableBayerCompression = true;
#endif

    return enableBayerCompression;
}

/* return value is true when previous and new mode is not same */
bool ExynosCameraParameters::setVideoStreamExistStatus(bool mode) {
    bool isChanged = false;

    if (m_videoStreamExist != mode)
        isChanged = true;

    m_videoStreamExist = mode;
 
    return isChanged;
}

/* Check scenario whether BDS off or not */
bool ExynosCameraParameters::check3aaBDSOff(void)
{
#ifdef USE_BDS_OFF
    bool bdsoff = false;
    int configMode;

#ifdef BDS_OFF_ALWAYS
    return true;
#endif


#ifdef BDS_OFF_VIDEO
    configMode = m_configurations->getConfigMode();

    if (configMode == CONFIG_MODE::NORMAL)
        bdsoff = true;
    else
        bdsoff = false;
#endif

#ifdef USE_SUPER_EIS
    if (m_configurations->getMode(CONFIGURATION_SUPER_EIS_MODE) == true) {
        uint32_t videoW, videoH;
        m_configurations->getSize(CONFIGURATION_VIDEO_SIZE, &videoW, &videoH);
        if (videoW == 1920 && videoH == 1080) {
            bdsoff = true;
        }
    }
#endif


    return bdsoff;
#else
    return false;
#endif
}

bool ExynosCameraParameters::isUseVideoHQISP(void)
{
    return false;
}

bool ExynosCameraParameters::isUse3aaBDSOff(void)
{
    if (m_videoStreamExist == true && check3aaBDSOff() == true)
        return true;
    else
        return false;
}

bool ExynosCameraParameters::isUseIspInputCrop(void)
{
#ifdef USE_ISP_INPUT_CROP
    return USE_ISP_INPUT_CROP;
#else
    if (isUse3aaInputCrop() == true
        || m_get3aaIspOtf() != HW_CONNECTION_MODE_M2M)
        return false;
    else
        return true;
#endif
}

bool ExynosCameraParameters::isUseMcscInputCrop(void)
{
#ifdef USE_MCSC_INPUT_CROP
    return USE_MCSC_INPUT_CROP;
#else
    if (isUse3aaInputCrop() == true
        || isUseIspInputCrop() == true
        || m_getIspMcscOtf() != HW_CONNECTION_MODE_M2M)
        return false;
    else
        return true;
#endif
}

bool ExynosCameraParameters::isUseReprocessing3aaInputCrop(void)
{
#ifdef USE_REPROCESSING_3AA_INPUT_CROP
    return USE_REPROCESSING_3AA_INPUT_CROP;
#else
    return true;
#endif
}

bool ExynosCameraParameters::isUseReprocessingIspInputCrop(void)
{
#ifdef USE_REPROCESSING_ISP_INPUT_CROP
    return USE_REPROCESSING_ISP_INPUT_CROP;
#else
    if (isUseReprocessing3aaInputCrop() == true
        || m_getReprocessing3aaIspOtf() != HW_CONNECTION_MODE_M2M)
        return false;
    else
        return true;
#endif
}

bool ExynosCameraParameters::isUseReprocessingMcscInputCrop(void)
{
#ifdef USE_REPROCESSING_MCSC_INPUT_CROP
    return USE_REPROCESSING_MCSC_INPUT_CROP;
#else
    if (isUseReprocessing3aaInputCrop() == true
        || isUseReprocessingIspInputCrop() == true
        || m_getReprocessingIspMcscOtf() != HW_CONNECTION_MODE_M2M)
        return false;
    else
        return true;
#endif
}

bool ExynosCameraParameters::isUseEarlyFrameReturn(void)
{
#if defined(USE_EARLY_FRAME_RETURN)
    return true;
#else
    return false;
#endif
}

bool ExynosCameraParameters::isUse3aaDNG(void)
{
#if defined(USE_3AA_DNG)
        return USE_3AA_DNG;
#else
        return false;
#endif
}

bool ExynosCameraParameters::isUseHWFC(void)
{
    /* Don't use HWFC when Video and Jpeg in same stream */
    if (isUse3aaBDSOff() == true)
        return false;

#if defined(USE_JPEG_HWFC)
    return USE_JPEG_HWFC;
#else
    return false;
#endif
}

bool ExynosCameraParameters::isHWFCOnDemand(void)
{
#if defined(USE_JPEG_HWFC_ONDEMAND)
    return USE_JPEG_HWFC_ONDEMAND;
#else
    return false;
#endif
}

bool ExynosCameraParameters::isUseRawReverseReprocessing(void)
{
#ifdef USE_RAW_REVERSE_PROCESSING
    bool isSupportRaw = false;
    isSupportRaw = (m_staticInfo->supportedCapabilities & CAPABILITIES_RAW);

    if ((isUse3aaDNG() == false) &&
        (isSupportRaw == true)) {
         return true;
    } else {
         return false;
    }
#else
    return false;
#endif
}

struct ExynosCameraSensorInfoBase *ExynosCameraParameters::getSensorStaticInfo()
{
    return m_staticInfo;
}

const struct ExynosCameraSensorInfoBase *ExynosCameraParameters::getOtherSensorStaticInfo(int camIdx)
{
    int otherCameraId = m_camIdInfo->cameraId[m_camIdInfo->sensorInfoCamIdx];

    ExynosCameraParameters *otherParam = m_configurations->getParameters(otherCameraId);
    const struct ExynosCameraSensorInfoBase *ret = otherParam->getSensorStaticInfo();

    return ret;
}

bool ExynosCameraParameters::needGSCForCapture(int camId)
{
    if (isBackCamera(camId)) {
        return USE_GSC_FOR_CAPTURE_BACK;
    } else {
        return USE_GSC_FOR_CAPTURE_FRONT;
    }
}

bool ExynosCameraParameters::getSetFileCtlMode(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL
    return true;
#else
    return false;
#endif
}

bool ExynosCameraParameters::getSetFileCtl3AA(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_3AA
    return SET_SETFILE_BY_SET_CTRL_3AA;
#else
    return false;
#endif
}

bool ExynosCameraParameters::getSetFileCtlISP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_ISP
    return SET_SETFILE_BY_SET_CTRL_ISP;
#else
    return false;
#endif
}

int32_t ExynosCameraParameters::getYuvStreamMaxNum(void)
{
    int32_t yuvStreamMaxNum = -1;

    if (m_staticInfo == NULL) {
        CLOGE("m_staticInfo is NULL");

        return INVALID_OPERATION;
    }

    yuvStreamMaxNum = m_staticInfo->maxNumOutputStreams[PROCESSED];
    if (yuvStreamMaxNum < 0) {
        CLOGE("Invalid MaxNumOutputStreamsProcessed %d", yuvStreamMaxNum);
        return BAD_VALUE;
    }

    return yuvStreamMaxNum;
}

int32_t ExynosCameraParameters::getInputStreamMaxNum(void)
{
    int32_t inputStreamMaxNum = -1;

    if (m_staticInfo == NULL) {
        CLOGE("m_staticInfo is NULL");

        return INVALID_OPERATION;
    }

    inputStreamMaxNum = m_staticInfo->maxNumInputStreams;
    if (inputStreamMaxNum < 0) {
        CLOGE("Invalid MaxNumInputStreams %d", inputStreamMaxNum);
        return BAD_VALUE;
    }

    return inputStreamMaxNum;
}

int ExynosCameraParameters::getMaxHighSpeedFps(void)
{
   int maxFps = 0;
   int (*sizeList)[2];

   if (m_staticInfo->highSpeedVideoFPSList != NULL) {
        sizeList = m_staticInfo->highSpeedVideoFPSList;
        for (int i = 0; i < m_staticInfo->highSpeedVideoFPSListMax; i++) {
            if ((sizeList[i][0] == sizeList[i][1])
                && (sizeList[i][1]/1000 > maxFps)) {
                maxFps = sizeList[i][1]/1000;
            }
       }
   }

   return maxFps;
}

bool ExynosCameraParameters::checkFaceDetectMeta(struct camera2_shot_ext *shot_ext)
{
    Mutex::Autolock lock(m_faceDetectMetaLock);
    bool ret = false;

    if (shot_ext->shot.ctl.stats.faceDetectMode > FACEDETECT_MODE_OFF) {
        if (shot_ext->shot.dm.stats.faceDetectMode > FACEDETECT_MODE_OFF
            && m_metadata.shot.dm.request.frameCount < shot_ext->shot.dm.request.frameCount) {
            CLOGV("update faceData to parameter. faceMode(%d), metaFrameCount(%d), dmFrameDount(%d)",
                    shot_ext->shot.dm.stats.faceDetectMode,
                    m_metadata.shot.dm.request.frameCount,
                    shot_ext->shot.dm.request.frameCount);

            m_metadata.shot.dm.request.frameCount = shot_ext->shot.dm.request.frameCount;
            m_metadata.shot.dm.stats.faceDetectMode = shot_ext->shot.dm.stats.faceDetectMode;

            for (int i = 0; i < CAMERA2_MAX_FACES; i++) {
                m_metadata.shot.dm.stats.faceIds[i] = shot_ext->shot.dm.stats.faceIds[i];
                m_metadata.shot.dm.stats.faceScores[i] = shot_ext->shot.dm.stats.faceScores[i];
                if (i < CAMERA2_MAX_FACES - 2) {
                    m_metadata.shot.dm.stats.faces[i] = shot_ext->shot.dm.stats.faces[i];
                }
                for (int j = 0; j < 6; j++) {
                    m_metadata.shot.dm.stats.faceLandmarks[i][j] = shot_ext->shot.dm.stats.faceLandmarks[i][j];
                }
                for (int j = 0; j < 4; j++) {
                    m_metadata.shot.dm.stats.faceRectangles[i][j] = shot_ext->shot.dm.stats.faceRectangles[i][j];
                }

                memcpy(&(m_metadata.vra_ext), &(shot_ext->vra_ext), sizeof(struct vra_ext_meta));
            }
        } else if (shot_ext->shot.dm.stats.faceDetectMode == FACEDETECT_MODE_OFF) {
            CLOGV("update faceData from parameter. faceMode(%d)", shot_ext->shot.dm.stats.faceDetectMode);

            shot_ext->shot.dm.stats.faceDetectMode = m_metadata.shot.dm.stats.faceDetectMode;

            for (int i = 0; i < CAMERA2_MAX_FACES; i++) {
                shot_ext->shot.dm.stats.faceIds[i] = m_metadata.shot.dm.stats.faceIds[i];
                shot_ext->shot.dm.stats.faceScores[i] = m_metadata.shot.dm.stats.faceScores[i];
                if (i < CAMERA2_MAX_FACES - 2) {
                    shot_ext->shot.dm.stats.faces[i] = m_metadata.shot.dm.stats.faces[i];
                }
                for (int j = 0; j < 6; j++) {
                    shot_ext->shot.dm.stats.faceLandmarks[i][j] = m_metadata.shot.dm.stats.faceLandmarks[i][j];
                }
                for (int j = 0; j < 4; j++) {
                    shot_ext->shot.dm.stats.faceRectangles[i][j] = m_metadata.shot.dm.stats.faceRectangles[i][j];
                }

                memcpy(&(shot_ext->vra_ext), &(m_metadata.vra_ext), sizeof(struct vra_ext_meta));
            }

            ret = true;
        }
    }

    return ret;
}

void ExynosCameraParameters::getFaceDetectMeta(struct camera2_shot_ext *shot_ext)
{
    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return;
    }

    switch (shot_ext->shot.ctl.stats.faceDetectMode) {
    case FACEDETECT_MODE_FULL:
        memcpy(shot_ext->shot.dm.stats.faceLandmarks, m_metadata.shot.dm.stats.faceLandmarks, sizeof(m_metadata.shot.dm.stats.faceLandmarks));
    case FACEDETECT_MODE_SIMPLE:
        shot_ext->shot.dm.stats.faceDetectMode = shot_ext->shot.ctl.stats.faceDetectMode;
        memcpy(shot_ext->shot.dm.stats.faceIds, m_metadata.shot.dm.stats.faceIds, sizeof(m_metadata.shot.dm.stats.faceIds));
        memcpy(shot_ext->shot.dm.stats.faceRectangles, m_metadata.shot.dm.stats.faceRectangles, sizeof(m_metadata.shot.dm.stats.faceRectangles));
        memcpy(shot_ext->shot.dm.stats.faceScores, m_metadata.shot.dm.stats.faceScores, sizeof(m_metadata.shot.dm.stats.faceScores));
        memcpy(&(shot_ext->vra_ext), &(m_metadata.vra_ext), sizeof(struct vra_ext_meta));
        break;
    case FACEDETECT_MODE_OFF:
        /* No operation */
        break;
    default:
        CLOGE("Not supported FD mode %d",
                shot_ext->shot.ctl.stats.faceDetectMode);
        break;
    }

    return;
}

void ExynosCameraParameters::updateMetaDataParam(__unused struct camera2_shot_ext *shot)
{
#ifdef LLS_CAPTURE
    m_setLLSValue(shot);
#endif

#ifdef USES_HIFI_LLS
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_HIFILLS))
        m_setHifiLLSMeta(shot);
#endif

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_REMOSAIC)) {
        int curGain = m_configurations->getModeValue(CONFIGURATION_TOTAL_GAIN_VALUE);
        int nextGain = (shot->shot.udm.sensor.analogGain/1000) * (shot->shot.udm.sensor.digitalGain/1000);
        if (curGain != nextGain) {
            m_configurations->setModeValue(CONFIGURATION_TOTAL_GAIN_VALUE, nextGain);
        }
    }
}

#ifdef USES_HIFI_LLS
void ExynosCameraParameters::m_setHifiLLSMeta(struct camera2_shot_ext *shot)
{
    Mutex::Autolock lock(m_hifillsMetaLock);

	int nextBV = shot->shot.udm.internal.vendorSpecific[2];
    if (m_brightnessValue != nextBV) {
        if (m_hifillsOn) {
            if (m_brightnessValue >= BOUNDARY_OUT_BV) {
                m_hifillsOn = false;
                CLOGD("update BV(%d -> %d) hifillsOn (%d -> %d)", m_brightnessValue, nextBV, true, m_hifillsOn);
            }
        } else {
            if (m_brightnessValue <= BOUNDDRY_IN_BV) {
                m_hifillsOn = true;
                CLOGD("update BV(%d -> %d) hifillsOn (%d -> %d)", m_brightnessValue, nextBV, false, m_hifillsOn);
            }
        }
        m_brightnessValue = nextBV;
    }
}

int ExynosCameraParameters::getHifiLLSMeta(int &brightness, bool &hifillsOn)
{
    Mutex::Autolock lock(m_hifillsMetaLock);
    status_t ret = NO_ERROR;
	brightness = m_brightnessValue;
	hifillsOn = m_hifillsOn;
    return ret;
}
#endif

int ExynosCameraParameters::getNumOfCaptureFrame(void)
{
    int captureCount = 1;

#ifdef USES_COMBINE_PLUGIN
    if (m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE) ||
        m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE) ||
        m_configurations->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) ||
        m_configurations->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE) ||
        m_configurations->getMode(CONFIGURATION_SPORTS_YUV_MODE)) {
        ////////////////////////////////////////////////
        // iso
        int iso = (int)m_metadata.shot.dm.aa.vendor_isoValue;

        ////////////////////////////////////////////////
        // digitalGain
        unsigned int digitalGain = m_metadata.shot.udm.sensor.digitalGain;

        ////////////////////////////////////////////////
        // flashMode
        int flashMode = m_configurations->getModeValue(CONFIGURATION_FLASH_MODE);
        int isFlash = 1;

        switch (flashMode) {
        case FLASH_MODE_OFF:
            isFlash = 0;
            break;
        case FLASH_MODE_AUTO:
        case FLASH_MODE_ON:
        case FLASH_MODE_RED_EYE:
        case FLASH_MODE_TORCH:
        default:
            isFlash = 1;
            break;
        }

        ////////////////////////////////////////////////
        // camera type
        int rearOrFront = getCameraId() % 2; /* facing back 0 front 1 */

        ////////////////////////////////////////////////
        // llsState
        int llsState = NONE;

        ////////////////////////////////////////////////
        // decide
        if (m_configurations->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == true) {
            const int slaveSuperNightShotMergeNum = 6;
            captureCount = (m_camType == MAIN_CAM)
                            ? getSuperNightShotBayerMergeNum()
                            : slaveSuperNightShotMergeNum;
        } else if (m_configurations->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE) == true) {
            captureCount = getFlashMultiFrameDenoiseYuvMergeNum();
        } else if (m_configurations->getMode(CONFIGURATION_SPORTS_YUV_MODE) == true) {
            captureCount = getSportsYuvMergeNum();
        } else {
            captureCount = getLowlightMergeNum();
        }

        ////////////////////////////////////////////////
        // log
        CLOGD("[NightShot] digitalGain(%d) iso(%d) flashMode(%d) facing(%d) llsState(%d)",
            digitalGain,
            iso,
            flashMode,
            rearOrFront,
            llsState);
    } else if (m_configurations->getMode(CONFIGURATION_HDR_BAYER_MODE)) {
        captureCount = getHdrBayerMergeNum();
    } else if (m_configurations->getMode(CONFIGURATION_HDR_YUV_MODE)) {
        captureCount = getHdrYuvMergeNum();
    } else if (m_configurations->getMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE)) {
        captureCount = getBeautyFaceMergeNum();
    } else if (m_configurations->getMode(CONFIGURATION_SUPER_RESOLUTION_MODE)) {
        captureCount = getSuperResolutionMergeNum();
    } else if (m_configurations->getMode(CONFIGURATION_OIS_DENOISE_YUV_MODE)) {
        captureCount = getOisDenoiseYuvMergeNum();
    } else
#endif
    {
        captureCount = 1;
    }

    ////////////////////////////////////////////////

    return captureCount;
}

void ExynosCameraParameters::checkPostProcessingCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;
    /* 0. reset preCondition. */
    m_resetPreCaptureCondition(request);

    /* check preCaptureCondition */
    m_checkPreCaptureCondition(request);

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_CLAHE_CAPTURE)) {
        ret = m_checkClaheCapture(request);
    }

    /* check combine single capture mode */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_COMBINE_SINGLE_CAPTURE)) {
        ret = m_checkCombineSingleCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    /* 2. check hifiCapture mode : for yuv reprocessing */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_HIFI)) {
        ret = m_checkHIFICapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    /* 2. check remosaic mode : for remosaic capture */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_REMOSAIC)) {
        ret = m_checkRemosaicCapture(request);
#ifndef USES_COMBINE_PLUGIN
        if (ret == NO_ERROR) {
            goto func_exit;
        }
#endif
    }

    /* 3. check hifills capture mode : for lls capture */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_HIFILLS)) {
        ret = m_checkHIFILLSCapture(request);
#ifndef USES_COMBINE_PLUGIN
        if (ret == NO_ERROR) {
            goto func_exit;
        }
#endif
    }

    /* 4. check LongExposure capture mode */
    /* LEC do not use the multiframe concept, so we don't use the this code.*/
/*
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_LONGEXPOSURECAPTURE)) {
        ret = m_checkLongExposureCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }
*/
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_BEAUTY_FACE_YUV)) {
        ret = m_checkBeautyFaceYuvCapture(request);
        // beauty face can be on, with other mode.
        /*
        if (ret == NO_ERROR) {
            goto func_exit;
        }
        */
    }

    /* check night shot capture mode */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_NIGHT_SHOT_BAYER)) {
        ret = m_checkNightShotBayerCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_NIGHT_SHOT_YUV)) {
        ret = m_checkNightShotYuvCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    /* check super night shot capture mode */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_SUPER_NIGHT_SHOT_BAYER)) {
        ret = m_checkSuperNightShotBayerCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    /* check hdr capture mode */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_HDR_BAYER)) {
        ret = m_checkHdrBayerCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_HDR_YUV)) {
        ret = m_checkHdrYuvCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_FLASH_MULTI_FRAME_DENOISE_YUV)) {
        ret = m_checkFlashMultiFrameDenoiseYuvCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_SUPER_RESOLUTION)) {
        ret = m_checkSuperResolutionCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_OIS_DENOISE_YUV)) {
        ret = m_checkOisDenoiseYuvCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_SPORTS_YUV)) {
        ret = m_checkSportsYuvCapture(request);
        if (ret == NO_ERROR) {
            goto func_exit;
        }
    }

func_exit:

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
            || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
            ret = m_checkBokehCapture(request);
            if (ret != NO_ERROR) {
                CLOGE("check bokeh capture is fail.(%d)", ret);
                return;
            }
        }
    }
#endif

    return;
}

status_t ExynosCameraParameters::m_resetPreCaptureCondition(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;

    m_configurations->setMode(CONFIGURATION_HIFI_LLS_MODE, false);
    m_configurations->setMode(CONFIGURATION_HIFI_MODE, false);
    m_configurations->setMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE, false);
    m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, false);
    m_configurations->setMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE, false);
    m_configurations->setMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE, false);
    m_configurations->setMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_HDR_BAYER_MODE, false);
    m_configurations->setMode(CONFIGURATION_HDR_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_SUPER_RESOLUTION_MODE, false);
    m_configurations->setMode(CONFIGURATION_OIS_DENOISE_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_SPORTS_YUV_MODE, false);
    m_configurations->setMode(CONFIGURATION_CLAHE_CAPTURE_MODE, false);

#ifdef SUPPORT_VENDOR_YUV_STALL
    m_configurations->setMode(CONFIGURATION_VENDOR_YUV_STALL, false);
#endif
#ifdef SUPPORT_VENDOR_YUV_STALL
    m_configurations->setModeValue(CONFIGURATION_VENDOR_YUV_STALL_PORT, 0);
#endif
    m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, 1);
    m_configurations->setModeValue(CONFIGURATION_ZSL_YUV_INPUT, 0);
    m_configurations->setModeValue(CONFIGURATION_ZSL_PRIV_INPUT, 0);

    m_configurations->setMode(CONFIGURATION_LONGEXPOSURE_CAPTURE_MODE, false);
    m_configurations->setMode(CONFIGURATION_BEST_SHOT_MODE, false);
    setBayerFrameLockCount(0, 0);

    return ret;
}


status_t ExynosCameraParameters::m_checkPreCaptureCondition(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;

    camera3_stream_buffer_t* buffer = request->getInputBuffer();
    if (buffer != NULL) {
        int streamId = 0;
        ExynosCameraStream *stream = static_cast<ExynosCameraStream *>(buffer->stream->priv);
        if(stream != NULL) {
            stream->getID(&streamId);

            CLOGD("requestKey %d buffer-StreamType(HAL_STREAM_ID_ZSL_INPUT[%d]) Buffer[%p], Stream[%p]",
                     request->getKey(), streamId, buffer, stream);

            switch (streamId % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_YUV_INPUT:
                m_configurations->setModeValue(CONFIGURATION_ZSL_YUV_INPUT, 1);
                break;
            case HAL_STREAM_ID_ZSL_INPUT:
                m_configurations->setModeValue(CONFIGURATION_ZSL_PRIV_INPUT, 1);
                break;
            default:
                break;
            }

        } else {
            CLOGE(" Stream is null (%d)", request->getKey());
        }
    }

    return ret;
}

status_t ExynosCameraParameters::m_checkCombineSingleCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;

#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    camera_metadata_entry_t entry;
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_OFF:
            default:
                break;
        }
    }
#else
    mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_COMBINE_SINGLE_CAPTURE);
#endif

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE, &value, 1);
#endif

    m_configurations->setMode(CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);
        m_configurations->setMode(CONFIGURATION_VENDOR_YUV_STALL, true);
        setBayerFrameLockCount(0, captureCount);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkHIFICapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;
#ifdef USES_HIFI
    bool supportSceanrio = true;
    bool mode = false;

    if (m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == false))
    {
        supportSceanrio = false;
    }

    if (supportSceanrio) {
        if (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT)) {
            mode = true;
            ret = NO_ERROR;
        }
    }

    m_configurations->setMode(CONFIGURATION_HIFI_MODE, mode);
    CLOGD("mode(%d)", mode);

#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkRemosaicCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;
    bool enable = false;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) > 0)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) > 0)
        )
    {
        supportScenario = false;
    }

#ifdef SUPPORT_REMOSAIC_DNG
    if (request->hasStream(HAL_STREAM_ID_RAW)) {
        uint8_t cmd = EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW;
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        vendorMeta->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION, &cmd, 1);
    }
#endif

#ifdef USE_REMOSAIC_CONTROL_REQUEST
    if (supportScenario) {
        camera_metadata_entry_t entry;
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_NONE:
                break;
            case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW:
            case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW:
                enable = true;
                ret = NO_ERROR;
                break;
            default:
                break;
            }
        }
    }
#else
    if (supportScenario) {
        /* TODO : Add condition */
        if (m_configurations->getModeValue(CONFIGURATION_TOTAL_GAIN_VALUE) == 1) {
            enable = true;
            ret = NO_ERROR;
        }
    }
#endif

    m_configurations->setMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE, enable);

    CLOGD("Remosaic Mode(%d)", enable);

    return ret;
}

status_t ExynosCameraParameters::m_checkHIFILLSCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;
#ifdef USES_HIFI_LLS
    bool mode = false;
    int32_t bv = 0;
    int usage = 0;
    bool hifillsOn = false;
    float zoomRatio = m_configurations->getZoomRatio();
    int captureCount = 1;
    bool supportSceanrio = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportSceanrio = false;
    }

#ifdef USE_HIFILLS_CONTROL_REQUEST
    if (supportSceanrio) {
        bool vendorYuvStall = false;
        camera_metadata_entry_t entry;
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        entry = vendorMeta->find(SLSI_MF_STILL_CAPTURE);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case SLSI_MF_STILL_FUNCTIONS_NONE:
                break;
            case SLSI_MF_STILL_FUNCTIONS_LLS:
                mode = true;
                /* captureCount = checkVendorMFStillCount(request); */
                captureCount = 3;
                vendorYuvStall = getVendorYUVStallMeta(request);
                m_configurations->setMode(CONFIGURATION_VENDOR_YUV_STALL, vendorYuvStall);
                break;
            case SLSI_MF_STILL_FUNCTIONS_SR:
                mode = true;
                captureCount = 5;
                /* captureCount = checkVendorMFStillCount(request); */
                vendorYuvStall = getVendorYUVStallMeta(request);
                m_configurations->setMode(CONFIGURATION_VENDOR_YUV_STALL, vendorYuvStall);
                break;
            default:
                break;
            }
        }
    }
#else
    if (supportSceanrio) {
        getHifiLLSMeta(bv, hifillsOn);

        if (isFrontCamera(m_cameraId)) {
            if (zoomRatio <= BOUNDDRY_IN_ZOOM_FRONT && hifillsOn) {
                mode = true;
                captureCount = 3;
            }
        } else if (isBackCamera(m_cameraId)) {
            if (hifillsOn) {
                mode = true;
                captureCount = 3;
            } else if(zoomRatio >= BOUNDDRY_SR_IN_ZOOM && bv >= BOUNDARY_SR_IN_BV) {
                mode = true;
                captureCount = 5;
            }
        }
    }
#endif
    m_configurations->setMode(CONFIGURATION_HIFI_LLS_MODE, mode);
    m_configurations->setModeValue(CONFIGURATION_BRIGHTNESS_VALUE, bv);
    if (mode) {
        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) brightness(%d) captureCount(%d)", mode, bv, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkLongExposureCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;
    bool mode = false;
    int captureCount = 1;
    bool supportSceanrio = true;
    uint64_t exposure = 0L;

#ifdef USE_LONGEXPOSURECAPTURE
    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportSceanrio = false;
    }

#ifdef USE_LONGEXPOSURECAPTURE_CONTROL_REQUEST
    if (supportSceanrio) {
        bool vendorYuvStall = false;
        camera_metadata_entry_t entryMode;
        camera_metadata_entry_t entry;
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        entryMode = vendorMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE);
        if (entryMode.count > 0) {
            switch (entryMode.data.u8[0]) {
            case EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_OFF:
                break;
            case EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_ON:
                entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI);
                if (entry.count > 0) {
                    exposure = entry.data.i64[0];
                }

                entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_COUNT);
                if (entry.count > 0) {
                    captureCount = entry.data.i32[0];
                }

                mode = true;
                break;
            default:
                break;
            }
        }
    }
#endif
#endif

    m_configurations->setMode(CONFIGURATION_LONGEXPOSURE_CAPTURE_MODE, mode);
    m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
    setBayerFrameLockCount(0, captureCount);


    CLOGD("mode(%d) exposure(%ju) captureCount(%d)", mode, exposure, captureCount);

    return ret;
}

status_t ExynosCameraParameters::m_checkNightShotBayerCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_NIGHT_SHOT_BAYER);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER, &value, 1);
#endif

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        setBayerFrameLockCount(0, captureCount);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkNightShotYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_NIGHT_SHOT_YUV);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT, &value, 1);
#endif

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        ////////////////////////////////////////////////
        // turn on OIS capture mode. (most MF still works like OIS)
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);

        ////////////////////////////////////////////////

        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkSuperNightShotBayerCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_SUPER_NIGHT_SHOT_BAYER);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER, &value, 1);
#endif

    m_configurations->setMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();
        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        setBayerFrameLockCount(0, captureCount);

        /* turn on OIS capture mode. (most MF still works like OIS) */
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkHdrBayerCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_HDR_BAYER);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_HDR_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_HDR_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_HDR_BAYER);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_HDR_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_HDR_BAYER, &value, 1);
#endif

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_HDR_BAYER_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        if (m_configurations->getMode(CONFIGURATION_DYNAMIC_BAYER_MODE) == true)
            setBayerFrameLockCount(0, 0);
        else
            setBayerFrameLockCount(0, captureCount);

        ////////////////////////////////////////////////
        // turn on OIS capture mode. (most MF still works like OIS)
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);

        ////////////////////////////////////////////////

        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkHdrYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_HDR_YUV);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_HDR_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_HDR_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_HDR_YUV);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_HDR_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_HDR_YUV, &value, 1);
#endif

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_HDR_YUV_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);

        if (m_configurations->getMode(CONFIGURATION_DYNAMIC_BAYER_MODE) == true)
            setBayerFrameLockCount(0, 0);
        else
            setBayerFrameLockCount(0, captureCount);

        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkFlashMultiFrameDenoiseYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_MODE_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_MODE_NONE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_FLASH_MULTI_FRAME_DENOISE_YUV);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_ANDROID_VENDOR_MODE_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV, &value, 1);
#endif

    /////////////////////////////////
    // turn off forcely, in case of flash-off / flash-auto(when it does not turn on flash).
    if (mode == true) {
        camera2_shot_ext *shot_ext = request->getServiceShot();

        if (shot_ext) {
            switch (shot_ext->shot.uctl.flashMode) {
            case CAMERA_FLASH_MODE_AUTO:
                /////////////////////////////////
                // when it does not turn on flash in auto mode, turn off forcely.
                if (shot_ext->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE) {
                    ExynosCameraActivityControl *activityControl = NULL;
                    activityControl = this->getActivityControl();

                    ExynosCameraActivityFlash *flashMgr = NULL;
                    flashMgr = activityControl->getFlashMgr();

                    ExynosCameraActivityFlash::FLASH_STEP flashStep;
                    flashMgr->getFlashStep(&flashStep);

                    if (flashStep != ExynosCameraActivityFlash::FLASH_STEP_MAIN_START) {
                        mode = false;
                        CLOGD("turn off forcely MULTI_FRAME_DENOISE_YUV_MODE, by flashMode(%d) flashStep(%d)",
                            shot_ext->shot.uctl.flashMode, flashStep);
                    }
                }
                break;
            case CAMERA_FLASH_MODE_ON:
            case CAMERA_FLASH_MODE_RED_EYE:
            case CAMERA_FLASH_MODE_TORCH:
                break;
            case CAMERA_FLASH_MODE_OFF:
            default:
                mode = false;
                CLOGD("turn off forcely MULTI_FRAME_DENOISE_YUV_MODE, by flashMode(%d)",
                    shot_ext->shot.uctl.flashMode);
                break;
            }
        }
    }

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        setBayerFrameLockCount(3, captureCount - 3);

        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        ////////////////////////////////////////////////
        // turn on OIS capture mode. (most MF still works like OIS)
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);
        ////////////////////////////////////////////////

        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkBeautyFaceYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USES_COMBINE_PLUGIN
    bool mode = false;
    int captureCount = 1;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
#ifdef USE_DUAL_CAMERA
        || m_configurations->getMode(CONFIGURATION_DUAL_MODE)
        || m_configurations->getMode(CONFIGURATION_DUAL_PRE_MODE)
#endif
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        supportScenario = false;
    }

    if (supportScenario) {
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;
        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN);
        if (entry.count > 0) {
            switch (entry.data.u8[0]) {
            case EXYNOS_COMBINE_PREVIEW_PLUGIN_ENABLE:
                mode = true;
                break;
            case EXYNOS_COMBINE_PREVIEW_PLUGIN_DISABLE:
            default:
                break;
            }
        }
#else
        mode = m_configurations->getEnabledByProperty(SUPPORTED_FUNCTION_BEAUTY_FACE_YUV);
#endif
    }

#if 0
    /////////////////////////////////
    // hack : force to turn on.
    mode = true;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    const uint8_t value = EXYNOS_COMBINE_PREVIEW_PLUGIN_ENABLE;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN, &value, 1);
#endif

    /////////////////////////////////

    m_configurations->setMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        ////////////////////////////////////////////////

        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkOisDenoiseYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

    bool mode = false;
    int captureCount = 1;

#ifdef USES_COMBINE_PLUGIN
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    camera_metadata_entry_t entry;
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_OFF:
            default:
                break;
        }
    }

#if 0
    // hack : force to turn on.
    mode = true;

    const uint8_t value = EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV, &value, 1);
#endif

    m_configurations->setMode(CONFIGURATION_OIS_DENOISE_YUV_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        // turn on OIS capture mode. (most MF still works like OIS)
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkSportsYuvCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

    bool mode = false;
    int captureCount = 1;
    int motionLevel = 0;

#ifdef USES_COMBINE_PLUGIN
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    camera_metadata_entry_t entry;
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_SPORTS_YUV);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_MODE_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_MODE_NONE:
            default:
                break;
        }
    }

#if 0
    // hack : force to turn on.
    mode = true;

    const uint8_t value = EXYNOS_ANDROID_VENDOR_MODE_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_SPORTS_YUV, &value, 1);

    const uint8_t value2 = 0; // 0, 1, 2
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL, &value2, 1);
#endif

    m_configurations->setMode(CONFIGURATION_SPORTS_YUV_MODE, mode);

    if (mode) {
        ////////////////////////////////////////////////
        // get motion Level.
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        camera_metadata_entry_t entry;

        entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL);
        if (entry.count > 0) {

            motionLevel = (int)entry.data.u8[0];

            if (motionLevel < 0) {
                CLOGW("force motionLevel from %d to 0", motionLevel);
                motionLevel = 0;
            } else if (2 < motionLevel) {
                CLOGW("force motionLevel from %d to 2", motionLevel);
                motionLevel = 2;
            }
        } else {
            CLOGW("cannot find EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL. just use 0");
                motionLevel = 0;
        }

        m_configurations->setModeValue(CONFIGURATION_SPORTS_YUV_MOTION_LEVEL, motionLevel);

        ////////////////////////////////////////////////
        // set CaptureCount & YUV Stall
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        // turn on OIS capture mode. (most MF still works like OIS)
        m_configurations->setMode(CONFIGURATION_OIS_CAPTURE_MODE, true);
        ret = NO_ERROR;
    }

    CLOGD("mode(%d) captureCount(%d) motionLevel(%d)", mode, captureCount, motionLevel);
#endif
    return ret;
}

status_t ExynosCameraParameters::m_checkSuperResolutionCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

#ifdef USE_SLSI_VENDOR_TAGS
    bool mode = false;
    int captureCount = 1;

    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    camera_metadata_entry_t entry;
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF:
            default:
                break;
        }
    }

#if 0
    // hack : force to turn on.
    mode = true;

    const uint8_t value = EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION, &value, 1);
#endif

    m_configurations->setMode(CONFIGURATION_SUPER_RESOLUTION_MODE, mode);

    if (mode) {
        captureCount = this->getNumOfCaptureFrame();

        m_configurations->setModeValue(CONFIGURATION_CAPTURE_COUNT, captureCount);
        m_configurations->setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_PICTURE);

        ret = NO_ERROR;
    }
    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
#endif
    return ret;
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCameraParameters::m_checkBokehCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;

#ifdef USES_COMBINE_PLUGIN
    int bayerKeepCount = 0;
    bool supportScenario = true;

    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == false
        || (m_scenario != SCENARIO_DUAL_REAR_PORTRAIT && m_scenario != SCENARIO_DUAL_FRONT_PORTRAIT)
        || m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_MODE)
        || m_configurations->getMode(CONFIGURATION_PIP_RECORDING_MODE)
        || m_configurations->getMode(CONFIGURATION_VISION_MODE)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_YUV_INPUT) == true)
        || (m_configurations->getModeValue(CONFIGURATION_ZSL_PRIV_INPUT) == true))
    {
        return NO_ERROR;
    }

    if (supportScenario == true) {
        /* need vendor configuration */
    }
#endif
    return ret;
}
#endif

status_t ExynosCameraParameters::m_checkClaheCapture(__unused ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = INVALID_OPERATION;

    bool mode = false;
    int captureCount = 1;
#ifdef USE_SLSI_VENDOR_TAGS
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
    camera_metadata_entry_t entry;
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
            case EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_ON:
                mode = true;
                break;
            case EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_OFF:
            default:
                break;
        }
    }
#endif
#if 0
    // hack : force to turn on.
    mode = true;

    const uint8_t value = EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_ON;
    vendorMeta->update(EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE, &value, 1);
#endif

    m_configurations->setMode(CONFIGURATION_CLAHE_CAPTURE_MODE, mode);

    CLOGD("mode(%d) captureCount(%d)", mode, captureCount);
    return ret;
}

bool ExynosCameraParameters::getHfdMode(void)
{
    bool useHfd = false;

    /*
       This is Static value,
       If you want off to HFD operation,
       turn off HFD pipe request
     */
#ifdef SUPPORT_HFD
    if (m_cameraId == CAMERA_ID_FRONT
        && m_configurations->getMode(CONFIGURATION_PIP_MODE) == false
        ) {
        useHfd = true;
    }
#endif

    return useHfd;
}

#ifdef USES_CAMERA_EXYNOS_VPL
bool ExynosCameraParameters::getNfdMode(void)
{
    bool useNfd = false;
    uint32_t minFps = 0, maxFps = 0;
    m_configurations->getPreviewFpsRange(&minFps, &maxFps);
    CLOGV("facedectect  %d", m_metadata.shot.ctl.stats.faceDetectMode);
    if (m_metadata.shot.dm.stats.faceDetectMode > FACEDETECT_MODE_OFF && maxFps <= 30) {
        useNfd = true;
    }
    return useNfd;
}
#endif

int ExynosCameraParameters::m_adjustDsInputPortId(const int dsInputPortId)
{
    int newDsInputPortId = MCSC_PORT_NONE;

    switch (dsInputPortId) {
    case MCSC_PORT_0:
    case MCSC_PORT_1:
    case MCSC_PORT_2:
    case MCSC_PORT_3:
    case MCSC_PORT_4:
        newDsInputPortId = dsInputPortId;
        break;
    default:
        newDsInputPortId = getDsInputPortId(false);
        if (newDsInputPortId <= MCSC_PORT_NONE
            || newDsInputPortId >= MCSC_PORT_MAX) {
            newDsInputPortId = getDsInputPortId(true);
        }

        CLOGV("NOT supported DS input port ID %d <= %d",
                dsInputPortId, newDsInputPortId);
        break;
    }

    return newDsInputPortId;
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCameraParameters::setStandbyState(dual_standby_state_t state)
{
    Mutex::Autolock lock(m_standbyStateLock);

    m_standbyState = state;

    return NO_ERROR;
}

dual_standby_state_t ExynosCameraParameters::getStandbyState(void)
{
    Mutex::Autolock lock(m_standbyStateLock);

    return m_standbyState;
}
#endif

bool ExynosCameraParameters::getGmvMode(void)
{
    return m_configurations->getMode(CONFIGURATION_GMV_MODE);
}

bool ExynosCameraParameters::supportJpegYuvRotation(void)
{
    bool ret = false;

#ifdef SUPPORT_ROTATION_STILL
    ret = true;
#endif

    return ret;
}

int ExynosCameraParameters::getSupportedSensorExMode(void)
{
    return m_staticInfo->supported_sensor_ex_mode;
}

int ExynosCameraParameters::getSupportedRemosaicModes(int &numberOfMode)
{
    numberOfMode = m_staticInfo->supported_remosaic_modeMax;
    return m_staticInfo->supported_remosaic_mode;
}

#ifdef SUPPORT_SENSOR_MODE_CHANGE
void ExynosCameraParameters::sensorModeTransition(bool toggle)
{
    if (m_sensorModeTransition != toggle) {
        CLOGD("sensor mode is changing (%d) ---> (%d)", m_sensorModeTransition, toggle);
    }

    m_sensorModeTransition = toggle;
}

bool ExynosCameraParameters::isSensorModeTransition()
{
    CLOGV("m_transitSensorMode(%d)", m_sensorModeTransition);
    return m_sensorModeTransition;
}

void ExynosCameraParameters::setSensorModeTransitionFrameCount(int fcount)
{
    m_sensorModeTransitionFrameCount = fcount;
}

int ExynosCameraParameters::getSensorModeTransitionFrameCount(void)
{
    return m_sensorModeTransitionFrameCount;
}

#endif

void ExynosCameraParameters::setBayerFrameLock(bool lock)
{
    m_bayerFrameLock = lock;
}

bool ExynosCameraParameters::getBayerFrameLock(void)
{
    return m_bayerFrameLock;
}

void ExynosCameraParameters::setBayerFrameLockCount(int oldCount, int newCount)
{
    m_setOldBayerFrameLockCount(oldCount);
    m_setNewBayerFrameLockCount(newCount);
    setBayerFrameLock((oldCount > 0 || newCount > 0) ? true : false);
}

void ExynosCameraParameters::m_setOldBayerFrameLockCount(int count)
{
    if (count > m_configurations->getModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE)) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):old bayer lock count(%d) cannot be greater than max old bayer keep count(%d)",
                            __FUNCTION__, __LINE__,
                            count, m_configurations->getModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE));
    }

    if (m_oldBayerFrameLockCount != count) {
        CLOGD("oldBayerFrameLockCount = %d", count);
        m_oldBayerFrameLockCount = count;
    }
}

int ExynosCameraParameters::getOldBayerFrameLockCount(void)
{
    return m_oldBayerFrameLockCount;
}

void ExynosCameraParameters::m_setNewBayerFrameLockCount(int count)
{
    if (count > m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT)) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):new bayer lock count(%d) cannot be greater than max capture count(%d)",
                            __FUNCTION__, __LINE__,
                            count, m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT));
    }

    if (m_newBayerFrameLockCount != count) {
        CLOGD("newBayerFrameLockCount = %d", count);
        m_newBayerFrameLockCount = count;
    }
}

int ExynosCameraParameters::m_getSensorInfoCamIdx(void)
{
    int camIdx = MAIN_CAM;

    switch (m_scenario) {
    case SCENARIO_DUAL_REAR_ZOOM:
        // try to get base sensorInfoCamIdx from camInfo
        // eg. W + T + UT -> T might be the base sensorInfo
        if (m_camIdInfo->sensorInfoCamIdx) {
            camIdx = m_camIdInfo->sensorInfoCamIdx;
        }
        break;
    default:
        break;
    }

    return camIdx;
}

int ExynosCameraParameters::getNewBayerFrameLockCount(void)
{
    return m_newBayerFrameLockCount;
}

}; /* namespace android */
