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
#define LOG_TAG "ExynosCameraConfigurations"
#include <log/log.h>

#include "ExynosCameraConfigurations.h"

namespace android {

ExynosCameraConfigurations::ExynosCameraConfigurations(cameraId_Info *camIdInfo)
{
    m_camIdInfo = camIdInfo;
    m_cameraId = camIdInfo->cameraId[MAIN_CAM];
    m_scenario = camIdInfo->scenario;

    if (getCamName(m_cameraId, m_name, sizeof(m_name)) != NO_ERROR) {
        memset(m_name, 0x00, sizeof(m_name));
        CLOGE("Invalid camera ID(%d)", m_cameraId);
    }

    m_staticInfo = createExynosCameraSensorInfo(m_cameraId, camIdInfo->serviceCameraId);

    m_exynosconfig = NULL;
    memset(&m_metaParameters, 0, sizeof(struct CameraMetaParameters));

    m_metaParameters.m_zoomRatio = 1.0f;
    m_metaParameters.m_flashMode = FLASH_MODE_OFF;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_parameters[i] = NULL;
    }

    m_exynosconfig = new ExynosConfigInfo();
    memset((void *)m_exynosconfig, 0x00, sizeof(struct ExynosConfigInfo));

    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START
    m_flagRestartStream = false;

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

    memset(m_width, 0, sizeof(m_width));
    memset(m_height, 0, sizeof(m_width));
    memset(m_mode, 0, sizeof(m_mode));
    memset(m_flagCheck, 0, sizeof(m_flagCheck));
    memset(m_modeValue, 0, sizeof(m_modeValue));
    memset(m_yuvWidth, 0, sizeof(m_yuvWidth));
    memset(m_yuvHeight, 0, sizeof(m_yuvHeight));
    memset(m_yuvFormat, 0, sizeof(m_yuvFormat));
    memset(m_yuvPixelSize, 0, sizeof(m_yuvPixelSize));
    memset(m_yuvBufferCount, 0, sizeof(m_yuvBufferCount));
    for (int i = 0; i < CONFIGURATION_MULTI_VALUE_MAX; i++)
        m_modeValueMap[i].clear();

    m_useDynamicBayer[CONFIG_MODE::NORMAL] = m_mode[CONFIGURATION_DYNAMIC_BAYER_MODE];

    if (isBackCamera(m_cameraId)) {
        m_mode[CONFIGURATION_DYNAMIC_BAYER_MODE] = USE_DYNAMIC_BAYER;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_60] = USE_DYNAMIC_BAYER_60FPS;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_120] = USE_DYNAMIC_BAYER_120FPS;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_240] = USE_DYNAMIC_BAYER_240FPS;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_480] = USE_DYNAMIC_BAYER_480FPS;
    } else {
        m_mode[CONFIGURATION_DYNAMIC_BAYER_MODE] = USE_DYNAMIC_BAYER_FRONT;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_60] = USE_DYNAMIC_BAYER_60FPS_FRONT;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_120] = USE_DYNAMIC_BAYER_120FPS_FRONT;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_240] = USE_DYNAMIC_BAYER_240FPS_FRONT;
        m_useDynamicBayer[CONFIG_MODE::HIGHSPEED_480] = USE_DYNAMIC_BAYER_480FPS_FRONT;
    }

    m_yuvBufferStat = false;

    for (int i = 0; i < YUV_OUTPUT_PORT_ID_MAX; i++) {
        m_yuvBufferCount[i] = 1; //YUV
    }

    m_minFps = 0;
    m_maxFps = 0;

    resetSize(CONFIGURATION_MIN_YUV_SIZE);
    setModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE, YUV_STALL_USAGE_DSCALED);

    m_exposureTimeCapture = 0L;

    // ois factory test related
    for (int i = 0; i < CONFIGURATION_OIS_TEST_MAX; i++) {
        m_oisTestRequireStatus[i] = false;
    }
    m_oisTestFwVer     = 0;
    m_oisTestHeaActual = 0;
    for (int i = 0; i < CONFIGURATION_OIS_HEA_MAX; i++) {
        m_oisTestHea[i] = 1;
    }

    m_vendorSpecificConstructor();

    // Initial Values : END
    setDefaultCameraInfo();
}

ExynosCameraConfigurations::~ExynosCameraConfigurations()
{
    if (m_exynosconfig != NULL) {
        memset((void *)m_exynosconfig, 0x00, sizeof(struct ExynosConfigInfo));
        delete m_exynosconfig;
        m_exynosconfig = NULL;
    }

    if (m_staticInfo != NULL) {
        delete m_staticInfo;
        m_staticInfo = NULL;
    }

    m_vendorSpecificDestructor();
}

void ExynosCameraConfigurations::setDefaultCameraInfo(void)
{
    CLOGI("");

    for (int i = 0; i < YUV_MAX; i++) {
        /* YUV */
        setYuvFormat(V4L2_PIX_FMT_NV21, i);
        setYuvPixelSize(CAMERA_PIXEL_SIZE_8BIT, i);

        /* YUV_STALL */
        setYuvFormat(V4L2_PIX_FMT_NV21, i + YUV_MAX);
        setYuvPixelSize(CAMERA_PIXEL_SIZE_8BIT, i + YUV_MAX);
    }

    /* Initalize Binning scale ratio */
    setModeValue(CONFIGURATION_BINNING_RATIO, 1000);

    setMode(CONFIGURATION_RECORDING_MODE, false);
    setMode(CONFIGURATION_PIP_MODE, false);
    setMode(CONFIGURATION_PIP_SUB_CAM_MODE, false);
}

void ExynosCameraConfigurations::setParameters(int cameraId, ExynosCameraParameters *parameters)
{
    if (cameraId < CAMERA_ID_BACK || CAMERA_ID_MAX <= cameraId) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid cameraId(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    m_parameters[cameraId] = parameters;
}

ExynosCameraParameters *ExynosCameraConfigurations::getParameters(int cameraId)
{
    if(cameraId < CAMERA_ID_BACK || CAMERA_ID_MAX <= cameraId) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid cameraId(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    return m_parameters[cameraId];
}

int ExynosCameraConfigurations::getScenario(void)
{
    return m_scenario;
}

void ExynosCameraConfigurations::setRestartStream(bool restart)
{
    Mutex::Autolock lock(m_modifyLock);
    m_flagRestartStream = restart;
}

bool ExynosCameraConfigurations::getRestartStream(void)
{
    Mutex::Autolock lock(m_modifyLock);
    return m_flagRestartStream;
}

bool ExynosCameraConfigurations::setConfig(struct ExynosConfigInfo* config)
{
    memcpy(m_exynosconfig, config, sizeof(struct ExynosConfigInfo));
    setConfigMode(m_exynosconfig->mode);

    return true;
}
struct ExynosConfigInfo* ExynosCameraConfigurations::getConfig(void)
{
    return m_exynosconfig;
}

bool ExynosCameraConfigurations::setConfigMode(uint32_t mode)
{
    bool ret = false;
    switch(mode){
    case CONFIG_MODE::NORMAL:
    case CONFIG_MODE::HIGHSPEED_60:
    case CONFIG_MODE::HIGHSPEED_120:
    case CONFIG_MODE::HIGHSPEED_240:
    case CONFIG_MODE::HIGHSPEED_480:
        m_exynosconfig->current = &m_exynosconfig->info[mode];
        m_exynosconfig->mode = mode;
        ret = true;
        break;
    default:
        CLOGE(" unknown config mode (%d)", mode);
    }
    return ret;
}

int ExynosCameraConfigurations::getConfigMode(void) const
{
    int ret = -1;
    switch(m_exynosconfig->mode){
    case CONFIG_MODE::NORMAL:
    case CONFIG_MODE::HIGHSPEED_60:
    case CONFIG_MODE::HIGHSPEED_120:
    case CONFIG_MODE::HIGHSPEED_240:
    case CONFIG_MODE::HIGHSPEED_480:
        ret = m_exynosconfig->mode;
        break;
    default:
        CLOGE(" unknown config mode (%d)", m_exynosconfig->mode);
    }

    return ret;
}

void ExynosCameraConfigurations::updateMetaParameter(struct CameraMetaParameters *metaParameters)
{
    memcpy(&this->m_metaParameters, metaParameters, sizeof(struct CameraMetaParameters));
    setModeValue(CONFIGURATION_FLASH_MODE, metaParameters->m_flashMode);
}

status_t ExynosCameraConfigurations::checkYuvFormat(const int format, const camera_pixel_size pixelSize, const int outputPortId)
{
    status_t ret = NO_ERROR;
    int curYuvFormat = -1, newYuvFormat = -1;
    int curPixelSize = getYuvPixelSize(outputPortId), newPixelSize = pixelSize;

    newYuvFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(format);
    curYuvFormat = getYuvFormat(outputPortId);

    if ((newYuvFormat != curYuvFormat) || (newPixelSize != curPixelSize)) {
        char curFormatName[V4L2_FOURCC_LENGTH] = {};
        char newFormatName[V4L2_FOURCC_LENGTH] = {};
        getV4l2Name(curFormatName, V4L2_FOURCC_LENGTH, curYuvFormat);
        getV4l2Name(newFormatName, V4L2_FOURCC_LENGTH, newYuvFormat);
        CLOGI("curYuvFormat %s newYuvFormat %s pixelSizeNum %d outputPortId %d",
                curFormatName, newFormatName, pixelSize, outputPortId);
        setYuvFormat(newYuvFormat, outputPortId);
        setYuvPixelSize(pixelSize, outputPortId);
    }

    return ret;
}

status_t ExynosCameraConfigurations::checkPreviewFpsRange(uint32_t minFps, uint32_t maxFps)
{
    status_t ret = NO_ERROR;
    uint32_t curMinFps = 0, curMaxFps = 0;

    getPreviewFpsRange(&curMinFps, &curMaxFps);

    if (m_adjustPreviewFpsRange(minFps, maxFps) != NO_ERROR) {
        CLOGE("Fails to adjust preview fps range");
        return INVALID_OPERATION;
    }

    if (curMinFps != minFps || curMaxFps != maxFps) {
        {
            if (curMaxFps <= 30 && maxFps == 60) {
                /* 60fps mode */
                setModeValue(CONFIGURATION_HIGHSPEED_MODE, (int)(CONFIG_MODE::HIGHSPEED_60));
                setRestartStream(true);
            } else if (curMaxFps == 60 && maxFps <= 30) {
                /* 30fps mode */
                setModeValue(CONFIGURATION_HIGHSPEED_MODE, (int)(CONFIG_MODE::NORMAL));
                setRestartStream(true);
            }
        }

        setPreviewFpsRange(minFps, maxFps);
    }

    return ret;
}

void ExynosCameraConfigurations::setPreviewFpsRange(uint32_t min, uint32_t max)
{
    m_minFps = min;
    m_maxFps = max;

#ifdef USE_DUAL_CAMERA
    CLOGV("fps min(%d) max(%d)", min, max);
#else
    CLOGI("fps min(%d) max(%d)", min, max);
#endif
}

void ExynosCameraConfigurations::getPreviewFpsRange(uint32_t *min, uint32_t *max)
{
    /* ex) min = 15 , max = 30 */
    *min = m_minFps;
    *max = m_maxFps;
}

status_t ExynosCameraConfigurations::m_adjustPreviewFpsRange(__unused uint32_t &newMinFps, uint32_t &newMaxFps)
{
#ifndef USE_LIMITATION_FOR_THIRD_PARTY
    UNUSED_VARIABLE(newMinFps);
#endif

    bool flagSpecialMode = false;

    if (getMode(CONFIGURATION_PIP_MODE) == true) {
        flagSpecialMode = true;
        CLOGV(" PIPMode(true), newMaxFps=%d", newMaxFps);
    }

    if (getMode(CONFIGURATION_PIP_RECORDING_MODE) == true) {
        flagSpecialMode = true;
        CLOGV("PIPRecordingHint(true), newMaxFps=%d", newMaxFps);
    }

    return NO_ERROR;
}

bool ExynosCameraConfigurations::setPhysStreamExistStatus(bool mode)
{
    m_physStreamExist = mode;
    return true;
}

bool ExynosCameraConfigurations::isPhysStreamExist(void)
{
    if (m_physStreamExist)
        return true;

    return false;
}

void ExynosCameraConfigurations::setYuvFormat(const int format, const int index)
{
    int formatArrayNum = sizeof(m_yuvFormat) / sizeof(m_yuvFormat[0]);

    if (formatArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid yuvFormat array length %d."\
                            " YUV_OUTPUT_PORT_ID_MAX %d",
                            __FUNCTION__, __LINE__,
                            formatArrayNum,
                            YUV_OUTPUT_PORT_ID_MAX);
        return;
    }

    m_yuvFormat[index] = format;
}

int ExynosCameraConfigurations::getYuvFormat(const int index)
{
    int formatArrayNum = sizeof(m_yuvFormat) / sizeof(m_yuvFormat[0]);

    if (formatArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid yuvFormat array length %d."\
                " YUV_OUTPUT_PORT_ID_MAX %d",
                __FUNCTION__, __LINE__,
                formatArrayNum,
                YUV_OUTPUT_PORT_ID_MAX);
        return - 1;
    }

    return m_yuvFormat[index];
}

void ExynosCameraConfigurations::setYuvPixelSize(const camera_pixel_size pixelSize, const int index)
{
    int pixelSizeArrayNum = sizeof(m_yuvPixelSize) / sizeof(m_yuvPixelSize[0]);

    if (pixelSizeArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid yuvPixelSize array length %d."\
                            " YUV_OUTPUT_PORT_ID_MAX %d",
                            __FUNCTION__, __LINE__,
                            pixelSizeArrayNum,
                            YUV_OUTPUT_PORT_ID_MAX);
        return;
    }

    m_yuvPixelSize[index] = pixelSize;
}

camera_pixel_size ExynosCameraConfigurations::getYuvPixelSize(const int index)
{
    int pixelSizeArrayNum = sizeof(m_yuvPixelSize) / sizeof(m_yuvPixelSize[0]);

    if (pixelSizeArrayNum != YUV_OUTPUT_PORT_ID_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid yuvPixelSize array length %d."\
                            " YUV_OUTPUT_PORT_ID_MAX %d",
                            __FUNCTION__, __LINE__,
                            pixelSizeArrayNum,
                            YUV_OUTPUT_PORT_ID_MAX);
        return CAMERA_PIXEL_SIZE_8BIT;
    }

    return m_yuvPixelSize[index];
}

status_t ExynosCameraConfigurations::setYuvBufferCount(const int count, const int outputPortId)
{
    if (count < 0 || count > VIDEO_MAX_FRAME
            || outputPortId < 0 || outputPortId >= YUV_OUTPUT_PORT_ID_MAX) {
        CLOGE("Invalid argument. count %d outputPortId %d", count, outputPortId);
        return BAD_VALUE;
    }

    m_yuvBufferCount[outputPortId] = count;

    return NO_ERROR;
}

int ExynosCameraConfigurations::getYuvBufferCount(const int outputPortId)
{
    if (outputPortId < 0 || outputPortId >= YUV_OUTPUT_PORT_ID_MAX) {
        CLOGE("Invalid index %d", outputPortId);
        return 0;
    }

    return m_yuvBufferCount[outputPortId];
}

void ExynosCameraConfigurations::resetYuvBufferCount(void)
{
    memset(m_yuvBufferCount, 0, sizeof(m_yuvBufferCount));
}

int ExynosCameraConfigurations::maxNumOfSensorBuffer(bool isRemosaic)
{
    ExynosConfigInfo *config = getConfig();

    if (config == NULL) {
        CLOGE("config ig NULL!");
        return -1;
    }

    int maxBufferCount = config->current->bufInfo.num_sensor_buffers;
    int index = m_camIdInfo->cameraId[MAIN_CAM];

    if (m_parameters[index]->getSensorControlDelay() == 0) {
#ifdef USE_DUAL_CAMERA
        if (getMode(CONFIGURATION_DUAL_MODE) == true) {
            maxBufferCount -= (SENSOR_REQUEST_DELAY * 2);
        } else
#endif
        {
            maxBufferCount -= SENSOR_REQUEST_DELAY;
        }
    }

#ifdef USE_DUAL_CAMERA
    if (getMode(CONFIGURATION_DUAL_MODE) == true) {
        maxBufferCount += (getModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE) * 2);
    } else
#endif
    {
        maxBufferCount += getModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE);
    }

#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    if (isRemosaic) {
        // it can be increase more
        maxBufferCount = 1;
    }
#endif

    int vendorMaxBufferCount = m_vendorMaxNumOfSensorBuffer(isRemosaic);

    CLOGD("maxBufferCount(%d), vendorMaxBufferCount(%d). so, set larger count to the max buffer(%d) [oldBayer:%d, sensorDelay:%d]",
            maxBufferCount, vendorMaxBufferCount, MAX(maxBufferCount, vendorMaxBufferCount),
            getModeValue(CONFIGURATION_MAX_OLD_BAYER_KEEP_VALUE), SENSOR_REQUEST_DELAY);

    maxBufferCount = MAX(maxBufferCount, vendorMaxBufferCount);
    if (maxBufferCount > MAX_BUFFERS) {
        android_printAssert(NULL, LOG_TAG,
                            "ASSERT(%s):Invalid buffer count(%d). The buffer count must be less than %d.",
                            __FUNCTION__, maxBufferCount, MAX_BUFFERS);
    }

    return maxBufferCount;
}

status_t ExynosCameraConfigurations::setFrameSkipCount(int count)
{
    m_frameSkipCounter.setCount(count);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::getFrameSkipCount(int *count)
{
    *count = m_frameSkipCounter.getCount();
    m_frameSkipCounter.decCount();

    return NO_ERROR;
}

float ExynosCameraConfigurations::getZoomRatio(void)
{
    return m_metaParameters.m_zoomRatio;
}

float ExynosCameraConfigurations::getPrevZoomRatio(void)
{
    return m_metaParameters.m_prevZoomRatio;
}

bool ExynosCameraConfigurations::checkClaheCaptureMode(void)
{
    uint32_t pictureW = 0, pictureH = 0;

    /* CLAHE only operates when picture height is multiple of 4 */
    getSize(CONFIGURATION_PICTURE_SIZE, &pictureW, &pictureH);
    if ((pictureH % 4) != 0) {
        return false;
    }

    bool enable = m_checkClaheCaptureByCaptureMode();
    CLOGD("Clahe enable(%d)", enable);

    return enable;
}

#ifdef USE_DUAL_CAMERA
enum DUAL_PREVIEW_MODE ExynosCameraConfigurations::getDualPreviewMode(void)
{
    /*
    * Before setParameters, we cannot know dualMode is valid or not
    * So, check and make assert for fast debugging
    */
    if (getMode(CONFIGURATION_DUAL_MODE) == false) {
        return DUAL_PREVIEW_MODE_OFF;
    }

    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        if (getMode(CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE) == true
            || getMode(CONFIGURATION_HDR_RECORDING_MODE) == true) {
            return DUAL_PREVIEW_MODE_SW_SWITCHING;      // switching dual
        } else {
            if (getDynamicMode(DYNAMIC_DUAL_FORCE_SWITCHING) == true) {
                return DUAL_PREVIEW_MODE_SW_SWITCHING;  // switching dual
            } else {
                return DUAL_PREVIEW_MODE_SW_FUSION;     // smooth zoom
            }
        }
    } else if(m_scenario == SCENARIO_DUAL_REAR_PORTRAIT || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        return DUAL_PREVIEW_MODE_SW_FUSION;   // bokeh
    } else {
        return DUAL_PREVIEW_MODE_OFF;
    }
}

enum DUAL_REPROCESSING_MODE ExynosCameraConfigurations::getDualReprocessingMode(void)
{
    /*
    * Before setParameters, we cannot know dualMode is valid or not
    * So, check and make assert for fast debugging
    */
    if (m_flagCheck[CONFIGURATION_DUAL_MODE] == false) {
        return DUAL_REPROCESSING_MODE_OFF;
    }

    if (getMode(CONFIGURATION_DUAL_MODE) == false) {
        return DUAL_REPROCESSING_MODE_OFF;
    }

#ifdef USE_DUAL_REPROCESSING_SW
    if (USE_DUAL_REPROCESSING_SW == true) {
        return DUAL_REPROCESSING_MODE_SW;
    } else
#endif
    {
        return DUAL_REPROCESSING_MODE_OFF;
    }
}


void ExynosCameraConfigurations::setDualOperationMode(enum DUAL_OPERATION_MODE mode)
{
    m_dualOperationMode = mode;
}

enum DUAL_OPERATION_MODE ExynosCameraConfigurations::getDualOperationMode(void)
{
    return m_dualOperationMode;
}

void ExynosCameraConfigurations::setDualOperationModeReprocessing(enum DUAL_OPERATION_MODE mode)
{
    m_dualOperationModeReprocessing = mode;
}

enum DUAL_OPERATION_MODE ExynosCameraConfigurations::getDualOperationModeReprocessing(void)
{
    return m_dualOperationModeReprocessing;
}

void ExynosCameraConfigurations::setDualOperationSensor(enum DUAL_OPERATION_SENSORS sensorId, bool isReprocessing)
{
    if (!isReprocessing)
        m_dualOperationSensor = sensorId;
    else
        m_dualOperationReprSensor = sensorId;
}

enum DUAL_OPERATION_SENSORS ExynosCameraConfigurations::getDualOperationSensor(bool isReprocessing)
{
    if (!isReprocessing)
        return m_dualOperationSensor;
    else
        return m_dualOperationReprSensor;
}

void ExynosCameraConfigurations::setDualDisplayCameraId(int32_t cameraId)
{
    Mutex::Autolock lock(m_lockDualDisplayCameraIdLock);

    CLOGV("%d -> %d", m_dualDisplayCameraId, cameraId);

    m_dualDisplayCameraId = cameraId;
}

int ExynosCameraConfigurations::getDualDisplayCameraId(void)
{
    Mutex::Autolock lock(m_lockDualDisplayCameraIdLock);

    return m_dualDisplayCameraId;
}

void ExynosCameraConfigurations::setDualCamId(int masterCamId, int slaveCamId, bool isReprocessing)
{
    if (!isReprocessing) {
        m_dualMasterCamId = masterCamId;
        m_dualSlaveCamId = slaveCamId;
    } else {
        m_dualMasterReprCamId = masterCamId;
        m_dualSlaveReprCamId = slaveCamId;
    }
}

void ExynosCameraConfigurations::getDualCamId(int &masterCamId, int &slaveCamId, bool isReprocessing)
{
    if (!isReprocessing) {
        masterCamId = m_dualMasterCamId;
        slaveCamId = m_dualSlaveCamId;
    } else {
        masterCamId = m_dualMasterReprCamId;
        slaveCamId = m_dualSlaveReprCamId;
    }
}

void ExynosCameraConfigurations::setDualOperationModeLockCount(int32_t count)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    CLOGV("%d -> %d", m_dualOperationModeLockCount, count);

    m_dualOperationModeLockCount = count;
}

int32_t ExynosCameraConfigurations::getDualOperationModeLockCount(void)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    return m_dualOperationModeLockCount;
}

void ExynosCameraConfigurations::decreaseDualOperationModeLockCount(void)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    if (m_dualOperationModeLockCount > 0)
        m_dualOperationModeLockCount--;
}


void ExynosCameraConfigurations::setDualHwSyncOn(bool hwSyncOn)
{
    m_dualHwSyncOn = hwSyncOn;
}

bool ExynosCameraConfigurations::getDualHwSyncOn(void) const
{
    return m_dualHwSyncOn;
}

#ifdef USE_SLSI_PLUGIN
PlugInScenario ExynosCameraConfigurations::getFusionCapturePluginScenario(void)
{
    PlugInScenario fusionCapturePlugin = (PlugInScenario)0;

    if (getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
        int scenario = getScenario();

#ifdef USES_COMBINE_PLUGIN
        if (scenario == SCENARIO_DUAL_REAR_ZOOM
            || scenario == SCENARIO_DUAL_REAR_PORTRAIT || scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
            fusionCapturePlugin = PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING;
        }
#elif defined(USES_DUAL_CAMERA_SOLUTION_FAKE)
        if (scenario == SCENARIO_DUAL_REAR_ZOOM
            || scenario == SCENARIO_DUAL_REAR_PORTRAIT || scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
            fusionCapturePlugin = PLUGIN_SCENARIO_FAKEFUSION_REPROCESSING;
        }
#elif defined(USES_DUAL_CAMERA_SOLUTION_ARCSOFT)
        switch (scenario) {
        case SCENARIO_DUAL_REAR_ZOOM:
            fusionCapturePlugin = PLUGIN_SCENARIO_ZOOMFUSION_REPROCESSING;
            break;
        case SCENARIO_DUAL_REAR_PORTRAIT:
        case SCENARIO_DUAL_FRONT_PORTRAIT:
            fusionCapturePlugin = PLUGIN_SCENARIO_BOKEHFUSION_REPROCESSING;
            break;
        default:
            CLOGE("Invalid Dual Scenario(%d). please be care", scenario);
            break;
        }
#endif
    }

    return fusionCapturePlugin;
}
#endif

int32_t ExynosCameraConfigurations::getCameraIdFromOperationSensor(enum DUAL_OPERATION_SENSORS id,
                                                                   int32_t *camId0, int32_t *camId1)
{
    int masterId = -1, slaveId = -1;

    switch (id) {
        case DUAL_OPERATION_SENSOR_BACK_MAIN:
        case DUAL_OPERATION_SENSOR_FRONT_MAIN:
            masterId = m_camIdInfo->cameraId[MAIN_CAM];  // SENSOR0
            break;

        case DUAL_OPERATION_SENSOR_BACK_SUB:
        case DUAL_OPERATION_SENSOR_FRONT_SUB:
            slaveId = m_camIdInfo->cameraId[SUB_CAM];  // SENSOR1
            break;

        case DUAL_OPERATION_SENSOR_BACK_SUB2:
            if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
                if (m_camIdInfo->numOfSensors > 2) {
                    slaveId = m_camIdInfo->cameraId[SUB_CAM2];
                } else {
                    slaveId = m_camIdInfo->cameraId[SUB_CAM];
                }
            }
            break;

        case DUAL_OPERATION_SENSOR_FRONT_MAIN_SUB:
        case DUAL_OPERATION_SENSOR_BACK_MAIN_SUB:
            masterId = m_camIdInfo->cameraId[MAIN_CAM]; // SENSOR0 & SENSOR1
            slaveId = m_camIdInfo->cameraId[SUB_CAM];
            break;

        case DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2:
            masterId = m_camIdInfo->cameraId[MAIN_CAM]; // SENSOR0 & SENSOR2
            if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) {
                slaveId = m_camIdInfo->cameraId[SUB_CAM];
            } else {
                if (m_camIdInfo->numOfSensors > 2) {
                    slaveId = m_camIdInfo->cameraId[SUB_CAM2];
                } else {
                    slaveId = m_camIdInfo->cameraId[SUB_CAM];
                }
            }
            break;

        case DUAL_OPERATION_SENSOR_BACK_SUB_MAIN:
            masterId = m_camIdInfo->cameraId[SUB_CAM];
            slaveId = m_camIdInfo->cameraId[MAIN_CAM]; // SENSOR1 & SENSOR0
            break;

        case DUAL_OPERATION_SENSOR_BACK_SUB2_MAIN:
            slaveId = m_camIdInfo->cameraId[MAIN_CAM]; // SENSOR2 & SENSOR0
            if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) {
                masterId = m_camIdInfo->cameraId[SUB_CAM];
            } else {
                if (m_camIdInfo->numOfSensors > 2) {
                    masterId = m_camIdInfo->cameraId[SUB_CAM2];
                } else {
                    masterId = m_camIdInfo->cameraId[SUB_CAM];
                }
            }
            break;

        default:
            break;
    }

    if (camId0)
        *camId0 = masterId;

    if (camId1)
        *camId1 = slaveId;

    return NO_ERROR;
}

bool ExynosCameraConfigurations::isFallbackOn(void)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    return (m_fallbackState == DUAL_FALLBACK_ON);
}

status_t ExynosCameraConfigurations::setFallbackOn(enum DUAL_OPERATION_MODE mode,
                                                   enum DUAL_OPERATION_SENSORS sensorId,
                                                   enum DUAL_OPERATION_SENSORS reprocessingSensorId)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    if (m_fallbackState == DUAL_FALLBACK_PROCESS) {
        CLOGW("already on processing fallback%d, mode:%d, sensor:%d",
                m_fallbackOn, m_fallbackDualOperationMode, m_fallbackDualOperationSensor);
        return INVALID_OPERATION;
    }

    bool valid = isValidDualOperationMode(mode, sensorId) &
                        isValidDualOperationMode(mode, reprocessingSensorId);
    if (!valid) {
        CLOGE("Invalid mode(%d), sensor(%d)!!", mode, sensorId);
        return BAD_VALUE;
    }

    if (m_dualOperationMode == mode &&
            m_dualOperationSensor == sensorId &&
            m_dualOperationReprSensor == reprocessingSensorId) {
        CLOGV("Already Fallbacked(%d) mode(%d), sensor(%d), repSensor(%d)",
                m_fallbackOn, mode, sensorId, reprocessingSensorId);
        return NO_ERROR;
    }

    m_fallbackOn = true;
    m_fallbackCount = 5;
    m_fallbackDualOperationMode = mode;
    m_fallbackDualOperationSensor = sensorId;
    m_fallbackDualOperationReprSensor = reprocessingSensorId;
    m_fallbackState = DUAL_FALLBACK_PROCESS;

    CLOGD("fallbackOn(mode:%d, sensor:%d, repSensor:%d)", mode, sensorId, reprocessingSensorId);

    return NO_ERROR;
}

status_t ExynosCameraConfigurations::setFallbackOff(void)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    if (m_fallbackState == DUAL_FALLBACK_PROCESS) {
        CLOGW("already on processing fallback%d, mode:%d, sensor:%d, repSensor:%d",
                m_fallbackOn, m_fallbackDualOperationMode, m_fallbackDualOperationSensor,
                m_fallbackDualOperationReprSensor);
        return INVALID_OPERATION;
    }

    if (m_fallbackState == DUAL_FALLBACK_OFF) {
        return NO_ERROR;
    }

    m_fallbackOn = false;
    m_fallbackCount = 15;
    m_fallbackDualOperationMode = DUAL_OPERATION_MODE_NONE;
    m_fallbackDualOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    m_fallbackDualOperationReprSensor = DUAL_OPERATION_SENSOR_MAX;
    m_fallbackState = DUAL_FALLBACK_PROCESS;

    CLOGD("fallbackOff");

    return NO_ERROR;
}

bool ExynosCameraConfigurations::getFallbackState(enum DUAL_OPERATION_MODE &mode,
                                                  enum DUAL_OPERATION_SENSORS &sensorId,
                                                  enum DUAL_OPERATION_SENSORS &reprocessingSensorId)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);

    mode = m_fallbackDualOperationMode;
    sensorId = m_fallbackDualOperationSensor;
    reprocessingSensorId = m_fallbackDualOperationReprSensor;

    return m_fallbackOn;
}

bool ExynosCameraConfigurations::checkFallbackState(void)
{
    Mutex::Autolock lock(m_lockDualOperationModeLock);
    if (m_fallbackState != DUAL_FALLBACK_PROCESS)
        return false;

    if (m_fallbackCount > 0)
        m_fallbackCount--;

    // for margin
    if (m_fallbackCount > 0)
        return false;

    if (m_fallbackOn == false) {
        // off
        // check if all sensor's state is stable
        bool stable = true;
        for (int i = 0; i < m_camIdInfo->numOfSensors; i++) {
            dual_standby_state_t state = m_parameters[m_camIdInfo->cameraId[i]]->getStandbyState();
            switch (state) {
            case DUAL_STANDBY_STATE_ON_READY:
            case DUAL_STANDBY_STATE_OFF_READY:
                stable = false;
                break;
            default:
                break;
            }

            if (stable) {
                m_fallbackState = DUAL_FALLBACK_OFF;
                return true;
            }
        }
    } else {
        // on
        // check if dualMode and sensor are changed to fallback
        if (m_dualOperationMode == m_fallbackDualOperationMode &&
                m_dualOperationSensor == m_fallbackDualOperationSensor &&
                m_dualOperationReprSensor == m_fallbackDualOperationReprSensor) {
            m_fallbackState = DUAL_FALLBACK_ON;
            return true;
        }
    }

    return false;
}

bool ExynosCameraConfigurations::isValidDualOperationMode(enum DUAL_OPERATION_MODE mode, enum DUAL_OPERATION_SENSORS sensorId)
{
    bool valid = false;

    switch (mode) {
    case DUAL_OPERATION_MODE_MASTER:
        switch (sensorId) {
        case DUAL_OPERATION_SENSOR_FRONT_MAIN:
        case DUAL_OPERATION_SENSOR_BACK_MAIN:
            valid = true;
            break;
        }
        break;
    case DUAL_OPERATION_MODE_SLAVE:
        switch (sensorId) {
        case DUAL_OPERATION_SENSOR_FRONT_SUB:
        case DUAL_OPERATION_SENSOR_BACK_SUB:
        case DUAL_OPERATION_SENSOR_BACK_SUB2:
            valid = true;
            break;
        }
        break;
    case DUAL_OPERATION_MODE_SYNC:
        switch (sensorId) {
        case DUAL_OPERATION_SENSOR_FRONT_MAIN_SUB:
        case DUAL_OPERATION_SENSOR_BACK_MAIN_SUB:
        case DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2:
        case DUAL_OPERATION_SENSOR_BACK_SUB_MAIN:
        case DUAL_OPERATION_SENSOR_BACK_SUB2_MAIN:
            valid = true;
            break;
        }
    default:
        break;
    }

    return valid;
}
#endif

bool ExynosCameraConfigurations::m_getGmvMode(void) const
{
    bool useGmv = false;

#ifdef SUPPORT_GMV
    useGmv = true;
#endif
    return useGmv;
}

void ExynosCameraConfigurations::setOisTestReqired(enum CONFIGURATION_OIS_TEST_TYPE index, bool required) {
    if (index >= sizeof(m_oisTestRequireStatus)) {
        CLOGW("[mot_factory]: OIS, only support %d cases", sizeof(m_oisTestRequireStatus));
        return;
    }
    m_oisTestRequireStatus[index] = required;
}

bool ExynosCameraConfigurations::getOisTestReqired(enum CONFIGURATION_OIS_TEST_TYPE index) const {
    if (index >= sizeof(m_oisTestRequireStatus)) {
        CLOGW("[mot_factory]: OIS, only support %d cases", sizeof(m_oisTestRequireStatus));
        return false;
    }

    return m_oisTestRequireStatus[index];
}

void ExynosCameraConfigurations::setOisTestFwVer(uint32_t data) {
    m_oisTestFwVer = data;
}

uint32_t ExynosCameraConfigurations::getOisTestFwVer() {
    return m_oisTestFwVer;
}

void ExynosCameraConfigurations::setOisTestHeaActual(uint32_t data) {
    m_oisTestHeaActual = data;
}

uint32_t ExynosCameraConfigurations::getOisTestHeaActual() {
    return m_oisTestHeaActual;
}

void ExynosCameraConfigurations::setOisTestHea(uint32_t *data) {
    for (int i = 0; i < CONFIGURATION_OIS_HEA_MAX; i++) {
        m_oisTestHea[i] = data[i];
    }
}

void ExynosCameraConfigurations::getOisTestHea(int *data) {
    for (int i = 0; i < CONFIGURATION_OIS_HEA_MAX; i++) {
        data[i] = static_cast<int>(m_oisTestHea[i]);
    }
}

enum aa_capture_intent ExynosCameraConfigurations::m_getCaptureIntent(void) const
{
    enum aa_capture_intent captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE;

    if (this->getMode(CONFIGURATION_HDR_BAYER_MODE) == true ||
        this->getMode(CONFIGURATION_HDR_YUV_MODE) == true) {
        captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_MFHDR_DYNAMIC_SHOT;
    } else if (this->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == true) {
        captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_SUPER_NIGHT_SHOT_HANDHELD;
    } else if (this->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE) == true) {
        captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_LLS_FLASH;
    } else if (this->getMode(CONFIGURATION_SPORTS_YUV_MODE) == true) {
        int motionLevel = this->getModeValue(CONFIGURATION_SPORTS_YUV_MOTION_LEVEL);

        switch (motionLevel) {
        case 0:
            captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_SPORT_MOTIONLEVEL0;
            break;
        case 1:
            captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_SPORT_MOTIONLEVEL1;
            break;
        case 2:
            captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_SPORT_MOTIONLEVEL2;
            break;
        default:
            CLOGW("Invalid motionLevel(%d). just set AA_CAPTURE_INTENT_STILL_CAPTURE_SPORT_MOTIONLEVEL0", motionLevel);
            captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_SPORT_MOTIONLEVEL0;
            break;
        }
    } else if (this->getMode(CONFIGURATION_OIS_CAPTURE_MODE) == true ||
               this->getMode(CONFIGURATION_OIS_DENOISE_YUV_MODE) == true) {
        captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_OIS_MULTI;
    }

    return captureIntent;
}

}; /* namespace android */
