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
#define LOG_TAG "ExynosCameraConfigurationsSec"
#include <log/log.h>

#include "ExynosCameraRequestManager.h"
#include "ExynosCameraConfigurations.h"
#include "ExynosCameraMetadataConverter.h"
#ifdef USE_DEBUG_PROPERTY
#include "ExynosCameraProperty.h"
#endif

namespace android {

int ExynosCameraConfigurations::m_staticValue[CONFIGURATION_STATIC_VALUE_MAX];

void ExynosCameraConfigurations::m_vendorSpecificConstructor(void)
{
    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START
    m_useFastenAeStable = false;

    m_isFactoryBin = false;

    m_bokehBlurStrength = 0;

    m_repeatingRequestHint = 0;
    m_temperature = 0xFFFF;
    m_yuvSizeRatioId = -1;
    m_pictureSizeRatioId = -1;

    m_onePortId = -1;
    m_secondPortId = -1;

    for (int i = 0; i < 2; i++) {
        m_ledCurrent[i] = m_staticInfo->ledCurrent;
    }

    m_ledCalibrationEnabled = false;
}

void ExynosCameraConfigurations::m_vendorSpecificDestructor(void)
{
    return;
}

status_t ExynosCameraConfigurations::setSize(enum CONFIGURATION_SIZE_TYPE type, uint32_t width, uint32_t height, int outputPortId)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case CONFIGURATION_YUV_SIZE:
    {
        int widthArrayNum = sizeof(m_yuvWidth)/sizeof(m_yuvWidth[0]);
        int heightArrayNum = sizeof(m_yuvHeight)/sizeof(m_yuvHeight[0]);

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

        m_yuvWidth[outputPortId] = width;
        m_yuvHeight[outputPortId] = height;
        break;
    }
    case CONFIGURATION_MIN_YUV_SIZE:
    case CONFIGURATION_MAX_YUV_SIZE:
    case CONFIGURATION_MIN_YUV_STALL_SIZE:
    case CONFIGURATION_MAX_YUV_STALL_SIZE:
    case CONFIGURATION_PREVIEW_SIZE:
    case CONFIGURATION_PREVIEW_CB_SIZE:
    case CONFIGURATION_VIDEO_SIZE:
    case CONFIGURATION_PICTURE_SIZE:
    case CONFIGURATION_THUMBNAIL_SIZE:
    case CONFIGURATION_CAPTURE_STREAM_BUFFER_SIZE:
    case CONFIGURATION_MAX_PICTURE_SIZE_OF_MULTISTREAM:
        m_width[type] = width;
        m_height[type] = height;
        break;
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraConfigurations::getSize(enum CONFIGURATION_SIZE_TYPE type, uint32_t *width, uint32_t *height, int outputPortId) const
{
    status_t ret = NO_ERROR;

    switch(type) {
    case CONFIGURATION_YUV_SIZE:
    {
        int widthArrayNum = sizeof(m_yuvWidth)/sizeof(m_yuvWidth[0]);
        int heightArrayNum = sizeof(m_yuvHeight)/sizeof(m_yuvHeight[0]);

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

        *width = m_yuvWidth[outputPortId];
        *height = m_yuvHeight[outputPortId];
        break;
    }
    //max size of multiple camera
    case CONFIGURATION_MAX_PICTURE_SIZE:
    case CONFIGURATION_MAX_SENSOR_SIZE:
    case CONFIGURATION_MAX_NOT_REMOSAIC_SENSOR_SIZE:
    case CONFIGURATION_MAX_REMOSAIC_SENSOR_SIZE:
    case CONFIGURATION_MAX_HW_YUV_SIZE:
    {
        *width = 0;
        *height = 0;

        for (int i = 0; i < m_camIdInfo->numOfSensors; i++) {
            int w = 0, h = 0;
            int index = m_camIdInfo->cameraId[i];

            switch(type) {
            case CONFIGURATION_MAX_PICTURE_SIZE:
                m_parameters[index]->getSize(HW_INFO_MAX_PICTURE_SIZE, (uint32_t *)&w, (uint32_t *)&h);
                break;
            case CONFIGURATION_MAX_SENSOR_SIZE:
                m_parameters[index]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&w, (uint32_t *)&h);
                break;
            case CONFIGURATION_MAX_NOT_REMOSAIC_SENSOR_SIZE:
                m_parameters[index]->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&w, (uint32_t *)&h);
                break;
            case CONFIGURATION_MAX_REMOSAIC_SENSOR_SIZE:
                m_parameters[index]->getSize(HW_INFO_HW_REMOSAIC_SENSOR_SIZE, (uint32_t *)&w, (uint32_t *)&h);
                break;
            case CONFIGURATION_MAX_HW_YUV_SIZE:
                m_parameters[index]->getSize(HW_INFO_MAX_HW_YUV_SIZE, (uint32_t *)&w, (uint32_t *)&h);
                break;
            default:
                break;
            }

            if ((w * h) > ((*width) * (*height))) {
                *width = w;
                *height = h;
            }
        }
        break;
    }
    case CONFIGURATION_MAX_HW_YUV_PORT_SIZE:
    {
        *width = 0;
        *height = 0;

        for (int i = 0; i < m_camIdInfo->numOfSensors; i++) {
            int w = 0, h = 0;
            int index = m_camIdInfo->cameraId[i];

            switch(type) {
            case CONFIGURATION_MAX_HW_YUV_PORT_SIZE:
                m_parameters[index]->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&w, (uint32_t *)&h, outputPortId);
                break;
            }

            if ((w * h) > ((*width) * (*height))) {
                *width = w;
                *height = h;
            }
        }
        break;
    }
    case CONFIGURATION_MIN_YUV_SIZE:
    case CONFIGURATION_MAX_YUV_SIZE:
    case CONFIGURATION_MIN_YUV_STALL_SIZE:
    case CONFIGURATION_MAX_YUV_STALL_SIZE:
    case CONFIGURATION_PREVIEW_SIZE:
    case CONFIGURATION_PREVIEW_CB_SIZE:
    case CONFIGURATION_VIDEO_SIZE:
    case CONFIGURATION_PICTURE_SIZE:
    case CONFIGURATION_THUMBNAIL_SIZE:
    case CONFIGURATION_CAPTURE_STREAM_BUFFER_SIZE:
    case CONFIGURATION_MAX_PICTURE_SIZE_OF_MULTISTREAM:
        *width = m_width[type];
        *height = m_height[type];
        break;
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraConfigurations::resetSize(enum CONFIGURATION_SIZE_TYPE type)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case CONFIGURATION_YUV_SIZE:
        memset(m_yuvWidth, 0, sizeof(m_yuvWidth));
        memset(m_yuvHeight, 0, sizeof(m_yuvHeight));
        break;
    case CONFIGURATION_MIN_YUV_SIZE:
    case CONFIGURATION_MAX_YUV_SIZE:
    case CONFIGURATION_MIN_YUV_STALL_SIZE:
    case CONFIGURATION_MAX_YUV_STALL_SIZE:
    case CONFIGURATION_PREVIEW_SIZE:
    case CONFIGURATION_VIDEO_SIZE:
    case CONFIGURATION_PICTURE_SIZE:
        m_width[type] = 0;
        m_height[type] = 0;
        break;
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraConfigurations::setMode(enum CONFIGURATION_MODE_TYPE type, bool enable)
{
    status_t ret = NO_ERROR;

    switch(type) {
#ifdef USE_DUAL_CAMERA
    case CONFIGURATION_DUAL_MODE:
    case CONFIGURATION_DUAL_PRE_MODE:
    case CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE:
    case CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE:
        m_flagCheck[type] = true;
        m_mode[type] = enable;
        break;
#endif
    case CONFIGURATION_DYNAMIC_BAYER_MODE:
        m_mode[type] = enable;
        m_useDynamicBayer[CONFIG_MODE::NORMAL] = enable;
        break;
    case CONFIGURATION_SESSION_MODE:
    case CONFIGURATION_ZERO_CAMERA_MODE:
    case CONFIGURATION_PIP_MODE:
    case CONFIGURATION_PIP_SUB_CAM_MODE:
    case CONFIGURATION_RECORDING_MODE:
        m_flagCheck[type] = true;
    case CONFIGURATION_OIS_CAPTURE_MODE:
    case CONFIGURATION_FULL_SIZE_LUT_MODE:
    case CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE:
    case CONFIGURATION_ODC_MODE:
    case CONFIGURATION_VISION_MODE:
    case CONFIGURATION_PIP_RECORDING_MODE:
    case CONFIGURATION_VIDEO_STABILIZATION_MODE:
    case CONFIGURATION_LONGEXPOSURE_CAPTURE_MODE:
    case CONFIGURATION_RESTART_FORCE_FLUSH:
    case CONFIGURATION_HDR_MODE:
    case CONFIGURATION_HDR_RECORDING_MODE:
    case CONFIGURATION_DVFS_LOCK_MODE:
    case CONFIGURATION_MANUAL_AE_CONTROL_MODE:
    case CONFIGURATION_HIFI_LLS_MODE:
    case CONFIGURATION_HIFI_MODE:
    case CONFIGURATION_REMOSAIC_CAPTURE_MODE:
    case CONFIGURATION_REMOSAIC_CAPTURE_ON:
    case CONFIGURATION_DYNAMIC_REMOSAIC_BUFFER_ALLOC_MODE:
    case CONFIGURATION_BEST_SHOT_MODE:
#ifdef SUPPORT_DEPTH_MAP
    case CONFIGURATION_DEPTH_MAP_MODE:
#endif
    case CONFIGURATION_COMPRESSION_PREVIEW_STREAM:
    case CONFIGURATION_COMPRESSION_VIDEO_STREAM:
    case CONFIGURATION_VIDEO_MODE:
        m_mode[type] = enable;
        break;
    case CONFIGURATION_YSUM_RECORDING_MODE:
#ifdef USE_YSUM_RECORDING
        m_mode[type] = enable;

        if (getMode(CONFIGURATION_PIP_RECORDING_MODE) == true)
#endif
        {
            m_mode[type] = false;
        }
        CLOGD("YSUM Recording %s", enable == true ? "ON" : "OFF");
        break;
    case CONFIGURATION_GYRO_MODE:
    case CONFIGURATION_ACCELEROMETER_MODE:
    case CONFIGURATION_ROTATION_MODE:
#ifdef USE_DUAL_CAMERA
    case CONFIGURATION_FUSION_CAPTURE_MODE:
#endif
#ifdef LLS_CAPTURE
    case CONFIGURATION_LIGHT_CONDITION_ENABLE_MODE:
#endif
    case CONFIGURATION_VENDOR_YUV_STALL:
    case CONFIGURATION_ALWAYS_FD_ON_MODE:
    case CONFIGURATION_NIGHT_SHOT_BAYER_MODE:
    case CONFIGURATION_NIGHT_SHOT_YUV_MODE:
    case CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE:
    case CONFIGURATION_HDR_BAYER_MODE:
    case CONFIGURATION_HDR_YUV_MODE:
    case CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE:
    case CONFIGURATION_BEAUTY_FACE_YUV_MODE:
    case CONFIGURATION_SUPER_RESOLUTION_MODE:
    case CONFIGURATION_OIS_DENOISE_YUV_MODE:
    case CONFIGURATION_SPORTS_YUV_MODE:
    case CONFIGURATION_SUPER_EIS_MODE:
    case CONFIGURATION_CLAHE_CAPTURE_MODE:
    case CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE:
        m_mode[type] = enable;
        break;
    default:
        break;
    }

    return ret;
}

bool ExynosCameraConfigurations::getMode(enum CONFIGURATION_MODE_TYPE type) const
{
    bool modeEnable = false;

    switch(type) {
    case CONFIGURATION_DYNAMIC_BAYER_MODE:
    {
        int32_t configMode = getConfigMode();
        if (configMode >= 0)
            modeEnable = m_useDynamicBayer[configMode];
        else {
            CLOGE(" invalid config mode. couldn't get Dynamic Bayer mode");
            return false;
        }
        break;
    }
    case CONFIGURATION_SESSION_MODE:
    case CONFIGURATION_ZERO_CAMERA_MODE:
    case CONFIGURATION_PIP_MODE:
    case CONFIGURATION_PIP_SUB_CAM_MODE:
    case CONFIGURATION_RECORDING_MODE:
#ifdef USE_DUAL_CAMERA
    case CONFIGURATION_DUAL_MODE:
    case CONFIGURATION_DUAL_PRE_MODE:
    case CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE:
#endif
        if (m_flagCheck[type] == false) {
            return false;
        }
#ifdef USE_DUAL_CAMERA
    case CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE:
        if (m_flagCheck[type] == false) {
            return false;
        }
#endif
    case CONFIGURATION_OIS_CAPTURE_MODE:
    case CONFIGURATION_FULL_SIZE_LUT_MODE:
    case CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE:
    case CONFIGURATION_ODC_MODE:
    case CONFIGURATION_VISION_MODE:
    case CONFIGURATION_PIP_RECORDING_MODE:
    case CONFIGURATION_YSUM_RECORDING_MODE:
    case CONFIGURATION_VIDEO_STABILIZATION_MODE:
    case CONFIGURATION_LONGEXPOSURE_CAPTURE_MODE:
    case CONFIGURATION_RESTART_FORCE_FLUSH:
    case CONFIGURATION_HDR_MODE:
    case CONFIGURATION_HDR_RECORDING_MODE:
    case CONFIGURATION_DVFS_LOCK_MODE:
    case CONFIGURATION_MANUAL_AE_CONTROL_MODE:
    case CONFIGURATION_HIFI_LLS_MODE:
    case CONFIGURATION_HIFI_MODE:
    case CONFIGURATION_COMPRESSION_PREVIEW_STREAM:
    case CONFIGURATION_COMPRESSION_VIDEO_STREAM:
    case CONFIGURATION_REMOSAIC_CAPTURE_MODE:
    case CONFIGURATION_REMOSAIC_CAPTURE_ON:
    case CONFIGURATION_DYNAMIC_REMOSAIC_BUFFER_ALLOC_MODE:
    case CONFIGURATION_BEST_SHOT_MODE:
#ifdef SUPPORT_DEPTH_MAP
    case CONFIGURATION_DEPTH_MAP_MODE:
#endif
    case CONFIGURATION_VIDEO_MODE:
        modeEnable = m_mode[type];
        break;
    case CONFIGURATION_SENSOR_GYRO_MODE:
    {
        if (m_staticInfo->sensorGyroSupport == true &&
            getOisTestReqired(CONFIGURATION_OIS_TEST_GEA) == true) {
            modeEnable = true;
        }
        break;
    }
    case CONFIGURATION_GMV_MODE:
        modeEnable = m_getGmvMode();
        break;
    case CONFIGURATION_GYRO_MODE:
    case CONFIGURATION_ACCELEROMETER_MODE:
    case CONFIGURATION_ROTATION_MODE:
#ifdef USE_DUAL_CAMERA
    case CONFIGURATION_FUSION_CAPTURE_MODE:
#endif
#ifdef LLS_CAPTURE
    case CONFIGURATION_LIGHT_CONDITION_ENABLE_MODE:
#ifdef SET_LLS_CAPTURE_SETFILE
    case CONFIGURATION_LLS_CAPTURE_MODE:
#endif
#endif
    case CONFIGURATION_VENDOR_YUV_STALL:
    case CONFIGURATION_ALWAYS_FD_ON_MODE:
    case CONFIGURATION_NIGHT_SHOT_BAYER_MODE:
    case CONFIGURATION_NIGHT_SHOT_YUV_MODE:
    case CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE:
    case CONFIGURATION_HDR_BAYER_MODE:
    case CONFIGURATION_HDR_YUV_MODE:
    case CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE:
    case CONFIGURATION_BEAUTY_FACE_YUV_MODE:
    case CONFIGURATION_SUPER_RESOLUTION_MODE:
    case CONFIGURATION_OIS_DENOISE_YUV_MODE:
    case CONFIGURATION_SPORTS_YUV_MODE:
    case CONFIGURATION_SUPER_EIS_MODE:
    case CONFIGURATION_CLAHE_CAPTURE_MODE:
    case CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE:
        modeEnable = m_mode[type];
        break;
    default:
        break;
    }

    return modeEnable;
}

bool ExynosCameraConfigurations::getDynamicMode(enum DYNAMIC_MODE_TYPE type)
{
    bool modeEnable = false;

    switch(type) {
    case DYNAMIC_UHD_RECORDING_MODE:
    {
        uint32_t videoW = 0, videoH = 0;

        getSize(CONFIGURATION_VIDEO_SIZE, &videoW, &videoH);

        if (getMode(CONFIGURATION_RECORDING_MODE) == true
            && ((videoW == 3840 && videoH == 2160)
                || (videoW == 2560 && videoH == 1440))) {
            modeEnable = true;
        }
        break;
    }
    case DYNAMIC_HIGHSPEED_RECORDING_MODE:
    {
        uint32_t configMode = (uint32_t)getConfigMode();
        switch(configMode){
        case CONFIG_MODE::HIGHSPEED_60:
        case CONFIG_MODE::HIGHSPEED_120:
        case CONFIG_MODE::HIGHSPEED_240:
        case CONFIG_MODE::HIGHSPEED_480:
            modeEnable = true;
            break;
        case CONFIG_MODE::NORMAL:
        default:
            modeEnable = false;
            break;
        }
        break;
    }
#ifdef USE_DUAL_CAMERA
    case DYNAMIC_DUAL_FORCE_SWITCHING:
    {
#if 0
        // TODO: Set "forcely switching scenario" due to H/W or S/W constraint
        uint32_t videoW = 0, videoH = 0;

        getSize(CONFIGURATION_VIDEO_SIZE, &videoW, &videoH);

        if (getMode(CONFIGURATION_RECORDING_MODE) == true
            && (videoW == 3840 && videoH == 2160)) {
            modeEnable = true;
        }
#endif
        break;
    }
    case DYNAMIC_DUAL_CAMERA_DISABLE:
    {
#if 0
        // TODO: Set "forcely fixing single camera" due to H/W or S/W constraint
#endif
        break;
    }
#endif
    default:
        break;
    }

    return modeEnable;
}

status_t ExynosCameraConfigurations::setStaticValue(enum CONFIGURATON_STATIC_VALUE_TYPE type, __unused int value)
{
    status_t ret = NO_ERROR;

    switch(type) {
    default:
        break;
    }
    return ret;
}

int ExynosCameraConfigurations::getStaticValue(enum CONFIGURATON_STATIC_VALUE_TYPE type) const
{
    int value = -1;

    switch(type) {
    default:
        break;
    }
    return value;
}

status_t ExynosCameraConfigurations::setModeValue(enum CONFIGURATION_VALUE_TYPE type, int value)
{
    status_t ret = NO_ERROR;

    switch(type) {
    default:
        CLOGV("m_modeValue[type(%d)] = value(%d)!", type, value);
        m_modeValue[type] = value;
        break;
    case CONFIGURATION_DEVICE_ORIENTATION:
        if (value < 0 || value % 90 != 0) {
            CLOGE("Invalid orientation value(%d)!", value);
            return BAD_VALUE;
        }

        m_modeValue[type] = value;
        break;
    case CONFIGURATION_FD_ORIENTATION:
        /* Gets the FD orientation angle in degrees. Calibrate FRONT FD orientation */
        if (isFrontCamera(getCameraId())) {
            m_modeValue[type] = (value + FRONT_ROTATION + 180) % 360;
        } else {
            m_modeValue[type] = (value + BACK_ROTATION) % 360;
        }
        break;
    case CONFIGURATION_HIGHSPEED_MODE:
        if (value < 0) {
            CLOGE("Invalid Highspeed Mode value(%d)!", value);
            return BAD_VALUE;
        }

        m_modeValue[type] = value;
        setConfigMode((uint32_t)value);
        break;
    case CONFIGURATION_FLASH_MODE:
        m_modeValue[type] = value;
        break;
    case CONFIGURATION_BINNING_RATIO:
        if (value < MIN_BINNING_RATIO || value > MAX_BINNING_RATIO) {
            CLOGE(" Out of bound, ratio(%d), min:max(%d:%d)",
                    value, MAX_BINNING_RATIO, MAX_BINNING_RATIO);
            return BAD_VALUE;
        }

        m_modeValue[type] = value;
        break;
    case CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE:
    case CONFIGURATION_VT_MODE:
    case CONFIGURATION_YUV_SIZE_RATIO_ID:
    case CONFIGURATION_FLIP_HORIZONTAL:
    case CONFIGURATION_FLIP_VERTICAL:
    case CONFIGURATION_PERFRAME_FLIP_H_PICTURE:
    case CONFIGURATION_PERFRAME_FLIP_V_PICTURE:
    case CONFIGURATION_RECORDING_FPS:
    case CONFIGURATION_BURSTSHOT_FPS:
    case CONFIGURATION_BURSTSHOT_FPS_TARGET:
    case CONFIGURATION_NEED_DYNAMIC_BAYER_COUNT:
    case CONFIGURATION_RESTART_VIDEO_STABILIZATION:
    case CONFIGURATION_3DHDR_MODE:
        if (value < 0) {
            CLOGE("Invalid mode(%d) value(%d)!", (int)type, value);
            return BAD_VALUE;
        }
    case CONFIGURATION_EXTEND_SENSOR_MODE:
    case CONFIGURATION_MARKING_EXIF_FLASH:
#ifdef DEBUG_DUMP_IMAGE
    case CONFIGURATION_DUMP_IMAGE_VALUE:
#endif
        m_modeValue[type] = value;
        break;
    case CONFIGURATION_YUV_STALL_PORT:
        if (value < 0) {
            m_modeValue[type] = ExynosCameraConfigurations::YUV_2;
            break;
        }
    case CONFIGURATION_YUV_STALL_PORT_USAGE:
#ifdef LLS_CAPTURE
    case CONFIGURATION_LLS_VALUE:
#endif
        m_modeValue[type] = value;
        break;
    case CONFIGURATION_BRIGHTNESS_VALUE:
    case CONFIGURATION_CAPTURE_INTENT:
    case CONFIGURATION_CAPTURE_COUNT:
    case CONFIGURATION_ZSL_YUV_INPUT:
    case CONFIGURATION_ZSL_PRIV_INPUT:
    case CONFIGURATION_VIDEO_STABILIZATION_ENABLE:
    case CONFIGURATION_TOTAL_GAIN_VALUE:
    case CONFIGURATION_VENDOR_YUV_STALL_PORT:
    case CONFIGURATION_REMOSAIC_CAPTURE_MODE_VALUE:
    case CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE:
    case CONFIGURATION_SPORTS_YUV_MOTION_LEVEL:
#ifdef SUPPORT_LIMITED_HW_LEVEL_FALSH
    case CONFIGURATION_CALLBACK_REQUEST_KEY_VALUE:
#endif
        m_modeValue[type] = value;
        break;
    }

    return ret;
}

int ExynosCameraConfigurations::getModeValue(enum CONFIGURATION_VALUE_TYPE type) const
{
    int value = -1;

    switch(type) {
    default:
        value = m_modeValue[type];
        break;
    case CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE:
    case CONFIGURATION_VT_MODE:
    case CONFIGURATION_YUV_SIZE_RATIO_ID:
    case CONFIGURATION_DEVICE_ORIENTATION:
    case CONFIGURATION_FD_ORIENTATION:
    case CONFIGURATION_FLIP_HORIZONTAL:
    case CONFIGURATION_FLIP_VERTICAL:
    case CONFIGURATION_PERFRAME_FLIP_H_PICTURE:
    case CONFIGURATION_PERFRAME_FLIP_V_PICTURE:
    case CONFIGURATION_HIGHSPEED_MODE:
    case CONFIGURATION_RECORDING_FPS:
    case CONFIGURATION_BURSTSHOT_FPS:
    case CONFIGURATION_BURSTSHOT_FPS_TARGET:
    case CONFIGURATION_NEED_DYNAMIC_BAYER_COUNT:
    case CONFIGURATION_EXTEND_SENSOR_MODE:
    case CONFIGURATION_MARKING_EXIF_FLASH:
    case CONFIGURATION_FLASH_MODE:
    case CONFIGURATION_BINNING_RATIO:
    case CONFIGURATION_RESTART_VIDEO_STABILIZATION:
    case CONFIGURATION_3DHDR_MODE:
#ifdef DEBUG_DUMP_IMAGE
    case CONFIGURATION_DUMP_IMAGE_VALUE:
#endif
        value = m_modeValue[type];
        break;
    case CONFIGURATION_YUV_STALL_PORT:
    case CONFIGURATION_YUV_STALL_PORT_USAGE:
    case CONFIGURATION_BRIGHTNESS_VALUE:
        value = m_modeValue[type];
        break;
    case CONFIGURATION_CAPTURE_INTENT:
        value = (int)m_getCaptureIntent();
        break;
    case CONFIGURATION_CAPTURE_COUNT:
    case CONFIGURATION_ZSL_YUV_INPUT:
    case CONFIGURATION_ZSL_PRIV_INPUT:
    case CONFIGURATION_VIDEO_STABILIZATION_ENABLE:
    case CONFIGURATION_TOTAL_GAIN_VALUE:
#ifdef LLS_CAPTURE
    case CONFIGURATION_LLS_VALUE:
#endif
    case CONFIGURATION_VENDOR_YUV_STALL_PORT:
    case CONFIGURATION_REMOSAIC_CAPTURE_MODE_VALUE:
    case CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE:
    case CONFIGURATION_SPORTS_YUV_MOTION_LEVEL:
#ifdef SUPPORT_LIMITED_HW_LEVEL_FALSH
    case CONFIGURATION_CALLBACK_REQUEST_KEY_VALUE:
#endif
        value = m_modeValue[type];
        break;
    }

    return value;
}

status_t ExynosCameraConfigurations::setModeMultiValue(enum CONFIGURATION_MULTI_VALUE_TYPE type, int key, int value)
{
    status_t ret = NO_ERROR;

    switch(type) {
    default:
        CLOGV("m_modeValue[type(%d)] = key(%d), value(%d)!", type, key, value);
        (m_modeValueMap[type])[key] = value;
        break;
    }

    return ret;
}

int ExynosCameraConfigurations::getModeMultiValue(enum CONFIGURATION_MULTI_VALUE_TYPE type, int key) const
{
    int ret = 0;

    int count = m_modeValueMap[type].count(key);
    if (count) {
        ret = m_modeValueMap[type].at(key);
    }

    return ret;
}

bool ExynosCameraConfigurations::isSupportedFunction(enum SUPPORTED_FUNCTION_TYPE type) const
{
    bool functionSupport = false;

    switch(type) {
    case SUPPORTED_FUNCTION_GDC:
#ifdef SUPPORT_HW_GDC
        functionSupport = SUPPORT_HW_GDC;
#endif
        break;
    case SUPPORTED_FUNCTION_SERVICE_BATCH_MODE:
#ifdef USE_SERVICE_BATCH_MODE
        functionSupport = true;
#else
        functionSupport = false;
#endif
        break;
    case SUPPORTED_FUNCTION_HIFILLS:
#ifdef USES_HIFI_LLS
#ifdef USE_HIFILLS_CONTROL_REQUEST
        functionSupport = true;
#else
        functionSupport = m_isEnabledByProperty(type);
#endif
#else
        functionSupport = false;
#endif
        break;
    case SUPPORTED_FUNCTION_SW_VDIS:
    {
        bool enable = false;

        for(size_t i = 0 ; i < m_staticInfo->videoStabilizationModesLength ; i++) {
            if (m_staticInfo->videoStabilizationModes[i] == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON) {
                enable = true;
                break;
            }
        }

#ifdef USES_SW_VDIS
        bool supportControlRequest = false;
#ifdef USE_SW_VDIS_CONTROL_REQUEST
        supportControlRequest = USE_SW_VDIS_CONTROL_REQUEST;
#endif
        functionSupport = (enable == true
                            && (m_isEnabledByProperty(type) || supportControlRequest))? true : false;
#else
        functionSupport = false;
#endif

        break;
    }
    case SUPPORTED_FUNCTION_HIFI:
#ifdef USES_HIFI
        if (m_staticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING) {
            functionSupport = true;
        }
#else
        functionSupport = false;
#endif
        break;
    case SUPPORTED_FUNCTION_REMOSAIC_BY_SESSION:
#ifdef USE_SLSI_VENDOR_TAGS
        if (getModeMultiValue(CONFIGURATION_MULTI_SESSION_MODE_VALUE, EXYNOS_SESSION_MODE_REMOSAIC))
            functionSupport = true;
#endif
        break;
    case SUPPORTED_FUNCTION_REMOSAIC:
#ifdef USE_REMOSAIC_SENSOR
        if (m_staticInfo->previewHighResolutionSizeLut != NULL
            && m_staticInfo->captureHighResolutionSizeLut != NULL) {
            functionSupport = true;
        }
#endif
        break;
    case SUPPORTED_FUNCTION_LONGEXPOSURECAPTURE:
#ifdef USE_LONGEXPOSURECAPTURE_CONTROL_REQUEST
        functionSupport = true;
#else
        functionSupport = m_isEnabledByProperty(type);
#endif
        break;
    case SUPPORTED_FUNCTION_NIGHT_SHOT_BAYER:
    case SUPPORTED_FUNCTION_NIGHT_SHOT_YUV:
    case SUPPORTED_FUNCTION_SUPER_NIGHT_SHOT_BAYER:
    case SUPPORTED_FUNCTION_HDR_BAYER:
    case SUPPORTED_FUNCTION_HDR_YUV:
    case SUPPORTED_FUNCTION_FLASH_MULTI_FRAME_DENOISE_YUV:
    case SUPPORTED_FUNCTION_BEAUTY_FACE_YUV:
    case SUPPORTED_FUNCTION_SUPER_RESOLUTION:
    case SUPPORTED_FUNCTION_OIS_DENOISE_YUV:
    case SUPPORTED_FUNCTION_SPORTS_YUV:
    case SUPPORTED_FUNCTION_COMBINE_SINGLE_CAPTURE:
#ifdef USES_COMBINE_PLUGIN
#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
        functionSupport = true;
#else
        functionSupport = m_isEnabledByProperty(type);
#endif
#else
        functionSupport = false;
#endif
        break;
    case SUPPORTED_FUNCTION_P3:
#ifdef USES_P3_IN_EXIF
        functionSupport = true;
#endif
        break;
    case SUPPORTED_FUNCTION_CLAHE_CAPTURE:
#ifdef USE_CLAHE_REPROCESSING
        functionSupport = true;
#endif
        break;
    default:
        break;
    }

    return functionSupport;
}

bool ExynosCameraConfigurations::getEnabledByProperty(enum SUPPORTED_FUNCTION_TYPE type) const
{
    return m_isEnabledByProperty(type);
}

bool ExynosCameraConfigurations::m_isEnabledByProperty(enum SUPPORTED_FUNCTION_TYPE type) const
{
    bool bEnabled = false;
    status_t ret = NO_ERROR;

#ifdef USE_DEBUG_PROPERTY
    ExynosCameraProperty property;

    switch(type) {
    case SUPPORTED_FUNCTION_HIFILLS:
        ret = property.get(ExynosCameraProperty::SOLUTION_HIFI_LLS_ENABLE, LOG_TAG, bEnabled);
        if (ret != NO_ERROR) {
            bEnabled = false;
        }
        break;
    case SUPPORTED_FUNCTION_SW_VDIS:
        ret = property.get(ExynosCameraProperty::SOLUTION_VDIS_ENABLE, LOG_TAG, bEnabled);
        if (ret != NO_ERROR) {
            bEnabled = false;
        }
        break;
    case SUPPORTED_FUNCTION_NIGHT_SHOT_YUV:
        ret = property.get(ExynosCameraProperty::SOLUTION_NIGHT_SHOT_ENABLE, LOG_TAG, bEnabled);
        if (ret != NO_ERROR) {
            bEnabled = false;
        }
        break;
    case SUPPORTED_FUNCTION_HDR_YUV:
        ret = property.get(ExynosCameraProperty::SOLUTION_HDR_ENABLE, LOG_TAG, bEnabled);
        if (ret != NO_ERROR) {
            bEnabled = false;
        }
        break;
    default:
        break;
    }
#endif

    bEnabled = (isCustomCameraMode() & bEnabled);

    return bEnabled;
}

bool ExynosCameraConfigurations::isCustomCameraMode() const
{
    bool bCustom = false;
    status_t ret = NO_ERROR;

#ifdef USE_DEBUG_PROPERTY
    ExynosCameraProperty property;

    ret = property.get(ExynosCameraProperty::CUSTOM_CAMERA_ENABLE, LOG_TAG, bCustom);
    if (ret != NO_ERROR) {
        bCustom = false;
    }
#endif

    return bCustom;
}

void ExynosCameraConfigurations::setFastEntrance(bool flag)
{
     m_fastEntrance = flag;
}

bool ExynosCameraConfigurations::getFastEntrance(void) const
{
    return m_fastEntrance;
}

void ExynosCameraConfigurations::setUseFastenAeStable(bool enable)
{
     m_useFastenAeStable = enable;
}

bool ExynosCameraConfigurations::getUseFastenAeStable(void) const
{
    return m_useFastenAeStable;
}

status_t ExynosCameraConfigurations::setParameters(const CameraParameters& params)
{
    status_t ret = NO_ERROR;

    if (m_checkFastEntrance(params) !=  NO_ERROR) {
        CLOGE("checkFastEntrance faild");
    }

    if (m_checkShootingMode(params) != NO_ERROR) {
        CLOGE("checkShootingMode failed");
    }

    if (m_checkRecordingFps(params) !=  NO_ERROR) {
        CLOGE("checkRecordingFps faild");
    }

    if (m_checkOperationMode(params) !=  NO_ERROR) {
        CLOGE("checkOperationMode faild");
    }

    if (m_checkVtMode(params) != NO_ERROR) {
        CLOGE("checkVtMode failed");
    }

    if (m_checkExtSensorMode() != NO_ERROR) {
        CLOGE("checkExtSensorMode failed");
    }

    return ret;
}

status_t ExynosCameraConfigurations::m_checkFastEntrance(const CameraParameters& params)
{
    const char *newStrFastEntrance = params.get("first-entrance");
    bool fastEntrance = false;

    if (newStrFastEntrance != NULL
        && !strcmp(newStrFastEntrance, "true")) {
        fastEntrance = true;
    }

    CLOGD("Fast Entrance %s !!!", (fastEntrance?"Enabled":"Disabled"));
    setFastEntrance(fastEntrance);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_checkRecordingFps(const CameraParameters& params)
{
    int newRecordingFps = params.getInt("recording-fps");

    CLOGD("%d fps", newRecordingFps);
    setModeValue(CONFIGURATION_RECORDING_FPS, newRecordingFps);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_checkOperationMode(__unused const CameraParameters& params)
{
    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_checkExtSensorMode()
{
    int extendedSensorMode = EXTEND_SENSOR_MODE_NONE;
#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (getMode(CONFIGURATION_REMOSAIC_CAPTURE_ON)) {
        if (getModeValue(CONFIGURATION_REMOSAIC_CAPTURE_MODE_VALUE) == EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW) {
            extendedSensorMode = EXTEND_SENSOR_MODE_SW_REMOSAIC;
        }
    } else
#endif
#ifdef SUPPORT_VENDOR_TAG_3DHDR
    if (getModeValue(CONFIGURATION_3DHDR_MODE) != EXYNOS_CONTROL_HDR_MODE_OFF) {
        extendedSensorMode = EXTEND_SENSOR_MODE_3DHDR;
    }
#endif

    setModeValue(CONFIGURATION_EXTEND_SENSOR_MODE, extendedSensorMode);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_checkVtMode(const CameraParameters& params)
{
    int newVTMode = params.getInt("vtmode");
    int curVTMode = -1;

    CLOGD("newVTMode (%d)", newVTMode);
    /*
     * VT mode
     *   1: 3G vtmode (176x144, Fixed 7fps)
     *   2: LTE or WIFI vtmode (640x480, Fixed 15fps)
     *   3: Reserved : Smart Stay
     *   4: CHINA vtmode (1280x720, Fixed 30fps)
     */
    if (newVTMode == 3 || (newVTMode < 0 || newVTMode > 4)) {
        newVTMode = 0;
    }

    curVTMode = getModeValue(CONFIGURATION_VT_MODE);

    if (curVTMode != newVTMode) {
        setModeValue(CONFIGURATION_VT_MODE, newVTMode);
    }

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_checkShootingMode(const CameraParameters& params)
{
    int newShootingMode = params.getInt("shootingmode");

    CLOGD("shootingmode %d", newShootingMode);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::checkExtSensorMode()
{
    return m_checkExtSensorMode();
}

bool ExynosCameraConfigurations::m_checkClaheCaptureByCaptureMode(void)
{
    bool mode = false;

#ifdef USE_CLAHE_REPROCESSING
    if (getMode(CONFIGURATION_CLAHE_CAPTURE_MODE) == true) {
        return true;
    }
#endif

    return mode;
}

status_t ExynosCameraConfigurations::reInit(void)
{
    resetSize(CONFIGURATION_MIN_YUV_SIZE);
    resetSize(CONFIGURATION_MAX_YUV_SIZE);

#ifdef USE_DUAL_CAMERA
    m_dualOperationMode = DUAL_OPERATION_MODE_NONE;
    m_dualOperationModeReprocessing = DUAL_OPERATION_MODE_NONE;
    m_dualOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    m_dualOperationReprSensor = DUAL_OPERATION_SENSOR_MAX;
    m_dualOperationModeLockCount = 0;
    m_dualHwSyncOn = true;
    m_dualMasterCamId = m_cameraId;
    m_dualMasterReprCamId = m_cameraId;
    m_dualDisplayCameraId = m_cameraId;
    m_fallbackOn = false;
    m_fallbackCount = 0;
    m_fallbackState = DUAL_FALLBACK_OFF;
    m_fallbackDualOperationMode = DUAL_OPERATION_MODE_NONE;
    m_fallbackDualOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    m_fallbackDualOperationReprSensor = DUAL_OPERATION_SENSOR_MAX;
#endif

    setMode(CONFIGURATION_COMPRESSION_PREVIEW_STREAM, false);
    setMode(CONFIGURATION_COMPRESSION_VIDEO_STREAM, false);

    for (int i = 0; i < CONFIGURATION_MULTI_VALUE_MAX; i++)
        m_modeValueMap[i].clear();

    m_vendorReInit();
    m_physStreamExist = false;

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::m_vendorReInit(void)
{
    resetYuvStallPort();
    resetSize(CONFIGURATION_THUMBNAIL_CB_SIZE);

    setMode(CONFIGURATION_RECORDING_MODE, false);
    setMode(CONFIGURATION_DEPTH_MAP_MODE, false);
    setModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE, 0);
    setModeValue(CONFIGURATION_RECORDING_FPS, 0);
    setMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE, false);
    setModeValue(CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE, 0);
    setMode(CONFIGURATION_SESSION_MODE, false);
    setMode(CONFIGURATION_ZERO_CAMERA_MODE, false);

    setModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE, 0);
    setMode(CONFIGURATION_RESTART_FORCE_FLUSH, false);
#ifdef USES_COMBINE_PLUGIN
    setMode(CONFIGURATION_COMBINE_PREVIEW_PLUGIN, true);
    setModeValue(CONFIGURATION_COMBINE_PREVIEW_PLUGIN_VALUE, 0);
#endif
#ifdef SUPPORT_LIMITED_HW_LEVEL_FALSH
    setModeValue(CONFIGURATION_CALLBACK_REQUEST_KEY_VALUE, 0);
#endif
#ifdef USE_ALWAYS_FD_ON
    setMode(CONFIGURATION_ALWAYS_FD_ON_MODE, false);
#endif
#ifdef USES_SUPER_RESOLUTION
    setMode(CONFIGURATION_SUPER_RESOLUTION_MODE, false);
#endif
    setMode(CONFIGURATION_CLAHE_CAPTURE_MODE, false);
    m_appliedZoomRatio = -1.0f;
    m_temperature = 0xFFFF;
    m_yuvSizeRatioId = -1;
    m_pictureSizeRatioId = -1;

    return NO_ERROR;
}

void ExynosCameraConfigurations::resetYuvStallPort(void)
{
    m_modeValue[CONFIGURATION_YUV_STALL_PORT] = -1;
}

void ExynosCameraConfigurations::setCaptureExposureTime(uint64_t exposureTime)
{
    m_exposureTimeCapture = exposureTime;
}

uint64_t ExynosCameraConfigurations::getCaptureExposureTime(void)
{
    return m_exposureTimeCapture;
}

uint64_t ExynosCameraConfigurations::getLongExposureTime(void)
{
    return 0;
}

int32_t ExynosCameraConfigurations::getLongExposureShotCount(void)
{
#ifdef CAMERA_ADD_BAYER_ENABLE
    if (m_exposureTimeCapture <= CAMERA_SENSOR_EXPOSURE_TIME_MAX)
#endif
    {
        return 1;
    }

    return 0;
}

void ExynosCameraConfigurations::setExposureTime(int64_t exposureTime)
{
    m_exposureTime = exposureTime;
}

int64_t ExynosCameraConfigurations::getExposureTime(void)
{
    return m_exposureTime;
}

void ExynosCameraConfigurations::setGain(int gain)
{
    m_gain = gain;
}

int ExynosCameraConfigurations::getGain(void)
{
    return m_gain;
}

void ExynosCameraConfigurations::setLedPulseWidth(int64_t ledPulseWidth)
{
    m_ledPulseWidth = ledPulseWidth;
}

int64_t ExynosCameraConfigurations::getLedPulseWidth(void)
{
    return m_ledPulseWidth;
}

void ExynosCameraConfigurations::setLedPulseDelay(int64_t ledPulseDelay)
{
    m_ledPulseDelay = ledPulseDelay;
}

int64_t ExynosCameraConfigurations::getLedPulseDelay(void)
{
    return m_ledPulseDelay;
}

void ExynosCameraConfigurations::setLedCurrent(int ledCurrent)
{
    this->setLedCurrent(0, ledCurrent);
}

void ExynosCameraConfigurations::setLedCurrent(int ledIndex, int ledCurrent)
{
    m_ledCurrent[ledIndex] = ledCurrent;
}

int ExynosCameraConfigurations::getLedCurrent(void)
{
    return this->getLedCurrent(0);
}

int ExynosCameraConfigurations::getLedCurrent(int ledIndex)
{
    return m_ledCurrent[ledIndex];
}

void ExynosCameraConfigurations::setLedMaxTime(int ledMaxTime)
{
    if (m_isFactoryBin) {
        m_ledMaxTime = 0L;
        return;
    }

    m_ledMaxTime = ledMaxTime;
}

int ExynosCameraConfigurations::getLedMaxTime(void)
{
    return m_ledMaxTime;
}

void ExynosCameraConfigurations::setLedCalibrationEnable(bool enable)
{
    m_ledCalibrationEnabled = enable;
}

bool ExynosCameraConfigurations::getLedCalibrationEnable(void)
{
    return m_ledCalibrationEnabled;
}

void ExynosCameraConfigurations::setYuvBufferStatus(bool flag)
{
    m_yuvBufferStat = flag;
}

bool ExynosCameraConfigurations::getYuvBufferStatus(void)
{
    return m_yuvBufferStat;
}

#if defined(DEBUG_RAWDUMP) || defined(DEBUG_DUMP_IMAGE_NEED_ALWAYS_BAYER)
bool ExynosCameraConfigurations::checkBayerDumpEnable(void)
{
#ifndef RAWDUMP_CAPTURE
    char enableRawDump[PROPERTY_VALUE_MAX];
    property_get("ro.debug.rawdump", enableRawDump, "0");

    if (strcmp(enableRawDump, "1") == 0) {
        /*CLOGD("checkBayerDumpEnable : 1");*/
        return true;
    } else {
        /*CLOGD("checkBayerDumpEnable : 0");*/
        return false;
    }
#endif
    return true;
}
#endif  /* DEBUG_RAWDUMP */

#ifdef USE_DUAL_CAMERA
bool ExynosCameraConfigurations::checkFusionCaptureMode(ExynosCameraRequestSP_sprt_t request,
                                                        enum DUAL_OPERATION_MODE dualOperationMode,
                                                        enum DUAL_OPERATION_SENSORS dualOperationSensor)
{
    bool ret = false;
    int masterCamId, slaveCamId;
    int isFusionCaptureReady = getModeValue(CONFIGURATION_FUSION_CAPTURE_READY);
    ExynosCameraParameters *parameters;
    ExynosCameraActivityControl *activityControl;
    ExynosCameraActivityFlash *flashMgr;
    getCameraIdFromOperationSensor(dualOperationSensor, &masterCamId, &slaveCamId);
    if (dualOperationMode == DUAL_OPERATION_MODE_SLAVE) {
        masterCamId = slaveCamId;
    }
    if (masterCamId < 0) masterCamId = m_cameraId;
    parameters = m_parameters[masterCamId];
    activityControl = parameters->getActivityControl();
    flashMgr = activityControl->getFlashMgr();

    bool oldFusionMode = getMode(CONFIGURATION_FUSION_CAPTURE_MODE);

    if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
        if (isFusionCaptureReady == false ||
            (getMode(CONFIGURATION_RECORDING_MODE) == true) ||
            (flashMgr->getNeedCaptureFlash() == true)) {
            CLOGV("[Fusion][R%d] SYNC : NOT FUSION", request->getKey());
        } else {
            CLOGV("[Fusion][R%d] SYNC : FUSION", request->getKey());
            ret = true;
        }
    }

    if (ret != oldFusionMode) {
        CLOGD("[Fusion][R%d] %d -> %d dualOperationMode(%d) param(mode:%d, sensor:%d) isFusionCaptureReady(%d)",
                request->getKey(), oldFusionMode, ret, m_dualOperationMode,
                dualOperationMode, dualOperationSensor, isFusionCaptureReady);
    }

    setMode(CONFIGURATION_FUSION_CAPTURE_MODE, (int)ret);

    return ret;
}
#endif

void ExynosCameraConfigurations::setBokehBlurStrength(int bokehBlurStrength)
{
    m_bokehBlurStrength = bokehBlurStrength;
}

int ExynosCameraConfigurations::getBokehBlurStrength(void)
{
#ifdef USE_SLSI_PLUGIN
    // hack : set just for test.
    return 99;
#else
    return m_bokehBlurStrength;
#endif
}

int ExynosCameraConfigurations::getAntibanding()
{
    return AA_AE_ANTIBANDING_AUTO;
}

void ExynosCameraConfigurations::getZoomRect(ExynosRect *zoomRect)
{
    zoomRect->x = m_metaParameters.m_zoomRect.x;
    zoomRect->y = m_metaParameters.m_zoomRect.y;
    zoomRect->w = m_metaParameters.m_zoomRect.w;
    zoomRect->h = m_metaParameters.m_zoomRect.h;
}

void ExynosCameraConfigurations::setAppliedZoomRatio(float zoomRatio)
{
    m_appliedZoomRatio = zoomRatio;
}

float ExynosCameraConfigurations::getAppliedZoomRatio(void)
{
    return m_appliedZoomRatio;
}

void ExynosCameraConfigurations::setRepeatingRequestHint(int repeatingRequestHint)
{
    m_repeatingRequestHint = repeatingRequestHint;
}

int ExynosCameraConfigurations::getRepeatingRequestHint(void)
{
    return m_repeatingRequestHint;
}

void ExynosCameraConfigurations::setTemperature(int temperature)
{
    m_temperature = temperature;
}

int ExynosCameraConfigurations::getTemperature(void)
{
    return m_temperature;
}

void ExynosCameraConfigurations::setYuvSizeRatioId(int yuvSizeRatio)
{
    m_yuvSizeRatioId = yuvSizeRatio;
}

int ExynosCameraConfigurations::getYuvSizeRatioId(void)
{
    return m_yuvSizeRatioId;
}

void ExynosCameraConfigurations::setPictureSizeRatioId(int pictureSizeRatio)
{
    m_pictureSizeRatioId = pictureSizeRatio;
}

int ExynosCameraConfigurations::getPictureSizeRatioId(void)
{
    return m_pictureSizeRatioId;
}

int ExynosCameraConfigurations::m_vendorMaxNumOfSensorBuffer(bool isRemosaic)
{
    int maxBayerCaptureCount = 0;

#ifdef USE_SLSI_PLUGIN
    if (getMode(CONFIGURATION_ZERO_CAMERA_MODE) == true) {
        if (isSupportedFunction(SUPPORTED_FUNCTION_NIGHT_SHOT_BAYER)) {
            maxBayerCaptureCount = MAX(maxBayerCaptureCount, getLowlightMergeNum());
        }

        if (isSupportedFunction(SUPPORTED_FUNCTION_SUPER_NIGHT_SHOT_BAYER)) {
            maxBayerCaptureCount = MAX(maxBayerCaptureCount, getSuperNightShotBayerMergeNum());
        }

        if (isSupportedFunction(SUPPORTED_FUNCTION_HDR_BAYER)) {
            maxBayerCaptureCount = MAX(maxBayerCaptureCount, getHdrBayerMergeNum());
        }
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
        // in case of remosaic, it return only 1 by default
        if (isRemosaic) {
            maxBayerCaptureCount = 1;
        }
#endif
    } else
#endif
    {
        CLOGV("vendor max sensor buffer is zero.");
    }

    return maxBayerCaptureCount;
}

bool ExynosCameraConfigurations::getMinLockFreq(int32_t &bigMinLock, int32_t &littleMinLock)
{
    bool ret = false;
#ifdef USE_QOS_SETTING
    // TODO: Cpu minlock freq information should be stored in ExynosConfig
    if (getModeValue(CONFIGURATION_HIGHSPEED_MODE) >= CONFIG_MODE::HIGHSPEED_240) {
        bigMinLock = BIG_CORE_MIN_LOCK_HIGHSPEED240_RECORDING;
        littleMinLock = LITTLE_CORE_MIN_LOCK_HIGHSPEED240_RECORDING;
        ret = true;
    }
#ifdef SUPPORT_SENSOR_MODE_CHANGE
#ifdef SUPPORT_CPU_BOOST_SW_REMOSAIC_CAPTURE_MODE
    else if (isSensorModeTransition() == true) {
        littleMinLock = LITTLE_CORE_MIN_LOCK_SW_REMOSAIC_CAPTURE;
        bigMinLock = BIG_CORE_MAX_LOCK_SW_REMOSAIC_CAPTURE;
        ret = true;
    }
#endif
#endif
#ifdef SUPPORT_CPU_BOOST_RECORDING_MODE
    else if (getMode(CONFIGURATION_RECORDING_MODE)) {
        littleMinLock = LITTLE_CORE_MIN_LOCK_RECORDING
        bigMinLock = BIG_CORE_MAX_LOCK_RECORDING
        ret = true;
    }
#endif
#ifdef USES_SW_VDIS
#ifdef SUPPORT_CPU_BOOST_VDIS_RECORDING_MODE
    else if (getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE) > 0) {
        int videoW = 0, videoH = 0;
        uint32_t fps = 0;
        getSize(CONFIGURATION_VIDEO_SIZE, (uint32_t *)&videoW, (uint32_t *)&videoH);
        fps = getModeValue(CONFIGURATION_RECORDING_FPS);

#ifdef USE_SW_VDIS_UHD_RECORDING
        if (videoW == 3840 && videoH == 2160 && fps == 30) {
            littleMinLock = LITTLE_CORE_MIN_LOCK_UHD30_VDIS_RECORDING;
            bigMinLock = BIG_CORE_MAX_LOCK_UHD30_VDIS_RECORDING;
            ret = true;
        } else
#endif
#ifdef USE_SW_VDIS_FHD60_RECORDING
        if (videoW == 1920 && videoH == 1080 && fps == 60) {
            littleMinLock = LITTLE_CORE_MIN_LOCK_FHD60_VDIS_RECORDING;
            bigMinLock = BIG_CORE_MAX_LOCK_FHD60_VDIS_RECORDING;
            ret = true;
        }
#endif
    }
#endif
#endif

#endif /*USE_QOS_SETTING*/

    return ret;
}
}; /* namespace android */
