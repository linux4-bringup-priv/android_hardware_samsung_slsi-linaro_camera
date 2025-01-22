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

#ifndef EXYNOS_CAMERA_SENSOR_INFO_H
#define EXYNOS_CAMERA_SENSOR_INFO_H

#include "ExynosCameraSensorInfoBase.h"

namespace android {

struct ExynosCameraSensor2L3 : public ExynosCameraSensor2L3Base {
public:
    ExynosCameraSensor2L3(int sensorId);
};

struct ExynosCameraSensor2L7 : public ExynosCameraSensor2L7Base {
public:
    ExynosCameraSensor2L7();
};

struct ExynosCameraSensor2P6: public ExynosCameraSensor2P6Base {
public:
    ExynosCameraSensor2P6();
};

struct ExynosCameraSensor2P8 : public ExynosCameraSensor2P8Base {
public:
    ExynosCameraSensor2P8();
};

struct ExynosCameraSensorIMX333_2L2 : public ExynosCameraSensorIMX333_2L2Base {
public:
    ExynosCameraSensorIMX333_2L2(int sensorId);
};

struct ExynosCameraSensor2L4 : public ExynosCameraSensor2L4Base {
public:
    ExynosCameraSensor2L4(int serviceCameraId, int sensorId);
};

struct ExynosCameraSensor3P9 : public ExynosCameraSensor3P9Base {
public:
    ExynosCameraSensor3P9(int sensorId);
};

struct ExynosCameraSensorIMX320_3H1 : public ExynosCameraSensorIMX320_3H1Base {
public:
    ExynosCameraSensorIMX320_3H1(int sensorId);
};

struct ExynosCameraSensorIMX576: public ExynosCameraSensorIMX576Base {
public:
    ExynosCameraSensorIMX576(int sensorId);
};

struct ExynosCameraSensor3J1 : public ExynosCameraSensor3J1Base {
public:
    ExynosCameraSensor3J1(int sensorId);
};

struct ExynosCameraSensor3M3 : public ExynosCameraSensor3M3Base {
public:
    ExynosCameraSensor3M3(int sensorId);
};

struct ExynosCameraSensorS5K5F1 : public ExynosCameraSensorS5K5F1Base {
public:
    ExynosCameraSensorS5K5F1(int sensorId);
};

struct ExynosCameraSensorS5KRPB : public ExynosCameraSensorS5KRPBBase {
public:
    ExynosCameraSensorS5KRPB(int sensorId);
};

struct ExynosCameraSensorS5K2P7SQ : public ExynosCameraSensor2P7SQBase {
public:
    ExynosCameraSensorS5K2P7SQ(int sensorId);
};


struct ExynosCameraSensorS5K2T7SX : public ExynosCameraSensor2T7SXBase {
public:
    ExynosCameraSensorS5K2T7SX(int sensorId);
};

struct ExynosCameraSensor6B2 : public ExynosCameraSensor6B2Base {
public:
    ExynosCameraSensor6B2(int sensorId);
};


struct ExynosCameraSensor5E9 : public ExynosCameraSensor5E9Base {
public:
    ExynosCameraSensor5E9(int sensorId);
};


struct ExynosCameraSensor4HA : public ExynosCameraSensor4HABase {
public:
    ExynosCameraSensor4HA(int sensorId);
};

struct ExynosCameraSensor3L2 : public ExynosCameraSensor3L2Base {
public:
    ExynosCameraSensor3L2(int sensorId);
};

struct ExynosCameraSensor4H5YC : public ExynosCameraSensor4H5YCBase {
public:
    ExynosCameraSensor4H5YC(int sensorId);
};

struct ExynosCameraSensorGM1SP : public ExynosCameraSensorGM1SPBase {
public:
    ExynosCameraSensorGM1SP(int sensorId);
};

/* Helpper functions */
struct ExynosCameraSensorInfoBase *createExynosCameraSensorInfo(int cameraId, int serviceCameraId = 0);
struct HAL_CameraInfo_t *getExynosCameraVendorDeviceInfoByServiceCamId(int serviceCameraId);
struct HAL_CameraInfo_t *getExynosCameraVendorDeviceInfoByCamIndex(int camIndex);
int getExynosCameraVendorDeviceInfoSize();

int getCameraIdInfo(cameraId_Info *camIdInfo);
int getPhysicalIdFromInternalId(int);

}; /* namespace android */
#endif
