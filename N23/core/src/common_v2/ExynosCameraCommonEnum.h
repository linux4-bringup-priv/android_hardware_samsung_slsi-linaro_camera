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

#ifndef EXYNOS_CAMERA_COMMON_ENUM_H
#define EXYNOS_CAMERA_COMMON_ENUM_H

enum CAMERA_ID {  // Internal ID
    CAMERA_ID_BACK              = 0,
    CAMERA_ID_FRONT             = 1,
    CAMERA_ID_BACK_2,
    CAMERA_ID_FRONT_2,
    CAMERA_ID_BACK_3,
    CAMERA_ID_FRONT_3,
    CAMERA_ID_BACK_4,
    CAMERA_ID_FRONT_4,
    CAMERA_ID_BACK_TOF,
    CAMERA_ID_FRONT_TOF,
    CAMERA_ID_SECURE,
    CAMERA_ID_LCAM_0,   //Logical Camera
    CAMERA_ID_MAX,
};

typedef enum
{
    SENSOR_NAME_NOTHING          = 0,
    /* 1~100: SAMSUNG sensors */
    SENSOR_NAME_S5K3H2           = 1,
    SENSOR_NAME_S5K6A3           = 2,
    SENSOR_NAME_S5K3H5           = 3,
    SENSOR_NAME_S5K3H7           = 4,
    SENSOR_NAME_S5K3H7_SUNNY     = 5,
    SENSOR_NAME_S5K3H7_SUNNY_2M  = 6,
    SENSOR_NAME_S5K6B2           = 7,
    SENSOR_NAME_S5K3L2           = 8,
    SENSOR_NAME_S5K4E5           = 9,
    SENSOR_NAME_S5K2P2           = 10,
    SENSOR_NAME_S5K8B1           = 11,
    SENSOR_NAME_S5K1P2           = 12,
    SENSOR_NAME_S5K4H5           = 13,
    SENSOR_NAME_S5K3M2           = 14,
    SENSOR_NAME_S5K2P2_12M       = 15,
    SENSOR_NAME_S5K6D1           = 16,
    SENSOR_NAME_S5K5E3           = 17,
    SENSOR_NAME_S5K2T2           = 18,
    SENSOR_NAME_S5K2P3           = 19,
    SENSOR_NAME_S5K2P8           = 20,
    SENSOR_NAME_S5K4E6           = 21,
    SENSOR_NAME_S5K5E2           = 22,
    SENSOR_NAME_S5K3P3           = 23,
    SENSOR_NAME_S5K4H5YC         = 24,
    SENSOR_NAME_S5K3L8_MASTER    = 25,
    SENSOR_NAME_S5K3L8_SLAVE     = 26,
    SENSOR_NAME_S5K4H8           = 27,
    SENSOR_NAME_S5K2X8           = 28,
    SENSOR_NAME_S5K2L1           = 29,
    SENSOR_NAME_S5K3P8           = 30,
    SENSOR_NAME_S5K3H1           = 31,
    SENSOR_NAME_S5K2L2           = 32,
    SENSOR_NAME_S5K3M3           = 33,
    SENSOR_NAME_S5K4H5YC_FF      = 34,
    SENSOR_NAME_S5K2L7           = 35,
    SENSOR_NAME_SAK2L3           = 36,
    SENSOR_NAME_SAK2L4           = 37,
    SENSOR_NAME_S5K3J1           = 38,
    SENSOR_NAME_S5K4HA           = 39,
    SENSOR_NAME_S5K3P9           = 40,
    SENSOR_NAME_S5K5E9           = 41,
    SENSOR_NAME_S5K2X5SP         = 42,
    SENSOR_NAME_S5KGM1SP         = 43,
    SENSOR_NAME_S5K3P8SP         = 44,
    SENSOR_NAME_S5K2P7SX         = 45,
    SENSOR_NAME_S5KRPB           = 46,
    SENSOR_NAME_S5K2P7SQ         = 47,
    SENSOR_NAME_S5K2T7SX         = 48,
    SENSOR_NAME_S5K2PAS          = 49,

    SENSOR_NAME_S5K4EC           = 57,
    SENSOR_NAME_S5K2P6           = 58,
    SENSOR_NAME_S5K3L6           = 59,
    SENSOR_NAME_S5K2X5           = 60,

    /* 101~200: SONY sensors */
    SENSOR_NAME_IMX135           = 101,
    SENSOR_NAME_IMX134           = 102,
    SENSOR_NAME_IMX175           = 103,
    SENSOR_NAME_IMX240           = 104,
    SENSOR_NAME_IMX220           = 105,
    SENSOR_NAME_IMX228           = 106,
    SENSOR_NAME_IMX219           = 107,
    SENSOR_NAME_IMX230           = 108,
    SENSOR_NAME_IMX260           = 109,
    SENSOR_NAME_IMX258           = 110,
    SENSOR_NAME_IMX320           = 111,
    SENSOR_NAME_IMX333           = 112,
    SENSOR_NAME_IMX241           = 113,
    SENSOR_NAME_IMX576           = 115,

    /* 201~255: Other vendor sensors */
    SENSOR_NAME_SR261            = 201,
    SENSOR_NAME_OV5693           = 202,
    SENSOR_NAME_SR544            = 203,
    SENSOR_NAME_OV5670           = 204,
    SENSOR_NAME_DSIM             = 205,
    SENSOR_NAME_SR259            = 206,
    SENSOR_NAME_VIRTUAL          = 207,
    SENSOR_NAME_OV12A10          = 208,
    SENSOR_NAME_OV12A10FF        = 209,
    SENSOR_NAME_OV16885C         = 210,

    /* 256~: currently not used */
    SENSOR_NAME_CUSTOM           = 301,
    SENSOR_NAME_SR200            = 302, // SoC Module
    SENSOR_NAME_SR352            = 303,
    SENSOR_NAME_SR130PC20        = 304,

    SENSOR_NAME_S5K5E6           = 305, // IRIS Camera Sensor
    SENSOR_NAME_S5K5F1           = 306, // STAR IRIS Sensor

    SENSOR_NAME_VIRTUAL_ZEBU     = 901,
    SENSOR_NAME_END,
} IS_SensorNameEnum;

#define DEFAULT_SLAVE_CAM_IDX (1)

enum {
    MAIN_CAM    = 0,
    SUB_CAM     = 1,
    SUB_CAM2    = 2,
    TOF_CAM     = 3,
    MAX_NUM_SENSORS
    /*
     * need to decide: Using CAMERA_ID_MAX instead of MAX_NUM_SENSORS
     * gives flexibility at the cost of memory size.
     */
};


enum DUAL_OPERATION_MODE {
    DUAL_OPERATION_MODE_NONE,
    DUAL_OPERATION_MODE_MASTER,
    DUAL_OPERATION_MODE_SLAVE,
    DUAL_OPERATION_MODE_SYNC,
    DUAL_OPERATION_MODE_MAX,
};

enum DUAL_OPERATION_SENSORS {
    /*
     * The sensor names are abstracted names.
     * CAM_ID[0] <==> MAIN
     * CAM_ID[1] <==> SUB
     * CAM_ID[2] <==> SUB2
     */
    DUAL_OPERATION_SENSOR_FRONT_MAIN,
    DUAL_OPERATION_SENSOR_FRONT_SUB,
    DUAL_OPERATION_SENSOR_FRONT_MAIN_SUB,
    DUAL_OPERATION_SENSOR_BACK_MAIN,    // SENSOR0
    DUAL_OPERATION_SENSOR_BACK_SUB,     // SENSOR1
    DUAL_OPERATION_SENSOR_BACK_SUB2,    // SENSOR2
    DUAL_OPERATION_SENSOR_BACK_MAIN_SUB, // SENSOR0 & SENSOR1
    DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2, // SENSOR0 & SENSOR2
    // for only reprocessing
    DUAL_OPERATION_SENSOR_BACK_SUB_MAIN,  // SENSOR1 & SENSOR0
    DUAL_OPERATION_SENSOR_BACK_SUB2_MAIN, // SENSOR2 & SENSOR0
    DUAL_OPERATION_SENSOR_MAX,
};

enum HW_CONNECTION_MODE {
    HW_CONNECTION_MODE_NONE                 = -1,
    HW_CONNECTION_MODE_M2M                  = 0,
    HW_CONNECTION_MODE_OTF,
    HW_CONNECTION_MODE_M2M_BUFFER_HIDING	= 2,
    /* Unused connection mode               = 3 */
    HW_CONNECTION_MODE_VIRTUAL_OTF          = 4,
};

enum HW_CHAIN_TYPE {
    HW_CHAIN_TYPE_NONE,
    HW_CHAIN_TYPE_SINGLE_CHAIN,     /* it has 3aa(1) and isp(1) */
    HW_CHAIN_TYPE_SEMI_DUAL_CHAIN,  /* it has 3aa(2) and isp(1) */
    HW_CHAIN_TYPE_DUAL_CHAIN,       /* it has 3aa(2) and isp(2) */
};

enum FRAME_FACTORY_TYPE {
    FRAME_FACTORY_TYPE_REPROCESSING = 0,
    FRAME_FACTORY_TYPE_REPROCESSING_MAX = FRAME_FACTORY_TYPE_REPROCESSING + MAX_NUM_SENSORS,
    FRAME_FACTORY_TYPE_CAPTURE_PREVIEW = FRAME_FACTORY_TYPE_REPROCESSING_MAX,
    FRAME_FACTORY_TYPE_CAPTURE_PREVIEW_MAX = FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + MAX_NUM_SENSORS,
    FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING,
    FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING_MAX = FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING + MAX_NUM_SENSORS,
    FRAME_FACTORY_TYPE_VISION,
    FRAME_FACTORY_TYPE_MAX,
};
#define GET_FRAME_FACTORY_TYPE(x) (enum FRAME_FACTORY_TYPE) (x)

enum {
    NOT_SUPPORT = -1,
    DISABLE = 0,
    ENABLE = 1,
};

typedef enum FRAME_SIZE_SCENARIO {
    FRAME_SIZE_BASE,
    FRAME_SIZE_BCROP_IN,
    FRAME_SIZE_BCROP_OUT,
    FRAME_SIZE_BDS,
    FRAME_SIZE_ISP_CROP_IN,
    FRAME_SIZE_MCS_CROP_IN,
    FRAME_SIZE_MCP_CROP,
    FRAME_SIZE_MCP_OUT,
    FRAME_SIZE_FUSION_CROP,
    FRAME_SIZE_FUSION_SCALEUP,
    FRAME_SIZE_VDIS_CROP,
    FRAME_SIZE_MAX,
} frame_size_scenario_t;

#endif /* EXYNOS_CAMERA_COMMON_CONFIG_H */
