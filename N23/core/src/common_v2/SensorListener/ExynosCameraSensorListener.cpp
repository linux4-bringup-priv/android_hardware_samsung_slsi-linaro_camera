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
#define LOG_TAG "ExynosCameraSensorListener"

#include "ExynosCameraSensorListener.h"

/*** static declarations ***/
static const float RADIANS_2_DEG = 57.29577957855f;
static const int   ORIENTATION_0_THRESH   = 315;
static const int   ORIENTATION_90_THRESH  = 45;
static const int   ORIENTATION_180_THRESH = 135;
static const int   ORIENTATION_270_THRESH = 225;

ExynosCameraSensorListener::~ExynosCameraSensorListener()
{
}

status_t ExynosCameraSensorListener::create(void)
{
    CLOGD("create() start");

    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == true) {
        CLOGE("It is already created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // init.
    m_sensorTypeBit = 0;

    for (int i = 0; i < SUPPORED_NUM_OF_SENSOR; i++) {
        m_samplingTimeMsec[i] = DEFAULT_SAMPLING_TIME_MSEC;
        memset(&m_event[i], 0, sizeof(ExynosCameraSensorListener::Event_t) * SUPPORED_NUM_OF_EVENT);

        memset(&m_eventSize[i],     0, sizeof(int));
        memset(&m_eventOldIndex[i], 0, sizeof(int));
        memset(&m_eventNewIndex[i], 0, sizeof(int));
    }

    ////////////////////////////////////////////////
    //
    ret = this->m_create();
    if (ret != NO_ERROR) {
        CLOGE("m_create() fail");
        goto done;
    }

    m_flagCreate = true;

    ////////////////////////////////////////////////
done:
    CLOGD("create() end");

    return ret;
}

void ExynosCameraSensorListener::destroy(void)
{
    CLOGD("destroy() start");

    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        goto done;
    }

    ////////////////////////////////////////////////
    //
    this->m_destroy();

    ////////////////////////////////////////////////

    m_flagCreate = false;

 done:
    CLOGD("destroy() end");
}

bool ExynosCameraSensorListener::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

status_t ExynosCameraSensorListener::start(int sensorType, int samplingTimeMsec)
{
    CLOGD("start(sensorType(%d:%s), samplingTimeMsec(%d)) start",
        sensorType,
        m_getSensorName(sensorType),
        samplingTimeMsec);

    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    if (samplingTimeMsec < 0) {
        CLOGW("samplingTimeMsec(%d) < 0. so it changed to DEFAULT_SAMPLING_TIME_MSEC", samplingTimeMsec);
        samplingTimeMsec = DEFAULT_SAMPLING_TIME_MSEC;
    }

    ////////////////////////////////////////////////
    // set sampleRate
    if (m_sensorTypeBit & (1 << sensorType)) {
        int oldSampleTimeMec = m_getSampleTimeMsec(sensorType);

        if (oldSampleTimeMec != samplingTimeMsec) {
            CLOGW("sensorType(%d:%s) is already started. so, ignore this %d msec after start()",
                sensorType,
                m_getSensorName(sensorType),
                samplingTimeMsec);
            goto done;
        }
    } else {
        m_setSampleTimeMsec(sensorType, samplingTimeMsec);
    }

    ////////////////////////////////////////////////
    // turn on sensor bit
    m_sensorTypeBit |= (1 << sensorType);

    ////////////////////////////////////////////////
    // start sensor
    ret = this->m_start(sensorType, samplingTimeMsec);
    if (ret != NO_ERROR) {
        CLOGE("this->m_start() fail");
        goto done;
    }

    ////////////////////////////////////////////////
done:
    CLOGD("start(sensorType(%d:%s), samplingTimeMsec(%d) end",
        sensorType,
        m_getSensorName(sensorType),
        samplingTimeMsec);

    return ret;
}

status_t ExynosCameraSensorListener::stop(int sensorType)
{
    CLOGD("stop(sensorType(%d:%s) start", sensorType, m_getSensorName(sensorType));

    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // check sensor is off or not
    if ((m_sensorTypeBit & (1 << sensorType)) == 0) {
        CLOGW("sensorType(%d:%s) is already stopped", sensorType, m_getSensorName(sensorType));
        goto done;
    }

    ////////////////////////////////////////////////
    // stop sensor
    ret = this->m_stop(sensorType);
    if (ret != NO_ERROR) {
        CLOGE("this->m_stop(sensorType(%d:%s)) fail", sensorType, m_getSensorName(sensorType));
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // turn off sensor bit
    m_sensorTypeBit = (m_sensorTypeBit & ~(1 << sensorType));

    ////////////////////////////////////////////////
done:
    CLOGD("stop(sensorType(%d:%s)) end", sensorType, m_getSensorName(sensorType));

    return ret;
}

status_t ExynosCameraSensorListener::setEvent(int sensorType, ExynosCameraSensorListener::Event_t *event)
{
    //CLOGD("setEvent(sensorType(%d:%s)) start", sensorType, m_getSensorName(sensorType));

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    //
    ret = this->m_setEvent(sensorType, event);

done:
    //CLOGD("setEvent(sensorType(%d:%s)) end", sensorType, m_getSensorName(sensorType));

    return ret;
}

status_t ExynosCameraSensorListener::getEvent(int sensorType, ExynosCameraSensorListener::Event_t *event, ExynosCameraSensorListener::EVENT_TYPE eventType)
{
    //CLOGD("getEvent(sensorType(%d:%s), eventType(%d)) start", sensorType, m_getSensorName(sensorType), eventType);

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    //
    ret = this->m_getEvent(sensorType, event, eventType);

done:
    //CLOGD("getEvent(sensorType(%d:%s)) end", sensorType, m_getSensorName(sensorType));

    return ret;
}

int ExynosCameraSensorListener::getNumberOfEvent(int sensorType)
{
    int ret = 0;

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    ret = m_getNumberOfEvent(sensorType);

done:
    //CLOGD("getEvent(sensorType(%d:%s)) end. ret(%d)", sensorType, m_getSensorName(sensorType), ret);

    return ret;
}

char const* ExynosCameraSensorListener::m_getSensorName(int sensorType)
{
    switch (sensorType) {
    case SENSOR_TYPE_ACCELEROMETER:
        return "Acc";
    case SENSOR_TYPE_MAGNETIC_FIELD:
        return "Mag";
    case SENSOR_TYPE_ORIENTATION:
        return "Ori";
    case SENSOR_TYPE_GYROSCOPE:
        return "Gyr";
    case SENSOR_TYPE_LIGHT:
        return "Lux";
    case SENSOR_TYPE_PRESSURE:
        return "Bar";
    case SENSOR_TYPE_TEMPERATURE:
        return "Tmp";
    case SENSOR_TYPE_PROXIMITY:
        return "Prx";
    case SENSOR_TYPE_GRAVITY:
        return "Grv";
    case SENSOR_TYPE_LINEAR_ACCELERATION:
        return "Lac";
    case SENSOR_TYPE_ROTATION_VECTOR:
        return "Rot";
    case SENSOR_TYPE_RELATIVE_HUMIDITY:
        return "Hum";
    case SENSOR_TYPE_AMBIENT_TEMPERATURE:
        return "Tam";
    }

    return "ukn";
}

int ExynosCameraSensorListener::m_axis2Orientation(float azimuth, float pitch, float roll)
{
    float x = azimuth;
    float y = pitch;
    float z = roll;
    int orientation = 0;

    ////////////////////////////////////////////////
    // calibrate value.
    float magnitude = x * x + y * y;

    if (magnitude * 4 >= z * z) {
        float angle = (float)atan2f(-y, x) * RADIANS_2_DEG;

        // normalize to 0 - 359 range
        //orientation = 90 - (int)angle;
        orientation = 270 - (int)angle;

        if (orientation < 0) {
            orientation += 360;
        }

        orientation %= 360;

        int newOrientation = 0;

        if (ORIENTATION_270_THRESH <= orientation && orientation < ORIENTATION_0_THRESH) {
            newOrientation = 270;
        } else if (ORIENTATION_180_THRESH <= orientation && orientation < ORIENTATION_270_THRESH) {
            newOrientation = 180;
        } else if (ORIENTATION_90_THRESH <= orientation && orientation < ORIENTATION_180_THRESH) {
            newOrientation = 90;
        } else {
            newOrientation = 0;
        }

        //ALOGD("angle(%.1f)-> orientation(%d) -> newOrientation(%d)", angle, orientation, newOrientation);

        orientation = newOrientation;
    }

    ////////////////////////////////////////////////

    return orientation;
}

status_t ExynosCameraSensorListener::m_create(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_create() start");

    status_t ret = NO_ERROR;

    SENSOR_LISTENER_DEBUG_LOG("m_create() end");

    return ret;
}

void ExynosCameraSensorListener::m_destroy(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_destroy() start");

    status_t ret = NO_ERROR;

    SENSOR_LISTENER_DEBUG_LOG("m_destroy() end");
}

bool ExynosCameraSensorListener::m_flagCreated(void)
{
    return m_flagCreate;
}

status_t ExynosCameraSensorListener::m_start(int sensorType, int samplingTimeMsec)
{
    SENSOR_LISTENER_DEBUG_LOG("m_start(sensorType(%s:%d), samplingTimeMsec(%d)) start ",
        sensorType,
        m_getSensorName(sensorType),
        samplingTimeMsec);

    status_t ret = NO_ERROR;

    SENSOR_LISTENER_DEBUG_LOG("m_start(sensorType(%s:%d)) end", sensorType, m_getSensorName(sensorType));

    return ret;
}

status_t ExynosCameraSensorListener::m_stop(int sensorType)
{
    SENSOR_LISTENER_DEBUG_LOG("m_stop(sensorType(%s:%d)) start", sensorType, m_getSensorName(sensorType));

    status_t ret = NO_ERROR;

    return ret;
}

status_t ExynosCameraSensorListener::m_setEvent(int sensorType, ExynosCameraSensorListener::Event_t *event)
{
    if (sensorType < 0 || SUPPORED_NUM_OF_SENSOR <= sensorType) {
        CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s). so fail",
            sensorType,
            m_getSensorName(sensorType),
            SUPPORED_NUM_OF_SENSOR,
            sensorType,
            m_getSensorName(sensorType));

            return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // lock it
    Mutex::Autolock lock(m_eventLock[sensorType]);

    ////////////////////////////////////////////////
    // set TimeStamp
    event->timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    ////////////////////////////////////////////////
    // increase the new event index
    if (SUPPORED_NUM_OF_EVENT - 1 <= m_eventNewIndex[sensorType]) {
        m_eventNewIndex[sensorType] = 0;
    } else {
        m_eventNewIndex[sensorType]++;
    }

    ////////////////////////////////////////////////
    // increase size
    if (m_eventSize[sensorType] < SUPPORED_NUM_OF_EVENT) {
        m_eventSize[sensorType]++;
    } else {
        ////////////////////////////////////////////////
        // it size is full, increase the old event index (== drop the oldest index)
        if (SUPPORED_NUM_OF_EVENT - 1 <= m_eventOldIndex[sensorType]) {
            m_eventOldIndex[sensorType] = 0;
        } else {
            m_eventOldIndex[sensorType]++;
        }
    }

    ////////////////////////////////////////////////
    // memcpy on eventQ;
    memcpy(&m_event[sensorType][m_eventNewIndex[sensorType]], event, sizeof(ExynosCameraSensorListener::Event_t));

    /*
    SENSOR_LISTENER_DEBUG_LOG("%s set m_eventNewIndex(%d) m_eventOldIndex(%d) in size(%d)",
        m_getSensorName(sensorType),
        m_eventNewIndex[sensorType],
        m_eventOldIndex[sensorType],
        m_eventSize[sensorType]);
    */

    return NO_ERROR;
}

status_t ExynosCameraSensorListener::m_getEvent(int sensorType, ExynosCameraSensorListener::Event_t *event, ExynosCameraSensorListener::EVENT_TYPE eventType)
{
    if (sensorType < 0 || SUPPORED_NUM_OF_SENSOR <= sensorType) {
        CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s). so fail",
            sensorType,
            m_getSensorName(sensorType),
            SUPPORED_NUM_OF_SENSOR,
            sensorType,
            m_getSensorName(sensorType));

            return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // lock it
    Mutex::Autolock lock(m_eventLock[sensorType]);

    ////////////////////////////////////////////////
    // decrease size
    switch (eventType) {
    case EVENT_TYPE_OLDEST:
        if (0 < m_eventSize[sensorType]) {
            m_eventSize[sensorType]--;

            ////////////////////////////////////////////////
            // decrease the old event index
            if (SUPPORED_NUM_OF_EVENT - 1 <= m_eventOldIndex[sensorType]) {
                m_eventOldIndex[sensorType] = 0;
            } else {
                m_eventOldIndex[sensorType]++;
            }
        }
        break;
    default:
    case EVENT_TYPE_LATEST:
        if (0 < m_eventSize[sensorType]) {
            m_eventSize[sensorType] = 0;
            m_eventOldIndex[sensorType] = m_eventNewIndex[sensorType];
        }
        break;
    }

    ////////////////////////////////////////////////
    // memcpy on eventQ;
    memcpy(event, &m_event[sensorType][m_eventOldIndex[sensorType]], sizeof(ExynosCameraSensorListener::Event_t));

    /*
    SENSOR_LISTENER_DEBUG_LOG("%s get m_eventNewIndex(%d) m_eventOldIndex(%d) in size(%d)",
        m_getSensorName(sensorType),
        m_eventNewIndex[sensorType],
        m_eventOldIndex[sensorType],
        m_eventSize[sensorType]);
    */

    return NO_ERROR;
}

int ExynosCameraSensorListener::m_getNumberOfEvent(int sensorType)
{
    if (sensorType < 0 || SUPPORED_NUM_OF_SENSOR <= sensorType) {
        CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s). so fail",
            sensorType,
            m_getSensorName(sensorType),
            SUPPORED_NUM_OF_SENSOR,
            sensorType,
            m_getSensorName(sensorType));

            return -1;
    }

    ////////////////////////////////////////////////
    // lock it
    Mutex::Autolock lock(m_eventLock[sensorType]);

    /*
    SENSOR_LISTENER_DEBUG_LOG("%s size m_eventNewIndex(%d) m_eventOldIndex(%d) in size(%d)",
        m_getSensorName(sensorType),
        m_eventNewIndex[sensorType],
        m_eventOldIndex[sensorType],
        m_eventSize[sensorType]);
    */

    return m_eventSize[sensorType];
}

void ExynosCameraSensorListener::m_init(void)
{
    ////////////////////////////////////////////////
    // initialize here
    m_flagCreate = false;

    m_sensorTypeBit = 0;

    for (int i = 0; i < SUPPORED_NUM_OF_SENSOR; i++) {
        m_samplingTimeMsec[i] = DEFAULT_SAMPLING_TIME_MSEC;
        memset(&m_event[i], 0, sizeof(ExynosCameraSensorListener::Event_t) * SUPPORED_NUM_OF_EVENT);

        memset(&m_eventSize[i],     0, sizeof(int));
        memset(&m_eventOldIndex[i], 0, sizeof(int));
        memset(&m_eventNewIndex[i], 0, sizeof(int));
    }

    this->setName("ExynosCameraSensorListener");

    ////////////////////////////////////////////////
}

int ExynosCameraSensorListener::m_getSensorTypeBit(void)
{
    return m_sensorTypeBit;
}

void ExynosCameraSensorListener::m_setSampleTimeMsec(int sensorType, int samplingTimeMsec)
{
    if (sensorType < 0 || SUPPORED_NUM_OF_SENSOR <= sensorType) {
        CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s) to set %d msec. so fail",
            sensorType,
            m_getSensorName(sensorType),
            SUPPORED_NUM_OF_SENSOR,
            sensorType,
            m_getSensorName(sensorType),
            samplingTimeMsec);
    } else if (samplingTimeMsec < 0) {
            CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s) to set %d msec. so fail",
                sensorType,
                m_getSensorName(sensorType),
                SUPPORED_NUM_OF_SENSOR,
                sensorType,
                m_getSensorName(sensorType),
                DEFAULT_SAMPLING_TIME_MSEC);
            m_samplingTimeMsec[sensorType] = DEFAULT_SAMPLING_TIME_MSEC;
    } else {
        m_samplingTimeMsec[sensorType] = samplingTimeMsec;
    }
}

int ExynosCameraSensorListener::m_getSampleTimeMsec(int sensorType)
{
    if (sensorType < 0 || SUPPORED_NUM_OF_SENSOR <= sensorType) {
        CLOGE("sensorType(%d, %s) < 0 || SUPPORED_NUM_OF_SENSOR(%d) <= sensorType(%d, %s). so fail. just return DEFAULT_SAMPLING_TIME_MSEC(%d)",
            sensorType,
            m_getSensorName(sensorType),
            SUPPORED_NUM_OF_SENSOR,
            sensorType,
            m_getSensorName(sensorType),
            DEFAULT_SAMPLING_TIME_MSEC);

        return DEFAULT_SAMPLING_TIME_MSEC;
    }

    return m_samplingTimeMsec[sensorType];
}
