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
#define LOG_TAG "ExynosCameraSensorInfo"
#include <log/log.h>

#include "ExynosCameraSensorInfo.h"

namespace android {

#ifdef USE_CONFLICTING_LISTS
__unused static char *CONFLICTING_LIST_OPEN_ID_REAR_0[] =
{
    TO_STRING(CAMERA_OPEN_ID_FRONT_1),
};

__unused static char *CONFLICTING_LIST_OPEN_ID_FRONT_1[] =
{
    TO_STRING(CAMERA_OPEN_ID_REAR_0),
};
#endif

static struct HAL_CameraInfo_t sCameraConfigTotalInfo[] = {
#ifdef CAMERA_OPEN_ID_REAR_0
    {
        CAMERA_OPEN_ID_REAR_0,
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
        51,      /* resoruce_cost               : [0 , 100] */
#if 0 /* #ifdef USE_CONFLICTING_LISTS */
        CONFLICTING_LIST_OPEN_ID_REAR_0,
        TO_SIZE(CONFLICTING_LIST_OPEN_ID_REAR_0, char*),
#else
        NULL,
        0,
#endif
    },
#endif
#ifdef CAMERA_OPEN_ID_FRONT_1
    {
        CAMERA_OPEN_ID_FRONT_1,
        CAMERA_FACING_FRONT,
        FRONT_ROTATION,  /* orientation */
        51,      /* resoruce_cost               : [0 , 100] */
#if 0 /* #ifdef USE_CONFLICTING_LISTS */
        CONFLICTING_LIST_OPEN_ID_FRONT_1,
        TO_SIZE(CONFLICTING_LIST_OPEN_ID_FRONT_1, char*),
#else
        NULL,
        0,
#endif
    },
#endif
#ifdef CAMERA_OPEN_ID_REAR_2
    {
        CAMERA_SERVICE_ID_BACK_2,
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
        51,      /* resoruce_cost               : [0 , 100] */
#if 0 /* #ifdef USE_CONFLICTING_LISTS */
        CONFLICTING_LIST_OPEN_ID_REAR_0,
        TO_SIZE(CONFLICTING_LIST_OPEN_ID_REAR_0, char*),
#else
        NULL,
        0,
#endif
    },
#endif

#ifdef CAMERA_OPEN_ID_REAR_3
    {
        CAMERA_SERVICE_ID_BACK_3,
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
        51,      /* resoruce_cost               : [0 , 100] */
#if 0 /* #ifdef USE_CONFLICTING_LISTS */
        CONFLICTING_LIST_OPEN_ID_REAR_0,
        TO_SIZE(CONFLICTING_LIST_OPEN_ID_REAR_0, char*),
#else
        NULL,
        0,
#endif
    },
#endif

#ifdef CAMERA_OPEN_ID_REAR_4
    {
        CAMERA_SERVICE_ID_BACK_4,
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
        51,      /* resoruce_cost               : [0 , 100] */
#if 0 /* #ifdef USE_CONFLICTING_LISTS */
        CONFLICTING_LIST_OPEN_ID_REAR_0,
        TO_SIZE(CONFLICTING_LIST_OPEN_ID_REAR_0, char*),
#else
        NULL,
        0,
#endif
    },
#endif

#ifdef CAMERA_OPEN_ID_LCAM_0
    {
        CAMERA_SERVICE_ID_LCAM_0,
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
        100,      /* resoruce_cost               : [0 , 100] */
        NULL,
        0,
    },
#endif
};

bool m_getSensorGyroSupport(int cameraId)
{
    bool ret = false;

    switch (cameraId) {
#ifdef SUPPORT_SENSOR_GYRO_BACK_0
    case CAMERA_ID_BACK:
        ret = true;
        break;
#endif
#ifdef SUPPORT_SENSOR_GYRO_FRONT_0
    case CAMERA_ID_FRONT:
        ret = true;
        break;
#endif
#ifdef SUPPORT_SENSOR_GYRO_FRONT_1
    case CAMERA_ID_FRONT_2:
        ret = true;
        break;
#endif
    default:
        break;
    }

    return ret;
}

struct ExynosCameraSensorInfoBase *createExynosCameraSensorInfo(int cameraId, __unused int serviceCameraId)
{
    struct ExynosCameraSensorInfoBase *sensorInfo = NULL;
    int sensorId = getSensorId(cameraId);
    if (sensorId < 0) {
        ALOGE("[CAM_ID(%d)]-ERR(%s[%d]):Inavalid sensorId %d",
                cameraId, __FUNCTION__, __LINE__, sensorId);
        sensorId = SENSOR_NAME_NOTHING;
    }

    switch (sensorId) {
    case SENSOR_NAME_S5K2L7:
        sensorInfo = new ExynosCameraSensor2L7();
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2L7");
        break;
    case SENSOR_NAME_S5K2P6:
        sensorInfo = new ExynosCameraSensor2P6();
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2P6");
        break;
    case SENSOR_NAME_S5K2P8:
        sensorInfo = new ExynosCameraSensor2P8();
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2P8");
        break;
    case SENSOR_NAME_S5K3M3:
        sensorInfo = new ExynosCameraSensor3M3(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K3M3");
        break;
    case SENSOR_NAME_SAK2L3:
        sensorInfo = new ExynosCameraSensor2L3(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "SAK2L3");
        break;
    case SENSOR_NAME_IMX333:
        sensorInfo = new ExynosCameraSensorIMX333_2L2(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "IMX333");
        break;
    case SENSOR_NAME_S5K2L2:
        sensorInfo = new ExynosCameraSensorIMX333_2L2(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2L2");
        break;
    case SENSOR_NAME_SAK2L4:
        sensorInfo = new ExynosCameraSensor2L4(serviceCameraId, sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "SAK2L4");
        break;
    case SENSOR_NAME_S5K3P9:
        sensorInfo = new ExynosCameraSensor3P9(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K3P9");
        break;
    case SENSOR_NAME_IMX320:
        sensorInfo = new ExynosCameraSensorIMX320_3H1(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "IMX320");
        break;
    case SENSOR_NAME_S5K3H1:
        sensorInfo = new ExynosCameraSensorIMX320_3H1(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K3H1");
        break;
    case SENSOR_NAME_S5K3J1:
        sensorInfo = new ExynosCameraSensor3J1(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K3J1");
        break;
    case SENSOR_NAME_S5K4HA:
        sensorInfo = new ExynosCameraSensor4HA(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K4HA");
        break;
    case SENSOR_NAME_S5K5E9:
        sensorInfo = new ExynosCameraSensor5E9(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K5E9");
        break;
    case SENSOR_NAME_S5K5F1:
        sensorInfo = new ExynosCameraSensorS5K5F1(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K5F1");
        break;
    case SENSOR_NAME_S5KRPB:
        sensorInfo = new ExynosCameraSensorS5KRPB(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "RPB");
        break;
    case SENSOR_NAME_S5K2P7SQ:
        sensorInfo = new ExynosCameraSensorS5K2P7SQ(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2P7SQ");
        break;
    case SENSOR_NAME_S5K2T7SX:
        sensorInfo = new ExynosCameraSensorS5K2T7SX(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "S5K2T7SX");
        break;
    case SENSOR_NAME_S5K6B2:
        sensorInfo = new ExynosCameraSensor6B2(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "6B2");
        break;
    case SENSOR_NAME_IMX576:
        sensorInfo = new ExynosCameraSensorIMX576(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "IMX576");
        break;
    case SENSOR_NAME_S5K2X5SP:
        sensorInfo = new ExynosCameraSensor2X5SP(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "2X5SP");
        break;
    case SENSOR_NAME_S5KGM1SP:
        sensorInfo = new ExynosCameraSensorGM1SP(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "GM1SP");
        break;
    case SENSOR_NAME_OV12A10:
        sensorInfo = new ExynosCameraSensor12A10(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "12A10");
        break;
    case SENSOR_NAME_OV12A10FF:
        sensorInfo = new ExynosCameraSensor12A10FF(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "12A10FF");
        break;
    case SENSOR_NAME_OV16885C:
        sensorInfo = new ExynosCameraSensor16885C(sensorId);
        snprintf(sensorInfo->name, sizeof(sensorInfo->name), "16885C");
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "[CAM_ID(%d)]-ASSERT(%s[%d]):Unknown sensorId %d",
                cameraId, __FUNCTION__, __LINE__, sensorId);
        break;
    }

    ////////////////////////////////////////////////
    // set common infomation
    sensorInfo->sensorId = sensorId;

    // decide sensor gyro.
    sensorInfo->sensorGyroSupport = m_getSensorGyroSupport(cameraId);

    ////////////////////////////////////////////////

    ALOGI("[CAM_ID(%d)]-INFO(%s[%d]):sensorId %d name %s",
            cameraId, __FUNCTION__, __LINE__, sensorId, sensorInfo->name);

    return sensorInfo;
}

struct HAL_CameraInfo_t *getExynosCameraVendorDeviceInfoByServiceCamId(int serviceCameraId)
{
    struct HAL_CameraInfo_t *cameraInfo = NULL;
    int size = getExynosCameraVendorDeviceInfoSize();

    for (int i = 0; i < size; i++) {
        if (sCameraConfigTotalInfo[i].cameraId == serviceCameraId) {
            cameraInfo = &sCameraConfigTotalInfo[i];
            break;
        }
    }
    return cameraInfo;
}

struct HAL_CameraInfo_t *getExynosCameraVendorDeviceInfoByCamIndex(int camIndex)
{
    return &sCameraConfigTotalInfo[camIndex];
}

int getExynosCameraVendorDeviceInfoSize()
{
    int size = sizeof(sCameraConfigTotalInfo) / sizeof(HAL_CameraInfo_t);
    return size;
}

ExynosCameraSensor2L7::ExynosCameraSensor2L7() : ExynosCameraSensor2L7Base()
{
    /* Use ExynosCameraSensor2L7Base Constructor */
    /* Optional capabilities */
    supportedCapabilities |= (CAPABILITIES_PRIVATE_REPROCESSING | CAPABILITIES_RAW |
                              CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
};

ExynosCameraSensor2P6::ExynosCameraSensor2P6() : ExynosCameraSensor2P6Base()
{
    /* Use ExynosCameraSensorS5K2P8Base Constructor */
    /* Optional capabilities : vendor feature */
    supportedCapabilities |= (CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
};

ExynosCameraSensor2P8::ExynosCameraSensor2P8() : ExynosCameraSensor2P8Base()
{
    /* Use ExynosCameraSensorS5K2P8Base Constructor */
};

ExynosCameraSensorIMX333_2L2::ExynosCameraSensorIMX333_2L2(int sensorId) : ExynosCameraSensorIMX333_2L2Base(sensorId)
{
    /* Use ExynosCameraSensorIMX333_2L2Base Constructor */
};

ExynosCameraSensor2L4::ExynosCameraSensor2L4(int serviceCameraId, int sensorId) : ExynosCameraSensor2L4Base(serviceCameraId, sensorId)
{
    /* Use ExynosCameraSensor2L4Base Constructor */
    supportedCapabilities |= (CAPABILITIES_PRIVATE_REPROCESSING | CAPABILITIES_RAW |
                              CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
};

ExynosCameraSensor3P9::ExynosCameraSensor3P9(int sensorId) : ExynosCameraSensor3P9Base(sensorId)
{
    /* Use ExynosCameraSensor3P9Base Constructor */
};

ExynosCameraSensorIMX320_3H1::ExynosCameraSensorIMX320_3H1(int sensorId) : ExynosCameraSensorIMX320_3H1Base(sensorId)
{
    /* Use ExynosCameraSensorIMX320_3H1Base Constructor */
    /* Optional capabilities */
    supportedCapabilities |= (CAPABILITIES_BURST_CAPTURE | CAPABILITIES_PRIVATE_REPROCESSING);
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    aperture = 1.9f;
    fNumber = 1.9f;
};

ExynosCameraSensorIMX576::ExynosCameraSensorIMX576(int sensorId) : ExynosCameraSensorIMX576Base(sensorId)
{
    /* Use ExynosCameraSensorIMX576Base Constructor */
};

ExynosCameraSensor2L3::ExynosCameraSensor2L3(int sensorId) : ExynosCameraSensor2L3Base(sensorId)
{
    /* Optional capabilities : vendor feature */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;

    supportedCapabilities |= (CAPABILITIES_PRIVATE_REPROCESSING | CAPABILITIES_RAW |
                                CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
};

ExynosCameraSensor3J1::ExynosCameraSensor3J1(int sensorId) : ExynosCameraSensor3J1Base(sensorId)
{
    /* Use ExynosCameraSensor3J1Base Constructor */
    supportedCapabilities |= (CAPABILITIES_BURST_CAPTURE | CAPABILITIES_PRIVATE_REPROCESSING);
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
};

ExynosCameraSensor3M3::ExynosCameraSensor3M3(int sensorId) : ExynosCameraSensor3M3Base(sensorId)
{
    /* Use ExynosCameraSensorS5K3M3Base Constructor */
};

ExynosCameraSensorS5K5F1::ExynosCameraSensorS5K5F1(int sensorId) : ExynosCameraSensorS5K5F1Base(sensorId)
{
    gain = 20;                      // 2.0;
    exposureTime = 332 * 100000;    // 33.2ms;
    ledCurrent = 5;                 // 450mA
    ledPulseDelay = 0 * 100000;     // 0ms
    ledPulseWidth = 240 * 100000;   // 24ms
    ledMaxTime = 10 * 1000;         // 10s;

    gainRange[MIN] = 1;
    gainRange[MAX] = 160;
    ledCurrentRange[MIN] = 1;                // 0mA
    ledCurrentRange[MAX] = 10;               // 950mA
    ledPulseDelayRange[MIN] = 0 * 100000;    // 0.0ms
    ledPulseDelayRange[MAX] = 1000 * 100000; // 100.0ms
    ledPulseWidthRange[MIN] = 0 * 100000;    // 0.0ms
    ledPulseWidthRange[MAX] = 333 * 100000;  // 33.3ms
    ledMaxTimeRange[MIN] = 1 * 1000;         // 1s
    ledMaxTimeRange[MAX] = 10 * 1000;        // 10s
};

ExynosCameraSensorS5KRPB::ExynosCameraSensorS5KRPB(int sensorId) : ExynosCameraSensorS5KRPBBase(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
};

ExynosCameraSensorS5K2P7SQ::ExynosCameraSensorS5K2P7SQ(int sensorId) : ExynosCameraSensor2P7SQBase(sensorId)
{
    /* Use ExynosCameraSensorS5K2P7SQBase Constructor */
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensorS5K2T7SX::ExynosCameraSensorS5K2T7SX(int sensorId) : ExynosCameraSensor2T7SXBase(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
};

ExynosCameraSensor6B2::ExynosCameraSensor6B2(int sensorId) : ExynosCameraSensor6B2Base(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
    colorTransformMatrix1 = COLOR_MATRIX1_S5K6B2_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_S5K6B2_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_S5K6B2_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_S5K6B2_3X3;
    supportedCapabilities |= (CAPABILITIES_YUV_REPROCESSING);

    supportedCapabilities &= ~(CAPABILITIES_PRIVATE_REPROCESSING);
};

ExynosCameraSensor5E9::ExynosCameraSensor5E9(int sensorId) : ExynosCameraSensor5E9Base(sensorId)
{
    /* Use ExynosCameraSensorS5K5E9Base Constructor */
    colorTransformMatrix1 = COLOR_MATRIX1_S5K6B2_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_S5K6B2_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_S5K6B2_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_S5K6B2_3X3;
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensorGM1SP::ExynosCameraSensorGM1SP(int sensorId) : ExynosCameraSensorGM1SPBase(sensorId)
{
    /* Use ExynosCameraSensorS5K5E9Base Constructor */
    sessionKeys = AVAILABLE_SESSION_KEYS_VENDOR;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_VENDOR);
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensor12A10::ExynosCameraSensor12A10(int sensorId) : ExynosCameraSensor12A10Base(sensorId)
{
    sessionKeys = AVAILABLE_SESSION_KEYS_VENDOR;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_VENDOR);
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensor12A10FF::ExynosCameraSensor12A10FF(int sensorId) : ExynosCameraSensor12A10FFBase(sensorId)
{
    sessionKeys = AVAILABLE_SESSION_KEYS_VENDOR;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_VENDOR);
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensor16885C::ExynosCameraSensor16885C(int sensorId) : ExynosCameraSensor16885CBase(sensorId)
{
    sessionKeys = AVAILABLE_SESSION_KEYS_VENDOR;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_VENDOR);
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensor2X5SP::ExynosCameraSensor2X5SP(int sensorId) : ExynosCameraSensor2X5SPBase(sensorId)
{
    /* Use ExynosCameraSensor2X5SPBase Constructor */
    sessionKeys = AVAILABLE_SESSION_KEYS_VENDOR;
    sessionKeysLength = ARRAY_LENGTH(AVAILABLE_SESSION_KEYS_VENDOR);
    supportedCapabilities |= (CAPABILITIES_RAW) | (CAPABILITIES_YUV_REPROCESSING);
};

ExynosCameraSensor4HA::ExynosCameraSensor4HA(int sensorId) : ExynosCameraSensor4HABase(sensorId)
{
    /* Use ExynosCameraSensorS5K4HABase Constructor */
};

/* Convert Id to the one for HAL. Refer to the enum CAMERA_ID in ExynosCameraSensorInfoBase.h  */
int getCameraIdInfo(cameraId_Info *camIdInfo)
{
    uint32_t numOfSensors = 1;
    camIdInfo->scenario = SCENARIO_NORMAL;

    switch (camIdInfo->serviceCameraId) {
        /* open ID */
#ifdef CAMERA_OPEN_ID_REAR_0
        case CAMERA_OPEN_ID_REAR_0:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = CAMERA_INDEX_REAR_0;
            camIdInfo->numOfSensors = 1;
            camIdInfo->logicalSyncType = -1;
            break;
#endif
#ifdef CAMERA_OPEN_ID_FRONT_1
        case CAMERA_OPEN_ID_FRONT_1:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_FRONT;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = CAMERA_INDEX_FRONT_1;
            camIdInfo->numOfSensors = 1;
            camIdInfo->logicalSyncType = -1;
            break;
#endif
#ifdef CAMERA_OPEN_ID_REAR_2
        case CAMERA_SERVICE_ID_BACK_2:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK_2;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = CAMERA_INDEX_REAR_2;
            camIdInfo->numOfSensors = 1;
            camIdInfo->logicalSyncType = -1;
            break;
#endif

#ifdef CAMERA_OPEN_ID_REAR_3
        case CAMERA_SERVICE_ID_BACK_3:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK_3;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = CAMERA_INDEX_REAR_3;
            camIdInfo->numOfSensors = 1;
            camIdInfo->logicalSyncType = -1;
            break;
#endif

#ifdef CAMERA_OPEN_ID_REAR_4
        case CAMERA_SERVICE_ID_BACK_4:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK_4;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = CAMERA_INDEX_REAR_4;
            camIdInfo->numOfSensors = 1;
            camIdInfo->logicalSyncType = -1;
            break;
#endif

#ifdef USES_DUAL_REAR_ZOOM
        case CAMERA_SERVICE_ID_DUAL_REAR_ZOOM:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK;
            camIdInfo->cameraId[SUB_CAM] = CAMERA_ID_BACK_2;
            camIdInfo->camInfoIndex = CAMERA_INDEX_DUAL_REAR_ZOOM;
            camIdInfo->numOfSensors = 2;
            camIdInfo->scenario = SCENARIO_DUAL_REAR_ZOOM;
            camIdInfo->logicalSyncType = SENSOR_SYNC_TYPE_CALIBRATED;
            camIdInfo->dualTransitionInfo = {
                { 1.49, { DUAL_OPERATION_MODE_MASTER, DUAL_OPERATION_MODE_MASTER, { CAMERA_ID_BACK,                    }, DUAL_OPERATION_SENSOR_BACK_MAIN    , DUAL_OPERATION_SENSOR_BACK_MAIN    } },
                { 1.99, { DUAL_OPERATION_MODE_SYNC,   DUAL_OPERATION_MODE_SYNC,   { CAMERA_ID_BACK,   CAMERA_ID_BACK_2 }, DUAL_OPERATION_SENSOR_BACK_MAIN_SUB, DUAL_OPERATION_SENSOR_BACK_MAIN_SUB} },
                { 2.29, { DUAL_OPERATION_MODE_SYNC,   DUAL_OPERATION_MODE_SLAVE,  { CAMERA_ID_BACK_2, CAMERA_ID_BACK   }, DUAL_OPERATION_SENSOR_BACK_MAIN_SUB, DUAL_OPERATION_SENSOR_BACK_SUB     } },
                { 10.0, { DUAL_OPERATION_MODE_SLAVE,  DUAL_OPERATION_MODE_SLAVE,  { CAMERA_ID_BACK_2,                  }, DUAL_OPERATION_SENSOR_BACK_SUB     , DUAL_OPERATION_SENSOR_BACK_SUB     } },
            };
            break;
#endif

#ifdef CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_TELE_MAIN
        case CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_TELE:
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_TELE_MAIN;
            camIdInfo->cameraId[SUB_CAM] = CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_TELE_SUB;
            camIdInfo->camInfoIndex = CAMERA_INDEX_DUAL_REAR_PORTRAIT_TELE;
            camIdInfo->numOfSensors = 2;
            camIdInfo->scenario = SCENARIO_DUAL_REAR_PORTRAIT;
            break;
#endif

#ifdef CAMERA_OPEN_ID_LCAM_0
        case CAMERA_SERVICE_ID_LCAM_0:
#if defined (LCAM_0_ID_0) && defined (LCAM_0_ID_1)
            camIdInfo->cameraId[MAIN_CAM] = LCAM_0_ID_0;
            camIdInfo->cameraId[SUB_CAM] = LCAM_0_ID_1;
#else
            camIdInfo->cameraId[MAIN_CAM] = CAMERA_ID_BACK;
            camIdInfo->cameraId[SUB_CAM] = CAMERA_ID_BACK_3;
#endif
            camIdInfo->camInfoIndex = CAMERA_INDEX_LCAM_0;
            camIdInfo->numOfSensors = 2;
            camIdInfo->scenario = SCENARIO_DUAL_REAR_ZOOM;
            camIdInfo->logicalSyncType = SENSOR_SYNC_TYPE_CALIBRATED;
        break;
#endif
        default:
            ALOGV("ERR(%s[%d]:Invaild Camera Id(%d)", __FUNCTION__, __LINE__, camIdInfo->serviceCameraId);
            camIdInfo->cameraId[MAIN_CAM] = -1;
            camIdInfo->cameraId[SUB_CAM] = -1;
            camIdInfo->camInfoIndex = -1;
            camIdInfo->numOfSensors = -1;
            camIdInfo->logicalSyncType = -1;
            break;
    }

    return camIdInfo->numOfSensors;
}

int getPhysicalIdFromInternalId(int id)
{
    int serviceID = -1;

    switch (id) {
        case CAMERA_ID_BACK:
#ifdef  CAMERA_OPEN_ID_REAR_0
            serviceID = CAMERA_OPEN_ID_REAR_0;
#endif
            break;
        case CAMERA_ID_FRONT:
#ifdef  CAMERA_OPEN_ID_FRONT_1
            serviceID = CAMERA_OPEN_ID_FRONT_1;
#endif
            break;

        case CAMERA_ID_BACK_2:
#ifdef  CAMERA_OPEN_ID_REAR_2
            serviceID = CAMERA_SERVICE_ID_BACK_2;
#endif
            break;
        case CAMERA_ID_BACK_3:
#ifdef  CAMERA_OPEN_ID_REAR_3
            serviceID = CAMERA_SERVICE_ID_BACK_3;
#endif
            break;

        default:
            return -1;
    }
    return serviceID;
}


}; /* namespace android */
