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
 * \file      ExynosCameraSensorListener.h
 * \brief     header file for ExynosCameraSensorListener
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/12/17
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_SENSOR_LISTENER_H
#define EXYNOS_CAMERA_SENSOR_LISTENER_H

#include <sys/types.h>
#include <log/log.h>
#include <hardware/sensors.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"
#include "ExynosCameraUtils.h"
#include "ExynosCameraCallback.h"

using namespace android;

#define SENSOR_LISTENER_DEBUG

#ifdef SENSOR_LISTENER_DEBUG
#define SENSOR_LISTENER_DEBUG_LOG CLOGD
#else
#define SENSOR_LISTENER_DEBUG_LOG CLOGV
#endif

/*
 * because SENSOR_TYPE_DYNAMIC_SENSOR_META is 32, it is maximum bit of int.
 * this is defined in hardware/libhardware/include/hardware/asensors-base.h
 */
#define SUPPORED_NUM_OF_SENSOR     (SENSOR_TYPE_DYNAMIC_SENSOR_META)
#define DEFAULT_SAMPLING_TIME_MSEC (33)
#define SUPPORED_NUM_OF_EVENT (20)

/*
 * Class ExynosCameraSensorListener
 */
class ExynosCameraSensorListener : public ExynosCameraObject
{
public:
    enum EVENT_TYPE
    {
        EVENT_TYPE_BASE,
        EVENT_TYPE_OLDEST,
        EVENT_TYPE_LATEST,
        EVENT_TYPE_MAX,
    };

public:
    typedef struct gyroUncaliEvent
    {
        float x;
        float y;
        float z;
        float x_bias;
        float y_bias;
        float z_bias;
    } gyroUncaliEvent_t;

    typedef struct gyroEvent
    {
        float x;
        float y;
        float z;
    } gyroEvent_t;

    typedef struct accelEvent
    {
        float x;
        float y;
        float z;
    } accelEvent_t;

    typedef struct accelUncaliEvent
    {
        float xUncali;
        float yUncali;
        float zUncali;
        float x;
        float y;
        float z;
    } accelUncaliEvent_t;

    typedef struct rotationEvent
    {
        int orientation;
    } rotationEvent_t;

    typedef struct Event
    {
        nsecs_t timestamp;

        union
        {
            gyroUncaliEvent_t   gyroUncali;
            gyroEvent_t         gyro;
            accelEvent_t        accel;
            accelUncaliEvent_t  accelUncali;
            rotationEvent_t     rotation;
        };
    } Event_t;

protected:
    ExynosCameraSensorListener(): ExynosCameraObject()
    {
        m_init();
    }

public:
    ExynosCameraSensorListener(int cameraId) : ExynosCameraObject()
    {
        m_init();

        m_cameraId = cameraId;
    }

    virtual ~ExynosCameraSensorListener();

public:
    // user API
    virtual status_t create(void) final;
    virtual void     destroy(void) final;
    virtual bool     flagCreated(void) final;

    // int sensorType is in hardware/libhardware/include/hardware/sensors-base.h
    virtual status_t start(int sensorType, int samplingTimeMsec = DEFAULT_SAMPLING_TIME_MSEC) final;
    virtual status_t stop(int sensorType) final;

    virtual status_t setEvent(int sensorType, ExynosCameraSensorListener::Event_t *event) final;
    virtual status_t getEvent(int sensorType, ExynosCameraSensorListener::Event_t *event, ExynosCameraSensorListener::EVENT_TYPE eventType = EVENT_TYPE_LATEST) final;
    virtual int      getNumberOfEvent(int sensorType) final;

public:
    static char const *m_getSensorName(int sensorType);
    static  int      m_axis2Orientation(float azimuth, float pitch, float roll);

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

    virtual status_t m_start(int sensorType, int samplingTimeMsec);
    virtual status_t m_stop(int sensorType);

    virtual status_t m_setEvent(int sensorType, ExynosCameraSensorListener::Event_t *event);
    virtual status_t m_getEvent(int sensorType, ExynosCameraSensorListener::Event_t *event, ExynosCameraSensorListener::EVENT_TYPE eventType);
    virtual int      m_getNumberOfEvent(int sensorType);

public:
    virtual int      m_getSensorTypeBit(void) final;

    virtual void     m_setSampleTimeMsec(int sensorType, int samplingTimeMsec) final;
    virtual int      m_getSampleTimeMsec(int sensorType) final;

private:
    void             m_init(void);
    virtual bool     m_flagCreated(void) final;
    static  void     m_listenerCallbackFunc(void *data0, void *data1);

private:
    Mutex            m_lock;
    bool             m_flagCreate;

    unsigned int     m_sensorTypeBit;
    int              m_samplingTimeMsec[SUPPORED_NUM_OF_SENSOR];

protected:
    ExynosCameraSensorListener::Event_t m_event[SUPPORED_NUM_OF_SENSOR][SUPPORED_NUM_OF_EVENT];

    Mutex            m_eventLock[SUPPORED_NUM_OF_SENSOR];
    int              m_eventSize[SUPPORED_NUM_OF_SENSOR];
    int              m_eventOldIndex[SUPPORED_NUM_OF_SENSOR];
    int              m_eventNewIndex[SUPPORED_NUM_OF_SENSOR];
};

#endif /* EXYNOS_CAMERA_SENSOR_LISTENER_H */
