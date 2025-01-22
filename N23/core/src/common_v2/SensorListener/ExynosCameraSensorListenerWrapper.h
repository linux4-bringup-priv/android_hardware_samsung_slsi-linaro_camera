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

/*!
 * \file      ExynosCameraSensorListenerWrapper.h
 * \brief     header file for ExynosCameraSensorListenerWrapper
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/12/17
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_SENSOR_LISTENER_WRAPPER_H
#define EXYNOS_CAMERA_SENSOR_LISTENER_WRAPPER_H

#include <sys/types.h>
#include <log/log.h>
#include <hardware/sensors.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"
#include "ExynosCameraUtils.h"

#include "ExynosCameraSensorListenerDummy.h"
#include "ExynosCameraSensorListenerASensor.h"

using namespace android;

class ExynosCameraSensorListenerWrapper {
public:
    typedef void* SensorListenerHandle_t;

    typedef enum SensorType
    {
        ST_INVALID                       = 0,
        ST_GYROSCOPE                     = 1 << 1,
        ST_GYROSCOPE_UNCALIBRATED        = 1 << 2,
        ST_ACCELEROMETER                 = 1 << 3,
        ST_LINEAR_ACCELERATION           = 1 << 4,
        ST_ACCELEROMETER_UNCALIBRATED    = 1 << 5,
        ST_ROTATION                      = 1 << 6,
    } SensorType_t;

    enum EVENT_TYPE
    {
        EVENT_TYPE_BASE,
        EVENT_TYPE_OLDEST,
        EVENT_TYPE_LATEST,
        EVENT_TYPE_MAX,
    };

    enum SCREEN_DEGREE {
        SCREEN_DEGREES_0,
        SCREEN_DEGREES_90,
        SCREEN_DEGREES_270,
        SCREEN_DEGREES_180,
    };

public:
    static SensorListenerHandle_t load(int cameraId);
    static int                    unload(SensorListenerHandle_t *handle);
    static int                    enable_sensor(SensorListenerHandle_t handle, SensorType_t sensor_type, int sampling_timeMsec);
    static int                    disable_sensor(SensorListenerHandle_t handle, SensorType_t sensor_type);
    static int                    get_data(SensorListenerHandle_t handle, SensorType_t sensorType, ExynosCameraSensorListener::Event_t *event, bool userFlag, ExynosCameraSensorListenerWrapper::EVENT_TYPE eventType = EVENT_TYPE_LATEST);
    static int                    get_numberOfdata(SensorListenerHandle_t handle, SensorType_t sensorType);

private:
    static int                    m_sensorType_t2SENSOR_TYPE(SensorType_t sensor_type);
    static void                   m_orietation2Event(int orientation, ExynosCameraSensorListener::Event_t *event);

};

#endif /* EXYNOS_CAMERA_SENSOR_LISTENER_WRAPPER_H */
