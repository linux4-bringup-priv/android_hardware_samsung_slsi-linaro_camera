/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraMetadataConverter"

#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraRequestManager.h"

namespace android {
#define SET_BIT(x)      (1 << x)

#if ENABLE_SERVICE_METADATA_VALIDATE_CHECK
#define META_VALIDATE_CHECK(x) do { \
        if (validate_camera_metadata_structure(x->getAndLock(), NULL)) CLOGE("ERR!!!"); \
        x->unlock(x->getAndLock()); \
    } while(0)
#else
#define META_VALIDATE_CHECK(x)
#endif

ExynosCameraMetadataConverter::ExynosCameraMetadataConverter(cameraId_Info *camIdInfo,
                                                             ExynosCameraConfigurations *configurations,
                                                             ExynosCameraParameters **parameters)
{
    m_camIdInfo = camIdInfo;
    int camIdx = MAIN_CAM;
    if (camIdInfo->sensorInfoCamIdx) {
        camIdx = camIdInfo->sensorInfoCamIdx;
    }
    m_cameraId = m_camIdInfo->cameraId[camIdx];
    m_configurations = configurations;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_parameters[i] = parameters[i];
        m_prevMetaPhysCam[i] = NULL;
    }

    m_rectUiSkipCount = 0;

    m_sensorStaticInfo = m_parameters[m_cameraId]->getSensorStaticInfo();
    m_preCaptureTriggerOn = false;
    m_isManualAeControl = false;
    m_prevMeta = NULL;

    m_afMode = AA_AFMODE_CONTINUOUS_PICTURE;
    m_preAfMode = AA_AFMODE_CONTINUOUS_PICTURE;
    for (int i = 0; i < sizeof(m_lastPreviewCropRegion)/sizeof(uint32_t); i++) {
        m_lastPreviewCropRegion[i] = 0;
    }

    m_focusDistance = -1;

    m_maxFps = 30;
    m_overrideFlashControl= false;
    m_defaultAntibanding = m_configurations->getAntibanding();
    m_sceneMode = 0;

    m_frameCountMapIndex = 0;
    memset(m_frameCountMap, 0x00, sizeof(m_frameCountMap));
    memset(m_name, 0x00, sizeof(m_name));

#ifdef USE_SLSI_VENDOR_TAGS
    memset(&m_oldShotExt, 0x00, sizeof(m_oldShotExt));
#endif

    memset(&m_dmMap, 0x00, sizeof(m_dmMap));
    memset(&m_udmMap, 0x00, sizeof(m_udmMap));
    memset(&m_shotExtUserMap, 0x00, sizeof(m_shotExtUserMap));
    memset(&m_vraExtMap, 0x00, sizeof(m_vraExtMap));

}

ExynosCameraMetadataConverter::~ExynosCameraMetadataConverter()
{
    m_defaultRequestSetting.clear();
    m_staticInfo.clear();
}

status_t ExynosCameraMetadataConverter::constructDefaultRequestSettings(int type, camera_metadata_t **request)
{
    CLOGD("Type(%d)", type);

    CameraMetadata settings;
    uint8_t hwLevel = m_sensorStaticInfo->supportedHwLevel;
    uint64_t supportedCapabilities = m_sensorStaticInfo->supportedCapabilities;

#ifdef USE_AF_FOV_COMPENSATION
    m_configurations->setMode(CONFIGURATION_VIDEO_MODE, false);
#endif

    /** android.request */
    /* request type */
    const uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    settings.update(ANDROID_REQUEST_TYPE, &requestType, 1);

    /* meta data mode */
    const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
    settings.update(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);

    /* id */
    const int32_t id = 0;
    settings.update(ANDROID_REQUEST_ID, &id, 1);

    /* frame count */
    const int32_t frameCount = 0;
    settings.update(ANDROID_REQUEST_FRAME_COUNT, &frameCount, 1);

    /** android.control */
    /* control intent */
    uint8_t controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        CLOGD("type is ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW");
        break;
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        CLOGD("type is CAMERA3_TEMPLATE_STILL_CAPTURE");
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
#ifdef USE_AF_FOV_COMPENSATION
    m_configurations->setMode(CONFIGURATION_VIDEO_MODE, true);
#endif
        CLOGD("type is CAMERA3_TEMPLATE_VIDEO_RECORD");
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        CLOGD("type is CAMERA3_TEMPLATE_VIDEO_SNAPSHOT");
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        CLOGD("type is CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG");
        break;
    case CAMERA3_TEMPLATE_MANUAL:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL;
        CLOGD("type is CAMERA3_TEMPLATE_MANUAL");
        break;
    default:
        CLOGD("Custom intent type is selected for setting control intent(%d)", type);
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    break;
    }
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    /* 3AA control */
    uint8_t controlMode = ANDROID_CONTROL_MODE_OFF;
    uint8_t afMode = ANDROID_CONTROL_AF_MODE_OFF;
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_OFF;
    uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
    int32_t aeTargetFpsRange[2];
    aeTargetFpsRange[0] = m_sensorStaticInfo->defaultFpsMin[DEFAULT_FPS_STILL];
    aeTargetFpsRange[1] = m_sensorStaticInfo->defaultFpsMAX[DEFAULT_FPS_STILL];

    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        /* Fix FPS for Recording */
        aeTargetFpsRange[0] = m_sensorStaticInfo->defaultFpsMin[DEFAULT_FPS_VIDEO];
        aeTargetFpsRange[1] = m_sensorStaticInfo->defaultFpsMAX[DEFAULT_FPS_VIDEO];
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_MANUAL:
        controlMode = ANDROID_CONTROL_MODE_OFF;
        afMode = ANDROID_CONTROL_AF_MODE_OFF;
        aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
        break;
    default:
        CLOGD("Custom intent type is selected for setting 3AA control(%d)", type);
        break;
    }
    settings.update(ANDROID_CONTROL_MODE, &controlMode, 1);
    if (m_sensorStaticInfo->minimumFocusDistance == 0.0f) {
        afMode = ANDROID_CONTROL_AF_MODE_OFF;
    }
    settings.update(ANDROID_CONTROL_AF_MODE, &afMode, 1);
    settings.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    settings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFpsRange, 2);

    const uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);

    const uint8_t aePrecaptureTrigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePrecaptureTrigger, 1);

    /* effect mode */
    const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    /* scene mode */
    const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    /* ae lock mode */
    const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    /* ae region */
    int w,h;
    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&w, (uint32_t *)&h);
    const int32_t controlRegions[5] = {
        0, 0, w, h, 0
    };

    /* HACK : temp code for af team */
    const int32_t afControlRegions[5] = {
        0, 0, 0, 0, 0
    };

    if (m_sensorStaticInfo->max3aRegions[AE] > 0) {
        settings.update(ANDROID_CONTROL_AE_REGIONS, controlRegions, 5);
    }
    if (m_sensorStaticInfo->max3aRegions[AWB] > 0) {
        settings.update(ANDROID_CONTROL_AWB_REGIONS, controlRegions, 5);
    }
    if (m_sensorStaticInfo->max3aRegions[AF] > 0) {
#if 1 /* HACK : temp code for af team */
        settings.update(ANDROID_CONTROL_AF_REGIONS, afControlRegions, 5);
#else
        settings.update(ANDROID_CONTROL_AF_REGIONS, controlRegions, 5);
#endif
    }

    /* exposure compensation */
    const int32_t aeExpCompensation = 0;
    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

    /* anti-banding mode */
    const uint8_t aeAntibandingMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &aeAntibandingMode, 1);

    /* awb lock */
    const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

    /* video stabilization mode */
    const uint8_t vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstabMode, 1);

    const int32_t rawsensitivity = 100;
    settings.update(ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST, &rawsensitivity, 1);

    /** android.lens */
    const float focusDistance = -1.0f;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);
    settings.update(ANDROID_LENS_FOCAL_LENGTH, &(m_sensorStaticInfo->focalLength), 1);
    settings.update(ANDROID_LENS_APERTURE, &(m_sensorStaticInfo->fNumber), 1); // ExifInterface :  TAG_APERTURE = "FNumber";
    settings.update(ANDROID_LENS_FILTER_DENSITY, &(m_sensorStaticInfo->filterDensity), 1);
#ifdef MOT_9609_SENSORS_TROIKA
    const uint8_t opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
#else
    const uint8_t opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON;
#endif
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &opticalStabilizationMode, 1);

    /** android.sensor */
    settings.update(ANDROID_SENSOR_EXPOSURE_TIME, &(m_sensorStaticInfo->exposureTime), 1);
    const int64_t frameDuration = 33333333L; /* 1/30 s */
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);
    const int32_t sensitivity = m_sensorStaticInfo->sensitivityRange[MIN];  // iso auto mode
    settings.update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

    /** android.flash */
    const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);
    const uint8_t firingPower = 0;
    settings.update(ANDROID_FLASH_FIRING_POWER, &firingPower, 1);
    const int64_t firingTime = 0;
    settings.update(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);

    /** android.led */
    if (m_sensorStaticInfo->leds != NULL) {
        const uint8_t ledTransmit = ANDROID_LED_TRANSMIT_OFF ;
        settings.update(ANDROID_LED_TRANSMIT, &ledTransmit, 1);
    }

    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_MANUAL_POST_PROCESSING) {
        /** android.color_correction */
        const camera_metadata_rational_t colorTransform[9] = {
            {1,1}, {0,1}, {0,1},
            {0,1}, {1,1}, {0,1},
            {0,1}, {0,1}, {1,1}
        };
        settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);

        const float correctionGains[4] = {
            0.0f, 0.0f,
            0.0f, 0.0f
        };
        settings.update(ANDROID_COLOR_CORRECTION_GAINS, correctionGains, 4);

        /** android.tonemap */
        const float tonemapCurve[4] = {
            0.0f, 0.0f,
            1.0f, 1.0f
        };
        settings.update(ANDROID_TONEMAP_CURVE_RED, tonemapCurve, 4);
        settings.update(ANDROID_TONEMAP_CURVE_GREEN, tonemapCurve, 4);
        settings.update(ANDROID_TONEMAP_CURVE_BLUE, tonemapCurve, 4);
    }

#if 0 //USE_SAMPLE_SESSION_KEYS
    /** android.noise_reduction */
    const uint8_t noiseStrength = 5;
    settings.update(ANDROID_NOISE_REDUCTION_STRENGTH, &noiseStrength, 1);

    /** android.edge */
    const uint8_t edgeStrength = 5;
    settings.update(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);

    /** android.shading */
    const uint8_t shadingStrength = 5;
    settings.update(ANDROID_SHADING_STRENGTH, &shadingStrength, 1);
#endif

    /** android.scaler */
    const int32_t cropRegion[4] = {
        0, 0, w, h
    };
    settings.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);

    /** android.jpeg */
    const uint8_t jpegQuality = 96;
    settings.update(ANDROID_JPEG_QUALITY, &jpegQuality, 1);
    const int32_t thumbnailSize[2] = {
        m_sensorStaticInfo->maxThumbnailW, m_sensorStaticInfo->maxThumbnailH
    };
    settings.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);
    const uint8_t thumbnailQuality = 100;
    settings.update(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);
    const double gpsCoordinates[3] = {
        0, 0 , 0
    };
    settings.update(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3);
    const uint8_t gpsProcessingMethod[32] = "None";
    settings.update(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, 32);
    const int64_t gpsTimestamp = 0;
    settings.update(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);
    const int32_t jpegOrientation = 0;
    settings.update(ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    /** android.stats */
    uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    uint8_t sharpnessMapMode = ANDROID_STATISTICS_SHARPNESS_MAP_MODE_OFF;
    uint8_t hotPixelMapMode = 0;

    settings.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);
    settings.update(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);
    settings.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);

    /** android.blacklevel */
    const uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
    settings.update(ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock, 1);

    const int32_t patternMode = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    settings.update(ANDROID_SENSOR_TEST_PATTERN_MODE, &patternMode, 1);

    /** Processing block modes */
    uint8_t hotPixelMode = ANDROID_HOT_PIXEL_MODE_OFF;
    uint8_t demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
    uint8_t noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
    uint8_t shadingMode = ANDROID_SHADING_MODE_OFF;
    uint8_t colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
    uint8_t tonemapMode = ANDROID_TONEMAP_MODE_CONTRAST_CURVE;
    uint8_t edgeMode = ANDROID_EDGE_MODE_OFF;
    uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    uint8_t colorCorrectionAberrationMode = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;

    switch (type) {
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        if (supportedCapabilities & CAPABILITIES_RAW) {
            lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON;
        }
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        shadingMode = ANDROID_SHADING_MODE_HIGH_QUALITY;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG;
        break;
    case CAMERA3_TEMPLATE_PREVIEW:
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
    default:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_FAST;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    }

    if (hwLevel != ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
        settings.update(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
        settings.update(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);
        settings.update(ANDROID_SHADING_MODE, &shadingMode, 1);
        settings.update(ANDROID_TONEMAP_MODE, &tonemapMode, 1);
        settings.update(ANDROID_EDGE_MODE, &edgeMode, 1);
        settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &lensShadingMapMode, 1);
    }
    settings.update(ANDROID_NOISE_REDUCTION_MODE, &noiseReductionMode, 1);
    settings.update(ANDROID_COLOR_CORRECTION_MODE, &colorCorrectionMode, 1);
    settings.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE, &colorCorrectionAberrationMode, 1);

    /* Vendor RequestSettings */
    m_constructVendorDefaultRequestSettings(type, &settings);

    *request = settings.release();
    m_defaultRequestSetting = *request;

    CLOGD("Registered default request template(%d)", type);
    return OK;
}

status_t ExynosCameraMetadataConverter::constructStaticInfo(cameraId_Info *camIdInfo,
                                                            camera_metadata_t **cameraInfo,
                                                            HAL_CameraInfo_t **HAL_CameraInfo)
{
    status_t ret = NO_ERROR;
    bool isLogicalCamera = isLogicalCam(camIdInfo->serviceCameraId);
    int camIdx = MAIN_CAM;
    if (camIdInfo->sensorInfoCamIdx) {
        camIdx = camIdInfo->sensorInfoCamIdx;
    }

    CLOGD2("[%d] serviceCameraId(%d), mainCameraId(%d), subCameraId(sub:%d,sub2:%d), scenario(%d)",
            camIdx, camIdInfo->serviceCameraId,
            camIdInfo->cameraId[MAIN_CAM], camIdInfo->cameraId[SUB_CAM], camIdInfo->cameraId[SUB_CAM2],
            camIdInfo->scenario);
    struct ExynosCameraSensorInfoBase *sensorStaticInfo = NULL;
    CameraMetadata info;
    Vector<int64_t> i64Vector;
    Vector<int32_t> i32Vector;
    Vector<uint8_t> i8Vector;

    sensorStaticInfo = createExynosCameraSensorInfo(camIdInfo->cameraId[camIdx], camIdInfo->serviceCameraId);
    if (sensorStaticInfo == NULL) {
        CLOGE2("sensorStaticInfo is NULL");
        return BAD_VALUE;
    }

    /* Get HAL CameraInfos by sensorInfo */
    if (HAL_CameraInfo != NULL && camIdInfo->serviceCameraId >= 0) {
        *HAL_CameraInfo = getExynosCameraVendorDeviceInfoByServiceCamId(camIdInfo->serviceCameraId);
    }

    /* android.colorCorrection static attributes */
    if (sensorStaticInfo->colorAberrationModes != NULL) {
        ret = info.update(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
                sensorStaticInfo->colorAberrationModes,
                sensorStaticInfo->colorAberrationModesLength);
        if (ret < 0) {
            CLOGD2("ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("colorAberrationModes at sensorStaticInfo is NULL");
    }

    /* andorid.control static attributes */
    if (sensorStaticInfo->antiBandingModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                sensorStaticInfo->antiBandingModes,
                sensorStaticInfo->antiBandingModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES update failed(%d)", ret);
    } else {
        CLOGD2("antiBandingModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->aeModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
                sensorStaticInfo->aeModes,
                sensorStaticInfo->aeModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AE_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("aeModes at sensorStaticInfo is NULL");
    }

    i32Vector.clear();
    m_createAeAvailableFpsRanges(sensorStaticInfo, &i32Vector);
    ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            i32Vector.array(), i32Vector.size());
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES update failed(%d)", ret);

    ret = info.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            sensorStaticInfo->exposureCompensationRange,
            ARRAY_LENGTH(sensorStaticInfo->exposureCompensationRange));
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_AE_COMPENSATION_RANGE update failed(%d)", ret);

    const camera_metadata_rational exposureCompensationStep =
    {(int32_t)((sensorStaticInfo->exposureCompensationStep) * 100.0), 100};
    ret = info.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            &exposureCompensationStep, 1);
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_AE_COMPENSATION_STEP update failed(%d)", ret);

    if (sensorStaticInfo->afModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                sensorStaticInfo->afModes,
                sensorStaticInfo->afModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AF_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("afModes is NULL");
    }

    if (sensorStaticInfo->effectModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
                sensorStaticInfo->effectModes,
                sensorStaticInfo->effectModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AVAILABLE_EFFECTS update failed(%d)", ret);
    } else {
        CLOGD2("effectModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->sceneModes != NULL) {
        i8Vector.clear();
        i8Vector.appendArray(sensorStaticInfo->sceneModes, sensorStaticInfo->sceneModesLength);
        if (sensorStaticInfo->sceneHDRSupport) {
            i8Vector.add(ANDROID_CONTROL_SCENE_MODE_HDR);
        }
        ret = info.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
                    i8Vector.array(), i8Vector.size());
        if (ret < 0)
            CLOGE2("ANDROID_CONTROL_AVAILABLE_SCENE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("sceneModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->sceneModeOverrides != NULL) {
        i8Vector.clear();
        i8Vector.appendArray(sensorStaticInfo->sceneModeOverrides, sensorStaticInfo->sceneModeOverridesLength);
        if (sensorStaticInfo->sceneHDRSupport) {
            i8Vector.add(ANDROID_CONTROL_AE_MODE_ON);
            i8Vector.add(ANDROID_CONTROL_AWB_MODE_AUTO);
            i8Vector.add(ANDROID_CONTROL_AF_MODE_AUTO);
        }
        ret  = info.update(ANDROID_CONTROL_SCENE_MODE_OVERRIDES,
                        i8Vector.array(), i8Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_SCENE_MODE_OVERRIDES update failed(%d)", ret);
    } else {
        CLOGD2("sceneModeOverrides at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->videoStabilizationModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                sensorStaticInfo->videoStabilizationModes,
                sensorStaticInfo->videoStabilizationModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES update failed(%d)", ret);
    } else {
        CLOGD2("videoStabilizationModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->awbModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                sensorStaticInfo->awbModes,
                sensorStaticInfo->awbModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AWB_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("awbModes at sensorStaticInfo is NULL");
    }

    ret = info.update(ANDROID_CONTROL_MAX_REGIONS,
            sensorStaticInfo->max3aRegions,
            ARRAY_LENGTH(sensorStaticInfo->max3aRegions));
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_MAX_REGIONS update failed(%d)", ret);

    i32Vector.clear();
    if (m_createControlAvailableHighSpeedVideoConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR ) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS,
             i32Vector.array(), i32Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS update failed(%d)", ret);
    } else {
        CLOGD2("ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS is NULL");
    }

    ret = info.update(ANDROID_CONTROL_AE_LOCK_AVAILABLE,
            &(sensorStaticInfo->aeLockAvailable), 1);
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_AE_LOCK_AVAILABLE update failed(%d)", ret);

    ret = info.update(ANDROID_CONTROL_AWB_LOCK_AVAILABLE,
            &(sensorStaticInfo->awbLockAvailable), 1);
    if (ret < 0)
        CLOGD2("ANDROID_CONTROL_AWB_LOCK_AVAILABLE update failed(%d)", ret);

    if (sensorStaticInfo->controlModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_MODES,
                sensorStaticInfo->controlModes,
                sensorStaticInfo->controlModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_CONTROL_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("controlModes at sensorStaticInfo is NULL");
    }

    ret = info.update(ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE,
            sensorStaticInfo->postRawSensitivityBoost,
            ARRAY_LENGTH(sensorStaticInfo->postRawSensitivityBoost));
    if (ret < 0) {
        CLOGD2("ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE update failed(%d)", ret);
    }

    /* android.edge static attributes */
    if (sensorStaticInfo->edgeModes != NULL) {
        i8Vector.clear();
        i8Vector.appendArray(sensorStaticInfo->edgeModes, sensorStaticInfo->edgeModesLength);
        if ((sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING) ||
            (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING)) {
            i8Vector.add(ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG);
        }
        ret = info.update(ANDROID_EDGE_AVAILABLE_EDGE_MODES, i8Vector.array(), i8Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_EDGE_AVAILABLE_EDGE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("edgeModes at sensorStaticInfo is NULL");
    }

    /* andorid.flash static attributes */
    ret = info.update(ANDROID_FLASH_INFO_AVAILABLE,
            &(sensorStaticInfo->flashAvailable), 1);
    if (ret < 0)
        CLOGD2("ANDROID_FLASH_INFO_AVAILABLE update failed(%d)", ret);

    ret = info.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
            &(sensorStaticInfo->chargeDuration), 1);
    if (ret < 0)
        CLOGD2("ANDROID_FLASH_INFO_CHARGE_DURATION update failed(%d)", ret);

    ret = info.update(ANDROID_FLASH_COLOR_TEMPERATURE,
            &(sensorStaticInfo->colorTemperature), 1);
    if (ret < 0)
        CLOGD2("ANDROID_FLASH_COLOR_TEMPERATURE update failed(%d)", ret);

    ret = info.update(ANDROID_FLASH_MAX_ENERGY,
            &(sensorStaticInfo->maxEnergy), 1);
    if (ret < 0)
        CLOGD2("ANDROID_FLASH_MAX_ENERGY update failed(%d)", ret);

    /* android.hotPixel static attributes */
    if (sensorStaticInfo->hotPixelModes != NULL) {
        ret = info.update(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES,
                sensorStaticInfo->hotPixelModes,
                sensorStaticInfo->hotPixelModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES update failed(%d)", ret);
    } else {
        CLOGD2("hotPixelModes at sensorStaticInfo is NULL");
    }

    /* andorid.jpeg static attributes */
    i32Vector.clear();
    m_createJpegAvailableThumbnailSizes(sensorStaticInfo, &i32Vector);
    ret = info.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, i32Vector.array(), i32Vector.size());
    if (ret < 0)
        CLOGD2("ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES update failed(%d)", ret);

    int32_t jpegMaxSize = 0;

    {
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
        if (camIdInfo->scenario == SCENARIO_DUAL_REAR_ZOOM) {
            int maxPictureW, maxPictureH;
            if (m_getMaxPictureSize(sensorStaticInfo, &maxPictureW, &maxPictureH) == NO_ERROR) {
                jpegMaxSize = maxPictureW * maxPictureH * getJPEGMaxBPPSize();
            } else {
                jpegMaxSize = sensorStaticInfo->maxPictureW * sensorStaticInfo->maxPictureH * getJPEGMaxBPPSize();
            }
        } else
#endif
        {
            jpegMaxSize = sensorStaticInfo->maxPictureW * sensorStaticInfo->maxPictureH * getJPEGMaxBPPSize();
        }
    }

    ret = info.update(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);
    if (ret < 0)
        CLOGD2("ANDROID_JPEG_MAX_SIZE update failed(%d)", ret);


    /* android.lens static attributes */
    if (sensorStaticInfo->availableApertureValues != NULL) {
        ret = info.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                sensorStaticInfo->availableApertureValues, sensorStaticInfo->availableApertureValuesLength);
        if (ret < 0)
            CLOGD2("ANDROID_LENS_INFO_AVAILABLE_APERTURES update failed(%d)", ret);
    } else {
        ret = info.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                &(sensorStaticInfo->fNumber), 1);
        if (ret < 0)
            CLOGD2("ANDROID_LENS_INFO_AVAILABLE_APERTURES update failed(%d)", ret);
    }

    ret = info.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
            &(sensorStaticInfo->filterDensity), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES update failed(%d)", ret);

    ret = info.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            &(sensorStaticInfo->focalLength), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS update failed(%d)", ret);

    if (sensorStaticInfo->opticalStabilization != NULL
        && m_hasTagInList(sensorStaticInfo->requestKeys,
                          sensorStaticInfo->requestKeysLength,
                          ANDROID_LENS_OPTICAL_STABILIZATION_MODE))
    {
        ret = info.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                sensorStaticInfo->opticalStabilization,
                sensorStaticInfo->opticalStabilizationLength);
        if (ret < 0)
            CLOGD2("ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION update failed(%d)",
                    ret);
    } else {
        CLOGD2("opticalStabilization at sensorStaticInfo is NULL");
    }

    ret = info.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
            &(sensorStaticInfo->hyperFocalDistance), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE update failed(%d)", ret);

    ret = info.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
            &(sensorStaticInfo->minimumFocusDistance), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE update failed(%d)", ret);

    ret = info.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE,
            sensorStaticInfo->shadingMapSize,
            ARRAY_LENGTH(sensorStaticInfo->shadingMapSize));
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_SHADING_MAP_SIZE update failed(%d)", ret);

    ret = info.update(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
            &(sensorStaticInfo->focusDistanceCalibration), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION update failed(%d)", ret);

    ret = info.update(ANDROID_LENS_FACING,
            &(sensorStaticInfo->lensFacing), 1);
    if (ret < 0)
        CLOGD2("ANDROID_LENS_FACING update failed(%d)", ret);

    /* android.noiseReduction static attributes */
    if (sensorStaticInfo->noiseReductionModes != NULL) {
        i8Vector.clear();
        i8Vector.appendArray(sensorStaticInfo->noiseReductionModes, sensorStaticInfo->noiseReductionModesLength);
        if ((sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING) ||
            (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING)) {
            i8Vector.add(ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG);
        }
        ret = info.update(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
                        i8Vector.array(), i8Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES update failed(%d)", ret);
    } else {
        CLOGD2("noiseReductionModes at sensorStaticInfo is NULL");
    }

    /* android.request static attributes */
    ret = info.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
            sensorStaticInfo->maxNumOutputStreams,
            ARRAY_LENGTH(sensorStaticInfo->maxNumOutputStreams));
    if (ret < 0)
        CLOGD2("ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS update failed(%d)", ret);

    int num_of_inputs = 0;
    if ((sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING) ||
        (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING)) {
        num_of_inputs = sensorStaticInfo->maxNumInputStreams;
    }

    ret = info.update(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, &num_of_inputs, 1);
    if (ret < 0)
        CLOGD2("ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS update failed(%d)", ret);

    ret = info.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
            &(sensorStaticInfo->maxPipelineDepth), 1);
    if (ret < 0)
        CLOGD2("ANDROID_REQUEST_PIPELINE_MAX_DEPTH update failed(%d)", ret);

    ret = info.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
            &(sensorStaticInfo->partialResultCount), 1);
    if (ret < 0)
        CLOGD2("ANDROID_REQUEST_PARTIAL_RESULT_COUNT update failed(%d)", ret);

    /* android.scaler static attributes */
    const float maxZoom = ((float)sensorStaticInfo->maxZoomRatio) / 1000.0f;
    ret = info.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);
    if (ret < 0) {
        CLOGD2("ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM update failed(%d)", ret);
    }

    i32Vector.clear();
    if (m_createScalerAvailableInputOutputFormatsMap(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        /* Update AvailableInputOutputFormatsMap only if private reprocessing is supported */
        ret = info.update(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP, i32Vector.array(), i32Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP update failed(%d)", ret);
    }

    i32Vector.clear();
    m_createScalerAvailableStreamConfigurationsOutput(sensorStaticInfo, &i32Vector);
    ret = info.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, i32Vector.array(), i32Vector.size());
    if (ret < 0)
        CLOGD2("ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS update failed(%d)", ret);

    i64Vector.clear();
    m_createScalerAvailableMinFrameDurations(sensorStaticInfo, &i64Vector);
    ret = info.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, i64Vector.array(), i64Vector.size());
    if (ret < 0)
        CLOGD2("ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS update failed(%d)", ret);

    if (sensorStaticInfo->stallDurations != NULL) {
        ret = info.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
                sensorStaticInfo->stallDurations,
                sensorStaticInfo->stallDurationsLength);
        if (ret < 0)
            CLOGD2("ANDROID_SCALER_AVAILABLE_STALL_DURATIONS update failed(%d)", ret);
    } else {
        CLOGD2("stallDurations at sensorStaticInfo is NULL");
    }

    ret = info.update(ANDROID_SCALER_CROPPING_TYPE,
            &(sensorStaticInfo->croppingType), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SCALER_CROPPING_TYPE update failed(%d)", ret);

    /* android.sensor static attributes */
    const int32_t kResolution[4] =
    {0, 0, sensorStaticInfo->maxSensorW, sensorStaticInfo->maxSensorH};
    ret = info.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, kResolution, 4);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
            sensorStaticInfo->sensitivityRange,
            ARRAY_LENGTH(sensorStaticInfo->sensitivityRange));
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_SENSITIVITY_RANGE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
            &(sensorStaticInfo->colorFilterArrangement), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            sensorStaticInfo->exposureTimeRange,
            ARRAY_LENGTH(sensorStaticInfo->exposureTimeRange));
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &(sensorStaticInfo->maxFrameDuration), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_MAX_FRAME_DURATION update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            sensorStaticInfo->sensorPhysicalSize,
            ARRAY_LENGTH(sensorStaticInfo->sensorPhysicalSize));
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_PHYSICAL_SIZE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, &(kResolution[2]), 2);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            &(sensorStaticInfo->whiteLevel), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_WHITE_LEVEL update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
            &(sensorStaticInfo->timestampSource), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT1,
            &(sensorStaticInfo->referenceIlluminant1), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_REFERENCE_ILLUMINANT1 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT2,
            &(sensorStaticInfo->referenceIlluminant2), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_REFERENCE_ILLUMINANT2 update failed(%d)", ret);

    int32_t calibrationRG = 1024, calibrationBG = 1024;

    camera_metadata_rational_t calibrationMatrix[9] = {
        {calibrationRG, 1024}, {0, 1024}, {0, 1024},
        {0, 1024}, {1024, 1024}, {0, 1024},
        {0, 1024}, {0, 1024}, {calibrationBG, 1024},
    };

    ret = info.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM1, calibrationMatrix, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_CALIBRATION_TRANSFORM2 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM2, calibrationMatrix, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_CALIBRATION_TRANSFORM2 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_COLOR_TRANSFORM1, sensorStaticInfo->colorTransformMatrix1, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_COLOR_TRANSFORM1 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_COLOR_TRANSFORM2, sensorStaticInfo->colorTransformMatrix2, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_COLOR_TRANSFORM2 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_FORWARD_MATRIX1, sensorStaticInfo->forwardMatrix1, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_FORWARD_MATRIX1 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_FORWARD_MATRIX2, sensorStaticInfo->forwardMatrix2, 9);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_FORWARD_MATRIX2 update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            sensorStaticInfo->blackLevelPattern,
            ARRAY_LENGTH(sensorStaticInfo->blackLevelPattern));
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_BLACK_LEVEL_PATTERN update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
            &(sensorStaticInfo->maxAnalogSensitivity), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_MAX_ANCLOG_SENSITIVITY update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_ORIENTATION,
            &(sensorStaticInfo->orientation), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_ORIENTATION update failed(%d)", ret);

    ret = info.update(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS,
            sensorStaticInfo->profileHueSatMapDimensions,
            ARRAY_LENGTH(sensorStaticInfo->profileHueSatMapDimensions));
    if (ret < 0)
        CLOGD2("ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS update failed(%d)", ret);

    if (sensorStaticInfo->testPatternModes != NULL) {
        ret = info.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
                sensorStaticInfo->testPatternModes,
                sensorStaticInfo->testPatternModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES update failed(%d)", ret);
    } else {
        CLOGD2("testPatternModes at sensorStaticInfo is NULL");
    }

    /* android.statistics static attributes */
    if (sensorStaticInfo->faceDetectModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
                sensorStaticInfo->faceDetectModes,
                sensorStaticInfo->faceDetectModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES update failed(%d)", ret);
    } else {
        CLOGD2("faceDetectModes at sensorStaticInfo is NULL");
    }

    ret = info.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
            &(sensorStaticInfo->histogramBucketCount), 1);
    if (ret < 0)
        CLOGD2("ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT update failed(%d)", ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            &sensorStaticInfo->maxNumDetectedFaces, 1);
    if (ret < 0)
        CLOGD2("ANDROID_STATISTICS_INFO_MAX_FACE_COUNT update failed(%d)", ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &sensorStaticInfo->maxHistogramCount, 1);
    if (ret < 0)
        CLOGD2("ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT update failed(%d)", ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &(sensorStaticInfo->maxSharpnessMapValue), 1);
    if (ret < 0)
        CLOGD2("ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE update failed(%d)", ret);

    ret = info.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sensorStaticInfo->sharpnessMapSize,
            ARRAY_LENGTH(sensorStaticInfo->sharpnessMapSize));
    if (ret < 0)
        CLOGD2("ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE update failed(%d)", ret);

    if (sensorStaticInfo->hotPixelMapModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
                sensorStaticInfo->hotPixelMapModes,
                sensorStaticInfo->hotPixelMapModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES update failed(%d)", ret);
    } else {
        CLOGD2("hotPixelMapModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->lensShadingMapModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
                sensorStaticInfo->lensShadingMapModes,
                sensorStaticInfo->lensShadingMapModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES update failed(%d)", ret);
    } else {
        CLOGD2("lensShadingMapModes at sensorStaticInfo is NULL");
    }

    if (sensorStaticInfo->shadingAvailableModes != NULL) {
        ret = info.update(ANDROID_SHADING_AVAILABLE_MODES,
                sensorStaticInfo->shadingAvailableModes,
                sensorStaticInfo->shadingAvailableModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_SHADING_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("shadingAvailableModes at sensorStaticInfo is NULL");
    }

    /* andorid.tonemap static attributes */
    ret = info.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
            &(sensorStaticInfo->tonemapCurvePoints), 1);
    if (ret < 0)
        CLOGD2("ANDROID_TONEMAP_MAX_CURVE_POINTS update failed(%d)", ret);

    if (sensorStaticInfo->toneMapModes != NULL) {
        ret = info.update(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES,
                sensorStaticInfo->toneMapModes,
                sensorStaticInfo->toneMapModesLength);
        if (ret < 0)
            CLOGD2("ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES update failed(%d)", ret);
    } else {
        CLOGD2("toneMapModes at sensorStaticInfo is NULL");
    }

    /* android.led static attributes */
    if (sensorStaticInfo->leds != NULL) {
        ret = info.update(ANDROID_LED_AVAILABLE_LEDS,
                sensorStaticInfo->leds,
                sensorStaticInfo->ledsLength);
        if (ret < 0)
            CLOGD2("ANDROID_LED_AVAILABLE_LEDS update failed(%d)", ret);
    } else {
        CLOGD2("leds at sensorStaticInfo is NULL");
    }

    /* andorid.info static attributes */
    ret = info.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
            &(sensorStaticInfo->supportedHwLevel), 1);
    if (ret < 0)
        CLOGD2("ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL update failed(%d)", ret);

    /* android.sync static attributes */
    ret = info.update(ANDROID_SYNC_MAX_LATENCY,
            &(sensorStaticInfo->maxLatency), 1);
    if (ret < 0)
        CLOGD2("ANDROID_SYNC_MAX_LATENCY update failed(%d)", ret);

    if ((sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING) ||
        (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING)) {
        /* Set Stall duration for reprocessing */
        ret = info.update(ANDROID_REPROCESS_MAX_CAPTURE_STALL, &(sensorStaticInfo->maxCaptureStall), 1);
        if (ret < 0)
            CLOGD2("ANDROID_REPROCESS_MAX_CAPTURE_STALL update failed(%d)", ret);
    }

    if (isLogicalCamera) {
        uint8_t sensorSyncType = (uint8_t)getLogicalCamSyncType(camIdInfo->serviceCameraId);
        int physID[MAX_NUM_SENSORS] = {-1, };

        i8Vector.clear();
        if(getLogicalCamPhysIDs(camIdInfo->serviceCameraId, physID) == 0) {
            for (int camNum = 0; camNum < camIdInfo->numOfSensors; camNum++) {
                char stringPhysID[10] = {'\0',};
                int stringPhysIDLength = 0;

                CLOGD2("ServiceID(%d), [%d/%d]Physical ID(%d)",
                    camIdInfo->serviceCameraId,
                    camNum,
                    camIdInfo->numOfSensors,
                    physID[camNum]);

                sprintf(stringPhysID, "%d", physID[camNum]);
                stringPhysIDLength = strlen(stringPhysID) + 1;
                for (int i = 0; i < stringPhysIDLength; i++) {
                    i8Vector.add(stringPhysID[i]);
                }
            }

            ret = info.update(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
                              i8Vector.array(), i8Vector.size());
            if (ret < 0) {
                CLOGD2("ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS update failed(%d)", ret);
            }
        } else {
            CLOGD2("physical_ids are not valid");
        }

        ret = info.update(ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE,
            &sensorSyncType, 1);
        if (ret < 0)
            CLOGD2("ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE update failed(%d)", ret);
    }

    /* android.request.availableCapabilities */
    i8Vector.clear();
    if (m_createAvailableCapabilities(sensorStaticInfo, &i8Vector, camIdInfo) == NO_ERROR) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, i8Vector.array(), i8Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_REQUEST_AVAILABLE_CAPABILITIES update failed(%d)", ret);
    }

    /* android.request.availableRequestKeys */
    /* android.request.availableResultKeys */
    /* android.request.availableCharacteristicsKeys */
    Vector<int32_t> requestList, resultList, characteristicsList, session;
    requestList.clear();
    resultList.clear();
    characteristicsList.clear();

    session.clear();
    if (m_createAvailableKeys(sensorStaticInfo, &session, &requestList, &resultList, &characteristicsList, camIdInfo->cameraId[camIdx]) == NO_ERROR) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_SESSION_KEYS,
                session.array(), session.size());
        if (ret < 0) {
            CLOGD2("ANDROID_REQUEST_AVAILABLE_SESSION_KEYS update failed(%d)", ret);
        }

        ret = info.update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
                            requestList.array(), requestList.size());
        if (ret < 0)
            CLOGD2("ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS update failed(%d)", ret);

        ret = info.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
                            resultList.array(), resultList.size());
        if (ret < 0)
            CLOGD2("ANDROID_REQUEST_AVAILABLE_RESULT_KEYS update failed(%d)", ret);

        ret = info.update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
                            characteristicsList.array(), characteristicsList.size());
        if (ret < 0)
            CLOGD2("ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS update failed(%d)", ret);
    }

    if (isLogicalCamera) {
        /*
         * Currently Logical Camera REQUEST_KEYS and Physical Camera REQUEST_KEYS are same.
         * TODO: exclude if any KEY is not supported for Physical Stream.
         */

        ret = info.update(ANDROID_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS,
                                    requestList.array(), requestList.size());
        if (ret < 0)
            CLOGD2("ANDROID_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS update failed(%d)", ret);

    }

#if 0
    if (sensorStaticInfo->depthOnlySensor) {
        ret = info.update(ANDROID_DEPTH_MAX_DEPTH_SAMPLES,
                &(sensorStaticInfo->maxDepthSamples), 1);
        if (ret < 0)
            CLOGD2("ANDROID_DEPTH_MAX_DEPTH_SAMPLES update failed(%d)", ret);

        i32Vector.clear();
        m_createDepthAvailableStreamConfigurations(sensorStaticInfo, &i32Vector);
        ret = info.update(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS, i32Vector.array(), i32Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS update failed(%d)", ret);

        i64Vector.clear();
        m_createDepthAvailableMinFrameDurations(sensorStaticInfo, &i64Vector);
        ret = info.update(ANDROID_DEPTH_AVAILABLE_DEPTH_MIN_FRAME_DURATIONS, i64Vector.array(), i64Vector.size());
        if (ret < 0)
            CLOGD2("ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS update failed(%d)", ret);

        if (sensorStaticInfo->stallDurations != NULL) {
            ret = info.update(ANDROID_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS,
                    sensorStaticInfo->stallDurations,
                    sensorStaticInfo->stallDurationsLength);
            if (ret < 0)
                CLOGD2("ANDROID_DEPTH_AVAILABLE_STALL_DURATIONS update failed(%d)", ret);
        } else {
            CLOGD2("stallDurations at sensorStaticInfo is NULL");
        }

        ret = info.update(ANDROID_DEPTH_DEPTH_IS_EXCLUSIVE, &(sensorStaticInfo->depthIsExclusive), 1);
        if (ret < 0)
            CLOGD2("ANDROID_DEPTH_DEPTH_IS_EXCLUSIVE update failed(%d)", ret);
    }
#endif

    /* Vendor staticInfo*/
    m_constructVendorStaticInfo(sensorStaticInfo, &info, camIdInfo, camIdx);

    if (*cameraInfo != NULL) {
        free_camera_metadata(*cameraInfo);
        *cameraInfo = NULL;
    }

    *cameraInfo = info.release();

    delete sensorStaticInfo;
    return OK;
}

void ExynosCameraMetadataConverter::setStaticInfo(cameraId_Info *camIdInfo, camera_metadata_t *info)
{
    if (info == NULL) {
        camera_metadata_t *meta = NULL;
        CLOGW("info is null");
        ExynosCameraMetadataConverter::constructStaticInfo(camIdInfo, &meta, NULL);
        m_staticInfo = meta;
    } else {
        m_staticInfo = info;
    }
}

void ExynosCameraMetadataConverter::setSessionParams(const camera_metadata_t *info)
{
    /* deep copy */
    if (info) {
        m_sessionParams = info;
        m_setSessionVendorParams();
    } else {
        CLOGD("setSessionParams is null");
        m_sessionParams.clear();
    }

    return;
}

status_t ExynosCameraMetadataConverter::initShotData(struct camera2_shot_ext *shot_ext,
                                                        int32_t physCamID)
{
    CLOGV("IN");

    struct ExynosCameraSensorInfoBase *sensorStaticInfo;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    memset(shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    struct camera2_shot *shot = &(shot_ext->shot);

    // TODO: make this from default request settings
    /* request */
    shot->ctl.request.id = 0;
    shot->ctl.request.metadataMode = METADATA_MODE_FULL;
    shot->ctl.request.frameCount = 0;

    /* lens */
    shot->ctl.lens.focusDistance = -1.0f;
    shot->ctl.lens.aperture = 0.0f;
    shot->ctl.lens.focalLength = sensorStaticInfo->focalLength;
    shot->ctl.lens.filterDensity = 0.0f;
    shot->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_OFF;

    shot->uctl.lensUd.pos = 0;
    shot->uctl.lensUd.posSize = 0;

    /* aa */
    shot->ctl.aa.vendor_afState = AA_AFSTATE_INACTIVE;
    shot->ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;

    int minFps = sensorStaticInfo->minFps;
    int maxFps = sensorStaticInfo->maxFps;

    /* The min fps can not be '0'. Therefore it is set up default value '15'. */
    if (minFps == 0) {
        CLOGW("Invalid min fps value(%d)", minFps);
        minFps = 7;
    }
    /*  The initial fps can not be '0' and bigger than '30'. Therefore it is set up default value '30'. */
    if (maxFps == 0 || 30 < maxFps) {
        CLOGW("Invalid max fps value(%d)", maxFps);
        maxFps = 30;
    }

    m_maxFps = maxFps;

    /* sensor */
    shot->ctl.sensor.exposureTime = 0;
    shot->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / maxFps;
    shot->ctl.sensor.sensitivity = 0;

    /* flash */
    shot->ctl.flash.flashMode = CAM2_FLASH_MODE_OFF;
    shot->ctl.flash.firingPower = 0;
    shot->ctl.flash.firingTime = 0;
    shot->uctl.flashMode = CAMERA_FLASH_MODE_OFF;
    m_overrideFlashControl = false;

    /* hotpixel */
    shot->ctl.hotpixel.mode = (enum processing_mode)0;

    /* demosaic */
    shot->ctl.demosaic.mode = (enum demosaic_processing_mode)0;

    /* noise */
    shot->ctl.noise.mode = ::PROCESSING_MODE_FAST;
    shot->ctl.noise.strength = 5;

    /* shading */
    shot->ctl.shading.mode = (enum processing_mode)0;

    /* color */
    shot->ctl.color.mode = COLORCORRECTION_MODE_FAST;
    static const camera_metadata_rational_t colorTransform[9] = {
        {1, 1}, {0, 1}, {0, 1},
        {0, 1}, {1, 1}, {0, 1},
        {0, 1}, {0, 1}, {1, 1},
    };
    memcpy(shot->ctl.color.transform, colorTransform, sizeof(shot->ctl.color.transform));
    shot->ctl.color.vendor_saturation = 3; /* "3" is default. */

    /* tonemap */
    shot->ctl.tonemap.mode = ::TONEMAP_MODE_FAST;
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };

    int tonemapCurveSize = sizeof(tonemapCurve);
    int sizeOfCurve = sizeof(shot->ctl.tonemap.curveRed) / sizeof(shot->ctl.tonemap.curveRed[0]);

    for (int i = 0; i < sizeOfCurve; i += 4) {
        memcpy(&(shot->ctl.tonemap.curveRed[i]),   tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveGreen[i]), tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveBlue[i]),  tonemapCurve, tonemapCurveSize);
    }

    /* edge */
    shot->ctl.edge.mode = ::PROCESSING_MODE_FAST;
    shot->ctl.edge.strength = 5;

    /* scaler */
    float zoomRatio = 1.0f;

    int maxSensorW;
    int maxSensorH;
    int maxPreviewW;
    int maxPreviewH;

    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);
    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_PREVIEW_SIZE, (uint32_t *)&maxPreviewW, (uint32_t *)&maxPreviewH);

    if (setMetaCtlCropRegion(shot_ext, 0, maxSensorW, maxSensorH, maxPreviewW, maxPreviewH, zoomRatio) != NO_ERROR) {
        CLOGE("m_setZoom() fail");
    }

    /* jpeg */
    shot->ctl.jpeg.quality = 96;
    shot->ctl.jpeg.thumbnailSize[0] = sensorStaticInfo->maxThumbnailW;
    shot->ctl.jpeg.thumbnailSize[1] = sensorStaticInfo->maxThumbnailH;
    shot->ctl.jpeg.thumbnailQuality = 100;
    shot->ctl.jpeg.gpsCoordinates[0] = 0;
    shot->ctl.jpeg.gpsCoordinates[1] = 0;
    shot->ctl.jpeg.gpsCoordinates[2] = 0;
    memset(&shot->ctl.jpeg.gpsProcessingMethod, 0x0,
        sizeof(shot->ctl.jpeg.gpsProcessingMethod));
    shot->ctl.jpeg.gpsTimestamp = 0L;
    shot->ctl.jpeg.orientation = 0L;

    /* stats */
    shot->ctl.stats.faceDetectMode = ::FACEDETECT_MODE_OFF;
    shot->ctl.stats.histogramMode = ::STATS_MODE_OFF;
    shot->ctl.stats.sharpnessMapMode = ::STATS_MODE_OFF;

    /* aa */
    shot->ctl.aa.captureIntent = ::AA_CAPTURE_INTENT_CUSTOM;
    shot->ctl.aa.mode = ::AA_CONTROL_AUTO;
    shot->ctl.aa.effectMode = ::AA_EFFECT_OFF;
    shot->ctl.aa.sceneMode = ::AA_SCENE_MODE_FACE_PRIORITY;
    shot->ctl.aa.videoStabilizationMode = VIDEO_STABILIZATION_MODE_OFF;

    /* default metering is center */
    shot->ctl.aa.aeMode = ::AA_AEMODE_CENTER;
    shot->ctl.aa.aeRegions[0] = 0;
    shot->ctl.aa.aeRegions[1] = 0;
    shot->ctl.aa.aeRegions[2] = 0;
    shot->ctl.aa.aeRegions[3] = 0;
    shot->ctl.aa.aeRegions[4] = 1000;
    shot->ctl.aa.aeExpCompensation = 0; /* 0 is middle */
    shot->ctl.aa.vendor_aeExpCompensationStep = sensorStaticInfo->exposureCompensationStep;
    shot->ctl.aa.aeLock = ::AA_AE_LOCK_OFF;

    shot->ctl.aa.aeTargetFpsRange[0] = minFps;
    shot->ctl.aa.aeTargetFpsRange[1] = maxFps;

    shot->ctl.aa.aeAntibandingMode = ::AA_AE_ANTIBANDING_AUTO;
    shot->ctl.aa.vendor_aeflashMode = ::AA_FLASHMODE_OFF;

    shot->ctl.aa.awbMode = ::AA_AWBMODE_WB_AUTO;
    shot->ctl.aa.awbLock = ::AA_AWB_LOCK_OFF;
    shot->ctl.aa.afMode = ::AA_AFMODE_OFF;
    shot->ctl.aa.afRegions[0] = 0;
    shot->ctl.aa.afRegions[1] = 0;
    shot->ctl.aa.afRegions[2] = 0;
    shot->ctl.aa.afRegions[3] = 0;
    shot->ctl.aa.afRegions[4] = 1000;
    shot->ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;

    shot->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
    shot->ctl.aa.vendor_isoValue = 0;
    shot->ctl.aa.vendor_videoMode = AA_VIDEOMODE_OFF;

    shot->uctl.opMode = CAMERA_OP_MODE_HAL3_GED;

    shot->uctl.cameraMode = AA_CAMERAMODE_SINGLE;
    if (isBackCamera(m_cameraId)) {
        shot->uctl.masterCamera = AA_SENSORPLACE_REAR;
    } else {
        shot->uctl.masterCamera = AA_SENSORPLACE_FRONT;
    }

    /* udm */

    /* magicNumber */
    shot->magicNumber = SHOT_MAGIC_NUMBER;

    for (int i = 0; i < INTERFACE_TYPE_MAX; i++) {
        shot->uctl.scalerUd.mcsc_sub_blk_port[i] = MCSC_PORT_NONE;
    }

    /* default setfile index */
    setMetaSetfile(shot_ext, ISS_SUB_SCENARIO_STILL_PREVIEW);

    /* user request */
#if 0
    shot_ext->tnr_mode = 0;
#endif
    shot_ext->fd_bypass  = 1;

    initShotVendorData(shot);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateColorControlData(CameraMetadata *settings,
                                                                struct camera2_shot_ext *dst_ext,
                                                                CameraMetadata *prevMeta,
                                                                int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_COLOR_CORRECTION_MODE */
    entry = settings->find(ANDROID_COLOR_CORRECTION_MODE);
    if (entry.count > 0) {
        dst->ctl.color.mode = (enum colorcorrection_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_COLOR_CORRECTION_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_COLOR_CORRECTION_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_COLOR_CORRECTION_TRANSFORM */
    entry = settings->find(ANDROID_COLOR_CORRECTION_TRANSFORM);
    if (entry.count > 0) {
        for (size_t i = 0; i < entry.count && i < 9; i++) {
            /* Convert rational to float */
            dst->ctl.color.transform[i].num = entry.data.r[i].numerator;
            dst->ctl.color.transform[i].den = entry.data.r[i].denominator;
        }
    }

    /* ANDROID_COLOR_CORRECTION_GAINS */
    entry = settings->find(ANDROID_COLOR_CORRECTION_GAINS);
    if (entry.count > 0) {
        for (size_t i = 0; i < entry.count && i < 4; i++) {
            dst->ctl.color.gains[i] = entry.data.f[i];
        }
        CLOGV("ANDROID_COLOR_CORRECTION_GAINS(%f,%f,%f,%f)",
                entry.data.f[0], entry.data.f[1], entry.data.f[2], entry.data.f[3]);
    }

    /* ANDROID_COLOR_CORRECTION_ABERRATION_MODE */
    entry = settings->find(ANDROID_COLOR_CORRECTION_ABERRATION_MODE);
    if (entry.count > 0) {
        dst->ctl.color.aberrationCorrectionMode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_COLOR_CORRECTION_ABERRATION_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_COLOR_CORRECTION_ABERRATION_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateControlControlData(ExynosCameraRequestSP_sprt_t request,
                                                                     CameraMetadata *settings,
                                                                     struct camera2_shot_ext *dst_ext,
                                                                     struct CameraMetaParameters *metaParameters,
                                                                     CameraMetadata *prevMeta,
                                                                     int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    uint32_t vendorAeMode = 0;
    uint32_t vendorAfMode = 0;
    ExynosCameraParameters *parameters;
    ExynosCameraActivityControl *activityControl;
    ExynosCameraActivityFlash *flashMgr;
    int32_t respectFlashMode = 0;
    int32_t cameraId = physCamID;

    if (cameraId < 0)
        cameraId = m_cameraId;

#ifdef USE_DUAL_CAMERA
    // HACK: need to get exact cameraId, not appoximate cameraId
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
            m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
        int masterCamId, slaveCamId;
        // get the masterCameraId of reprocessing
        m_configurations->getDualCamId(masterCamId, slaveCamId, true);
        if (masterCamId >= 0) cameraId = masterCamId;
    }
#endif

    parameters = m_parameters[cameraId];
    activityControl = parameters->getActivityControl();
    flashMgr = activityControl->getFlashMgr();
    if (flashMgr == NULL) {
        CLOGE("FlashMgr is NULL!!");
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    translatePreVendorControlControlData(settings, dst_ext, prevMeta);

    /* ANDROID_CONTROL_AE_ANTIBANDING_MODE */
    entry = settings->find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) FIMC_IS_METADATA(entry.data.u8[0]);

#if 1
        CLOGV("ANDROID_CONTROL_AE_ANTIBANDING_MODE(%d) aeAntibandingMode(%d)",
            entry.data.u8[0], dst->ctl.aa.aeAntibandingMode);
#else
        if (dst->ctl.aa.aeAntibandingMode != AA_AE_ANTIBANDING_OFF) {
            dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) m_defaultAntibanding;
        }
        CLOGV("ANDROID_CONTROL_AE_ANTIBANDING_MODE(%d) m_defaultAntibanding(%d)",
            entry.data.u8[0], m_defaultAntibanding);
#endif
    }

    /* ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION */
    entry = settings->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
    if (entry.count > 0) {
        dst->ctl.aa.aeExpCompensation = (int32_t) (entry.data.i32[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        if (prev_entry.count > 0) {
            prev_value = (int32_t) (prev_entry.data.i32[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.aeExpCompensation) {
            CLOGD("ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION(%d)",
                dst->ctl.aa.aeExpCompensation);
        }
        isMetaExist = false;
    }

    /*
     * ANDROID_FLASH_MODE:
     * The control is effective when aeMode is ON or OFF.
     * This control will be overridden in case of ON_AUTO_FLASH, ON_ALWAYS_FLASH, or ON_AUTO_FLASH_REDEYE
     * When FLASH_MODE: OFF => the camera device will not fire flash for this capture.
     * When FLASH_MODE: SINGLE => fire flash regardless of the camera device's auto-exposure routine's result.
     * When FLASH_MODE: TORCH => flash will be on continuously.
     * TORCH mode can be used for use cases such as preview, auto-focus assist, still capture, or video recording.
     */

    entry = settings->find(ANDROID_FLASH_MODE);
    if (entry.count > 0) {
        enum flash_mode flashMode = (enum flash_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        camera_metadata_entry_t entry_temp;
        enum aa_aemode aeMode = AA_AEMODE_OFF;

        switch (flashMode) {
        case CAM2_FLASH_MODE_OFF:
        case CAM2_FLASH_MODE_SINGLE:
        case CAM2_FLASH_MODE_TORCH:
            respectFlashMode = 1;
            break;
        default:
            break;
        }

        /* ANDROID_CONTROL_AE_MODE */
        entry_temp = settings->find(ANDROID_CONTROL_AE_MODE);
        if (entry_temp.count > 0) {
            aeMode = (enum aa_aemode) FIMC_IS_METADATA(entry_temp.data.u8[0]);
            switch (aeMode) {
            case AA_AEMODE_ON_AUTO_FLASH_REDEYE:
            case AA_AEMODE_ON_AUTO_FLASH:
            case AA_AEMODE_ON_ALWAYS_FLASH:
                respectFlashMode = 0;
                break;
            default:
                break;
            }
            CLOGV("FLASH_DEBUG: [R%d] aeMode (%d)", request->getKey(), aeMode);
        }

        if (respectFlashMode) {
            enum ExynosCameraActivityFlash::FLASH_REQ flashReq = ExynosCameraActivityFlash::FLASH_REQ_END;
            dst->ctl.flash.flashMode = flashMode;
            switch (flashMode) {
            case CAM2_FLASH_MODE_OFF:
                metaParameters->m_flashMode = FLASH_MODE_OFF;
                flashReq = ExynosCameraActivityFlash::FLASH_REQ_OFF;
                break;
            break;
            case CAM2_FLASH_MODE_SINGLE:
                metaParameters->m_flashMode = FLASH_MODE_ON;
                dst->uctl.flashMode = CAMERA_FLASH_MODE_ON;
                m_configurations->setModeValue(CONFIGURATION_FLASH_MODE, FLASH_MODE_ON);
                flashReq = ExynosCameraActivityFlash::FLASH_REQ_SINGLE;
                break;
            case CAM2_FLASH_MODE_TORCH:
                metaParameters->m_flashMode = FLASH_MODE_TORCH;
                dst->uctl.flashMode = CAMERA_FLASH_MODE_TORCH;
                m_configurations->setModeValue(CONFIGURATION_FLASH_MODE, FLASH_MODE_TORCH);
                flashReq = ExynosCameraActivityFlash::FLASH_REQ_TORCH;
                m_overrideFlashControl = true;
                break;
            default:
                break;
            }

            entry_temp = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
            if (entry_temp.count > 0) {
                if ((enum aa_capture_intent) entry_temp.data.u8[0] == AA_CAPTURE_INTENT_STILL_CAPTURE
                    && flashMode >= CAM2_FLASH_MODE_SINGLE) {
                    m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 1);
                }
            }

            if (flashReq != ExynosCameraActivityFlash::FLASH_REQ_END) {
                CLOGV("FLASH_DEBUG: [R%d] flashReq(%d) m_overrideFlashControl (%d)", request->getKey(), flashReq, m_overrideFlashControl);
                flashMgr->setFlashReq(flashReq, m_overrideFlashControl);
            }
        } else {
            dst->ctl.flash.flashMode = CAM2_FLASH_MODE_OFF;
        }

        CLOGV("FLASH_DEBUG: [R%d] ANDROID_FLASH_MODE(%d), flashMode (%d)", request->getKey(), entry.data.u8[0], dst->ctl.flash.flashMode);

    }

    /* ANDROID_CONTROL_AE_MODE */
    entry = settings->find(ANDROID_CONTROL_AE_MODE);
    if (entry.count > 0) {
        enum aa_aemode aeMode = AA_AEMODE_OFF;
        enum ExynosCameraActivityFlash::FLASH_REQ flashReq = ExynosCameraActivityFlash::FLASH_REQ_OFF;

        vendorAeMode = entry.data.u8[0];
        aeMode = (enum aa_aemode) FIMC_IS_METADATA(entry.data.u8[0]);

        dst->ctl.aa.aeMode = aeMode;

        switch (aeMode) {
        case AA_AEMODE_ON_AUTO_FLASH_REDEYE:
            metaParameters->m_flashMode = FLASH_MODE_RED_EYE;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_RED_EYE;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON_AUTO_FLASH:
            metaParameters->m_flashMode = FLASH_MODE_AUTO;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_AUTO;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON_ALWAYS_FLASH:
            metaParameters->m_flashMode = FLASH_MODE_ON;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_ON;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_ON;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON:
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
        case AA_AEMODE_OFF:
        default:
            m_overrideFlashControl = false;
            if (!respectFlashMode) {
                dst->uctl.flashMode = CAMERA_FLASH_MODE_OFF;
                metaParameters->m_flashMode = FLASH_MODE_OFF;
            }
            break;
        }

        flashMgr->setFlashExposure(aeMode);
        if (!respectFlashMode) {
            CLOGV("flashReq(%d)", flashReq);
            flashMgr->setFlashReq(flashReq, m_overrideFlashControl);
        }

        /* ANDROID_CONTROL_AE_MODE */
        entry = m_prevMeta->find(ANDROID_CONTROL_AE_MODE);
        if (entry.count > 0) {
            prev_value = entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != (int32_t)vendorAeMode) {
            CLOGD("[R%d] ANDROID_CONTROL_AE_MODE(%d)",
                    request->getKey(),
                    (int32_t)vendorAeMode);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_LOCK */
    entry = settings->find(ANDROID_CONTROL_AE_LOCK);
    if (entry.count > 0) {
        dst->ctl.aa.aeLock = (enum aa_ae_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_LOCK);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AE_LOCK(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_REGIONS */
    entry = settings->find(ANDROID_CONTROL_AE_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 aeRegion;

        aeRegion.x1 = entry.data.i32[0];
        aeRegion.y1 = entry.data.i32[1];
        aeRegion.x2 = entry.data.i32[2];
        aeRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.aeRegions[4] = entry.data.i32[4];

        m_convertActiveArrayTo3AARegion(&aeRegion, "AE");

        dst->ctl.aa.aeRegions[0] = aeRegion.x1;
        dst->ctl.aa.aeRegions[1] = aeRegion.y1;
        dst->ctl.aa.aeRegions[2] = aeRegion.x2;
        dst->ctl.aa.aeRegions[3] = aeRegion.y2;
        CLOGV("ANDROID_CONTROL_AE_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);

        // If AE region has meaningful value, AE region can be applied to the output image
        if (entry.data.i32[0] && entry.data.i32[1] && entry.data.i32[2] && entry.data.i32[3]) {
            if (dst->ctl.aa.aeMode == AA_AEMODE_CENTER) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER_TOUCH;
            } else if (dst->ctl.aa.aeMode == AA_AEMODE_MATRIX) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH;
            } else if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_SPOT_TOUCH;
            }
            CLOGV("update AA_AEMODE(%d)", dst->ctl.aa.aeMode);
        }
    }

    /* ANDROID_CONTROL_AWB_REGIONS */
    /* AWB region value would not be used at the f/w,
    because AWB is not related with a specific region */
    entry = settings->find(ANDROID_CONTROL_AWB_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 awbRegion;

        awbRegion.x1 = entry.data.i32[0];
        awbRegion.y1 = entry.data.i32[1];
        awbRegion.x2 = entry.data.i32[2];
        awbRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.awbRegions[4] = entry.data.i32[4];
        m_convertActiveArrayTo3AARegion(&awbRegion, "AWB");

        dst->ctl.aa.awbRegions[0] = awbRegion.x1;
        dst->ctl.aa.awbRegions[1] = awbRegion.y1;
        dst->ctl.aa.awbRegions[2] = awbRegion.x2;
        dst->ctl.aa.awbRegions[3] = awbRegion.y2;
        CLOGV("ANDROID_CONTROL_AWB_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    /* ANDROID_CONTROL_AF_REGIONS */
    entry = settings->find(ANDROID_CONTROL_AF_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 afRegion;

        afRegion.x1 = entry.data.i32[0];
        afRegion.y1 = entry.data.i32[1];
        afRegion.x2 = entry.data.i32[2];
        afRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.afRegions[4] = entry.data.i32[4];
        m_convertActiveArrayTo3AARegion(&afRegion, "AF");

        dst->ctl.aa.afRegions[0] = afRegion.x1;
        dst->ctl.aa.afRegions[1] = afRegion.y1;
        dst->ctl.aa.afRegions[2] = afRegion.x2;
        dst->ctl.aa.afRegions[3] = afRegion.y2;
        CLOGV("ANDROID_CONTROL_AF_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    /* ANDROID_CONTROL_AE_TARGET_FPS_RANGE */
    entry = settings->find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
    if (entry.count > 0) {
        int32_t prev_fps[2] = {0, };
        uint32_t max_fps, min_fps;

        m_configurations->checkPreviewFpsRange(entry.data.i32[0], entry.data.i32[1]);
        m_configurations->getPreviewFpsRange(&min_fps, &max_fps);
        dst->ctl.aa.aeTargetFpsRange[0] = min_fps;
        dst->ctl.aa.aeTargetFpsRange[1] = max_fps;
        m_maxFps = dst->ctl.aa.aeTargetFpsRange[1];

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        if (prev_entry.count > 0) {
            prev_fps[0] = prev_entry.data.i32[0];
            prev_fps[1] = prev_entry.data.i32[1];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_fps[0] != (int32_t)dst->ctl.aa.aeTargetFpsRange[0] ||
            prev_fps[1] != (int32_t)dst->ctl.aa.aeTargetFpsRange[1]) {
            CLOGD("ANDROID_CONTROL_AE_TARGET_FPS_RANGE(%d-%d)",
                dst->ctl.aa.aeTargetFpsRange[0], dst->ctl.aa.aeTargetFpsRange[1]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER */
    entry = settings->find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
    if (entry.count > 0) {
        dst->ctl.aa.aePrecaptureTrigger = (enum aa_ae_precapture_trigger) entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_ae_precapture_trigger) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.aePrecaptureTrigger) {
            CLOGD("[R%d] ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER(%d)",
                request->getKey(),
                dst->ctl.aa.aePrecaptureTrigger);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AF_MODE */
    entry = settings->find(ANDROID_CONTROL_AF_MODE);
    if (entry.count > 0) {
        int faceDetectionMode = FACEDETECT_MODE_OFF;
        camera_metadata_entry_t fd_entry;

        fd_entry = settings->find(ANDROID_STATISTICS_FACE_DETECT_MODE);
        if (fd_entry.count > 0) {
            faceDetectionMode = (enum facedetect_mode) FIMC_IS_METADATA(fd_entry.data.u8[0]);
        }

        vendorAfMode = entry.data.u8[0];
        dst->ctl.aa.afMode = (enum aa_afmode) FIMC_IS_METADATA(entry.data.u8[0]);

        switch (dst->ctl.aa.afMode) {
        case AA_AFMODE_AUTO:
            if (faceDetectionMode > FACEDETECT_MODE_OFF) {
                dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            } else {
                dst->ctl.aa.vendor_afmode_option = 0x00;
            }
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_MACRO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_MACRO);
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_CONTINUOUS_VIDEO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_CONTINUOUS_PICTURE:
            if (faceDetectionMode > FACEDETECT_MODE_OFF) {
                dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            } else {
                dst->ctl.aa.vendor_afmode_option = 0x00;
            }
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_OFF:
        default:
            {
                dst->ctl.aa.vendor_afmode_option = 0x00;
                dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            }
            break;
        }

        m_preAfMode = m_afMode;
        m_afMode = dst->ctl.aa.afMode;

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AF_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AF_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AF_TRIGGER */
    entry = settings->find(ANDROID_CONTROL_AF_TRIGGER);
    if (entry.count > 0) {
        dst->ctl.aa.afTrigger = (enum aa_af_trigger)entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AF_TRIGGER);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AF_TRIGGER(%d)", entry.data.u8[0]);
#ifdef USE_DUAL_CAMERA
            if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
                m_configurations->setDualOperationModeLockCount(10);
            }
#endif
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_LOCK */
    entry = settings->find(ANDROID_CONTROL_AWB_LOCK);
    if (entry.count > 0) {
        dst->ctl.aa.awbLock = (enum aa_awb_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_LOCK);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AWB_LOCK(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_MODE */
    entry = settings->find(ANDROID_CONTROL_AWB_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.awbMode = (enum aa_awbmode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AWB_MODE(%d)", (int32_t)entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_EFFECT_MODE */
    entry = settings->find(ANDROID_CONTROL_EFFECT_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.effectMode = (enum aa_effect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_EFFECT_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_EFFECT_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_MODE */
    entry = settings->find(ANDROID_CONTROL_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.mode = (enum aa_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_MODE(%d)", (int32_t)entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_MODE check must be prior to ANDROID_CONTROL_SCENE_MODE check */
    /* ANDROID_CONTROL_SCENE_MODE */
    entry = settings->find(ANDROID_CONTROL_SCENE_MODE);
    if (entry.count > 0) {
        uint8_t scene_mode;

        scene_mode = (uint8_t)entry.data.u8[0];
        m_sceneMode = scene_mode;
        setSceneMode(scene_mode, dst_ext);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_SCENE_MODE);
        if (prev_entry.count > 0) {
            prev_value = (uint8_t)prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (m_configurations->getRestartStream() == false) {
            if (isMetaExist && prev_value != scene_mode) {
                if (prev_value == ANDROID_CONTROL_SCENE_MODE_HDR
                    || scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
                    m_configurations->setRestartStream(true);
                    CLOGD("setRestartStream(SCENE_MODE_HDR)");
                }
            }
        }

        if (!isMetaExist || prev_value != scene_mode) {
            CLOGD("ANDROID_CONTROL_SCENE_MODE(%d)", scene_mode);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_VIDEO_STABILIZATION_MODE */
    entry = settings->find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
    if (entry.count > 0) {
        bool changeMetadata = false;
        dst->ctl.aa.videoStabilizationMode = (enum aa_videostabilization_mode) entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_videostabilization_mode) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.videoStabilizationMode) {
            CLOGD("ANDROID_CONTROL_VIDEO_STABILIZATION_MODE(%d)",
                dst->ctl.aa.videoStabilizationMode);
            changeMetadata = true;
        }

#if defined(USE_SW_VDIS_CONTROL_REQUEST) && (USE_SW_VDIS_CONTROL_REQUEST == true)
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        vendorMeta->update(entry.tag, entry.data.u8, entry.count);

        if (changeMetadata && m_configurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE) == true) {
            uint32_t max_fps = 0, min_fps = 0;
            m_configurations->getPreviewFpsRange(&min_fps, &max_fps);
            if (isMetaExist) {
                if (m_configurations->getRestartStream() == false) {
                    m_configurations->setRestartStream(true);
                    CLOGD("setRestartStream(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE)");
                }

                m_configurations->setMode(CONFIGURATION_RESTART_FORCE_FLUSH, true);
                m_configurations->setModeValue(CONFIGURATION_RECORDING_FPS, max_fps);
                m_configurations->setModeValue(CONFIGURATION_RESTART_VIDEO_STABILIZATION, dst->ctl.aa.videoStabilizationMode);
            } else {
                m_configurations->setModeValue(CONFIGURATION_RECORDING_FPS, max_fps);
                m_configurations->setModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE, dst->ctl.aa.videoStabilizationMode);
            }
            CLOGD("ANDROID_CONTROL_VIDEO_STABILIZATION_MODE CONFIGURATION_VIDEO_STABILIZATION_ENABLE(%d) fps(%d)", dst->ctl.aa.videoStabilizationMode, max_fps);
        }
#endif
        isMetaExist = false;
    }

    enum ExynosCameraActivityFlash::FLASH_STEP flashStep = ExynosCameraActivityFlash::FLASH_STEP_OFF;
    enum ExynosCameraActivityFlash::FLASH_STEP curFlashStep = ExynosCameraActivityFlash::FLASH_STEP_END;
    bool isFlashStepChanged = false;

    flashMgr->getFlashStep(&curFlashStep);

    /* Check Precapture Trigger to turn on the pre-flash */
    switch (dst->ctl.aa.aePrecaptureTrigger) {
    case AA_AE_PRECAPTURE_TRIGGER_START:
        if (flashMgr->getNeedCaptureFlash() == true
            && (flashMgr->getFlashStatus() == AA_FLASHMODE_OFF || curFlashStep == ExynosCameraActivityFlash::FLASH_STEP_OFF)) {
            {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_START;
            }
            flashMgr->setCaptureStatus(true);
            isFlashStepChanged = true;
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_CANCEL:
        if (flashMgr->getNeedCaptureFlash() == true
            // only change the flash step to cancel if current step is not OFF
            && (curFlashStep != ExynosCameraActivityFlash::FLASH_STEP_OFF)
            && (flashMgr->getFlashStatus() != AA_FLASHMODE_OFF
                    || curFlashStep != ExynosCameraActivityFlash::FLASH_STEP_OFF)
            && (flashMgr->getFlashStatus() != AA_FLASHMODE_CANCEL
                    || curFlashStep != ExynosCameraActivityFlash::FLASH_STEP_CANCEL)) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_CANCEL;
            flashMgr->setCaptureStatus(false);
            isFlashStepChanged = true;
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_IDLE:
    default:
        break;
    }
    /* Check Capture Intent to turn on the main-flash */

    /* ANDROID_CONTROL_CAPTURE_INTENT */
    entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
    if (entry.count > 0) {
        dst->ctl.aa.captureIntent = (enum aa_capture_intent) entry.data.u8[0];
#ifdef SUPPORT_LIMITED_HW_LEVEL_FALSH
        // [HACK] In the case of limited hw level device, since still capture intent continuously comes about 30,
        // so change to use still capture intent only when callback request is came
        if (dst->ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE) {
            int reqNum = m_configurations->getModeValue(CONFIGURATION_CALLBACK_REQUEST_KEY_VALUE);
            CLOGV("[R%d] reqNum (%d)", request->getKey(), reqNum);
            if (reqNum != request->getKey()) {
                dst->ctl.aa.captureIntent = AA_CAPTURE_INTENT_PREVIEW;
            } else {
                dst->ctl.aa.captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE;
                m_configurations->setModeValue(CONFIGURATION_CALLBACK_REQUEST_KEY_VALUE, 0);
                CLOGD("[R%d] ANDROID_CONTROL_CAPTURE_INTENT(%d)",
                        request->getKey(), dst->ctl.aa.captureIntent);
            }
        }
#else
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_CAPTURE_INTENT);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_capture_intent) prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.captureIntent) {
            CLOGD("[R%d] ANDROID_CONTROL_CAPTURE_INTENT(%d)",
                request->getKey(), dst->ctl.aa.captureIntent);
        }
        isMetaExist = false;
#endif
    }

    switch (dst->ctl.aa.captureIntent) {
    case AA_CAPTURE_INTENT_STILL_CAPTURE:
        // in case of zsl, skip this flash control
        if (request->getNumOfInputBuffer() > 0)
            break;

        if (flashMgr->getNeedCaptureFlash() == true) {
            flashMgr->getFlashStep(&curFlashStep);
            if (curFlashStep == ExynosCameraActivityFlash::FLASH_STEP_OFF ||
                metaParameters->m_flashMode == FLASH_MODE_OFF) {
                CLOGD("[R%d] curFlashStep(%d) status(%d) metaParameters->m_flashMode(%d)", request->getKey(), curFlashStep,
                        flashMgr->getFlashStatus(), metaParameters->m_flashMode);
                flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
                flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_OFF);
                m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 0);
            } else {
                {
                    CLOGD("[R%d] FLASH_STEP_MAIN_START", request->getKey());
                    flashStep = ExynosCameraActivityFlash::FLASH_STEP_MAIN_START;
                }

                isFlashStepChanged = true;
                m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 1);
            }
        } else {
            m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 0);
        }
        break;
    case AA_CAPTURE_INTENT_VIDEO_RECORD:
    case AA_CAPTURE_INTENT_CUSTOM:
    case AA_CAPTURE_INTENT_PREVIEW:
    case AA_CAPTURE_INTENT_VIDEO_SNAPSHOT:
    case AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
    case AA_CAPTURE_INTENT_MANUAL:
    default:
        break;
    }

    if (isFlashStepChanged == true && flashMgr != NULL)
        flashMgr->setFlashStep(flashStep);

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)) {
        dst->ctl.aa.vendor_videoMode = AA_VIDEOMODE_ON;
    }

    /* If aeMode or Mode is NOT Off, Manual AE control can NOT be operated */
    if (dst->ctl.aa.aeMode == AA_AEMODE_OFF
        || dst->ctl.aa.mode == AA_CONTROL_OFF) {
        m_isManualAeControl = true;
        CLOGV("Operate Manual AE Control, aeMode(%d), Mode(%d)",
                dst->ctl.aa.aeMode, dst->ctl.aa.mode);
    } else {
        m_isManualAeControl = false;
    }
    m_configurations->setMode(CONFIGURATION_MANUAL_AE_CONTROL_MODE, m_isManualAeControl);

    translateVendorControlControlData(request, settings, dst_ext, prevMeta, physCamID);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateDemosaicControlData(CameraMetadata *settings,
                                                        struct camera2_shot_ext *dst_ext,
                                                        CameraMetadata *prevMeta,
                                                        int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_DEMOSAIC_MODE */
    entry = settings->find(ANDROID_DEMOSAIC_MODE);
    if (entry.count > 0) {
        dst->ctl.demosaic.mode = (enum demosaic_processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_DEMOSAIC_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_DEMOSAIC_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateEdgeControlData(CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    CameraMetadata *prevMeta,
                                                                    int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_EDGE_STRENGTH */
    entry = settings->find(ANDROID_EDGE_STRENGTH);
    if (entry.count > 0) {
        dst->ctl.edge.strength = (uint32_t) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_EDGE_STRENGTH);
        if (prev_entry.count > 0 ) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.edge.strength) {
            CLOGD("ANDROID_EDGE_STRENGTH(%d)", dst->ctl.edge.strength);
        }
        isMetaExist = false;
    }

    /* ANDROID_EDGE_MODE */
    entry = settings->find(ANDROID_EDGE_MODE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case ANDROID_EDGE_MODE_OFF:
            dst->ctl.edge.mode = PROCESSING_MODE_OFF;
            break;
        case ANDROID_EDGE_MODE_FAST:
            dst->ctl.edge.mode = PROCESSING_MODE_FAST;
            break;
        case ANDROID_EDGE_MODE_HIGH_QUALITY:
            dst->ctl.edge.mode = PROCESSING_MODE_HIGH_QUALITY;
            break;
        case ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG:
            dst->ctl.edge.mode = PROCESSING_MODE_ZERO_SHUTTER_LAG;
            break;
        default:
            CLOGW("Invalid ANDROID_EDGE_MODE(%d). so, just set ANDROID_EDGE_MODE_OFF", entry.data.u8[0]);
            dst->ctl.edge.mode = PROCESSING_MODE_OFF;
            break;
        }

        prev_entry = prevMeta->find(ANDROID_EDGE_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_EDGE_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
        switch (entry.data.u8[0]) {
        case ANDROID_EDGE_MODE_FAST:
        case ANDROID_EDGE_MODE_HIGH_QUALITY:
            dst->ctl.edge.strength = 5;
            break;
        case ANDROID_EDGE_MODE_OFF:
        case ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG:
            dst->ctl.edge.strength = 2;
            break;
        default:
            break;
        }
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateFlashControlData(CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    CameraMetadata *prevMeta,
                                                                    int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    uint64_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (m_overrideFlashControl == true) {
        return OK;
    }

    /* ANDROID_FLASH_FIRING_POWER */
    entry = settings->find(ANDROID_FLASH_FIRING_POWER);
    if (entry.count > 0) {
        dst->ctl.flash.firingPower = (uint32_t) entry.data.u8[0];

        prev_entry = prevMeta->find(ANDROID_FLASH_FIRING_POWER);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.flash.firingPower) {
            CLOGD("ANDROID_FLASH_FIRING_POWER(%d)", dst->ctl.flash.firingPower);
        }
        isMetaExist = false;
    }

    /* ANDROID_FLASH_FIRING_TIME */
    entry = settings->find(ANDROID_FLASH_FIRING_TIME);
    if (entry.count > 0 ) {
        dst->ctl.flash.firingTime = (uint64_t) entry.data.i64[0];
        prev_entry = prevMeta->find(ANDROID_FLASH_FIRING_TIME);
        if (prev_entry.count > 0) {
            prev_value = (uint64_t) (prev_entry.data.i64[0]);
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != dst->ctl.flash.firingTime) {
            CLOGD("ANDROID_FLASH_FIRING_TIME(%ju)", dst->ctl.flash.firingTime);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateHotPixelControlData(CameraMetadata *settings,
                                                                            struct camera2_shot_ext *dst_ext,
                                                                            CameraMetadata *prevMeta,
                                                                            int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_HOT_PIXEL_MODE */
    entry = settings->find(ANDROID_HOT_PIXEL_MODE);
    if (entry.count > 0) {
        dst->ctl.hotpixel.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_HOT_PIXEL_MODE);
        if (prev_entry.count > 0) {
            prev_value = entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_HOT_PIXEL_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateJpegControlData(CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    CameraMetadata *prevMeta,
                                                                    int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    uint64_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_JPEG_GPS_COORDINATES */
    entry = settings->find(ANDROID_JPEG_GPS_COORDINATES);
    if (entry.count > 0) {
        for (size_t i = 0; i < entry.count && i < 3; i++)
            dst->ctl.jpeg.gpsCoordinates[i] = entry.data.d[i];
        CLOGV("ANDROID_JPEG_GPS_COORDINATES(%f,%f,%f)",
                entry.data.d[0], entry.data.d[1], entry.data.d[2]);
    }

    /* ANDROID_JPEG_GPS_PROCESSING_METHOD */
    entry = settings->find(ANDROID_JPEG_GPS_PROCESSING_METHOD);
    if (entry.count > 0) {
        size_t len = entry.count;

        if (len > GPS_PROCESSING_METHOD_SIZE) {
            len = GPS_PROCESSING_METHOD_SIZE;
        }
        strncpy((char *)dst->ctl.jpeg.gpsProcessingMethod, (char *)entry.data.u8, len);
        CLOGV("ANDROID_JPEG_GPS_PROCESSING_METHOD(%s)",
                dst->ctl.jpeg.gpsProcessingMethod);
    }

    /* ANDROID_JPEG_GPS_TIMESTAMP */
    entry = settings->find(ANDROID_JPEG_GPS_TIMESTAMP);
    if (entry.count > 0) {
        dst->ctl.jpeg.gpsTimestamp = (uint64_t) entry.data.i64[0];
        prev_entry = prevMeta->find(ANDROID_JPEG_GPS_TIMESTAMP);
        if (prev_entry.count > 0) {
            prev_value = (uint64_t) (prev_entry.data.i64[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.jpeg.gpsTimestamp) {
            CLOGD("ANDROID_JPEG_GPS_TIMESTAMP(%ju)", dst->ctl.jpeg.gpsTimestamp);
        }
        isMetaExist = false;
    }

    /* ANDROID_JPEG_ORIENTATION */
    entry = settings->find(ANDROID_JPEG_ORIENTATION);
    if (entry.count > 0) {
        dst->ctl.jpeg.orientation = (uint32_t) entry.data.i32[0];
        prev_entry = prevMeta->find(ANDROID_JPEG_ORIENTATION);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.i32[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.jpeg.orientation) {
            CLOGD("ANDROID_JPEG_ORIENTATION(%d)", dst->ctl.jpeg.orientation);
        }
        isMetaExist = false;
    }

    /* ANDROID_JPEG_QUALITY */
    entry = settings->find(ANDROID_JPEG_QUALITY);
    if (entry.count > 0) {
        dst->ctl.jpeg.quality = (uint32_t) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_JPEG_QUALITY);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.jpeg.quality) {
            CLOGD("ANDROID_JPEG_QUALITY(%d)", dst->ctl.jpeg.quality);
        }
        isMetaExist = false;
    }

    /* ANDROID_JPEG_THUMBNAIL_QUALITY */
    entry = settings->find(ANDROID_JPEG_THUMBNAIL_QUALITY);
    if (entry.count > 0) {
        dst->ctl.jpeg.thumbnailQuality = (uint32_t) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_JPEG_THUMBNAIL_QUALITY);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.jpeg.thumbnailQuality) {
            CLOGD("ANDROID_JPEG_THUMBNAIL_QUALITY(%d)", dst->ctl.jpeg.thumbnailQuality);
        }
        isMetaExist = false;
    }

    /* ANDROID_JPEG_THUMBNAIL_SIZE */
    entry = settings->find(ANDROID_JPEG_THUMBNAIL_SIZE);
    if (entry.count > 0) {
        for (size_t i = 0; i < entry.count && i < 2; i++) {
            dst->ctl.jpeg.thumbnailSize[i] = (uint32_t) entry.data.i32[i];
        }

        prev_entry = prevMeta->find(ANDROID_JPEG_THUMBNAIL_SIZE);
        if (prev_entry.count > 0) {
           isMetaExist = true;
        }

        if (!isMetaExist || (prev_entry.data.i32[0] != entry.data.i32[0]) ||
            (prev_entry.data.i32[1] != entry.data.i32[1])) {
            CLOGD("ANDROID_JPEG_THUMBNAIL_SIZE(%d, %d)", entry.data.i32[0], entry.data.i32[1]);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateLensControlData(CameraMetadata *settings,
                                                                  struct camera2_shot_ext *dst_ext,
                                                                  struct CameraMetaParameters *metaParameters,
                                                                  CameraMetadata *prevMeta,
                                                                  int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    float prev_value_f;
    bool isMetaExist = false;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_LENS_APERTURE */
    if (m_isManualAeControl == true) {
        entry = settings->find(ANDROID_LENS_APERTURE);
        if (entry.count > 0) {
            dst->ctl.lens.aperture = (int32_t)(entry.data.f[0] * 100);
            prev_entry = prevMeta->find(ANDROID_LENS_APERTURE);
            if (prev_entry.count > 0) {
                prev_value_f = (int32_t)(prev_entry.data.f[0] * 100);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value_f != (int32_t)(entry.data.f[0] * 100)) {
                CLOGD("ANDROID_LENS_APERTURE(%d)", (int32_t)(entry.data.f[0] * 100));
            }
            isMetaExist = false;
        }
    } else {
        dst->ctl.lens.aperture = 0;
    }

    /* ANDROID_LENS_FILTER_DENSITY */
    entry = settings->find(ANDROID_LENS_FILTER_DENSITY);
    if (entry.count > 0) {
        dst->ctl.lens.filterDensity = entry.data.f[0];
        prev_entry = prevMeta->find(ANDROID_LENS_FILTER_DENSITY);
        if (prev_entry.count > 0) {
            prev_value_f = prev_entry.data.f[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value_f != dst->ctl.lens.filterDensity) {
            CLOGD("ANDROID_LENS_FILTER_DENSITY(%f)", dst->ctl.lens.filterDensity);
        }
        isMetaExist = false;
    }

    /* ANDROID_LENS_FOCAL_LENGTH */
    entry = settings->find(ANDROID_LENS_FOCAL_LENGTH);
    if (entry.count > 0) {
        dst->ctl.lens.focalLength = entry.data.f[0];
        prev_entry = prevMeta->find(ANDROID_LENS_FOCAL_LENGTH);
        if (prev_entry.count > 0) {
            prev_value_f = prev_entry.data.f[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value_f != dst->ctl.lens.focalLength) {
            CLOGD("ANDROID_LENS_FOCAL_LENGTH(%f)", dst->ctl.lens.focalLength);
        }
        isMetaExist = false;
    }

    /* ANDROID_LENS_FOCUS_DISTANCE */
    entry = settings->find(ANDROID_LENS_FOCUS_DISTANCE);
    if (entry.count > 0) {
        if (m_afMode != AA_AFMODE_OFF || m_afMode != m_preAfMode || m_focusDistance == entry.data.f[0]) {
            dst->ctl.lens.focusDistance = -1;
        } else {
            if (entry.data.f[0] <= sensorStaticInfo->minimumFocusDistance) {
                dst->ctl.lens.focusDistance = entry.data.f[0];
            } else {
                dst->ctl.lens.focusDistance = sensorStaticInfo->minimumFocusDistance;
            }
        }
        m_focusDistance = dst->ctl.lens.focusDistance;
        prev_entry = prevMeta->find(ANDROID_LENS_FOCUS_DISTANCE);
        if (prev_entry.count > 0) {
            prev_value_f = prev_entry.data.f[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value_f != entry.data.f[0]) {
            CLOGD("ANDROID_LENS_FOCUS_DISTANCE(%f)", entry.data.f[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_LENS_OPTICAL_STABILIZATION_MODE */
    entry = settings->find(ANDROID_LENS_OPTICAL_STABILIZATION_MODE);
    if (entry.count > 0) {
        uint8_t ois_mode = (uint8_t)entry.data.u8[0];

        /* Force OIS disable when EIS enable*/
        entry = settings->find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
        if ((entry.count > 0) && (entry.data.u8[0] == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON)) {
            ois_mode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
            CLOGV("Force OIS disable when EIS enable ANDROID_LENS_OPTICAL_STABILIZATION_MODE(%d)", ois_mode);
        }

        prev_entry = prevMeta->find(ANDROID_LENS_OPTICAL_STABILIZATION_MODE);
        if (prev_entry.count > 0) {
            prev_value = (uint8_t)prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != ois_mode) {
            CLOGV("ANDROID_LENS_OPTICAL_STABILIZATION_MODE(%d)", ois_mode);
        }
        isMetaExist = false;

        /* OIS disable */
        char ois_prop[PROPERTY_VALUE_MAX];
        memset(ois_prop, 0, sizeof(ois_prop));
        property_get("persist.vendor.camera.ois.disable", ois_prop, "0");
        uint8_t ois_disable = (uint8_t)atoi(ois_prop);
        if(ois_disable) {
            ois_mode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
        }

        switch (ois_mode) {
        case ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON:
            if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)) {
                dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VIDEO;
            } else {
                dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_STILL;
            }
            break;
        case ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF:
        default:
            dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
            break;
        }
    }

    translateVendorLensControlData(settings, dst_ext, metaParameters, prevMeta);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateNoiseControlData(CameraMetadata *settings,
                                                            struct camera2_shot_ext *dst_ext,
                                                            CameraMetadata *prevMeta,
                                                            int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_NOISE_REDUCTION_STRENGTH */
    entry = settings->find(ANDROID_NOISE_REDUCTION_STRENGTH);
    if (entry.count > 0) {
        dst->ctl.noise.strength = (uint32_t) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_NOISE_REDUCTION_STRENGTH);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.noise.strength) {
            CLOGD("ANDROID_NOISE_REDUCTION_STRENGTH(%d)", dst->ctl.noise.strength);
        }
        isMetaExist = false;
    }

    /* ANDROID_NOISE_REDUCTION_MODE */
    entry = settings->find(ANDROID_NOISE_REDUCTION_MODE);
    if (entry.count > 0) {
        dst->ctl.noise.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_NOISE_REDUCTION_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_NOISE_REDUCTION_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;

        switch (entry.data.u8[0]) {
        case ANDROID_NOISE_REDUCTION_MODE_FAST:
        case ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY:
            dst->ctl.noise.strength = 5;
            break;
        case ANDROID_NOISE_REDUCTION_MODE_OFF:
        case ANDROID_NOISE_REDUCTION_MODE_MINIMAL:
        case ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG:
            dst->ctl.noise.strength = 2;
            break;
        default:
            break;
        }
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateRequestControlData(CameraMetadata *settings,
                                            struct camera2_shot_ext *dst_ext,
                                            int *reqId,
                                            CameraMetadata *prevMeta,
                                            int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    uint32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_REQUEST_ID */
    entry = settings->find(ANDROID_REQUEST_ID);
    if (entry.count > 0) {
        dst->ctl.request.id = (uint32_t) entry.data.i32[0];
        prev_entry = prevMeta->find(ANDROID_REQUEST_ID);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.i32[0]);
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != dst->ctl.request.id) {
            CLOGD("ANDROID_REQUEST_ID(%d)", dst->ctl.request.id);
        }
        isMetaExist = false;

        if (reqId != NULL)
            *reqId = dst->ctl.request.id;
    }

    /* ANDROID_REQUEST_METADATA_MODE */
    entry = settings->find(ANDROID_REQUEST_METADATA_MODE);
    if (entry.count > 0) {
        dst->ctl.request.metadataMode = (enum metadata_mode) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_REQUEST_METADATA_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum metadata_mode) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.request.metadataMode) {
            CLOGD("ANDROID_REQUEST_METADATA_MODE(%d)", dst->ctl.request.metadataMode);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateScalerControlData(CameraMetadata *settings,
                                                                       struct camera2_shot_ext *dst_ext,
                                                                       struct CameraMetaParameters *metaParameters,
                                                                       CameraMetadata *prevMeta,
                                                                       int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int maxSensorW  = 0, maxSensorH  = 0;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;
    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);

    /* ANDROID_SCALER_CROP_REGION */
    entry = settings->find(ANDROID_SCALER_CROP_REGION);
    if (entry.count > 0) {
        float newZoomRatio = 0.0f;
        float maxZoomRatio = m_parameters[m_cameraId]->getMaxZoomRatio() / 1000.0f;

        for (size_t i = 0; i < entry.count && i < 4; i++) {
            dst->ctl.scaler.cropRegion[i] = (uint32_t) entry.data.i32[i];
        }

        newZoomRatio = (float)(maxSensorW) / (float)(dst->ctl.scaler.cropRegion[2]);

        prev_entry = prevMeta->find(ANDROID_SCALER_CROP_REGION);
        if (prev_entry.count > 0) {
            float prevZoomRatio = (float)(maxSensorW) / (float)(prev_entry.data.i32[2]);
            float lastPreviewZoomRatio = (m_lastPreviewCropRegion[2] == 0 ? \
                                        newZoomRatio : ((float)(maxSensorW) / (float)(m_lastPreviewCropRegion[2])));
            aa_capture_intent captureIntent = AA_CAPTURE_INTENT_PREVIEW;

            camera_metadata_entry_t capture_entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
            if (capture_entry.count > 0) {
                captureIntent = (enum aa_capture_intent)capture_entry.data.u8[0];
            }

            metaParameters->m_prevZoomRatio = prevZoomRatio;

            if (captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE
                && m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
                if (lastPreviewZoomRatio != newZoomRatio) {
                    /* Using previous crop region */
                    CLOGD("Stay lastPreviewCropRegion(%f/%f). CaptureIntent(%d)",
                            lastPreviewZoomRatio, newZoomRatio, capture_entry.data.u8[0]);

                    for (size_t i = 0; i < entry.count && i < 4; i++) {
                        dst->ctl.scaler.cropRegion[i] = m_lastPreviewCropRegion[i];
                    }
                    newZoomRatio = lastPreviewZoomRatio;
                }
            } else {
                if (prevZoomRatio != newZoomRatio) {
                    CLOGD("ANDROID_SCALER_CROP_REGION(%d,%d,%d,%d), RATIO(%f , %f)",
                            entry.data.i32[0], entry.data.i32[1],
                            entry.data.i32[2], entry.data.i32[3],
                            newZoomRatio, prevZoomRatio);
                    if (newZoomRatio - prevZoomRatio >= 1.0f && newZoomRatio < 2.1f) {
                        m_rectUiSkipCount = 15;
                        CLOGD("detect 2x Button.");
                    } else if (prevZoomRatio - newZoomRatio >= 1.0f && prevZoomRatio < 2.1f) {
                        m_rectUiSkipCount = 15;
                        CLOGD("detect 1x Button.");
                    } else {
                        m_rectUiSkipCount = 2;
                    }
                } else {
                    if (m_rectUiSkipCount > 0) {
                        m_rectUiSkipCount--;
                    }
                }
            }
        } else {
            m_rectUiSkipCount = 8;
        }

        if (newZoomRatio > maxZoomRatio) {
            int cropRegionMinW = 0, cropRegionMinH = 0;

            cropRegionMinW = ceil((float)maxSensorW / maxZoomRatio);
            cropRegionMinH = ceil((float)maxSensorH / maxZoomRatio);
            dst->ctl.scaler.cropRegion[2] = cropRegionMinW;
            dst->ctl.scaler.cropRegion[3] = cropRegionMinH;
            newZoomRatio = maxZoomRatio;

            CLOGW("invalid CROP_REGION(%d,%d,%d,%d), change CROP_REGION(%d,%d,%d,%d)",
                    entry.data.i32[0], entry.data.i32[1],
                    entry.data.i32[2], entry.data.i32[3],
                    dst->ctl.scaler.cropRegion[0],
                    dst->ctl.scaler.cropRegion[1],
                    dst->ctl.scaler.cropRegion[2],
                    dst->ctl.scaler.cropRegion[3]);
            CLOGW("MaxSensorSize(%dx%d)newZoomRatio(%f)MaxZoom Ratio(%f)",
                    maxSensorW, maxSensorH, newZoomRatio, maxZoomRatio);
        }
        CLOGV("MaxSensorSize(%dx%d) newZoomRatio(%f)",
                maxSensorW, maxSensorH, newZoomRatio);
        /* set ZoomRatio */
        metaParameters->m_zoomRatio = newZoomRatio;

        /* set Zoom Rect */
        metaParameters->m_zoomRect.x = m_lastPreviewCropRegion[0] = dst->ctl.scaler.cropRegion[0];
        metaParameters->m_zoomRect.y = m_lastPreviewCropRegion[1] = dst->ctl.scaler.cropRegion[1];
        metaParameters->m_zoomRect.w = m_lastPreviewCropRegion[2] = dst->ctl.scaler.cropRegion[2];
        metaParameters->m_zoomRect.h = m_lastPreviewCropRegion[3] = dst->ctl.scaler.cropRegion[3];
    }else {
        /* set Default Zoom Ratio */
        metaParameters->m_zoomRatio = 1.0f;

        /* set Default Zoom Rect */
        metaParameters->m_zoomRect.x = 0;
        metaParameters->m_zoomRect.y = 0;
        metaParameters->m_zoomRect.w = maxSensorW;
        metaParameters->m_zoomRect.h = maxSensorH;
    }

    dst->uctl.zoomRatio = metaParameters->m_zoomRatio;

    translateVendorScalerControlData(settings, dst_ext, prevMeta, metaParameters);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateSensorControlData(ExynosCameraRequestSP_sprt_t request,
                                                                CameraMetadata *settings,
                                                                struct camera2_shot_ext *dst_ext,
                                                                CameraMetadata *prevMeta,
                                                                int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    uint64_t prev_value64;
    uint32_t prev_value32;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_SENSOR_EXPOSURE_TIME */
    if (m_isManualAeControl == true) {
        entry = settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
        if (entry.count > 0) {
            dst->ctl.sensor.exposureTime = (uint64_t) entry.data.i64[0];
            prev_entry = prevMeta->find(ANDROID_SENSOR_EXPOSURE_TIME);
            if (prev_entry.count > 0) {
                prev_value64 = (uint64_t) (prev_entry.data.i64[0]);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value64 != (uint64_t) entry.data.i64[0]) {
                CLOGD("ANDROID_SENSOR_EXPOSURE_TIME(%ju)",
                    (uint64_t) entry.data.i64[0]);
            }
            isMetaExist = false;
        }
    }

    /* ANDROID_SENSOR_FRAME_DURATION */
    if (m_isManualAeControl == true) {
        entry = settings->find(ANDROID_SENSOR_FRAME_DURATION);
        if (entry.count > 0) {
            camera_metadata_entry_t exposure_entry;
            uint64_t frameDuration = 0L;
            uint64_t exposureTime = 0L;
            frameDuration = (uint64_t) entry.data.i64[0];

            exposure_entry = settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
            if (exposure_entry.count > 0) {
                exposureTime = (uint64_t) exposure_entry.data.i64[0];
            }
            if (frameDuration == 0L || frameDuration < exposureTime) {
                frameDuration = exposureTime;
            }

            dst->ctl.sensor.frameDuration = frameDuration;
            prev_entry = prevMeta->find(ANDROID_SENSOR_FRAME_DURATION);
            if (prev_entry.count > 0) {
                prev_value64 = (uint64_t) (prev_entry.data.i64[0]);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value64 != (uint64_t) entry.data.i64[0]) {
                CLOGD("ANDROID_SENSOR_FRAME_DURATION(%ju)",
                    (uint64_t) entry.data.i64[0]);
            }
            isMetaExist = false;
        } else {
            /* default value */
            dst->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / m_maxFps;
        }
    } else {
        /* default value */
        dst->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / m_maxFps;
    }

    /* ANDROID_SENSOR_SENSITIVITY */
    if (m_isManualAeControl == true) {
        entry = settings->find(ANDROID_SENSOR_SENSITIVITY);
        if (entry.count > 0) {
            dst->ctl.aa.vendor_isoMode = AA_ISOMODE_MANUAL;
            dst->ctl.sensor.sensitivity = (uint32_t) entry.data.i32[0];
            dst->ctl.aa.vendor_isoValue = (uint32_t) entry.data.i32[0];
            prev_entry = prevMeta->find(ANDROID_SENSOR_SENSITIVITY);
            if (prev_entry.count > 0) {
                prev_value32 = (uint32_t) (prev_entry.data.i32[0]);
                isMetaExist = true;
            }
            if (!isMetaExist || prev_value32 != (uint32_t) entry.data.i32[0]) {
                CLOGD("ANDROID_SENSOR_SENSITIVITY(%d)", (uint32_t) entry.data.i32[0]);
            }
            isMetaExist = false;
        } else {
            dst->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
            dst->ctl.sensor.sensitivity = 0;
            dst->ctl.aa.vendor_isoValue = 0;
        }
    } else {
        dst->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        dst->ctl.sensor.sensitivity = 0;
        dst->ctl.aa.vendor_isoValue = 0;
    }

    /* ANDROID_SENSOR_TEST_PATTERN_DATA */
    entry = settings->find(ANDROID_SENSOR_TEST_PATTERN_DATA);
    if (entry.count > 0) {
        for (size_t i = 0; i < entry.count && i < 4; i++)
            dst->ctl.sensor.testPatternData[i] = entry.data.i32[i];
        CLOGV("ANDROID_SENSOR_TEST_PATTERN_DATA(%d,%d,%d,%d)",
                entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3]);
    }

    /* ANDROID_SENSOR_TEST_PATTERN_MODE */
    entry = settings->find(ANDROID_SENSOR_TEST_PATTERN_MODE);
    if (entry.count > 0) {
        /* TODO : change SENSOR_TEST_PATTERN_MODE_CUSTOM1 from 256 to 267 */
        if (entry.data.i32[0] == ANDROID_SENSOR_TEST_PATTERN_MODE_CUSTOM1)
            dst->ctl.sensor.testPatternMode = SENSOR_TEST_PATTERN_MODE_CUSTOM1;
        else
            dst->ctl.sensor.testPatternMode = (enum sensor_test_pattern_mode) FIMC_IS_METADATA(entry.data.i32[0]);

        prev_entry = prevMeta->find(ANDROID_SENSOR_TEST_PATTERN_MODE);
        if (prev_entry.count > 0) {
            prev_value32 = (uint32_t)prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value32 != (uint32_t)entry.data.i32[0]) {
            CLOGD("ANDROID_SENSOR_TEST_PATTERN_MODE(%d)", entry.data.i32[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_SENSOR_TIMESTAMP : Get sensor result meta for ZSL_INPUT */
    entry = settings->find(ANDROID_SENSOR_TIMESTAMP);
    if (entry.count > 0) {
        dst->udm.sensor.timeStampBoot = (uint64_t)entry.data.i64[0];
        int32_t mapIndex = m_getFrameInfoIndexForTimeStamp(dst->udm.sensor.timeStampBoot);
        if (mapIndex >= 0) {
            dst->dm.request.frameCount = m_frameCountMap[mapIndex][FRAMECOUNT];
            dst->dm.lens.aperture = m_frameCountMap[mapIndex][APERTURE];
        }
        CLOGD("ANDROID_SENSOR_TIMESTAMP(%ju)",
                dst->udm.sensor.timeStampBoot);
        CLOGD("dst->dm.request.frameCount(%d), dst->dm.lens.aperture(%d)",
                dst->dm.request.frameCount, dst->dm.lens.aperture);
    }

    /* ANDROID_SENSOR_EXPOSURE_TIME */
    entry = settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
    if (entry.count > 0) {
        dst->dm.sensor.exposureTime = (uint64_t)entry.data.i64[0];
        CLOGV("ANDROID_SENSOR_EXPOSURE_TIME(%ju)",
                dst->dm.sensor.exposureTime);
    }

    /* ANDROID_SENSOR_SENSITIVITY */
    entry = settings->find(ANDROID_SENSOR_SENSITIVITY);
    if (entry.count > 0) {
        dst->dm.sensor.sensitivity = entry.data.i32[0];
        CLOGV("ANDROID_SENSOR_SENSITIVITY(%d)",
                dst->dm.sensor.sensitivity);
    }

    translateVendorSensorControlData(request, settings, dst_ext, prevMeta);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateShadingControlData(CameraMetadata *settings,
                                                            struct camera2_shot_ext *dst_ext,
                                                            CameraMetadata *prevMeta,
                                                            int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_SHADING_MODE */
    entry = settings->find(ANDROID_SHADING_MODE);
    if (entry.count > 0) {
        dst->ctl.shading.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_SHADING_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_SHADING_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_SHADING_STRENGTH */
    entry = settings->find(ANDROID_SHADING_STRENGTH);
    if (entry.count > 0) {
        dst->ctl.shading.strength = (uint32_t) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_SHADING_STRENGTH);
        if (prev_entry.count > 0) {
            prev_value = (uint32_t) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.shading.strength) {
            CLOGD("ANDROID_SHADING_STRENGTH(%d)",
                dst->ctl.shading.strength);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateStatisticsControlData(CameraMetadata *settings,
                                                    struct camera2_shot_ext *dst_ext,
                                                    CameraMetadata *prevMeta,
                                                    int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_STATISTICS_FACE_DETECT_MODE */
    entry = settings->find(ANDROID_STATISTICS_FACE_DETECT_MODE);
    if (entry.count > 0) {
        dst->ctl.stats.faceDetectMode = (enum facedetect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        dst_ext->fd_bypass = (dst->ctl.stats.faceDetectMode == FACEDETECT_MODE_OFF) ? 1 : 0;

        if (m_parameters[m_cameraId]->getHfdMode() == true
            && m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == false
            ) {
            dst_ext->hfd.hfd_enable = !(dst_ext->fd_bypass);
        }

        prev_entry = prevMeta->find(ANDROID_STATISTICS_FACE_DETECT_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_STATISTICS_FACE_DETECT_MODE(%d)", entry.data.u8[0]);
        }

#ifdef USE_ALWAYS_FD_ON
        if (m_configurations->getMode(CONFIGURATION_ALWAYS_FD_ON_MODE) == true
            && dst->ctl.stats.faceDetectMode == FACEDETECT_MODE_OFF) {
            dst->ctl.stats.faceDetectMode = ALWAYS_FD_MODE;
        }
#endif
        isMetaExist = false;
    }

    /* ANDROID_STATISTICS_HISTOGRAM_MODE */
    entry = settings->find(ANDROID_STATISTICS_HISTOGRAM_MODE);
    if (entry.count > 0) {
        dst->ctl.stats.histogramMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_STATISTICS_HISTOGRAM_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_STATISTICS_HISTOGRAM_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_STATISTICS_SHARPNESS_MAP_MODE */
    entry = settings->find(ANDROID_STATISTICS_SHARPNESS_MAP_MODE);
    if (entry.count > 0) {
        dst->ctl.stats.sharpnessMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_STATISTICS_SHARPNESS_MAP_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_STATISTICS_SHARPNESS_MAP_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE */
    entry = settings->find(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE);
    if (entry.count > 0) {
        dst->ctl.stats.hotPixelMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_STATISTICS_LENS_SHADING_MAP_MODE */
    entry = settings->find(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE);
    if (entry.count > 0) {
        dst->ctl.stats.lensShadingMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_STATISTICS_LENS_SHADING_MAP_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateTonemapControlData(CameraMetadata *settings,
                                                            struct camera2_shot_ext *dst_ext,
                                                            CameraMetadata *prevMeta,
                                                            int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_TONEMAP_MODE */
    entry = settings->find(ANDROID_TONEMAP_MODE);
    if (entry.count > 0) {
        dst->ctl.tonemap.mode = (enum tonemap_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = prevMeta->find(ANDROID_TONEMAP_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_TONEMAP_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    if(dst->ctl.tonemap.mode == TONEMAP_MODE_CONTRAST_CURVE) {
        /* ANDROID_TONEMAP_CURVE_BLUE */
        entry = settings->find(ANDROID_TONEMAP_CURVE_BLUE);
        if (entry.count > 0) {
            float tonemapCurveBlue[64] = {0, };

            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveBlue[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveBlue[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        CLOGV("ANDROID_TONEMAP_CURVE_BLUE([%zu]:%f)", i, tonemapCurveBlue[i]);
                    }
                    tonemapCurveBlue[62] = entry.data.f[2];
                    tonemapCurveBlue[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveBlue[2*i] = entry.data.f[i];
                        tonemapCurveBlue[2*i+1] = entry.data.f[i+1];
                        tonemapCurveBlue[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveBlue[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveBlue[2*i] = entry.data.f[i];
                    tonemapCurveBlue[2*i+1] = entry.data.f[i+1];
                    tonemapCurveBlue[2*i+2] = entry.data.f[i];
                    tonemapCurveBlue[2*i+3] = entry.data.f[i+1];
                } else {
                    CLOGE("ANDROID_TONEMAP_CURVE_BLUE( entry count : %zu)", entry.count);
                }
            } else if(entry.count == 128) {
                for (size_t i = 0; i < entry.count; i += 4) {
                    tonemapCurveBlue[i / 2] = entry.data.f[i];
                    tonemapCurveBlue[(i / 2) + 1] = entry.data.f[i + 1];
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveBlue[i] = entry.data.f[i];
                    CLOGV("ANDROID_TONEMAP_CURVE_BLUE([%zu]:%f)", i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveBlue[0]), tonemapCurveBlue, sizeof(float)*64);
        }

        /* ANDROID_TONEMAP_CURVE_GREEN */
        entry = settings->find(ANDROID_TONEMAP_CURVE_GREEN);
        if (entry.count > 0) {
            float tonemapCurveGreen[64] = {0, };

            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveGreen[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveGreen[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        CLOGV("ANDROID_TONEMAP_CURVE_GREEN([%zu]:%f)", i, tonemapCurveGreen[i]);
                    }
                    tonemapCurveGreen[62] = entry.data.f[2];
                    tonemapCurveGreen[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveGreen[2*i] = entry.data.f[i];
                        tonemapCurveGreen[2*i+1] = entry.data.f[i+1];
                        tonemapCurveGreen[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveGreen[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveGreen[2*i] = entry.data.f[i];
                    tonemapCurveGreen[2*i+1] = entry.data.f[i+1];
                    tonemapCurveGreen[2*i+2] = entry.data.f[i];
                    tonemapCurveGreen[2*i+3] = entry.data.f[i+1];
                } else {
                    CLOGE("ANDROID_TONEMAP_CURVE_GREEN( entry count : %zu)", entry.count);
                }
            } else if(entry.count == 128) {
                for (size_t i = 0; i < entry.count; i += 4) {
                    tonemapCurveGreen[i / 2] = entry.data.f[i];
                    tonemapCurveGreen[(i / 2) + 1] = entry.data.f[i + 1];
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveGreen[i] = entry.data.f[i];
                    CLOGV("ANDROID_TONEMAP_CURVE_GREEN([%zu]:%f)", i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveGreen[0]), tonemapCurveGreen, sizeof(float)*64);
        }

        /* ANDROID_TONEMAP_CURVE_RED */
        entry = settings->find(ANDROID_TONEMAP_CURVE_RED);
        if (entry.count > 0) {
            float tonemapCurveRed[64] = {0, };

            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveRed[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveRed[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        CLOGV("ANDROID_TONEMAP_CURVE_RED([%zu]:%f)", i, tonemapCurveRed[i]);
                    }
                    tonemapCurveRed[62] = entry.data.f[2];
                    tonemapCurveRed[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveRed[2*i] = entry.data.f[i];
                        tonemapCurveRed[2*i+1] = entry.data.f[i+1];
                        tonemapCurveRed[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveRed[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveRed[2*i] = entry.data.f[i];
                    tonemapCurveRed[2*i+1] = entry.data.f[i+1];
                    tonemapCurveRed[2*i+2] = entry.data.f[i];
                    tonemapCurveRed[2*i+3] = entry.data.f[i+1];
                } else {
                    CLOGE("ANDROID_TONEMAP_CURVE_RED( entry count : %zu)", entry.count);
                }
            } else if(entry.count == 128) {
                for (size_t i = 0; i < entry.count; i += 4) {
                    tonemapCurveRed[i / 2] = entry.data.f[i];
                    tonemapCurveRed[(i / 2) + 1] = entry.data.f[i + 1];
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveRed[i] = entry.data.f[i];
                    CLOGV("ANDROID_TONEMAP_CURVE_RED([%zu]:%f)", i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveRed[0]), tonemapCurveRed, sizeof(float)*64);
        }
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateLedControlData(CameraMetadata *settings,
                                        struct camera2_shot_ext *dst_ext,
                                        CameraMetadata *prevMeta,
                                        int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_LED_TRANSMIT */
    if (sensorStaticInfo->leds != NULL) {
        entry = settings->find(ANDROID_LED_TRANSMIT);
        if (entry.count > 0) {
            dst->ctl.led.transmit = (enum led_transmit) entry.data.u8[0];
            prev_entry = prevMeta->find(ANDROID_LED_TRANSMIT);
            if (prev_entry.count > 0) {
                prev_value = (enum led_transmit) (prev_entry.data.u8[0]);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != dst->ctl.led.transmit) {
                CLOGD("ANDROID_LED_TRANSMIT(%d)", dst->ctl.led.transmit);
            }
            isMetaExist = false;
        }
    }

    translateVendorLedControlData(settings, dst_ext, prevMeta);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateBlackLevelControlData(CameraMetadata *settings,
                                        struct camera2_shot_ext *dst_ext,
                                        CameraMetadata *prevMeta,
                                        int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    UNUSED_VARIABLE(physCamID);

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    /* ANDROID_BLACK_LEVEL_LOCK */
    entry = settings->find(ANDROID_BLACK_LEVEL_LOCK);
    if (entry.count > 0) {
        dst->ctl.blacklevel.lock = (enum blacklevel_lock) entry.data.u8[0];
        prev_entry = prevMeta->find(ANDROID_BLACK_LEVEL_LOCK);
        if (prev_entry.count > 0) {
            prev_value = (enum blacklevel_lock) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.blacklevel.lock) {
            CLOGD("ANDROID_BLACK_LEVEL_LOCK(%d)", dst->ctl.blacklevel.lock);
        }
        isMetaExist = false;
    }

    return OK;
}

void ExynosCameraMetadataConverter::setPreviousMeta(CameraMetadata *meta)
{
    m_prevMeta = meta;
}

void ExynosCameraMetadataConverter::setPreviousMetaPhysCam(CameraMetadata *meta, int camID)
{
    if ((camID < CAMERA_ID_MAX) && (camID >= 0))
        m_prevMetaPhysCam[camID] = meta;
}

status_t ExynosCameraMetadataConverter::convertRequestToShot(ExynosCameraRequestSP_sprt_t request,
                        int *reqId, int32_t physCamID)
{
    status_t ret = OK;
    uint32_t errorFlag = 0;
    struct camera2_shot_ext *dst_ext = NULL;
    CameraMetadata *meta;
    CameraMetadata *prevMeta = NULL;
    struct CameraMetaParameters *metaParameters = NULL;
    request->setRequestLock();

    if (physCamID < 0) {
        meta = request->getServiceMeta();
        dst_ext = request->getServiceShot();
        metaParameters = request->getMetaParameters();
        prevMeta = m_prevMeta;
    } else {
#if 0
        meta = request->getServiceMetaPhysCam(physCamID);
        dst_ext = request->getServiceShotPhysCam(physCamID);
        metaParameters = request->getMetaParamPhysCam(physCamID);
        prevMeta = m_prevMetaPhysCam[physCamID];
#endif
    }

    if ((meta == NULL) || meta->isEmpty()) {
        CLOGE("Settings is NULL!!");
        request->setRequestUnlock();
        return BAD_VALUE;
    }

    if (prevMeta == NULL) {
        CLOGE("prevMeta is NULL!!");
        request->setRequestUnlock();
        return BAD_VALUE;
    }

    if (dst_ext == NULL) {
        CLOGE("dst_ext is NULL!!");
        request->setRequestUnlock();
        return BAD_VALUE;
    }

    if (metaParameters == NULL) {
        CLOGE("metaParameters is NULL!!");
        request->setRequestUnlock();
        return BAD_VALUE;
    }

    initShotData(dst_ext, physCamID);

    META_VALIDATE_CHECK(meta);

    /* get previous preview's dm, udm */
    if (m_sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING) {
        /* ANDROID_SENSOR_TIMESTAMP : Get sensor result meta for ZSL_INPUT */
        camera_metadata_entry_t entry;
        entry = meta->find(ANDROID_SENSOR_TIMESTAMP);
        if (entry.count > 0) {
            uint64_t timeStamp = (uint64_t)entry.data.i64[0];
            int32_t mapIndex = m_getFrameInfoIndexForTimeStamp(timeStamp);
            if (mapIndex >= 0) {
                memcpy(&dst_ext->shot.udm, &m_udmMap[mapIndex], sizeof(struct camera2_udm));
                memcpy(&dst_ext->shot.dm, &m_dmMap[mapIndex], sizeof(struct camera2_dm));
                memcpy(&dst_ext->user,    &m_shotExtUserMap[mapIndex], sizeof(struct camera2_shot_ext_user));
                memcpy(&dst_ext->vra_ext, &m_vraExtMap[mapIndex], sizeof(struct vra_ext_meta));
            }
        }
    }

    ret = translateColorControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 0);
    ret = translateControlControlData(request, meta, dst_ext, metaParameters, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 1);
    ret = translateDemosaicControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 2);
    ret = translateEdgeControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 3);
    ret = translateFlashControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 4);
    ret = translateHotPixelControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 5);
    ret = translateJpegControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 6);
    ret = translateScalerControlData(meta, dst_ext, metaParameters, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 7);
    ret = translateLensControlData(meta, dst_ext, metaParameters, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 8);
    ret = translateNoiseControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 9);
    ret = translateRequestControlData(meta, dst_ext, reqId, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 10);
    ret = translateSensorControlData(request, meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 11);
    ret = translateShadingControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 12);
    ret = translateStatisticsControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 13);
    ret = translateTonemapControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 14);
    ret = translateLedControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 15);
    ret = translateBlackLevelControlData(meta, dst_ext, prevMeta, physCamID);
    if (ret != OK)
        errorFlag |= (1 << 16);

    request->setRequestUnlock();

    if (errorFlag != 0) {
        CLOGE("failed to translate Control Data(%d)", errorFlag);
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::translateColorMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    const uint8_t colorMode = (uint8_t) CAMERA_METADATA(src->dm.color.mode);
    settings->update(ANDROID_COLOR_CORRECTION_MODE, &colorMode, 1);
    CLOGV("dm.color.mode(%d)", src->dm.color.mode);

    camera_metadata_rational_t colorTransform[9];
    for (int i = 0; i < 9; i++) {
        colorTransform[i].numerator = (int32_t) src->dm.color.transform[i].num;
        colorTransform[i].denominator = (int32_t) src->dm.color.transform[i].den;
    }
    settings->update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);
    CLOGV("dm.color.transform");

    float colorGains[4];
    for (int i = 0; i < 4; i++) {
        colorGains[i] = src->dm.color.gains[i];
    }
    settings->update(ANDROID_COLOR_CORRECTION_GAINS, colorGains, 4);
    CLOGV("dm.color.gains(%f,%f,%f,%f)",
            colorGains[0], colorGains[1], colorGains[2], colorGains[3]);

    const uint8_t aberrationMode = (uint8_t) CAMERA_METADATA(src->dm.color.aberrationCorrectionMode);
    settings->update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE, &aberrationMode, 1);
    CLOGV("dm.color.aberrationCorrectionMode(%d)",
            src->dm.color.aberrationCorrectionMode);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateControlMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                        CameraMetadata *settings, struct camera2_shot_ext *shot_ext,
                                        int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

	const uint8_t afSceneChange = 0; // src->dm.aa.afSceneChange;
    settings->update(ANDROID_CONTROL_AF_SCENE_CHANGE, &afSceneChange, 1);
    CLOGV("afSceneChange=%d", afSceneChange);

    translateVendorControlMetaData(settings, shot_ext, requestInfo);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateEdgeMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateFlashMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                   CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    ExynosCameraParameters *parameters;
    ExynosCameraActivityControl *activityControl;
    ExynosCameraActivityFlash *flashMgr;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;
    camera_metadata_entry_t entry;
    int bestFlashShotFcount = 0;
    int32_t cameraId = physCamID;

    if (cameraId < 0)
        cameraId = m_cameraId;

#ifdef USE_DUAL_CAMERA
    // HACK: need to get exact cameraId, not appoximate cameraId
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
            m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
        int masterCamId, slaveCamId;
        // get the masterCameraId of reprocessing
        m_configurations->getDualCamId(masterCamId, slaveCamId, true);
        if (masterCamId >= 0) cameraId = masterCamId;
    }
#endif

    parameters = m_parameters[cameraId];
    activityControl = parameters->getActivityControl();
    flashMgr = activityControl->getFlashMgr();
    sensorStaticInfo = parameters->getSensorStaticInfo();

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    uint8_t flashState = ANDROID_FLASH_STATE_READY;
    if (flashMgr == NULL) {
        flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
    } else {
        bestFlashShotFcount = flashMgr->getBestFlashShotFcount();

        if (m_sensorStaticInfo->flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_FALSE) {
            {
                flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
            }
        } else {
            flashState = src->dm.flash.flashState;
        }
    }

    entry = settings->find(ANDROID_FLASH_MODE);
    if (entry.count > 0) {
        enum flash_mode flashMode = (enum flash_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        /*
         * Using android.control.aeMode == ON_ALWAYS_FLASH will always return FIRED.
         * Using android.flash.mode == TORCH will always return FIRED.
         */
        if (((shot_ext->shot.dm.flash.flashMode == CAM2_FLASH_MODE_TORCH)
                && (flashMode == CAM2_FLASH_MODE_TORCH))
                || flashMode == CAM2_FLASH_MODE_SINGLE) {
            flashState = ANDROID_FLASH_STATE_FIRED;
        }
    }

    int requestCount = requestInfo->getKey();
    int frameCount = getMetaDmRequestFrameCount(shot_ext);

    if (shot_ext->shot.dm.flash.vendor_firingStable == CAPTURE_STATE_FLASH) {
        int gapOfFCount         = bestFlashShotFcount - frameCount;

        CLOGD("[Flash][R%d][F%d] vendor_firingStable : %d. so, best flash frame will be [R%d][F%d]",
            requestCount, frameCount,
            shot_ext->shot.dm.flash.vendor_firingStable,
            requestCount + gapOfFCount, bestFlashShotFcount);
    }

    if (flashState == ANDROID_FLASH_STATE_FIRED) {
        if ((unsigned int)frameCount == bestFlashShotFcount) {
            CLOGD("[Flash][R%d][F%d][TS%ju] flashState ANDROID_FLASH_STATE_FIRED on best",
                requestCount, frameCount, getMetaUdmSensorTimeStampBoot(shot_ext));
        }
    }

    settings->update(ANDROID_FLASH_STATE, &flashState , 1);
    CLOGV("flashState=%d", flashState);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateHotPixelMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateJpegMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                   CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    translateVendorJpegMetaData(requestInfo, settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateLensMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                   CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    const uint8_t lensState = src->dm.lens.state;
    settings->update(ANDROID_LENS_STATE, &lensState, 1);
    CLOGV("dm.lens.state(%d)", src->dm.lens.state);

    const float focusRange[2] =
    { src->dm.lens.focusRange[0], src->dm.lens.focusRange[1] };
    settings->update(ANDROID_LENS_FOCUS_RANGE, focusRange, 2);
    CLOGV("dm.lens.focusRange(%f,%f)",
            focusRange[0], focusRange[1]);

    /* Focus distance 0 means infinite */
    float focusDistance = src->dm.lens.focusDistance;
    if(focusDistance < 0) {
        focusDistance = 0;
    } else if (focusDistance > sensorStaticInfo->minimumFocusDistance) {
        focusDistance = sensorStaticInfo->minimumFocusDistance;
    }
    settings->update(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);
    CLOGV("dm.lens.focusDistance(%f)", src->dm.lens.focusDistance);

    camera_metadata_entry_t entry;
    entry = m_staticInfo.find(ANDROID_LENS_APERTURE);
    if (entry.count > 0) {
        float aperture = entry.data.f[0];
        settings->update(ANDROID_LENS_APERTURE, &aperture, 1);
        CLOGV("dm.lens.aperture(%f)",entry.data.f[0]);
    }

    translateVendorLensMetaData(requestInfo, settings, shot_ext);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateNoiseMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateRequestMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

/*
 *
 * pipelineDepth is filed of 'REQUEST'
 *
 * but updating pipelineDepth data can be conflict
 * and we separeted this data not using data but request's private data
 *
 * remaining this code as comment is that to prevent missing update pieplineDepth data in the medta of 'REQUEST' field
 *
 */
/*
 *   const uint8_t pipelineDepth = src->dm.request.pipelineDepth;
 *   settings.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipelineDepth, 1);
 *   CLOGV("ANDROID_REQUEST_PIPELINE_DEPTH(%d)", pipelineDepth);
 */

    return OK;
}

status_t ExynosCameraMetadataConverter::translateScalerMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                                    CameraMetadata *settings,
                                                                    struct camera2_shot_ext *shot_ext,
                                                                    int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    translateVendorScalerMetaData(shot_ext);
    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    const int32_t cropRegion[4] =
    {
        static_cast<int32_t>(src->dm.scaler.cropRegion[0]),
        static_cast<int32_t>(src->dm.scaler.cropRegion[1]),
        static_cast<int32_t>(src->dm.scaler.cropRegion[2]),
        static_cast<int32_t>(src->dm.scaler.cropRegion[3])
    };

    settings->update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
    CLOGV("dm.scaler.cropRegion(%d,%d,%d,%d)",
            src->ctl.scaler.cropRegion[0],
            src->ctl.scaler.cropRegion[1],
            src->ctl.scaler.cropRegion[2],
            src->ctl.scaler.cropRegion[3]);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateSensorMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                   CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    int64_t frameDuration = (int64_t) src->dm.sensor.frameDuration;
    settings->update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);
    CLOGV("dm.sensor.frameDuration(%ju)", src->dm.sensor.frameDuration);

    int64_t exposureTime = (int64_t)src->dm.sensor.exposureTime;

    /*
     * HACK
     * The frameDuration must always be bigger than the exposureTime.
     * But firmware may not guarantee that. so we check the exposureTime and frameDuration.
     */
    if (exposureTime == 0 ||
        (frameDuration > 0 && exposureTime > frameDuration)) {
        exposureTime = frameDuration;
    }
    src->dm.sensor.exposureTime = exposureTime; //  for  EXIF Data
    settings->update(ANDROID_SENSOR_EXPOSURE_TIME, &exposureTime, 1);

    int32_t sensitivity = (int32_t) src->dm.sensor.sensitivity;
    if (sensitivity < sensorStaticInfo->sensitivityRange[MIN]) {
        sensitivity = sensorStaticInfo->sensitivityRange[MIN];
    } else if (sensitivity > sensorStaticInfo->sensitivityRange[MAX]) {
        sensitivity = sensorStaticInfo->sensitivityRange[MAX];
    }
    src->dm.sensor.sensitivity = sensitivity; //  for  EXIF Data
    settings->update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

    CLOGV("[frameCount is %d] exposureTime(%ju) sensitivity(%d)",
            src->dm.request.frameCount, exposureTime, sensitivity);

    const int64_t timeStamp = (int64_t) src->udm.sensor.timeStampBoot;
    settings->update(ANDROID_SENSOR_TIMESTAMP, &timeStamp, 1);
    CLOGV("udm.sensor.timeStampBoot(%ju)", src->udm.sensor.timeStampBoot);

    const camera_metadata_rational_t neutralColorPoint[3] =
    {
        {(int32_t) src->dm.sensor.neutralColorPoint[0].num,
         (int32_t) src->dm.sensor.neutralColorPoint[0].den},
        {(int32_t) src->dm.sensor.neutralColorPoint[1].num,
         (int32_t) src->dm.sensor.neutralColorPoint[1].den},
        {(int32_t) src->dm.sensor.neutralColorPoint[2].num,
         (int32_t) src->dm.sensor.neutralColorPoint[2].den}
    };

    settings->update(ANDROID_SENSOR_NEUTRAL_COLOR_POINT, neutralColorPoint, 3);
    CLOGV("dm.sensor.neutralColorPoint(%d/%d,%d/%d,%d/%d)",
            src->dm.sensor.neutralColorPoint[0].num,
            src->dm.sensor.neutralColorPoint[0].den,
            src->dm.sensor.neutralColorPoint[1].num,
            src->dm.sensor.neutralColorPoint[1].den,
            src->dm.sensor.neutralColorPoint[2].num,
            src->dm.sensor.neutralColorPoint[2].den);

    const double noiseProfile[8] =
    {
        src->dm.sensor.noiseProfile[0][0], src->dm.sensor.noiseProfile[0][1],
        src->dm.sensor.noiseProfile[1][0], src->dm.sensor.noiseProfile[1][1],
        src->dm.sensor.noiseProfile[2][0], src->dm.sensor.noiseProfile[2][1],
        src->dm.sensor.noiseProfile[3][0], src->dm.sensor.noiseProfile[3][1]
    };
    settings->update(ANDROID_SENSOR_NOISE_PROFILE, noiseProfile , 8);
    CLOGV("dm.sensor.noiseProfile({%f,%f},{%f,%f},{%f,%f},{%f,%f})",
            src->dm.sensor.noiseProfile[0][0],
            src->dm.sensor.noiseProfile[0][1],
            src->dm.sensor.noiseProfile[1][0],
            src->dm.sensor.noiseProfile[1][1],
            src->dm.sensor.noiseProfile[2][0],
            src->dm.sensor.noiseProfile[2][1],
            src->dm.sensor.noiseProfile[3][0],
            src->dm.sensor.noiseProfile[3][1]);

    const float greenSplit = src->dm.sensor.greenSplit;
    settings->update(ANDROID_SENSOR_GREEN_SPLIT, &greenSplit, 1);
    CLOGV("dm.sensor.greenSplit(%f)", src->dm.sensor.greenSplit);

    const int64_t rollingShutterSkew = (int64_t) src->dm.sensor.rollingShutterSkew;
    settings->update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &rollingShutterSkew, 1);
    CLOGV("dm.sensor.rollingShutterSkew(%ju)",
            src->dm.sensor.rollingShutterSkew);

    //settings->update(ANDROID_SENSOR_TEMPERATURE, , );
    //settings->update(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP, , );
    //settings->update(ANDROID_SENSOR_PROFILE_TONE_CURVE, , );

    translateVendorSensorMetaData(requestInfo, settings, shot_ext, physCamID);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateShadingMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateStatisticsMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                CameraMetadata *settings,
                                                struct camera2_shot_ext *shot_ext,
                                                int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);
    camera_metadata_entry_t entry;

    META_VALIDATE_CHECK(settings);

    entry = settings->find(ANDROID_STATISTICS_FACE_DETECT_MODE);
    if (entry.count > 0) {
        uint8_t faceDetectMode = entry.data.u8[0];
        if (faceDetectMode > ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {
            shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port) requestInfo->getDsInputPortId();
            m_updateFaceDetectionMetaData(requestInfo, settings, shot_ext);
        }
    }

    /* HACK : F/W does NOT support this field */
    //int32_t *hotPixelMap = (int32_t *) src->dm.stats.hotPixelMap;
    const int32_t hotPixelMap[] = {};

    META_VALIDATE_CHECK(settings);

    settings->update(ANDROID_STATISTICS_HOT_PIXEL_MAP, hotPixelMap, ARRAY_LENGTH(hotPixelMap));
    CLOGV("dm.stats.hotPixelMap");

    /* HACK : F/W does NOT support this field */
    //float *lensShadingMap = (float *) src->dm.stats.lensShadingMap;
    const float lensShadingMap[] = {1.0, 1.0, 1.0, 1.0};

    META_VALIDATE_CHECK(settings);

    settings->update(ANDROID_STATISTICS_LENS_SHADING_MAP, lensShadingMap, 4);
    CLOGV("dm.stats.lensShadingMap(%f,%f,%f,%f)",
            lensShadingMap[0], lensShadingMap[1],
            lensShadingMap[2], lensShadingMap[3]);

    uint8_t sceneFlicker = (uint8_t) CAMERA_METADATA(src->dm.stats.sceneFlicker);

    META_VALIDATE_CHECK(settings);

    settings->update(ANDROID_STATISTICS_SCENE_FLICKER, &sceneFlicker, 1);
    CLOGV("dm.stats.sceneFlicker(%d)", src->dm.stats.sceneFlicker);

    translateVendorStatisticsMetaData(settings, shot_ext);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateTonemapMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateLedMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    //settings->update(ANDROID_LED_TRANSMIT, (uint8_t *) NULL, 0);
    CLOGV("dm.led.transmit(%d)", src->dm.led.transmit);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateBlackLevelMetaData(CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    UNUSED_VARIABLE(physCamID);

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

    return OK;
}

status_t ExynosCameraMetadataConverter::translateSyncMetaData(ExynosCameraRequestSP_sprt_t requestInfo)
{
    CameraMetadata *settings;
    struct camera2_shot_ext *shot_ext = NULL;
    struct camera2_shot *src = NULL;
    int64_t frameCount = 0;

    if (requestInfo == NULL) {
        CLOGE("RequestInfo is NULL");
        return BAD_VALUE;
    }

    settings = requestInfo->getServiceMeta();
    shot_ext = requestInfo->getServiceShot();
    src = &(shot_ext->shot);

    frameCount = (int64_t)requestInfo->getKey();

    settings->update(ANDROID_SYNC_FRAME_NUMBER, &frameCount, 1);
    CLOGV("sync.framecount(%jd)", frameCount);

    META_VALIDATE_CHECK(settings);

    return OK;

}

status_t ExynosCameraMetadataConverter::translatePartialMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                   CameraMetadata *settings,
                                                   struct camera2_shot_ext *shot_ext,
                                                   enum metadata_type metaType,
                                                   int32_t physCamID)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t cropRegionEntry;
    struct camera2_shot *src = NULL;
    ExynosCameraActivityControl *activityControl = NULL;
    ExynosCameraActivityFlash *flashMgr = NULL;
    struct ExynosCameraSensorInfoBase *sensorStaticInfo;
    bool isNeedMetaUpdate;

    if (physCamID < 0)
        physCamID = m_cameraId;

    sensorStaticInfo = m_parameters[physCamID]->getSensorStaticInfo();

    src = &(shot_ext->shot);

    META_VALIDATE_CHECK(settings);

#ifdef USE_DUAL_CAMERA
    // TODO: change perframe control
    int32_t slaveCamId = requestInfo->getSlaveCamId();
    if (slaveCamId >= 0
        && m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM
        && m_parameters[slaveCamId] != NULL
        && (m_configurations->getDualOperationModeReprocessing()
            == DUAL_OPERATION_MODE_SLAVE)) {
        activityControl = m_parameters[slaveCamId]->getActivityControl();
    } else
#endif
    {
        activityControl = m_parameters[m_cameraId]->getActivityControl();
    }

    if (metaType == PARTIAL_3AA) {
        const int64_t timeStamp = (int64_t) src->udm.sensor.timeStampBoot;

        META_VALIDATE_CHECK(settings);

        settings->update(ANDROID_SENSOR_TIMESTAMP, &timeStamp, 1);
        CLOGV("udm.sensor.timeStampBoot(%ju)", src->udm.sensor.timeStampBoot);

        CLOGV("ANDROID_SENSOR_TIMESTAMP(%ju) (%u)",
                src->udm.sensor.timeStampBoot, src->dm.request.frameCount);

        if (m_sensorStaticInfo->max3aRegions[AE] > 0) {
            /* HACK: Result AE_REGION must be updated based of the value from  F/W */
            int32_t aeRegion[5] = {0, 0, 0, 0, 0};
            ExynosRect2 aeRect;
            isNeedMetaUpdate = false;
            cropRegionEntry = settings->find(ANDROID_SCALER_CROP_REGION);
            entry = settings->find(ANDROID_CONTROL_AE_REGIONS);
            if (cropRegionEntry.count > 0 && entry.count > 0) {
                /* ae region is bigger than crop region */
                if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                        || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                    aeRegion[0] = cropRegionEntry.data.i32[0];
                    aeRegion[1] = cropRegionEntry.data.i32[1];
                    aeRegion[2] = cropRegionEntry.data.i32[2] + aeRegion[0];
                    aeRegion[3] = cropRegionEntry.data.i32[3] + aeRegion[1];
                    aeRegion[4] = entry.data.i32[4];
                } else {
                    aeRegion[0] = entry.data.i32[0];
                    aeRegion[1] = entry.data.i32[1];
                    aeRegion[2] = entry.data.i32[2];
                    aeRegion[3] = entry.data.i32[3];
                    aeRegion[4] = entry.data.i32[4];
                }
                isNeedMetaUpdate = true;
            } else if (src->dm.aa.aeMode > AA_AEMODE_OFF) {
                aeRect.x1 = src->ctl.aa.aeRegions[0];
                aeRect.y1 = src->ctl.aa.aeRegions[1];
                aeRect.x2 = src->ctl.aa.aeRegions[2];
                aeRect.y2 = src->ctl.aa.aeRegions[3];

                m_convert3AAToActiveArrayRegion(&aeRect, "AE");
                aeRegion[0] = aeRect.x1;
                aeRegion[1] = aeRect.y1;
                aeRegion[2] = aeRect.x2;
                aeRegion[3] = aeRect.y2;
                aeRegion[4] = src->ctl.aa.aeRegions[4];
                isNeedMetaUpdate = true;
            }

            if (isNeedMetaUpdate) {
                settings->update(ANDROID_CONTROL_AE_REGIONS, aeRegion, 5);
                CLOGV("dm.aa.aeRegions(%d,%d,%d,%d,%d)",
                        src->dm.aa.aeRegions[0],
                        src->dm.aa.aeRegions[1],
                        src->dm.aa.aeRegions[2],
                        src->dm.aa.aeRegions[3],
                        src->dm.aa.aeRegions[4]);
            }
        }

        if (m_sensorStaticInfo->max3aRegions[AWB] > 0) {
            /* HACK: Result AWB_REGION must be updated based of the value from  F/W */
            int32_t awbRegion[5] = {0, 0, 0, 0, 0};
            ExynosRect2 awbRect;
            isNeedMetaUpdate = false;
            cropRegionEntry = settings->find(ANDROID_SCALER_CROP_REGION);
            entry = settings->find(ANDROID_CONTROL_AWB_REGIONS);
            if (cropRegionEntry.count > 0 && entry.count > 0) {
                /* awb region is bigger than crop region */
                if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                        || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                    awbRegion[0] = cropRegionEntry.data.i32[0];
                    awbRegion[1] = cropRegionEntry.data.i32[1];
                    awbRegion[2] = cropRegionEntry.data.i32[2] + awbRegion[0];
                    awbRegion[3] = cropRegionEntry.data.i32[3] + awbRegion[1];
                    awbRegion[4] = entry.data.i32[4];
                } else {
                    awbRegion[0] = entry.data.i32[0];
                    awbRegion[1] = entry.data.i32[1];
                    awbRegion[2] = entry.data.i32[2];
                    awbRegion[3] = entry.data.i32[3];
                    awbRegion[4] = entry.data.i32[4];
                }
                isNeedMetaUpdate = true;
            } else if (src->dm.aa.awbMode > AA_AWBMODE_OFF) {
                awbRect.x1 = src->ctl.aa.awbRegions[0];
                awbRect.y1 = src->ctl.aa.awbRegions[1];
                awbRect.x2 = src->ctl.aa.awbRegions[2];
                awbRect.y2 = src->ctl.aa.awbRegions[3];

                m_convert3AAToActiveArrayRegion(&awbRect, "AWB");
                awbRegion[0] = awbRect.x1;
                awbRegion[1] = awbRect.y1;
                awbRegion[2] = awbRect.x2;
                awbRegion[3] = awbRect.y2;
                awbRegion[4] = src->ctl.aa.awbRegions[4];
                isNeedMetaUpdate = true;
            }

            if (isNeedMetaUpdate) {
                META_VALIDATE_CHECK(settings);

                settings->update(ANDROID_CONTROL_AWB_REGIONS, awbRegion, 5);
                CLOGV("dm.aa.awbRegions(%d,%d,%d,%d,%d)",
                        src->dm.aa.awbRegions[0],
                        src->dm.aa.awbRegions[1],
                        src->dm.aa.awbRegions[2],
                        src->dm.aa.awbRegions[3],
                        src->dm.aa.awbRegions[4]);
            }
        }

        if (m_sensorStaticInfo->max3aRegions[AF] > 0) {
            int32_t afRegion[5] = {0, 0, 0, 0, 0};
            ExynosRect2 afRect;
            cropRegionEntry = settings->find(ANDROID_SCALER_CROP_REGION);
            entry = settings->find(ANDROID_CONTROL_AF_REGIONS);
            isNeedMetaUpdate = false;

           /*
            * CTS(testDigitalZoom) require that af region value is exactly same about control value.
            * If af mode is AA_AFMODE_CONTINUOUS_VIDEO or AA_AFMODE_CONTINUOUS_PICTURE,
            * af region in DM is not considered about CONTROL value.
            * So, HAL should set af region based on control value.
            */
            if (cropRegionEntry.count > 0 && entry.count > 0) {
                /* af region is bigger than crop region */
                if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                    || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                    afRegion[0] = cropRegionEntry.data.i32[0];
                    afRegion[1] = cropRegionEntry.data.i32[1];
                    afRegion[2] = cropRegionEntry.data.i32[2] + afRegion[0];
                    afRegion[3] = cropRegionEntry.data.i32[3] + afRegion[1];
                    afRegion[4] = entry.data.i32[4];
                } else {
                    afRegion[0] = entry.data.i32[0];
                    afRegion[1] = entry.data.i32[1];
                    afRegion[2] = entry.data.i32[2];
                    afRegion[3] = entry.data.i32[3];
                    afRegion[4] = entry.data.i32[4];
                }
                isNeedMetaUpdate = true;
            } else if (src->dm.aa.afMode > AA_AFMODE_OFF) {
                afRect.x1 = src->dm.aa.afRegions[0];
                afRect.y1 = src->dm.aa.afRegions[1];
                afRect.x2 = src->dm.aa.afRegions[2];
                afRect.y2 = src->dm.aa.afRegions[3];

                m_convert3AAToActiveArrayRegion(&afRect, "AF");
                afRegion[0] = afRect.x1;
                afRegion[1] = afRect.y1;
                afRegion[2] = afRect.x2;
                afRegion[3] = afRect.y2;
                afRegion[4] = src->dm.aa.afRegions[4];
                isNeedMetaUpdate = true;
            }

            if (isNeedMetaUpdate) {
                META_VALIDATE_CHECK(settings);
            }

            settings->update(ANDROID_CONTROL_AF_REGIONS, afRegion, 5);
            CLOGV("physCamID[%d]: dm.aa.afRegions(%d,%d,%d,%d,%d)",
                    physCamID, afRegion[0], afRegion[1], afRegion[2], afRegion[3], afRegion[4]);
        } else {
            entry = settings->find(ANDROID_CONTROL_AF_REGIONS);
            if (entry.count > 0) {
                CLOGV("[Cam_Id-%d][R%d]Erase ANDROID_CONTROL_AF_REGIONS. count %zu",
                    physCamID, requestInfo->getKey(), entry.count);
                settings->erase(ANDROID_CONTROL_AF_REGIONS);
            }
        }

        uint8_t tmpAeState = (uint8_t) CAMERA_METADATA(src->dm.aa.aeState);

        /* HACK: forcely set AE state during init skip count (FW not supported) */
        if (src->dm.request.frameCount < INITIAL_SKIP_FRAME) {
            tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_SEARCHING);
        }

#ifdef USE_AE_CONVERGED_UDM
        if (isBackCamera(m_cameraId)) {
            if (tmpAeState == (uint8_t) CAMERA_METADATA(AE_STATE_CONVERGED)) {
                uint32_t aeUdmState = (uint32_t)src->udm.ae.vendorSpecific[397];
                /*  1: converged, 0: searching */
                if (aeUdmState == 0) {
                    tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_SEARCHING);
                }
            }
        }
#endif

        flashMgr = activityControl->getFlashMgr();
        if (flashMgr == NULL) {
            CLOGE("flashMgr is NULL");
            return BAD_VALUE;
        }

        uint8_t aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        entry = settings->find(ANDROID_CONTROL_AE_MODE);
        if (entry.count > 0) {
            aeMode = entry.data.u8[0];
        }

        int frameCount = getMetaDmRequestFrameCount(shot_ext);
        int bestFlashFrameCount = flashMgr->getBestFlashShotFcount();

        switch (src->dm.aa.aeState) {
            case AE_STATE_CONVERGED:
            case AE_STATE_LOCKED:
                if (flashMgr != NULL)
                    flashMgr->notifyAeResult();

                if (aeMode == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH) {
                    if (frameCount == bestFlashFrameCount - 1) {
                        CLOGD("[Flash][F%d] != best flash frame [F%d] tmpAeState(%d) -> AE_STATE_FLASH_REQUIRED. aeState(%d) afState(%d)",
                            frameCount, bestFlashFrameCount, tmpAeState, getMetaDmAeState(shot_ext), getMetaDmAfState(shot_ext));
                    } else if (frameCount == bestFlashFrameCount) {
                        CLOGD("[Flash][F%d] == best flash frame [F%d] tmpAeState(%d) -> AE_STATE_FLASH_REQUIRED. aeState(%d) afState(%d)",
                            frameCount, bestFlashFrameCount, tmpAeState, getMetaDmAeState(shot_ext), getMetaDmAfState(shot_ext));
                    } else {
                        // nop
                    }

                    tmpAeState = (uint8_t)CAMERA_METADATA(AE_STATE_FLASH_REQUIRED);
                }
                break;
            case AE_STATE_INACTIVE:
            case AE_STATE_SEARCHING:
            case AE_STATE_FLASH_REQUIRED:
            case AE_STATE_PRECAPTURE:
            default:
                break;
        }

        const uint8_t aeState = tmpAeState;

        META_VALIDATE_CHECK(settings);

        settings->update(ANDROID_CONTROL_AE_STATE, &aeState, 1);
        CLOGV("dm.aa.aeState(%d), AE_STATE(%d)", src->dm.aa.aeState, aeState);

        src->dm.aa.afState = translateVendorAfStateMetaData(src->dm.aa.afState);
        const uint8_t afState = (uint8_t) CAMERA_METADATA(src->dm.aa.afState);

        META_VALIDATE_CHECK(settings);

        settings->update(ANDROID_CONTROL_AF_STATE, &afState, 1);
        CLOGV("dm.aa.afState(%d)", src->dm.aa.afState);

        const uint8_t awbState = (uint8_t) CAMERA_METADATA(src->dm.aa.awbState);

        META_VALIDATE_CHECK(settings);

        settings->update(ANDROID_CONTROL_AWB_STATE, &awbState, 1);
        CLOGV("dm.aa.awbState(%d)", src->dm.aa.awbState);

        switch (src->dm.aa.afState) {
        case AA_AFSTATE_FOCUSED_LOCKED:
        case AA_AFSTATE_NOT_FOCUSED_LOCKED:
            if (flashMgr != NULL)
                flashMgr->notifyAfResultHAL3();
#ifdef USE_DUAL_CAMERA
            if (m_configurations->getDualOperationModeLockCount() > 3) {
                m_configurations->setDualOperationModeLockCount(3);
            }
#endif
            break;
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_PASSIVE_SCAN:
        case AA_AFSTATE_PASSIVE_FOCUSED:
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
#ifdef USE_DUAL_CAMERA
            m_configurations->setDualOperationModeLockCount(10);
#endif
            break;
        case AA_AFSTATE_PASSIVE_UNFOCUSED:
        default:
            break;
        }
    }

    translateVendorPartialMetaData(requestInfo, settings, shot_ext, metaType);

    return OK;
}

status_t ExynosCameraMetadataConverter::updateDynamicMeta(ExynosCameraRequestSP_sprt_t requestInfo,
                                                            enum metadata_type metaType, int32_t physCamID)
{
    status_t ret = OK;
    uint32_t errorFlag = 0;

    struct camera2_shot_ext *dst_ext = NULL;
    CameraMetadata *meta = NULL;

    CLOGV("%d frame", requestInfo->getFrameCount());
    /* Validation check */
    if (requestInfo == NULL) {
        CLOGE("RequestInfo is NULL!!");
        return BAD_VALUE;
    }

    requestInfo->setRequestLock();

    if (physCamID < 0) {
        meta = requestInfo->getServiceMeta();
        dst_ext = requestInfo->getServiceShot();
    } else {
#if 0
        meta = requestInfo->getServiceMetaPhysCam(physCamID);
        dst_ext = requestInfo->getServiceShotPhysCam(physCamID);
        if (metaType == PARTIAL_NONE) {
            ret = translatePartialMetaData(requestInfo, meta, dst_ext, PARTIAL_3AA, physCamID);
            if (ret != OK)
                errorFlag |= (1 << 16);
        }
#endif
    }

    if (metaType == PARTIAL_NONE) {
        ret = translateColorMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 0);
        ret = translateControlMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 1);
        ret = translateEdgeMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 2);
        ret = translateFlashMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 3);
        ret = translateHotPixelMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 4);
        ret = translateJpegMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 5);
        ret = translateLensMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 6);
        ret = translateNoiseMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 7);
        ret = translateRequestMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 8);
        ret = translateScalerMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 9);
        ret = translateSensorMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 10);
        ret = translateShadingMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 11);
        ret = translateStatisticsMetaData(requestInfo, meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 12);
        ret = translateTonemapMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 13);
        ret = translateLedMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 14);
        ret = translateBlackLevelMetaData(meta, dst_ext, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 15);
        ret = translateSyncMetaData(requestInfo);
        if (ret != OK)
            errorFlag |= (1 << 16);
    } else {
        ret = translatePartialMetaData(requestInfo, meta, dst_ext, metaType, physCamID);
        if (ret != OK)
            errorFlag |= (1 << 17);
    }

    // if all meta is updated. (== not partial)
    if (metaType == PARTIAL_NONE) {
        m_keepMetaData(requestInfo);
    }

    requestInfo->setRequestUnlock();

    if (errorFlag != 0) {
        CLOGE("failed to translate Meta Type(%d) Data(%d)", metaType, errorFlag);
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCameraMetadataConverter::m_keepMetaData(ExynosCameraRequestSP_sprt_t requestInfo)
{
    status_t ret = NO_ERROR;
    struct camera2_shot_ext *shot_ext = NULL;
    struct camera2_shot *src = NULL;

    shot_ext = requestInfo->getServiceShot();
    src = &(shot_ext->shot);

    /* Store the timeStamp and framecount mapping info */
    int32_t mapIndex = m_getFrameInfoIndexForTimeStamp(src->udm.sensor.timeStampBoot);
    if (mapIndex < 0) {
        Mutex::Autolock lock(m_frameCountMapIndexLock);

        m_frameCountMap[m_frameCountMapIndex][TIMESTAMP] = src->udm.sensor.timeStampBoot;
        m_frameCountMap[m_frameCountMapIndex][FRAMECOUNT] = (uint64_t) src->dm.request.frameCount;
        m_frameCountMap[m_frameCountMapIndex][APERTURE] = (uint64_t) src->dm.lens.aperture;

        if (m_sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING) {
            memcpy(&m_dmMap[m_frameCountMapIndex], &src->dm, sizeof(struct camera2_dm));
            memcpy(&m_udmMap[m_frameCountMapIndex], &src->udm, sizeof(struct camera2_udm));
            memcpy(&m_shotExtUserMap[m_frameCountMapIndex], &shot_ext->user, sizeof(struct camera2_shot_ext_user));
            memcpy(&m_vraExtMap[m_frameCountMapIndex], &shot_ext->vra_ext, sizeof(struct vra_ext_meta));
        }

        m_frameCountMapIndex = (m_frameCountMapIndex + 1) % FRAMECOUNT_MAP_LENGTH;
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::checkAvailableStreamFormat(int format)
{
    int ret = OK;
    CLOGD(" format(%d)", format);

    // TODO:check available format
    return ret;
}

status_t ExynosCameraMetadataConverter::m_createAvailableCapabilities(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *capabilities, cameraId_Info *camIdInfo)
{
    status_t ret = NO_ERROR;
    uint8_t hwLevel;
    uint64_t supportedCapabilities;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (capabilities == NULL) {
        CLOGE2("NULL");
        return BAD_VALUE;
    }

    hwLevel = sensorStaticInfo->supportedHwLevel;
    supportedCapabilities = sensorStaticInfo->supportedCapabilities;

    CLOGD2("supportedHwLevel(%d) supportedCapabilities(0x%4ju)", hwLevel, supportedCapabilities);

    capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE);

    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_MANUAL_SENSOR) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR);
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS);
    }
    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_MANUAL_POST_PROCESSING) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING);
    }
    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_BURST_CAPTURE) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE);
    }
    if (supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING);
    }
    if (supportedCapabilities & CAPABILITIES_YUV_REPROCESSING) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING);
    }
    if (supportedCapabilities & CAPABILITIES_RAW) {
        if (!isLogicalCam(camIdInfo->serviceCameraId)) {
            capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW);
        }
    }
    if (supportedCapabilities & CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    }

    if (supportedCapabilities & CAPABILITIES_MOTION_TRACKING) {
        capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MOTION_TRACKING);
    }

    if (supportedCapabilities & CAPABILITIES_LOGICAL_MULTI_CAMERA) {
        if (isLogicalCam(camIdInfo->serviceCameraId)) {
            capabilities->add(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createAvailableKeys(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *session, Vector<int32_t> *request, Vector<int32_t> *result, Vector<int32_t> *characteristics, int cameraId)
{
    status_t ret = NO_ERROR;
    uint8_t hwLevel;
    uint64_t supportedCapabilities;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (request == NULL || result == NULL || characteristics == NULL) {
        CLOGE2("NULL");
        return BAD_VALUE;
    }

    hwLevel = sensorStaticInfo->supportedHwLevel;
    supportedCapabilities = sensorStaticInfo->supportedCapabilities;

    if (sensorStaticInfo->requestKeys != NULL) {
        request->appendArray(sensorStaticInfo->requestKeys, sensorStaticInfo->requestKeysLength);
    }
    if (sensorStaticInfo->resultKeys != NULL) {
        result->appendArray(sensorStaticInfo->resultKeys, sensorStaticInfo->resultKeysLength);
    }
    if (sensorStaticInfo->characteristicsKeys != NULL) {
        characteristics->appendArray(sensorStaticInfo->characteristicsKeys,
                                    sensorStaticInfo->characteristicsKeysLength);
    }

#ifdef SUPPORT_SESSION_PARAMETERS
    /*
    * Session KEYS: The listed KEYS config is constant between the configStreams() call.
    * Include all the KEYS that can't be supported per-frame.
    * Include all the KEYS that cause unexpected delays to apply per-frame
    */
    /* android.request.availableSessionKeys */
    if (sensorStaticInfo->sessionKeys != NULL) {
        session->appendArray(sensorStaticInfo->sessionKeys, sensorStaticInfo->sessionKeysLength);
    }
#endif

    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL) {
        request->add(ANDROID_EDGE_MODE);
        result->add(ANDROID_EDGE_MODE);
        characteristics->add(ANDROID_EDGE_AVAILABLE_EDGE_MODES);
    }
    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_MANUAL_SENSOR) {
        request->insertArrayAt(availableRequestManualSensor, request->size(), ARRAY_LENGTH(availableRequestManualSensor));
        result->insertArrayAt(availableResultManualSensor, result->size(), ARRAY_LENGTH(availableResultManualSensor));
    }
    if (hwLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL
        || supportedCapabilities & CAPABILITIES_MANUAL_POST_PROCESSING) {
        request->insertArrayAt(availableRequestManualPostProcessing, request->size(), ARRAY_LENGTH(availableRequestManualPostProcessing));
        result->insertArrayAt(availableResultManualPostProcessing, result->size(), ARRAY_LENGTH(availableResultManualPostProcessing));
        characteristics->add(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES);
        characteristics->add(ANDROID_TONEMAP_MAX_CURVE_POINTS);
    }
    if (supportedCapabilities & CAPABILITIES_RAW) {
        request->insertArrayAt(availableRequestRaw, request->size(), ARRAY_LENGTH(availableRequestRaw));
        result->insertArrayAt(availableResultRaw, result->size(), ARRAY_LENGTH(availableResultRaw));
        characteristics->add(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES);
        characteristics->add(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES);
    }

    if (supportedCapabilities & CAPABILITIES_LOGICAL_MULTI_CAMERA) {
        characteristics->add(ANDROID_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS);
    }

    if (sensorStaticInfo->max3aRegions[AE] > 0) {
        request->add(ANDROID_CONTROL_AE_REGIONS);
        result->add(ANDROID_CONTROL_AE_REGIONS);
    }
    if (sensorStaticInfo->max3aRegions[AWB] > 0) {
        request->add(ANDROID_CONTROL_AWB_REGIONS);
        result->add(ANDROID_CONTROL_AWB_REGIONS);
    }
    if (sensorStaticInfo->max3aRegions[AF] > 0) {
        request->add(ANDROID_CONTROL_AF_REGIONS);
        result->add(ANDROID_CONTROL_AF_REGIONS);
    }

    if (sensorStaticInfo->controlZslSupport) {
        request->add(ANDROID_CONTROL_ENABLE_ZSL);
    }

    m_createVendorAvailableKeys(sensorStaticInfo, session, request, result, characteristics, cameraId);

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createControlAvailableHighSpeedVideoConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    int (*highSpeedVideoSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int highSpeedVideoSizeListLength = 0;
    int (*highSpeedVideoFPSList)[2] = NULL;
    int highSpeedVideoFPSListLength = 0;
    int streamConfigSize = 0;
    bool isSupportHighSpeedVideo = false;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    isSupportHighSpeedVideo = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);

    if (isSupportHighSpeedVideo) {
        highSpeedVideoSizeList = sensorStaticInfo->highSpeedVideoList;
        highSpeedVideoSizeListLength = sensorStaticInfo->highSpeedVideoListMax;
        highSpeedVideoFPSList = sensorStaticInfo->highSpeedVideoFPSList;
        highSpeedVideoFPSListLength = sensorStaticInfo->highSpeedVideoFPSListMax;

        streamConfigSize = (highSpeedVideoSizeListLength * highSpeedVideoFPSListLength * 5);

        for (int i = 0; i < highSpeedVideoFPSListLength; i++) {
            for (int j = 0; j < highSpeedVideoSizeListLength; j++) {
                if (highSpeedVideoSizeList[j][2] == highSpeedVideoFPSList[i][1]/1000) {
                    streamConfigs->add(highSpeedVideoSizeList[j][0]);
                    streamConfigs->add(highSpeedVideoSizeList[j][1]);
                    streamConfigs->add(highSpeedVideoFPSList[i][0]/1000);
                    streamConfigs->add(highSpeedVideoFPSList[i][1]/1000);
#if defined(SUPPORT_HFR_BATCH_MODE) && !defined(USE_SERVICE_BATCH_MODE)
                    streamConfigs->add((highSpeedVideoFPSList[i][1]/1000)/MULTI_BUFFER_BASE_FPS);
#else
                    streamConfigs->add(1);
#endif
                }
            }
        }
        return NO_ERROR;
    } else {
        return NAME_NOT_FOUND;
    }
}

/*
   - Returns NO_ERROR if private reprocessing is supported: streamConfigs will have valid entries.
   - Returns NAME_NOT_FOUND if private reprocessing is not supported: streamConfigs will be returned as is,
     and scaler.AvailableInputOutputFormatsMap should not be updated.
*/
status_t ExynosCameraMetadataConverter::m_createScalerAvailableInputOutputFormatsMap(const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                         Vector<int32_t> *streamConfigs)
{
    status_t ret = NAME_NOT_FOUND;
    int streamConfigSize = 0;
    bool isSupportPrivReprocessing = false;
    bool isSupportYuvReprocessing = false;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    isSupportPrivReprocessing = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING);
    if (isSupportPrivReprocessing == true)
        streamConfigSize += 4;

    isSupportYuvReprocessing = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING);
    if (isSupportYuvReprocessing == true)
        streamConfigSize += 4;

    streamConfigs->setCapacity(streamConfigSize);

    if (isSupportPrivReprocessing == true) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(2);
        streamConfigs->add(HAL_PIXEL_FORMAT_YCbCr_420_888);
        streamConfigs->add(HAL_PIXEL_FORMAT_BLOB);

        ret = NO_ERROR;
    }

    if (isSupportYuvReprocessing == true) {
        streamConfigs->add(HAL_PIXEL_FORMAT_YCbCr_420_888);
        streamConfigs->add(2);
        streamConfigs->add(HAL_PIXEL_FORMAT_YCbCr_420_888);
        streamConfigs->add(HAL_PIXEL_FORMAT_BLOB);

        ret = NO_ERROR;
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createScalerAvailableStreamConfigurationsOutput(const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                                           Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int (*yuvReprocessingSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvReprocessingSizeListLength = 0;
    int (*rawOutputSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int rawOutputSizeListLength = 0;
    int jpegSizeListLength = 0;
    int streamConfigSize = 0;
    bool isSupportHighResolution = false;
    bool isSupportPrivReprocessing = false;
    bool isSupportYuvReprocessing = false;
    bool isSupportRaw = false;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    isSupportHighResolution = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_BURST_CAPTURE);

    yuvSizeList = sensorStaticInfo->yuvList;
    yuvSizeListLength = sensorStaticInfo->yuvListMax;
    jpegSizeList = sensorStaticInfo->jpegList;
    jpegSizeListLength = sensorStaticInfo->jpegListMax;

    /* Check wheather the private reprocessing is supported or not */
    isSupportPrivReprocessing = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_PRIVATE_REPROCESSING);
    isSupportRaw = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_RAW);
    isSupportYuvReprocessing = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_YUV_REPROCESSING);
    if (isSupportYuvReprocessing) {
        yuvReprocessingSizeList         = sensorStaticInfo->yuvReprocessingInputList;
        yuvReprocessingSizeListLength   = sensorStaticInfo->yuvReprocessingInputListMax;
    }

    if (isSupportRaw) {
        rawOutputSizeList           = sensorStaticInfo->rawOutputList;
        rawOutputSizeListLength     = sensorStaticInfo->rawOutputListMax;
    }


    /* TODO: Add YUV reprocessing if necessary */

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream configuration list size */
    streamConfigSize = yuvSizeListLength * 4;
    /* YUV output stream configuration list size */
    streamConfigSize += (yuvSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
    /* Stall output stream configuration list size */
    streamConfigSize += (jpegSizeListLength * 4) * (ARRAY_LENGTH(STALL_FORMATS));
    /* RAW output stream configuration list size */
    if (isSupportRaw == true) {
        streamConfigSize += (1 * 4) * (ARRAY_LENGTH(RAW_FORMATS));
    }
    /* ZSL input stream configuration list size */
    if (isSupportPrivReprocessing == true) {
        streamConfigSize += 4;
    }

    /* YUV input stream configuration list size */
    if (isSupportYuvReprocessing) {
#if 0 /* basically max yuv size support */
        streamConfigSize += (yuvSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
#else
        streamConfigSize += (yuvReprocessingSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
#endif
    }

    streamConfigs->setCapacity(streamConfigSize);

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream supported size list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(yuvSizeList[i][0]);
        streamConfigs->add(yuvSizeList[i][1]);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    /* YUV output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(YUV_FORMATS); i++) {
        for (int j = 0; j < yuvSizeListLength; j++) {
            int pixelSize = yuvSizeList[j][0] * yuvSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                streamConfigSize -= 4;
                continue;
            }

            streamConfigs->add(YUV_FORMATS[i]);
            streamConfigs->add(yuvSizeList[j][0]);
            streamConfigs->add(yuvSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    /* Stall output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            streamConfigs->add(STALL_FORMATS[i]);
            streamConfigs->add(jpegSizeList[j][0]);
            streamConfigs->add(jpegSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    /* RAW output stream supported size list */
    if (isSupportRaw == true) {
        for (size_t i = 0; i < ARRAY_LENGTH(RAW_FORMATS); i++) {
            for (int j = 0; j < rawOutputSizeListLength; j++) {
                /* Add sensor max size */
                streamConfigs->add(RAW_FORMATS[i]);
                streamConfigs->add(rawOutputSizeList[j][0]);
                streamConfigs->add(rawOutputSizeList[j][1]);
                streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
            }
        }
    }

    /* ZSL input stream supported size list */
    if(isSupportPrivReprocessing == true) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(yuvSizeList[0][0]);
        streamConfigs->add(yuvSizeList[0][1]);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT);
    }

    /* YUV input stream supported size list */
    if (isSupportYuvReprocessing == true) {

        for (size_t i = 0; i < ARRAY_LENGTH(YUV_FORMATS); i++) {
#if 0 /* basically max yuv size support */
            for (int j = 0; j < yuvSizeListLength; j++) {
                int pixelSize = yuvSizeList[j][0] * yuvSizeList[j][1];
                streamConfigs->add(YUV_FORMATS[i]);
                streamConfigs->add(yuvSizeList[j][0]);
                streamConfigs->add(yuvSizeList[j][1]);
                streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT);
            }
#else
            for (int j = 0; j < yuvReprocessingSizeListLength; j++)
            {
                streamConfigs->add(YUV_FORMATS[i]);
                streamConfigs->add(yuvReprocessingSizeList[j][0]);
                streamConfigs->add(yuvReprocessingSizeList[j][1]);
                streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT);
            }
#endif
        }
    }

    streamConfigs->setCapacity(streamConfigSize);

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int32_t* streamConfigArray = NULL;
    streamConfigArray = streamConfigs->array();
    for (int i = 0; i < streamConfigSize; i = i + 4) {
        CLOGD2("Size %4dx%4d Format %2x %6s",
                streamConfigArray[i+1], streamConfigArray[i+2],
                streamConfigArray[i],
                (streamConfigArray[i+3] == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT)?
                "OUTPUT" : "INPUT");
    }
#endif

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createScalerAvailableMinFrameDurations(const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                                 Vector<int64_t> *minDurations)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int minDurationSize = 0;
    bool isSupportHighResolution = false;
    bool isSupportRaw = false;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (minDurations == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    isSupportHighResolution = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_BURST_CAPTURE);
    isSupportRaw = (sensorStaticInfo->supportedCapabilities & CAPABILITIES_RAW);

    yuvSizeList = sensorStaticInfo->yuvList;
    yuvSizeListLength = sensorStaticInfo->yuvListMax;
    jpegSizeList = sensorStaticInfo->jpegList;
    jpegSizeListLength = sensorStaticInfo->jpegListMax;

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream min frame duration list size */
    minDurationSize = yuvSizeListLength * 4;
    /* YUV output stream min frame duration list size */
    minDurationSize += (yuvSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
    /* Stall output stream configuration list size */
    minDurationSize += (jpegSizeListLength * 4) * (ARRAY_LENGTH(STALL_FORMATS));
    /* RAW output stream min frame duration list size */
    if (isSupportRaw == true) {
        minDurationSize += (1 * 4) * (ARRAY_LENGTH(RAW_FORMATS));
    }
    minDurations->setCapacity(minDurationSize);

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream min frame duration list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        minDurations->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        minDurations->add((int64_t)yuvSizeList[i][0]);
        minDurations->add((int64_t)yuvSizeList[i][1]);
        minDurations->add((int64_t)yuvSizeList[i][2]);
    }

    /* YUV output stream min frame duration list */
    for (size_t i = 0; i < ARRAY_LENGTH(YUV_FORMATS); i++) {
        for (int j = 0; j < yuvSizeListLength; j++) {
            int pixelSize = yuvSizeList[j][0] * yuvSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                minDurationSize -= 4;
                continue;
            }

            minDurations->add((int64_t)YUV_FORMATS[i]);
            minDurations->add((int64_t)yuvSizeList[j][0]);
            minDurations->add((int64_t)yuvSizeList[j][1]);
            minDurations->add((int64_t)yuvSizeList[j][2]);
        }
    }

    /* Stall output stream min frame duration list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            minDurations->add((int64_t)STALL_FORMATS[i]);
            minDurations->add((int64_t)jpegSizeList[j][0]);
            minDurations->add((int64_t)jpegSizeList[j][1]);
            minDurations->add((int64_t)jpegSizeList[j][2]);
        }
    }

    /* RAW output stream min frame duration list */
    if (isSupportRaw == true) {
        for (size_t i = 0; i < ARRAY_LENGTH(RAW_FORMATS); i++) {
            /* Add sensor max size */
            minDurations->add((int64_t)RAW_FORMATS[i]);
            minDurations->add((int64_t)sensorStaticInfo->maxSensorW);
            minDurations->add((int64_t)sensorStaticInfo->maxSensorH);
            minDurations->add((int64_t)yuvSizeList[0][2]);
        }
    }

    minDurations->setCapacity(minDurationSize);

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int64_t* minDurationArray = NULL;
    minDurationArray = minDurations->array();
    for (int i = 0; i < minDurationSize; i = i + 4) {
        CLOGD2("Size %4lldx%4lld Format %2x MinDuration %9lld",
                minDurationArray[i+1], minDurationArray[i+2],
                (int)minDurationArray[i], minDurationArray[i+3]);
    }
#endif

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createJpegAvailableThumbnailSizes(const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                             Vector<int32_t> *thumbnailSizes)
{
    int ret = OK;
    int (*thumbnailSizeList)[SIZE_OF_RESOLUTION] = NULL;
    size_t thumbnailSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (thumbnailSizes == NULL) {
        CLOGE2("Thumbnail sizes is NULL");
        return BAD_VALUE;
    }

    thumbnailSizeList = sensorStaticInfo->thumbnailList;
    thumbnailSizeListLength = sensorStaticInfo->thumbnailListMax;
    thumbnailSizes->setCapacity(thumbnailSizeListLength * 2);

    /* JPEG thumbnail sizes must be delivered with ascending ordering */
    for (int i = (int)thumbnailSizeListLength - 1; i >= 0; i--) {
        thumbnailSizes->add(thumbnailSizeList[i][0]);
        thumbnailSizes->add(thumbnailSizeList[i][1]);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createAeAvailableFpsRanges(const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                      Vector<int32_t> *fpsRanges)
{
    int ret = OK;
    int (*fpsRangesList)[2] = NULL;
    size_t fpsRangesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (fpsRanges == NULL) {
        CLOGE2("FPS ranges is NULL");
        return BAD_VALUE;
    }

    fpsRangesList = sensorStaticInfo->fpsRangesList;
    fpsRangesLength = sensorStaticInfo->fpsRangesListMax;

    fpsRanges->setCapacity(fpsRangesLength * 2);

    for (size_t i = 0; i < fpsRangesLength; i++) {
        fpsRanges->add(fpsRangesList[i][0]/1000);
        fpsRanges->add(fpsRangesList[i][1]/1000);
    }

    return ret;
}

bool ExynosCameraMetadataConverter::m_hasTagInList(int32_t *list, size_t listSize, int32_t tag)
{
    bool hasTag = false;

    for (size_t i = 0; i < listSize; i++) {
        if (list[i] == tag) {
            hasTag = true;
            break;
        }
    }

    return hasTag;
}

bool ExynosCameraMetadataConverter::m_hasTagInList(uint8_t *list, size_t listSize, int32_t tag)
{
    bool hasTag = false;

    for (size_t i = 0; i < listSize; i++) {
        if (list[i] == tag) {
            hasTag = true;
            break;
        }
    }

    return hasTag;
}

status_t ExynosCameraMetadataConverter::m_integrateOrderedSizeList(int (*list1)[SIZE_OF_RESOLUTION], size_t list1Size,
                                                                    int (*list2)[SIZE_OF_RESOLUTION], size_t list2Size,
                                                                    int (*orderedList)[SIZE_OF_RESOLUTION])
{
    int *currentSize = NULL;
    size_t sizeList1Index = 0;
    size_t sizeList2Index = 0;

    if (list1 == NULL || list2 == NULL || orderedList == NULL) {
        CLOGE2("Arguments are NULL. list1 %p list2 %p orderedlist %p",
                list1, list2, orderedList);
        return BAD_VALUE;
    }

    /* This loop will integrate two size list in descending order */
    for (size_t i = 0; i < list1Size + list2Size; i++) {
        if (sizeList1Index >= list1Size) {
            currentSize = list2[sizeList2Index++];
        } else if (sizeList2Index >= list2Size) {
            currentSize = list1[sizeList1Index++];
        } else {
            if (list1[sizeList1Index][0] < list2[sizeList2Index][0]) {
                currentSize = list2[sizeList2Index++];
            } else if (list1[sizeList1Index][0] > list2[sizeList2Index][0]) {
                currentSize = list1[sizeList1Index++];
            } else {
                if (list1[sizeList1Index][1] < list2[sizeList2Index][1])
                    currentSize = list2[sizeList2Index++];
                else
                    currentSize = list1[sizeList1Index++];
            }
        }
        orderedList[i][0] = currentSize[0];
        orderedList[i][1] = currentSize[1];
        orderedList[i][2] = currentSize[2];
    }

    return NO_ERROR;
}

void ExynosCameraMetadataConverter::m_updateFaceDetectionMetaData(__unused ExynosCameraRequestSP_sprt_t request,
                                    CameraMetadata *settings,
                                    struct camera2_shot_ext *shot_ext)
{
    // 1. validation check

    if (request == NULL) {
        CLOGE("Request is NULL");
        return;
    }

    if (settings == NULL) {
        CLOGE("[R%d] CameraMetadata is NULL", request->getKey());
        return;
    }

    if (shot_ext == NULL) {
        CLOGE("[R%d] camera2_shot_ext is NULL", request->getKey());
        return;
    }

    // 2. transform original face detection info by all perframe size control

    int32_t faceIds[NUM_OF_DETECTED_FACES] = { 0, };
    /* {leftEyeX, leftEyeY, rightEyeX, rightEyeY, mouthX, mouthY} */
    int32_t faceLandmarks[NUM_OF_DETECTED_FACES * FACE_LANDMARKS_MAX_INDEX] = { 0, };
    /* {xmin, ymin, xmax, ymax} with the absolute coordinate */
    int32_t faceRectangles[NUM_OF_DETECTED_FACES * RECTANGLE_MAX_INDEX] = { 0, };
    uint8_t faceScores[NUM_OF_DETECTED_FACES] = { 0, };
    uint8_t detectedFaceCount = 0;

    m_updateFaceDetectionMetaDataImpl(request, settings, shot_ext,
            faceIds, faceLandmarks, faceRectangles, faceScores, &detectedFaceCount);

    // 3. update face detection info to metadata

    if (detectedFaceCount == 0) return;

    switch (shot_ext->shot.dm.stats.faceDetectMode) {
        case FACEDETECT_MODE_FULL:

            META_VALIDATE_CHECK(settings);

            settings->update(ANDROID_STATISTICS_FACE_LANDMARKS, faceLandmarks,
                    detectedFaceCount * FACE_LANDMARKS_MAX_INDEX);
            CLOGV("dm.stats.faceLandmarks(%d)", detectedFaceCount);
        case FACEDETECT_MODE_SIMPLE:

            META_VALIDATE_CHECK(settings);

            settings->update(ANDROID_STATISTICS_FACE_IDS, faceIds, detectedFaceCount);
            CLOGV("dm.stats.faceIds(%d)", detectedFaceCount);

            settings->update(ANDROID_STATISTICS_FACE_RECTANGLES, faceRectangles,
                    detectedFaceCount * RECTANGLE_MAX_INDEX);
            CLOGV("dm.stats.faceRectangles(%d)", detectedFaceCount);

            settings->update(ANDROID_STATISTICS_FACE_SCORES, faceScores, detectedFaceCount);
            CLOGV("dm.stats.faceScores(%d)", detectedFaceCount);
            break;
        case FACEDETECT_MODE_OFF:
        default:
            if (detectedFaceCount > 0) {
                CLOGE("Not supported FD mode(%d)",
                        shot_ext->shot.dm.stats.faceDetectMode);
            }
            break;
    }

    return;
}

#ifdef SUPPORT_FULL_FOV_FACE_DETECT
void ExynosCameraMetadataConverter::m_updateFaceDetectionMetaDataImpl(__unused ExynosCameraRequestSP_sprt_t request,
                                    CameraMetadata *settings,
                                    struct camera2_shot_ext *shot_ext,
                                    int32_t *faceIds,
                                    int32_t *faceLandmarks,
                                    int32_t *faceRectangles,
                                    uint8_t *faceScores,
                                    uint8_t *detectedFaceCount)
{
    status_t ret = NO_ERROR;
    int32_t displayCameraId = request->getDisplayCameraId();
    if (displayCameraId < 0 || displayCameraId >= CAMERA_ID_MAX) {
        CLOGE("Invalid displayCameraId %d");
        return;
    }

    ExynosCameraParameters *parameters = m_parameters[displayCameraId];
    bool is3aaVraM2M = (parameters->getHwConnectionMode(PIPE_3AA, PIPE_VRA) == HW_CONNECTION_MODE_M2M);
    if (!is3aaVraM2M) {
        // TODO : support ds port case
        CLOGE("[R%d] can't support ds port case for face detection", request->getKey());
        return;
    }

    // skip condition
    bool needFaceInfo = m_checkSkipFaceDetection(request, settings, shot_ext);
    if (needFaceInfo == false) return;

    // get related all size
    ExynosRect maxActiveArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwBcropSize;
    ExynosRect ds3aaInputSize;
    ExynosRect fdInputSize;
    ExynosRect activeArraySize;

    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxActiveArraySize.w, (uint32_t *)&maxActiveArraySize.h);
    parameters->getStatCropSize(&hwSensorSize, &hwBcropSize);
    ret = getCropRectAlign(maxActiveArraySize.w, maxActiveArraySize.h,
            hwBcropSize.w, hwBcropSize.h,
            &activeArraySize.x, &activeArraySize.y,
            &activeArraySize.w, &activeArraySize.h,
            2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
    }
    if (parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
        ds3aaInputSize.w = hwSensorSize.w;
        ds3aaInputSize.h = hwSensorSize.h;
    } else {
        ds3aaInputSize.w = hwBcropSize.w;
        ds3aaInputSize.h = hwBcropSize.h;
    }
    parameters->getHw3aaVraInputSize(ds3aaInputSize.w, ds3aaInputSize.h, &fdInputSize);

    float activeScaleW = (float)activeArraySize.w / (float)hwBcropSize.w;
    float activeScaleH = (float)activeArraySize.h / (float)hwBcropSize.h;

    // get accumulated offset and scaleRatio
    int32_t accumulateOffsetX = 0;
    int32_t accumulateOffsetY = 0;
    float accumulateScaleRatioW = 1.0f;
    float accumulateScaleRatioH = 1.0f;
    if (parameters->isUse3aaInputCrop() == true) {
        accumulateOffsetX += (hwBcropSize.x);
        accumulateOffsetY += (hwBcropSize.y);
    }

    int32_t srcW = hwBcropSize.w;
    int32_t srcH = hwBcropSize.h;
    frame_size_info_t info;

    int32_t sPoint = FRAME_SIZE_BCROP_OUT;
    for (; sPoint < FRAME_SIZE_MAX; sPoint++) {
        if (!request->getSizeInfo((frame_size_scenario_t)sPoint, info, displayCameraId)) continue;

        bool crop = (info.rect.x || info.rect.y);
        if (crop) {
            accumulateOffsetX += (info.rect.x * accumulateScaleRatioW);
            accumulateOffsetY += (info.rect.y * accumulateScaleRatioH);
        } else {
            float scaleW = (float)srcW / (float)info.rect.w;
            float scaleH = (float)srcH / (float)info.rect.h;

            accumulateScaleRatioW *= scaleW;
            accumulateScaleRatioH *= scaleH;
        }

        srcW = info.rect.w;
        srcH = info.rect.h;
    }

    accumulateScaleRatioW *= activeScaleW;
    accumulateScaleRatioH *= activeScaleH;
    accumulateOffsetX *= activeScaleW;
    accumulateOffsetY *= activeScaleW;
    accumulateOffsetX += activeArraySize.x;
    accumulateOffsetY += activeArraySize.y;

    // 2. loop for all face rect and then transform and select
    for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
        if (shot_ext->shot.dm.stats.faceIds[i] <= 0) continue;

        switch (shot_ext->shot.dm.stats.faceDetectMode) {
        case FACEDETECT_MODE_FULL:
            // TODO: need to consider faceLandmark..
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_X]  = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][LEFT_EYE_X]  * accumulateScaleRatioW) + accumulateOffsetX);
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_Y]  = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][LEFT_EYE_Y]  * accumulateScaleRatioH) + accumulateOffsetY);
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_X] = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][RIGHT_EYE_X] * accumulateScaleRatioW) + accumulateOffsetX);
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_Y] = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][RIGHT_EYE_Y] * accumulateScaleRatioH) + accumulateOffsetY);
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_X]     = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][MOUTH_X]     * accumulateScaleRatioW) + accumulateOffsetX);
            faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_Y]     = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][MOUTH_Y]     * accumulateScaleRatioH) + accumulateOffsetY);
        case FACEDETECT_MODE_SIMPLE:
            {
                bool isOverrapped = true;

                // make original face map to bcrop size
                int32_t srcW = hwBcropSize.w;
                int32_t srcH = hwBcropSize.h;
                int32_t dstW = fdInputSize.w;
                int32_t dstH = fdInputSize.h;

                float scaleW = (float)srcW / (float)dstW;
                float scaleH = (float)srcH / (float)dstH;

                int32_t x1 = shot_ext->shot.dm.stats.faceRectangles[i][X1] * scaleW;
                int32_t x2 = shot_ext->shot.dm.stats.faceRectangles[i][X2] * scaleW;
                int32_t y1 = shot_ext->shot.dm.stats.faceRectangles[i][Y1] * scaleH;
                int32_t y2 = shot_ext->shot.dm.stats.faceRectangles[i][Y2] * scaleH;

                CLOGV("[R%d][%d] START: Id[%d], FaceRect(org[%d,%d,%d,%d] -> chg[%d,%d,%d,%d]), bcrop(%dx%d) fdInput(%dx%d)" \
                      "scale(%f/%f), ActiveRect[%d,%d,%d,%d] Accumulate(xy(%d,%d), scaleRatio(%f,%f))",
                        request->getKey(), i, faceIds[i],
                        shot_ext->shot.dm.stats.faceRectangles[i][X1],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y1],
                        shot_ext->shot.dm.stats.faceRectangles[i][X2],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y2],
                        x1, y1, x2, y2,
                        srcW, srcH, dstW, dstH,
                        scaleW, scaleH,
                        activeArraySize.x, activeArraySize.y,
                        activeArraySize.w, activeArraySize.h,
                        accumulateOffsetX, accumulateOffsetY,
                        accumulateScaleRatioW, accumulateScaleRatioH);

                sPoint = FRAME_SIZE_BCROP_OUT;
                // transform face map to display size and remove "not overlapped rect"
                for (; sPoint < FRAME_SIZE_MAX; sPoint++) {
                    if (!request->getSizeInfo((frame_size_scenario_t)sPoint, info, displayCameraId)) continue;

                    bool crop = (info.rect.x || info.rect.y);
                    if (crop) {
                        // 1. crop case
                        //   - find out this face rect is available
                        //   - if it's not overrapped, skip that face rect.
                        //   - transform this face rect to relative face rect based on crop area

                        ExynosRect2 src, dst, overlapped;

                        // try to find overlapped rectangle
                        src.x1 = info.rect.x;
                        src.x2 = src.x1 + info.rect.w;
                        src.y1 = info.rect.y;
                        src.y2 = src.y1 + info.rect.h;

                        dst.x1 = x1; dst.x2 = x2; dst.y1 = y1; dst.y2 = y2;

                        isOverrapped = getOverlapInfoFromTwoRects(src, dst, &overlapped);
                        if (!isOverrapped) {
                            CLOGV("[R%d] No overlapped faceIds[%d][S%d] Id(%d), rect(%d,%d,%d,%d) by info(%d,%d,%d,%d), src(%dx%d)",
                                    request->getKey(), i, sPoint,
                                    faceIds[i], x1, y1, x2, y2,
                                    info.rect.x, info.rect.y,
                                    info.rect.w, info.rect.h,
                                    srcW, srcH);
                            break;
                        }

                        // transform x,y
                        x1 -= info.rect.x;
                        y1 -= info.rect.y;
                        x2 -= info.rect.x;
                        y2 -= info.rect.y;
                    } else {
                        // 2. scaling case
                        //   - just scaling by its scaleRatio

                        scaleW = (float)srcW / (float)info.rect.w;
                        scaleH = (float)srcH / (float)info.rect.h;

                        x1 /= scaleW; x2 /= scaleW; y1 /= scaleH; y2 /= scaleH;

                        CLOGV("[R%d] rect[%d,%d,%d,%d] by src(%dx%d)->dst(%dx%d), scale(%f,%f)",
                            request->getKey(),
                            x1, y1, x2, y2,
                            srcW, srcH, info.rect.w, info.rect.h,
                            scaleW, scaleH);
                    }

                    CLOGV("[R%d][%d] ING(scenario:%d, info(%d,%d,%d,%d)): Id[%d], FaceRect(org[%d,%d,%d,%d] -> chg[%d,%d,%d,%d]), src(%dx%d)" \
                            "scale(%f/%f), ActiveRect[%d,%d,%d,%d] Accumulate(xy(%d,%d), scaleRatio(%f,%f))",
                            request->getKey(), i, sPoint,
                            info.rect.x, info.rect.y,
                            info.rect.w, info.rect.h,
                            faceIds[i],
                            shot_ext->shot.dm.stats.faceRectangles[i][X1],
                            shot_ext->shot.dm.stats.faceRectangles[i][Y1],
                            shot_ext->shot.dm.stats.faceRectangles[i][X2],
                            shot_ext->shot.dm.stats.faceRectangles[i][Y2],
                            x1, y1, x2, y2,
                            srcW, srcH, scaleW, scaleH,
                            activeArraySize.x, activeArraySize.y,
                            activeArraySize.w, activeArraySize.h,
                            accumulateOffsetX, accumulateOffsetY,
                            accumulateScaleRatioW, accumulateScaleRatioH);

                    srcW = info.rect.w;
                    srcH = info.rect.h;

                    // validation check
                    if (x1 < 0) x1 = 0;
                    if (x2 < 0) x2 = 0;
                    if (y1 < 0) y1 = 0;
                    if (y2 < 0) y2 = 0;

                    if (x1 > srcW) x1 = srcW;
                    if (x2 > srcW) x2 = srcW;
                    if (y1 > srcH) y1 = srcH;
                    if (y2 > srcH) y2 = srcH;
                }

                if (!isOverrapped) continue;

                int idx = (*detectedFaceCount);

                faceIds[idx] = shot_ext->shot.dm.stats.faceIds[i];
                /* Normalize the score into the range of [0, 100] */
                faceScores[idx] = (uint8_t) ((float)shot_ext->shot.dm.stats.faceScores[i] / (255.0f / 100.0f));

                x1 = x1 * accumulateScaleRatioW + accumulateOffsetX;
                y1 = y1 * accumulateScaleRatioH + accumulateOffsetY;
                x2 = x2 * accumulateScaleRatioW + accumulateOffsetX;
                y2 = y2 * accumulateScaleRatioH + accumulateOffsetY;

                // validation check
                if (x1 < 0) x1 = 0;
                if (y1 < 0) y1 = 0;
                if (x2 < 0) x2 = 0;
                if (y2 < 0) y2 = 0;

                if (x1 > activeArraySize.w) x1 = activeArraySize.w;
                if (y1 > activeArraySize.h) y1 = activeArraySize.h;
                if (x2 > activeArraySize.w) x2 = activeArraySize.w;
                if (y2 > activeArraySize.h) y2 = activeArraySize.h;

                faceRectangles[(idx * RECTANGLE_MAX_INDEX) + X1] = x1;
                faceRectangles[(idx * RECTANGLE_MAX_INDEX) + Y1] = y1;
                faceRectangles[(idx * RECTANGLE_MAX_INDEX) + X2] = x2;
                faceRectangles[(idx * RECTANGLE_MAX_INDEX) + Y2] = y2;

                CLOGV("[R%d][Num:%d][Idx:%d]: ID[%d], Score[%d], Rect[%d,%d,%d,%d] ActiveScale(%f/%f), Accumulate(xy(%d,%d), scaleRatio(%f,%f))",
                        request->getKey(), (*detectedFaceCount), idx, faceIds[idx], faceScores[idx],
                        faceRectangles[(idx * RECTANGLE_MAX_INDEX) + X1],
                        faceRectangles[(idx * RECTANGLE_MAX_INDEX) + Y1],
                        faceRectangles[(idx * RECTANGLE_MAX_INDEX) + X2],
                        faceRectangles[(idx * RECTANGLE_MAX_INDEX) + Y2],
                        activeScaleW, activeScaleH,
                        accumulateOffsetX, accumulateOffsetY,
                        accumulateScaleRatioW, accumulateScaleRatioH);
                (*detectedFaceCount)++;
            }
            break;
        case FACEDETECT_MODE_OFF:
            continue;
        default:
            CLOGE("Not supported FD mode(%d)",
                    shot_ext->shot.dm.stats.faceDetectMode);
            break;
        }
    }
}
#else
void ExynosCameraMetadataConverter::m_updateFaceDetectionMetaDataImpl(__unused ExynosCameraRequestSP_sprt_t requestInfo,
                                    CameraMetadata *settings,
                                    struct camera2_shot_ext *shot_ext,
                                    int32_t *faceIds,
                                    int32_t *faceLandmarks,
                                    int32_t *faceRectangles,
                                    uint8_t *faceScores,
                                    uint8_t *detectedFaceCount)
{
    status_t ret = NO_ERROR;
    ExynosRect activeArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwActiveArraySize;
    ExynosRect hwBcropSize;
    ExynosRect hwBdsSize;
    ExynosRect mcscInputSize;
    ExynosRect hwYuvSize;
    ExynosRect dsInputSize;
    ExynosRect ds3aaInputSize;
    ExynosRect vraInputSize;
    int32_t displayCameraId = requestInfo->getDisplayCameraId();
    if (displayCameraId < 0 || displayCameraId >= CAMERA_ID_MAX) {
        CLOGV("Invalid displayCameraId %d");
        return;
    }

    ExynosCameraParameters *parameters = m_parameters[displayCameraId];
    __unused int dsInputPortId = parameters->getDsInputPortId(false);
    int offsetX = 0, offsetY = 0;
    bool is3aaVraM2M = (parameters->getHwConnectionMode(PIPE_3AA, PIPE_VRA) == HW_CONNECTION_MODE_M2M);

    /* Original FD region : based on FD input size (e.g. preview size)
     * Camera Metadata FD region : based on sensor active array size (e.g. max sensor size)
     * The FD region from firmware must be scaled into the size based on sensor active array size
     */
    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
    parameters->getStatCropSize(&hwSensorSize, &hwBcropSize);
    parameters->getPreviewBdsSize(&hwBdsSize);

#ifdef CAPTURE_FD_SYNC_WITH_PREVIEW
    if (shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] == MCSC_PORT_NONE) {
        parameters->getYuvVendorSize(&hwYuvSize.w, &hwYuvSize.h, dsInputPortId, hwBdsSize);
    } else
#endif
    {
        parameters->getYuvVendorSize(&hwYuvSize.w, &hwYuvSize.h,
                                     shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS],
                                     hwBdsSize);
    }

    if (is3aaVraM2M) {
        // TODO:  need to change to logic to handle the per frame info properly
        if (parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
            ds3aaInputSize.w = hwSensorSize.w;
            ds3aaInputSize.h = hwSensorSize.h;
        } else {
            ds3aaInputSize.w = hwBcropSize.w;
            ds3aaInputSize.h = hwBcropSize.h;
        }
        ds3aaInputSize.x = 0;
        ds3aaInputSize.y = 0;
        parameters->getHw3aaVraInputSize(ds3aaInputSize.w, ds3aaInputSize.h, &vraInputSize);
    } else if (parameters->getHwConnectionMode(PIPE_MCSC, PIPE_VRA) == HW_CONNECTION_MODE_M2M) {
#ifdef CAPTURE_FD_SYNC_WITH_PREVIEW
        if (shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] == MCSC_PORT_NONE) {
            parameters->getHwVraInputSize(&vraInputSize.w, &vraInputSize.h, &shot_ext->shot, (mcsc_port)dsInputPortId);
        } else
#endif
        {
            parameters->getHwVraInputSize(&vraInputSize.w, &vraInputSize.h, &shot_ext->shot);
        }
    } else {
        parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&vraInputSize.w, (uint32_t *)&vraInputSize.h, 0);
    }

    CLOGV("[F(%d)]ActiveArraySize %dx%d(%d) HWSensorSize %dx%d(%d) HWBcropSize %d,%d %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            activeArraySize.w, activeArraySize.h, SIZE_RATIO(activeArraySize.w, activeArraySize.h),
            hwSensorSize.w, hwSensorSize.h, SIZE_RATIO(hwSensorSize.w, hwSensorSize.h),
            hwBcropSize.x, hwBcropSize.y, hwBcropSize.w, hwBcropSize.h, SIZE_RATIO(hwBcropSize.w, hwBcropSize.h));
    CLOGV("[F(%d)]HWBdsSize %dx%d(%d) HWYUVSize %dx%d(%d) VRAInputSize %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            hwBdsSize.w, hwBdsSize.h, SIZE_RATIO(hwBdsSize.w, hwBdsSize.h),
            hwYuvSize.w, hwYuvSize.h, SIZE_RATIO(hwYuvSize.w, hwYuvSize.h),
            vraInputSize.w, vraInputSize.h, SIZE_RATIO(vraInputSize.w, vraInputSize.h));

    /* Calculate HW active array size for current sensor aspect ratio
     * based on active array size
     */
    ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
            hwSensorSize.w, hwSensorSize.h,
            &hwActiveArraySize.x, &hwActiveArraySize.y,
            &hwActiveArraySize.w, &hwActiveArraySize.h,
            2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
        return;
    }

    if (!is3aaVraM2M) {
        /* Calculate HW YUV crop region */
        ret = getCropRectAlign(hwBdsSize.w, hwBdsSize.h,
                               hwYuvSize.w, hwYuvSize.h,
                               &mcscInputSize.x, &mcscInputSize.y,
                               &mcscInputSize.w, &mcscInputSize.h,
                               2, 2, 1.0f);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                    hwBdsSize.w, hwBdsSize.h,
                    hwYuvSize.w, hwYuvSize.h);
            return;
        }
    }

    /* Calculate final DS input crop region for VRA with considering preview margin */
    if (is3aaVraM2M) {
        // TODO: Need to consider fusion size scenario
        ret = getCropRectAlign(ds3aaInputSize.w, ds3aaInputSize.h,
                           vraInputSize.w, vraInputSize.h,
                           &dsInputSize.x, &dsInputSize.y,
                           &dsInputSize.w, &dsInputSize.h,
                           2, 2, 1.0f);
    } else {
        ret = getCropRectAlign(hwYuvSize.w, hwYuvSize.h,
                           vraInputSize.w, vraInputSize.h,
                           &dsInputSize.x, &dsInputSize.y,
                           &dsInputSize.w, &dsInputSize.h,
                           2, 2, 1.0f);
    }
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                dsInputSize.w, dsInputSize.h,
                vraInputSize.w, vraInputSize.h);
        return;
    }

#ifdef USE_DUAL_CAMERA
    int cameraScenario = m_configurations->getScenario();
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
    int32_t slaveCamId = (requestInfo != NULL) ? requestInfo->getSlaveCamId() : CAMERA_ID_BACK_2;

    if (cameraScenario == SCENARIO_DUAL_REAR_ZOOM && (!is3aaVraM2M)
        && dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION) {
        parameters->getVendorRatioCropSizeForVRA(&dsInputSize);

        /* Add DS input crop offset */
        if (hwYuvSize.w < dsInputSize.w) {
            offsetX = 0;
        } else {
            offsetX = (hwYuvSize.w - dsInputSize.w)/2;
        }

        if (hwYuvSize.h < dsInputSize.h) {
            offsetY = 0;
        } else {
            offsetY = (hwYuvSize.h - dsInputSize.h)/2;
        }
    }
    else
#endif
    {
        /* Add DS input crop offset */
        offsetX = dsInputSize.x;
        offsetY = dsInputSize.y;
    }

    CLOGV("[F(%d)]HwActiveArraySize %d,%d %dx%d(%d) MCSCInputSize %d,%d %dx%d(%d) DSInputSize %d,%d %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            hwActiveArraySize.x, hwActiveArraySize.y, hwActiveArraySize.w, hwActiveArraySize.h,
            SIZE_RATIO(hwActiveArraySize.w, hwActiveArraySize.h),
            mcscInputSize.x, mcscInputSize.y, mcscInputSize.w, mcscInputSize.h,
            SIZE_RATIO(mcscInputSize.w, mcscInputSize.h),
            dsInputSize.x, dsInputSize.y, dsInputSize.w, dsInputSize.h,
            SIZE_RATIO(dsInputSize.w, dsInputSize.h));

    /* Calculate DS scaling ratio */
    float dsScaleRatioW = 0.0f, dsScaleRatioH = 0.0f;
    dsScaleRatioW = (float) dsInputSize.w / (float) vraInputSize.w;
    dsScaleRatioH = (float) dsInputSize.h / (float) vraInputSize.h;

    float yuvScaleRatioW = 1.0f, yuvScaleRatioH = 1.0f;
    float bdsScaleRatioW = 1.0f, bdsScaleRatioH = 1.0f;

    if (!is3aaVraM2M) {
        /* Calculate YUV scaling ratio */

        yuvScaleRatioW = (float) mcscInputSize.w / (float) hwYuvSize.w;
        yuvScaleRatioH = (float) mcscInputSize.h / (float) hwYuvSize.h;

        /* Scale offset with YUV scaling ratio */
        offsetX *= yuvScaleRatioW;
        offsetY *= yuvScaleRatioH;

        /* Add MCSC input crop offset */
        offsetX += mcscInputSize.x;
        offsetY += mcscInputSize.y;

        /* Calculate BDS scaling ratio */
        bdsScaleRatioW = (float) hwBcropSize.w / (float) hwBdsSize.w;
        bdsScaleRatioH = (float) hwBcropSize.h / (float) hwBdsSize.h;

        /* Scale offset with BDS scaling ratio */
        offsetX *= bdsScaleRatioW;
        offsetY *= bdsScaleRatioH;
    }

    /* Add HW bcrop offset */
    offsetX += hwBcropSize.x;
    offsetY += hwBcropSize.y;

    /* Calculate Sensor scaling ratio */
    float sensorScaleRatioW = 0.0f, sensorScaleRatioH = 0.0f;
    sensorScaleRatioW = (float) hwActiveArraySize.w / (float) hwSensorSize.w;
    sensorScaleRatioH = (float) hwActiveArraySize.h / (float) hwSensorSize.h;

    /* Scale offset with Sensor scaling ratio */
    offsetX *= sensorScaleRatioW;
    offsetY *= sensorScaleRatioH;

    /* Add HW active array offset */
    offsetX += hwActiveArraySize.x;
    offsetY += hwActiveArraySize.y;

    /* Calculate final scale ratio */
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;
    scaleRatioW = dsScaleRatioW * yuvScaleRatioW * bdsScaleRatioW * sensorScaleRatioW;
    scaleRatioH = dsScaleRatioH * yuvScaleRatioH * bdsScaleRatioH * sensorScaleRatioH;

    CLOGV("[F(%d)]HwActiveArraySize %d,%d %dx%d(%d) DSInputSize %d,%d %dx%d(%d) Offset %d %d is3aaVraM2M %d"
        "dsScaleRatio (%f %f) bdsScaleRatio (%f %f) sensorScaleRatio (%f %f) scaleRatio (%f %f) hwSensorSize (%d %d)",
            shot_ext->shot.dm.request.frameCount,
            hwActiveArraySize.x, hwActiveArraySize.y, hwActiveArraySize.w, hwActiveArraySize.h,
            SIZE_RATIO(hwActiveArraySize.w, hwActiveArraySize.h),
            dsInputSize.x, dsInputSize.y, dsInputSize.w, dsInputSize.h,
            SIZE_RATIO(dsInputSize.w, dsInputSize.h),
            offsetX, offsetY, is3aaVraM2M, dsScaleRatioW, dsScaleRatioH, bdsScaleRatioW, bdsScaleRatioH,
            sensorScaleRatioW, sensorScaleRatioH,
            scaleRatioW, scaleRatioH, hwSensorSize.w, hwSensorSize.h);

    /* Apply offset and scale ratio to FD region
     * FD region' = (FD region x scaleRatioW,H) + offsetX,Y
     */
    bool needFaceInfo = m_checkSkipFaceDetection(requestInfo, settings, shot_ext);
    for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
        if (shot_ext->shot.dm.stats.faceIds[i] > 0
            && (needFaceInfo)) {
            switch (shot_ext->shot.dm.stats.faceDetectMode) {
                case FACEDETECT_MODE_FULL:
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_X]  = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][LEFT_EYE_X]  * scaleRatioW) + offsetX);
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_Y]  = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][LEFT_EYE_Y]  * scaleRatioH) + offsetY);
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_X] = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][RIGHT_EYE_X] * scaleRatioW) + offsetX);
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_Y] = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][RIGHT_EYE_Y] * scaleRatioH) + offsetY);
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_X]     = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][MOUTH_X]     * scaleRatioW) + offsetX);
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_Y]     = (int32_t)((shot_ext->shot.dm.stats.faceLandmarks[i][MOUTH_Y]     * scaleRatioH) + offsetY);
                case FACEDETECT_MODE_SIMPLE:
                    faceIds[i] = shot_ext->shot.dm.stats.faceIds[i];
                    /* Normalize the score into the range of [0, 100] */
                    faceScores[i] = (uint8_t) ((float)shot_ext->shot.dm.stats.faceScores[i] / (255.0f / 100.0f));

                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][X1] * scaleRatioW) + offsetX);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][Y1] * scaleRatioH) + offsetY);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][X2] * scaleRatioW) + offsetX);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][Y2] * scaleRatioH) + offsetY);
                    CLOGV("faceIds[%d](%d), faceScores[%d](%d), original(%d,%d,%d,%d), converted(%d,%d,%d,%d)",
                        i, faceIds[i], i, faceScores[i],
                        shot_ext->shot.dm.stats.faceRectangles[i][X1],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y1],
                        shot_ext->shot.dm.stats.faceRectangles[i][X2],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y2],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2]);

                    (*detectedFaceCount)++;

                    /*
                    if (shot_ext->shot.dm.stats.faceDetectMode == FACEDETECT_MODE_FULL) {
                        m_checkFacePostion(&faceRectangles[(i * RECTANGLE_MAX_INDEX)],
                                           &faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX)],
                                           shot_ext->shot.dm.stats.faceRectangles[i]);
                    }
                    */

                    break;
                case FACEDETECT_MODE_OFF:
                    faceScores[i] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX)+ Y2] = 0;
                    break;
                default:
                    CLOGE("Not supported FD mode(%d)",
                            shot_ext->shot.dm.stats.faceDetectMode);
                    break;
            }
        } else {
            faceIds[i] = 0;
            faceScores[i] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2] = 0;
        }
    }
}
#endif

void ExynosCameraMetadataConverter::m_convertActiveArrayTo3AARegion(ExynosRect2 *region, const char *str)
{
    status_t ret = NO_ERROR;
    ExynosRect activeArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwActiveArraySize;
    ExynosRect hwBcropSize;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;

    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
    m_parameters[m_cameraId]->getStatCropSize(&hwSensorSize, &hwBcropSize);

    CLOGV("[%s] ActiveArraySize %dx%d(%d) Region %d,%d %d,%d %dx%d(%d) HWSensorSize %dx%d(%d) HWBcropSize %d,%d %dx%d(%d)",
            str,
            activeArraySize.w, activeArraySize.h, SIZE_RATIO(activeArraySize.w, activeArraySize.h),
            region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1, region->y2 - region->y1, SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1),
            hwSensorSize.w, hwSensorSize.h, SIZE_RATIO(hwSensorSize.w, hwSensorSize.h),
            hwBcropSize.x, hwBcropSize.y, hwBcropSize.w, hwBcropSize.h, SIZE_RATIO(hwBcropSize.w, hwBcropSize.h));

    /* Calculate HW active array size for current sensor aspect ratio
       based on active array size
     */
    ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
                           hwSensorSize.w, hwSensorSize.h,
                           &hwActiveArraySize.x, &hwActiveArraySize.y,
                           &hwActiveArraySize.w, &hwActiveArraySize.h,
                           2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
        return;
    }

    /* Scale down the 3AA region & HW active array size
       to adjust them to the 3AA input size without sensor margin
     */
    scaleRatioW = (float) hwSensorSize.w / (float) hwActiveArraySize.w;
    scaleRatioH = (float) hwSensorSize.h / (float) hwActiveArraySize.h;

    region->x1 = (int) (((float) region->x1) * scaleRatioW);
    region->y1 = (int) (((float) region->y1) * scaleRatioH);
    region->x2 = (int) (((float) region->x2) * scaleRatioW);
    region->y2 = (int) (((float) region->y2) * scaleRatioH);

    hwActiveArraySize.x = (int) (((float) hwActiveArraySize.x) * scaleRatioW);
    hwActiveArraySize.y = (int) (((float) hwActiveArraySize.y) * scaleRatioH);
    hwActiveArraySize.w = (int) (((float) hwActiveArraySize.w) * scaleRatioW);
    hwActiveArraySize.h = (int) (((float) hwActiveArraySize.h) * scaleRatioH);

    /* Remove HW active array size offset */
    /* Top-left */
    if (region->x1 < hwActiveArraySize.x) region->x1 = 0;
    else region->x1 -= hwActiveArraySize.x;
    if (region->y1 < hwActiveArraySize.y) region->y1 = 0;
    else region->y1 -= hwActiveArraySize.y;
    if (region->x2 < hwActiveArraySize.x) region->x2 = 0;
    else region->x2 -= hwActiveArraySize.x;
    if (region->y2 < hwActiveArraySize.y) region->y2 = 0;
    else region->y2 -= hwActiveArraySize.y;

    /* Bottom-right */
    if (region->x1 > hwActiveArraySize.w) region->x1 = hwActiveArraySize.w;
    if (region->y1 > hwActiveArraySize.h) region->y1 = hwActiveArraySize.h;
    if (region->x2 > hwActiveArraySize.w) region->x2 = hwActiveArraySize.w;
    if (region->y2 > hwActiveArraySize.h) region->y2 = hwActiveArraySize.h;

    /* Adjust the boundary of HW bcrop size */
    /* Top-left */
    if (region->x1 < hwBcropSize.x) region->x1 = 0;
    else region->x1 -= hwBcropSize.x;
    if (region->y1 < hwBcropSize.y) region->y1 = 0;
    else region->y1 -= hwBcropSize.y;
    if (region->x2 < hwBcropSize.x) region->x2 = 0;
    else region->x2 -= hwBcropSize.x;
    if (region->y2 < hwBcropSize.y) region->y2 = 0;
    else region->y2 -= hwBcropSize.y;

    /* Bottom-right */
    if (region->x1 > hwBcropSize.w) region->x1 = hwBcropSize.w;
    if (region->y1 > hwBcropSize.h) region->y1 = hwBcropSize.h;
    if (region->x2 > hwBcropSize.w) region->x2 = hwBcropSize.w;
    if (region->y2 > hwBcropSize.h) region->y2 = hwBcropSize.h;

    CLOGV("[%s] Region %d,%d %d,%d %dx%d(%d)",
            str,
            region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1, region->y2 - region->y1, SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1));
}

void ExynosCameraMetadataConverter::m_convert3AAToActiveArrayRegion(ExynosRect2 *region, const char *str)
{
    status_t ret = NO_ERROR;
    ExynosRect activeArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwActiveArraySize;
    ExynosRect hwBcropSize;
    int offsetX = 0, offsetY = 0;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;

    m_parameters[m_cameraId]->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
    m_parameters[m_cameraId]->getStatCropSize(&hwSensorSize, &hwBcropSize);

    /* Calculate HW active array size for current sensor aspect ratio
    based on active array size
    */
    ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
                           hwSensorSize.w, hwSensorSize.h,
                           &hwActiveArraySize.x, &hwActiveArraySize.y,
                           &hwActiveArraySize.w, &hwActiveArraySize.h,
                           2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
        return;
    }

    /* Add HW bcrop offset */
    offsetX = hwBcropSize.x;
    offsetY = hwBcropSize.y;

    /* Scale down HW active array offset */
    scaleRatioW = (float) hwSensorSize.w / (float) hwActiveArraySize.w;
    scaleRatioH = (float) hwSensorSize.h / (float) hwActiveArraySize.h;

    hwActiveArraySize.x = (int) (((float) hwActiveArraySize.x) * scaleRatioW);
    hwActiveArraySize.y = (int) (((float) hwActiveArraySize.y) * scaleRatioH);

    /* Add HW active array size offset */
    offsetX += hwActiveArraySize.x;
    offsetY += hwActiveArraySize.y;

    region->x1 = (region->x1 + offsetX) / scaleRatioW;
    region->y1 = (region->y1 + offsetY) / scaleRatioH;
    region->x2 = (region->x2 + offsetX) / scaleRatioW;
    region->y2 = (region->y2 + offsetY) / scaleRatioH;

    CLOGV("[%s] Region %d,%d %d,%d %dx%d(%d)",
            str,
            region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1, region->y2 - region->y1, SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1));
}

void ExynosCameraMetadataConverter::setMetaCtlSceneMode(struct camera2_shot_ext *shot_ext,
                                                        enum aa_scene_mode sceneMode, int mode)
{
    enum processing_mode default_edge_mode = PROCESSING_MODE_FAST;
    enum processing_mode default_noise_mode = PROCESSING_MODE_FAST;
    int default_edge_strength = 5;
    int default_noise_strength = 5;

    shot_ext->shot.ctl.aa.sceneMode = sceneMode;
    if (mode != 0) {
        shot_ext->shot.ctl.aa.mode = (enum aa_mode)mode;
    }

    switch (sceneMode) {
    case AA_SCENE_MODE_ACTION:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_PORTRAIT:
    case AA_SCENE_MODE_LANDSCAPE:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_NIGHT:
        /* AE_LOCK is prohibited */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF ||
            shot_ext->shot.ctl.aa.aeLock == AA_AE_LOCK_ON) {
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
        }

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_NIGHT_PORTRAIT:
    case AA_SCENE_MODE_THEATRE:
    case AA_SCENE_MODE_BEACH:
    case AA_SCENE_MODE_SNOW:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_SUNSET:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_DAYLIGHT;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_STEADYPHOTO:
    case AA_SCENE_MODE_FIREWORKS:
    case AA_SCENE_MODE_SPORTS:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_PARTY:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_MANUAL;
        shot_ext->shot.ctl.aa.vendor_isoValue = 200;
        shot_ext->shot.ctl.sensor.sensitivity = 200;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 4; /* "4" is default + 1. */
        break;
    case AA_SCENE_MODE_CANDLELIGHT:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_AQUA:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
        break;
    case AA_SCENE_MODE_FACE_PRIORITY:
    default:
        break;
    }
}

status_t ExynosCameraMetadataConverter::checkMetaValid(camera_metadata_tag_t tag, const void *data)
{
    status_t ret = NO_ERROR;
    camera_metadata_entry_t entry;

    int32_t value = 0;
    const int32_t *i32Range = NULL;

    if (m_staticInfo.exists(tag) == false) {
        CLOGE("Cannot find entry, tag(%d)", tag);
        return BAD_VALUE;
    }

    entry = m_staticInfo.find(tag);

    /* TODO: handle all tags
     *       need type check
     */
    switch (tag) {
    case ANDROID_SENSOR_INFO_SENSITIVITY_RANGE:
         value = *(int32_t *)data;
         i32Range = entry.data.i32;
        if (value < i32Range[0] || value > i32Range[1]) {
            CLOGE("Invalid Sensitivity value(%d), range(%d, %d)",
                value, i32Range[0], i32Range[1]);
            ret = BAD_VALUE;
        }
        break;
    default:
        CLOGE("Tag (%d) is not implemented", tag);
        break;
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::getDefaultSetting(camera_metadata_tag_t tag, void *data)
{
    status_t ret = NO_ERROR;
    camera_metadata_entry_t entry;

    const int32_t *i32Range = NULL;

    if (m_defaultRequestSetting.exists(tag) == false) {
        CLOGE("Cannot find entry, tag(%d)", tag);
        return BAD_VALUE;
    }

    entry = m_defaultRequestSetting.find(tag);

    /* TODO: handle all tags
     *       need type check
     */
    switch (tag) {
    case ANDROID_SENSOR_SENSITIVITY:
         i32Range = entry.data.i32;
         *(int32_t *)data = i32Range[0];
        break;
    default:
        CLOGE("Tag (%d) is not implemented", tag);
        break;
    }

    return ret;
}

void ExynosCameraMetadataConverter::updateFaceDetectionMetaData(ExynosCameraRequestSP_sprt_t request)
{
    CameraMetadata *meta = NULL;
    struct camera2_shot_ext *shot_ext = NULL;
    camera_metadata_entry_t entry;

    request->setRequestLock();
    meta = request->getServiceMeta();
    if (meta->isEmpty() == true) {
        CLOGE("[R%d]CameraMetadata is NULL", request->getKey());
        goto updateFaceDetectionMetaData_end;
    }

    shot_ext = request->getServiceShot();
    if (shot_ext == NULL) {
        CLOGE("[R%d]shot_ext is NULL", request->getKey());
        goto updateFaceDetectionMetaData_end;
    }

    /* Delete existing metadata */
    entry = meta->find(ANDROID_STATISTICS_FACE_LANDMARKS);
    if (entry.count > 0) {
        CLOGV("[R%d]Erase LANDMARKS. count %zu", request->getKey(), entry.count);
        meta->erase(ANDROID_STATISTICS_FACE_LANDMARKS);
    }
    entry = meta->find(ANDROID_STATISTICS_FACE_IDS);
    if (entry.count > 0) {
        CLOGV("[R%d]Erase IDS. count %zu", request->getKey(), entry.count);
        meta->erase(ANDROID_STATISTICS_FACE_IDS);
    }
    entry = meta->find(ANDROID_STATISTICS_FACE_RECTANGLES);
    if (entry.count > 0) {
        CLOGV("[R%d]Erase RECTANGLES. count %zu", request->getKey(), entry.count);
        meta->erase(ANDROID_STATISTICS_FACE_RECTANGLES);
    }
    entry = meta->find(ANDROID_STATISTICS_FACE_SCORES);
    if (entry.count > 0) {
        CLOGV("[R%d]Erase SCORES. count %zu", request->getKey(), entry.count);
        meta->erase(ANDROID_STATISTICS_FACE_SCORES);
    }

    if (shot_ext->shot.ctl.stats.faceDetectMode > FACEDETECT_MODE_OFF) {
        m_updateFaceDetectionMetaData(request, meta, shot_ext);
    }
updateFaceDetectionMetaData_end:
    request->setRequestUnlock();
}

int32_t ExynosCameraMetadataConverter::m_getFrameInfoIndexForTimeStamp(uint64_t timeStamp)
{
    int mapIndex = (m_frameCountMapIndex + FRAMECOUNT_MAP_LENGTH - 1) % FRAMECOUNT_MAP_LENGTH;

    for (int i = 0; i < FRAMECOUNT_MAP_LENGTH; i++) {
        if (m_frameCountMap[mapIndex][TIMESTAMP] == timeStamp)
            return mapIndex;
        mapIndex = (mapIndex + FRAMECOUNT_MAP_LENGTH - 1) % FRAMECOUNT_MAP_LENGTH;
    }

    return -1;
}

int ExynosCameraMetadataConverter::getExynosCameraDeviceInfoSize()
{
    int size = getExynosCameraVendorDeviceInfoSize();
    return size;
}

HAL_CameraInfo_t *ExynosCameraMetadataConverter::getExynosCameraDeviceInfoByCamIndex(int camIndex)
{
    HAL_CameraInfo_t *CameraInfo = getExynosCameraVendorDeviceInfoByCamIndex(camIndex);
    return CameraInfo;
}

bool ExynosCameraMetadataConverter::m_checkSkipFaceDetection(__unused ExynosCameraRequestSP_sprt_t request,
                                    CameraMetadata *settings,
                                    struct camera2_shot_ext *shot_ext)
{
    camera_metadata_entry_t entry_Intent;
    int32_t captureIntent = 0;

    entry_Intent = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
    if (entry_Intent.count > 0) {
        captureIntent = entry_Intent.data.u8[0];
    }

    return ((m_rectUiSkipCount <= 0) || (captureIntent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE));
}

bool ExynosCameraMetadataConverter::m_checkFacePostion(int32_t *faceRectangles, int32_t *faceLandmarks, uint32_t *shotFaceLandmarks)
{
    #define STR_CASE_SIZE (32)

    bool ret = true;
    char strCase[STR_CASE_SIZE];

    if (faceLandmarks[LEFT_EYE_X]  < faceRectangles[X1] ||
        faceLandmarks[RIGHT_EYE_X] < faceRectangles[X1] ||
        faceLandmarks[MOUTH_X]     < faceRectangles[X1]) {
        strncpy(strCase, "Left side", STR_CASE_SIZE);
        ret = false;
    }

    if (faceLandmarks[LEFT_EYE_X]  > faceRectangles[X2] ||
        faceLandmarks[RIGHT_EYE_X] > faceRectangles[X2] ||
        faceLandmarks[MOUTH_X]     > faceRectangles[X2]) {
        strncpy(strCase, "Right side", STR_CASE_SIZE);
        ret = false;
    }

    if (faceLandmarks[LEFT_EYE_Y]  < faceRectangles[Y1] ||
        faceLandmarks[RIGHT_EYE_Y] < faceRectangles[Y1] ||
        faceLandmarks[MOUTH_Y]     < faceRectangles[Y1]) {
        strncpy(strCase, "Up side", STR_CASE_SIZE);
        ret = false;
    }

    if (faceLandmarks[LEFT_EYE_Y]  > faceRectangles[Y2] ||
        faceLandmarks[RIGHT_EYE_Y] > faceRectangles[Y2] ||
        faceLandmarks[MOUTH_Y]     > faceRectangles[Y2]) {
        strncpy(strCase, "Down side", STR_CASE_SIZE);
        ret = false;
    }

    if (ret == false) {
        CLOGW("strange face detect(%s) : eyeL(%d, %d), eyeR(%d, %d), mouth(%d, %d), face(x[%d ~ %d] / y[%d ~ %d]",
            strCase,
            faceLandmarks[LEFT_EYE_X],
            faceLandmarks[LEFT_EYE_Y],
            faceLandmarks[RIGHT_EYE_X],
            faceLandmarks[RIGHT_EYE_Y],
            faceLandmarks[MOUTH_X],
            faceLandmarks[MOUTH_Y],
            faceRectangles[X1],
            faceRectangles[X2],
            faceRectangles[Y1],
            faceRectangles[Y2]);

        CLOGW("because shotFaceLandmarks : eyeL(%d, %d), eyeR(%d, %d), mouth(%d, %d), face(x[%d ~ %d] / y[%d ~ %d]",
            shotFaceLandmarks[LEFT_EYE_X],
            shotFaceLandmarks[LEFT_EYE_Y],
            shotFaceLandmarks[RIGHT_EYE_X],
            shotFaceLandmarks[RIGHT_EYE_Y],
            shotFaceLandmarks[MOUTH_X],
            shotFaceLandmarks[MOUTH_Y],
            shotFaceLandmarks[X1],
            shotFaceLandmarks[X2],
            shotFaceLandmarks[Y1],
            shotFaceLandmarks[Y2]);
    }

    return ret;
}


}; /* namespace android */
