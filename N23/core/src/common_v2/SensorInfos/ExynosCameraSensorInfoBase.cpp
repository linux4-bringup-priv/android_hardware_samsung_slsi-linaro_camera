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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraSensorInfoBase"
#include <log/log.h>

#include "ExynosCameraSensorInfoBase.h"
#include "ExynosExif.h"

namespace android {

#ifdef SENSOR_NAME_GET_FROM_FILE
int g_rearSensorId = -1;
int g_rear2SensorId = -1;
int g_rear3SensorId = -1;
int g_rear4SensorId = -1;
int g_rearTofSensorId = -1;
int g_frontSensorId = -1;
int g_front2SensorId = -1;
int g_front3SensorId = -1;
int g_front4SensorId = -1;
int g_frontTofSensorId = -1;
int g_secureSensorId = -1;
#endif

static camera_metadata_rational UNIT_MATRIX[] =
{
    {1024, 1024}, {   0, 1024}, {   0, 1024},
    {   0, 1024}, {1024, 1024}, {   0, 1024},
    {   0, 1024}, {   0, 1024}, {1024, 1024}
};

static int AVAILABLE_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    {  512,  384, 0, SIZE_RATIO_4_3},
    {  512,  288, 0, SIZE_RATIO_16_9},
    {  384,  384, 0, SIZE_RATIO_1_1},
    {    0,    0, 0, SIZE_RATIO_1_1},
};

static int AVAILABLE_HIDDEN_THUMBNAIL_LIST[][2] =
{
    {  512,  248}, /* SIZE_RATIO_18P5_9 */
};

static int AVAILABLE_THUMBNAIL_CALLBACK_SIZE_LIST[][2] =
{
    {  512,  384},
    {  512,  288},
    {  512,  248},
    {  384,  384},
};

static int AVAILABLE_THUMBNAIL_CALLBACK_FORMATS_LIST[] =
{
    HAL_PIXEL_FORMAT_YCbCr_420_888,
};

int getSensorId(int camId)
{
    int sensorId = -1;

#ifdef SENSOR_NAME_GET_FROM_FILE
    int *curSensorId = NULL;

    switch(camId) {
        case CAMERA_ID_BACK:
            curSensorId = &g_rearSensorId;
            break;
        case CAMERA_ID_BACK_2:
            curSensorId = &g_rear2SensorId;
            break;
        case CAMERA_ID_BACK_3:
            curSensorId = &g_rear3SensorId;
            break;
        case CAMERA_ID_BACK_4:
            curSensorId = &g_rear4SensorId;
            break;
        case CAMERA_ID_FRONT:
            curSensorId = &g_frontSensorId;
            break;
        case CAMERA_ID_FRONT_2:
            curSensorId = &g_front2SensorId;
            break;
        case CAMERA_ID_FRONT_3:
            curSensorId = &g_front3SensorId;
            break;
        case CAMERA_ID_FRONT_4:
            curSensorId = &g_front4SensorId;
            break;
        case CAMERA_ID_SECURE:
            curSensorId = &g_secureSensorId;
            break;
        case CAMERA_ID_BACK_TOF:
            curSensorId = &g_rearTofSensorId;
            break;
        case CAMERA_ID_FRONT_TOF:
            curSensorId = &g_frontTofSensorId;
            break;
        default:
            ALOGE("invalid cameraId(%d)", camId);
            break;
    }

    if (curSensorId != NULL) {
        if (*curSensorId < 0) {
            *curSensorId = getSensorIdFromFile(camId);
            if (*curSensorId < 0) {
                ALOGE("ERR(%s): invalid sensor ID %d", __FUNCTION__, sensorId);
            }
        }

        sensorId = *curSensorId;
    }
#else
    switch(camId) {
        case CAMERA_ID_BACK:
            sensorId = MAIN_CAMERA_SENSOR_NAME;
            break;
#ifdef BACK_1_CAMERA_SENSOR_NAME
        case CAMERA_ID_BACK_2:
            sensorId = BACK_1_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef BACK_2_CAMERA_SENSOR_NAME
        case CAMERA_ID_BACK_3:
            sensorId = BACK_2_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef BACK_3_CAMERA_SENSOR_NAME
        case CAMERA_ID_BACK_4:
            sensorId = BACK_3_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef BACK_4_CAMERA_SENSOR_NAME
        case CAMERA_ID_BACK_TOF:
            sensorId = BACK_4_CAMERA_SENSOR_NAME;
            break;
#endif

        case CAMERA_ID_FRONT:
            sensorId = FRONT_CAMERA_SENSOR_NAME;
            break;
#ifdef FRONT_1_CAMERA_SENSOR_NAME
        case CAMERA_ID_FRONT_2:
            sensorId = FRONT_1_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef FRONT_2_CAMERA_SENSOR_NAME
        case CAMERA_ID_FRONT_3:
            sensorId = FRONT_2_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef FRONT_3_CAMERA_SENSOR_NAME
        case CAMERA_ID_FRONT_4:
            sensorId = FRONT_3_CAMERA_SENSOR_NAME;
            break;
#endif
#ifdef FRONT_3_CAMERA_SENSOR_NAME
        case CAMERA_ID_FRONT_TOF:
            sensorId = FRONT_3_CAMERA_SENSOR_NAME;
            break;
#endif
        case CAMERA_ID_SECURE:
            sensorId = SECURE_CAMERA_SENSOR_NAME;
            break;
        default:
            ALOGE("ERR(%s):Unknown camera ID(%d)", __FUNCTION__, camId);
            break;
    }
#endif

    if (sensorId == SENSOR_NAME_NOTHING) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):camId(%d):sensorId == SENSOR_NAME_NOTHING, assert!!!!",
            __FUNCTION__, __LINE__, camId);
    }

    return sensorId;
}

ExynosCameraSensorInfoBase::ExynosCameraSensorInfoBase()
{
    /* implement AP chip variation */
    memset(name, 0x00, sizeof(name));
    sensorId = SENSOR_NAME_NOTHING;

    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxSensorW = 4128;
    maxSensorH = 3096;

    maxCroppedPreviewW = 0;
    maxCroppedPreviewH = 0;
    maxCroppedPictureW = 0;
    maxCroppedPictureH = 0;
    maxCroppedSensorW = 0;
    maxCroppedSensorH = 0;

    memset(&useSensorCrop, 0, sizeof(useSensorCrop));

    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorMarginBase[LEFT_BASE] = 0;
    sensorMarginBase[TOP_BASE] = 0;
    sensorMarginBase[WIDTH_BASE] = 0;
    sensorMarginBase[HEIGHT_BASE] = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    minFps = 7;
    maxFps = 30;
    defaultFpsMin[DEFAULT_FPS_STILL] = 15;
    defaultFpsMAX[DEFAULT_FPS_STILL] = 30;
    defaultFpsMin[DEFAULT_FPS_VIDEO] = 30;
    defaultFpsMAX[DEFAULT_FPS_VIDEO] = 30;
    defaultFpsMin[DEFAULT_FPS_EFFECT_STILL] = 10;
    defaultFpsMAX[DEFAULT_FPS_EFFECT_STILL] = 24;
    defaultFpsMin[DEFAULT_FPS_EFFECT_VIDEO] = 24;
    defaultFpsMAX[DEFAULT_FPS_EFFECT_VIDEO] = 24;

    stillShotSensorFpsSupport = false;
    stillShotSensorFps = 30;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;

    bnsSupport = false;
    /* flite->3aa otf support */
    flite3aaOtfSupport = true;
    sensorGyroSupport = false;

    gain = 0;
    exposureTime = 0L;
    ledCurrent = 0;
    ledPulseDelay = 0L;
    ledPulseWidth = 0L;
    ledMaxTime = 0;

    gainRange[MIN] = 0;
    gainRange[MAX] = 0;
    ledCurrentRange[MIN] = 0;
    ledCurrentRange[MAX] = 0;
    ledPulseDelayRange[MIN] = 0;
    ledPulseDelayRange[MAX] = 0;
    ledPulseWidthRange[MIN] = 0;
    ledPulseWidthRange[MAX] = 0;
    ledMaxTimeRange[MIN] = 0;
    ledMaxTimeRange[MAX] = 0;

    yuvListMax = 0;
    yuvFullListMax = 0;
    rawListMax = 0;
    jpegListMax = 0;
    jpegFullListMax = 0;
    tetraJpegListMax = 0;
    depthListMax = 0;
    thumbnailListMax =
        sizeof(AVAILABLE_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax = 0;
    fpsRangesListMax = 0;
    highSpeedVideoFPSListMax = 0;

    yuvList = NULL;
    yuvFullList = NULL;
    rawList = NULL;
    jpegList = NULL;
    jpegFullList = NULL;
    tetraJpegList = NULL;
    depthList = NULL;
    thumbnailList = AVAILABLE_THUMBNAIL_LIST;
    highSpeedVideoList = NULL;
    fpsRangesList = NULL;
    highSpeedVideoFPSList = NULL;

    previewSizeLutMax           = 0;
    previewFullSizeLutMax       = 0;
    pipPreviewSizeLutMax        = 0;
    pictureSizeLutMax           = 0;
    pictureFullSizeLutMax       = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    videoSizeLutHighSpeed240Max = 0;
    videoSizeLutHighSpeed480Max = 0;
    vtcallSizeLutMax            = 0;
    fastAeStableLutMax          = 0;
    depthMapSizeLutMax          = 0;
    yuvReprocessingInputListMax = 0;
    rawOutputListMax            = 0;

    previewSizeLut              = NULL;
    previewFullSizeLut          = NULL;
    pipPreviewSizeLut           = NULL;
    pictureSizeLut              = NULL;
    pictureFullSizeLut          = NULL;
    videoSizeLut                = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    videoSizeLutHighSpeed240    = NULL;
    videoSizeLutHighSpeed480    = NULL;
    vtcallSizeLut               = NULL;
    fastAeStableLut             = NULL;
    depthMapSizeLut             = NULL;
#ifdef SUPPORT_PD_IMAGE
    pdImageSizeLut              = NULL;
    pdImageSizeLutMax           = 0;
#endif
#ifdef SUPPORT_REMOSAIC_CAPTURE
    previewHighResolutionSizeLutMax = 0;
    previewHighResolutionSizeLut = NULL;
    captureHighResolutionSizeLutMax = 0;
    captureHighResolutionSizeLut = NULL;
#endif //SUPPORT_REMOSAIC_CAPTURE
    yuvReprocessingInputList = NULL;
    rawOutputList            = NULL;

    sizeTableSupport      = false;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */
    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = NULL;
    colorAberrationModesLength = 0;

    /* Android Control Static Metadata */
    antiBandingModes = NULL;
    aeModes = NULL;
    exposureCompensationRange[MIN] = -2;
    exposureCompensationRange[MAX] = 2;
    exposureCompensationStep = 1.0f;
    afModes = NULL;
    effectModes = NULL;
    sceneModes = NULL;
    videoStabilizationModes = NULL;
    awbModes = NULL;
    controlModes = NULL;
    controlModesLength = 0;
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = NULL;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE;
    antiBandingModesLength = 0;
    aeModesLength = 0;
    afModesLength = 0;
    effectModesLength = 0;
    sceneModesLength = 0;
    videoStabilizationModesLength = 0;
    awbModesLength = 0;
    sceneModeOverridesLength = 0;
    postRawSensitivityBoost[MIN] = 100;
    postRawSensitivityBoost[MAX] = 100;

    /* Android Edge Static Metadata */
    edgeModes = NULL;
    edgeModesLength = 0;

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = NULL;
    hotPixelModesLength = 0;

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.8f;
    focalLengthIn35mmLength = 31;
    focalLengthIn35mmLengthOnCroppedSize = -1.0f;
    opticalStabilization = NULL;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 0;
    shadingMapSize[HEIGHT] = 0;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 0.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilizationLength = 0;
    /* Vendor AF FOV Metadata */
    afFovList = NULL;
    afFovListMax = 0;

    poseReference = ANDROID_LENS_POSE_REFERENCE_PRIMARY_CAMERA;

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = NULL;
    noiseReductionModesLength = 0;

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;
    requestKeys = NULL;
    resultKeys = NULL;
    characteristicsKeys = NULL;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = 0;
    resultKeysLength = 0;
    characteristicsKeysLength = 0;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android Scaler Static Metadata */
    zoomSupport = false;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    minZoomRatioVendor = 1000;
    stallDurations = NULL;
    stallDurationsLength = 0;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevel = 64;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;

    LedCalibrationMasterCool[R]  = 383;
    LedCalibrationMasterCool[GR] = 792;
    LedCalibrationMasterCool[GB] = 793;
    LedCalibrationMasterCool[B]  = 413;

    LedCalibrationMasterWarm[R]  = 746;
    LedCalibrationMasterWarm[GR] = 739;
    LedCalibrationMasterWarm[GB] = 741;
    LedCalibrationMasterWarm[B]  = 175;

    LedCalibrationMasterCoolWarm[R]  = 546;
    LedCalibrationMasterCoolWarm[GR] = 775;
    LedCalibrationMasterCoolWarm[GB] = 777;
    LedCalibrationMasterCoolWarm[B]  = 314;

    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = NULL;
    testPatternModesLength = 0;
    colorTransformMatrix1 = UNIT_MATRIX;
    colorTransformMatrix2 = UNIT_MATRIX;
    forwardMatrix1 = UNIT_MATRIX;
    forwardMatrix2 = UNIT_MATRIX;
    calibration1 = UNIT_MATRIX;
    calibration2 = UNIT_MATRIX;
    masterRGain = 0.0f;
    masterBGain = 0.0f;

    /* Android Statistics Static Metadata */
    faceDetectModes = NULL;
    histogramBucketCount = 64;
    maxNumDetectedFaces = 1;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = NULL;
    faceDetectModesLength = 0;
    hotPixelMapModesLength = 0;
    lensShadingMapModes = NULL;
    lensShadingMapModesLength = 0;
    shadingAvailableModes = NULL;
    shadingAvailableModesLength = 0;

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = NULL;
    toneMapModesLength = 0;

    /* Android LED Static Metadata */
    leds = NULL;
    ledsLength = 0;

    /* Android Reprocess Static Metadata */
    maxCaptureStall = 4;

    /* Android Info Static Metadata */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
    supportedCapabilities = CAPABILITIES_BACKWARD_COMPATIBLE;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableThumbnailCallbackSizeListMax =
        sizeof(AVAILABLE_THUMBNAIL_CALLBACK_SIZE_LIST) / (sizeof(int) * 2);
    availableThumbnailCallbackSizeList =
        AVAILABLE_THUMBNAIL_CALLBACK_SIZE_LIST;
    availableThumbnailCallbackFormatListMax =
        sizeof(AVAILABLE_THUMBNAIL_CALLBACK_FORMATS_LIST) / sizeof(int);
    availableThumbnailCallbackFormatList =
        AVAILABLE_THUMBNAIL_CALLBACK_FORMATS_LIST;

    availableIrisSizeListMax = 0;
    availableIrisSizeList = NULL;
    availableIrisFormatListMax = 0;
    availableIrisFormatList = NULL;

#ifdef SUPPORT_DEPTH_MAP
    availableDepthSizeListMax = 0;
    availableDepthSizeList = NULL;
    availableDepthFormatListMax = 0;
    availableDepthFormatList = NULL;
#endif

    availableVideoListMax = 0;
    availableVideoList = NULL;

    availableVideoBeautyListMax = 0;
    availableVideoBeautyList = NULL;

    previewCropSizeLutMax = 0;
    previewCropSizeLut = NULL;

    pictureCropSizeLutMax = 0;
    pictureCropSizeLut = NULL;

    previewCropFullSizeLutMax = 0;
    previewCropFullSizeLut = NULL;

    pictureCropFullSizeLutMax = 0;
    pictureCropFullSizeLut = NULL;

    availableVendorDepthSizeListMax = 0;
    availableVendorDepthSizeList = NULL;

    availableVendorDepthFormatListMax = 0;
    availableVendorDepthFormatList = NULL;

    maxDepthSamples = 0;

    availableHighSpeedVideoListMax = 0;
    availableHighSpeedVideoList = NULL;

    hiddenPreviewListMax = 0;
    hiddenPreviewList = NULL;

    hiddenPictureListMax = 0;
    hiddenPictureList = NULL;

    hiddenThumbnailListMax =
        sizeof(AVAILABLE_HIDDEN_THUMBNAIL_LIST) / (sizeof(int) * 2);
    hiddenThumbnailList = AVAILABLE_HIDDEN_THUMBNAIL_LIST;

    hiddenfpsRangeListMax = 0;
    hiddenfpsRangeList = NULL;

    /* Vendor Feature */
    /* Vendor Control Metadata */
    vendorAwbModes = NULL;
    vendorAwbModesLength = 0;
    vendorWbColorTemp = 0;
    vendorWbColorTempRange[MIN] = 0;
    vendorWbColorTempRange[MAX] = 0;
    vendorWbLevelRange[MIN] = 0;
    vendorWbLevelRange[MAX] = 0;

    vendorExposureTimeRange[MIN] = 0;
    vendorExposureTimeRange[MAX] = 0;

    /* Vendor Scaler Metadata */
    vendorFlipModes = NULL;
    vendorFlipModesLength = 0;

    /* Vendor Feature Available & Support */
    sceneHDRSupport = false;
    screenFlashAvailable = false;
    objectTrackingAvailable = false;
    fixedFaceFocusAvailable = false;
    factoryDramTestCount = 0;
    controlZslSupport = false;
    superNightShotSupport = false;
    videoBokehSupport = false;
    multiStreamCaptureSupport = false;
    depthOnlySensor = false;
    depthIsExclusive = false;
#ifdef SUPPORT_SENSOR_REMOSAIC_SW
    swRemosaicSensor = false;
#endif

    availableBasicFeaturesListLength = 0;
    availableBasicFeaturesList = NULL;

    availableOptionalFeaturesListLength = 0;
    availableOptionalFeaturesList = NULL;

    effectFpsRangesListMax = 0;
    effectFpsRangesList = NULL;

    availableApertureValues = NULL;
    availableApertureValuesLength = 0;

    availableBurstShotFps = NULL;
    availableBurstShotFpsLength = 0;

    supported_sensor_ex_mode = 0;

    bayerPattern = BAYER_BGGR;
    supported_remosaic_mode = 0;
    supported_remosaic_modeMax = 0;
    isTetraSensor = false;
    /* END of Camera HAL 3.2 Static Metadatas */
}

ExynosCameraSensor2L7Base::ExynosCameraSensor2L7Base() : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4032;
    maxSensorH = 3024;
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_S5K2L7)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_S5K2L7)              / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_S5K2L7)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_S5K2L7)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_S5K2L7)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_S5K2L7)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K2L7)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_S5K2L7)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_S5K2L7)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_S5K2L7)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_S5K2L7;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_S5K2L7;
    pictureSizeLut              = PICTURE_SIZE_LUT_S5K2L7;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_S5K2L7;
    videoSizeLut                = VIDEO_SIZE_LUT_S5K2L7;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_S5K2L7;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K2L7;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_S5K2L7;
    vtcallSizeLut               = VTCALL_SIZE_LUT_S5K2L7;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_S5K2L7;

    yuvListMax                  = sizeof(S5K2L7_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K2L7_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K2L7_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K2L7_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K2L7_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    yuvList                     = S5K2L7_YUV_LIST;
    jpegList                    = S5K2L7_JPEG_LIST;
    highSpeedVideoList          = S5K2L7_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = S5K2L7_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = S5K2L7_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING | CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.70f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 4.2f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_2L7_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_2L7_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_2L7_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_2L7_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2P6Base::ExynosCameraSensor2P6Base() : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4608;
    maxSensorH = 3456;
    maxPreviewW = 4608;
    maxPreviewH = 3456;
    maxPictureW = 4608;
    maxPictureH = 3456;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport            = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P6)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2P6)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_2P6)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P6)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2P6)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P6)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P6)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P6)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2P6)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_2P6;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2P6;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_2P6;
    pictureSizeLut              = PICTURE_SIZE_LUT_2P6;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2P6;
    videoSizeLut                = VIDEO_SIZE_LUT_2P6;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2P6;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P6;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2P6;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2P6;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K2P6_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K2P6_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K2P6_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K2P6_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K2P6_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = S5K2P6_YUV_LIST;
    jpegList                    = S5K2P6_JPEG_LIST;
    highSpeedVideoList          = S5K2P6_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = S5K2P6_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = S5K2P6_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 3.6f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 1.0f / 3.93f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 64;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 22000;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_S5K2P6_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_S5K2P6_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_S5K2P6_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_S5K2P6_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.4f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 62.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 46.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 62.4f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 58.5f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 62.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 57.2f;
    //horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    //horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    verticalViewAngle = 46.8f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2P8Base::ExynosCameraSensor2P8Base() : ExynosCameraSensorInfoBase()
{
    maxSensorW = 5328;
    maxSensorH = 3000;
    maxPreviewW = 5328;
    maxPreviewH = 3000;
    maxPictureW = 5328;
    maxPictureH = 3000;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 16;
    sensorMarginH = 12;
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;
    sensorArrayRatio = SIZE_RATIO_16_9;

    bnsSupport = false;
    sizeTableSupport            = true;

    previewSizeLutMax           = sizeof(PREVIEW_FULL_SIZE_LUT_2P8)              / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2P8)             / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P8)                  / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2P8)             / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P8_BDS_DIS_FHD)        / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8)        / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8)  / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P8)  / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P8)                   / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2P8)           / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_FULL_SIZE_LUT_2P8;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2P8;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_2P8_BDS;
    pictureSizeLut              = PICTURE_SIZE_LUT_2P8;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2P8;
    videoSizeLut                = VIDEO_SIZE_LUT_2P8_BDS_DIS_FHD;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P8;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2P8;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2P8;

    yuvListMax                  = sizeof(S5K2P8_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K2P8_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K2P8_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K2P8_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K2P8_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    yuvList                 = S5K2P8_YUV_LIST;
    jpegList                = S5K2P8_JPEG_LIST;
    highSpeedVideoList      = S5K2P8_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = S5K2P8_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = S5K2P8_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BACKWARD_COMPATIBLE | CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.8f;
    focalLengthIn35mmLength = 31;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.05f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_2P8_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_2P8_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_2P8_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_2P8_3X3;
    calibration1 = UNIT_MATRIX_2P8_3X3;
    calibration2 = UNIT_MATRIX_2P8_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 43.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 33.6f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensorIMX333_2L2Base::ExynosCameraSensorIMX333_2L2Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4032;
    maxSensorH = 3024;
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX333_2L2)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_IMX333_2L2)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_IMX333_2L2)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX333_2L2)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_IMX333_2L2)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX333_2L2)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_IMX333_2L2)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX333_2L2)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX333_2L2)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX333_2L2)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_IMX333_2L2)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_IMX333_2L2;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_IMX333_2L2;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_IMX333_2L2;
    pictureSizeLut              = PICTURE_SIZE_LUT_IMX333_2L2;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_IMX333_2L2;
    videoSizeLut                = VIDEO_SIZE_LUT_IMX333_2L2;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_IMX333_2L2;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX333_2L2;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX333_2L2;
    vtcallSizeLut               = VTCALL_SIZE_LUT_IMX333_2L2;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_IMX333_2L2;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(IMX333_2L2_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(IMX333_2L2_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(IMX333_2L2_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(IMX333_2L2_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(IMX333_2L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = IMX333_2L2_YUV_LIST;
    jpegList                    = IMX333_2L2_JPEG_LIST;
    highSpeedVideoList          = IMX333_2L2_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = IMX333_2L2_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = IMX333_2L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.70f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 4.2f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.66f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2L2_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2L2_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2L2_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2L2_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2L3Base::ExynosCameraSensor2L3Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4032;
    maxSensorH = 3024;
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2L3)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2L3)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_2L3)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2L3)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2L3)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2L3)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_2L3)                      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2L3)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2L3)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2L3)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2L3)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_2L3;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2L3;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_2L3;
    pictureSizeLut              = PICTURE_SIZE_LUT_2L3;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2L3;
    videoSizeLut                = VIDEO_SIZE_LUT_2L3;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2L3;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2L3;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2L3;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2L3;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2L3;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAK2L3_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAK2L3_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAK2L3_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAK2L3_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAK2L3_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = SAK2L3_YUV_LIST;
    jpegList                    = SAK2L3_JPEG_LIST;
    highSpeedVideoList          = SAK2L3_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAK2L3_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAK2L3_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.5f;
    fNumber = 1.5f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2L3_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2L3_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2L3_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2L3_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2L4Base::ExynosCameraSensor2L4Base(__unused int serviceCameraId, __unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4032;
    maxSensorH = 3024;
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2L4)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2L4)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_2L4)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2L4)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2L4)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2L4)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_2L4)                      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2L4)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2L4)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2L4)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2L4)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_2L4;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2L4;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_2L4;
    pictureSizeLut              = PICTURE_SIZE_LUT_2L4;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2L4;
    videoSizeLut                = VIDEO_SIZE_LUT_2L4;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2L4;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2L4;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2L4;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2L4;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2L4;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAK2L4_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAK2L4_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAK2L4_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAK2L4_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAK2L4_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = SAK2L4_YUV_LIST;
    jpegList                    = SAK2L4_JPEG_LIST;
    highSpeedVideoList          = SAK2L4_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAK2L4_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAK2L4_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.5f;
    fNumber = 1.5f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2L4_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2L4_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2L4_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2L4_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor3P9Base::ExynosCameraSensor3P9Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4608;
    maxSensorH = 3456;
    maxPreviewW = 4608;
    maxPreviewH = 3456;
    maxPictureW = 4608;
    maxPictureH = 3456;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3P9)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_3P9)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_3P9)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3P9)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_3P9)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3P9)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_3P9)                      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3P9)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_3P9)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_3P9)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_3P9)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_3P9;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_3P9;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_3P9;
    pictureSizeLut              = PICTURE_SIZE_LUT_3P9;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_3P9;
    videoSizeLut                = VIDEO_SIZE_LUT_3P9;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_3P9;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3P9;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_3P9;
    vtcallSizeLut               = VTCALL_SIZE_LUT_3P9;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_3P9;

#ifdef SUPPORT_PD_IMAGE
    pdImageSizeLut              = PD_IMAGE_SIZE_LUT_2L4;
    pdImageSizeLutMax           = sizeof(PD_IMAGE_SIZE_LUT_2L4) / (sizeof(int) * PD_IMAGE_LUT_SIZE);;
#endif

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K3P9_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K3P9_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K3P9_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K3P9_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K3P9_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = S5K3P9_YUV_LIST;
    jpegList                    = S5K3P9_JPEG_LIST;
    highSpeedVideoList          = S5K3P9_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = S5K3P9_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = S5K3P9_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.5f;
    fNumber = 1.5f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_3P9_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_3P9_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_3P9_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_3P9_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor6B2Base::ExynosCameraSensor6B2Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 1936;
    maxSensorH = 1090;
    maxPreviewW = 1936;
    maxPreviewH = 1090;
    maxPictureW = 1936;
    maxPictureH = 1090;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_16_9;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_6B2)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_6B2)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_6B2)              / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_6B2)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_6B2)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_6B2)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_6B2)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = 0;

    previewSizeLut              = PREVIEW_SIZE_LUT_6B2;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_6B2;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_6B2;
    pictureSizeLut              = PICTURE_SIZE_LUT_6B2;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_6B2;
    videoSizeLut                = VIDEO_SIZE_LUT_6B2;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    vtcallSizeLut               = VTCALL_SIZE_LUT_6B2;
    fastAeStableLut             = NULL;

    /* Set the max of size/fps lists */
    yuvListMax              = sizeof(S5K6B2_YUV_LIST)               / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax             = sizeof(S5K6B2_JPEG_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax        = sizeof(S5K6B2_THUMBNAIL_LIST)         / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax        = sizeof(S5K6B2_FPS_RANGE_LIST)         / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(S5K6B2_YUV_REPROCESSING_INPUT_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(S5K6B2_RAW_OUTPUT_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported size/fps lists */
    yuvList                 = S5K6B2_YUV_LIST;
    jpegList                = S5K6B2_JPEG_LIST;
    thumbnailList           = S5K6B2_THUMBNAIL_LIST;
    fpsRangesList           = S5K6B2_FPS_RANGE_LIST;
    yuvReprocessingInputList = S5K6B2_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList            = S5K6B2_RAW_OUTPUT_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BACKWARD_COMPATIBLE | CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 1.86f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 250000000L;
    sensorPhysicalSize[WIDTH] = 3.495f;
    sensorPhysicalSize[HEIGHT] = 2.626f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.7f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 64.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 54.2f;
    verticalViewAngle = 39.4f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensorIMX320_3H1Base::ExynosCameraSensorIMX320_3H1Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 3264;
    maxSensorH = 2448;
    maxPreviewW = 3264;
    maxPreviewH = 2448;
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX320_3H1)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_IMX320_3H1)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_IMX320_3H1)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX320_3H1)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_IMX320_3H1)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX320_3H1)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX320_3H1)    / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX320_3H1)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX320_3H1)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX320_3H1)   / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_IMX320_3H1;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_IMX320_3H1;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_IMX320_3H1;
    pictureSizeLut              = PICTURE_SIZE_LUT_IMX320_3H1;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_IMX320_3H1;
    videoSizeLut                = VIDEO_SIZE_LUT_IMX320_3H1;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX320_3H1;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX320_3H1;
    vtcallSizeLut               = VTCALL_SIZE_LUT_IMX320_3H1;
    fastAeStableLut             = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX320_3H1;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(IMX320_3H1_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(IMX320_3H1_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(IMX320_3H1_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(IMX320_3H1_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(IMX320_3H1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported size/fps lists */
    yuvList                 = IMX320_3H1_YUV_LIST;
    jpegList                = IMX320_3H1_JPEG_LIST;
    highSpeedVideoList      = IMX320_3H1_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = IMX320_3H1_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = IMX320_3H1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    /* Limited-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.7f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 2.92f;
    focalLengthIn35mmLength = 25;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 3.982f;
    sensorPhysicalSize[HEIGHT] = 2.987f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 67.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 63.0f;
    verticalViewAngle = 53.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor3J1Base::ExynosCameraSensor3J1Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 3968;
    maxSensorH = 2736;
    maxPreviewW = 3968;
    maxPreviewH = 2736;
    maxPictureW = 3968;
    maxPictureH = 2736;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3J1)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_3J1)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_3J1)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3J1)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_3J1)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3J1)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3J1)    / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3J1)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_3J1)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3J1)   / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_3J1;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_3J1;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_3J1;
    pictureSizeLut              = PICTURE_SIZE_LUT_3J1;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_3J1;
    videoSizeLut                = VIDEO_SIZE_LUT_3J1;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3J1;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3J1;
    vtcallSizeLut               = VTCALL_SIZE_LUT_3J1;
    fastAeStableLut             = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3J1;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K3J1_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K3J1_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K3J1_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K3J1_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K3J1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported size/fps lists */
    yuvList                 = S5K3J1_YUV_LIST;
    jpegList                = S5K3J1_JPEG_LIST;
    highSpeedVideoList      = S5K3J1_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = S5K3J1_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = S5K3J1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    /* Limited-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.7f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 2.92f;
    focalLengthIn35mmLength = 25;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 3.982f;
    sensorPhysicalSize[HEIGHT] = 2.987f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 67.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 63.0f;
    verticalViewAngle = 53.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensorIMX576Base::ExynosCameraSensorIMX576Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 5664;
    maxSensorH = 4248;
    maxPreviewW = 5664;
    maxPreviewH = 4248;
    maxPictureW = 5664;
    maxPictureH = 4248;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX576)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_IMX576)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_IMX576)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX576)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_IMX576)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX576_BNS)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX576)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX576)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX576)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_IMX576)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_IMX576;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_IMX576;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_IMX576;
    pictureSizeLut              = PICTURE_SIZE_LUT_IMX576;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_IMX576;
    videoSizeLut                = VIDEO_SIZE_LUT_IMX576_BNS;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX576;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX576;
    vtcallSizeLut               = VTCALL_SIZE_LUT_IMX576;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_IMX576;

#ifdef SUPPORT_REMOSAIC_CAPTURE
    previewHighResolutionSizeLutMax = sizeof(PREVIEW_SIZE_LUT_IMX576)                   / (sizeof(int) * SIZE_OF_LUT);
    captureHighResolutionSizeLutMax = sizeof(PICTURE_SIZE_LUT_IMX576)                   / (sizeof(int) * SIZE_OF_LUT);
    previewHighResolutionSizeLut    = PREVIEW_SIZE_LUT_IMX576;
    captureHighResolutionSizeLut    = PICTURE_SIZE_LUT_IMX576;
#endif

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(IMX576_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(IMX576_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(IMX576_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(IMX576_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(IMX576_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported size/fps lists */
    yuvList                 = IMX576_YUV_LIST;
    jpegList                = IMX576_JPEG_LIST;
    highSpeedVideoList      = IMX576_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = IMX576_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = IMX576_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    /* Limited-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.7f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 2.92f;
    focalLengthIn35mmLength = 25;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 3.982f;
    sensorPhysicalSize[HEIGHT] = 2.987f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 67.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 63.0f;
    verticalViewAngle = 53.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor3M3Base::ExynosCameraSensor3M3Base(__unused int cameraId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4032;
    maxSensorH = 3024;
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    /*
    zoomSupport = true;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    */

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3M3)    / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_3M3)    / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3M3)        / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_3M3)             / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3M3)      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3M3) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3M3) / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_3M3)     / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_3M3) / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut        = PREVIEW_SIZE_LUT_3M3;
    previewFullSizeLut    = PREVIEW_FULL_SIZE_LUT_3M3;
    pictureSizeLut        = PICTURE_SIZE_LUT_3M3;
    pictureFullSizeLut    = PICTURE_FULL_SIZE_LUT_3M3;
    videoSizeLut          = VIDEO_SIZE_LUT_3M3;
    videoSizeLutHighSpeed60  = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3M3;
    videoSizeLutHighSpeed120 = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3M3;
    vtcallSizeLut         = VTCALL_SIZE_LUT_3M3;
    fastAeStableLut       = FAST_AE_STABLE_SIZE_LUT_3M3;


    /* Set the max of size/fps lists */
    yuvListMax              = sizeof(S5K3M3_YUV_LIST)               / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax             = sizeof(S5K3M3_JPEG_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax        = sizeof(S5K3M3_FPS_RANGE_LIST)         / (sizeof(int) * 2);

    /* Set supported size/fps lists */
    yuvList                 = S5K3M3_YUV_LIST;
    jpegList                = S5K3M3_JPEG_LIST;
    fpsRangesList           = S5K3M3_FPS_RANGE_LIST;

  /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.4f;
    fNumber = 2.4f;
    filterDensity = 0.0f;
    focalLength = 6.0f;
    focalLengthIn35mmLength = 52;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 25;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 22000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_3M3_3X3; /*Copied from 2L1 setting */
    colorTransformMatrix2 = COLOR_MATRIX2_3M3_3X3; /* Copied from 2L1 setting */
    forwardMatrix1 = FORWARD_MATRIX1_3M3_3X3; /* Copied from 2L1 setting */
    forwardMatrix2 = FORWARD_MATRIX2_3M3_3X3; /* Copied from 2L1 setting */

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensorS5K5F1Base::ExynosCameraSensorS5K5F1Base(__unused int cameraId) : ExynosCameraSensorInfoBase()
{
    maxPreviewW = 2400;
    maxPreviewH = 2400;
    maxSensorW = 2400;
    maxSensorH = 2400;
    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorMarginBase[LEFT_BASE] = 0;
    sensorMarginBase[TOP_BASE] = 0;
    sensorMarginBase[WIDTH_BASE] = 0;
    sensorMarginBase[HEIGHT_BASE] = 0;

    sensorArrayRatio = SIZE_RATIO_1_1;

    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_5F1) / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_5F1) / (sizeof(int) * SIZE_OF_LUT);
    previewSizeLut              = PREVIEW_SIZE_LUT_5F1;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_5F1;
    sizeTableSupport            = true;

    /* Set the max of size/fps lists */
    yuvListMax              = sizeof(S5K5F1_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax        = sizeof(S5K5F1_FPS_RANGE_LIST) / (sizeof(int) * 2);
    thumbnailListMax = 0;
    hiddenThumbnailListMax = 0;

    /* Set supported size/fps lists */
    yuvList                 = S5K5F1_YUV_LIST;
    fpsRangesList           = S5K5F1_FPS_RANGE_LIST;
    thumbnailList = NULL;
    hiddenThumbnailList = NULL;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
    requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
    resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;

    /* Android Edge Static Metadata */

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;

    /* Android Hot Pixel Static Metadata */

    /* Android Lens Static Metadata */
    aperture = 1.9f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 2.2f;
    focalLengthIn35mmLength = 22;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;

    /* Android Noise Reduction Static Metadata */

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 0; //RAW
    maxNumOutputStreams[PROCESSED] = 0; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 0; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = false;
    maxZoomRatio = 1000;
    maxZoomRatioVendor = 1000;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 100000L; //   0.1ms
    exposureTimeRange[MAX] = 33200000L; //  33.2ms
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.495f;
    sensorPhysicalSize[HEIGHT] = 2.626f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = SECURE_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;

    /* Android Statistics Static Metadata */
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;

    horizontalViewAngle[SIZE_RATIO_16_9] = 78.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 78.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 62.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 78.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 75.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 78.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 73.0f;
    verticalViewAngle = 61.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_UNKNOWN;

    availableThumbnailCallbackSizeListMax = 0;
    availableThumbnailCallbackSizeList = NULL;
    availableThumbnailCallbackFormatListMax = 0;
    availableThumbnailCallbackFormatList = NULL;
    /* END of Camera HAL 3.2 Static Metadatas */
};

/* based on IMX333/2L2 */
ExynosCameraSensorS5KRPBBase::ExynosCameraSensorS5KRPBBase(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4656;
    maxSensorH = 3520;
    maxPreviewW = 4656;
    maxPreviewH = 3492;
    maxPictureW = 4656;
    maxPictureH = 3492;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_RPB)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_RPB)              / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_RPB)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_RPB)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_RPB)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_RPB)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_RPB)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_RPB)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed480Max = sizeof(VIDEO_SIZE_LUT_480FPS_HIGH_SPEED_RPB)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_RPB)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_RPB)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_RPB;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_RPB;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_RPB;
    pictureSizeLut              = PICTURE_SIZE_LUT_RPB;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_RPB;
    videoSizeLut                = VIDEO_SIZE_LUT_RPB;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_RPB;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_RPB;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_RPB;
    videoSizeLutHighSpeed480    = VIDEO_SIZE_LUT_480FPS_HIGH_SPEED_RPB;
    vtcallSizeLut               = VTCALL_SIZE_LUT_RPB;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_RPB;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(RPB_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(RPB_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(RPB_THUMBNAIL_LIST)                     / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(RPB_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(RPB_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(RPB_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = RPB_YUV_LIST;
    jpegList                    = RPB_JPEG_LIST;
    thumbnailList               = RPB_THUMBNAIL_LIST;
    highSpeedVideoList          = RPB_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = RPB_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = RPB_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BACKWARD_COMPATIBLE | CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                             CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.70f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 4.2f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.66f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_RPB_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_RPB_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_RPB_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_RPB_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2P7SQBase::ExynosCameraSensor2P7SQBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4608;
    maxSensorH = 3456;
    maxPreviewW = 4608;
    maxPreviewH = 3456;
    maxPictureW = 4608;
    maxPictureH = 3456;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P7SQ)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2P7SQ)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_2P7SQ)               / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P7SQ)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2P7SQ)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P7SQ)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_2P7SQ)                    / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P7SQ)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P7SQ)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P7SQ)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2P7SQ)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_2P7SQ;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2P7SQ;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_2P7SQ;
    pictureSizeLut              = PICTURE_SIZE_LUT_2P7SQ;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2P7SQ;
    videoSizeLut                = VIDEO_SIZE_LUT_2P7SQ;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2P7SQ;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P7SQ;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P7SQ;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2P7SQ;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2P7SQ;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAK2P7SQ_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAK2P7SQ_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAK2P7SQ_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAK2P7SQ_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAK2P7SQ_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(SAK2P7SQ_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(SAK2P7SQ_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = SAK2P7SQ_YUV_LIST;
    jpegList                    = SAK2P7SQ_JPEG_LIST;
    highSpeedVideoList          = SAK2P7SQ_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAK2P7SQ_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAK2P7SQ_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = SAK2P7SQ_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = SAK2P7SQ_RAW_OUTPUT_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);
    /* Vendor AF FOV Metadata */
    afFovList = SAK2P7SQ_AF_FOV_SIZE_LIST;
    afFovListMax    = sizeof(SAK2P7SQ_AF_FOV_SIZE_LIST) / (sizeof(int) * 6);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2P7SQ_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2P7SQ_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2P7SQ_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2P7SQ_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(SAK2P7SQ_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = SAK2P7SQ_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2T7SXBase::ExynosCameraSensor2T7SXBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 5184;
    maxSensorH = 3880;
    maxPreviewW = 5184;
    maxPreviewH = 3880;
    maxPictureW = 5184;
    maxPictureH = 3880;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2T7SX)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2T7SX)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(DUAL_PREVIEW_SIZE_LUT_2T7SX)               / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2T7SX)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2T7SX)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2T7SX)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_2T7SX)                    / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2T7SX)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2T7SX)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2T7SX)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2T7SX)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_2T7SX;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2T7SX;
    pipPreviewSizeLut           = DUAL_PREVIEW_SIZE_LUT_2T7SX;
    pictureSizeLut              = PICTURE_SIZE_LUT_2T7SX;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2T7SX;
    videoSizeLut                = VIDEO_SIZE_LUT_2T7SX;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2T7SX;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2T7SX;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2T7SX;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2T7SX;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2T7SX;

#ifdef SUPPORT_PD_IMAGE
    pdImageSizeLut              = PD_IMAGE_SIZE_LUT_3J1;
    pdImageSizeLutMax           = sizeof(PD_IMAGE_SIZE_LUT_3J1) / (sizeof(int) * PD_IMAGE_LUT_SIZE);;
#endif

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAK2T7SX_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAK2T7SX_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAK2T7SX_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAK2T7SX_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAK2T7SX_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(SAK2T7SX_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(SAK2T7SX_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);


    /* Set supported  size/fps lists */
    yuvList                     = SAK2T7SX_YUV_LIST;
    jpegList                    = SAK2T7SX_JPEG_LIST;
    highSpeedVideoList          = SAK2T7SX_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAK2T7SX_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAK2T7SX_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = SAK2T7SX_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = SAK2T7SX_RAW_OUTPUT_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.5f;
    fNumber = 1.5f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);
    /* Vendor AF FOV Metadata */
    afFovList = SAK2T7SX_AF_FOV_SIZE_LIST;
    afFovListMax = sizeof(SAK2T7SX_AF_FOV_SIZE_LIST) / (sizeof(int) * 6);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2T7SX_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2T7SX_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2T7SX_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2T7SX_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor4HABase::ExynosCameraSensor4HABase(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 3280;
    maxSensorH = 2458;
    maxPreviewW = 3280;
    maxPreviewH = 2458;
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_4HA)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_4HA)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_4HA)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_4HA)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_4HA)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4HA)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = 0;
    videoSizeLutHighSpeed240Max = 0;
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_4HA)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_4HA)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_4HA;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_4HA;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_4HA;
    pictureSizeLut              = PICTURE_SIZE_LUT_4HA;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_4HA;
    videoSizeLut                = VIDEO_SIZE_LUT_4HA;
    videoSizeLutHighSpeed120    = NULL;
    videoSizeLutHighSpeed240    = NULL;
    vtcallSizeLut               = VTCALL_SIZE_LUT_4HA;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_4HA;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K4HA_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K4HA_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K4HA_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K4HA_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K4HA_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported size/fps lists */
    yuvList                 = S5K4HA_YUV_LIST;
    jpegList                = S5K4HA_JPEG_LIST;
    highSpeedVideoList      = S5K4HA_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = S5K4HA_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = S5K4HA_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    /* Limited-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.7f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 2.92f;
    focalLengthIn35mmLength = 25;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 3.982f;
    sensorPhysicalSize[HEIGHT] = 2.987f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 43.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 33.6f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor5E9Base::ExynosCameraSensor5E9Base(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 2592;
    maxSensorH = 1944;
    maxPreviewW = 2592;
    maxPreviewH = 1944;
    maxPictureW = 2592;
    maxPictureH = 1944;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_5E9)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_5E9)              / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_5E9)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_FULL_SIZE_LUT_5E9)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_5E9)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_5E9)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = 0;
    videoSizeLutHighSpeed240Max = 0;
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_5E9)                    / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_5E9)            / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_5E9;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_5E9;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_5E9;
    pictureSizeLut              = PICTURE_FULL_SIZE_LUT_5E9;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_5E9;
    videoSizeLut                = VIDEO_SIZE_LUT_5E9;
    videoSizeLutHighSpeed120    = NULL;
    videoSizeLutHighSpeed240    = NULL;
    vtcallSizeLut               = VTCALL_SIZE_LUT_5E9;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_5E9;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K5E9_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K5E9_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K5E9_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K5E9_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K5E9_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(S5K5E9_YUV_REPROCESSING_INPUT_LIST)        / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(S5K5E9_RAW_OUTPUT_LIST)                    / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported size/fps lists */
    yuvList                 = S5K5E9_YUV_LIST;
    jpegList                = S5K5E9_JPEG_LIST;
    highSpeedVideoList      = S5K5E9_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = S5K5E9_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = S5K5E9_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList= S5K5E9_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList           = S5K5E9_RAW_OUTPUT_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* Limited-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                             CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
#ifdef MOT_9609_SENSORS_TROIKA
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 1.98f;
#else
    aperture = 2.25f;
    fNumber = 2.25f;
    filterDensity = 0.0f;
    focalLength = 2.166f;
#endif
    focalLengthIn35mmLength = 25;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 2.90304f;
    sensorPhysicalSize[HEIGHT] = 2.177728f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_5E9_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_5E9_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_5E9_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_5E9_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

#ifdef MOT_9609_SENSORS_TROIKA
    horizontalViewAngle[SIZE_RATIO_4_3]  = 70.1f;
    verticalViewAngle = 55.6f;
#else
    horizontalViewAngle[SIZE_RATIO_4_3]  = 67.7f;
    verticalViewAngle = 53.4f;
#endif

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(S5K5E9_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = S5K5E9_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* END of Camera HAL 3.2 Static Metadatas */
};

/* based on S5K3L2 */
ExynosCameraSensor3L2Base::ExynosCameraSensor3L2Base(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4144;
    maxSensorH = 3106;
    maxPreviewW = 4128;
    maxPreviewH = 3096;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3L2)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_3L2)              / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3L2)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_3L2)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3L2)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_3L2)                     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3L2)   / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_3L2)                    / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_3L2;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_3L2;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_3L2;
    pictureSizeLut              = PICTURE_SIZE_LUT_3L2;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_3L2;
    videoSizeLut                = VIDEO_SIZE_LUT_3L2;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_3L2;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3L2;
    vtcallSizeLut               = VTCALL_SIZE_LUT_3L2;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K3L2_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K3L2_JPEG_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K3L2_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K3L2_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K3L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = S5K3L2_YUV_LIST;
    jpegList                    = S5K3L2_JPEG_LIST;
    highSpeedVideoList          = S5K3L2_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = S5K3L2_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = S5K3L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING | CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 3.426f;
    minimumFocusDistance = 1.66f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 500000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    if (lensFacing == ANDROID_LENS_FACING_BACK) {
        orientation = BACK_ROTATION;
    } else {
        orientation = FRONT_ROTATION;
    }
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_3L2_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_3L2_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_3L2_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_3L2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

/* based on S5K4H5YC */
ExynosCameraSensor4H5YCBase::ExynosCameraSensor4H5YCBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 3280;
    maxSensorH = 2458;
    maxPreviewW = 3264;
    maxPreviewH = 2448;
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_4H5)                   / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_4H5)              / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_4H5_YC)                   / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_4H5)              / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4H5)                     / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_4H5;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_4H5;
    pipPreviewSizeLut           = PREVIEW_SIZE_LUT_4H5;
    pictureSizeLut              = PICTURE_SIZE_LUT_4H5_YC;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_4H5;
    videoSizeLut                = VIDEO_SIZE_LUT_4H5;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(S5K4H5_YC_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K4H5_YC_PICTURE_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K4H5_YC_FPS_RANGE_LIST)                     / (sizeof(int) * 2);

    /* Set supported  size/fps lists */
    yuvList                     = S5K4H5_YC_YUV_LIST;
    jpegList                    = S5K4H5_YC_PICTURE_LIST;
    fpsRangesList               = S5K4H5_YC_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING | CAPABILITIES_BURST_CAPTURE);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 3.426f;
    minimumFocusDistance = 1.66f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1250;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 500000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    if (lensFacing == ANDROID_LENS_FACING_BACK) {
        orientation = BACK_ROTATION;
    } else {
        orientation = FRONT_ROTATION;
    }
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_4H5_YC_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_4H5_YC_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_4H5_YC_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_4H5_YC_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor3P8SPBase::ExynosCameraSensor3P8SPBase(__unused int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4608;
    maxSensorH = 3456;
    maxPreviewW = 2304;
    maxPreviewH = 1728;
    maxPictureW = 4608;
    maxPictureH = 3456;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;

    //minFps = 1;
    //maxFps = 30;

    /* vendor specifics */
    bnsSupport = true;

    previewSizeLutMax            = sizeof(PREVIEW_SIZE_LUT_S5K3P8SP) / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax        = sizeof(YUV_SIZE_LUT_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax            = sizeof(PREVIEW_SIZE_LUT_S5K3P8SP) / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax        = sizeof(YUV_SIZE_LUT_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_S5K3P8SP) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max    = sizeof(VIDEO_SIZE_LUT_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax            = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K3P8SP_BNS) / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut                = PREVIEW_SIZE_LUT_S5K3P8SP;
    previewFullSizeLut            = YUV_SIZE_LUT_S5K3P8SP_BNS;
    pipPreviewSizeLut             = PREVIEW_SIZE_LUT_S5K3P8SP_BNS;
    videoSizeLut                = VIDEO_SIZE_LUT_S5K3P8SP_BNS;
    //videoSizeBnsLut              = VIDEO_SIZE_LUT_S5K3P8SP_BNS;
    pictureSizeLut                = PREVIEW_SIZE_LUT_S5K3P8SP;
    pictureFullSizeLut            = YUV_SIZE_LUT_S5K3P8SP_BNS;
    vtcallSizeLut                = VTCALL_SIZE_LUT_S5K3P8SP;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_S5K3P8SP_BNS;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K3P8SP_BNS;
    fastAeStableLut             = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K3P8SP_BNS;

#ifdef SUPPORT_REMOSAIC_CAPTURE
    previewHighResolutionSizeLut = PREVIEW_SIZE_LUT_S5K3P8SP;
    previewHighResolutionSizeLutMax = sizeof(PREVIEW_SIZE_LUT_S5K3P8SP) / (sizeof(int) * SIZE_OF_LUT);
    captureHighResolutionSizeLut = PICTURE_SIZE_LUT_S5K3P8SP;
    captureHighResolutionSizeLutMax = sizeof(PICTURE_SIZE_LUT_S5K3P8SP) / (sizeof(int) * SIZE_OF_LUT);
#endif //SUPPORT_REMOSAIC_CAPTURE

    sizeTableSupport      = true;

    thumbnailList = S5K3P8SP_THUMBNAIL_LIST;
    hiddenPreviewListMax = sizeof(S5K3P8SP_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenPictureListMax = sizeof(S5K3P8SP_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);


    yuvListMax                  = sizeof(S5K3P8SP_YUV_LIST)                           / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(S5K3P8SP_YUV_LIST)                          / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K3P8SP_HIGH_SPEED_VIDEO_LIST)              / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(S5K3P8SP_FPS_RANGE_LIST)                     / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K3P8SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)    / (sizeof(int) * 2);

    yuvList                 = S5K3P8SP_YUV_LIST;
    jpegList                = S5K3P8SP_YUV_LIST;
    highSpeedVideoList      = S5K3P8SP_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList           = S5K3P8SP_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = S5K3P8SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
            supportedCapabilities = (CAPABILITIES_BURST_CAPTURE);
            requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
            resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif

    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);

    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;

    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;

    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 3.6f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 1.0f / 3.6f;
    //minimumFocusDistance = 1.0f / 0.1f;
    minimumFocusDistance = 0.0f;

    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }

    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 64;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 22000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = FRONT_ROTATION;

    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_S5K3P8SP_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_S5K3P8SP_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_S5K3P8SP_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_S5K3P8SP_3X3;

    calibration1 = UNIT_MATRIX_S5K3P8SP_3X3;
    calibration2 = UNIT_MATRIX_S5K3P8SP_3X3;


    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 50.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 64.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 46.0f;
    verticalViewAngle = 39.0f;
    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);
#if 0
    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);
#endif

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
}


ExynosCameraSensorGM1SPBase::ExynosCameraSensorGM1SPBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4000;
    maxSensorH = 3000;
    maxPreviewW = 4000;
    maxPreviewH = 3000;
    maxPictureW = 4000;
    maxPictureH = 3000;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_GM1SP)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_GM1SP)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_GM1SP)                / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_FULL_SIZE_LUT_GM1SP)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_GM1SP)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_GM1SP)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_GM1SP)      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_GM1SP)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_GM1SP)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_GM1SP)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_GM1SP)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_GM1SP;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_GM1SP;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_GM1SP;
    pictureSizeLut              = PICTURE_FULL_SIZE_LUT_GM1SP;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_GM1SP;
    videoSizeLut                = VIDEO_SIZE_LUT_GM1SP;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_GM1SP;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_GM1SP;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_GM1SP;
    vtcallSizeLut               = VTCALL_SIZE_LUT_GM1SP;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_GM1SP;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAKGM1SP_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAKGM1SP_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAKGM1SP_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAKGM1SP_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAKGM1SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(SAKGM1SP_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(SAKGM1SP_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(SAKGM1SP_THUMBNAIL_LIST)                   / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = SAKGM1SP_YUV_LIST;
    jpegList                    = SAKGM1SP_JPEG_LIST;
    highSpeedVideoList          = SAKGM1SP_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAKGM1SP_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAKGM1SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = SAKGM1SP_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = SAKGM1SP_RAW_OUTPUT_LIST;
    thumbnailList               = SAKGM1SP_THUMBNAIL_LIST;

    /* Set Vendor lists */
    hiddenfpsRangeListMax        = sizeof(SAKGM1SP_HIDDEN_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    hiddenfpsRangeList           = SAKGM1SP_HIDDEN_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
#ifdef MOT_9609_SENSORS_KANE
    aperture = 1.7f;
    fNumber = 1.7f;
#else
    aperture = 1.75f;
    fNumber = 1.75f;
#endif
    filterDensity = 0.0f;
    focalLength = 4.745f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
#ifdef MOT_9609_SENSORS_KANE
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
#else
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
#endif
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 60000L;
#ifdef MOT_9609_SENSORS_KANE
    exposureTimeRange[MAX] = 250000000L;
#else
    exposureTimeRange[MAX] = 100000000L;
#endif
    maxFrameDuration = 142857142L;
#ifdef MOT_9609_SENSORS_KANE
    sensorPhysicalSize[WIDTH] = 6.400f;
    sensorPhysicalSize[HEIGHT] = 4.800f;
#else
    sensorPhysicalSize[WIDTH] = 5.645f;
    sensorPhysicalSize[HEIGHT] = 4.234f;
#endif
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;

    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_GM1SP_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_GM1SP_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_GM1SP_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_GM1SP_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 65.0f;
    verticalViewAngle = 41.0f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(SAKGM1SP_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = SAKGM1SP_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* Vendor Feature Metadata */
    vendorExposureTimeRange[MIN] = 60000L;
    vendorExposureTimeRange[MAX] = 32000000000L;

    /* END of Camera HAL 3.2 Static Metadatas */
};


ExynosCameraSensor12A10Base::ExynosCameraSensor12A10Base(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4096;
    maxSensorH = 3072;
    maxPreviewW = 4096;
    maxPreviewH = 3072;
    maxPictureW = 4096;
    maxPictureH = 3072;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_12A10)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_12A10)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_12A10)                / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_12A10)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_12A10)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_12A10)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_12A10)      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_12A10)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_12A10)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_12A10)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_12A10)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_12A10;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_12A10;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_12A10;
    pictureSizeLut              = PICTURE_SIZE_LUT_12A10;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_12A10;
    videoSizeLut                = VIDEO_SIZE_LUT_12A10;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_12A10;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_12A10;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_12A10;
    vtcallSizeLut               = VTCALL_SIZE_LUT_12A10;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_12A10;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(OV12A10_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(OV12A10_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(OV12A10_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(OV12A10_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(OV12A10_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(OV12A10_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(OV12A10_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(OV12A10_THUMBNAIL_LIST)                   / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = OV12A10_YUV_LIST;
    jpegList                    = OV12A10_JPEG_LIST;
    highSpeedVideoList          = OV12A10_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = OV12A10_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = OV12A10_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = OV12A10_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = OV12A10_RAW_OUTPUT_LIST;
    thumbnailList               = OV12A10_THUMBNAIL_LIST;

    /* Set Vendor lists */
    hiddenfpsRangeListMax        = sizeof(OV12A10_HIDDEN_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    hiddenfpsRangeList           = OV12A10_HIDDEN_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
#ifdef MOT_9609_SENSORS_TROIKA
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
#else
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
#endif
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.8f;
    fNumber = 1.8f;
    filterDensity = 0.0f;
    focalLength = 3.95f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.00f / 0.1f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.1071f;
    sensorPhysicalSize[HEIGHT] = 3.8353f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;

    LedCalibrationMasterCool[R]  = 290;
    LedCalibrationMasterCool[GR] = 606;
    LedCalibrationMasterCool[GB] = 605;
    LedCalibrationMasterCool[B]  = 335;

    LedCalibrationMasterWarm[R]  = 691;
    LedCalibrationMasterWarm[GR] = 629;
    LedCalibrationMasterWarm[GB] = 631;
    LedCalibrationMasterWarm[B]  = 178;

    LedCalibrationMasterCoolWarm[R]  = 571;
    LedCalibrationMasterCoolWarm[GR] = 792;
    LedCalibrationMasterCoolWarm[GB] = 793;
    LedCalibrationMasterCoolWarm[B]  = 355;

    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_OV12A10_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_OV12A10_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_OV12A10_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_OV12A10_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 64.1f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 64.1f;
    verticalViewAngle = 50.2f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(OV12A10_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = OV12A10_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* Vendor Feature Metadata */
    vendorExposureTimeRange[MIN] = 60000L;
    vendorExposureTimeRange[MAX] = 32000000000L;

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor12A10FFBase::ExynosCameraSensor12A10FFBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 4096;
    maxSensorH = 3072;
    maxPreviewW = 4096;
    maxPreviewH = 3072;
    maxPictureW = 4096;
    maxPictureH = 3072;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_12A10FF)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_12A10FF)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_12A10FF)                / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_12A10FF)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_12A10FF)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_12A10FF)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_12A10FF)      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_12A10FF)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_12A10FF)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_12A10FF)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_12A10FF)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_12A10FF;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_12A10FF;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_12A10FF;
    pictureSizeLut              = PICTURE_SIZE_LUT_12A10FF;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_12A10FF;
    videoSizeLut                = VIDEO_SIZE_LUT_12A10FF;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_12A10FF;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_12A10FF;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_12A10FF;
    vtcallSizeLut               = VTCALL_SIZE_LUT_12A10FF;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_12A10FF;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(OV12A10FF_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(OV12A10FF_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(OV12A10FF_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(OV12A10FF_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(OV12A10FF_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(OV12A10FF_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(OV12A10FF_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(OV12A10FF_THUMBNAIL_LIST)                   / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = OV12A10FF_YUV_LIST;
    jpegList                    = OV12A10FF_JPEG_LIST;
    highSpeedVideoList          = OV12A10FF_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = OV12A10FF_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = OV12A10FF_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = OV12A10FF_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = OV12A10FF_RAW_OUTPUT_LIST;
    thumbnailList               = OV12A10FF_THUMBNAIL_LIST;

    /* Set Vendor lists */
    hiddenfpsRangeListMax        = sizeof(OV12A10FF_HIDDEN_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    hiddenfpsRangeList           = OV12A10FF_HIDDEN_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.0f;
    fNumber = 2.0f;
    filterDensity = 0.0f;
    focalLength = 3.81f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.1071f;
    sensorPhysicalSize[HEIGHT] = 3.8353f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;

    maxAnalogSensitivity = 640;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_OV12A10FF_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_OV12A10FF_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_OV12A10FF_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_OV12A10FF_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 66.4f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 66.4f;
    verticalViewAngle = 52.1f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(OV12A10FF_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = OV12A10FF_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* Vendor Feature Metadata */
    vendorExposureTimeRange[MIN] = 60000L;
    vendorExposureTimeRange[MAX] = 32000000000L;

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor16885CBase::ExynosCameraSensor16885CBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 2336;
    maxSensorH = 1752;
    maxPreviewW = 2336;
    maxPreviewH = 1752;
    maxPictureW = 2336;
    maxPictureH = 1752;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = false;
    sizeTableSupport = true;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_16885C)                     / (sizeof(int) * SIZE_OF_LUT);
    previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_16885C)            / (sizeof(int) * SIZE_OF_LUT);
    pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_16885C)                / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_16885C)                     / (sizeof(int) * SIZE_OF_LUT);
    pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_16885C)            / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_16885C)                   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_16885C)      / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_16885C)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_16885C)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_16885C)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_16885C)          / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_16885C;
    previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_16885C;
    pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_16885C;
    pictureSizeLut              = PICTURE_SIZE_LUT_16885C;
    pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_16885C;
    videoSizeLut                = VIDEO_SIZE_LUT_16885C;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_16885C;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_16885C;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_16885C;
    vtcallSizeLut               = VTCALL_SIZE_LUT_16885C;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_16885C;

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(OV16885C_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(OV16885C_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(OV16885C_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(OV16885C_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(OV16885C_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(OV16885C_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(OV16885C_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = OV16885C_YUV_LIST;
    jpegList                    = OV16885C_JPEG_LIST;
    highSpeedVideoList          = OV16885C_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = OV16885C_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = OV16885C_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = OV16885C_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = OV16885C_RAW_OUTPUT_LIST;

    /* Set Vendor lists */
    hiddenfpsRangeListMax        = sizeof(OV16885C_HIDDEN_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    hiddenfpsRangeList           = OV16885C_HIDDEN_FPS_RANGE_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_BACK;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_ENABLE_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 2.24f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR;
    exposureTimeRange[MIN] = 60000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 4.74163f;
    sensorPhysicalSize[HEIGHT] =3.56429f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 640;
    orientation = 0; //default rotation is 0
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    colorTransformMatrix1 = COLOR_MATRIX1_OV16885C_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_OV16885C_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_OV16885C_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_OV16885C_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 95.6f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 95.6f;
    verticalViewAngle = 76.6f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(OV16885C_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = OV16885C_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCameraSensor2X5SPBase::ExynosCameraSensor2X5SPBase(int sensorId) : ExynosCameraSensorInfoBase()
{
    maxSensorW = 2880;
    maxSensorH = 2160;
    maxPreviewW = 2880;
    maxPreviewH = 2160;
    maxPictureW = 5760;
    maxPictureH = 4320;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorArrayRatio = SIZE_RATIO_4_3;

    bnsSupport = true;
    sizeTableSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2X5SP_BNS)                / (sizeof(int) * SIZE_OF_LUT);
        previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2X5SP)            / (sizeof(int) * SIZE_OF_LUT);
        pipPreviewSizeLutMax        = sizeof(PREVIEW_SIZE_LUT_2X5SP_BNS)             / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2X5SP_BNS)                / (sizeof(int) * SIZE_OF_LUT);
        pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2X5SP)            / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2X5SP_BNS)                  / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_2X5SP_BNS;
        previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2X5SP;
        pipPreviewSizeLut           = PREVIEW_SIZE_LUT_2X5SP_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_2X5SP_BNS;
        pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2X5SP;
        videoSizeLut                = VIDEO_SIZE_LUT_2X5SP_BNS;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2X5SP)                     / (sizeof(int) * SIZE_OF_LUT);
        previewFullSizeLutMax       = sizeof(PREVIEW_FULL_SIZE_LUT_2X5SP)            / (sizeof(int) * SIZE_OF_LUT);
        pipPreviewSizeLutMax        = sizeof(PREVIEW_FULL_SIZE_LUT_2X5SP)                / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2X5SP)                     / (sizeof(int) * SIZE_OF_LUT);
        pictureFullSizeLutMax       = sizeof(PICTURE_FULL_SIZE_LUT_2X5SP)            / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2X5SP)                   / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_2X5SP;
        previewFullSizeLut          = PREVIEW_FULL_SIZE_LUT_2X5SP;
        pipPreviewSizeLut           = PREVIEW_FULL_SIZE_LUT_2X5SP;
        pictureSizeLut              = PICTURE_SIZE_LUT_2X5SP;
        pictureFullSizeLut          = PICTURE_FULL_SIZE_LUT_2X5SP;
        videoSizeLut                = VIDEO_SIZE_LUT_2X5SP;
    }

    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_2X5SP)                    / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2X5SP)     / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2X5SP)     / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2X5SP)                  / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_2X5SP)          / (sizeof(int) * SIZE_OF_LUT);

    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_2X5SP;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2X5SP;
    videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2X5SP;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2X5SP;
    fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_2X5SP;

#ifdef SUPPORT_REMOSAIC_CAPTURE
    previewHighResolutionSizeLut = PREVIEW_SIZE_LUT_2X5SP;
    previewHighResolutionSizeLutMax = sizeof(PREVIEW_SIZE_LUT_2X5SP) / (sizeof(int) * SIZE_OF_LUT);
    captureHighResolutionSizeLut = PICTURE_SIZE_LUT_2X5SP;
    captureHighResolutionSizeLutMax = sizeof(PICTURE_SIZE_LUT_2X5SP) / (sizeof(int) * SIZE_OF_LUT);
#endif //SUPPORT_REMOSAIC_CAPTURE

    /* Set the max of size/fps lists */
    yuvListMax                  = sizeof(SAK2X5SP_YUV_LIST)                         / (sizeof(int) * SIZE_OF_RESOLUTION);
    jpegListMax                 = sizeof(SAK2X5SP_JPEG_LIST)                            / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(SAK2X5SP_HIGH_SPEED_VIDEO_LIST)                / (sizeof(int) * SIZE_OF_RESOLUTION);
    fpsRangesListMax            = sizeof(SAK2X5SP_FPS_RANGE_LIST)                   / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(SAK2X5SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST)  / (sizeof(int) * 2);
    yuvReprocessingInputListMax = sizeof(SAK2X5SP_YUV_REPROCESSING_INPUT_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rawOutputListMax            = sizeof(SAK2X5SP_RAW_OUTPUT_LIST)                  / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(SAK2X5SP_THUMBNAIL_LIST)                   / (sizeof(int) * SIZE_OF_RESOLUTION);

    /* Set supported  size/fps lists */
    yuvList                     = SAK2X5SP_YUV_LIST;
    jpegList                    = SAK2X5SP_JPEG_LIST;
    highSpeedVideoList          = SAK2X5SP_HIGH_SPEED_VIDEO_LIST;
    fpsRangesList               = SAK2X5SP_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = SAK2X5SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;
    yuvReprocessingInputList    = SAK2X5SP_YUV_REPROCESSING_INPUT_LIST;
    rawOutputList               = SAK2X5SP_RAW_OUTPUT_LIST;
    thumbnailList               = SAK2X5SP_THUMBNAIL_LIST;

    supported_sensor_ex_mode    = (1 << EXTEND_SENSOR_MODE_3DHDR);

    /* Set Vendor lists */
    hiddenPictureListMax        = sizeof(SAK2X5SP_HIDDEN_JPEG_LIST)                 / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenPictureList           = SAK2X5SP_HIDDEN_JPEG_LIST;

    /*
     ** Camera HAL 3.2 Static Metadatas
     **
     ** The order of declaration follows the order of
     ** Android Camera HAL3.2 Properties.
     ** Please refer the "/system/media/camera/docs/docs.html"
     */

    lensFacing = ANDROID_LENS_FACING_FRONT;
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    /* FULL-Level default capabilities */
    supportedCapabilities = (CAPABILITIES_MANUAL_SENSOR | CAPABILITIES_MANUAL_POST_PROCESSING |
                            CAPABILITIES_BURST_CAPTURE | CAPABILITIES_RAW | CAPABILITIES_PRIVATE_REPROCESSING |
                            CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
    requestKeys = AVAILABLE_REQUEST_KEYS_BASIC;
    resultKeys = AVAILABLE_RESULT_KEYS_BASIC;
    characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_BASIC;
    sessionKeys = AVAILABLE_SESSION_KEYS_BASIC;
    requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_BASIC);
    resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_BASIC);
    characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_BASIC);
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_BASIC);

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    if (flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_TRUE) {
        aeModes = AVAILABLE_AE_MODES;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES);
    } else {
        aeModes = AVAILABLE_AE_MODES_NOFLASH;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_NOFLASH);
    }

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.0f;
    fNumber = 2.0f;
    filterDensity = 0.0f;
    focalLength = 3.81f;
    focalLengthIn35mmLength = 21;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 0.0f;
    if (minimumFocusDistance > 0.0f) {
        afModes = AVAILABLE_AF_MODES;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    } else {
        afModes = AVAILABLE_AF_MODES_FIXED;
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FIXED);
        focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    }
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 1;
    maxPipelineDepth = 8;
    partialResultCount = 2;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomRatio = MAX_ZOOM_RATIO;
    maxZoomRatioVendor = MAX_ZOOM_RATIO_VENDOR;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 40;
    sensitivityRange[MAX] = 3200;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 100000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 142857142L;
    sensorPhysicalSize[WIDTH] = 5.184f;
    sensorPhysicalSize[HEIGHT] = 3.888f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 64;
    blackLevelPattern[GR] = 64;
    blackLevelPattern[GB] = 64;
    blackLevelPattern[B] = 64;
    maxAnalogSensitivity = 640;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX333) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX333_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX333_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX333_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX333_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2X5SP_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2X5SP_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2X5SP_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2X5SP_3X3;
    }

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    horizontalViewAngle[SIZE_RATIO_16_9] = 67.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 67.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 51.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 61.0f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_9_16] = 27.4f;
    horizontalViewAngle[SIZE_RATIO_18P5_9] = 65.0f;
    horizontalViewAngle[SIZE_RATIO_19_9] = 65.0f;
    verticalViewAngle = 52.7f;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0

    availableHighSpeedVideoListMax = sizeof(SAK2X5SP_AVAILABLE_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * 5);
    availableHighSpeedVideoList = SAK2X5SP_AVAILABLE_HIGH_SPEED_VIDEO_LIST;

    /* Vendor Feature Metadata */
    vendorExposureTimeRange[MIN] = 60000L;
    vendorExposureTimeRange[MAX] = 32000000000L;

    /* END of Camera HAL 3.2 Static Metadatas */
};

}; /* namespace android */
