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
#define LOG_TAG "ExynosCameraSensorListenerDummy"

#include "ExynosCameraSensorListenerDummy.h"

/*** static declarations ***/
static const float RADIANS_2_DEG = 57.29577957855f;
static const int   ORIENTATION_90_THRESH = 50;
static const int   ORIENTATION_180_THRESH = 170;
static const int   ORIENTATION_270_THRESH = 250;

ExynosCameraSensorListenerDummy::~ExynosCameraSensorListenerDummy()
{
}

status_t ExynosCameraSensorListenerDummy::m_create(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_create() start");

    status_t ret = NO_ERROR;

    ExynosCameraCallback::Args args;
    args.set(ExynosCameraSensorListenerDummy::m_listenerCallbackFunc, this);

    ////////////////////////////////////////////////
    // set callback
    m_callback = new ExynosCameraCallback(getCameraId(), "ExynosCameraSensorListenerCallback");
    ret = m_callback->create();
    if (ret != 0) {
        CLOGE("m_callback->create() fail");
        goto done;
    }

    m_callback->setCallback(&args, ExynosCameraCallback::CALLBACK_TYPE_REPEAT);

done:
    ////////////////////////////////////////////////
    // error handling
    if (ret != NO_ERROR) {
        if (m_callback) {
            if (m_callback->flagCreated() == true) {
                m_callback->destroy();
            }
        }
    }

    ////////////////////////////////////////////////

    SENSOR_LISTENER_DEBUG_LOG("m_create() end");

    return ret;
}

void ExynosCameraSensorListenerDummy::m_destroy(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_destroy() start");

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // destroy callback
    if (m_callback) {
        if (m_callback->flagCreated() == true) {
            m_callback->destroy();
        }
    }

    ////////////////////////////////////////////////

    SENSOR_LISTENER_DEBUG_LOG("m_destroy() end");
}

status_t ExynosCameraSensorListenerDummy::m_start(int sensorType, int samplingTimeMsec)
{
    SENSOR_LISTENER_DEBUG_LOG("m_start(sensorType : %d, samplingTimeMsec : %d) start ", sensorType, samplingTimeMsec);

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // activate sensor

    ////////////////////////////////////////////////
    // start callback
    ret = m_callback->start();
    if (ret != NO_ERROR) {
        CLOGE("m_callback->start() fail");
        goto done;
    }

    ////////////////////////////////////////////////

done:
    SENSOR_LISTENER_DEBUG_LOG("m_start(sensorType : %d) end", sensorType);

    return ret;
}

status_t ExynosCameraSensorListenerDummy::m_stop(int sensorType)
{
    SENSOR_LISTENER_DEBUG_LOG("m_stop(sensorType : %d) start", sensorType);

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // deactivate sensor

    ////////////////////////////////////////////////
    // stop callback
    m_callback->stop();

    ////////////////////////////////////////////////

    SENSOR_LISTENER_DEBUG_LOG("m_stop(sensorType : %d) end", sensorType);

    return ret;
}

void ExynosCameraSensorListenerDummy::m_init(void)
{
    ////////////////////////////////////////////////
    // initialize here

    this->setName("ExynosCameraSensorListenerDummy");

    ////////////////////////////////////////////////
}

void ExynosCameraSensorListenerDummy::m_listenerCallbackFunc(void *data0, void *data1)
{
    ExynosCameraSensorListener *listener = (ExynosCameraSensorListener *)data0;

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // rotation angle every 3sec.
    static int count = 0;
    static int orientation = 0;

    count++;
    if (count % 100 == 0) {
        orientation += 90;
    }

    unsigned int sensorTypeBit = listener->m_getSensorTypeBit();

    for (int i = 0; i < SUPPORED_NUM_OF_SENSOR; i++) {
        if (sensorTypeBit & (1 << i)) {
            switch (i) {
            case SENSOR_TYPE_ORIENTATION:
            case SENSOR_TYPE_ACCELEROMETER:
                ExynosCameraSensorListener::Event_t event;
                event.rotation.orientation = orientation;

                listener->setEvent(i, &event);
                break;
            default:
                break;
            }
        }
    }

    usleep(33000); // 33msec
}
