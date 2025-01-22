/*
 * Copyright 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCameraConfig.h
 * \brief     hearder file for ExynosCameraConfig
 * \author    Pilsun Jang(pilsun.jang@samsung.com)
 * \date      2013/7/15
 *
 */

#ifndef EXYNOS_CAMERA_CONFIG_H__
#define EXYNOS_CAMERA_CONFIG_H__

#include "VendorVideoAPI.h"
#include "ExynosCameraCommonMacroUtils.h"
#include "ExynosCameraCommonEnum.h"

/* frame with smart pointer */
#define USE_SMARTPOINTER_FRAME

#ifndef USE_VENDOR_SPECIFIC_CONFIG_HEADER

#define SUPPORT_64BITS
#ifndef CAMERA_DATA_PATH
#define CAMERA_DATA_PATH "/data/vendor/camera/"
#endif

#define USES_OFFLINE_CAPTURE

/* METADATA INTERFACE */
#define DISABLE_USER_META
#define DISABLE_VRA_EXT_META
#define DISABLE_THERMAL_META
#define DISABLE_MCSC_FLIP_META

/******************************
 * Basic Device Features *
 ******************************/
#define CAMERA_OPEN_ID_REAR_0       0
#define CAMERA_OPEN_ID_FRONT_1      1
//#define CAMERA_OPEN_ID_REAR_2       0
//#define CAMERA_OPEN_ID_REAR_2
//#define CAMERA_OPEN_ID_REAR_3
//#define CAMERA_OPEN_ID_LCAM_0
#ifndef USE_DUAL_CAMERA
/* DUAL_CAMERA must be enabled for Logical camera Feature */
#undef CAMERA_OPEN_ID_LCAM_0
#endif

#ifdef  CAMERA_OPEN_ID_LCAM_0
#define USE_LCAM
#if !defined(CAMERA_OPEN_ID_REAR_2) && !defined(CAMERA_OPEN_ID_REAR_3)
#define CAMERA_OPEN_ID_REAR_3
#endif
#endif

#define SUPPORT_SESSION_PARAMETERS

/******************************
 * Debugging Configurations *
 ******************************/
#define MONITOR_LOG_SYNC
#define MONITOR_LOG_SYNC_INTERVAL 100

/* #define PREVIEW_DURATION_DEBUG */
/* #define DEBUG_RAWDUMP */
#ifdef DEBUG_RAWDUMP
#define RAWDUMP_CAPTURE
#endif
/* #define YUV_DUMP */
/* #define DEBUG_STREAM_CONFIGURATIONS */
#define DEBUG_DUMP_IMAGE
#ifdef DEBUG_DUMP_IMAGE
/*
 * ex. if INTERVAL 10, MAX 100 ? dump image of frameCount 1, 11, 21 .. 91
 * ex. if INTERVAL 1,  MAX 100 ? dump image of frameCount 1, 2, 3, 4,.. 100
 * ex. if INTERVAL 33, MAX 100 ? dump image of frameCount 1, 34, 67 .. 100
 */

/*
 *ex. If (pipeId == PIPE_VC0), INTERVAL 10 & MAX_CNT 100, dumps every 10th bayer image (VC0) upto total count 100.
 * ex. if TARGET_PIP MCSC0 INTERVAL 1,  MAX_CNT 100, dumps every MCSC0 YUV image upto total count 100.
 * NOTE: The Images will be dumped only if the correposnding TARGET_PIPE is enabled.
 * Based on the following config, it supports Image dump from the various PIPES.
 * It can support Image Dump for Bayer / YUV Images.
 * It can support MetaPlane Dump (struct camera2_shot_ext)
 * The DUMP_IMAGE mechanism uses different buffer pool to avoid any major
   performance/timing issues to the normal data path.
 * Sample Dump File: CAM0_F16_PIPE_3AA_PIPE-1_BAYER_Fmt-pBAA_PCnt-1_Meta-1_4032x3024_20190424_051611_batchIndex-0.raw
 * The Dump file name has all the basic info related to the dumpled Image.
 ** CAM<0> => Camera ID
 ** F<16> => Frame Count
 ** PIPE_3AA => Pipe Leader
 ** PIPE-<1> => PipeID (Port from which the Image was dumped)
 ** BAYER => Node Name (node correponds to the dumped PIPE)
 ** Fmt-<pBAA> => Format Name (V4L2)
 ** PCnt-<1> => Plane CCount
 ** Meta-<1> => Meta Plane is existed or Not ( 0 or 1)
 ** 4032x3024 => Width x height
 ** 20190424_051611 => YYYYMMDD_HHMMSS
 ** batchIndex-0 => batch index
 */

//If we want to dump every 5th image, DEBUG_DUMP_INTERVAL should be 5
#define DEBUG_DUMP_INTERVAL 1
// If we want to dump 30 images, DEBUG_DUMP_MAX_CNT should be 30
#define DEBUG_DUMP_MAX_CNT 30
// Max number of buffers could be allocated for DUMP_IMAGE. The more the buffers better the performance.
#define DEBUG_DUMP_ALLOC_BUF_CNT (30)
// If we want to dump Images from MCSC ports, DEBUG_DUMP_YUV_REQUIRED should be enabled. it is used for prior buffer allocation.
#define DEBUG_DUMP_YUV_REQUIRED
// If we want to dump metaplane along with Image data, DEBUG_DUMP_METADATA should be enabled.
#define DEBUG_DUMP_METADATA
// If we want to use runtime property config for DUMP_IMAGE feature, USE_PROPERTY_CTRL should be enabled.
// //setprop vendor.hal.camera.debug.dump.image 1
#define DEBUG_DUMP_IMAGE_USE_PROPERTY_CTRL
// If we want to disable DYNAMIC bayer in case of DUMP_IMAGE, please enable NEED_ALWAYS_BAYER.
#define DEBUG_DUMP_IMAGE_NEED_ALWAYS_BAYER
// If we want to dump images from only PIPE_VC0, set the DUMP_CONDITION as below
#define DEBUG_DUMP_CONDITION(pipeId) (pipeId == PIPE_MCSC0)
// setprop vendor.hal.camera.debug.dump.image.pipe 1,8
#define DEBUG_DUMP_IMAGE_USE_RUNTIME_CONFIG

// If we want to dump images from PIPE_VC0 & PIPE_MCSC0, set the DUMP_CONDITION as below
// #define DEBUG_DUMP_CONDITION(pipeId) ((pipeId == PIPE_VC0 || pipeId == PIPE_MCSC0))

#define SEQUENTIAL_RAW_DUMP
#ifdef SEQUENTIAL_RAW_DUMP
#define FRAME_HOLD_MAX_CNT 30
#define SEQUENTIAL_RAW_DUMP_COUNT 30
#define SEQUENTIAL_RAW_DUMP_FRAME_COUNT "vendor.hal.camera.dump.capture_sequential_raw_frame_count"
#ifndef DEBUG_DUMP_IMAGE_NEED_ALWAYS_BAYER
#define DEBUG_DUMP_IMAGE_NEED_ALWAYS_BAYER
#endif
#ifndef DEBUG_DUMP_METADATA
#define DEBUG_DUMP_METADATA
#endif
#endif
#endif

#define SUPPORT_ROTATION_STILL
/* #define USE_ROTATION_STILL_ALWAYS */

#define OIS_CAPTURE
/* #define USE_EXPOSURE_DYNAMIC_SHOT */

/* feature : long exposure */
#define CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT          (100000)
#define PERFRAME_CONTROL_CAMERA_EXPOSURE_TIME_MAX   (300000)
#ifdef USE_EXPOSURE_DYNAMIC_SHOT
#define CAMERA_SENSOR_EXPOSURE_TIME_MAX             (2000000)
#else
#define CAMERA_SENSOR_EXPOSURE_TIME_MAX             (500000)
#endif
/* #define CAMERA_ADD_BAYER_ENABLE */

#define CAPTURE_FD_SYNC_WITH_PREVIEW
#define USE_ALWAYS_FD_ON
#ifdef USE_ALWAYS_FD_ON
#define ALWAYS_FD_MODE (FACEDETECT_MODE_FULL)
#endif

/* dump class memory usage */
/* #define DEBUG_CLASS_INFO */
#ifdef DEBUG_CLASS_INFO
/* use property for class memory usage */
#define DEBUG_PROPERTY_FUNCTION_CLASSINFO
#ifdef DEBUG_PROPERTY_FUNCTION_CLASSINFO
#define CAMERA_DEBUG_PROPERTY_KEY_FUNCTION_CLASSINFO "camera.debug.function.classinfo"
#endif
#endif

#ifdef USE_REMOSAIC_SENSOR
#define SUPPORT_SENSOR_MODE_CHANGE
#define SUPPORT_SENSOR_REMOSAIC_HW
//#define SUPPORT_SENSOR_REMOSAIC_SW
#define SENSOR_MOODE_TRANSITION_FRAME_COUNT 4
#define SUPPORT_REMOSAIC_CAPTURE
#define SUPPORT_REMOSAIC_MODE_BY_SESSION_PARAMETER
#define SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
//#define SUPPORT_REMOSAIC_ROTATION
#define REMOSAIC_ROTATION (90)
#define USE_REMOSAIC_CONTROL_REQUEST
/* force enable remosaic DNG with RAW stream */
#define SUPPORT_REMOSAIC_DNG
#define USE_PURE_BAYER_REMOSAIC_REPROCESSING (true)
#endif //USE_REMOSAIC_SENSOR

/* reprocessing use the preview lut in highspeed scenario.  */
#define SUPPORT_HIGHSPEED_REPROCESSING_LUT

#define SUPPORT_VENDOR_DYNAMIC_SENSORMODE
#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE
/* #define SUPPORT_VENDOR_DYNAMIC_SENSORMODE_BY_STREAM_SIZE */
/* #define SUPPORT_VENDOR_DYNAMIC_SENSORMODE_BY_SESSION_PARAMETER */
/* #define BACK_0_CAMERA_DYNAMIC_SENSOR_MODE */
/* #define BACK_0_CAMERA_DYNAMIC_SENSOR_MODE */
#define FRONT_0_CAMERA_DYNAMIC_SENSOR_MODE
#ifdef FRONT_0_CAMERA_DYNAMIC_SENSOR_MODE
#define FRONT_0_BOUNDARY_IN_DYNAMIC_SENSOR_SIZE (2880*2160)
#define FRONT_0_DYNAMIC_FULL_SENSOR_FPS         (24)
#endif
#endif

#define META_USE_NODE_PIXELSIZE

#ifdef BOARD_CAMERA_USES_SBWC
#define USE_SBWC
#endif

/* #define TIME_LOGGER_ENABLE */
#ifdef TIME_LOGGER_ENABLE
#define TIME_LOGGER_LAUNCH_ENABLE
#endif

#define CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT          (100000)

#ifndef USE_3AG_CAPTURE
#define USE_RAW_REVERSE_PROCESSING
#endif

#ifdef USE_RAW_REVERSE_PROCESSING
/* #define USE_SW_RAW_REVERSE_PROCESSING */
#define USE_HW_RAW_REVERSE_PROCESSING
#endif

/***********************
 * Size Configurations *
 ***********************/
#define LCD_SIZE_DEFAULT                (0)
#define LCD_SIZE_800_480                (1)
#define LCD_SIZE_1280_720               (2)
#define LCD_SIZE_1920_1080              (3)
#define LCD_SIZE_2560_1440              (4)

#define CAMERA_LCD_SIZE                 LCD_SIZE_2560_1440

#define PIP_CAMERA_SUPPORTED
#define PIP_CAMERA_MAX_YUV_STREAM       (3)
#define PIP_CAMERA_MAX_YUV_HEIGHT       (5760)
#define PIP_CAMERA_MAX_YUV_SIZE         (4320 * PIP_CAMERA_MAX_YUV_HEIGHT)

#define CAMERA_BCROP_ALIGN              (4)
#define CAMERA_MCSC_ALIGN               (4)
#define CAMERA_3AA_DS_ALIGN_W           (4)

#define CAMERA_TPU_CHUNK_ALIGN_W        (64)
#define CAMERA_TPU_CHUNK_ALIGN_H        (4)
#define CAMERA_16PX_ALIGN               (16)

/* This value for GSC alignment refer to "csc.h" */
#define GSCALER_IMG_ALIGN               (2)
/* This value for MFC alignment */
#define MFC_ALIGN                       (16)

#define USE_WQHD_RECORDING
#define USE_UHD_RECORDING
#define USE_ISP_BUFFER_SIZE_TO_BDS

#define SUPPORT_X10_ZOOM
#define SCALER_MAX_SCALE_UP_RATIO       (10)
#define SCALER_HIGHSPEED_MAX_SCALE_UP_RATIO  (16)

/* Following flags are used in Single chain configuration.
   Defined here to prevent compilation error */
#define M2M_SCALER_MAX_DOWNSCALE_RATIO  (16)
#define MCSC_DOWN_RATIO_SMALL_YUV       (2)

/*
This code is HACK for additional buffer allocation for the S / W M2M scaler.
S/W M2M scaler need to allocation addtional buffer each planes.
plane[0] = 512, other plane[1~N] = 512/2
*/
#define USE_M2M_SCLAER_BUFFER_MARGIN
#ifdef USE_M2M_SCLAER_BUFFER_MARGIN
#define M2M_SCALER_BUFFER_MARGIN        (512)
#endif

#define USE_CAMERA_SIZE_TABLE           (true)

#define HIGH_RESOLUTION_MIN_PIXEL_SIZE  (12 * 1024 * 1024) /* 12MP pixel */
#define FHD_PIXEL_SIZE                  (1920 * 1080)

/* UHD 4:3 */
#define UHD_DVFS_CEILING_WIDTH          (3840)
#define UHD_DVFS_CEILING_HEIGHT         (2880)

#define UHD_16X9_DVFS_CEILING_WIDTH          (3840)
#define UHD_16X9_DVFS_CEILING_HEIGHT         (2160)

#define YUVSTALL_DSCALED_SIZE_W         (1920)
#define YUVSTALL_DSCALED_SIZE_H         (1080)

#define MAX_VRA_INPUT_WIDTH     (320)
#define MAX_VRA_INPUT_HEIGHT    (240)

#define CAMERA_ME_FORMAT        V4L2_PIX_FMT_YUV32;
#define CAMERA_ME_WIDTH         (32400)
#define CAMERA_ME_HEIGHT        (1)
#define CAMERA_ME_MAX_BUFFER    (32)

#ifdef USES_SW_VDIS
#define VIDEO_MARGIN_UHD_W 4608 /* VIDEO UHD, PREVIEW FHD */
#define VIDEO_MARGIN_UHD_H 2592
#define VIDEO_MARGIN_FHD_W 2304 /* VIDEO FHD, PREVIEW FHD */
#define VIDEO_MARGIN_FHD_H 1296
#define VIDEO_MARGIN_HD_W 1536  /* VIDEO HD, PREVIEW FHD */
#define VIDEO_MARGIN_HD_H 864
#define VIDEO_MARGIN_1920_W 2304  /* VIDEO 1920X816, PREVIEW FHD */
#define VIDEO_MARGIN_816_H  980

#ifdef USE_SUPER_EIS
#define VIDEO_SUPER_EIS_MARGIN_FHD_W 3456 /* VIDEO FHD, PREVIEW FHD */
#define VIDEO_SUPER_EIS_MARGIN_FHD_H 1944
#define VIDEO_SUPER_EIS_MARGIN_HD_W 2304  /* VIDEO HD, PREVIEW FHD */
#define VIDEO_SUPER_EIS_MARGIN_HD_H 1296
#endif

#define NUM_SW_VDIS_INTERNAL_BUFFERS 27
#define USE_SW_VDIS_CONTROL_REQUEST     (true)
#define USE_SW_VDIS_WITH_PREVIEW
#define USE_SW_VDIS_HD30_RECORDING
#define USE_SW_VDIS_FHD60_RECORDING
/* #define USE_SW_VDIS_UHD_RECORDING   */
#define USE_SW_VDIS_1920_816_30_RECORDING
#define USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE
#define USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE_HD
#endif

/* configuration for long exposure capture */
#define USE_LONGEXPOSURECAPTURE
#ifdef USE_LONGEXPOSURECAPTURE
#define USE_LONGEXPOSURECAPTURE_CONTROL_REQUEST
#define ERROR_RESULT_LONGEXPOSURECAPTURE_DALAY_COUNT  (20*10)
#define LONG_EXPSOTURECAPTURE_RETRY (30*6) /* 200ms * 180 : 34sec */
#endif

#define USE_QOS_SETTING
#ifdef USE_QOS_SETTING
#define CLUSTER_MASK                0xFFFF
#define CLUSTER_MIN_SHIFT           0
#define CLUSTER_MAX_SHIFT           16

#define cpu_clk_lock(max_clk, min_clk)  \
        (((max_clk&CLUSTER_MASK) << CLUSTER_MAX_SHIFT)\
        |((min_clk&CLUSTER_MASK) << CLUSTER_MIN_SHIFT))

/* #define SUPPORT_CPU_BOOST_RECORDING_MODE */
/* #define SUPPORT_CPU_BOOST_SW_REMOSAIC_CAPTURE_MODE */
#define SUPPORT_CPU_BOOST_VDIS_RECORDING_MODE
#define SUPPORT_CPU_BOOST_HIGHSPEED240_RECORDING_MODE

#ifdef SUPPORT_CPU_BOOST_RECORDING_MODE
#define BIG_CORE_MAX_LOCK_RECORDING         cpu_clk_lock(1800, 0)
#define LITTLE_CORE_MIN_LOCK_RECORDING      cpu_clk_lock(0, 1248)
#endif

#ifdef SUPPORT_CPU_BOOST_SW_REMOSAIC_CAPTURE_MODE
#define BIG_CORE_MAX_LOCK_SW_REMOSAIC_CAPTURE       cpu_clk_lock(0, 1456)
#define LITTLE_CORE_MIN_LOCK_SW_REMOSAIC_CAPTURE    cpu_clk_lock(0, 442)
#endif

#ifdef SUPPORT_CPU_BOOST_VDIS_RECORDING_MODE
#define BIG_CORE_MAX_LOCK_UHD30_VDIS_RECORDING    cpu_clk_lock(0, 1456)
#define LITTLE_CORE_MIN_LOCK_UHD30_VDIS_RECORDING cpu_clk_lock(0, 442)
#define BIG_CORE_MAX_LOCK_FHD60_VDIS_RECORDING    cpu_clk_lock(0, 1508)
#define LITTLE_CORE_MIN_LOCK_FHD60_VDIS_RECORDING cpu_clk_lock(0, 442)
#endif

#ifdef SUPPORT_CPU_BOOST_HIGHSPEED240_RECORDING_MODE
#define BIG_CORE_MIN_LOCK_HIGHSPEED240_RECORDING        cpu_clk_lock(0, 2210)
#define LITTLE_CORE_MIN_LOCK_HIGHSPEED240_RECORDING     cpu_clk_lock(0, 1638)
#endif

#endif

#if 0
#define USE_3AA_INPUT_CROP                  (false)
#define USE_REPROCESSING_3AA_INPUT_CROP     (false)
#define USE_ISP_INPUT_CROP                  (false)
#define USE_REPROCESSING_ISP_INPUT_CROP     (false)
#define USE_MCSC_INPUT_CROP                 (true)
#define USE_REPROCESSING_MCSC_INPUT_CROP    (true)
#define SUPPORT_PICTURE_YUV_CROP
#endif

/* In case of Picture zoom scenario. Use MSC instead of MCSC input crop */
/* When POST_SCALER_ZOOM is enabled, USE_MCSC_INPUT_CROP must be set true */
/* #define SUPPORT_POST_SCALER_ZOOM */
#define POST_SCALER_ZOOM_PIPE_ID        (PIPE_JPEG_REPROCESSING)

#if 0
#define USE_3AG_CAPTURE
#define CONVERT_3AC_TO_3AG(x) (x + FIMC_IS_VIDEO_30G_NUM - FIMC_IS_VIDEO_30C_NUM)
#define CONVERT_3AP_TO_3AG(x) (x + FIMC_IS_VIDEO_30G_NUM - FIMC_IS_VIDEO_30P_NUM)
#endif

#if defined(USE_3AA_INPUT_CROP) && (USE_3AA_INPUT_CROP == false)
#define SUPPORT_FULL_FOV_FACE_DETECT
#endif

/**************************
 * Control Configurations *
 **************************/
#define USE_SUBDIVIDED_EV

/* #define SET_SETFILE_BY_SHOT */
#define SET_SETFILE_BY_SET_CTRL
#ifdef SET_SETFILE_BY_SET_CTRL
#define SET_SETFILE_BY_SET_CTRL_3AA     (false)
#define SET_SETFILE_BY_SET_CTRL_ISP     (true)
#endif
#define BINNING_SETFILE_INDEX           ISS_SUB_SCENARIO_FHD_240FPS

/********************************
 * SUPPORT VENDOR TAG FEATURE *
 ********************************/
#ifdef USE_SLSI_VENDOR_TAGS

/* Support Vendor YUV Stall */
#define SUPPORT_VENDOR_YUV_STALL

#define SUPPORT_VENDOR_TAG_SCENE_DETECTION

#define SUPPORT_VENDOR_TAG_SENSOR_NAME
#define SUPPORT_VENDOR_TAG_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB
#ifdef SUPPORT_VENDOR_TAG_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB
#define ARCSOFT_DUAL_CALIB_BLOB_PATH "/data/vendor/camera/dual_cal_dump.bin"
#define ARCSOFT_DUAL_CALIB_BLOB_SIZE (2*1024)
#define ARCSOFT_DUAL_RE_CALIB_BLOB_PATH "/mnt/vendor/persist/camera/dualcal/dual_cal_data.bin"
#define ARCSOFT_DUAL_RE_CALIB_MODULE_ID_PATH "/mnt/vendor/persist/camera/dualcal/module_id.bin"
#define DUMP_SERIAL_NUMBER_SIZE 8
#define DUMP_SERIAL_NUMBER_OFFSET 121
#endif
#define SUPPORT_VEDNOR_TAG_SENSOR_FPS_SUPPORT_RANGE
#define SUPPORT_VENDOR_TAG_ENVINFO_LUX_STD
#define SUPPORT_VENDOR_TAG_ENVINFO_LUX_IDX
#define SUPPORT_VENDOR_TAG_ENVINFO_ANALOG_GAIN
#define SUPPORT_VENDOR_TAG_ENVINFO_THERMAL_LEVEL
#define SUPPORT_VENDOR_TAG_ENVINFO_ISO100_GAIN
#define SUPPORT_VENDOR_TAG_ENVINFO_AWB_CCT
#define SUPPORT_VENDOR_TAG_ENVINFO_AWB_DEC
#define SUPPORT_VENDOR_TAG_ENVINFO_LINECOUNT
#define SUPPORT_VENDOR_TAG_ENVINFO_LENS_POS
#define SUPPORT_VENDOR_TAG_ENVINFO_AFD_SUBMODE
#define SUPPORT_VENDOR_TAG_ENVINFO_FLICKER_DETECT

#define SUPPORT_VENDOR_TAG_CONTROL_EXP_PRI
#define SUPPORT_VENDOR_TAG_CONTROL_ISO_PRI
#define SUPPORT_VENDOR_TAG_CONTROL_FLIP_STILL
#define SUPPORT_VENDOR_TAG_CONTROL_FLIP_VIDEO

#ifdef SUPPORT_ROTATION_STILL
#define SUPPORT_VENDOR_TAG_CONTROL_ROTATION_STILL
#endif

#define SUPPORT_VENDOR_TAG_CONTROL_VSTAB
#define SUPPORT_VENDOR_TAG_CONTROL_EV
#define SUPPORT_VENDOR_TAG_CONTROL_CAMERA_ID
#define SUPPORT_VENDOR_TAG_CONTROL_ISO_TOTAL
#define SUPPORT_VENDOR_TAG_CONTROL_HDR_GAIN
#define SUPPORT_VENDOR_TAG_CONTROL_HDR_SHUTTER
#define SUPPORT_VENDOR_TAG_CONTROL_SENSOR_GAIN

#define SUPPORT_VENDOR_TAG_CHI_OVERRIDE_EXPINDEX

#ifndef DISABLE_VRA_EXT_META
#define SUPPORT_VENDOR_TAG_STATS_BLINK_DEGREE
#define SUPPORT_VENDOR_TAG_STATS_BLINK_DETECTED
#define SUPPORT_VENDOR_TAG_STATS_SMILE_CONFIDENCE
#define SUPPORT_VENDOR_TAG_STATS_SMILE_DEGREE
#define SUPPORT_VENDOR_TAG_STATS_GAZE_ANGLE
#define SUPPORT_VENDOR_TAG_STATS_GAZE_DIRECTION
#endif
#define SUPPORT_VENDOR_TAG_STATS_AEC_AECLUX
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AWB_CCT_VALUE
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AEC_AEC_STATUS
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AF_STATUS
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_LENS_SHIFT_MM
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_OBJECT_DISTANCE_MM
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_NEAR_FIELD_MM
#define SUPPORT_VENDOR_TAG_STATS_3RD_INFO_FAR_FIELD_MM

#define SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ENABLE
#define SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_RECT
#define SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ROI

#define SUPPORT_VENDOR_TAG_SNAPSHOT_APPX
#define SUPPORT_VENDOR_TAG_SNAPSHOT_EXIF
#define SUPPORT_VENDOR_TAG_SNAPSHOT_MAKERNOTE

#define SUPPORT_VENDOR_TAG_FACTORY_FOCUS_POS
#define SUPPORT_VENDOR_TAG_FACTORY_CALIBRATION_STATUS
#define SUPPORT_VENDOR_TAG_FACTORY_MODULE_ID
#define SUPPORT_VENDOR_TAG_FACTORY_OIS_GEA
#define SUPPORT_VENDOR_TAG_FACTORY_OIS_HEA
#define SUPPORT_VENDOR_TAG_FACTORY_OIS_FW_VER
#define SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION

#define SUPPORT_VENDOR_TAG_AUX_INFO
#define SUPPORT_VENDOR_TAG_REMOSAIC
#define SUPPORT_VENDOR_TAG_MF_STILL
#define SUPPORT_VENDOR_TAG_NIGHT_SHOT
#define SUPPORT_VENDOR_TAG_SUPER_NIGHT_SHOT
#define SUPPORT_VENDOR_TAG_HDR
#define SUPPORT_VENDOR_TAG_MULTI_FRAME_DENOISE
#define SUPPORT_VENDOR_TAG_SPORT_SHOT
#define SUPPORT_VENDOR_TAG_COMBINE_SINGLE_CAPTURE
#define SUPPORT_VENDOR_TAG_3DHDR
#define SUPPORT_VENDOR_TAG_QUAD_PIXEL
#define SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
#define SUPPORT_VENDOR_TAG_ABORT_CAPTURE
#define SUPPORT_VENDOR_TAG_VIDEO_ACTION
#endif

/**
 * Service IDs must be in a sequence.
 * CAMERA_OPEN_ID_XXX are used for conditional code compilation.
 * Even if some CAMERA_OPEN_ID_XXX is disabled, All enabled IDs must be in a sequence.
 * CAMERA_SERVICE_ID_XXX are used for the camera service IDs.
 */
enum CAMERA_SERV_ID {
#ifdef  CAMERA_OPEN_ID_REAR_0
    CAMERA_SERVICE_ID_BACK,
#endif
#ifdef  CAMERA_OPEN_ID_FRONT_1
    CAMERA_SERVICE_ID_FRONT,
#endif
#ifdef  CAMERA_OPEN_ID_REAR_2
    CAMERA_SERVICE_ID_BACK_2,
#endif
#ifdef  CAMERA_OPEN_ID_FRONT_2
    CAMERA_SERVICE_ID_FRONT_2,
#endif
#ifdef  CAMERA_OPEN_ID_REAR_3
    CAMERA_SERVICE_ID_BACK_3,
#endif
#ifdef  CAMERA_OPEN_ID_FRONT_3
    CAMERA_SERVICE_ID_FRONT_3,
#endif
#ifdef  CAMERA_OPEN_ID_REAR_4
    CAMERA_SERVICE_ID_BACK_4,
#endif
#ifdef  CAMERA_OPEN_ID_FRONT_4
    CAMERA_SERVICE_ID_FRONT_4,
#endif

#ifdef  CAMERA_OPEN_ID_LCAM_0
    CAMERA_SERVICE_ID_LCAM_0,
#endif
    CAMERA_SERVICE_ID_OPEN_MAX,
};

/********************************
 * User Scenario Configurations *
 ********************************/
#define DUAL_CAMERA_SUPPORTED
#define USE_HIGHSPEED_RECORDING         (true)

/* Enabled by BOARD_CAMERA_USES_DUAL_CAMERA */
#ifdef USE_DUAL_CAMERA
//#define USE_MASTER_SLAVE_SWITCHING
#define USE_STANDBY_Q_SKIP_MODE
#define USE_DEFER_STANDB_ON_CONTROL
#define RECHECK_DUAL_MODE_BASED_ON_STANDBY_STATE

#define USE_DUAL_PREVIEW_SW             (true)
#define USE_DUAL_REPROCESSING_SW        (true)
#define USE_DUAL_BAYER_SYNC

#define SUPPORT_MASTER_SENSOR_STANDBY   (true)
#define SUPPORT_SLAVE_SENSOR_STANDBY    (true)

#if (USE_DUAL_PREVIEW_SW)
#define NUM_FUSION_BUFFERS              (14)
#else
#define NUM_FUSION_BUFFERS              (0)
#endif
#define DUAL_SWITCH_TRANSITION_FRAME_COUNT (10)
#define DUAL_TRANSITION_FRAME_COUNT     (30)
#define DUAL_CAPTURE_LOCK_COUNT         (30)
#define SYNC_WAITING_COUNT              (2)
#define DUAL_DYNAMIC_HW_SYNC
#define SUPPORT_MULTI_STREAM_CAPTURE
// TODO: Below info should be moved to sensorInfo
#define MULTI_REAR_CAMERA_MAIN_RATIO    (2.0f) //tele
#define MULTI_REAR_CAMERA_SUB_RATIO     (1.0f) //wide
#define MULTI_REAR_CAMERA_SUB2_RATIO    (5.0f) //ultratele
#endif

#ifdef BOARD_CAMERA_EARLY_FD
#define USE_EARLY_FD_PREVIEW
//#define USE_EARLY_FD_REPROCES
#endif

#ifndef USE_3AG_CAPTURE
#ifdef BOARD_CAMERA_3AA_DNG
#define USE_3AA_DNG                     (true)
#endif
#endif

#ifdef USE_DUAL_CAMERA
#define USES_DUAL_REAR_ZOOM
#define USES_DUAL_REAR_PORTRAIT

#ifdef USES_COMBINE_PLUGIN
#define USES_BOKEH_REFOCUS_CAPTURE
#endif
#endif

#define SUPPORT_PERFRAME_FLIP

#ifdef USES_DUAL_REAR_ZOOM
#define DUAL_PREVIEW_SYNC_MODE_MIN_ZOOM_RATIO 1.5f
#define DUAL_PREVIEW_SYNC_MODE_MAX_ZOOM_RATIO 4.0f
#define DUAL_CAPTURE_SYNC_MODE_MIN_ZOOM_RATIO 1.5f
#define DUAL_CAPTURE_SYNC_MODE_MAX_ZOOM_RATIO 2.0f
#define DUAL_SWITCHING_SYNC_MODE_MIN_ZOOM_RATIO 1.5f
#define DUAL_SWITCHING_SYNC_MODE_MAX_ZOOM_RATIO 2.0f
#endif

/*********************************
 * Internal Logic Configurations *
 *********************************/
#define INITIAL_SKIP_FRAME              (8)
#define VISION_SKIP_FRAME               (4)

#define USE_CAMERA2_USE_FENCE
#define USE_SW_FENCE

//#define SENSOR_NAME_GET_FROM_FILE

/* #define RESERVED_MEMORY_ENABLE */
#define RESERVED_BUFFER_COUNT_MAX       (0)

/* #define FPS_CHECK */
#define FIRST_PREVIEW_TIME_CHECK /* to get time processCaptureRequest() ~ first preview result to frameworks */

#define USE_FD_AE
#define FD_ROTATION                     (true)

#define SUPPORT_BACK_HW_VDIS                (false)
#define SUPPORT_FRONT_HW_VDIS               (false)

#define HW_VDIS_W_RATIO                 (1.2f)
#define HW_VDIS_H_RATIO                 (1.2f)

#define USE_FASTEN_AE_STABLE            (false)
#define USE_FASTEN_AE_STABLE_FRONT      (false)

#define FASTEN_AE_FPS                   (120)
#define FASTEN_AE_FPS_FRONT             (112)
#define MULTI_BUFFER_BASE_FPS           (60)
#define DEFAULT_BINNING_RATIO           (1)
#define MIN_BINNING_RATIO               (1000)
#define MAX_BINNING_RATIO               (6000)

#ifdef SENSOR_NAME_GET_FROM_FILE
#define SENSOR_NAME_PATH_BACK "/sys/class/camera/rear/rear_sensorid"
#define SENSOR_NAME_PATH_BACK_1 "/sys/class/camera/rear/rear2_sensorid"
#define SENSOR_NAME_PATH_FRONT "/sys/class/camera/front/front_sensorid"
#define SENSOR_NAME_PATH_SECURE "/sys/class/camera/secure/secure_sensorid"
#endif

/* EEPROM */
#define SENSOR_FW_GET_FROM_FILE
#define SENSOR_FW_EEPROM_SIZE   (16 * 1024) // 16K

#define SENSOR_FW_PATH_BACK "/data/vendor/camera/gm1_eeprom_data.bin"
#define SENSOR_FW_PATH_BACK_1 "/data/vendor/camera/5e9_eeprom_data.bin"
#define SENSOR_FW_PATH_FRONT "/data/vendor/camera/2x5_otp_cal_data.bin"
#define SENSOR_FW_PATH_FRONT_1 "/sys/class/camera/front/front2_camfw"

/* flashlight control */
#define TORCH_REAR_FILE_PATH "/sys/class/leds/fled-s2mu107/fled_mode"
#define TORCH_FRONT_FILE_PATH "/sys/class/camera/flash/front_torch_flash"
#define TORCH_REAR2_FILE_PATH "/sys/class/camera/flash/rear_torch_flash2"
#define TORCH_REAR3_FILE_PATH "/sys/class/camera/flash/rear_torch_flash3"

#define FLASH_ON_VAL "1 1"
#define FLASH_OFF_VAL "1 0"

/* factory led calibration */
#define LED_CALIBRATION_FILE_PATH "/mnt/vendor/persist/camera/ledcal/rear"

#define USE_MCPIPE_SERIALIZATION_MODE
#ifdef USE_MCPIPE_SERIALIZATION_MODE
#define SUPPORT_HWFC_SERIALIZATION
#endif

#ifdef USE_PIPE_HANDLER
#define SUPPORT_VIRTUALFD_REPROCESSING
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
#define VIRTUALFD_SIZE_W (144)
#define VIRTUALFD_SIZE_H (108)
/*#define DEBUG_VIRTUALFD*/ /* for debugging usage */
#endif
#endif

/* #define SUPPORT_PIP_LIMITTATION_FD */ /* Sub camera do not support FD in PIP mode */

/* #define SUPPORT_HFR_BATCH_MODE */
/* #define USE_SERVICE_BATCH_MODE */

#define USE_3DNR_ALWAYS

#define MAX_OLD_BAYER_KEEP_COUNT (0)

#define SUPPORT_DISPLAY_REGION

/***********************
 * Time Configurations *
 ***********************/
#define HDR_DELAY                       (3)
#define SENSOR_REQUEST_DELAY            (2)
#define SENSOR_STANDBY_DELAY            (2)
#define ZSL_CAPTURE_REQUEST_DELAY       (1) /* To compensate ZSL timing for capture request delay */

/* HACK : To support LLS LCD FLASH scenario. */
#define SUPPORT_LLS_LCD_FLASH_DELAY
#ifdef SUPPORT_LLS_LCD_FLASH_DELAY
#define LLS_LCD_FLASH_DELAY             (4)
#endif


#define CORRECT_TIMESTAMP_FOR_SENSORFUSION
#ifdef CORRECT_TIMESTAMP_FOR_SENSORFUSION
#define CORRECTION_SENSORFUSION         (3000000) /* + 3.0ms */
#endif

/********************************
 * H/W Scenario Configurations *
 ********************************/
#define USE_BINNING_MODE
/* #define USE_YSUM_RECORDING */
#define USE_3AA_CROP_AFTER_BDS
#define SUPPORT_DEPTH_MAP
#ifdef SUPPORT_DEPTH_MAP
#define NUM_DEPTHMAP_BUFFERS             (6)
#define MAX_DEPTH_SIZE_W (504)
#define MAX_DEPTH_SIZE_H (378)
#endif

#define USE_GYRO_HISTORY_FOR_TNR

//#define MAX_SENSOR_GYRO_SIZE_W  // it is sensor size.
#define MAX_SENSOR_GYRO_SIZE_H (4)
#define SENSOR_GYRO_FORMAT     (V4L2_PIX_FMT_SBGGR8)

#ifdef USES_SENSOR_GYRO_FACTORY_MODE
#define SUPPORT_SENSOR_GYRO_BACK_0
//#define SUPPORT_SENSOR_GYRO_BACK_1
//#define SUPPORT_SENSOR_GYRO_FRONT_0
//#define SUPPORT_SENSOR_GYRO_FRONT_1
#endif // USES_SENSOR_GYRO_FACTORY_MODE

/* Enable the define to set the BDS_OFF mode for Recording Scenarios */
#define USE_BDS_OFF
/* #define BDS_OFF_ALWAYS */
/* #define BDS_OFF_VIDEO */

/* #define SUPPORT_HW_GDC               (true) */

#define USE_SW_MCSC_REPROCESSING        (true)
/* it is use the nv21-jpeg capture, hifills, lls or etc. */
/* use the remain reprocessing mcsc port for nv21 capture. */
/* #define USE_NV21JPEG_REMAIN_MCSCPORT    (true) */
#define USE_RESERVED_NODE_PPJPEG_MCSCPORT    (true)

//#define USE_HW_BEAUTY_CONTROL
#ifdef USE_HW_BEAUTY_CONTROL
#define BEAUTY_FACE_STRENGTH_MIN    (-1)
#define BEAUTY_FACE_STRENGTH_MAX    (1)
#define BEAUTY_FACE_STRENGTH_DEFAULT (0)
#endif

#ifdef USES_SW_VDIS
#define SUPPORT_ME
#endif

/* For PlugIn Features */
#ifdef USES_HIFI_LLS
/*
    1. check HIFI LLS condition.
     - front camera not support zoom

                       Zoom
                        |         |                   |                        |
                        |         |                   |                        | Back(5)
(BOUNDDRY_IN_ZOOM)      |---------|-------------------|------------------------|-------
                        | Back(3) |                   |                        |
                        | Front(3)|                   |                        |
(BOUNDDRY_IN_ZOOM_FRONT)-------------------------------------------------------|------- BV
                             (BOUNDARY_IN_VALUE) (BOUNDARY_OUT_VALUE) (BOUNDARY_SR_IN_BV)
*/
#define BOUNDDRY_SR_IN_ZOOM     (4.0F)
#define BOUNDDRY_IN_ZOOM        (4.0F)
#define BOUNDDRY_IN_ZOOM_FRONT  (1.0F)
#define BOUNDDRY_IN_BV  (-770)
#define BOUNDARY_OUT_BV (-740)
#define BOUNDARY_SR_IN_BV (1536)
#ifdef SUPPORT_VENDOR_TAG_MF_STILL
#define USE_HIFILLS_CONTROL_REQUEST
#endif
#endif

#ifdef USES_NIGHT_SHOT_YUV
#define USES_NIGHT_SHOT_YUV_CONTROL_REQUEST
#endif

#ifdef USES_HDR_YUV
#define USES_HDR_YUV_CONTROL_REQUEST
#endif

#define SKIP_HDR_PREVIEW

#ifdef USES_COMBINE_PLUGIN
#define USES_COMBINE_PLUGIN_CONTROL_REQUEST
/* To support YUV stream for preview plugin */
/* #define SUPPORT_PREVIEW_PLUGIN_YUV_STREAM */
#endif

/* config mode change for restart */
#define SUPPORT_RESTART_TRANSITION_HIGHSPEED

/* For capture */
#define USE_JPEG_HWFC                   (false)
#define USE_JPEG_HWFC_ONDEMAND          (false)

#define MAIN_CAMERA_SINGLE_REPROCESSING     (true)
#define MAIN_CAMERA_SINGLE_SCC_CAPTURE      (false)
#define MAIN_CAMERA_DUAL_REPROCESSING       (true)
#define MAIN_CAMERA_DUAL_SCC_CAPTURE        (false)

#define FRONT_CAMERA_SINGLE_REPROCESSING    (true)
#define FRONT_CAMERA_SINGLE_SCC_CAPTURE     (false)
#define FRONT_CAMERA_DUAL_REPROCESSING      (true)
#define FRONT_CAMERA_DUAL_SCC_CAPTURE       (false)

#define USE_PURE_BAYER_REPROCESSING                    (true)
#define USE_PURE_BAYER_REPROCESSING_ON_RECORDING       (true)
#define USE_PURE_BAYER_REPROCESSING_ON_PIP             (false)
#define USE_PURE_BAYER_REPROCESSING_ON_PIP_RECORDING   (false)

#define USE_PURE_BAYER_REPROCESSING_FRONT              (true)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING (true)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_PIP       (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_PIP_RECORDING  (false)

#ifdef USE_DUAL_CAMERA
#define USE_PURE_BAYER_REPROCESSING_ON_DUAL            (false)
#define USE_PURE_BAYER_REPROCESSING_ON_DUAL_RECORDING  (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL      (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL_RECORDING  (false)
#endif

/* This USE_DYNAMIC_BAYER define is for default scenario.
 * See <ExynosCameraParameter.cpp> for details of dynamic bayer setting
 */
#define USE_DYNAMIC_BAYER               (true)
#define USE_DYNAMIC_BAYER_60FPS         (true)
#define USE_DYNAMIC_BAYER_120FPS        (true)
#define USE_DYNAMIC_BAYER_240FPS        (true)
#define USE_DYNAMIC_BAYER_480FPS        (true)

#define USE_DYNAMIC_BAYER_FRONT         (true)
#define USE_DYNAMIC_BAYER_60FPS_FRONT   (true)
#define USE_DYNAMIC_BAYER_120FPS_FRONT  (true)
#define USE_DYNAMIC_BAYER_240FPS_FRONT  (true)
#define USE_DYNAMIC_BAYER_480FPS_FRONT  (true)

enum REPROCESSING_BAYER_MODE {
    REPROCESSING_BAYER_MODE_NONE            = 0, /* This means capture do not use reprocessing */
    REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON,
    REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON,
    REPROCESSING_BAYER_MODE_PURE_DYNAMIC,
    REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC,
    REPROCESSING_BAYER_MODE_MAX,
};

#define USE_DYNAMIC_SCC_REAR                (false)
#define USE_DYNAMIC_SCC_FRONT               (false)

#define USE_GSC_FOR_CAPTURE_BACK            (false)
#define USE_GSC_FOR_CAPTURE_FRONT           (false)

/* for PDP, PASTAT */
#define USE_PAF
#ifdef USE_PAF
#define USE_PAF_FOR_PREVIEW
#endif

/* #define USE_VRA_FD */
/* #define USE_MCSC_DS */

#define SUPPORT_3AF

/* Node num infomation */
/* back */
#define BACK_CAMERA_FLITE_NUM             FIMC_IS_VIDEO_SS0_NUM
#define BACK_2_CAMERA_FLITE_NUM             FIMC_IS_VIDEO_SS2_NUM
#define BACK_3_CAMERA_FLITE_NUM             FIMC_IS_VIDEO_SS3_NUM
#define BACK_4_CAMERA_FLITE_NUM             FIMC_IS_VIDEO_SS6_NUM

#define MAIN_CAMERA_FLITE_NUM               BACK_CAMERA_FLITE_NUM
#define MAIN_1_CAMERA_FLITE_NUM             BACK_2_CAMERA_FLITE_NUM

#define BACK_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS0VC1_NUM
#define BACK_2_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS2VC1_NUM
#define BACK_3_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS4VC1_NUM
#define BACK_4_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS6VC1_NUM

#define MAIN_CAMERA_DEPTH_VC_NUM            BACK_CAMERA_DEPTH_VC_NUM
#define MAIN_CAMERA_HAS_OWN_SCC             (false)

/* front */
#define FRONT_0_CAMERA_FLITE_NUM            FIMC_IS_VIDEO_SS1_NUM
#define FRONT_2_CAMERA_FLITE_NUM            FIMC_IS_VIDEO_SS3_NUM
#define FRONT_3_CAMERA_FLITE_NUM            FIMC_IS_VIDEO_SS5_NUM
#define FRONT_4_CAMERA_FLITE_NUM            FIMC_IS_VIDEO_SS7_NUM

#define FRONT_CAMERA_FLITE_NUM              FRONT_0_CAMERA_FLITE_NUM

#define FRONT_0_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS1VC1_NUM
#define FRONT_2_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS3VC1_NUM
#define FRONT_3_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS5VC1_NUM
#define FRONT_4_CAMERA_DEPTH_VC_NUM          FIMC_IS_VIDEO_SS7VC1_NUM

#define FRONT_CAMERA_DEPTH_VC_NUM           FRONT_0_CAMERA_DEPTH_VC_NUM
#define FRONT_CAMERA_HAS_OWN_SCC            (false)

/* Vision, Secure Camera */
#define VISION_CAMERA_FLITE_NUM  FIMC_IS_VIDEO_SS1_NUM
#define SECURE_CAMERA_FLITE_NUM  FIMC_IS_VIDEO_SS3_NUM  //IRIS use FIMC_IS_VIDEO_SS3_NUM

/* H/W path configuration */
#define TYPE_OF_HW_CHAINS                   (HW_CHAIN_TYPE_SEMI_DUAL_CHAIN)
#define NUM_OF_MCSC_INPUT_PORTS             (1)
#define NUM_OF_MCSC_OUTPUT_PORTS            (3)

/* Preview */
/* FLITE - 3AA */
#define MAIN_CAMERA_SINGLE_FLITE_3AA_OTF    (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_SINGLE_FLITE_3AA_OTF   (HW_CONNECTION_MODE_OTF)
#define MAIN_CAMERA_PIP_FLITE_3AA_OTF       (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_PIP_FLITE_3AA_OTF      (HW_CONNECTION_MODE_OTF)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_FLITE_3AA_OTF      (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_DUAL_FLITE_3AA_OTF     (HW_CONNECTION_MODE_OTF)
#define SUB_CAMERA_DUAL_FLITE_3AA_OTF       (HW_CONNECTION_MODE_OTF)
#endif

/* 3AA - ISP */
#define MAIN_CAMERA_SINGLE_3AA_ISP_OTF      (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_3AA_ISP_OTF     (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_PIP_3AA_ISP_OTF         (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_PIP_3AA_ISP_OTF        (HW_CONNECTION_MODE_M2M)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_3AA_ISP_OTF        (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_DUAL_3AA_ISP_OTF       (HW_CONNECTION_MODE_M2M)
#define SUB_CAMERA_DUAL_3AA_ISP_OTF         (HW_CONNECTION_MODE_M2M)
#endif

#define MAIN_CAMERA_SINGLE_3AA_VRA_OTF      (HW_CONNECTION_MODE_NONE)

/* ISP - MCSC */
#define MAIN_CAMERA_SINGLE_ISP_MCSC_OTF     (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_SINGLE_ISP_MCSC_OTF    (HW_CONNECTION_MODE_OTF)
#define MAIN_CAMERA_PIP_ISP_MCSC_OTF        (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_PIP_ISP_MCSC_OTF       (HW_CONNECTION_MODE_OTF)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_ISP_MCSC_OTF       (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_DUAL_ISP_MCSC_OTF      (HW_CONNECTION_MODE_OTF)
#define SUB_CAMERA_DUAL_ISP_MCSC_OTF        (HW_CONNECTION_MODE_OTF)
#endif

/* MCSC - VRA */
#define MAIN_CAMERA_SINGLE_MCSC_VRA_OTF    	(HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_SINGLE_MCSC_VRA_OTF    (HW_CONNECTION_MODE_NONE)
#define MAIN_CAMERA_PIP_MCSC_VRA_OTF        (HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_PIP_MCSC_VRA_OTF       (HW_CONNECTION_MODE_NONE)
#define MAIN_CAMERA_PIP_3AA_VRA_OTF         (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_PIP_3AA_VRA_OTF         (HW_CONNECTION_MODE_M2M)

/* 3AA - VRA */
#define MAIN_CAMERA_SINGLE_3AA_VRA_OTF      (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_3AA_VRA_OTF     (HW_CONNECTION_MODE_M2M)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_3AA_VRA_OTF        (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_DUAL_3AA_VRA_OTF       (HW_CONNECTION_MODE_M2M)
#define SUB_CAMERA_DUAL_3AA_VRA_OTF         (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_DUAL_MCSC_VRA_OTF      	(HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_DUAL_MCSC_VRA_OTF      (HW_CONNECTION_MODE_NONE)
#define SUB_CAMERA_DUAL_MCSC_VRA_OTF        (HW_CONNECTION_MODE_NONE)
#endif

/* MCSC - CLAHE */
#define MAIN_CAMERA_SINGLE_MCSC_CLAHE_OTF   (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_MCSC_CLAHE_OTF  (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_PIP_MCSC_CLAHE_OTF      (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_PIP_MCSC_CLAHE_OTF     (HW_CONNECTION_MODE_M2M)


/* Reprocessing */
#ifdef USE_PAF
/* PAF - 3AA */
#define MAIN_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING     (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_SINGLE_PAF_3AA_OTF_REPROCESSING    (HW_CONNECTION_MODE_OTF)
#define MAIN_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING        (HW_CONNECTION_MODE_NONE) /* need to be same with FRONT_CAMERA_DUAL_PAF_3AA_OTF on dual case */
#define FRONT_CAMERA_PIP_PAF_3AA_OTF_REPROCESSING       (HW_CONNECTION_MODE_NONE)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING       (HW_CONNECTION_MODE_NONE) /* need to be same with FRONT_CAMERA_DUAL_PAF_3AA_OTF on dual case */
#define FRONT_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING      (HW_CONNECTION_MODE_NONE)
#define SUB_CAMERA_DUAL_PAF_3AA_OTF_REPROCESSING        (HW_CONNECTION_MODE_NONE)
#endif
#endif /* USE_PAF */

/* 3AA - ISP */
#define MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING     (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING    (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING        (HW_CONNECTION_MODE_M2M) /* need to be same with FRONT_CAMERA_DUAL_3AA_ISP_OTF on dual case */
#define FRONT_CAMERA_PIP_3AA_ISP_OTF_REPROCESSING       (HW_CONNECTION_MODE_M2M)
#ifdef USE_REMOSAIC_SENSOR
#define MAIN_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING     (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_REMOSAIC_3AA_ISP_OTF_REPROCESSING    (HW_CONNECTION_MODE_M2M)
#endif
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING       (HW_CONNECTION_MODE_M2M) /* need to be same with FRONT_CAMERA_DUAL_3AA_ISP_OTF on dual case */
#define FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING      (HW_CONNECTION_MODE_M2M)
#define SUB_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING        (HW_CONNECTION_MODE_M2M)
#endif

/* ISP - MCSC */
#define MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING    (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING   (HW_CONNECTION_MODE_OTF)
#define MAIN_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING       (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_PIP_ISP_MCSC_OTF_REPROCESSING      (HW_CONNECTION_MODE_OTF)
#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING      (HW_CONNECTION_MODE_OTF)
#define FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING     (HW_CONNECTION_MODE_OTF)
#define SUB_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING       (HW_CONNECTION_MODE_OTF)
#endif

/* MCSC - VRA */
#define MAIN_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING     (HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_SINGLE_MCSC_VRA_OTF_REPROCESSING    (HW_CONNECTION_MODE_NONE)
#define MAIN_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING        (HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_PIP_MCSC_VRA_OTF_REPROCESSING       (HW_CONNECTION_MODE_NONE)

/* 3AA - VRA */
#define MAIN_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING     (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_3AA_VRA_OTF_REPROCESSING    (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING        (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_PIP_3AA_VRA_OTF_REPROCESSING       (HW_CONNECTION_MODE_M2M)

#ifdef USE_DUAL_CAMERA
#define MAIN_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING       (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING      (HW_CONNECTION_MODE_M2M)
#define SUB_CAMERA_DUAL_3AA_VRA_OTF_REPROCESSING        (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING      (HW_CONNECTION_MODE_NONE)
#define FRONT_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING      (HW_CONNECTION_MODE_NONE)
#define SUB_CAMERA_DUAL_MCSC_VRA_OTF_REPROCESSING        (HW_CONNECTION_MODE_NONE)
#endif

/* MCSC - CLAHE */
#define MAIN_CAMERA_SINGLE_MCSC_CLAHE_OTF_REPROCESSING    (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_SINGLE_MCSC_CLAHE_OTF_REPROCESSING   (HW_CONNECTION_MODE_M2M)
#define MAIN_CAMERA_PIP_MCSC_CLAHE_OTF_REPROCESSING       (HW_CONNECTION_MODE_M2M)
#define FRONT_CAMERA_PIP_MCSC_CLAHE_OTF_REPROCESSING      (HW_CONNECTION_MODE_M2M)

#define SCENARIO_SHIFT              (28)
#define REPROCESSING_SHIFT          (28)
#define SSX_VINDEX_SHIFT            (16)

#define SENSOR_INSTANT_SHIFT        (16)
#define SENSOR_USE_STANDBY_SHIFT    (4)

#define INPUT_SENSOR_MASK    0xFC000000 /* sensor scenario */
#define INPUT_SENSOR_SHIFT    26
#define INPUT_STREAM_MASK    0x03000000 /* stream type : 1 (reprocessing) */
#define INPUT_STREAM_SHIFT    24
#define INPUT_POSITION_MASK  0x00FF0000 /* sensor position */
#define INPUT_POSITION_SHIFT  16
#define INPUT_VINDEX_MASK    0x0000FF00 /* video node index : connected node */
#define INPUT_VINDEX_SHIFT    8
#define INPUT_MEMORY_MASK    0x000000F0 /* memory interface : 0 (on-the-fly) */
#define INPUT_MEMORY_SHIFT    4
#define INPUT_LEADER_MASK    0x0000000F /* leader : 1 (leader video node) */
#define INPUT_LEADER_SHIFT    0

#define PREVIEW_GSC_NODE_NUM            (4)  /* 4 = MSC from Exynos5420 */
#define PICTURE_GSC_NODE_NUM            (4)  /* 0,1,2 = GSC */
#define VIDEO_GSC_NODE_NUM              (4)

#define FRONT_CAMERA_3AA_NUM            FIMC_IS_VIDEO_30S_NUM

/*************************
 * Buffer Configurations *
 *************************/
#define MAX_BUFFERS                         (VIDEO_MAX_FRAME)

#define NUM_REQUEST_RAW_BUFFER              (6)
#define NUM_REQUEST_BAYER_BUFFER            (6)
#define NUM_REQUEST_PREVIEW_BUFFER          (8)
#define NUM_REQUEST_CALLBACK_BUFFER         (8)
#define NUM_REQUEST_VIDEO_BUFFER            (8)
#define NUM_REQUEST_JPEG_BUFFER             (4)
#define NUM_REQUEST_BURST_CAPTURE_BUFFER    (4)
#define NUM_REQUEST_CAPTURE_BUFFER          (2)
#ifdef USE_DUAL_CAMERA
#define DUAL_NUM_REQUEST_PREVIEW_BUFFER     (NUM_REQUEST_PREVIEW_BUFFER + SYNC_WAITING_COUNT)
#define DUAL_NUM_REQUEST_CALLBACK_BUFFER    (NUM_REQUEST_CALLBACK_BUFFER + SYNC_WAITING_COUNT)
#define DUAL_NUM_REQUEST_VIDEO_BUFFER       (NUM_REQUEST_VIDEO_BUFFER + SYNC_WAITING_COUNT)
#endif

#define NUM_FASTAESTABLE_BUFFERS            (10)
#define NUM_SENSOR_BUFFERS                  (4 + REPROCESSING_BAYER_HOLD_COUNT + SENSOR_REQUEST_DELAY)
#define NUM_3AA_BUFFERS                     (6 + SENSOR_REQUEST_DELAY)
#define NUM_HW_DIS_BUFFERS                  (NUM_3AA_BUFFERS + 1)
#define NUM_VRA_BUFFERS                     (6)

#ifdef USE_DUAL_CAMERA
#ifdef USE_DUAL_BAYER_SYNC
#define DUAL_NUM_SENSOR_BUFFERS             ((NUM_SENSOR_BUFFERS * 2) + SYNC_WAITING_COUNT)
#else
#define DUAL_NUM_SENSOR_BUFFERS             (NUM_SENSOR_BUFFERS * 2)
#endif
#define DUAL_NUM_3AA_BUFFERS                (NUM_3AA_BUFFERS * 2)
#define DUAL_NUM_HW_DIS_BUFFERS             (NUM_HW_DIS_BUFFERS * 2)
#define DUAL_NUM_SYNC_FUSION_BUFFERS        ((SYNC_WAITING_COUNT + 2) * 2)
#endif

#define FRONT_NUM_SENSOR_BUFFERS            (NUM_SENSOR_BUFFERS)
#define FRONT_NUM_3AA_BUFFERS               (NUM_3AA_BUFFERS)

#define NUM_REPROCESSING_BUFFERS            (4)
#define NUM_PICTURE_BUFFERS                 (5)

#define NUM_BAYER_BUFFERS                   (5 + REPROCESSING_BAYER_HOLD_COUNT)
#define NUM_PREVIEW_BUFFERS                 (9)
#define NUM_RECORDING_BUFFERS               (8)

#define PIPE_FLITE_PREPARE_COUNT            (2)
#define PIPE_3AA_PREPARE_COUNT              (2)

#ifdef DEBUG_RAWDUMP
#define RESERVED_NUM_BAYER_BUFFERS          (0)
#define RESERVED_NUM_ISP_BUFFERS            (0)
#define FRONT_RESERVED_NUM_BAYER_BUFFERS    (0)
#define FRONT_RESERVED_NUM_ISP_BUFFERS      (0)
#else
#define RESERVED_NUM_BAYER_BUFFERS          (NUM_BAYER_BUFFERS)
#define RESERVED_NUM_ISP_BUFFERS            (0)
#define FRONT_RESERVED_NUM_BAYER_BUFFERS    (6)
#define FRONT_RESERVED_NUM_ISP_BUFFERS      (0)
#endif /* DEBUG_RAWDUMP */
#define RESERVED_NUM_SECURE_BUFFERS     (5)

/* TO DO : will remove */
#define REPROCESSING_BAYER_HOLD_COUNT   (1)

#if (USE_HIGHSPEED_RECORDING)
/* HIGHSPEED_120 */
#define FPS120_NUM_REQUEST_RAW_BUFFER           (NUM_REQUEST_RAW_BUFFER)
#define FPS120_NUM_REQUEST_BAYER_BUFFER         (NUM_REQUEST_BAYER_BUFFER)
#define FPS120_NUM_REQUEST_PREVIEW_BUFFER       (NUM_REQUEST_PREVIEW_BUFFER)
#define FPS120_NUM_REQUEST_CALLBACK_BUFFER      (NUM_REQUEST_CALLBACK_BUFFER * 3)
#define FPS120_NUM_REQUEST_VIDEO_BUFFER         (NUM_REQUEST_VIDEO_BUFFER * 3)
#define FPS120_NUM_REQUEST_JPEG_BUFFER          (NUM_REQUEST_JPEG_BUFFER)
#define FPS120_NUM_REQUEST_BURST_CAPTURE_BUFFER (NUM_REQUEST_BURST_CAPTURE_BUFFER)
#define FPS120_NUM_REQUEST_CAPTURE_BUFFER       (NUM_REQUEST_CAPTURE_BUFFER)

#define FPS120_NUM_SENSOR_BUFFERS               (NUM_SENSOR_BUFFERS * 3)
#define FPS120_NUM_3AA_BUFFERS                  (NUM_3AA_BUFFERS * 3)
#define FPS120_NUM_HW_DIS_BUFFERS               (NUM_HW_DIS_BUFFERS * 3)

#define FPS120_NUM_BAYER_BUFFERS                (NUM_BAYER_BUFFERS * 3)
#define FPS120_NUM_PREVIEW_BUFFERS              (NUM_PREVIEW_BUFFERS * 3)
#define FPS120_NUM_RECORDING_BUFFERS            (NUM_RECORDING_BUFFERS * 3)

#ifdef SUPPORT_HFR_BATCH_MODE
#define FPS120_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT)
#define FPS120_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT)
#else
#define FPS120_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT * 3)
#define FPS120_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT * 3)
#endif

/* HIGHSPEED_240 */
#define FPS240_NUM_REQUEST_RAW_BUFFER           (NUM_REQUEST_RAW_BUFFER)
#define FPS240_NUM_REQUEST_BAYER_BUFFER         (NUM_REQUEST_BAYER_BUFFER)
#define FPS240_NUM_REQUEST_PREVIEW_BUFFER       (NUM_REQUEST_PREVIEW_BUFFER)
#define FPS240_NUM_REQUEST_CALLBACK_BUFFER      (NUM_REQUEST_CALLBACK_BUFFER)
#define FPS240_NUM_REQUEST_VIDEO_BUFFER         (NUM_REQUEST_VIDEO_BUFFER * 4)
#define FPS240_NUM_REQUEST_JPEG_BUFFER          (NUM_REQUEST_JPEG_BUFFER)
#define FPS240_NUM_REQUEST_BURST_CAPTURE_BUFFER (NUM_REQUEST_BURST_CAPTURE_BUFFER)
#define FPS240_NUM_REQUEST_CAPTURE_BUFFER       (NUM_REQUEST_CAPTURE_BUFFER)

#define FPS240_NUM_SENSOR_BUFFERS               (NUM_SENSOR_BUFFERS * 4)
#define FPS240_NUM_3AA_BUFFERS                  (32)   /* VIDEO_MAX_FRAME : 32 */
#define FPS240_NUM_HW_DIS_BUFFERS               (NUM_HW_DIS_BUFFERS * 3)

#define FPS240_NUM_BAYER_BUFFERS                (NUM_BAYER_BUFFERS * 3)
#define FPS240_NUM_PREVIEW_BUFFERS              (NUM_PREVIEW_BUFFERS * 3)
#define FPS240_NUM_RECORDING_BUFFERS            (NUM_RECORDING_BUFFERS * 3)

#ifdef SUPPORT_HFR_BATCH_MODE
#define FPS240_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT)
#define FPS240_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT)
#else
#define FPS240_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT * 3)
#define FPS240_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT * 3)
#endif

/* HIGHSPEED_480 */
#define FPS480_NUM_REQUEST_RAW_BUFFER           (NUM_REQUEST_RAW_BUFFER)
#define FPS480_NUM_REQUEST_PREVIEW_BUFFER       (NUM_REQUEST_PREVIEW_BUFFER)
#define FPS480_NUM_REQUEST_CALLBACK_BUFFER      (NUM_REQUEST_CALLBACK_BUFFER * 3)
#define FPS480_NUM_REQUEST_VIDEO_BUFFER         (NUM_REQUEST_VIDEO_BUFFER * 3)
#define FPS480_NUM_REQUEST_JPEG_BUFFER          (NUM_REQUEST_JPEG_BUFFER)

#define FPS480_NUM_SENSOR_BUFFERS               (NUM_SENSOR_BUFFERS * 3)
#define FPS480_NUM_3AA_BUFFERS                  (32)   /* VIDEO_MAX_FRAME : 32 */
#define FPS480_NUM_HW_DIS_BUFFERS               (NUM_HW_DIS_BUFFERS * 3)

#define FPS480_NUM_BAYER_BUFFERS                (NUM_BAYER_BUFFERS * 3)
#define FPS480_NUM_PREVIEW_BUFFERS              (NUM_PREVIEW_BUFFERS * 3)
#define FPS480_NUM_RECORDING_BUFFERS            (NUM_RECORDING_BUFFERS * 3)

#ifdef SUPPORT_HFR_BATCH_MODE
#define FPS480_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT)
#define FPS480_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT)
#else
#define FPS480_PIPE_FLITE_PREPARE_COUNT         (PIPE_FLITE_PREPARE_COUNT * 3)
#define FPS480_PIPE_3AA_PREPARE_COUNT           (PIPE_3AA_PREPARE_COUNT * 3)
#endif
#endif

#define EXYNOS_CAMERA_BUFFER_MAX_PLANES (17)     /* img buffer 4 + metadata 1 */
#define EXYNOS_CAMERA_META_PLANE_SIZE   (50 * 1024)
#define EXYNOS_CAMERA_VIDEO_META_PLANE_SIZE   sizeof(ExynosVideoMeta)
#define GRALLOC_LOCK_FOR_CAMERA         (GRALLOC_SET_USAGE_FOR_CAMERA)

#define EXYNOS_CAMERA_DEBUG_INFO_PLANE_SIZE   (100 * 1024)
#define META_PLANE_MASK          0x0000000F /* metadata plane */
#define META_PLANE_SHIFT         0
#define DEBUG_INFO_PLANE_MASK    0x000000F0 /* debugInfo plane */
#define DEBUG_INFO_PLANE_SHIFT   4

#define THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX "DBG^Stat"
#define THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX_SIZE sizeof(THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX)
#define THUMBNAIL_HISTOGRAM_STAT_PRAT1      (60 * 1024)
#define THUMBNAIL_HISTOGRAM_STAT_PRAT2      (40 * 1024)
/*******************************
 * Color Format Configurations *
 *******************************/
#ifndef DEBUG_RAWDUMP
#define CAMERA_PACKED_BAYER_ENABLE
#else
#undef USE_SBWC
#endif
/* #define CAMERA_PACKED_BAYER_ENABLE */


#ifdef USE_SBWC
#define SBWC_BLOCK_WIDTH (256)
#define SBWC_BLOCK_HEIGHT (1)
#define SBWC_MAX_HEIGHT  (2016) //actual limt 2048
#define SBWC_COMP_TYPE   (COMP_LOSS)
#endif
#define USE_BUFFER_WITH_STRIDE

#define V4L2_CAMERA_MEMORY_TYPE         (V4L2_MEMORY_DMABUF) /* (V4L2_MEMORY_USERPTR) */
#define PREVIEW_OUTPUT_COLOR_FMT        (V4L2_PIX_FMT_NV21M)
#define SCC_OUTPUT_COLOR_FMT            (V4L2_PIX_FMT_NV21)
#define JPEG_INPUT_COLOR_FMT            (SCC_OUTPUT_COLOR_FMT)
#define CAMERA_3AF_OUTPUT_FORMAT        (V4L2_PIX_FMT_BGR24)

#ifdef CAMERA_PACKED_BAYER_ENABLE
#define CAMERA_FLITE_BAYER_FORMAT               (V4L2_PIX_FMT_SBGGR10P)

#define CAMERA_3AC_BAYER_FORMAT                 (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AP_BAYER_FORMAT                 (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AC_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR10)
//#define CAMERA_3AP_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AP_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR12P)
#else
//#define CAMERA_FLITE_BAYER_FORMAT               (V4L2_PIX_FMT_SBGGR16)
#define CAMERA_FLITE_BAYER_FORMAT               (V4L2_PIX_FMT_SBGGR10)
#define CAMERA_3AC_BAYER_FORMAT                 (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AP_BAYER_FORMAT                 (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AC_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR10)
//#define CAMERA_3AP_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR12P)
#define CAMERA_3AP_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR12P)
#endif
#define CAMERA_3AG_REPROCESSING_BAYER_FORMAT    (V4L2_PIX_FMT_SBGGR10)

#define DEPTH_MAP_FORMAT                (V4L2_PIX_FMT_SBGGR16)
#define CAMERA_DUMP_BAYER_FORMAT        (V4L2_PIX_FMT_SBGGR16)

#define CAMERA_FLITE_BAYER_FORMAT_LED_CAL       (V4L2_PIX_FMT_SBGGR10)

/***********
 * Defines *
 ***********/
#define ERROR_POLLING_DETECTED          (-1001)
#define ERROR_DQ_BLOCKED_DETECTED       (-1002)
#define ERROR_DQ_BLOCKED_COUNT          (15)
#define ERROR_RESULT_DALAY_COUNT        (20)
#define MONITOR_THREAD_INTERVAL         (200000)

#define EXYNOS_CAMERA_PREVIEW_FPS_REFERENCE  (60)

#define  NUM_OF_DETECTED_FACES           (16)
#define  NUM_OF_DETECTED_FACES_THRESHOLD (0)

#define PERFRAME_NODE_GROUP_MAX          (7)

#define PERFRAME_INFO_INDEX_MAX          (7)
#define PERFRAME_INFO_INDEX_0            (0)
#define PERFRAME_INFO_INDEX_1            (1)
#define PERFRAME_INFO_INDEX_2            (2)
#define PERFRAME_INFO_INDEX_3            (3)
#define PERFRAME_INFO_INDEX_4            (4)
#define PERFRAME_INFO_INDEX_5            (5)
#define PERFRAME_INFO_INDEX_6            (6)

#define PERFRAME_CONTROL_PIPE                   PIPE_3AA
#define PERFRAME_CONTROL_REPROCESSING_PIPE      PIPE_3AA_REPROCESSING

#define PERFRAME_INFO_3AA                       PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_ISP                       PERFRAME_INFO_INDEX_1
#define PERFRAME_INFO_DCP                       PERFRAME_INFO_INDEX_2
#define PERFRAME_INFO_TPU                       PERFRAME_INFO_INDEX_2
#define PERFRAME_INFO_DIS                       PERFRAME_INFO_TPU
#define PERFRAME_INFO_MCSC                      PERFRAME_INFO_INDEX_3
#define PERFRAME_INFO_FLITE                     PERFRAME_INFO_INDEX_4
#define PERFRAME_INFO_VRA                       PERFRAME_INFO_INDEX_5
#define PERFRAME_INFO_CLAHE                     PERFRAME_INFO_INDEX_6

#define PERFRAME_INFO_SWMCSC                    PERFRAME_INFO_INDEX_2

#define PERFRAME_INFO_DIRTY_REPROCESSING_ISP    PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_DIRTY_REPROCESSING_DCP    PERFRAME_INFO_INDEX_1
#define PERFRAME_INFO_DIRTY_REPROCESSING_MCSC   PERFRAME_INFO_INDEX_2
#define PERFRAME_INFO_DIRTY_REPROCESSING_VRA    PERFRAME_INFO_INDEX_3

#define PERFRAME_INFO_PURE_REPROCESSING_3AA     PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_PURE_REPROCESSING_ISP     PERFRAME_INFO_INDEX_1
#define PERFRAME_INFO_PURE_REPROCESSING_MCSC    PERFRAME_INFO_INDEX_2
#define PERFRAME_INFO_PURE_REPROCESSING_VRA     PERFRAME_INFO_INDEX_3

#define PERFRAME_INFO_REPROCESSING_SWMCSC       PERFRAME_INFO_INDEX_4

#define PERFRAME_BACK_VC0_POS           (0)
#define PERFRAME_BACK_3AC_POS           (1)
#define PERFRAME_BACK_3AP_POS           (2)
#define PERFRAME_BACK_ISPC_POS          (3)
#define PERFRAME_BACK_ISPP_POS          (4)
#define PERFRAME_BACK_DCPS0_POS         (5)
#define PERFRAME_BACK_DCPS1_POS         (6)
#define PERFRAME_BACK_DCPC0_POS         (7)
#define PERFRAME_BACK_DCPC1_POS         (8)
#define PERFRAME_BACK_DCPC2_POS         (9)
#define PERFRAME_BACK_DCPC3_POS         (10)
#define PERFRAME_BACK_DCPC4_POS         (11)
#define PERFRAME_BACK_CIPC0_POS         (12)
#define PERFRAME_BACK_CIPC1_POS         (13)
#define PERFRAME_BACK_SCC_POS           (0)
#define PERFRAME_BACK_SCP_POS           (4)
#define PERFRAME_BACK_MCSC0_POS         PERFRAME_BACK_SCP_POS
#define PERFRAME_BACK_MCSC1_POS         (1)
#define PERFRAME_BACK_MCSC2_POS         (2)
#define PERFRAME_BACK_MCSC3_POS         (3)
#define PERFRAME_BACK_MCSC4_POS         (4)
#define PERFRAME_BACK_MCSC5_POS         (5)
#define PERFRAME_BACK_ME_POS            (6)
#define PERFRAME_BACK_MCSC_JPEG_POS  (PERFRAME_BACK_MCSC3_POS)
#define PERFRAME_BACK_MCSC_THUMB_POS (PERFRAME_BACK_MCSC4_POS)

#define PERFRAME_FRONT_VC0_POS          (PERFRAME_BACK_VC0_POS)
#define PERFRAME_FRONT_3AC_POS          (PERFRAME_BACK_3AC_POS)
#define PERFRAME_FRONT_3AP_POS          (PERFRAME_BACK_3AP_POS)
#define PERFRAME_FRONT_ISPC_POS         (PERFRAME_BACK_ISPC_POS)
#define PERFRAME_FRONT_ISPP_POS         (PERFRAME_BACK_ISPP_POS)
#define PERFRAME_FRONT_SCC_POS          (PERFRAME_BACK_SCC_POS)
#define PERFRAME_FRONT_SCP_POS          (PERFRAME_BACK_SCP_POS)
#define PERFRAME_FRONT_ME_POS           (PERFRAME_BACK_ME_POS)

#define PERFRAME_REPROCESSING_3AC_POS     (0)
#define PERFRAME_REPROCESSING_3AP_POS     (0)
#define PERFRAME_REPROCESSING_SCC_POS     (2)
#define PERFRAME_REPROCESSING_ISPC_POS    (2)
#define PERFRAME_REPROCESSING_ISPP_POS    (2)
#define PERFRAME_REPROCESSING_DCPS0_POS   (3)
#define PERFRAME_REPROCESSING_DCPC0_POS   (4)
#define PERFRAME_REPROCESSING_DCPS1_POS   (5)
#define PERFRAME_REPROCESSING_DCPC1_POS   (6)
#define PERFRAME_REPROCESSING_DCPC2_POS   (7)
#define PERFRAME_REPROCESSING_DCPC3_POS   (8)
#define PERFRAME_REPROCESSING_DCPC4_POS   (9)
#define PERFRAME_REPROCESSING_CIPS0_POS   (10)
#define PERFRAME_REPROCESSING_CIPS1_POS   (11)
#define PERFRAME_REPROCESSING_MCSC0_POS   (1)
#define PERFRAME_REPROCESSING_MCSC1_POS   (2)
#define PERFRAME_REPROCESSING_MCSC2_POS   (3)
#define PERFRAME_REPROCESSING_MCSC3_POS   (4)
#define PERFRAME_REPROCESSING_MCSC4_POS   (5)
#define PERFRAME_REPROCESSING_MCSC5_POS   (6)
#define PERFRAME_REPROCESSING_MCSC_JPEG_POS  (PERFRAME_REPROCESSING_MCSC3_POS)
#define PERFRAME_REPROCESSING_MCSC_THUMB_POS (PERFRAME_REPROCESSING_MCSC4_POS)

#define WAITING_TIME                     (5000)   /* 5msec */
#define TOTAL_WAITING_TIME               (3000 * 1000)  /* 3000msec */
#define TOTAL_WAITING_COUNT              (3)
#define TOTAL_FLASH_WATING_COUNT         (10)
#define CAPTURE_WAITING_COUNT            (15)

#define DM_WAITING_TIME                  (30 * 1000) /* 30msec */
#define DM_WAITING_COUNT                 (10)

/*vision */
/* #define VISION_DUMP */
#define VISION_WIDTH                     (320)
#define VISION_HEIGHT                    (180)

/* Secure Camera */
#define SECURE_CAMERA_WIDTH              (2400)
#define SECURE_CAMERA_HEIGHT             (2400)

/* MCSC Restriction*/
#define NO_MCSC_RESTRICTION

#define USE_MEM2MEM_GSC
#ifdef USE_MEM2MEM_GSC
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP | \
     GRALLOC_USAGE_HW_CAMERA_MASK)
#else
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP)
#endif

enum YUV_RANGE {
    YUV_FULL_RANGE = 0,
    YUV_LIMITED_RANGE = 1,
};

enum pipeline {
    PIPE_FLITE                      = 0,
    PIPE_VC0,
    PIPE_VC1,
    PIPE_VC2,
    PIPE_VC3,
#ifdef USE_PAF
    PIPE_PAF,
#endif
    PIPE_3AA,
    PIPE_3AC,
    PIPE_3AP,
    PIPE_3AF,
    PIPE_3AG,
    PIPE_ISP,                       // 10
    PIPE_ISPC,
    PIPE_ISPP,
    PIPE_ME,
    PIPE_DIS,
    PIPE_TPU = PIPE_DIS,
    PIPE_TPUP,
    PIPE_MCSC,                      // 25
    PIPE_SCP,
    PIPE_MCSC0 = PIPE_SCP,
    PIPE_MCSC1,
    PIPE_MCSC2,
    PIPE_MCSC3,                     // 29
    PIPE_MCSC4,                     // 30
    PIPE_MCSC_JPEG = PIPE_MCSC3,    // 29
    PIPE_MCSC_THUMB = PIPE_MCSC4,   // 30
    PIPE_MCSC5,
    PIPE_VRA,                       // 32
    PIPE_HFD,
    PIPE_GMV,
    PIPE_3AA_ISP,
    PIPE_POST_3AA_ISP,
    PIPE_SCC,
    PIPE_GDC,
    PIPE_GSC,
    PIPE_GSC_VIDEO,                 // 40
    PIPE_GSC_PICTURE,
    PIPE_JPEG,
    PIPE_HWFC_JPEG_SRC,
    PIPE_HWFC_THUMB_SRC,
    PIPE_HWFC_JPEG_DST,             // 45
    PIPE_HWFC_THUMB_DST,
    PIPE_BAYER_SYNC,
    PIPE_SYNC,
    PIPE_FUSION,
    PIPE_FUSION_FRONT,
    PIPE_VDIS,
    PIPE_VDIS_PREVIEW,
#ifdef USE_SLSI_PLUGIN
    PIPE_PLUGIN_BASE,
    PIPE_PLUGIN1 = PIPE_PLUGIN_BASE,
    PIPE_PLUGIN2,
    PIPE_PLUGIN3,
    PIPE_PLUGIN4,
    PIPE_PLUGIN_MAX = PIPE_PLUGIN4,
#endif
    PIPE_NFD,
#ifdef USE_CLAHE_PREVIEW
    PIPE_CLAHE,
    PIPE_CLAHEC,
#endif
    MAX_PIPE_NUM,

    /*
     * PIPE_XXX_FRONT are deprecated define.
     * Don't use this. (just let for common code compile)
     */
    PIPE_FLITE_FRONT = 100,
    PIPE_3AA_FRONT,
    PIPE_3AC_FRONT,
    PIPE_3AP_FRONT,
    PIPE_ISP_FRONT,
    PIPE_ISPC_FRONT,
    PIPE_ISPP_FRONT,
    PIPE_SCP_FRONT,
    PIPE_3AA_ISP_FRONT,
    PIPE_POST_3AA_ISP_FRONT,
    PIPE_DIS_FRONT,
    PIPE_SCC_FRONT,
    PIPE_GSC_FRONT,
    PIPE_GSC_VIDEO_FRONT,
    PIPE_GSC_PICTURE_FRONT,
    PIPE_JPEG_FRONT,
    MAX_PIPE_NUM_FRONT,

    PIPE_FLITE_REPROCESSING     = 200,
    PIPE_VC0_REPROCESSING,
#ifdef USE_PAF
    PIPE_PAF_REPROCESSING,
#endif
    PIPE_3AA_REPROCESSING,
    PIPE_3AC_REPROCESSING,
    PIPE_3AP_REPROCESSING,
    PIPE_3AF_REPROCESSING,
    PIPE_3AG_REPROCESSING,
    PIPE_ISP_REPROCESSING,
    PIPE_ISPC_REPROCESSING,
    PIPE_ISPP_REPROCESSING,
    PIPE_TPU_REPROCESSING,
    PIPE_TPUP_REPROCESSING,
    PIPE_MCSC_REPROCESSING,
    PIPE_MCSC0_REPROCESSING,
    PIPE_MCSC1_REPROCESSING,
    PIPE_MCSC2_REPROCESSING,
    PIPE_MCSC3_REPROCESSING,
    PIPE_MCSC_JPEG_REPROCESSING = PIPE_MCSC3_REPROCESSING,
    PIPE_MCSC4_REPROCESSING,
    PIPE_MCSC_THUMB_REPROCESSING = PIPE_MCSC4_REPROCESSING,
    PIPE_MCSC5_REPROCESSING,
    PIPE_MCSC_PP_REPROCESSING,
    PIPE_VRA_REPROCESSING,
    PIPE_SCC_REPROCESSING,
    PIPE_SCP_REPROCESSING,
    PIPE_GSC_REPROCESSING,
    PIPE_GSC_REPROCESSING2,
    PIPE_GSC_REPROCESSING3,
#ifdef UVS
    PIPE_UVS_REPROCESSING,
#endif
    PIPE_JPEG_REPROCESSING,
    PIPE_JPEG0_REPROCESSING,
    PIPE_JPEG1_REPROCESSING,
    MAX_PIPE_NUM_JPEG_DST_REPROCESSING = PIPE_JPEG1_REPROCESSING,
    PIPE_HWFC_JPEG_SRC_REPROCESSING,
    PIPE_HWFC_THUMB_SRC_REPROCESSING,
    PIPE_HWFC_JPEG_DST_REPROCESSING,
    PIPE_HWFC_THUMB_DST_REPROCESSING,
    PIPE_SYNC_REPROCESSING,
    PIPE_FUSION_REPROCESSING,
    PIPE_FUSION0_REPROCESSING,
    PIPE_FUSION1_REPROCESSING,
    PIPE_FUSION2_REPROCESSING,
    PIPE_LLS_REPROCESSING,
    PIPE_REMOSAIC_REPROCESSING,
#ifdef USE_SLSI_PLUGIN
    PIPE_PLUGIN_BASE_REPROCESSING,
    PIPE_PLUGIN_PRE1_REPROCESSING = PIPE_PLUGIN_BASE_REPROCESSING,
    PIPE_PLUGIN_PRE2_REPROCESSING,
    PIPE_PLUGIN_POST1_REPROCESSING,
    PIPE_PLUGIN_POST2_REPROCESSING,
    PIPE_PLUGIN_MAX_REPROCESSING = PIPE_PLUGIN_POST2_REPROCESSING,
#endif
    PIPE_NFD_REPROCESSING,
#ifdef USE_SW_MCSC_REPROCESSING
    PIPE_SW_MCSC_REPEOCESSING,
#endif
#ifdef USE_CLAHE_REPROCESSING
    PIPE_CLAHE_REPROCESSING,
    PIPE_CLAHEC_REPROCESSING,
#endif
    MAX_PIPE_NUM_REPROCESSING,

    PIPE_CLONE_START = 1000,
    PIPE_GSC_CLONE =     PIPE_CLONE_START + PIPE_GSC,
    PIPE_FUSION_CLONE =  PIPE_CLONE_START + PIPE_FUSION,
    PIPE_PLUGIN_CALLBACK  = PIPE_CLONE_START + PIPE_PLUGIN1,
    PIPE_PLUGIN_RECORDING = PIPE_PLUGIN_CALLBACK + 1,
    PIPE_PLUGIN_PREVIEW   = PIPE_PLUGIN_RECORDING + 1,
    MAX_PIPE_NUM_CLONE = PIPE_CLONE_START + MAX_PIPE_NUM,

#ifdef DEBUG_DUMP_IMAGE
    PIPE_DUMP_IMAGE,
#endif
};

enum fimc_is_video_dev_num {
    FIMC_IS_VIDEO_BAS_NUM = 100,
    FIMC_IS_VIDEO_SS0_NUM = 101,
    FIMC_IS_VIDEO_SS1_NUM,
    FIMC_IS_VIDEO_SS2_NUM,
    FIMC_IS_VIDEO_SS3_NUM,
    FIMC_IS_VIDEO_SS4_NUM,
    FIMC_IS_VIDEO_SS5_NUM,
    FIMC_IS_VIDEO_SS6_NUM = FIMC_IS_VIDEO_SS5_NUM,   /* TODO: define proper node */
    FIMC_IS_VIDEO_SS7_NUM = FIMC_IS_VIDEO_SS5_NUM,   /* TODO: define proper node */
    FIMC_IS_VIDEO_BNS_NUM = 107,
    FIMC_IS_VIDEO_PRE_NUM = 109,
    FIMC_IS_VIDEO_30S_NUM = 110,
    FIMC_IS_VIDEO_30C_NUM,
    FIMC_IS_VIDEO_30P_NUM,
    FIMC_IS_VIDEO_30F_NUM,
    FIMC_IS_VIDEO_30G_NUM,
    FIMC_IS_VIDEO_31S_NUM = 120,
    FIMC_IS_VIDEO_31C_NUM,
    FIMC_IS_VIDEO_31P_NUM,
    FIMC_IS_VIDEO_31F_NUM,
    FIMC_IS_VIDEO_31G_NUM,
    FIMC_IS_VIDEO_I0S_NUM = 130,
    FIMC_IS_VIDEO_I0C_NUM,
    FIMC_IS_VIDEO_I0P_NUM,
    FIMC_IS_VIDEO_I1S_NUM = 140,
    FIMC_IS_VIDEO_I1C_NUM,
    FIMC_IS_VIDEO_I1P_NUM,
    FIMC_IS_VIDEO_ME0C_NUM = 148,
    FIMC_IS_VIDEO_ME1C_NUM = 149,
    FIMC_IS_VIDEO_DCP0S_NUM = 150,
    FIMC_IS_VIDEO_DCP0C_NUM,
    FIMC_IS_VIDEO_DCP1S_NUM,
    FIMC_IS_VIDEO_DCP1C_NUM,
    FIMC_IS_VIDEO_DCP2C_NUM,
    FIMC_IS_VIDEO_GDC_NUM = 155,
    FIMC_IS_VIDEO_DCP3C_NUM,
    FIMC_IS_VIDEO_DCP4C_NUM,
    FIMC_IS_VIDEO_SCC_NUM,
    FIMC_IS_VIDEO_SCP_NUM,
    FIMC_IS_VIDEO_M0S_NUM = 160,
    FIMC_IS_VIDEO_M1S_NUM,
    FIMC_IS_VIDEO_M0P_NUM = 170,
    FIMC_IS_VIDEO_M1P_NUM,
    FIMC_IS_VIDEO_M2P_NUM,
    FIMC_IS_VIDEO_M3P_NUM,
    FIMC_IS_VIDEO_M4P_NUM,
    FIMC_IS_VIDEO_M5P_NUM,
    FIMC_IS_VIDEO_VRA_NUM = 180,
    FIMC_IS_VIDEO_D0S_NUM = 190,
    FIMC_IS_VIDEO_D0C_NUM,
    FIMC_IS_VIDEO_D1S_NUM,
    FIMC_IS_VIDEO_D1C_NUM,
    FIMC_IS_VIDEO_CLH0S_NUM,
    FIMC_IS_VIDEO_CLH0C_NUM,
    FIMC_IS_VIDEO_HWFC_JPEG_NUM = 200,
    FIMC_IS_VIDEO_HWFC_THUMB_NUM = 201,
    FIMC_IS_VIDEO_CIP0S_NUM = 205,
    FIMC_IS_VIDEO_CIP1S_NUM = 206,
    FIMC_IS_VIDEO_SS0VC0_NUM = 210,
    FIMC_IS_VIDEO_SS0VC1_NUM,
    FIMC_IS_VIDEO_SS0VC2_NUM,
    FIMC_IS_VIDEO_SS0VC3_NUM,
    FIMC_IS_VIDEO_SS1VC0_NUM,
    FIMC_IS_VIDEO_SS1VC1_NUM,
    FIMC_IS_VIDEO_SS1VC2_NUM,
    FIMC_IS_VIDEO_SS1VC3_NUM,
    FIMC_IS_VIDEO_SS2VC0_NUM,
    FIMC_IS_VIDEO_SS2VC1_NUM,
    FIMC_IS_VIDEO_SS2VC2_NUM = 220,
    FIMC_IS_VIDEO_SS2VC3_NUM,
    FIMC_IS_VIDEO_SS3VC0_NUM,
    FIMC_IS_VIDEO_SS3VC1_NUM,
    FIMC_IS_VIDEO_SS3VC2_NUM,
    FIMC_IS_VIDEO_SS3VC3_NUM = 225,
    FIMC_IS_VIDEO_SS4VC0_NUM,
    FIMC_IS_VIDEO_SS4VC1_NUM,
    FIMC_IS_VIDEO_SS4VC2_NUM,
    FIMC_IS_VIDEO_SS4VC3_NUM,
    FIMC_IS_VIDEO_SS5VC0_NUM,
    FIMC_IS_VIDEO_SS5VC1_NUM,
    FIMC_IS_VIDEO_SS5VC2_NUM,
    FIMC_IS_VIDEO_SS5VC3_NUM,
    FIMC_IS_VIDEO_SS6VC1_NUM,  /* TODO: define proper node */
    FIMC_IS_VIDEO_SS7VC1_NUM,  /* TODO: define proper node */
#ifdef USE_PAF
    FIMC_IS_VIDEO_PAF0S_NUM = 240,
    FIMC_IS_VIDEO_PAF1S_NUM = 241,
#endif
    FIMC_IS_VIDEO_MAX_NUM
};

enum fimc_is_virtual_video_dev_num {
    FIMC_IS_VIRTUAL_VIDEO_BASE = FIMC_IS_VIDEO_MAX_NUM,
    FIMC_IS_VIRTUAL_VIDEO_PP,
    FIMC_IS_VIRTUAL_VIDEO_MAX,
};

#define VIRTUAL_VIDEO_COUNT     (FIMC_IS_VIRTUAL_VIDEO_MAX - FIMC_IS_VIRTUAL_VIDEO_BASE - 1)
#define VIRTUAL_MCSC_MAX        (MCSC_PORT_MAX + VIRTUAL_VIDEO_COUNT + 1)
#define VIRTUAL_MCSC_PORT_6     (MCSC_PORT_MAX + VIRTUAL_VIDEO_COUNT)

/* overflow handling */
//#define SENSOR_OVERFLOW_CHECK
#define AVOID_ASSERT_FRAME
#define AVOID_ASSERT_WAIT_STATE_FLUSH

#ifdef SUPPORT_ME
#define LEADER_PIPE_OF_ME PIPE_3AA
#endif //SUPPORT_ME

/* ########################################### */
/* configuration for plugin library version    */

#define PLUGIN_HIFILLS_VERSION     "MFSTILL ver(1.1)"
/* ########################################### */

#endif /*USE_VENDOR_SPECIFIC_CONFIG_HEADER*/

#endif /* EXYNOS_CAMERA_CONFIG_H__ */
