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
#define LOG_TAG "ExynosCameraActivityUCTL"
#include <log/log.h>

#include "ExynosCameraActivityUCTL.h"
//#include "ExynosCamera.h"

namespace android {

class ExynosCamera;

ExynosCameraActivityUCTL::ExynosCameraActivityUCTL(int cameraId,
                                                   cameraId_Info *camIdInfo,
                                                   struct ExynosCameraSensorInfoBase *staticInfo)
                                                   : ExynosCameraActivityBase(cameraId, camIdInfo, staticInfo)
{
    m_metadata = NULL;
    m_rotation = 0;
}

ExynosCameraActivityUCTL::~ExynosCameraActivityUCTL()
{
}

int ExynosCameraActivityUCTL::t_funcNull(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_funcSensorBefore(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_funcSensorAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_funcISPBefore(__unused void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[buf->getMetaPlaneIndex()]);

    if (shot_ext != NULL && m_metadata != NULL) {
#ifdef USE_GYRO_HISTORY_FOR_TNR
        memcpy(&(shot_ext->shot.uctl.aaUd.gyroHistoryInfo),
                &(m_metadata->shot.uctl.aaUd.gyroHistoryInfo),
                sizeof(struct camera2_gyro_sensor_history_info));
#endif
    }

    return 1;
}

int ExynosCameraActivityUCTL::t_funcISPAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_func3ABefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[buf->getMetaPlaneIndex()]);

    if (shot_ext != NULL) {
#ifdef FD_ROTATION
        shot_ext->shot.uctl.scalerUd.orientation = m_rotation;
#endif
    }

    return 1;
}

int ExynosCameraActivityUCTL::t_func3AAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_func3ABeforeHAL3(__unused void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[buf->getMetaPlaneIndex()]);

    if (shot_ext != NULL) {
        if (m_metadata != NULL) {
            memcpy(&(shot_ext->shot.uctl.aaUd.gyroInfo),
                &(m_metadata->shot.uctl.aaUd.gyroInfo),
                sizeof(struct camera2_gyro_sensor_info));

#ifdef USE_GYRO_HISTORY_FOR_TNR
            memcpy(&(shot_ext->shot.uctl.aaUd.gyroHistoryInfo),
                &(m_metadata->shot.uctl.aaUd.gyroHistoryInfo),
                sizeof(struct camera2_gyro_sensor_history_info));
#endif

            memcpy(&(shot_ext->shot.uctl.aaUd.accInfo),
                &(m_metadata->shot.uctl.aaUd.accInfo),
                sizeof(struct camera2_accelerometer_sensor_info));
        }

#ifdef FD_ROTATION
        shot_ext->shot.uctl.scalerUd.orientation = m_rotation;
#endif
    }

    return 1;
}

int ExynosCameraActivityUCTL::t_func3AAfterHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_funcVRABefore(__unused void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[buf->getMetaPlaneIndex()]);

    if (shot_ext != NULL) {
#ifdef FD_ROTATION
        shot_ext->shot.uctl.scalerUd.orientation = m_rotation;
#endif
        CLOGV("[F(%d) B%d]-IN-",
            shot_ext->shot.dm.request.frameCount, buf->index);
    }

    return 1;
}

int ExynosCameraActivityUCTL::t_funcVRAAfter(__unused void *args)
{
    CLOGV("");

    return 1;
}

void ExynosCameraActivityUCTL::setMetadata(struct camera2_shot_ext *metadata)
{
    if (metadata == NULL) {
        CLOGE("metadata is NULL");
        return;
    }

    m_metadata = metadata;
    return;
}

void ExynosCameraActivityUCTL::setDeviceRotation(int rotation)
{
    m_rotation = rotation;
}

int ExynosCameraActivityUCTL::getDeviceRotation(void)
{
    return m_rotation;
}
} /* namespace android */

