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

#ifndef EXYNOS_CAMERA_SENSOR_INFO_BASE_H
#define EXYNOS_CAMERA_SENSOR_INFO_BASE_H

#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <CameraMetadata.h>
#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSizeTable.h"
#include "ExynosCameraAvailabilityTable.h"
#include "fimc-is-metadata.h"

#define UNIQUE_ID_BUF_SIZE          (32)

#ifdef SENSOR_FW_EEPROM_SIZE
#else
#define SENSOR_FW_EEPROM_SIZE          (32)
#endif

#if defined(SUPPORT_X10_ZOOM)
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_ZOOM_RATIO_VENDOR (10000)
#elif defined(SUPPORT_X8_ZOOM)
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_ZOOM_RATIO_VENDOR (8000)
#else
#define MAX_ZOOM_RATIO (4000)
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_ZOOM_RATIO_VENDOR (4000)
#endif

#define ARRAY_LENGTH(x)          (sizeof(x)/sizeof(x[0]))
#define COMMON_DENOMINATOR       (100)
#define EFFECTMODE_META_2_HAL(x) (1 << (x -1))

#define SENSOR_ID_EXIF_SIZE         42
#define SENSOR_ID_EXIF_UNIT_SIZE    16
#define SENSOR_ID_EXIF_TAG         "ssuniqueid"

namespace android {

#define xstr(x)         #x
#define toStr(x)        (char *)xstr(x)
#define toSize(x,y)     (sizeof(x)/sizeof(y))

struct HAL_CameraInfo_t {
    int cameraId;
    int facing_info;
    int orientation;
    int resource_cost;
    char **conflicting_devices;
    size_t conflicting_devices_length;
};

enum max_3a_region {
    AE,
    AWB,
    AF,
    REGIONS_INDEX_MAX,
};
enum size_direction {
    WIDTH,
    HEIGHT,
    SIZE_DIRECTION_MAX,
};
enum coordinate_3d {
    X_3D,
    Y_3D,
    Z_3D,
    COORDINATE_3D_MAX,
};
enum output_streams_type {
    RAW,
    PROCESSED,
    PROCESSED_STALL,
    OUTPUT_STREAM_TYPE_MAX,
};
enum range_type {
    MIN,
    MAX,
    RANGE_TYPE_MAX,
};
enum bayer_cfa_mosaic_channel {
    R,
    GR,
    GB,
    B,
    BAYER_CFA_MOSAIC_CHANNEL_MAX,
};
enum hue_sat_value_index {
    HUE,
    SATURATION,
    VALUE,
    HUE_SAT_VALUE_INDEX_MAX,
};
enum sensor_margin_base_index {
    LEFT_BASE,
    TOP_BASE,
    WIDTH_BASE,
    HEIGHT_BASE,
    BASE_MAX,
};
enum default_fps_index {
    DEFAULT_FPS_STILL,
    DEFAULT_FPS_VIDEO,
    DEFAULT_FPS_EFFECT_STILL,
    DEFAULT_FPS_EFFECT_VIDEO,
    DEFAULT_FPS_MAX,
};

enum SCENARIO {
    SCENARIO_NORMAL              = 0,
    SCENARIO_DUAL_REAR_ZOOM      = 2,
    SCENARIO_DUAL_REAR_PORTRAIT  = 3,
    SCENARIO_DUAL_FRONT_PORTRAIT = 4,
    SCENARIO_MAX
};
enum MODE {
    MODE_PREVIEW = 0,
    MODE_PICTURE,
    MODE_VIDEO,
    MODE_THUMBNAIL,
    MODE_DNG_PICTURE,
};
enum {
    FOCUS_MODE_AUTO                     = (1 << 0),
    FOCUS_MODE_INFINITY                 = (1 << 1),
    FOCUS_MODE_MACRO                    = (1 << 2),
    FOCUS_MODE_FIXED                    = (1 << 3),
    FOCUS_MODE_EDOF                     = (1 << 4),
    FOCUS_MODE_CONTINUOUS_VIDEO         = (1 << 5),
    FOCUS_MODE_CONTINUOUS_PICTURE       = (1 << 6),
    FOCUS_MODE_TOUCH                    = (1 << 7),
    FOCUS_MODE_CONTINUOUS_PICTURE_MACRO = (1 << 8),
};
enum {
    FLASH_MODE_OFF     = (1 << 0),
    FLASH_MODE_AUTO    = (1 << 1),
    FLASH_MODE_ON      = (1 << 2),
    FLASH_MODE_RED_EYE = (1 << 3),
    FLASH_MODE_TORCH   = (1 << 4),
};

enum SERIES_SHOT_MODE {
    SERIES_SHOT_MODE_NONE              = 0,
    SERIES_SHOT_MODE_LLS               = 1,
    SERIES_SHOT_MODE_SIS               = 2,
    SERIES_SHOT_MODE_BURST             = 3,
    SERIES_SHOT_MODE_ERASER            = 4,
    SERIES_SHOT_MODE_BEST_FACE         = 5,
    SERIES_SHOT_MODE_BEST_PHOTO        = 6,
    SERIES_SHOT_MODE_MAGIC             = 7,
    SERIES_SHOT_MODE_SELFIE_ALARM      = 8,
    SERIES_SHOT_MODE_MAX,
};
enum MULTI_CAPTURE_MODE {
    MULTI_CAPTURE_MODE_NONE  = 0,
    MULTI_CAPTURE_MODE_BURST = 1,
    MULTI_CAPTURE_MODE_AGIF  = 2,
    MULTI_CAPTURE_MODE_MAX,
};

enum TRANSIENT_ACTION {
    TRANSIENT_ACTION_NONE               = 0,
    TRANSIENT_ACTION_ZOOMING            = 1,
    TRANSIENT_ACTION_MANUAL_FOCUSING    = 2,
    TRANSIENT_ACTION_MAX,
};

/* refer to fimc_is_ex_mode on kernel side*/
enum EXTEND_SENSOR_MODE {
    EXTEND_SENSOR_MODE_NONE = 0,
    EXTEND_SENSOR_MODE_DRAM_TEST = 1,
    EXTEND_SENSOR_MODE_LIVE_FOCUS = 2,
    EXTEND_SENSOR_MODE_3DHDR = 7,
    EXTEND_SENSOR_MODE_SW_REMOSAIC = 8,
};

enum SENSOR_SYNC_TYPE {
    SENSOR_SYNC_TYPE_APPROXIMATE,
    SENSOR_SYNC_TYPE_CALIBRATED,
};

enum BAYER_PATTERN {
    BAYER_BGGR = 0,
    BAYER_GBRG = 1,
    BAYER_GRBG = 2,
    BAYER_RGGB = 3,
    BAYER_Y = 4,
};

enum SUPPORT_REMOSAIC_MODE {
    SUPPORT_REMOSAIC_MODE_NONE  = 0,
    SUPPORT_REMOSAIC_MODE_HW,
    SUPPORT_REMOSAIC_MODE_SW,
    SUPPORT_REMOSAIC_MODE_MAX,
};

#ifdef SENSOR_NAME_GET_FROM_FILE
int getSensorIdFromFile(int camId);
#endif
#ifdef SENSOR_FW_GET_FROM_FILE
const char *getSensorFWFromFile(struct ExynosCameraSensorInfoBase *info, int camId, int *sensorFwSize);
#endif

typedef enum CAMERA_INDEX {
#if defined(CAMERA_OPEN_ID_REAR_0)
    CAMERA_INDEX_REAR_0,
#endif
#if defined(CAMERA_OPEN_ID_FRONT_1)
    CAMERA_INDEX_FRONT_1,
#endif
#if defined(CAMERA_OPEN_ID_REAR_2)
    CAMERA_INDEX_REAR_2,
#endif
#if defined(CAMERA_OPEN_ID_REAR_3)
    CAMERA_INDEX_REAR_3,
#endif
#if defined(CAMERA_OPEN_ID_REAR_4)
    CAMERA_INDEX_REAR_4,
#endif
#if defined(CAMERA_OPEN_ID_LCAM_0)
    CAMERA_INDEX_LCAM_0,
#endif
    CAMERA_INDEX_DUAL_REAR_ZOOM,
    CAMERA_INDEX_DUAL_REAR_PORTRAIT_TELE,
    CAMERA_INDEX_DUAL_FRONT_PORTRAIT,
    CAMERA_INDEX_DUAL_REAR_PORTRAIT_WIDE,
    CAMERA_INDEX_LOGICAL_REAR_TOF,
    CAMERA_INDEX_LOGICAL_FRONT_TOF,
    CAMERA_INDEX_HIDDEN_REAR_2,
    CAMERA_INDEX_HIDDEN_FRONT_2,
    CAMERA_INDEX_HIDDEN_REAR_3,
    CAMERA_INDEX_HIDDEN_FRONT_3,
    CAMERA_INDEX_HIDDEN_REAR_4,
    CAMERA_INDEX_HIDDEN_FRONT_4,
    CAMERA_INDEX_REAR_TOF,
    CAMERA_INDEX_FRONT_TOF,
    CAMERA_INDEX_IRIS_SECURE,
    CAMERA_INDEX_FRONT_SECURE,
    CAMERA_INDEX_ID_MAX,
} camera_index_t;

struct sensor_id_exif_data {
    char sensor_id_exif[SENSOR_ID_EXIF_SIZE];
};

/* Helpper functions */
int getSensorId(int camId);
#ifdef USE_DUAL_CAMERA
void getDualCameraId(int *cameraId_0, int *cameraId_1, int scenario = SCENARIO_DUAL_REAR_ZOOM);
#endif
#ifdef SENSOR_NAME_GET_FROM_FILE
int getSensorIdFromFile(int camId);
#endif
#ifdef SENSOR_FW_GET_FROM_FILE
const char *getSensorFWFromFile(struct ExynosCameraSensorInfoBase *info, int camId);
#endif

struct exynos_camera_info {
public:
    int     previewW;
    int     previewH;
    int     previewFormat;
    int     previewStride;

    int     pictureW;
    int     pictureH;
    int     pictureFormat;
    int     hwPictureFormat;
    camera_pixel_size hwPicturePixelSize;

    int     videoW;
    int     videoH;

    int     yuvWidth[6];
    int     yuvHeight[6];
    int     yuvFormat[6];

    int     hwYuvWidth[6];
    int     hwYuvHeight[6];

    /* This size for internal */
    int     hwSensorW;
    int     hwSensorH;
    int     yuvSizeRatioId;
    int     yuvSizeLutIndex;
    int     hwPictureW;
    int     hwPictureH;
    int     pictureSizeRatioId;
    int     pictureSizeLutIndex;
    int     hwDisW;
    int     hwDisH;
    int     hwPreviewFormat;

    int     hwBayerCropW;
    int     hwBayerCropH;
    int     hwBayerCropX;
    int     hwBayerCropY;

    int     bnsW;
    int     bnsH;

    int     jpegQuality;
    int     thumbnailW;
    int     thumbnailH;
    int     thumbnailQuality;

    uint32_t    bnsScaleRatio;
    uint32_t    binningScaleRatio;

    bool    is3dnrMode;
    bool    isDrcMode;
    bool    isOdcMode;

    int     flipHorizontal;
    int     flipVertical;

    int     operationMode;
    bool    visionMode;
    int     visionModeFps;
    int     visionModeAeTarget;

    bool    recordingHint;
    bool    ysumRecordingMode;
    bool    pipMode;
#ifdef USE_DUAL_CAMERA
    bool    dualMode;
#endif
    bool    pipRecordingHint;
    bool    effectHint;
    bool    effectRecordingHint;

    bool    highSpeedRecording;
    bool    videoStabilization;
    bool    swVdisMode;
    bool    hyperlapseMode;
    int     hyperlapseSpeed;
    int     shotMode;
    int     vtMode;
    bool    hdrMode;

    char    imageUniqueId[UNIQUE_ID_BUF_SIZE];
    bool    isFactoryApp;
    int     recordingFps;

    int     seriesShotMode;
    int     seriesShotCount;
    int     multiCaptureMode;

    int     deviceOrientation;
};

struct ExynosCameraSensorInfoBase {
public:
#ifdef SENSOR_FW_GET_FROM_FILE
    char	sensor_fw[SENSOR_FW_EEPROM_SIZE];
#endif
    struct sensor_id_exif_data sensor_id_exif_info;
    char    name[EXYNOS_CAMERA_NAME_STR_SIZE];
    int     sensorId;

    int     maxPreviewW;
    int     maxPreviewH;
    int     maxPictureW;
    int     maxPictureH;
    int     maxSensorW;
    int     maxSensorH;

    int     maxCroppedPreviewW;
    int     maxCroppedPreviewH;
    int     maxCroppedPictureW;
    int     maxCroppedPictureH;
    int     maxCroppedSensorW;
    int     maxCroppedSensorH;

    bool    useSensorCrop[SCENARIO_MAX][MAX_NUM_SENSORS];

    int     sensorMarginW;
    int     sensorMarginH;
    int     sensorMarginBase[BASE_MAX];
    int     sensorArrayRatio;

    int     maxThumbnailW;
    int     maxThumbnailH;

    float   horizontalViewAngle[SIZE_RATIO_END];
    float   verticalViewAngle;

    int     minFps;
    int     maxFps;
    int     defaultFpsMin[DEFAULT_FPS_MAX];
    int     defaultFpsMAX[DEFAULT_FPS_MAX];

    bool   stillShotSensorFpsSupport;
    int    stillShotSensorFps;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */
    /* Android ColorCorrection Static Metadata */
    uint8_t    *colorAberrationModes;
    size_t     colorAberrationModesLength;

    /* Android Control Static Metadata */
    uint8_t    *antiBandingModes;
    uint8_t    *aeModes;
    int32_t    exposureCompensationRange[RANGE_TYPE_MAX];
    float      exposureCompensationStep;
    uint8_t    *afModes;
    uint8_t    *effectModes;
    uint8_t    *sceneModes;
    uint8_t    *videoStabilizationModes;
    uint8_t    *awbModes;
    int32_t    *vendorAwbModes;
    int32_t    vendorWbColorTempRange[RANGE_TYPE_MAX];
    int32_t    vendorWbColorTemp;
    int32_t    vendorWbLevelRange[RANGE_TYPE_MAX];
    int32_t    max3aRegions[REGIONS_INDEX_MAX];
    uint8_t    *controlModes;
    size_t     controlModesLength;
    uint8_t    *sceneModeOverrides;
    uint8_t    aeLockAvailable;
    uint8_t    awbLockAvailable;
    size_t     antiBandingModesLength;
    size_t     aeModesLength;
    size_t     afModesLength;
    size_t     effectModesLength;
    size_t     sceneModesLength;
    size_t     videoStabilizationModesLength;
    size_t     awbModesLength;
    size_t     vendorAwbModesLength;
    size_t     sceneModeOverridesLength;
    int32_t    postRawSensitivityBoost[RANGE_TYPE_MAX];

    /* Android Edge Static Metadata */
    uint8_t    *edgeModes;
    size_t     edgeModesLength;

    /* Android Flash Static Metadata */
    uint8_t    flashAvailable;
    int64_t    chargeDuration;
    uint8_t    colorTemperature;
    uint8_t    maxEnergy;

    /* Android Hot Pixel Static Metadata */
    uint8_t   *hotPixelModes;
    size_t    hotPixelModesLength;

    /* Android Lens Static Metadata */
    float     aperture;
    float     fNumber;
    float     filterDensity;
    float     focalLength;
    int       focalLengthIn35mmLength;
    int       focalLengthIn35mmLengthOnCroppedSize;
    uint8_t   *opticalStabilization;
    float     hyperFocalDistance;
    float     minimumFocusDistance;
    int32_t   shadingMapSize[SIZE_DIRECTION_MAX];
    uint8_t   focusDistanceCalibration;
    uint8_t   lensFacing;
    float     opticalAxisAngle[2];
    float     lensPosition[COORDINATE_3D_MAX];
    size_t    opticalStabilizationLength;
    float     poseRotaion[4];
    float     poseTranslation[3];
    float     instrinsicCalibration[5];
    float     distortion[5];
    uint8_t   poseReference;
    /* Vendor Defined Metadata for Lens Static Metadata */
    int       afFovListMax;
    int       (*afFovList)[6];
    /* Android Noise Reduction Static Metadata */
    uint8_t   *noiseReductionModes;
    size_t    noiseReductionModesLength;

    /* Android Request Static Metadata */
    int32_t   maxNumOutputStreams[OUTPUT_STREAM_TYPE_MAX];
    int32_t   maxNumInputStreams;
    uint8_t   maxPipelineDepth;
    int32_t   partialResultCount;
    int32_t   *requestKeys;
    int32_t   *resultKeys;
    int32_t   *characteristicsKeys;
    int32_t   *sessionKeys;
    size_t    requestKeysLength;
    size_t    resultKeysLength;
    size_t    characteristicsKeysLength;
    size_t    sessionKeysLength;

    /* Android Scaler Static Metadata */
    bool      zoomSupport;
    int       maxZoomRatio;
    int       minZoomRatioVendor;
    int       maxZoomRatioVendor;
    int64_t   *stallDurations;
    uint8_t   croppingType;
    size_t    stallDurationsLength;

    /* Android Sensor Static Metadata */
    int32_t   sensitivityRange[RANGE_TYPE_MAX];
    uint8_t   colorFilterArrangement;
    int64_t   exposureTimeRange[RANGE_TYPE_MAX];
    int64_t   maxFrameDuration;
    float     sensorPhysicalSize[SIZE_DIRECTION_MAX];
    int32_t   whiteLevel;
    uint8_t   timestampSource;
    uint8_t   referenceIlluminant1;
    uint8_t   referenceIlluminant2;
    int32_t   blackLevel;
    int32_t   blackLevelPattern[BAYER_CFA_MOSAIC_CHANNEL_MAX];

    int32_t   LedCalibrationMasterCool[BAYER_CFA_MOSAIC_CHANNEL_MAX];
    int32_t   LedCalibrationMasterWarm[BAYER_CFA_MOSAIC_CHANNEL_MAX];
    int32_t   LedCalibrationMasterCoolWarm[BAYER_CFA_MOSAIC_CHANNEL_MAX];

    int32_t   maxAnalogSensitivity;
    int32_t   orientation;
    int32_t   profileHueSatMapDimensions[HUE_SAT_VALUE_INDEX_MAX];
    int32_t   *testPatternModes;
    size_t    testPatternModesLength;
    camera_metadata_rational *colorTransformMatrix1;
    camera_metadata_rational *colorTransformMatrix2;
    camera_metadata_rational *forwardMatrix1;
    camera_metadata_rational *forwardMatrix2;
    camera_metadata_rational *calibration1;
    camera_metadata_rational *calibration2;
    float   masterRGain;
    float   masterBGain;

    int32_t   gain;
    int64_t   exposureTime;
    int32_t   ledCurrent;
    int64_t   ledPulseDelay;
    int64_t   ledPulseWidth;
    int32_t   ledMaxTime;

    int32_t   gainRange[RANGE_TYPE_MAX];
    int32_t   ledCurrentRange[RANGE_TYPE_MAX];
    int64_t   ledPulseDelayRange[RANGE_TYPE_MAX];
    int64_t   ledPulseWidthRange[RANGE_TYPE_MAX];
    int32_t   ledMaxTimeRange[RANGE_TYPE_MAX];

    /* Android Statistics Static Metadata */
    uint8_t   *faceDetectModes;
    int32_t   histogramBucketCount;
    int32_t   maxNumDetectedFaces;
    int32_t   maxHistogramCount;
    int32_t   maxSharpnessMapValue;
    int32_t   sharpnessMapSize[SIZE_DIRECTION_MAX];
    uint8_t   *hotPixelMapModes;
    uint8_t   *lensShadingMapModes;
    size_t    lensShadingMapModesLength;
    uint8_t   *shadingAvailableModes;
    size_t    shadingAvailableModesLength;
    size_t    faceDetectModesLength;
    size_t    hotPixelMapModesLength;

    /* Android Tone Map Static Metadata */
    int32_t   tonemapCurvePoints;
    uint8_t   *toneMapModes;
    size_t    toneMapModesLength;

    /* Android LED Static Metadata */
    uint8_t   *leds;
    size_t    ledsLength;

    /* Android Reprocess Static Metadata */
    int32_t 	maxCaptureStall;

    /* Samsung Vendor Feature */
    int32_t   *vendorFlipModes;
    size_t    vendorFlipModesLength;

    int64_t   vendorExposureTimeRange[RANGE_TYPE_MAX];

    float   *availableApertureValues;
    size_t  availableApertureValuesLength;

    int32_t *availableBurstShotFps;
    size_t  availableBurstShotFpsLength;

    /* Android Info Static Metadata */
    uint8_t   supportedHwLevel;
    uint64_t  supportedCapabilities;

    /* Android Sync Static Metadata */
    int32_t   maxLatency;
    /* END of Camera HAL 3.2 Static Metadatas */

    /* common  available */
    bool   bnsSupport;
    bool   flite3aaOtfSupport;
    bool   sensorGyroSupport;

    /* vendor specifics available */
    bool   sceneHDRSupport;
    bool   screenFlashAvailable;
    bool   objectTrackingAvailable;
    bool   fixedFaceFocusAvailable;
    int    factoryDramTestCount;
    bool   controlZslSupport;
    bool   superNightShotSupport;
    bool   videoBokehSupport;
    bool   multiStreamCaptureSupport;
    bool   depthOnlySensor;
    uint8_t   depthIsExclusive;
#ifdef SUPPORT_SENSOR_REMOSAIC_SW
    bool   swRemosaicSensor;
#endif
    enum BAYER_PATTERN bayerPattern;

    /* The number of YUV(preview, video)/JPEG(picture) sizes in each list */
    int    yuvListMax;
    int    yuvFullListMax;
    int    rawListMax;
    int    jpegListMax;
    int    jpegFullListMax;
    int    tetraJpegListMax;
    int    depthListMax;
    int    thumbnailListMax;
    int    highSpeedVideoListMax;
    int    availableVideoListMax;
    int    availableVideoBeautyListMax;
    int    availableHighSpeedVideoListMax;
    int    fpsRangesListMax;
    int    highSpeedVideoFPSListMax;
    int    yuvReprocessingInputListMax;
    int    rawOutputListMax;

    /* Supported YUV(preview, video)/JPEG(picture) Lists */
    int    (*yuvList)[SIZE_OF_RESOLUTION];
    int    (*yuvFullList)[SIZE_OF_RESOLUTION];
    int    (*rawList)[SIZE_OF_RESOLUTION];
    int    (*jpegList)[SIZE_OF_RESOLUTION];
    int    (*jpegFullList)[SIZE_OF_RESOLUTION];
    int    (*tetraJpegList)[SIZE_OF_RESOLUTION_EXT];
    int    (*depthList)[SIZE_OF_RESOLUTION];
    int    (*thumbnailList)[SIZE_OF_RESOLUTION];
    int    (*highSpeedVideoList)[SIZE_OF_RESOLUTION];
    int    (*availableVideoList)[7];
    int    (*availableVideoBeautyList)[7];
    int    (*availableHighSpeedVideoList)[5];
    int    (*fpsRangesList)[2];
    int    (*highSpeedVideoFPSList)[2];
    int    (*yuvReprocessingInputList)[SIZE_OF_RESOLUTION];
    int    (*rawOutputList)[SIZE_OF_RESOLUTION];

    int    previewSizeLutMax;
    int    pictureSizeLutMax;
    int    videoSizeLutMax;
    int    previewCropSizeLutMax;
    int    pictureCropSizeLutMax;
    int    previewCropFullSizeLutMax;
    int    pictureCropFullSizeLutMax;
    int    pipPreviewSizeLutMax;
    int    vtcallSizeLutMax;
    int    videoSizeLutHighSpeed60Max;
    int    videoSizeLutHighSpeed120Max;
    int    videoSizeLutHighSpeed240Max;
    int    videoSizeLutHighSpeed480Max;
    int    fastAeStableLutMax;
    int    previewFullSizeLutMax;
    int    pictureFullSizeLutMax;
    int    depthMapSizeLutMax;

    int    (*previewSizeLut)[SIZE_OF_LUT];
    int    (*pictureSizeLut)[SIZE_OF_LUT];
    int    (*videoSizeLut)[SIZE_OF_LUT];
    int    (*previewCropSizeLut)[SIZE_OF_LUT];
    int    (*pictureCropSizeLut)[SIZE_OF_LUT];
    int    (*previewCropFullSizeLut)[SIZE_OF_LUT];
    int    (*pictureCropFullSizeLut)[SIZE_OF_LUT];
    int    (*pipPreviewSizeLut)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed60)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed120)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed240)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed480)[SIZE_OF_LUT];
    int    (*vtcallSizeLut)[SIZE_OF_LUT];
    int    (*fastAeStableLut)[SIZE_OF_LUT];
    int    (*previewFullSizeLut)[SIZE_OF_LUT];
    int    (*pictureFullSizeLut)[SIZE_OF_LUT];
    int    (*depthMapSizeLut)[3];
#ifdef SUPPORT_PD_IMAGE
    int    (*pdImageSizeLut)[PD_IMAGE_LUT_SIZE];
    int    pdImageSizeLutMax;
#endif
    bool   sizeTableSupport;
    bool   isTetraSensor;
#ifdef SUPPORT_REMOSAIC_CAPTURE
    int    previewHighResolutionSizeLutMax;
    int    (*previewHighResolutionSizeLut)[SIZE_OF_LUT];
    int    captureHighResolutionSizeLutMax;
    int    (*captureHighResolutionSizeLut)[SIZE_OF_LUT];
#endif //SUPPORT_REMOSAIC_CAPTURE

    int    hiddenPreviewListMax;
    int    (*hiddenPreviewList)[SIZE_OF_RESOLUTION];

    int    hiddenPictureListMax;
    int    (*hiddenPictureList)[SIZE_OF_RESOLUTION];

    int    hiddenThumbnailListMax;
    int    (*hiddenThumbnailList)[2];

    int    hiddenfpsRangeListMax;
    int    (*hiddenfpsRangeList)[2];

    int    effectFpsRangesListMax;
    int    (*effectFpsRangesList)[2];

#ifdef SUPPORT_DEPTH_MAP
    int    availableDepthSizeListMax;
    int    (*availableDepthSizeList)[2];
    int    availableDepthFormatListMax;
    int    *availableDepthFormatList;
#endif

    int    availableVendorDepthSizeListMax;
    int    (*availableVendorDepthSizeList)[2];
    int    availableVendorDepthFormatListMax;
    int    *availableVendorDepthFormatList;
    int    maxDepthSamples;

    int    availableThumbnailCallbackSizeListMax;
    int    (*availableThumbnailCallbackSizeList)[2];
    int    availableThumbnailCallbackFormatListMax;
    int    *availableThumbnailCallbackFormatList;

    int    availableIrisSizeListMax;
    int    (*availableIrisSizeList)[2];
    int    availableIrisFormatListMax;
    int    *availableIrisFormatList;

    int     availableBasicFeaturesListLength;
    int     *availableBasicFeaturesList;

    int     availableOptionalFeaturesListLength;
    int     *availableOptionalFeaturesList;

    int    supported_sensor_ex_mode;
    int    supported_remosaic_mode;
    int    supported_remosaic_modeMax;
public:
    ExynosCameraSensorInfoBase();
};

struct ExynosCameraSensor2L7Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2L7Base();
};

struct ExynosCameraSensor2P6Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2P6Base();
};

struct ExynosCameraSensor2P8Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2P8Base();
};

struct ExynosCameraSensorIMX333_2L2Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorIMX333_2L2Base(int sensorId);
};

struct ExynosCameraSensorIMX576Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorIMX576Base(int sensorId);
};

struct ExynosCameraSensor2L3Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2L3Base(int sensorId);
};

struct ExynosCameraSensor2L4Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2L4Base(int serviceCameraId, int sensorId);
};

struct ExynosCameraSensor3P9Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor3P9Base(int sensorId);
};

struct ExynosCameraSensor6B2Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor6B2Base(int sensorId);
};

struct ExynosCameraSensorIMX320_3H1Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorIMX320_3H1Base(int sensorId);
};

struct ExynosCameraSensor3J1Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor3J1Base(int sensorId);
};

struct ExynosCameraSensor3M3Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor3M3Base(int sensorId);
};

struct ExynosCameraSensorS5K5F1Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorS5K5F1Base(int sensorId);
};

struct ExynosCameraSensorS5KRPBBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorS5KRPBBase(int sensorId);
};

struct ExynosCameraSensor2P7SQBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2P7SQBase(int sensorId);
};

struct ExynosCameraSensor5E9Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor5E9Base(int sensorId);
};

struct ExynosCameraSensor3P8SPBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor3P8SPBase(int sensorId);
};

struct ExynosCameraSensor2T7SXBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2T7SXBase(int sensorId);
};

struct ExynosCameraSensor4HABase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor4HABase(int sensorId);
};

struct ExynosCameraSensor3L2Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor3L2Base(int sensorId);
};

struct ExynosCameraSensor4H5YCBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor4H5YCBase(int sensorId);
};

struct ExynosCameraSensorGM1SPBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensorGM1SPBase(int sensorId);
};

struct ExynosCameraSensor12A10Base : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor12A10Base(int sensorId);
};

struct ExynosCameraSensor12A10FFBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor12A10FFBase(int sensorId);
};

struct ExynosCameraSensor16885CBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor16885CBase(int sensorId);
};

struct ExynosCameraSensor2X5SPBase : public ExynosCameraSensorInfoBase {
public:
    ExynosCameraSensor2X5SPBase(int sensorId);
};

}; /* namespace android */
#endif

