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
#define LOG_TAG "ExynosCameraSensorListenerASensor"

#include "ExynosCameraSensorListenerASensor.h"

ExynosCameraSensorListenerASensor::~ExynosCameraSensorListenerASensor()
{
}

status_t ExynosCameraSensorListenerASensor::m_create(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_create() start");

    status_t ret = NO_ERROR;

    m_eventQueue       = NULL;
    m_looper           = NULL;
    m_thread           = NULL;
    m_flagThreadRun = false;

    ////////////////////////////////////////////////
    // open sensor hw
    /*
    ASensorManager* mgr = ASensorManager_getInstanceForPackage(NULL);
    if (!mgr) {
        CLOGD("Failed to get sensor manager");
        return INVALID_OPERATION;
    }

    ASensorList aSensorList;
    nt          numOfSensor = 0;

    numOfSensor = ASensorManager_getSensorList(mgr, &aSensorList);
    CLOGD("numSensors=%d", numOfSensor);

    for (int i = 0; i < numOfSensor; i++) {
        CLOGD("sensor list: %s", ASensor_getName(aSensorList[i]));
        CLOGD("sensor type: %d", ASensor_getType(aSensorList[i]));
    }
    */

    ////////////////////////////////////////////////

    SENSOR_LISTENER_DEBUG_LOG("m_create() end");

    return ret;
}

void ExynosCameraSensorListenerASensor::m_destroy(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_destroy() start");

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // sensor close

    ////////////////////////////////////////////////

    SENSOR_LISTENER_DEBUG_LOG("m_destroy() end");
}

status_t ExynosCameraSensorListenerASensor::m_start(int sensorType, int samplingTimeMsec)
{
    SENSOR_LISTENER_DEBUG_LOG("m_start(sensorType : %d, samplingTimeMsec : %d) start ", sensorType, samplingTimeMsec);

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // start sensor
    ret = m_startLoop();
    if (ret != NO_ERROR) {
        CLOGE("m_startLoop() fail");
        ret = 0;
    }

//done:
    ////////////////////////////////////////////////
    SENSOR_LISTENER_DEBUG_LOG("m_start() end");

    return ret;
}

status_t ExynosCameraSensorListenerASensor::m_stop(int sensorType)
{
    SENSOR_LISTENER_DEBUG_LOG("m_stop(sensorType : %d) start", sensorType);

    status_t ret = NO_ERROR;

    unsigned int sensorTypeBit = m_getSensorTypeBit();

    ////////////////////////////////////////////////
    // deactivate sensor
    ASensorManager* mgr = ASensorManager_getInstanceForPackage(NULL);
    ASensor const* sensor;

    int hwSensorType = sensorType;

    // ORIENTATION works by ACCELEROMETER
    if (sensorType == SENSOR_TYPE_ORIENTATION) {
        hwSensorType = SENSOR_TYPE_ACCELEROMETER;
    }

    sensor = ASensorManager_getDefaultSensor(mgr, hwSensorType);
    if (sensor == NULL) {
        CLOGE("ASensorManager_getDefaultSensor(sensorType(%d:%s) -> hwSensorType(%d:%s)) fail",
            sensorType, m_getSensorName(sensorType),
            hwSensorType, m_getSensorName(hwSensorType));

        ret = INVALID_OPERATION;
        goto done;
    }

    ASensorEventQueue_disableSensor(m_eventQueue, sensor);

    ////////////////////////////////////////////////
    // stop Loop
    sensorTypeBit = (sensorTypeBit & ~(1 << sensorType));

    if (sensorTypeBit == 0) {
        ret = m_stopLoop();
        if (ret != NO_ERROR) {
            CLOGE("m_stopLoop() fail");
            ret = NO_ERROR;
        }
    }

done:
    ////////////////////////////////////////////////
    SENSOR_LISTENER_DEBUG_LOG("m_stop(sensorType : %d) end", sensorType);

    return ret;
}

void ExynosCameraSensorListenerASensor::m_init(void)
{
    ////////////////////////////////////////////////
    // initialize here
    m_eventQueue    = NULL;
    m_looper        = NULL;
    m_thread        = NULL;
    m_flagThreadRun = false;
    m_lastTs        = 0LL;

    this->setName("ExynosCameraSensorListenerASensor");
    ////////////////////////////////////////////////
}

status_t ExynosCameraSensorListenerASensor::m_startLoop(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_startLoop() start");

    status_t ret = NO_ERROR;

    if (m_thread.get() != NULL) {
        goto out;
    }

    m_thread = new SensorListenerThread(this);
    if (m_thread.get() == NULL) {
        CLOGE("Couldn't create sensor looper thread");
        ret = NO_MEMORY;
        goto out;
    }

    ret = m_thread->run("sensor_looper_thread", PRIORITY_DEFAULT);
    if (ret == INVALID_OPERATION) {
        CLOGD("thread already running ?!?");
    } else if (ret != NO_ERROR) {
        CLOGE("couldn't run thread");
        goto out;
    }

out:
    SENSOR_LISTENER_DEBUG_LOG("m_startLoop() end");

    return ret;
}

status_t ExynosCameraSensorListenerASensor::m_stopLoop(void)
{
    SENSOR_LISTENER_DEBUG_LOG("m_stopLoop() start");

    status_t ret = NO_ERROR;

    if (m_thread.get()) {
        m_thread->requestExit();
        ALooper_wake(m_looper);
        m_thread->join();
        m_thread.clear();
        m_thread = NULL;

        ASensorManager* mgr = ASensorManager_getInstanceForPackage(NULL);
        ASensorManager_destroyEventQueue(mgr, m_eventQueue);
        m_eventQueue = NULL;

        m_flagThreadRun = false;
    }

    SENSOR_LISTENER_DEBUG_LOG("m_stopLoop() end");

    return ret;
}

//static int sensor_events_listener(int fd, int numOfEvents, void* data)
int ExynosCameraSensorListenerASensor::m_sensorEventListener(int fd, int numOfEvents, void* data)
{
    ExynosCameraSensorListenerASensor *listener = reinterpret_cast<ExynosCameraSensorListenerASensor*>(data);
    ASensorEventQueue*    event_queue = listener->m_getSensorEventQueue();
    ExynosCameraSensorListener::Event_t event;
    ssize_t               num_sensors;
    ASensorEvent          sensorEvents[8];
    int orientation       = 0;
    unsigned int sensoTypeBit = 0;

    while ((num_sensors = ASensorEventQueue_getEvents(event_queue, sensorEvents, sizeof(sensorEvents)/sizeof(ASensorEvent))) > 0) {
        for (int i = 0; i < num_sensors; i++) {

            switch (sensorEvents[i].type) {
            case ASENSOR_TYPE_GYROSCOPE:
                event.gyro.x = sensorEvents[i].data[0];
                event.gyro.y = sensorEvents[i].data[1];
                event.gyro.z = sensorEvents[i].data[2];

                listener->setEvent(sensorEvents[i].type, &event);

                /*
                CLOGD2("gyro vector(%.3f, %.3f, %.3f) -> event.gyro(%.3f, %.3f, %.3f)",
                    sensorEvents[i].vector.x,
                    sensorEvents[i].vector.y,
                    sensorEvents[i].vector.z,
                    event.gyro.x,
                    event.gyro.y,
                    event.gyro.z);
                */
                break;
            case ASENSOR_TYPE_ACCELEROMETER:
            case ASENSOR_TYPE_ORIENTATION: // it is same with ASENSOR_TYPE_ACCELEROMETER:
                sensoTypeBit = listener->m_getSensorTypeBit();

                if (sensoTypeBit & (1 << SENSOR_TYPE_ACCELEROMETER)) {
                    event.accel.x = sensorEvents[i].data[0];
                    event.accel.y = sensorEvents[i].data[1];
                    event.accel.z = sensorEvents[i].data[2];

                    listener->setEvent(sensorEvents[i].type, &event);

                    /*
                    CLOGD2("accelerometer vector(%.3f, %.3f, %.3f) -> event.accelerometer(%.3f, %.3f, %.3f)",
                        sensorEvents[i].vector.x,
                        sensorEvents[i].vector.y,
                        sensorEvents[i].vector.z,
                        event.accel.x,
                        event.accel.y,
                        event.accel.z);
                    */
                } else {
                    orientation = ExynosCameraSensorListener::m_axis2Orientation(sensorEvents[i].data[0],
                        sensorEvents[i].data[1],
                        sensorEvents[i].data[2]);

                    event.rotation.orientation = orientation;

                    // ORIENTATION works by ACCELEROMETER
                    sensorEvents[i].type = ASENSOR_TYPE_ORIENTATION;

                    listener->setEvent(sensorEvents[i].type, &event);

                    //CLOGD2("orientation : %d  -> event.rotation.orientation : %d", orientation, event.rotation.orientation);
                }
                break;
            default:
                break;
            }
        }
    }

    if (num_sensors < 0 && num_sensors != -EAGAIN) {
        CLOGE2("reading events failed: %zd", num_sensors);
    }

    return 1;
}

bool ExynosCameraSensorListenerASensor::m_sensorLooperThread(void)
{
    bool ret = true;

    ASensorManager* mgr = ASensorManager_getInstanceForPackage(NULL);
    ASensor const* sensor;

    if (m_flagThreadRun == false) {
        m_looper = ALooper_forThread();
        if (m_looper == NULL) {
            m_looper = ALooper_prepare(0);
        }

        if (m_eventQueue == NULL) {
            m_eventQueue = ASensorManager_createEventQueue(mgr, m_looper, 0, ExynosCameraSensorListenerASensor::m_sensorEventListener, this);
        }

        unsigned int sensoTypeBit = m_getSensorTypeBit();

        for (int i = 0; i < SUPPORED_NUM_OF_SENSOR; i++) {

            int sensorType = i;
            int hwSensorType = sensorType;

            // ORIENTATION works by ACCELEROMETER
            if (sensorType == SENSOR_TYPE_ORIENTATION) {
                hwSensorType = SENSOR_TYPE_ACCELEROMETER;
            }

            if (sensoTypeBit & (1 << sensorType)) {
                sensor = ASensorManager_getDefaultSensor(mgr, hwSensorType);
                if (sensor == NULL) {
                    CLOGE2("ASensorManager_getDefaultSensor(sensorType(%d:%s) -> hwSensorType(%d:%s)) fail",
                        sensorType, m_getSensorName(sensorType),
                        hwSensorType, m_getSensorName(hwSensorType));

                    ASensorManager_destroyEventQueue(mgr, m_eventQueue);
                    m_eventQueue = NULL;
                    return false;
                }
                ASensorEventQueue_enableSensor(m_eventQueue, sensor);
                ASensorEventQueue_setEventRate(m_eventQueue, sensor, m_getSampleTimeMsec(sensorType) * 1000);
                //CLOGD2("sensorType %d on", sensoTypeBit);
            }
        }

        m_flagThreadRun = true;
    }

    /* Wait 1.5 sec */
    ALooper_pollOnce(DEFAULT_SAMPLING_TIME_MSEC, NULL, NULL, NULL);

    return ret;
}

ASensorEventQueue* ExynosCameraSensorListenerASensor::m_getSensorEventQueue(void)
{
    return m_eventQueue;
}

void ExynosCameraSensorListenerASensor::m_setLatestTs(nsecs_t ts)
{
    m_lastTs = ts;
}

nsecs_t ExynosCameraSensorListenerASensor::m_getLatestTs(void)
{
    return m_lastTs;
}