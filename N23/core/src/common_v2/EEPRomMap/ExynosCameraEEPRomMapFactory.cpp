/*
 * Copyright@ Samsung Electronics Co. LTD
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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraEEPRomMapFactory"

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSensorInfoBase.h"

#include "ExynosCameraEEPRomMapFactory.h"
#include "ExynosCameraEEPRomMapDefault.h"
#include "ExynosCameraEEPRomMap2P7SQ.h"
#include "ExynosCameraEEPRomMap6B2.h"
#include "ExynosCameraEEPRomMapGM1SP.h"
#include "ExynosCameraEEPRomMap2X5SP.h"
#include "ExynosCameraEEPRomMap5E9.h"
#include "ExynosCameraEEPRomMap5E9_OTP.h"
#include "ExynosCameraEEPRomMapOV12A10.h"
#include "ExynosCameraEEPRomMapOV12A10FF.h"
#include "ExynosCameraEEPRomMapOV16885C.h"

ExynosCameraEEPRomMap *ExynosCameraEEPRomMapFactory::newEEPRomMap(int cameraId)
{
    ExynosCameraEEPRomMap *newEEPRom = NULL;

    int m_cameraId = cameraId;
    char m_name[EXYNOS_CAMERA_NAME_STR_SIZE] = "ExynosCameraEEPRomMapFactory";

    ////////////////////////////////////////////////
    // get snesorId by cameraId
    int sensorId = getSensorId(cameraId);
    if(sensorId < 0) {
        CLOGE("Inavalid sensorId(%d), by cameraId(%d)", sensorId, cameraId);
        sensorId = SENSOR_NAME_NOTHING;
    }

    ////////////////////////////////////////////////
    // new EEPRom, according sensorId
    switch (sensorId) {
    case SENSOR_NAME_S5K2P7SQ:
        //newEEPRom = new ExynosCameraEEPRomMap2P7SQ(cameraId);
        newEEPRom = new ExynosCameraEEPRomMapGM1SP(cameraId);
        break;
    case SENSOR_NAME_S5K6B2:
        //newEEPRom = new ExynosCameraEEPRomMap6B2(cameraId);
        newEEPRom = new ExynosCameraEEPRomMap2X5SP(cameraId);
        break;
    case SENSOR_NAME_S5K5E9:
#ifdef MOT_9609_SENSORS_TROIKA
        newEEPRom = new ExynosCameraEEPRomMap5E9_OTP(cameraId);
#else
        newEEPRom = new ExynosCameraEEPRomMap5E9(cameraId);
#endif
        break;
    case SENSOR_NAME_S5KGM1SP:
        newEEPRom = new ExynosCameraEEPRomMapGM1SP(cameraId);
        break;
    case SENSOR_NAME_S5K2X5SP:
        newEEPRom = new ExynosCameraEEPRomMap2X5SP(cameraId);
        break;
	case SENSOR_NAME_OV12A10:
	     newEEPRom = new ExynosCameraEEPRomMapOV12A10(cameraId);
        break;
	case SENSOR_NAME_OV12A10FF:
		newEEPRom = new ExynosCameraEEPRomMapOV12A10FF(cameraId);
		break;
	case SENSOR_NAME_OV16885C:
		newEEPRom = new ExynosCameraEEPRomMapOV16885C(cameraId);
		break;

    default:
        newEEPRom = new ExynosCameraEEPRomMapDefault(cameraId);
        CLOGE("Unknown sensorId %d. just use ExynosCameraEEPRomMapDefault", sensorId);
        break;
    }

    if (newEEPRom != NULL) {
        CLOGD("new %s(cameraId : %d, sensorId : %d)", newEEPRom->getName(), cameraId, sensorId);
    }

    ////////////////////////////////////////////////

    return newEEPRom;
}
