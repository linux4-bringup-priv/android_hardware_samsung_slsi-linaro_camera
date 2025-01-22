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
#define LOG_TAG "ExynosCameraSensorListenerWrapper"

#include "ExynosCameraSensorListenerWrapper.h"

ExynosCameraSensorListenerWrapper::SensorListenerHandle_t ExynosCameraSensorListenerWrapper::load(int cameraId)
{
    CLOGD2("%s() start", __FUNCTION__);

    ////////////////////////////////////////////////
    // initialize
    status_t ret = NO_ERROR;
    int m_cameraId = cameraId;
    char             m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    strncpy(m_name, __FUNCTION__, EXYNOS_CAMERA_NAME_STR_SIZE);

    ////////////////////////////////////////////////
    // new listener
    //ExynosCameraSensorListener* listener = new ExynosCameraSensorListener(cameraId);
    //ExynosCameraSensorListener* listener = new ExynosCameraSensorListenerDummy(cameraId);
    ExynosCameraSensorListener* listener = new ExynosCameraSensorListenerASensor(cameraId);

    ////////////////////////////////////////////////
    // create
    ret = listener->create();
    if (ret != NO_ERROR) {
        CLOGE2("listener->create() fail!");
        SAFE_DELETE(listener);
    }

    ////////////////////////////////////////////////

    CLOGD2("%s() end (handler %p)", __FUNCTION__, listener);

    return reinterpret_cast<ExynosCameraSensorListenerWrapper::SensorListenerHandle_t>(listener);
}

int ExynosCameraSensorListenerWrapper::unload(ExynosCameraSensorListenerWrapper::SensorListenerHandle_t *handle)
{
    CLOGD2("%s() start", __FUNCTION__);

    int ret = NO_ERROR;

    if (*handle) {
        ExynosCameraSensorListener* listener;
        listener = reinterpret_cast<ExynosCameraSensorListener*>(*handle);
        *handle = NULL;

        CLOGD2("sensorlistener with handler %p will be deleted!", listener);
        listener->destroy();
        SAFE_DELETE(listener);

    } else {
        CLOGE2("*handle == NULL. so, fail");
        ret = INVALID_OPERATION;
    }

    CLOGD2("%s() end", __FUNCTION__);

    return ret;
}

int ExynosCameraSensorListenerWrapper::enable_sensor(ExynosCameraSensorListenerWrapper::SensorListenerHandle_t handle, SensorType_t sensor_type, int samplingTimeMsec)
{
    CLOGD2("%s(sensor_type : %d, samplingTimeMsec : %d) start", __FUNCTION__, sensor_type, samplingTimeMsec);

    int ret = NO_ERROR;

    ExynosCameraSensorListener* listener = reinterpret_cast<ExynosCameraSensorListener*>(handle);

    if (listener) {
        CLOGD2("Enable: sensorlistener with handler %p!", listener);

        int sensorType = ExynosCameraSensorListenerWrapper::m_sensorType_t2SENSOR_TYPE(sensor_type);
        if (sensorType < 0) {
            CLOGE2("Invalid sensor_type(%d). so, fail", sensor_type);
            ret = INVALID_OPERATION;
            goto done;
        }

        ret = listener->start(sensorType, samplingTimeMsec);
        if (ret != NO_ERROR) {
            CLOGE2("listener->start(sensorType(%d), samplingTimeMsec(%d)) fail", sensorType, samplingTimeMsec);
        }
    } else {
        CLOGE2("listener == NULL. so, fail");
        ret = INVALID_OPERATION;
    }

done:
    CLOGD2("%s(sensor_type : %d, samplingTimeMsec : %d) end", __FUNCTION__, sensor_type, samplingTimeMsec);

    return ret;
}

int ExynosCameraSensorListenerWrapper::disable_sensor(ExynosCameraSensorListenerWrapper::SensorListenerHandle_t handle, SensorType_t sensor_type)
{
    CLOGD2("%s(sensor_type : %d) start", __FUNCTION__, sensor_type);

    int ret = NO_ERROR;

    ExynosCameraSensorListener* listener = reinterpret_cast<ExynosCameraSensorListener*>(handle);

    if (listener) {
        CLOGD2("Disable: sensorlistener with handler %p!", listener);

        int sensorType = ExynosCameraSensorListenerWrapper::m_sensorType_t2SENSOR_TYPE(sensor_type);
        if (sensorType < 0) {
            CLOGE2("Invalid sensor_type(%d). so, fail", sensor_type);
            ret = INVALID_OPERATION;
            goto done;
        }

        ret = listener->stop(sensorType);
        if (ret != NO_ERROR) {
            CLOGE2("listener->stop(sensorType(%d)) fail", sensorType);
        }
    } else {
        CLOGE2("listener == NULL. so, fail");
        ret = INVALID_OPERATION;
    }

done:
    CLOGD2("%s(sensor_type : %d) end", __FUNCTION__, sensor_type);

    return ret;
}

int ExynosCameraSensorListenerWrapper::get_data(ExynosCameraSensorListenerWrapper::SensorListenerHandle_t handle, SensorType_t sensor_type, ExynosCameraSensorListener::Event_t *event, bool userFlag, ExynosCameraSensorListenerWrapper::EVENT_TYPE eventType)
{
    //CLOGD2("%s(handle : %p, sensor_type : %d) start", __FUNCTION__, handle, sensor_type);

    int ret = NO_ERROR;

    ExynosCameraSensorListener* listener = reinterpret_cast<ExynosCameraSensorListener*>(handle);

    if (listener) {
        ////////////////////////////////////////////////
        // change SensorType_t to SENSOR_TYPE
        int sensorType = ExynosCameraSensorListenerWrapper::m_sensorType_t2SENSOR_TYPE(sensor_type);
        if (sensorType < 0) {
            CLOGE2("Invalid sensor_type(%d). so, fail", sensor_type);
            ret = INVALID_OPERATION;
            goto done;
        }

        ////////////////////////////////////////////////
        // get Event
        ExynosCameraSensorListener::EVENT_TYPE newEventType = ExynosCameraSensorListener::EVENT_TYPE_BASE;

        switch (eventType) {
        case ExynosCameraSensorListenerWrapper::EVENT_TYPE_OLDEST:
            newEventType = ExynosCameraSensorListener::EVENT_TYPE_OLDEST;
            break;
        default:
        case ExynosCameraSensorListenerWrapper::EVENT_TYPE_LATEST:
            newEventType = ExynosCameraSensorListener::EVENT_TYPE_LATEST;
            break;
        }

        ret = listener->getEvent(sensorType, event, newEventType);
        if (ret != NO_ERROR) {
            CLOGE2("listener->getEvent(sensorType(%d), newEventType(%d)) fail", sensorType, newEventType);
            goto done;
        }

        // do something, after event
        switch (sensorType) {
        case SENSOR_TYPE_ORIENTATION:
            m_orietation2Event(event->rotation.orientation, event);
            break;
        default:
            break;
        }

        ////////////////////////////////////////////////
    } else {
        CLOGE2("listener == NULL. so, fail");
        ret = INVALID_OPERATION;
    }

done:
    //CLOGD2("%s(handle : %p, sensor_type : %d) end", __FUNCTION__, handle, sensor_type);
    return ret;
}

int ExynosCameraSensorListenerWrapper::get_numberOfdata(ExynosCameraSensorListenerWrapper::SensorListenerHandle_t handle, SensorType_t sensor_type)
{
    //CLOGD2("%s(handle : %p, sensor_type : %d) start", __FUNCTION__, handle, sensor_type);

    int ret = 0;

    ExynosCameraSensorListener* listener = reinterpret_cast<ExynosCameraSensorListener*>(handle);

    if (listener) {
        ////////////////////////////////////////////////
        // change SensorType_t to SENSOR_TYPE
        int sensorType = ExynosCameraSensorListenerWrapper::m_sensorType_t2SENSOR_TYPE(sensor_type);
        if (sensorType < 0) {
            CLOGE2("Invalid sensor_type(%d). so, fail", sensor_type);
            ret = -1;
            goto done;
        }

        ////////////////////////////////////////////////
        // get Event
        ret = listener->getNumberOfEvent(sensorType);
        if (ret < 0) {
            CLOGE2("listener->getNumberOfEvent(sensorType(%d)) fail", sensorType);
            goto done;
        }

        ////////////////////////////////////////////////
    } else {
        CLOGE2("listener == NULL. so, fail");
        ret = -1;
    }

done:
    //CLOGD2("%s(handle : %p, sensor_type : %d) end", __FUNCTION__, handle, sensor_type);
    return ret;
}

void ExynosCameraSensorListenerWrapper::m_orietation2Event(int orientation, ExynosCameraSensorListener::Event_t *event)
{
    switch(orientation % 360) {
    case 0:
    case 360:
        event->rotation.orientation = ExynosCameraSensorListenerWrapper::SCREEN_DEGREES_0;
        break;
    case 90:
        event->rotation.orientation = ExynosCameraSensorListenerWrapper::SCREEN_DEGREES_90;
        break;
    case 180:
        event->rotation.orientation = ExynosCameraSensorListenerWrapper::SCREEN_DEGREES_180;
        break;
    case 270:
        event->rotation.orientation = ExynosCameraSensorListenerWrapper::SCREEN_DEGREES_270;
        break;
    default:
        CLOGW2("Invalid orientation(%d). so, fail", orientation);
        event->rotation.orientation = ExynosCameraSensorListenerWrapper::SCREEN_DEGREES_0;
        break;
    }
}

int ExynosCameraSensorListenerWrapper::m_sensorType_t2SENSOR_TYPE(SensorType_t sensor_type)
{
    int sensorType = 0;

    switch (sensor_type) {
    case ST_GYROSCOPE:
        sensorType = SENSOR_TYPE_GYROSCOPE;
        break;
    case ST_ACCELEROMETER:
        sensorType = SENSOR_TYPE_ACCELEROMETER;
        break;
    case ST_ROTATION:
        sensorType = SENSOR_TYPE_ORIENTATION;
        break;
    default:
        CLOGE2("Invalid sensor_type(%d). so, fail", sensor_type);
        sensorType = -1;
        break;
    }

    return sensorType;
}
