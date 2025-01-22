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
#define LOG_TAG "ExynosCameraParametersSec"
#include <log/log.h>

#include "ExynosCameraUtils.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraMetadataConverter.h"

namespace android {

void ExynosCameraParameters::vendorSpecificConstructor(int cameraId)
{
    ////////////////////////////////////////////////
    // reset num_of_appmarker as 0
    mDebugInfo.num_of_appmarker = 0;
    mDebugInfo2.num_of_appmarker = 0;

    memset(&mDebugInfo, 0x00, sizeof(debug_attribute_t));
    memset(&mDebugInfo2, 0x00, sizeof(debug_attribute_t));

#ifdef USES_P3_IN_EXIF
    ////////////////////////////////////////////////
    // p3
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_P3) == true) {
        debug_attribute_t *debugInfo = NULL;

        // debugInfo
        debugInfo = &mDebugInfo;

        debugInfo->idx[debugInfo->num_of_appmarker][0] = APP_MARKER_2;
        debugInfo->debugSize[APP_MARKER_2] = sizeof(icc_format_version);
        debugInfo->debugData[APP_MARKER_2] = new char[debugInfo->debugSize[APP_MARKER_2]];
        memcpy((void *)debugInfo->debugData[APP_MARKER_2], (void *)icc_format_version, debugInfo->debugSize[APP_MARKER_2]);

        debugInfo->num_of_appmarker++;

        // debugInfo2
        debugInfo = &mDebugInfo2;

        debugInfo->idx[debugInfo->num_of_appmarker][0] = APP_MARKER_2;
        debugInfo->debugSize[APP_MARKER_2] = sizeof(icc_format_version);
        debugInfo->debugData[APP_MARKER_2] = new char[debugInfo->debugSize[APP_MARKER_2]];
        memcpy((void *)debugInfo->debugData[APP_MARKER_2], (void *)icc_format_version, debugInfo->debugSize[APP_MARKER_2]);

        debugInfo->num_of_appmarker++;
    }
#endif

    ////////////////////////////////////////////////
    // APP4
    mDebugInfo.idx[mDebugInfo.num_of_appmarker][0] = APP_MARKER_4; /* matching the app marker 4 */
    mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm);
    mDebugInfo.debugSize[APP_MARKER_4] += sizeof(struct ddk_setfile_ver);
    mDebugInfo.debugData[APP_MARKER_4] = new char[mDebugInfo.debugSize[APP_MARKER_4]];
    memset((void *)mDebugInfo.debugData[APP_MARKER_4], 0, mDebugInfo.debugSize[APP_MARKER_4]);

    mDebugInfo.num_of_appmarker++;

    /* DebugInfo2 */
    mDebugInfo2.idx[mDebugInfo2.num_of_appmarker][0] = APP_MARKER_4; /* matching the app marker 4 */
    mDebugInfo2.debugSize[APP_MARKER_4] = mDebugInfo.debugSize[APP_MARKER_4];
    mDebugInfo2.debugData[APP_MARKER_4] = new char[mDebugInfo2.debugSize[APP_MARKER_4]];
    memset((void *)mDebugInfo2.debugData[APP_MARKER_4], 0, mDebugInfo2.debugSize[APP_MARKER_4]);

    mDebugInfo2.num_of_appmarker++;

    ////////////////////////////////////////////////
    // APP5
    // Check debug_attribute_t struct in ExynosExif.h
    getDebugInfoPlaneProperty();
    mDebugInfo.idx[mDebugInfo.num_of_appmarker][0] = APP_MARKER_5; /* matching the app marker 5 */
    if (getDebugInfoPlanePropertyValue()) {
        if (m_camType == MAIN_CAM)
            initThumbNailInfo();
        mDebugInfo.debugSize[APP_MARKER_5] = THUMBNAIL_HISTOGRAM_STAT_PRAT1;
    } else {
        mDebugInfo.debugSize[APP_MARKER_5] = 0;
    }

    if (mDebugInfo.debugSize[APP_MARKER_5] != 0) {
        mDebugInfo.debugData[APP_MARKER_5] = new char[mDebugInfo.debugSize[APP_MARKER_5]];
        memset((void *)mDebugInfo.debugData[APP_MARKER_5], 0, mDebugInfo.debugSize[APP_MARKER_5]);

        mDebugInfo.num_of_appmarker++;

        /* DebugInfo2 */
        mDebugInfo2.idx[mDebugInfo2.num_of_appmarker][0] = APP_MARKER_5; /* matching the app marker 5 */
        mDebugInfo2.debugSize[APP_MARKER_5] = mDebugInfo.debugSize[APP_MARKER_5];
        mDebugInfo2.debugData[APP_MARKER_5] = new char[mDebugInfo2.debugSize[APP_MARKER_5]];
        memset((void *)mDebugInfo2.debugData[APP_MARKER_5], 0, mDebugInfo2.debugSize[APP_MARKER_5]);

        mDebugInfo2.num_of_appmarker++;
    }

    ////////////////////////////////////////////////
    // APP6
    // Check debug_attribute_t struct in ExynosExif.h
    /*
    mDebugInfo.idx[mDebugInfo.num_of_appmarker][0] = APP_MARKER_6; // matching the app marker 6 /
    if (getDebugInfoPlanePropertyValue())
        mDebugInfo.debugSize[APP_MARKER_6] = THUMBNAIL_HISTOGRAM_STAT_PRAT2;
    else
        mDebugInfo.debugSize[APP_MARKER_6] = 0;

    if (mDebugInfo.debugSize[APP_MARKER_6] != 0) {
        mDebugInfo.debugSize[APP_MARKER_6] = THUMBNAIL_HISTOGRAM_STAT_PRAT2;
        mDebugInfo.debugData[APP_MARKER_6] = new char[mDebugInfo.debugSize[APP_MARKER_6]];
        memset((void *)mDebugInfo.debugData[APP_MARKER_6], 0, mDebugInfo.debugSize[APP_MARKER_6]);

        mDebugInfo.num_of_appmarker++;

        // DebugInfo2 /
        mDebugInfo2.idx[mDebugInfo2.num_of_appmarker][0] = APP_MARKER_6; // matching the app marker 6 /
        mDebugInfo2.debugSize[APP_MARKER_6] = mDebugInfo.debugSize[APP_MARKER_6];
        mDebugInfo2.debugData[APP_MARKER_6] = new char[mDebugInfo2.debugSize[APP_MARKER_6]];
        memset((void *)mDebugInfo2.debugData[APP_MARKER_6], 0, mDebugInfo2.debugSize[APP_MARKER_6]);

        mDebugInfo2.num_of_appmarker++;
    }
    */
    ////////////////////////////////////////////////

#if defined(SENSOR_FW_GET_FROM_FILE)
    m_eepromMap = NULL;
    m_flagEEPRomMapRead = false;
#endif
    m_makersNote = NULL;

    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START

    m_configurations->setUseFastenAeStable(true);

    m_activeZoomRatio = 0.0f;
    m_activeZoomMargin = 0;
    m_activeZoomRect = {0, };
    m_activePictureZoomRatio = 0.0f;
    m_activePictureZoomMargin = 0;
    m_activePictureZoomRect = {0, };

    m_binningProperty = 0;

    m_depthMapW = 0;
    m_depthMapH = 0;

    m_vendorConstructorInitalize(cameraId);

}

void ExynosCameraParameters::m_vendorSpecificDestructor(void)
{
    for (int i = 0; i < mDebugInfo.num_of_appmarker; i++) {
        if (mDebugInfo.debugData[mDebugInfo.idx[i][0]] != NULL)
            delete[] mDebugInfo.debugData[mDebugInfo.idx[i][0]];
        mDebugInfo.debugData[mDebugInfo.idx[i][0]] = NULL;
        mDebugInfo.debugSize[mDebugInfo.idx[i][0]] = 0;
    }

    for (int i = 0; i < mDebugInfo2.num_of_appmarker; i++) {
        if (mDebugInfo2.debugData[mDebugInfo2.idx[i][0]] != NULL)
            delete[] mDebugInfo2.debugData[mDebugInfo2.idx[i][0]];
        mDebugInfo2.debugData[mDebugInfo2.idx[i][0]] = NULL;
        mDebugInfo2.debugSize[mDebugInfo2.idx[i][0]] = 0;
    }

    ////////////////////////////////////////////////
    // destroy m_makersNote
    if (m_makersNote) {
        if (m_makersNote->flagCreated() == true) {
            m_makersNote->destroy();
        }

        SAFE_DELETE(m_makersNote);
    }

    ////////////////////////////////////////////////
    // destroy m_thumbHistInfo
    if(m_thumbHistInfo.bufAddr[0] != NULL)
        delete[] m_thumbHistInfo.bufAddr[0];
    m_thumbHistInfo.bufAddr[0] = NULL;
    m_thumbHistInfo.bufSize[0] = 0;

    if(m_thumbHistInfo.bufAddr[1] != NULL)
        delete[] m_thumbHistInfo.bufAddr[1];
    m_thumbHistInfo.bufAddr[1] = NULL;
    m_thumbHistInfo.bufSize[1] = 0;

#if defined(SENSOR_FW_GET_FROM_FILE)
    ////////////////////////////////////////////////
    // destroy m_eepromMap
    if (m_eepromMap) {
        if (m_eepromMap->flagCreated() == true) {
            m_eepromMap->destroy();
        }

        SAFE_DELETE(m_eepromMap);
    }
#endif
}

status_t ExynosCameraParameters::setSize(enum HW_INFO_SIZE_TYPE type, uint32_t width, uint32_t height, int outputPortId)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case HW_INFO_HW_YUV_SIZE:
    {
        int widthArrayNum = sizeof(m_hwYuvWidth)/sizeof(m_hwYuvWidth[0]);
        int heightArrayNum = sizeof(m_hwYuvHeight)/sizeof(m_hwYuvHeight[0]);

        if (widthArrayNum != YUV_OUTPUT_PORT_ID_MAX
            || heightArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
            android_printAssert(NULL, LOG_TAG, "ASSERT:Invalid yuvSize array length %dx%d."\
                    " YUV_OUTPUT_PORT_ID_MAX %d",
                    widthArrayNum, heightArrayNum,
                    YUV_OUTPUT_PORT_ID_MAX);
            return INVALID_OPERATION;
        }

        if (outputPortId < 0) {
            CLOGE("Invalid outputPortId(%d)", outputPortId);
            return INVALID_OPERATION;
        }

        m_hwYuvWidth[outputPortId] = width;
        m_hwYuvHeight[outputPortId] = height;
        break;
    }
    case HW_INFO_HW_BNS_SIZE:
        m_width[type] = width;
        m_height[type] = height;
        updateHwSensorSize();
        break;
    case HW_INFO_MAX_HW_YUV_SIZE:
    case HW_INFO_HW_MAX_PICTURE_SIZE:
    case HW_INFO_HW_SENSOR_SIZE:
    case HW_INFO_HW_PICTURE_SIZE:
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    case HW_INFO_SENSOR_MODE_CHANGE_BDS_SIZE:
#endif
        m_width[type] = width;
        m_height[type] = height;
        break;
    case HW_INFO_HW_YUV_INPUT_SIZE:
    {
        m_hwYuvInputWidth = width;
        m_hwYuvInputHeight = height;
        break;
    }
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraParameters::getSize(enum HW_INFO_SIZE_TYPE type, uint32_t *width, uint32_t *height, int outputPortId)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case HW_INFO_SENSOR_MARGIN_SIZE:
        *width = m_staticInfo->sensorMarginW;
        *height = m_staticInfo->sensorMarginH;
        break;
    case HW_INFO_MAX_SENSOR_SIZE:
        *width = m_staticInfo->maxSensorW;
        *height = m_staticInfo->maxSensorH;
        break;
    case HW_INFO_MAX_PREVIEW_SIZE:
        *width = m_staticInfo->maxPreviewW;
        *height = m_staticInfo->maxPreviewH;
        break;
    case HW_INFO_MAX_PICTURE_SIZE:
        *width = m_staticInfo->maxPictureW;
        *height = m_staticInfo->maxPictureH;
        break;
    case HW_INFO_MAX_THUMBNAIL_SIZE:
        *width = m_staticInfo->maxThumbnailW;
        *height = m_staticInfo->maxThumbnailH;
        break;
    case HW_INFO_HW_YUV_SIZE:
    {
        int widthArrayNum = sizeof(m_hwYuvWidth)/sizeof(m_hwYuvWidth[0]);
        int heightArrayNum = sizeof(m_hwYuvHeight)/sizeof(m_hwYuvHeight[0]);

        if (widthArrayNum != YUV_OUTPUT_PORT_ID_MAX
            || heightArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
            android_printAssert(NULL, LOG_TAG, "ASSERT:Invalid yuvSize array length %dx%d."\
                    " YUV_OUTPUT_PORT_ID_MAX %d",
                    widthArrayNum, heightArrayNum,
                    YUV_OUTPUT_PORT_ID_MAX);
            return INVALID_OPERATION;
        }

        if (outputPortId < 0) {
            CLOGE("Invalid outputPortId(%d)", outputPortId);
            return INVALID_OPERATION;
        }

        *width = m_hwYuvWidth[outputPortId];
        *height = m_hwYuvHeight[outputPortId];
        break;
    }
    case HW_INFO_HW_YUV_INPUT_SIZE:
    {
        *width = m_hwYuvInputWidth;
        *height = m_hwYuvInputHeight;
        break;
    }
    case HW_INFO_HW_SENSOR_SIZE:
    {
        int sizeList[SIZE_LUT_INDEX_END];
        /* matched ratio LUT is not existed, use equation */
        if (m_useSizeTable == true
            && m_getPreviewSizeList(sizeList) == NO_ERROR) {
            *width = sizeList[SENSOR_W];
            *height = sizeList[SENSOR_H];
            break;
        }
    }
    case HW_INFO_HW_SENSOR_GYRO_SIZE:
    {
        int sizeList[SIZE_LUT_INDEX_END];
        /* matched ratio LUT is not existed, use equation */
        if (m_useSizeTable == true
            && m_getPreviewSizeList(sizeList) == NO_ERROR) {

            *width = sizeList[SENSOR_W];
            *height = MAX_SENSOR_GYRO_SIZE_H;
            break;
        }
    }
    case HW_INFO_MAX_HW_YUV_SIZE:
    case HW_INFO_HW_BNS_SIZE:
    case HW_INFO_HW_MAX_PICTURE_SIZE:
    case HW_INFO_HW_PICTURE_SIZE:
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    case HW_INFO_SENSOR_MODE_CHANGE_BDS_SIZE:
#endif
        *width = m_width[type];
        *height = m_height[type];
        break;
    case HW_INFO_HW_PREVIEW_SIZE:
    {
        int previewPort = -1;
        enum HW_INFO_SIZE_TYPE sizeType = HW_INFO_MAX_PREVIEW_SIZE;

        previewPort = getPreviewPortId();

        if (previewPort >= YUV_0 && previewPort < YUV_MAX) {
            sizeType = HW_INFO_HW_YUV_SIZE;
        } else {
            CLOGE("Invalid port Id. Set to default yuv size.");
            previewPort = -1;
        }

        getSize(sizeType, width, height, previewPort);
        break;
    }
#ifdef SUPPORT_REMOSAIC_CAPTURE
    case HW_INFO_HW_REMOSAIC_SENSOR_SIZE:
    {
        int sizeList[SIZE_LUT_INDEX_END];

        if (m_useSizeTable == true
           && m_staticInfo->previewHighResolutionSizeLut != NULL) {
            *width = m_staticInfo->previewHighResolutionSizeLut[m_cameraInfo.yuvSizeLutIndex][SENSOR_W];
            *height = m_staticInfo->previewHighResolutionSizeLut[m_cameraInfo.yuvSizeLutIndex][SENSOR_H];
        } else {
            *width = 0;
            *height = 0;
            CLOGE("Can not get HW_INFO_HW_REMOSAIC_SENSOR_SIZE");
        }
        break;
    }
#endif
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraParameters::resetSize(enum HW_INFO_SIZE_TYPE type)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case HW_INFO_HW_YUV_SIZE:
        memset(m_hwYuvWidth, 0, sizeof(m_hwYuvWidth));
        memset(m_hwYuvHeight, 0, sizeof(m_hwYuvHeight));
        break;
    case HW_INFO_HW_YUV_INPUT_SIZE:
    {
        m_hwYuvInputWidth = 0;
        m_hwYuvInputHeight = 0;
        break;
    }
    case HW_INFO_MAX_HW_YUV_SIZE:
        m_width[type] = 0;
        m_height[type] = 0;
        break;
    default:
        break;
    }

    return ret;
}

bool ExynosCameraParameters::isSupportedFunction(enum SUPPORTED_HW_FUNCTION_TYPE type) const
{
    bool functionSupport = false;

    switch(type) {
    case SUPPORTED_HW_FUNCTION_SENSOR_STANDBY:
        switch (m_cameraId) {
        case CAMERA_ID_BACK:
        case CAMERA_ID_FRONT:
#ifdef SUPPORT_MASTER_SENSOR_STANDBY
            functionSupport = SUPPORT_MASTER_SENSOR_STANDBY;
#endif
            break;
        case CAMERA_ID_BACK_2:
        case CAMERA_ID_BACK_3:
        case CAMERA_ID_BACK_4:
        case CAMERA_ID_FRONT_2:
        case CAMERA_ID_FRONT_3:
        case CAMERA_ID_FRONT_4:
#ifdef SUPPORT_SLAVE_SENSOR_STANDBY
            functionSupport = SUPPORT_SLAVE_SENSOR_STANDBY;
#endif
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return functionSupport;
}

status_t ExynosCameraParameters::m_getPreviewSizeList(int *sizeList)
{
    int *tempSizeList = NULL;
    int (*previewSizelist)[SIZE_OF_LUT] = NULL;
    int previewSizeLutMax = 0;
    int configMode = -1;
    int videoRatioEnum = SIZE_RATIO_16_9;
    int index = 0;

#ifdef USE_BINNING_MODE
    if (getBinningMode() == true) {
        tempSizeList = getBinningSizeTable();
        if (tempSizeList == NULL) {
            CLOGE(" getBinningSizeTable is NULL");
            return INVALID_OPERATION;
        }
    } else
#endif
    {
        configMode = m_configurations->getConfigMode();
        switch (configMode) {
        case CONFIG_MODE::NORMAL:
            {
                if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true) {
                    previewSizelist = m_staticInfo->pipPreviewSizeLut;
                    previewSizeLutMax = m_staticInfo->pipPreviewSizeLutMax;
                } else
#ifdef USE_DUAL_CAMERA
                if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
                    && m_staticInfo->useSensorCrop[m_scenario][m_camType] == true
                    && m_staticInfo->previewCropFullSizeLut != NULL) {
                    previewSizelist = m_staticInfo->previewCropFullSizeLut;
                    previewSizeLutMax = m_staticInfo->previewCropFullSizeLutMax;
                } else
#endif
                {
                    if (m_configurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE) == true) {
                        previewSizelist = m_staticInfo->videoSizeLut;
                        previewSizeLutMax = m_staticInfo->videoSizeLutMax;
                    } else if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true
                        && m_staticInfo->bnsSupport == true) {
                        previewSizelist = m_staticInfo->previewSizeLut;
                        previewSizeLutMax = m_staticInfo->previewSizeLutMax;
                    } else
#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE
                    if (m_configurations->getMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE) == false
                        && m_staticInfo->bnsSupport == true) {
                        previewSizelist = m_staticInfo->previewSizeLut;
                        previewSizeLutMax = m_staticInfo->previewSizeLutMax;
                    } else
#endif
                    {
#ifdef SUPPORT_SENSOR_MODE_CHANGE
                        if (isSensorModeTransition() == true) {
#ifdef SUPPORT_REMOSAIC_CAPTURE
                            if (m_configurations->getMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE)
                                && m_staticInfo->previewHighResolutionSizeLut != NULL) {
                                CLOGD("[remosaic] previewHighResolutionSizeLut");
                                previewSizelist = m_staticInfo->previewHighResolutionSizeLut;
                                previewSizeLutMax = m_staticInfo->previewHighResolutionSizeLutMax;
                            } else
#endif //SUPPORT_REMOSAIC_CAPTURE
                            {
                                previewSizelist = m_staticInfo->previewSizeLut;
                                previewSizeLutMax = m_staticInfo->previewSizeLutMax;
                            }
                        } else
#endif //SUPPORT_SENSOR_MODE_CHANGE
                        {
                            previewSizelist = m_staticInfo->previewFullSizeLut;
                            previewSizeLutMax = m_staticInfo->previewFullSizeLutMax;
                        }
                    }
                }

                if (previewSizelist == NULL) {
                    CLOGE("previewSizeLut is NULL");
                    return INVALID_OPERATION;
                }

                if (m_getSizeListIndex(previewSizelist, previewSizeLutMax, m_cameraInfo.yuvSizeRatioId, &m_cameraInfo.yuvSizeLutIndex) != NO_ERROR) {
                    CLOGE("unsupported preview ratioId(%d)", m_cameraInfo.yuvSizeRatioId);
                    return BAD_VALUE;
                }

                tempSizeList = previewSizelist[m_cameraInfo.yuvSizeLutIndex];
            }
            break;
        case CONFIG_MODE::HIGHSPEED_60:
            {
                if (m_staticInfo->videoSizeLutHighSpeed60Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed60 == NULL) {
                    CLOGE("videoSizeLutHighSpeed60 is NULL");
                } else {
                    for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed60Max; index++) {
                        if (m_staticInfo->videoSizeLutHighSpeed60[index][RATIO_ID] == videoRatioEnum) {
                            break;
                        }
                    }

                    if (index >= m_staticInfo->videoSizeLutHighSpeed60Max)
                        index = 0;

                    tempSizeList = m_staticInfo->videoSizeLutHighSpeed60[index];
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_120:
            {
                if (m_staticInfo->videoSizeLutHighSpeed120Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed120 == NULL) {
                     CLOGE(" videoSizeLutHighSpeed120 is NULL");
                } else {
                    for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed120Max; index++) {
                        if (m_staticInfo->videoSizeLutHighSpeed120[index][RATIO_ID] == videoRatioEnum) {
                            break;
                        }
                    }

                    if (index >= m_staticInfo->videoSizeLutHighSpeed120Max)
                        index = 0;

                    tempSizeList = m_staticInfo->videoSizeLutHighSpeed120[index];
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_240:
            {
                if (m_staticInfo->videoSizeLutHighSpeed240Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed240 == NULL) {
                     CLOGE(" videoSizeLutHighSpeed240 is NULL");
                } else {
                    for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed240Max; index++) {
                        if (m_staticInfo->videoSizeLutHighSpeed240[index][RATIO_ID] == videoRatioEnum) {
                            break;
                        }
                    }

                    if (index >= m_staticInfo->videoSizeLutHighSpeed240Max)
                        index = 0;

                    tempSizeList = m_staticInfo->videoSizeLutHighSpeed240[index];
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_480:
            {
                if (m_staticInfo->videoSizeLutHighSpeed480Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed480 == NULL) {
                    CLOGE(" videoSizeLutHighSpeed480 is NULL");
                } else {
                    for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed480Max; index++) {
                        if (m_staticInfo->videoSizeLutHighSpeed480[index][RATIO_ID] == videoRatioEnum) {
                            break;
                        }
                    }

                    if (index >= m_staticInfo->videoSizeLutHighSpeed480Max)
                        index = 0;

                    tempSizeList = m_staticInfo->videoSizeLutHighSpeed480[index];
                }
            }
            break;
        }
    }

    if (tempSizeList == NULL) {
         CLOGE(" fail to get LUT");
        return INVALID_OPERATION;
    }

    for (int i = 0; i < SIZE_LUT_INDEX_END; i++)
        sizeList[i] = tempSizeList[i];

    return NO_ERROR;
}

status_t ExynosCameraParameters::m_getPictureSizeList(int *sizeList)
{
    int *tempSizeList = NULL;
    int (*pictureSizelist)[SIZE_OF_LUT] = NULL;
    int pictureSizelistMax = 0;
    int configMode = -1;
    int captureRatio = SIZE_RATIO_16_9;

#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (m_configurations->getMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE) == true) {
        CLOGD("[REMOSAIC] pictureSize by captureHighResolutionSizeLut");
        pictureSizelist = m_staticInfo->captureHighResolutionSizeLut;
        pictureSizelistMax = m_staticInfo->captureHighResolutionSizeLutMax;
    } else
#endif //SUPPORT_REMOSAIC_CAPTURE
#ifdef SUPPORT_HIGHSPEED_REPROCESSING_LUT
    if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
        configMode = m_configurations->getConfigMode();

        switch (configMode) {
        case CONFIG_MODE::NORMAL:
            break;
        case CONFIG_MODE::HIGHSPEED_60:
            {
                if (m_staticInfo->videoSizeLutHighSpeed60Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed60 == NULL) {
                    CLOGE("videoSizeLutHighSpeed60 is NULL");
                } else {
                    pictureSizelist = m_staticInfo->videoSizeLutHighSpeed60;
                    pictureSizelistMax = m_staticInfo->videoSizeLutHighSpeed60Max;
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_120:
            {
                if (m_staticInfo->videoSizeLutHighSpeed120Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed120 == NULL) {
                     CLOGE(" videoSizeLutHighSpeed120 is NULL");
                } else {
                    pictureSizelist = m_staticInfo->videoSizeLutHighSpeed120;
                    pictureSizelistMax = m_staticInfo->videoSizeLutHighSpeed120Max;
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_240:
            {
                if (m_staticInfo->videoSizeLutHighSpeed240Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed240 == NULL) {
                     CLOGE(" videoSizeLutHighSpeed240 is NULL");
                } else {
                    pictureSizelist = m_staticInfo->videoSizeLutHighSpeed240;
                    pictureSizelistMax = m_staticInfo->videoSizeLutHighSpeed240Max;
                }
            }
            break;
        case CONFIG_MODE::HIGHSPEED_480:
            {
                if (m_staticInfo->videoSizeLutHighSpeed480Max == 0
                        || m_staticInfo->videoSizeLutHighSpeed480 == NULL) {
                    CLOGE(" videoSizeLutHighSpeed480 is NULL");
                } else {
                    pictureSizelist = m_staticInfo->videoSizeLutHighSpeed480;
                    pictureSizelistMax = m_staticInfo->videoSizeLutHighSpeed480Max;
                }
            }
            break;
        }

    } else
#endif
#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_staticInfo->useSensorCrop[m_scenario][m_camType] == true
        && m_staticInfo->pictureCropFullSizeLut != NULL) {
        pictureSizelist = m_staticInfo->pictureCropFullSizeLut;
        pictureSizelistMax = m_staticInfo->pictureCropFullSizeLutMax;
    } else
#endif
#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE
    if (m_configurations->getMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE)) {
        pictureSizelist = m_staticInfo->pictureFullSizeLut;
        pictureSizelistMax = m_staticInfo->pictureFullSizeLutMax;
    } else
#endif
    {
        pictureSizelist = m_staticInfo->pictureSizeLut;
        pictureSizelistMax = m_staticInfo->pictureSizeLutMax;
    }

    if (pictureSizelist == NULL) {
        CLOGE("pictureSizelist is NULL");
        return INVALID_OPERATION;
    }

    if (m_getSizeListIndex(pictureSizelist, pictureSizelistMax, m_cameraInfo.pictureSizeRatioId, &m_cameraInfo.pictureSizeLutIndex) != NO_ERROR) {
        CLOGE("unsupported picture ratioId(%d)", m_cameraInfo.pictureSizeRatioId);
        return BAD_VALUE;
    }

    tempSizeList = pictureSizelist[m_cameraInfo.pictureSizeLutIndex];

    if (tempSizeList == NULL) {
         CLOGE(" fail to get LUT");
        return INVALID_OPERATION;
    }

    for (int i = 0; i < SIZE_LUT_INDEX_END; i++)
        sizeList[i] = tempSizeList[i];

    return NO_ERROR;
}

bool ExynosCameraParameters::m_isSupportedFullSizePicture(void)
{
    /* To support multi ratio picture, use size of picture as full size. */
    return true;
}

status_t ExynosCameraParameters::getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom)
{
    return getPictureBayerCropSizeImpl(srcRect, dstRect, applyZoom, isUseReprocessing3aaInputCrop());
}

status_t ExynosCameraParameters::getRemosaicBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom)
{
    return getPictureBayerCropSizeImpl(srcRect, dstRect, applyZoom, true);
}

status_t ExynosCameraParameters::getPictureBayerCropSizeImpl(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom, bool is3aaInputCrop)
{
    int hwBnsW   = 0;
    int hwBnsH   = 0;
    int hwBcropW = 0;
    int hwBcropH = 0;
    int hwSensorMarginW = 0;
    int hwSensorMarginH = 0;
    int sizeList[SIZE_LUT_INDEX_END];

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_getPictureSizeList(sizeList) != NO_ERROR
        || m_isSupportedFullSizePicture() == false)
        return calcPictureBayerCropSize(srcRect, dstRect);

    /* use LUT */
    hwBnsW   = sizeList[BNS_W];
    hwBnsH   = sizeList[BNS_H];
    hwBcropW = sizeList[BCROP_W];
    hwBcropH = sizeList[BCROP_H];

    /* Re-calculate the BNS size for removing Sensor Margin.
       On Capture Stream(3AA_M2M_Input), the BNS is not used.
       So, the BNS ratio is not needed to be considered for sensor margin
     */
    getSize(HW_INFO_SENSOR_MARGIN_SIZE, (uint32_t *)&hwSensorMarginW, (uint32_t *)&hwSensorMarginH);
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

    getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);

    if (is3aaInputCrop == true) {
        {
            m_getPictureCropRegion(&cropRegion.x, &cropRegion.y, &cropRegion.w, &cropRegion.h);
        }
    }

    if (applyZoom == false) {
        cropRegion.x = 0;
        cropRegion.y = 0;
        cropRegion.w = activeArraySize.w;
        cropRegion.h = activeArraySize.h;
    }

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
       to adjust it to the 3AA input size without sensor margin
     */
    scaleRatioW = (float) hwBnsW / (float) hwActiveArraySize.w;
    scaleRatioH = (float) hwBnsH / (float) hwActiveArraySize.h;

    cropRegion.x = ALIGN_DOWN((int) (((float) cropRegion.x) * scaleRatioW), CAMERA_BCROP_ALIGN);
    cropRegion.y = ALIGN_DOWN((int) (((float) cropRegion.y) * scaleRatioH), 2);
    cropRegion.w = (int) ((float) cropRegion.w) * scaleRatioW;
    cropRegion.h = (int) ((float) cropRegion.h) * scaleRatioH;

    hwActiveArraySize.x = ALIGN_DOWN((int) (((float) hwActiveArraySize.x) * scaleRatioW), CAMERA_BCROP_ALIGN);
    hwActiveArraySize.y = ALIGN_DOWN((int) (((float) hwActiveArraySize.y) * scaleRatioH), 2);
    hwActiveArraySize.w = (int) (((float) hwActiveArraySize.w) * scaleRatioW);
    hwActiveArraySize.h = (int) (((float) hwActiveArraySize.h) * scaleRatioH);

    if (cropRegion.w < 1 || cropRegion.h < 1) {
        CLOGW("Invalid scaleRatio %fx%f, cropRegion' %d,%d %dx%d.",
                scaleRatioW, scaleRatioH,
                cropRegion.x, cropRegion.y, cropRegion.w, cropRegion.h);
        cropRegion.x = 0;
        cropRegion.y = 0;
        cropRegion.w = hwBnsW;
        cropRegion.h = hwBnsH;
    }

    /* Calculate HW bcrop size inside crop region' */
    if ((cropRegion.w > hwBcropW) && (cropRegion.h > hwBcropH)) {
        dstRect->x = ALIGN_DOWN(((cropRegion.w - hwBcropW) >> 1), CAMERA_BCROP_ALIGN);
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

        dstRect->x = ALIGN_DOWN(dstRect->x, CAMERA_BCROP_ALIGN);
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

    CLOGV("HWBcropSize %d,%d %dx%d(%d)",
            dstRect->x, dstRect->y, dstRect->w, dstRect->h, SIZE_RATIO(dstRect->w, dstRect->h));

    /* Compensate the crop size to satisfy Max Scale Up Ratio */
    if (dstRect->w * SCALER_MAX_SCALE_UP_RATIO < hwBnsW
        || dstRect->h * SCALER_MAX_SCALE_UP_RATIO < hwBnsH) {
        dstRect->w = ALIGN_UP((int)ceil((float)hwBnsW / SCALER_MAX_SCALE_UP_RATIO), CAMERA_BCROP_ALIGN);
        dstRect->h = ALIGN_UP((int)ceil((float)hwBnsH / SCALER_MAX_SCALE_UP_RATIO), CAMERA_BCROP_ALIGN);
    }

#if DEBUG_PERFRAME
    CLOGD("hwBnsSize %dx%d, hwBcropSize %d,%d %dx%d",
            srcRect->w, srcRect->h,
            dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCameraParameters::m_getPreviewBdsSize(ExynosRect *dstRect)
{
    int hwBdsW = 0;
    int hwBdsH = 0;
    int sizeList[SIZE_LUT_INDEX_END];
    int maxYuvW = 0, maxYuvH = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_getPreviewSizeList(sizeList) != NO_ERROR) {
        ExynosRect rect;
        return calcPreviewBDSSize(&rect, dstRect);
    }

    /* use LUT */
    hwBdsW = sizeList[BDS_W];
    hwBdsH = sizeList[BDS_H];

    getSize(HW_INFO_MAX_HW_YUV_SIZE, (uint32_t *)&maxYuvW, (uint32_t *)&maxYuvH);

#ifndef USE_FIXED_BDS_SIZE
    if (isBackCamera(getCameraId())) {
        {
            if (maxYuvW > UHD_DVFS_CEILING_WIDTH || maxYuvH > UHD_DVFS_CEILING_HEIGHT) {
                hwBdsW = sizeList[BCROP_W];
                hwBdsH = sizeList[BCROP_H];
                m_configurations->setMode(CONFIGURATION_FULL_SIZE_LUT_MODE, true);
            } else if (maxYuvW > sizeList[BDS_W] || maxYuvH > sizeList[BDS_H]) {
                switch(m_staticInfo->sensorArrayRatio) {
                case SIZE_RATIO_16_9:
                    hwBdsW = UHD_16X9_DVFS_CEILING_WIDTH;
                    hwBdsH = UHD_16X9_DVFS_CEILING_HEIGHT;
                    break;
                case SIZE_RATIO_4_3:
                default:
                    hwBdsW = UHD_DVFS_CEILING_WIDTH;
                    hwBdsH = UHD_DVFS_CEILING_HEIGHT;
                    break;
                }
                m_configurations->setMode(CONFIGURATION_FULL_SIZE_LUT_MODE, true);
            }
        }
    } else {
        {
            if (maxYuvW > sizeList[BDS_W] || maxYuvH > sizeList[BDS_H]) {
                hwBdsW = sizeList[BCROP_W];
                hwBdsH = sizeList[BCROP_H];
                m_configurations->setMode(CONFIGURATION_FULL_SIZE_LUT_MODE, true);
            }
        }
    }
#endif

/*
#ifdef USE_FIXED_BDS_SIZE
    int onePortId = m_configurations->getOnePortId();
    m_configurations->getSize(CONFIGURATION_YUV_SIZE, (uint32_t *)&hwBdsW, (uint32_t *) &hwBdsH, onePortId);
#endif
*/

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = hwBdsW;
    dstRect->h = hwBdsH;

#ifdef DEBUG_PERFRAME
    CLOGD("hwBdsSize (%dx%d)", dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCameraParameters::checkYuvSize(const int width, const int height, const int outputPortId, bool reprocessing)
{
    int curYuvWidth = 0;
    int curYuvHeight = 0;
    uint32_t minYuvW = 0, minYuvH = 0;
    uint32_t maxYuvW = 0, maxYuvH = 0;
    int curRatio = m_configurations->getYuvSizeRatioId();
    bool isMultipleRatio = false;
    bool ret = true;

    m_configurations->getSize(CONFIGURATION_YUV_SIZE, (uint32_t *)&curYuvWidth, (uint32_t *)&curYuvHeight, outputPortId);

    int camIdx = m_getSensorInfoCamIdx();
    if (m_camType == camIdx) {
        if (reprocessing) {
            ret = m_isSupportedPictureSize(width, height);
        } else {
            ret = m_isSupportedYuvSize(width, height, outputPortId, &curRatio);
        }

        if (!ret) {
            CLOGE("Invalid YUV size. %dx%d", width, height);
            return BAD_VALUE;
        }
    }

    if (curRatio < 0) {
        CLOGE("Invalid curRatio(%d)!!! / camType(%d) curYuvSize(%dx%d) newYuvSize(%dx%d) outputPortId(%d)",
                curRatio, m_camType, curYuvWidth, curYuvHeight, width, height, outputPortId);
    } else {
        CLOGI("camType(%d) curYuvSize(%dx%d) newYuvSize(%dx%d) outputPortId(%d) curRatio(%d)",
                m_camType, curYuvWidth, curYuvHeight, width, height, outputPortId, curRatio);
    }

    if ((curYuvWidth != width || curYuvHeight != height) && m_camType == camIdx) {
        m_configurations->setSize(CONFIGURATION_YUV_SIZE, width, height, outputPortId);
    }

    if (outputPortId < YUV_MAX) {
        /* Update minimum YUV size */
        m_configurations->getSize(CONFIGURATION_MIN_YUV_SIZE, &minYuvW, &minYuvH);
        if (minYuvW == 0) {
            m_configurations->setSize(CONFIGURATION_MIN_YUV_SIZE, width, minYuvH);
        } else if ((uint32_t)width < minYuvW) {
            m_configurations->setSize(CONFIGURATION_MIN_YUV_SIZE, width, minYuvH);
        }

        m_configurations->getSize(CONFIGURATION_MIN_YUV_SIZE, &minYuvW, &minYuvH);
        if (minYuvH == 0) {
            m_configurations->setSize(CONFIGURATION_MIN_YUV_SIZE, minYuvW, height);
        } else if ((uint32_t)height < minYuvH) {
            m_configurations->setSize(CONFIGURATION_MIN_YUV_SIZE, minYuvW, height);
        }

        if (m_cameraInfo.yuvSizeRatioId != curRatio && m_cameraInfo.yuvSizeRatioId != m_staticInfo->sensorArrayRatio) {
            isMultipleRatio = true;
        }

        /* Update maximum YUV size */
        m_configurations->getSize(CONFIGURATION_MAX_YUV_SIZE, &maxYuvW, &maxYuvH);
        if(maxYuvW == 0) {
            m_configurations->setSize(CONFIGURATION_MAX_YUV_SIZE, width, maxYuvH);
            m_configurations->setYuvSizeRatioId(curRatio);
            m_cameraInfo.yuvSizeRatioId = curRatio;
        } else if ((uint32_t)width >= maxYuvW) {
            m_configurations->setSize(CONFIGURATION_MAX_YUV_SIZE, width, maxYuvH);
            m_configurations->setYuvSizeRatioId(curRatio);
            m_cameraInfo.yuvSizeRatioId = curRatio;
        }

        m_configurations->getSize(CONFIGURATION_MAX_YUV_SIZE, &maxYuvW, &maxYuvH);
        if(maxYuvH == 0) {
            m_configurations->setSize(CONFIGURATION_MAX_YUV_SIZE, maxYuvW, height);
            m_configurations->setYuvSizeRatioId(curRatio);
            m_cameraInfo.yuvSizeRatioId = curRatio;
        } else if ((uint32_t)height >= maxYuvH) {
            m_configurations->setSize(CONFIGURATION_MAX_YUV_SIZE, maxYuvW, height);
            m_configurations->setYuvSizeRatioId(curRatio);
            m_cameraInfo.yuvSizeRatioId = curRatio;
        }

        if (isMultipleRatio == true) {
            m_configurations->setYuvSizeRatioId(m_staticInfo->sensorArrayRatio);
            m_cameraInfo.yuvSizeRatioId = m_staticInfo->sensorArrayRatio;
        }

        m_configurations->getSize(CONFIGURATION_MIN_YUV_SIZE, &minYuvW, &minYuvH);
        m_configurations->getSize(CONFIGURATION_MAX_YUV_SIZE, &maxYuvW, &maxYuvH);

#ifdef USE_DUAL_CAMERA
        if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
            //forcely set the curRatio to all sensor.
            m_cameraInfo.yuvSizeRatioId = curRatio;
        }
#endif
        CLOGV("m_minYuvW(%d) m_minYuvH(%d) m_maxYuvW(%d) m_maxYuvH(%d)",
            minYuvW, minYuvH, maxYuvW, maxYuvH);
    } else {
        /* Update minimum YUV size */
        m_configurations->getSize(CONFIGURATION_MIN_YUV_STALL_SIZE, &minYuvW, &minYuvH);
        minYuvW = (minYuvW == 0) ? width : (((uint32_t)width < minYuvW) ? width : minYuvW);
        minYuvH = (minYuvH == 0) ? height : (((uint32_t)height < minYuvH) ? height : minYuvH);
        m_configurations->setSize(CONFIGURATION_MIN_YUV_STALL_SIZE, minYuvW, minYuvH);

        /* Update maximum YUV size */
        m_configurations->getSize(CONFIGURATION_MAX_YUV_STALL_SIZE, &maxYuvW, &maxYuvH);
        maxYuvW = (maxYuvW == 0) ? width : (((uint32_t)width > maxYuvW) ? width : maxYuvW);
        maxYuvH = (maxYuvH == 0) ? height : (((uint32_t)height > maxYuvH) ? height : maxYuvH);
        m_configurations->setSize(CONFIGURATION_MAX_YUV_STALL_SIZE, maxYuvW, maxYuvH);

        CLOGV("[STALL] m_minYuvW(%d) m_minYuvH(%d) m_maxYuvW(%d) m_maxYuvH(%d)",
            minYuvW, minYuvH, maxYuvW, maxYuvH);
    }

    return NO_ERROR;
}

status_t ExynosCameraParameters::checkHwYuvSize(const int hwWidth, const int hwHeight, const int outputPortId, bool isSameSensorSize)
{
    int curHwYuvWidth = 0;
    int curHwYuvHeight = 0;

    getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&curHwYuvWidth, (uint32_t *)&curHwYuvHeight, outputPortId);

    CLOGI("curHwYuvSize %dx%d newHwYuvSize %dx%d outputPortId %d",
            curHwYuvWidth, curHwYuvHeight, hwWidth, hwHeight, outputPortId);

    if (curHwYuvWidth != hwWidth || curHwYuvHeight != hwHeight) {
        if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) {
            if (m_camType == MAIN_CAM || isSameSensorSize == true) {
                setSize(HW_INFO_HW_YUV_SIZE, hwWidth, hwHeight, outputPortId);
            } else {
                int sizeList[SIZE_LUT_INDEX_END];
                if (m_getPreviewSizeList(sizeList) != NO_ERROR) {
                    CLOGE("Fail m_getPreviewSizeList()!!");
                } else {
                    CLOGI("[setParameters] Sub Camera PreviweSize (%d %d)",
                            sizeList[TARGET_W], sizeList[TARGET_H]);
                    setSize(HW_INFO_HW_YUV_SIZE, sizeList[TARGET_W], sizeList[TARGET_H], outputPortId);
                }
            }
        } else {
            setSize(HW_INFO_HW_YUV_SIZE, hwWidth, hwHeight, outputPortId);
        }
    }

    if (outputPortId < YUV_MAX) {
        /* Update maximum YUV size */
        uint32_t maxHwYuvW = 0, maxHwYuvH = 0;

        getSize(HW_INFO_MAX_HW_YUV_SIZE, &maxHwYuvW, &maxHwYuvH);
        getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&curHwYuvWidth, (uint32_t *)&curHwYuvHeight, outputPortId);

        if ((uint32_t)curHwYuvWidth > maxHwYuvW) {
            maxHwYuvW = curHwYuvWidth;
        }

        if ((uint32_t)curHwYuvHeight > maxHwYuvH) {
            maxHwYuvH = curHwYuvHeight;
        }

        setSize(HW_INFO_MAX_HW_YUV_SIZE, maxHwYuvW, maxHwYuvH);
        CLOGD("m_maxYuvW(%d) m_maxYuvH(%d)", maxHwYuvW, maxHwYuvH);
    }

    return NO_ERROR;
}

void ExynosCameraParameters::getYuvVendorSize(int *width, int *height, int index, __unused ExynosRect ispSize)
{
    int widthArrayNum = sizeof(m_cameraInfo.hwYuvWidth)/sizeof(m_cameraInfo.hwYuvWidth[0]);
    int heightArrayNum = sizeof(m_cameraInfo.hwYuvHeight)/sizeof(m_cameraInfo.hwYuvHeight[0]);
    bool hifiLLsMode = false, nightShotYuv = false, hdrYuv = false, beautyFaceYuv = false, oisDenoiseYuv = false, superResolution = false, singleCapture = false;

#ifdef USES_HIFI_LLS
    hifiLLsMode = m_configurations->getMode(CONFIGURATION_HIFI_LLS_MODE);
#endif
#ifdef USES_COMBINE_PLUGIN
    nightShotYuv = m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE);
    hdrYuv = m_configurations->getMode(CONFIGURATION_HDR_YUV_MODE);
    beautyFaceYuv = m_configurations->getMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE);
    oisDenoiseYuv = m_configurations->getMode(CONFIGURATION_OIS_DENOISE_YUV_MODE);
    superResolution = m_configurations->getMode(CONFIGURATION_SUPER_RESOLUTION_MODE);
    singleCapture = m_configurations->getMode(CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE);
#endif

    if (widthArrayNum != YUV_OUTPUT_PORT_ID_MAX
            || heightArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT:Invalid yuvSize array length %dx%d."\
                " YUV_OUTPUT_PORT_ID_MAX %d",
                widthArrayNum, heightArrayNum,
                YUV_OUTPUT_PORT_ID_MAX);
        return;
    }

    if ((index >= ExynosCameraParameters::YUV_STALL_0)
            && (m_configurations->getModeValue(CONFIGURATION_YUV_STALL_PORT) == index)) {
        if (m_configurations->getModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE) == YUV_STALL_USAGE_DSCALED) {
            m_configurations->getSize(CONFIGURATION_DS_YUV_STALL_SIZE, (uint32_t *)width, (uint32_t *)height);
        } else {
#ifdef USE_DUAL_CAMERA
            if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
                getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)width, (uint32_t *)height);
                CLOGI("[DUAL] Set the YUV[%d] size to HW_PICTURE_SIZE (%d x %d)", index, *width, *height);
            } else
#endif
            if (hifiLLsMode || nightShotYuv || hdrYuv || beautyFaceYuv || oisDenoiseYuv || superResolution || singleCapture) {
                status_t ret = NO_ERROR;
                ExynosRect ratioCropSize;
                uint32_t jpegWidth = 0, jpegHeight = 0;

                if (m_configurations->getMode(CONFIGURATION_VENDOR_YUV_STALL)) {
                    m_configurations->getSize(CONFIGURATION_MAX_YUV_STALL_SIZE, (uint32_t *)&jpegWidth, (uint32_t *)&jpegHeight);
                } else {
                    getSize(HW_INFO_HW_PICTURE_SIZE, &jpegWidth, &jpegHeight);
                }

                ////////////////////////////////////////////////
                // This code is for align, on mcsc output size.
                int wAlign = CAMERA_MCSC_ALIGN;
                int hAlign = 2;

                if (hifiLLsMode == true) {
                    wAlign = CAMERA_16PX_ALIGN;
                    hAlign = CAMERA_16PX_ALIGN;
                }

                ret = getCropRectAlign(
                        jpegWidth, jpegHeight, jpegWidth, jpegHeight,
                        &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
                        wAlign, hAlign, 1.0f);

                *width = ratioCropSize.w;
                *height = ratioCropSize.h;
                CLOGD("jpeg size(%d %d) isp(%d %d) crop(%d %d %d %d)", jpegWidth, jpegHeight, ispSize.w, ispSize.h, ratioCropSize.x, ratioCropSize.y, ratioCropSize.w, ratioCropSize.h);

                ////////////////////////////////////////////////
            } else {
                getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)width, (uint32_t *)height, index);
                CLOGI("Set the YUV[%d] size to HW_PICTURE_SIZE (%d x %d)", index, *width, *height);
            }
        }
    } else {
#ifdef USE_DUAL_CAMERA
        if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION
                && m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
            /*
             * Multiple camera should use oneport for fusion scenario.
             * But in that case, it should be considered with margin size.
             */
            if (isAlternativePreviewPortId(index) == true ||
                isPreviewPortId(index) == true ||
                isPreviewCbPortId(index) == true) {
                ExynosRect fusionSrcRect;
                ExynosRect fusionDstRect;
                int margin = getActiveZoomMargin();

                getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)width, (uint32_t *)height, index);
                if (margin == DUAL_SOLUTION_MARGIN_VALUE_30 ||
                        margin == DUAL_SOLUTION_MARGIN_VALUE_20) {
                    getFusionSize(*width, *height,
                            &fusionSrcRect, &fusionDstRect,
                            margin);
                    *width = fusionSrcRect.w;
                    *height = fusionSrcRect.h;
                }
            } else {
                getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)width, (uint32_t *)height, index);
            }
        } else
#endif
        {
            getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)width, (uint32_t *)height, index);
        }
    }
}

#ifdef SUPPORT_DEPTH_MAP
bool ExynosCameraParameters::isDepthMapSupported(void) {
    return false;
}

status_t ExynosCameraParameters::getDepthMapSize(int *depthMapW, int *depthMapH)
{
    /* set by HAL if Depth Stream does not exist */
    if (isDepthMapSupported() == true && m_depthMapW == 0 && m_depthMapH == 0) {
        for (int index = 0; index < m_staticInfo->depthMapSizeLutMax; index++) {
            if (m_staticInfo->depthMapSizeLut[index][RATIO_ID] == m_cameraInfo.yuvSizeRatioId) {
                *depthMapW = m_staticInfo->depthMapSizeLut[index][SENSOR_W];
                *depthMapH = m_staticInfo->depthMapSizeLut[index][SENSOR_H];
                return NO_ERROR;
            }
        }
    }

    /* set by user */
    *depthMapW = m_depthMapW;
    *depthMapH = m_depthMapH;

    return NO_ERROR;
}

void ExynosCameraParameters::setDepthMapSize(int depthMapW, int depthMapH)
{
    m_depthMapW = depthMapW;
    m_depthMapH = depthMapH;
}
#endif

int ExynosCameraParameters::getSensorGyroFormat(void)
{
    return SENSOR_GYRO_FORMAT;
}

bool ExynosCameraParameters::isSensorGyroSupported(void)
{
    bool ret = false;

    ExynosCameraSensorInfoBase *sensorInfo = this->getSensorStaticInfo();
    ret = sensorInfo->sensorGyroSupport;

    return ret;
}

#ifdef SUPPORT_PD_IMAGE
bool ExynosCameraParameters::isPDImageSupported(void) {
    if (m_staticInfo->pdImageSizeLut != NULL) {
        return true;
    }

    return false;
}

status_t ExynosCameraParameters::getPDImageSize(int &pdImageW, int &pdImageH)
{
    int hwSensorW = 0, hwSensorH = 0;

    getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    if (isPDImageSupported() == true) {
        for (int index = 0; index < m_staticInfo->pdImageSizeLutMax; index++) {
            if ((m_staticInfo->pdImageSizeLut[index][PD_SENSOR_W] == hwSensorW)
                && (m_staticInfo->pdImageSizeLut[index][PD_SENSOR_H] == hwSensorH)) {
                pdImageW = m_staticInfo->pdImageSizeLut[index][PD_IMAGE_W];
                pdImageH = m_staticInfo->pdImageSizeLut[index][PD_IMAGE_H];
                return NO_ERROR;
            }
        }
    }

    /* set by user */
    pdImageW = MAX_PD_IMAGE_W;
    pdImageH = MAX_PD_IMAGE_H;

    return NO_ERROR;
}
#endif

void ExynosCameraParameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
                                                        ExynosRect       *pictureRect,
                                                        ExynosRect       *thumbnailRect,
                                                        camera2_shot_ext *shot_ext,
                                                        bool                useDebugInfo2)
{
    unsigned int offset = 0;
    bool isWriteExif = false;
    uint64_t exposureTimeCapture = 0L;
    debug_attribute_t &debugInfo = ((useDebugInfo2 == false) ? mDebugInfo : mDebugInfo2);
    camera2_shot_t *shot = &(shot_ext->shot);

    m_staticInfoExifLock.lock();
    /* Maker Note */
    /* Clear previous debugInfo data */
    if (debugInfo.debugData[APP_MARKER_4] != NULL) {
        memset((void *)debugInfo.debugData[APP_MARKER_4], 0, debugInfo.debugSize[APP_MARKER_4]);
        /* back-up udm info for exif's maker note */
        memcpy((void *)debugInfo.debugData[APP_MARKER_4], (void *)&shot->udm, sizeof(struct camera2_udm));
        offset = sizeof(struct camera2_udm);

#ifndef DISABLE_USER_META
        memcpy((void *)(debugInfo.debugData[APP_MARKER_4] + offset), (void *)&(shot_ext->user.ddk_version), sizeof(struct ddk_setfile_ver));
        offset += sizeof(struct ddk_setfile_ver);
#endif
    }

    m_vendorSetExifChangedAttribute(debugInfo, offset, isWriteExif, shot_ext, useDebugInfo2);

    /* APP5 Maker Note */
    /* Clear previous debugInfo data */
    if (debugInfo.debugData[APP_MARKER_5] != NULL && debugInfo.debugSize[APP_MARKER_5] != 0) {
        memset((void *)debugInfo.debugData[APP_MARKER_5], 0, debugInfo.debugSize[APP_MARKER_5]);
        if (m_thumbHistInfo.bufSize[0] != 0 && m_thumbHistInfo.bufAddr[0] !=NULL ){
            /* Store the first part thumbnail&histogram stat data. */
            memcpy((void *)debugInfo.debugData[APP_MARKER_5], (void *)(m_thumbHistInfo.bufAddr[0]),m_thumbHistInfo.bufSize[0]);
        }
    }
    /* APP6 Maker Note */
    /* Clear previous debugInfo data */
    /*
    if (debugInfo.debugData[APP_MARKER_6] != NULL && debugInfo.debugSize[APP_MARKER_6] != 0) {
        memset((void *)debugInfo.debugData[APP_MARKER_6], 0, debugInfo.debugSize[APP_MARKER_6]);
        if (m_thumbHistInfo.bufSize[1] != 0 && m_thumbHistInfo.bufAddr[1] != NULL ){
            // Store the second of thumbnail&histogram stat data. 
            memcpy((void *)debugInfo.debugData[APP_MARKER_6], m_thumbHistInfo.magicPrefix, sizeof(m_thumbHistInfo.magicPrefix) - 1);
            offset = sizeof(m_thumbHistInfo.magicPrefix) - 1;
            memcpy((void *)(debugInfo.debugData[APP_MARKER_6] + offset), (void *)(m_thumbHistInfo.bufAddr[1]),m_thumbHistInfo.bufSize[1]- offset);
        }
    }
    */
    m_staticInfoExifLock.unlock();

    if (exifInfo == NULL || pictureRect == NULL || thumbnailRect == NULL) {
        return;
    }

    /* JPEG Picture Size */
    exifInfo->width = pictureRect->w;
    exifInfo->height = pictureRect->h;

    /* Orientation */
    switch (shot->ctl.jpeg.orientation) {
        case 90:
            exifInfo->orientation = EXIF_ORIENTATION_90;
            break;
        case 180:
            exifInfo->orientation = EXIF_ORIENTATION_180;
            break;
        case 270:
            exifInfo->orientation = EXIF_ORIENTATION_270;
            break;
        case 0:
        default:
            exifInfo->orientation = EXIF_ORIENTATION_UP;
            break;
    }

    /* Date Time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    snprintf((char *)exifInfo->sec_time, 5, "%04d", (int)(rawtime.tv_usec/1000));

    /*
     * vendorSpecific2[0] : info
     * vendorSpecific2[100] : 0:sirc 1:cml
     * vendorSpecific2[101] : cml exposure
     * vendorSpecific2[102] : cml iso(gain)
     * vendorSpecific2[103] : cml Bv
     */

    /* ISO Speed Rating */
#if 0 /* TODO: Must be same with the sensitivity in Result Metadata */
    exifInfo->iso_speed_rating = shot->udm.internal.vendorSpecific[1];
#else
    if (shot->dm.sensor.sensitivity < (uint32_t)m_staticInfo->sensitivityRange[MIN]) {
        exifInfo->iso_speed_rating = m_staticInfo->sensitivityRange[MIN];
    } else {
        exifInfo->iso_speed_rating = shot->dm.sensor.sensitivity;
    }
#endif

    exposureTimeCapture = m_configurations->getCaptureExposureTime();

    /* Exposure Program */
    if (exposureTimeCapture == 0) {
        exifInfo->exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    } else {
        exifInfo->exposure_program = EXIF_DEF_EXPOSURE_MANUAL;
    }

    /* Exposure Time */
    exifInfo->exposure_time.num = 1;
    if (exposureTimeCapture <= CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT) {
        /* HACK : Sometimes, F/W does NOT send the exposureTime */
        if (shot->dm.sensor.exposureTime != 0)
            exifInfo->exposure_time.den = (uint32_t)round((double)1e9 / shot->dm.sensor.exposureTime);
        else
            exifInfo->exposure_time.num = 0;
    } else {
        exifInfo->exposure_time.num = (uint32_t) (exposureTimeCapture / 1000);
        exifInfo->exposure_time.den = 1000;
    }

    /* Shutter Speed */
    if (exposureTimeCapture <= CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT) {
        exifInfo->shutter_speed.num = (int32_t) (ROUND_OFF_HALF(((double) (shot->udm.internal.vendorSpecific[3] / 256.f) * EXIF_DEF_APEX_DEN), 0));
        exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;
    } else {
        exifInfo->shutter_speed.num = (int32_t)(log2((double)exposureTimeCapture / 1000000) * -100);
        exifInfo->shutter_speed.den = 100;
    }

    /* Aperture */
    uint32_t aperture_num = 0;
    float aperture_f = 0.0f;

    if (m_staticInfo->availableApertureValues == NULL) {
        aperture_num = exifInfo->fnumber.num;
    } else {
        aperture_f = ((float) shot->dm.lens.aperture / 100);
        aperture_num = (uint32_t)(round(aperture_f * COMMON_DENOMINATOR));
        exifInfo->fnumber.num = aperture_num;
    }

    exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double) (aperture_num) / (double) (exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
    exifInfo->aperture.den = COMMON_DENOMINATOR;

    /* Brightness */
    int temp = shot->udm.internal.vendorSpecific[2];
    if ((int) shot->udm.ae.vendorSpecific[102] < 0)
        temp = -temp;
    exifInfo->brightness.num = (int32_t) (ROUND_OFF_HALF((double)((temp * EXIF_DEF_APEX_DEN)/256.f), 0));
    if ((int) shot->udm.ae.vendorSpecific[102] < 0)
        exifInfo->brightness.num = -exifInfo->brightness.num;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;

    CLOGD(" udm->internal.vendorSpecific2[100](%d)", shot->udm.internal.vendorSpecific[0]);
    CLOGD(" udm->internal.vendorSpecific2[101](%d)", shot->udm.internal.vendorSpecific[1]);
    CLOGD(" udm->internal.vendorSpecific2[102](%d)", shot->udm.internal.vendorSpecific[2]);
    CLOGD(" udm->internal.vendorSpecific2[103](%d)", shot->udm.internal.vendorSpecific[3]);

    CLOGD(" iso_speed_rating(%d)", exifInfo->iso_speed_rating);
    CLOGD(" exposure_time(%d/%d)", exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    CLOGD(" shutter_speed(%d/%d)", exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    CLOGD(" aperture     (%d/%d)", exifInfo->aperture.num, exifInfo->aperture.den);
    CLOGD(" brightness   (%d/%d)", exifInfo->brightness.num, exifInfo->brightness.den);

    /* Exposure Bias */
    exifInfo->exposure_bias.num = shot->ctl.aa.aeExpCompensation * (m_staticInfo->exposureCompensationStep * 10);
    exifInfo->exposure_bias.den = 10;

    /* Metering Mode */
    {
        switch (shot->ctl.aa.aeMode) {
        case AA_AEMODE_CENTER:
                exifInfo->metering_mode = EXIF_METERING_CENTER;
                break;
        case AA_AEMODE_MATRIX:
                exifInfo->metering_mode = EXIF_METERING_AVERAGE;
                break;
        case AA_AEMODE_SPOT:
                exifInfo->metering_mode = EXIF_METERING_SPOT;
                break;
            default:
                exifInfo->metering_mode = EXIF_METERING_AVERAGE;
                break;
        }
    }

    /* Flash Mode */
   if (shot->uctl.flashMode == CAMERA_FLASH_MODE_TORCH) {
        exifInfo->flash = 1;
    } else {
        short val_short = 0;
        int flash_mode_exif, flash_fired;
        if (shot->dm.flash.flashState == FLASH_STATE_FIRED) {
            flash_fired = 1;
        } else {
            flash_fired = 0;
        }

        switch(shot->uctl.flashMode) {
        case  CAMERA_FLASH_MODE_OFF:
          flash_mode_exif = 0x2;
          break;
        case CAMERA_FLASH_MODE_ON:
          flash_mode_exif = 0x1;
          break;
        case CAMERA_FLASH_MODE_AUTO:
          flash_mode_exif = 0x3;
          break;
        default:
          flash_mode_exif = 0x3;
          CLOGE("Unsupported flash mode");
        }
        val_short = (short)(flash_fired | (flash_mode_exif << 3));
        exifInfo->flash = val_short;
    }

    /* Custom Rendered */
    /*
     * Only 0 and 1 are standard EXIF, but other values are used by Vendor.
     * 0 = Normal
     * 1 = Custom
     * 3 = HDR
     * 6 = Panorama
     * 8 = Portrait
     */
    exifInfo->custom_rendered = 0;

    /* White Balance */
    if (shot->ctl.aa.awbMode == AA_AWBMODE_WB_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    /* Digital Zoom Ratio */
    float zoomRatio = shot->udm.zoomRatio;
    exifInfo->digital_zoom_ratio.num = (uint32_t)(shot->udm.zoomRatio * 100);
    exifInfo->digital_zoom_ratio.den = 100;

    /* Focal Length in 35mm length */
    exifInfo->focal_length_in_35mm_length = getFocalLengthIn35mmFilm();

    /* Scene Capture Type */
    switch (shot->ctl.aa.sceneMode) {
        case AA_SCENE_MODE_PORTRAIT:
        case AA_SCENE_MODE_FACE_PRIORITY:
            exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
            break;
        case AA_SCENE_MODE_LANDSCAPE:
            exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
            break;
        case AA_SCENE_MODE_NIGHT:
            exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
            break;
        default:
            exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
            break;
    }

    /* Contrast */
    const int maxContrast = 5;
    const int stepConstrast = (maxContrast / 3);

    if (shot->dm.color.vendor_contrast < stepConstrast) {
        exifInfo->contrast = 1; // Soft
    } else if ((maxContrast - stepConstrast) < shot->dm.color.vendor_contrast) {
        exifInfo->contrast = 2; // Hard
    } else {
        exifInfo->contrast = 0; // Normal
    }

    /* Saturation */
    const int maxSaturation = 5;
    const int stepSaturation = (maxSaturation / 3);

    if (shot->dm.color.vendor_saturation < stepSaturation) {
        exifInfo->saturation = 1; // Soft
    } else if ((maxSaturation - stepSaturation) < shot->dm.color.vendor_saturation) {
        exifInfo->saturation = 2; // Hard
    } else {
        exifInfo->saturation = 0; // Normal
    }


    /* Sharpness */
    const int maxSharpness = 10;
    const int stepSharpness = (maxSharpness / 3);

    if (shot->dm.edge.strength < stepSharpness) {
        exifInfo->sharpness = 1; // Soft
    } else if ((maxSaturation - stepSharpness) < shot->dm.edge.strength) {
        exifInfo->sharpness = 2; // Hard
    } else {
        exifInfo->sharpness = 0; // Normal
    }

#if 0 // this can repeatly read file. very weird.
    /* Image Unique ID */
#if defined(SENSOR_FW_GET_FROM_FILE)
    char *front_fw = NULL;
    char *rear2_fw = NULL;
    char *savePtr;

    if (getCameraId() == CAMERA_ID_BACK){
        memset(exifInfo->unique_id, 0, sizeof(exifInfo->unique_id));
        strncpy((char *)exifInfo->unique_id,
                getSensorFWFromFile(m_staticInfo, m_cameraId), sizeof(exifInfo->unique_id) - 1);
    } else if (getCameraId() == CAMERA_ID_FRONT) {
        front_fw = strtok_r((char *)getSensorFWFromFile(m_staticInfo, m_cameraId), " ", &savePtr);
        strcpy((char *)exifInfo->unique_id, front_fw);
    }
#ifdef USE_DUAL_CAMERA
    else if (getCameraId() == CAMERA_ID_BACK_2) {
        rear2_fw = strtok_r((char *)getSensorFWFromFile(m_staticInfo, m_cameraId), " ", &savePtr);
        strcpy((char *)exifInfo->unique_id, rear2_fw);
    }
#endif /* USE_DUAL_CAMERA */
#endif
#endif

    /* GPS Coordinates */
    double gpsLatitude = shot->ctl.jpeg.gpsCoordinates[0];
    double gpsLongitude = shot->ctl.jpeg.gpsCoordinates[1];
    double gpsAltitude = shot->ctl.jpeg.gpsCoordinates[2];
    if (gpsLatitude != 0 && gpsLongitude != 0) {
        if (gpsLatitude > 0)
            strncpy((char *) exifInfo->gps_latitude_ref, "N", 2);
        else
            strncpy((char *) exifInfo->gps_latitude_ref, "S", 2);

        if (gpsLongitude > 0)
            strncpy((char *) exifInfo->gps_longitude_ref, "E", 2);
        else
            strncpy((char *) exifInfo->gps_longitude_ref, "W", 2);

        if (gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        gpsLatitude = fabs(gpsLatitude);
        gpsLongitude = fabs(gpsLongitude);
        gpsAltitude = fabs(gpsAltitude);

        exifInfo->gps_latitude[0].num = (uint32_t) gpsLatitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((gpsLatitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)(round((((gpsLatitude - exifInfo->gps_latitude[0].num) * 60)
                        - exifInfo->gps_latitude[1].num) * 60));
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)gpsLongitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((gpsLongitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)(round((((gpsLongitude - exifInfo->gps_longitude[0].num) * 60)
                        - exifInfo->gps_longitude[1].num) * 60));
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)gpsAltitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        long gpsTimestamp = (long) shot->ctl.jpeg.gpsTimestamp;
        gmtime_r(&gpsTimestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        if (strlen((char *)shot->ctl.jpeg.gpsProcessingMethod) > 0) {
            size_t len = GPS_PROCESSING_METHOD_SIZE;
            memset(exifInfo->gps_processing_method, 0, sizeof(exifInfo->gps_processing_method));

            if (len > sizeof(exifInfo->gps_processing_method)) {
                len = sizeof(exifInfo->gps_processing_method);
            }
            strncpy((char *)exifInfo->gps_processing_method, (char *)shot->ctl.jpeg.gpsProcessingMethod, len);
        }

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    /* Thumbnail Size */
    exifInfo->widthThumb = thumbnailRect->w;
    exifInfo->heightThumb = thumbnailRect->h;

    /* Makers Note */
    m_setExifChangedMakersNote(exifInfo, shot_ext);
}

void ExynosCameraParameters::m_setExifChangedMakersNote(exif_attribute_t *exifInfo,
                                                        struct camera2_shot_ext *shot_ext)
{
    status_t ret = NO_ERROR;

#if defined(SENSOR_FW_GET_FROM_FILE)
    ExynosCameraEEPRomMap *eepromMap = NULL;
    ExynosCameraMakersNote *makersNote = NULL;
    ExynosCameraMakersNote::Args args;
    ////////////////////////////////////////////////
    // get eepromMap
    eepromMap = this->getEEPRomMap();
    if (eepromMap == NULL) {
        goto done;
    }

    if (eepromMap->flagCreated() == false) {
        CLOGE("eepromMap->flagCreated() == false. so, fail. you cannot get the exact eeprom value");
        goto done;
    }

    ////////////////////////////////////////////////
    // get makersNote
    makersNote = this->getMakersNote();
    if (makersNote == NULL) {
        CLOGE("this->getMakersNote() fail");
        goto done;
    }

    ////////////////////////////////////////////////
    // makersNote will fill exifInfo->maker_note, using eepromMap, and meta
    args.eepromMap  = eepromMap;
    args.shot_ext   = shot_ext;
    args.sensorInfo = this->getSensorStaticInfo();
    args.buf        = (char *)exifInfo->maker_note;
    args.bufSize    = exifInfo->maker_note_size;

    ret = makersNote->fillMakersNote(&args);
    if (ret != NO_ERROR) {
        CLOGE("makersNote->fillMakersNote() fail");
        goto done;
    }

    ////////////////////////////////////////////////
done:
#endif //SENSOR_FW_GET_FROM_FILE

    return;
}

void ExynosCameraParameters::m_getVendorUsePureBayerReprocessing(bool &usePureBayerReprocessing)
{
#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (m_configurations->getMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE)) {
        usePureBayerReprocessing = getUsePureBayerRemosaicReprocessing();
    } else
#endif
#ifdef SUPPORT_SESSION_PARAMETERS
    if (m_configurations->getModeMultiValue(CONFIGURATION_MULTI_SESSION_MODE_VALUE, EXYNOS_SESSION_MODE_PRO) == true) {
        usePureBayerReprocessing = true;
    } else
#endif
#ifdef USES_COMBINE_PLUGIN
    if (m_configurations->getModeValue(CONFIGURATION_COMBINE_PREVIEW_PLUGIN_VALUE) > 0) {
        usePureBayerReprocessing = false;
    } else
#endif
    {
        CLOGV("Don't change to usePureBayerReprocessing(%d)", usePureBayerReprocessing);
    }
}

int ExynosCameraParameters::getSensorControlDelay()
{
    int sensorRequestDelay = 0;
#ifdef SENSOR_REQUEST_DELAY
    if (m_configurations->getMode(CONFIGURATION_VISION_MODE) == true) {
        sensorRequestDelay = 0;
    } else {
        sensorRequestDelay = SENSOR_REQUEST_DELAY;
    }
#else
    android_printAssert(NULL, LOG_TAG, "SENSOR_REQUEST_DELAY is NOT defined.");
#endif
    CLOGV(" sensorRequestDelay %d", sensorRequestDelay);

    return sensorRequestDelay;
}

status_t ExynosCameraParameters::duplicateCtrlMetadata(void *buf)
{
    if (buf == NULL) {
        CLOGE("ERR: buf is NULL");
        return BAD_VALUE;
    }

    struct camera2_shot_ext *meta_shot_ext = (struct camera2_shot_ext *)buf;
    memcpy(&meta_shot_ext->shot.ctl, &m_metadata.shot.ctl, sizeof(struct camera2_ctl));

    setMetaVtMode(meta_shot_ext, (enum camera_vt_mode)m_configurations->getModeValue(CONFIGURATION_VT_MODE));

#ifdef SUPPORT_DEPTH_MAP
    meta_shot_ext->shot.uctl.isModeUd.disparity_mode = m_metadata.shot.uctl.isModeUd.disparity_mode;
#endif

    return NO_ERROR;
}

void ExynosCameraParameters::setActiveZoomRatio(float zoomRatio)
{
    m_activeZoomRatio = zoomRatio;
}

float ExynosCameraParameters::getActiveZoomRatio(void)
{
    return m_activeZoomRatio;
}

void ExynosCameraParameters::setActiveZoomRect(ExynosRect zoomRect)
{
    m_activeZoomRect.x = zoomRect.x;
    m_activeZoomRect.y = zoomRect.y;
    m_activeZoomRect.w = zoomRect.w;
    m_activeZoomRect.h = zoomRect.h;
}

void ExynosCameraParameters::getActiveZoomRect(ExynosRect *zoomRect)
{
    zoomRect->x = m_activeZoomRect.x;
    zoomRect->y = m_activeZoomRect.y;
    zoomRect->w = m_activeZoomRect.w;
    zoomRect->h = m_activeZoomRect.h;
}

void ExynosCameraParameters::setActiveZoomMargin(int zoomMargin)
{
    m_activeZoomMargin = zoomMargin;
}

int ExynosCameraParameters::getActiveZoomMargin(void)
{
    return m_activeZoomMargin;
}

void ExynosCameraParameters::setPictureActiveZoomRatio(float zoomRatio)
{
    m_activePictureZoomRatio = zoomRatio;
}

float ExynosCameraParameters::getPictureActiveZoomRatio(void)
{
    return m_activePictureZoomRatio;
}

void ExynosCameraParameters::setPictureActiveZoomRect(ExynosRect zoomRect)
{
    m_activePictureZoomRect.x = zoomRect.x;
    m_activePictureZoomRect.y = zoomRect.y;
    m_activePictureZoomRect.w = zoomRect.w;
    m_activePictureZoomRect.h = zoomRect.h;
}

void ExynosCameraParameters::getPictureActiveZoomRect(ExynosRect *zoomRect)
{
    zoomRect->x = m_activePictureZoomRect.x;
    zoomRect->y = m_activePictureZoomRect.y;
    zoomRect->w = m_activePictureZoomRect.w;
    zoomRect->h = m_activePictureZoomRect.h;
}

void ExynosCameraParameters::setPictureActiveZoomMargin(int zoomMargin)
{
    m_activePictureZoomMargin = zoomMargin;
}

int ExynosCameraParameters::getPictureActiveZoomMargin(void)
{
    return m_activePictureZoomMargin;
}


void ExynosCameraParameters::updatePreviewStatRoi(struct camera2_shot_ext *shot_ext, ExynosRect *bCropRect)
{
#ifdef SUPPORT_DISPLAY_REGION
    shot_ext->shot.uctl.statsRoi[0] = bCropRect->x;
    shot_ext->shot.uctl.statsRoi[1] = bCropRect->y;
    shot_ext->shot.uctl.statsRoi[2] = bCropRect->w;
    shot_ext->shot.uctl.statsRoi[3] = bCropRect->h;
#else
    float zoomRatio = m_configurations->getZoomRatio();
    float activeZoomRatio = getActiveZoomRatio();
    float statRoiZoomRatio = 1.0f;
    ExynosRect statRoi = {0, };

    if (m_scenario == SCENARIO_NORMAL
        || zoomRatio == activeZoomRatio) {
        shot_ext->shot.uctl.statsRoi[0] = bCropRect->x;
        shot_ext->shot.uctl.statsRoi[1] = bCropRect->y;
        shot_ext->shot.uctl.statsRoi[2] = bCropRect->w;
        shot_ext->shot.uctl.statsRoi[3] = bCropRect->h;
        return;
    }

    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        if (m_cameraId == CAMERA_ID_BACK_2) {
            /* for Tele View Angle */
            statRoiZoomRatio = (zoomRatio/2 < 1.0f) ? 1.0f : zoomRatio/2;
        } else {
            statRoiZoomRatio = zoomRatio;
        }
    } else {
        statRoiZoomRatio = zoomRatio;
    }

    statRoi.w = (int) ceil((float)(bCropRect->w * activeZoomRatio) / (float)statRoiZoomRatio);
    statRoi.h = (int) ceil((float)(bCropRect->h * activeZoomRatio) / (float)statRoiZoomRatio);
    statRoi.x = ALIGN_DOWN(((bCropRect->w - statRoi.w) >> 1), 2);
    statRoi.y = ALIGN_DOWN(((bCropRect->h - statRoi.h) >> 1), 2);

    CLOGV2("CameraId(%d), zoomRatio(%f->%f), bnsSize(%d,%d,%d,%d), statRoi(%d,%d,%d,%d)",
        m_cameraId,
        zoomRatio,
        statRoiZoomRatio,
        bCropRect->x,
        bCropRect->y,
        bCropRect->w,
        bCropRect->h,
        statRoi.x,
        statRoi.y,
        statRoi.w,
        statRoi.h);

    shot_ext->shot.uctl.statsRoi[0] = statRoi.x;
    shot_ext->shot.uctl.statsRoi[1] = statRoi.y;
    shot_ext->shot.uctl.statsRoi[2] = statRoi.w;
    shot_ext->shot.uctl.statsRoi[3] = statRoi.h;
#endif
}

void ExynosCameraParameters::updateDisplayStatRoi(ExynosCameraFrameSP_sptr_t frame, struct camera2_shot_ext *shot_ext)
{
    int32_t dispCameraId = m_cameraId;
    ExynosRect dispRect;

    getDisplayStatRoi(dispCameraId, dispRect);
#ifndef USE_CAMERA_EXYNOS850_META
#ifndef USE_CAMERA_EXYNOS3830_META
    if (checkValidateRect(dispRect) == false) {
        // Not updated. Use statsRoi by default.
        shot_ext->shot.uctl.displayRegion[0] = shot_ext->shot.uctl.statsRoi[0];
        shot_ext->shot.uctl.displayRegion[1] = shot_ext->shot.uctl.statsRoi[1];
        shot_ext->shot.uctl.displayRegion[2] = shot_ext->shot.uctl.statsRoi[2];
        shot_ext->shot.uctl.displayRegion[3] = shot_ext->shot.uctl.statsRoi[3];
    } else {
        // updated
        shot_ext->shot.uctl.displayRegion[0] = dispRect.x;
        shot_ext->shot.uctl.displayRegion[1] = dispRect.y;
        shot_ext->shot.uctl.displayRegion[2] = dispRect.w;
        shot_ext->shot.uctl.displayRegion[3] = dispRect.h;
    }
#endif
#endif
}

status_t ExynosCameraParameters::m_vendorReInit(void)
{
    m_depthMapW = 0;
    m_depthMapH = 0;
    m_alternativePreviewPortId = -1;
#ifdef LLS_CAPTURE
    m_LLSValue = 0;
    memset(m_needLLS_history, 0x00, sizeof(int) * LLS_HISTORY_COUNT);
#endif
    setBayerFrameLockCount(0, 0);

    return NO_ERROR;
}

#ifdef USES_SENSOR_LISTENER
void ExynosCameraParameters::setGyroData(ExynosCameraSensorListener::Event_t data)
{
    setMetaCtlGyro(&m_metadata, data.gyro.x, data.gyro.y, data.gyro.z);
    memcpy(&m_gyroListenerData, &data, sizeof(ExynosCameraSensorListener::Event_t));

#ifdef USE_GYRO_HISTORY_FOR_TNR
    setMetaCtlGyroHistory(&m_metadata, data.gyro.x, data.gyro.y, data.gyro.z, (data.timestamp / 1000) /* convert ns to us */);
#endif
}

ExynosCameraSensorListener::Event_t *ExynosCameraParameters::getGyroData(void)
{
    return &m_gyroListenerData;
}

void ExynosCameraParameters::setAccelerometerData(ExynosCameraSensorListener::Event_t data)
{
    setMetaCtlAccelerometer(&m_metadata, data.accel.x, data.accel.y, data.accel.z);
    memcpy(&m_accelerometerListenerData, &data, sizeof(ExynosCameraSensorListener::Event_t));
}

ExynosCameraSensorListener::Event_t *ExynosCameraParameters::getAccelerometerData(void)
{
    return &m_accelerometerListenerData;
}
#endif

void ExynosCameraParameters::getVendorRatioCropSizeForVRA(__unused ExynosRect *ratioCropSize)
{
}

void ExynosCameraParameters::getVendorRatioCropSize(ExynosRect *ratioCropSize, ExynosRect *mcscSize, int portIndex, int isReprocessing)
{
    status_t ret = NO_ERROR;


    if (isReprocessing == true) {
#ifdef SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO
        if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
            /* prevent mcsc scaleup ratio with highspeed scenario. */
            if (ratioCropSize->w * SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO < mcscSize->w
                || ratioCropSize->h * SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO < mcscSize->h) {
                ratioCropSize->w = ALIGN_UP((int)ceil((float)mcscSize->w / SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO), CAMERA_MCSC_ALIGN);
                ratioCropSize->h = ALIGN_UP((int)ceil((float)mcscSize->h / SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO), CAMERA_MCSC_ALIGN);
            }
        }
#endif
        return;
    }

#ifdef USES_SW_VDIS
    ExynosRect ratioCropResult;

    float cropRatio = 1.2f;

    if (ratioCropSize == NULL || mcscSize == NULL) {
        CLOGE("Invalid parameters");
        return;
    }

    if (m_configurations->getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE) > 0) {
        if (getPreviewPortId() == portIndex) {

            uint32_t previewW = 0, previewH = 0, videoW = 0, videoH = 0;
            getSize(HW_INFO_HW_YUV_SIZE, &previewW, &previewH, getPreviewPortId());
            getSize(HW_INFO_HW_YUV_SIZE, &videoW, &videoH, getRecordingPortId());
            if( videoW > previewH) {
                cropRatio = (float)videoW / (float)previewW;
                CLOGV("crop preview ratio(%f) previewW(%d), videoW(%d)", cropRatio, previewW, videoW);
            } else {
                CLOGW("Do not need to crop preview");
                return;
            }
            ret = getCropRectAlign(ratioCropSize->w, ratioCropSize->h, mcscSize->w, mcscSize->h,
                        &ratioCropResult.x, &ratioCropResult.y, &ratioCropResult.w, &ratioCropResult.h,
                        CAMERA_MCSC_ALIGN, 2, cropRatio);
            if (ret != NO_ERROR) {
                ratioCropSize->x = mcscSize->x;
                ratioCropSize->y = mcscSize->y;
                ratioCropSize->w = mcscSize->w;
                ratioCropSize->h = mcscSize->h;
            } else {
                ratioCropSize->x += ratioCropResult.x;
                ratioCropSize->y += ratioCropResult.y;
                ratioCropSize->w = ratioCropResult.w;
                ratioCropSize->h = ratioCropResult.h;
            }
        }
    }
#endif
}

void ExynosCameraParameters::setImageUniqueId(char *uniqueId)
{
    memcpy(m_cameraInfo.imageUniqueId, uniqueId, sizeof(m_cameraInfo.imageUniqueId));
}


#ifdef USE_DUAL_CAMERA
status_t ExynosCameraParameters::adjustDualSolutionSize(int targetWidth, int targetHeight)
{
   // TODO: add dual solution LUT to handle specific scenario
   return NO_ERROR;
}

void ExynosCameraParameters::getDualSolutionSize(int *srcW, int *srcH,
                                                 int *dstW, int *dstH,
                                                 int orgW, int orgH,
                                                 int margin)
{
   // TODO: add dual solution LUT to handle specific scenario
    return;
}

status_t ExynosCameraParameters::getFusionSize(int w, int h, ExynosRect *srcRect, ExynosRect *dstRect, int margin)
{
    status_t ret = NO_ERROR;

    ret = m_getFusionSize(w, h, srcRect, true, margin);
    if (ret != NO_ERROR) {
        CLOGE("m_getFusionSize(%d, %d, true) fail", w, h);
        return ret;
    }

    ret = m_getFusionSize(w, h, dstRect, false, margin);
    if (ret != NO_ERROR) {
        CLOGE("m_getFusionSize(%d, %d, false) fail", w, h);
        return ret;
    }

    return ret;
}

status_t ExynosCameraParameters::m_getFusionSize(int w, int h, ExynosRect *rect, bool flagSrc, int margin)
{
    status_t ret = NO_ERROR;

    if (w <= 0 || h <= 0) {
        CLOGE("w(%d) <= 0 || h(%d) <= 0", w, h);
        return INVALID_OPERATION;
    }

    rect->x = 0;
    rect->y = 0;

    rect->w = w;
    rect->h = h;

    rect->fullW = rect->w;
    rect->fullH = rect->h;

    rect->colorFormat = getHwPreviewFormat();

    // TODO: Need to manage fusion size by scenario
    if (margin == DUAL_SOLUTION_MARGIN_VALUE_NONE)
        return ret;

    int srcW, srcH, dstW, dstH;
    getDualSolutionSize(&srcW, &srcH, &dstW, &dstH, w, h, margin);
    if (flagSrc == true) {
        rect->w = srcW;
        rect->h = srcH;
    } else {
        rect->w = dstW;
        rect->h = dstH;
    }

    rect->fullW = rect->w;
    rect->fullH = rect->h;

    return ret;
}
#endif

#ifdef LLS_CAPTURE
void ExynosCameraParameters::m_setLLSValue(struct camera2_shot_ext *shot)
{
    int lls_value = 0;

    if (shot == NULL) {
        return;
    }

    m_needLLS_history[3] = m_needLLS_history[2];
    m_needLLS_history[2] = m_needLLS_history[1];
    m_needLLS_history[1] = getLLS(shot);

    if (m_needLLS_history[1] == m_needLLS_history[2]
            && m_needLLS_history[1] == m_needLLS_history[3]) {
        lls_value = m_needLLS_history[0] = m_needLLS_history[1];
    } else {
        lls_value = m_needLLS_history[0];
    }

    if (lls_value != getLLSValue()) {
        m_LLSValue = lls_value;
        CLOGD("[%d[%d][%d][%d] LLS_value(%d)",
                m_needLLS_history[0], m_needLLS_history[1], m_needLLS_history[2], m_needLLS_history[3],
                m_LLSValue);
    }
}

int ExynosCameraParameters::getLLSValue(void)
{
    return m_LLSValue;
}
#endif

status_t ExynosCameraParameters::m_vendorConstructorInitalize(__unused int cameraId)
{
    status_t ret = NO_ERROR;

#ifdef TEST_LLS_REPROCESING
    /* Always enable LLS Capture */
    m_LLSOn = true;
    m_LLSCaptureOn = true;
#endif

    // Check debug_attribute_t struct in ExynosExif.h

#ifdef LLS_CAPTURE
    m_LLSValue = 0;
    memset(m_needLLS_history, 0x00, sizeof(int) * LLS_HISTORY_COUNT);
#endif

    memset(&m_thumbHistInfo, 0x00, sizeof(thumbnail_histogram_info_t));

    return ret;
}

void ExynosCameraParameters::m_vendorSWVdisMode(__unused bool *ret)
{
}

void ExynosCameraParameters::m_vendorSetExifChangedAttribute(__unused debug_attribute_t   &debugInfo,
                                                                          __unused unsigned int &offset,
                                                                          __unused bool &isWriteExif,
                                                                          __unused camera2_shot_ext *shot_ext,
                                                                          __unused bool useDebugInfo2)
{
}

bool ExynosCameraParameters::isAlternativePreviewPortId(int outputPortId)
{
    bool result = false;

    if (outputPortId >= YUV_0 && outputPortId < YUV_MAX
        && outputPortId == m_alternativePreviewPortId) {
        result = true;
    } else {
        result = false;
    }

    return result;
}

void ExynosCameraParameters::setAlternativePreviewPortId(int outputPortId)
{
    m_alternativePreviewPortId = outputPortId;
}

int ExynosCameraParameters::getAlternativePreviewPortId(void)
{
    return m_alternativePreviewPortId;
}

}; /* namespace android */
