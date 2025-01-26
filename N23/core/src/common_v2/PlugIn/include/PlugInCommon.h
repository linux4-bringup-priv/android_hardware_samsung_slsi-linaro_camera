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
#ifndef PLUG_IN_COMMON_H
#define PLUG_IN_COMMON_H

#include <map>

#include "PlugInCommonExt.h"

enum VendorID{
    SLSI = 1,
    ARCSOFT = 2,
};

enum Type {
    TYPE_PREVIEW = 1,
    TYPE_REPROCESSING,
};

enum Handle {
    LIB_FAKEFUSION = 1,
    LIB_ARCSOFTFUSION,
    LIB_LOW_LIGHTSHOT,
    LIB_VDIS,
    LIB_HIFI_LLS,
    LIB_HIFI, /* for yuv reprocessing: denoise and edge */
    LIB_COMBINE_PREVIEW,
    LIB_COMBINE_REPROCESSING,
};

enum ScenarioID {
    FAKE_FUSION =1,
    DUAL_ZOOM_FUSION,
    DUAL_BOKEH_FUSION,
    BAYER_LLS,
    SW_VDIS,
    HIFI_LLS,
    HIFI, /* for yuv reprocessing: denoise and edge */
    COMBINE,
    COMBINE_FUSION,
};

/* */
#define PLUGIN_VENDORID_OFFSET   (24)
#define PLUGIN_VENDORID_MASK     (0XFF000000)
#define PLUGIN_TYPE_OFFSET       (20)
#define PLUGIN_TYPE_MASK         (0X00F00000)
#define PLUGIN_HANDLE_OFFSET     (12)
#define PLUGIN_HANDLE_MASK       (0x000FF000)
#define PLUGIN_SCENARIO_OFFSET   ( 0)
#define PLUGIN_SCENARIO_MASK     (0x00000FFF)

#define MAKE_LIBRARY_ID(VENDORID, TYPE, HANDLE, SCENARIOID) (VENDORID<<24 | TYPE<<20 | HANDLE<<12 | SCENARIOID)

enum PlugInScenario {
    PLUGIN_SCENARIO_FAKEFUSION_PREVIEW          = MAKE_LIBRARY_ID(SLSI,     TYPE_PREVIEW,      LIB_FAKEFUSION,              FAKE_FUSION), /* for fake fusion for dual camera. */
    PLUGIN_SCENARIO_FAKEFUSION_REPROCESSING     = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_FAKEFUSION,              FAKE_FUSION), /* for fake fusion for dual camera. */
    PLUGIN_SCENARIO_ZOOMFUSION_PREVIEW          = MAKE_LIBRARY_ID(ARCSOFT,  TYPE_PREVIEW,      LIB_ARCSOFTFUSION,           DUAL_ZOOM_FUSION), /* for arcsoft zoom fusion for dual camera */
    PLUGIN_SCENARIO_ZOOMFUSION_REPROCESSING     = MAKE_LIBRARY_ID(ARCSOFT,  TYPE_PREVIEW,      LIB_ARCSOFTFUSION,           DUAL_BOKEH_FUSION), /* for arcsoft zoom fusion for dual camera */
    PLUGIN_SCENARIO_BOKEHFUSION_PREVIEW         = MAKE_LIBRARY_ID(ARCSOFT,  TYPE_REPROCESSING, LIB_ARCSOFTFUSION,           DUAL_ZOOM_FUSION), /* for arcsoft zoom fusion for dual camera */
    PLUGIN_SCENARIO_BOKEHFUSION_REPROCESSING    = MAKE_LIBRARY_ID(ARCSOFT,  TYPE_REPROCESSING, LIB_ARCSOFTFUSION,           DUAL_BOKEH_FUSION), /* for arcsoft zoom fusion for dual camera */
    PLUGIN_SCENARIO_BAYERLLS_REPROCESSING       = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_LOW_LIGHTSHOT,           BAYER_LLS), /* for LSI Bayer LLS solution */
    PLUGIN_SCENARIO_SWVDIS_PREVIEW              = MAKE_LIBRARY_ID(SLSI,     TYPE_PREVIEW,      LIB_VDIS,                    SW_VDIS), /* for LSI SW VDIS solution */
    PLUGIN_SCENARIO_HIFILLS_REPROCESSING        = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_HIFI_LLS,                HIFI_LLS), /* for LSI HIFI LLS solution */
    PLUGIN_SCENARIO_HIFI_REPROCESSING           = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_HIFI,                    HIFI), /* for LSI HIFI(denoise and edge) solution */
    PLUGIN_SCENARIO_COMBINE_PREVIEW             = MAKE_LIBRARY_ID(SLSI,     TYPE_PREVIEW,      LIB_COMBINE_PREVIEW,         COMBINE), /* for combined preview plugin solution (sceneDetect, faceBeauty) */
    PLUGIN_SCENARIO_COMBINE_REPROCESSING        = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_COMBINE_REPROCESSING,    COMBINE), /* for COMBINE solution */
    PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING  = MAKE_LIBRARY_ID(SLSI,     TYPE_REPROCESSING, LIB_COMBINE_REPROCESSING,    COMBINE_FUSION), /* for COMBINE fusion solution */
};

/*
   VendorID : algorithm vendor
   Type     : algorithm type for preview / reprocessing
   Handle   : library handle managing infomation
   Scenario : algorithm functionality(LLS/VDIS/etc)
*/

#define getLibVendorID(info) ((info & PLUGIN_VENDORID_MASK) >> PLUGIN_VENDORID_OFFSET)
#define getLibType(info)     ((info & PLUGIN_TYPE_MASK) >> PLUGIN_TYPE_OFFSET)
#define getLibHandleID(info) ((info & PLUGIN_HANDLE_MASK) >> PLUGIN_HANDLE_OFFSET)
#define getLibScenario(info) ((info & PLUGIN_SCENARIO_MASK) >> PLUGIN_SCENARIO_OFFSET)

/*
 * define
 */
#define PLUGIN_MAX_BUF 32
#define PLUGIN_MAX_PLANE 8

/* To access Array_buf_rect_t */
#define PLUGIN_ARRAY_RECT_X 0
#define PLUGIN_ARRAY_RECT_Y 1
#define PLUGIN_ARRAY_RECT_W 2
#define PLUGIN_ARRAY_RECT_H 3
#define PLUGIN_ARRAY_RECT_FULL_W 4
#define PLUGIN_ARRAY_RECT_FULL_H 5
#define PLUGIN_ARRAY_RECT_MAX 6

/*
 * typedef
 */
typedef int64_t              Map_data_t;
typedef std::map<int, Map_data_t> Map_t;

typedef bool                 Data_bool_t;
typedef char                 Data_char_t;
typedef int32_t              Data_int32_t;
typedef uint32_t             Data_uint32_t;
typedef int64_t              Data_int64_t;
typedef uint64_t             Data_uint64_t;
typedef float                Data_float_t;

typedef float *              Pointer_float_t;
typedef double *             Pointer_double_t;
typedef const char *         Pointer_const_char_t;
typedef char *               Pointer_char_t;
typedef unsigned char *      Pointer_uchar_t;
typedef int *                Pointer_int_t;
typedef void *               Pointer_void_t;

typedef char *               Single_buf_t;
typedef int *                Array_buf_t;
typedef int *                Array_plane_t;
typedef int (*               Array_buf_plane_t)[PLUGIN_MAX_PLANE];
typedef int (*               Array_buf_rect_t)[PLUGIN_ARRAY_RECT_MAX];
typedef char **              Array_buf_addr_t;
typedef float **             Array_float_addr_t;
typedef int **               Array_pointer_int_t;

/*
 * macro
 */
#define MAKE_VERSION(major, minor) (((major) << 16) | (minor))
#define GET_MAJOR_VERSION(version) ((version) >> 16)
#define GET_MINOR_VERSION(version) ((version) & 0xFFFF)
#define PLUGIN_LOCATION_ID         "(%s[%d]):"
#define PLUGIN_LOCATION_ID_PARM    __FUNCTION__, __LINE__
#define PLUGIN_LOG_ASSERT(fmt, ...) \
        android_printAssert(NULL, LOG_TAG, "ASSERT" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__);
#define PLUGIN_LOGD(fmt, ...) \
        ALOGD("DEBUG" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__)
#define PLUGIN_LOGV(fmt, ...) \
        ALOGV("VERB" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__)
#define PLUGIN_LOGW(fmt, ...) \
        ALOGW("WARN" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__)
#define PLUGIN_LOGE(fmt, ...) \
        ALOGE("ERR" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__)
#define PLUGIN_LOGI(fmt, ...) \
        ALOGI("INFO" PLUGIN_LOCATION_ID fmt, PLUGIN_LOCATION_ID_PARM, ##__VA_ARGS__)

/*
 * Default KEY (1 ~ 1000)
 * PIPE -> Converter
 */
#define PLUGIN_CONVERT_TYPE            1    /* int                     / int                     */
enum PLUGIN_CONVERT_TYPE_T {
    PLUGIN_CONVERT_BASE,
    PLUGIN_CONVERT_SETUP_BEFORE,
    PLUGIN_CONVERT_SETUP_AFTER,
    PLUGIN_CONVERT_PROCESS_BEFORE,
    PLUGIN_CONVERT_PROCESS_AFTER,
    PLUGIN_CONVERT_MAX,
};
#define PLUGIN_CONVERT_FRAME        2   /* ExynosCameraFrame     / ExynosCameraFrame     */
#define PLUGIN_CONVERT_PARAMETER    3   /* ExynosCameraParamerer / ExynosCameraParamerer */
#define PLUGIN_CONVERT_CONFIGURATIONS    4   /* ExynosCameraConfigurations / ExynosCameraConfigurations */
#define PLUGIN_CONVERT_BUFFERSUPPLIER    5   /* ExynosCameraBufferSupplier / ExynosCameraBufferSupplier */

/*
 * Source Buffer Info (1001 ~ 2000)            Map Data Type    / Actual Type
 */
#define PLUGIN_SRC_BUF_CNT          1001    /* Data_int32_t     / int            */
#define PLUGIN_SRC_BUF_PLANE_CNT    1002    /* Array_plane_t    / int[PLUGIN_MAX_BUF]    */
#define PLUGIN_SRC_BUF_SIZE         1003    /* Array_buf_plane_t/ int[PLUGIN_MAX_BUF][PLUGIN_MAX_PLANE]    */
#define PLUGIN_SRC_BUF_RECT         1004    /* Array_buf_rect_t / int[PLUGIN_MAX_BUF][4] */
#define PLUGIN_SRC_BUF_WSTRIDE      1005    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_HSTRIDE      1006    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_V4L2_FORMAT  1007    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_HAL_FORMAT   1008    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_ROTATION     1009    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_FLIPH        1010    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_BUF_FLIPV        1011    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_SRC_FRAMECOUNT       1012    /* Data_int32_t     / int                 */
#define PLUGIN_SRC_FRAME_STATE      1013    /* Data_int32_t     / int                 */
#define PLUGIN_SRC_FRAME_TYPE       1014    /* Data_int32_t     / int                 */
#define PLUGIN_SRC_BUF_INDEX        1015    /* Data_int32_t     / int                 */
#define PLUGIN_SRC_BUF_FD           1016    /* Array_buf_plane_t/ int[PLUGIN_MAX_BUF][PLUGIN_MAX_PLANE]    */
#define PLUGIN_SRC_BUF_USED         1017    /* Data_int32_t     / int                 */
#define PLUGIN_MASTER_FACE_RECT     1018    /* Array_buf_rect_t / int[PLUGIN_MAX_BUF][4] */
#define PLUGIN_SLAVE_FACE_RECT      1019    /* Array_buf_rect_t / int[PLUGIN_MAX_BUF][4] */
#define PLUGIN_RESULT_FACE_RECT     1020    /* Array_buf_rect_t / int[PLUGIN_MAX_BUF][4] */
#define PLUGIN_FACE_NUM             1021    /* Data_int32_t     / int                 */

/*
 * Source Buffer Address (2001 ~ 3000)
 */
#define PLUGIN_SRC_BUF_1        2001    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_2        2002    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_3        2003    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_4        2004    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_5        2005    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_6        2006    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_7        2007    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_8        2008    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_9        2009    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_10       2010    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_11       2011    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_12       2012    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_13       2013    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_14       2014    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_15       2015    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_16       2016    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_17       2017    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_18       2018    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_19       2019    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_20       2020    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_21       2021    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_22       2022    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_23       2023    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_24       2024    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_25       2025    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_26       2026    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_27       2027    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_28       2028    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_29       2029    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_30       2030    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_31       2031    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_SRC_BUF_32       2032    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */

/*
 * Destination Buffer Address (3001 ~ 4000)
 */
#define PLUGIN_DST_BUF_CNT          3001    /* Data_int32_t     / int            */
#define PLUGIN_DST_BUF_PLANE_CNT    3002    /* Array_plane_t    / int[PLUGIN_MAX_BUF]    */
#define PLUGIN_DST_BUF_SIZE         3003    /* Array_buf_plane_t/ int[PLUGIN_MAX_BUF][PLUGIN_MAX_PLANE]    */
#define PLUGIN_DST_BUF_RECT         3004    /* Array_buf_rect_t / int[PLUGIN_MAX_BUF][4] */
#define PLUGIN_DST_BUF_WSTRIDE      3005    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_HSTRIDE      3006    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_V4L2_FORMAT  3007    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_HAL_FORMAT   3008    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_ROTATION     3009    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_FLIPH        3010    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_FLIPV        3011    /* Array_buf_t      / int[PLUGIN_MAX_BUF] */
#define PLUGIN_DST_BUF_INDEX        3012    /* Data_int32_t     / int            */
#define PLUGIN_DST_BUF_FD           3013    /* Array_buf_plane_t/ int[PLUGIN_MAX_BUF][PLUGIN_MAX_PLANE]    */
#define PLUGIN_DST_BUF_VALID        3014    /* Data_int32_t     / int                 */

/*
 * Destination Buffer Address (4001 ~ 5000)
 */
#define PLUGIN_DST_BUF_1        4001    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_2        4002    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_3        4003    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_4        4004    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_5        4005    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_6        4006    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_7        4007    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_8        4008    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_9        4009    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_10       4010    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_11       4011    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_12       4012    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_13       4013    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_14       4014    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_15       4015    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_16       4016    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_17       4017    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_18       4018    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_19       4019    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_20       4020    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_21       4021    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_22       4022    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_23       4023    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_24       4024    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_25       4025    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_26       4026    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_27       4027    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_28       4028    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_29       4029    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_30       4030    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_31       4031    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_DST_BUF_32       4032    /* Array_buf_addr_t / char *[PLUGIN_MAX_PLANE] */

/*
 * ETC Info (5001 ~ 0xF0000000)
 */
#define PLUGIN_CONFIG_FILE_PATH             5001    /* Pointer_const_char_t/ const char * */
#define PLUGIN_CALB_FILE_PATH               5002    /* Pointer_const_char_t/ const char * */
#define PLUGIN_ZOOM_RATIO_WIDE              5003    /* Pointer_float_t     / float *      */
#define PLUGIN_ZOOM_RATIO_TELE              5004    /* Pointer_float_t     / float *      */
#define PLUGIN_LENS_POSITION_WIDE           5005    /* Data_int32_t        / int          */
#define PLUGIN_LENS_POSITION_TELE           5006    /* Data_int32_t        / int          */
#define PLUGIN_GLOBAL_DISP_X                5007    /* Data_int32_t        / int          */
#define PLUGIN_GLOBAL_DISP_Y                5008    /* Data_int32_t        / int          */
#define PLUGIN_FUSION_ENABLE                5009    /* Data_bool_t         / bool        */
#define PLUGIN_ROI2D                        5010    /* Not defined         / Not defined  */
#define PLUGIN_RECTIFICATION_RIGHT_WRAP     5011    /* Data_bool_t         / bool         */
#define PLUGIN_RECTIFICATION_DISP_MARGIN    5012    /* Data_int32_t        / int          */
#define PLUGIN_RECTIFICATION_IMG_SHIFT      5013    /* Data_int32_t        / int          */
#define PLUGIN_RECTIFICATION_MANUAL_CAL_Y   5014    /* Data_int32_t        / int          */
#define PLUGIN_SENSOR_ID                    5015    /* Data_int32_t        / int          */

/* LLS Lib Start */
#define PLUGIN_LLS_INTENT                   5100    /* Data_int32_t        / int          */
enum PLUGIN_LLS_INTENT_ENUM {
    NONE,
    CAPTURE_START,      /* capture first frame */
    CAPTURE_PROCESS,    /* capture not first frame */
    PREVIEW_START,      /* preview first frame */
    PREVIEW_PROCESS,    /* preview not first frame */
};
#define PLUGIN_ME_DOWNSCALED_BUF            5101    /* Single_Buf_addr_t   / char *       */
#define PLUGIN_MOTION_VECTOR_BUF            5102    /* Single_Buf_addr_t   / char *       */
#define PLUGIN_CURRENT_PATCH_BUF            5103    /* Single_Buf_addr_t   / char *       */
#define PLUGIN_ME_FRAMECOUNT                5104    /* Data_int32_t        / int          */
#define PLUGIN_TIMESTAMP                    5105    /* Data_int64_t        / int64        */
#define PLUGIN_EXPOSURE_TIME                5106    /* Data_int64_t        / int64        */
#define PLUGIN_ISO                          5107    /* Data_int32_t        / int          */
#define PLUGIN_FPS                          5120    /* Data_int32_t        / int          */

#ifdef USE_PLUGIN_LLS
#define PLUGIN_MAX_BCROP_X                  5108    /* Data_int32_t        / int          */
#define PLUGIN_MAX_BCROP_Y                  5119    /* Data_int32_t        / int          */
#define PLUGIN_MAX_BCROP_W                  5110    /* Data_int32_t        / int          */
#define PLUGIN_MAX_BCROP_H                  5111    /* Data_int32_t        / int          */
#define PLUGIN_MAX_BCROP_FULLW              5112    /* Data_int32_t        / int          */
#define PLUGIN_MAX_BCROP_FULLH              5113    /* Data_int32_t        / int          */
#define PLUGIN_BAYER_V4L2_FORMAT            5114    /* Data_int32_t        / int          */
/* LLS Lib End */
#endif

#define PLUGIN_MAX_SENSOR_W                 5108    /* Data_int32_t        / int          */
#define PLUGIN_MAX_SENSOR_H                 5109    /* Data_int32_t        / int          */
#define PLUGIN_TIMESTAMPBOOT                5110    /* Data_int32_t        / int          */
#define PLUGIN_FRAME_DURATION               5111    /* Data_int32_t        / int          */
#define PLUGIN_ROLLING_SHUTTER_SKEW         5112    /* Data_int32_t        / int          */
#define PLUGIN_OPTICAL_STABILIZATION_MODE_CTRL   5113    /* Data_int32_t        / int          */
#define PLUGIN_OPTICAL_STABILIZATION_MODE_DM  5114    /* Data_int32_t        / int          */
#define PLUGIN_GYRO_DATA                    5115    /*Array_pointer_gyro_data_t  /plugin_gyro_data_t*    */
#define PLUGIN_GYRO_DATA_SIZE                    5116    /* Data_int32_t        / int          */
#define PLUGIN_ISO_LIST                     5117    /* Array_buf_t         / int          */
#define PLUGIN_GYRO_FLOAT_ARRAY             5118    /* Pointer_float_t      / float *     */
#define PLUGIN_EIS_PREVIEW_CROP             5119    /* Data_pointer_rect_t  / struct plugin_rect_t    */

#define PLUGIN_NOISE_CONTROL_MODE           5130    /* Data_int32_t         / int          */
#define PLUGIN_EDGE_CONTROL_MODE            5131    /* Data_int32_t         / int          */
#define PLUGIN_NOISE_CONTROL_STRENGTH       5132    /* Data_int32_t         / int          */
#define PLUGIN_EDGE_CONTROL_STRENGTH        5133    /* Data_int32_t         / int          */

#define PLUGIN_ANALOG_GAIN                  5134    /* Data_uint32_t        / unsigned int  */
#define PLUGIN_DIGITAL_GAIN                 5135    /* Data_uint32_t        / unsigned int  */
#define PLUGIN_FLASH_MODE                   5136    /* Data_int32_t         / int           */
#define PLUGIN_JPEG_ORIENTATION             5137    /* Data_uint32_t        / unsigned int        */
#define PLUGIN_AE_REGION                    5138    /* Pointer_int_t        / int *         */

#define PLUGIN_SCENARIO                     5139    /* Data_int32_t         / int           */
#define PLUGIN_AEC_SETTLED                  5140    /* Data_int32_t         / int           */

#define PLUGIN_HIFI_TOTAL_BUFFER_NUM        5200
#define PLUGIN_HIFI_CUR_BUFFER_NUM          5250
#define PLUGIN_HIFI_OPERATION_MODE          5201
#define PLUGIN_HIFI_MAX_SENSOR_WIDTH        5202
#define PLUGIN_HIFI_MAX_SENSOR_HEIGHT       5203
#define PLUGIN_HIFI_CROP_FLAG               5204
#define PLUGIN_HIFI_CAMERA_TYPE             5205
#define PLUGIN_HIFI_SENSOR_TYPE             5206
#define PLUGIN_HIFI_HDR_MODE                5207
#define PLUGIN_HIFI_ZOOM                    5208
#define PLUGIN_HIFI_EXPOSURE_TIME           5209
#define PLUGIN_HIFI_EXPOSURE_VALUE          5210
#define PLUGIN_HIFI_BV                      5211
#define PLUGIN_HIFI_SHUTTER_SPEED           5212
#define PLUGIN_HIFI_FRAME_ISO               5213

#define PLUGIN_HIFI_FOV_RECT                5214
#define PLUGIN_HIFI_FRAME_FACENUM           5215
#define PLUGIN_HIFI_FRAME_FACERECT          5216
/* PLUGIN_HIFI_INPUT_BUFFER_PTR : already define */
/* PLUGIN_HIFI_INPUT_BUFFER_FD : already define */
#define PLUGIN_HIFI_INPUT_WIDTH             5219
#define PLUGIN_HIFI_INPUT_HEIGHT            5220
/* PLUGIN_HIFI_OUTPUT_BUFFER_PTR : already define */
/* PLUGIN_HIFI_OUTPUT_BUFFER_FD : already define */
#define PLUGIN_HIFI_OUTPUT_WIDTH            5223
#define PLUGIN_HIFI_OUTPUT_HEIGHT           5224

#define PLUGIN_WIDE_FULLSIZE_W              6001    /* Data_int32_t        / int          */
#define PLUGIN_WIDE_FULLSIZE_H              6002    /* Data_int32_t        / int          */
#define PLUGIN_TELE_FULLSIZE_W              6003    /* Data_int32_t        / int          */
#define PLUGIN_TELE_FULLSIZE_H              6004    /* Data_int32_t        / int          */
#define PLUGIN_FOCUS_POSTRION_X             6005    /* Data_int32_t        / int          */
#define PLUGIN_FOCUS_POSTRION_Y             6006    /* Data_int32_t        / int          */
#define PLUGIN_AF_STATUS                    6007    /* Array_buf_t         / int          */
#define PLUGIN_ZOOM_RATIO                   6007    /* Pointer_float_t     / int          */
#define PLUGIN_ZOOM_RATIO_LIST_SIZE         6012    /* Array_buf_t         / int          */
#define PLUGIN_ZOOM_RATIO_LIST              6013    /* Array_float_addr_t  / int          */
#define PLUGIN_VIEW_RATIO                   6014    /* Data_float_t        / float        */
#define PLUGIN_IMAGE_RATIO_LIST             6015    /* Array_float_addr_t  / int          */
#define PLUGIN_IMAGE_RATIO                  6016    /* Pointer_float_t     / int          */
#define PLUGIN_NEED_MARGIN_LIST             6017    /* Array_pointer_int_t  / int          */
#define PLUGIN_CAMERA_INDEX                 6018    /* Data_int32_t        / int          */
#define PLUGIN_IMAGE_SHIFT_X                6019    /* Data_int32_t        / int          */
#define PLUGIN_IMAGE_SHIFT_Y                6020    /* Data_int32_t        / int          */
#define PLUGIN_ZOOM_LEVEL                   6021    /* Data_int32_t        / int          */

#define PLUGIN_BOKEH_INTENSITY              6022    /* Data_int32_t        / int          */
#define PLUGIN_BOKEH_TEST                   6023    /* Data_int32_t        / int          */

#define PLUGIN_SYNC_TYPE                    6024    /* Data_int32_t        / int          */
#define PLUGIN_NEED_SWITCH_CAMERA           6025    /* Data_int32_t        / int          */
#define PLUGIN_MASTER_CAMERA_ARC2HAL        6026    /* Data_int32_t        / int          */
#define PLUGIN_MASTER_CAMERA_HAL2ARC        6027    /* Data_int32_t        / int          */
#define PLUGIN_FALLBACK                     6028    /* Data_bool_t         / bool         */

#define PLUGIN_BCROP_RECT                   6028    /* Data_pointer_rect_t         / struct plugin_rect_t */
#define PLUGIN_BUTTON_ZOOM                  6030    /* Data_int32_t        / int          */

#define PLUGIN_CALI_ENABLE                  7001    /* Data_int32_t        / int          */
#define PLUGIN_CALI_RESULT                  7002    /* Data_int32_t        / int          */
#define PLUGIN_VERI_ENABLE                  7003    /* Data_int32_t        / int          */
#define PLUGIN_VERI_RESULT                  7004    /* Data_int32_t        / int          */
#define PLUGIN_CALI_DATA                    7005    /* Array_buf_addr_t    / char *[PLUGIN_MAX_PLANE] */
#define PLUGIN_CALI_SIZE                    7006    /* Data_int32_t        / int          */

/* Scene Detect Lib Start */
#define PLUGIN_SCENE_TYPE                   7100    /* Data_int32_t         / int           */
/* Scene Detect Lib End  */

/* Dual Lib Start */
#define PLUGIN_DUAL_MODE                    7200    /* Data_bool_t          / bool          */
#define PLUGIN_DUAL_SCENARIO                7201    /* Data_int32_t         / int           */
enum {
    PLUGIN_DUAL_SCENARIO_ZOOM,
    PLUGIN_DUAL_SCENARIO_BOKEH,
};
#define PLUGIN_DUAL_DISP_CAM                7202    /* Data_int32_t          / int  */
#define PLUGIN_DUAL_MASTER_CROP             7203    /* Data_pointer_rect_t   / struct plugin_rect_t  */
#define PLUGIN_DUAL_SLAVE_CROP              7204    /* Data_pointer_rect_t   / struct plugin_rect_t  */
#define PLUGIN_DUAL_DETECTED_FACES          7305    /*    / int32                */
#define PLUGIN_DUAL_FACE_ID                 7306    /*    / uint32 [MAX_FACES]   */
#define PLUGIN_DUAL_FACE_RECT               7307    /*    / float [MAX_FACES][4] */
#define PLUGIN_DUAL_FACE_SCORE              7308    /*    / float [MAX_FACES]    */
#define PLUGIN_DUAL_FACE_ROTATION           7309    /*    / float [MAX_FACES]    */
#define PLUGIN_DUAL_FACE_YAW                7310    /*    / float [MAX_FACES]    */
#define PLUGIN_DUAL_FACE_PITCH              7311    /*    / float [MAX_FACES]    */
#define PLUGIN_DUAL_FACE_ORIENTATION        7312    /*    / int32 chenhan add    */

/* Dual Lib End  */

/* VPL Related Start */
#define PLUGIN_NFD_DETECTED_FACES           7300    /*    / int32                */
#define PLUGIN_NFD_FACE_ID                  7301    /*    / uint32 [MAX_FACES]   */
#define PLUGIN_NFD_FACE_RECT                7302    /*    / float [MAX_FACES][4] */
#define PLUGIN_NFD_FACE_SCORE               7303    /*    / float [MAX_FACES]    */
#define PLUGIN_NFD_FACE_ROTATION            7304    /*    / float [MAX_FACES]    */
#define PLUGIN_NFD_FACE_YAW                 7305    /*    / float [MAX_FACES]    */
#define PLUGIN_NFD_FACE_PITCH               7306    /*    / float [MAX_FACES]    */
/* VPL Related End */

/* LEC Related Start */
#define PLUGIN_OPER_MODE                    7400    /*    / int32                */
#define PLUGIN_RESULT                       7401    /*    / int32                */
/* LEC Related End */

/* Bokeh Super Night Shot Related Start */
#define PLUGIN_ANCHOR_FRAME_INDEX           7410    /*    / int32                */
/* Bokeh Super Night Shot Related end */


/*
 * Query ID (0xF0000001 ~)
 */
#define PLUGIN_VERSION                  0xF0000001    /* Data_int32_t         / int            */
#define PLUGIN_LIB_NAME                 0xF0000002    /* Pointer_const_char_t / const char *   */
#define PLUGIN_BUILD_DATE               0xF0000003    /* Pointer_const_char_t / const char *   */  //use __DATE__
#define PLUGIN_BUILD_TIME               0xF0000004    /* Pointer_const_char_t / const char *   */  //use __TIME__
#define PLUGIN_PLUGIN_CUR_SRC_BUF_CNT   0xF0000005    /* Data_int32_t         / int            */
#define PLUGIN_PLUGIN_CUR_DST_BUF_CNT   0xF0000006    /* Data_int32_t         / int            */
#define PLUGIN_PLUGIN_MAX_SRC_BUF_CNT   0xF0000007    /* Data_int32_t         / int            */
#define PLUGIN_PLUGIN_MAX_DST_BUF_CNT   0xF0000008    /* Data_int32_t         / int            */
#define PLUGIN_PRIVATE_HANDLE           0xF0000009    /* Pointer_void_t       / void *         */

#endif
