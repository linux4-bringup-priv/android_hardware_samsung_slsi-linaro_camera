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
 * \file      ExynosCameraSensorListenerASensor.h
 * \brief     header file for ExynosCameraSensorListenerASensor
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/12/17
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_SENSOR_LISTENER_ASENSOR_H
#define EXYNOS_CAMERA_SENSOR_LISTENER_ASENSOR_H

#include "ExynosCameraSensorListener.h"

#include <log/log.h>
#include <android/sensor.h>
#include <utils/Looper.h>

using namespace android;

#ifndef ASENSOR_TYPE_ORIENTATION
#define ASENSOR_TYPE_ORIENTATION (3)  // this is not defined in frameworks/native/include/androidsensor.h
#endif

/*
 * Class ExynosCameraSensorListenerASensor
 */
class ExynosCameraSensorListenerASensor: public ExynosCameraSensorListener
{
protected:
    ExynosCameraSensorListenerASensor(): ExynosCameraSensorListener()
    {
        m_init();
    }

public:
    ExynosCameraSensorListenerASensor(int cameraId): ExynosCameraSensorListener(cameraId)
    {
        m_init();
    }

    virtual ~ExynosCameraSensorListenerASensor();

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

    virtual status_t m_start(int sensorType, int samplingTimeMsec);
    virtual status_t m_stop(int sensorType);

private:
    void             m_init(void);

private:
    class SensorListenerThread : public Thread
    {
    public:
        SensorListenerThread(ExynosCameraSensorListenerASensor* sensorListener)
            : Thread(false)
        {
            m_sensorListener = sensorListener;
        }
        ~SensorListenerThread()
        {
        }

        virtual bool threadLoop()
        {
            m_sensorListener->m_sensorLooperThread();
            return true;
        }

    private:
        ExynosCameraSensorListenerASensor* m_sensorListener;
    };

    int  m_startLoop(void);
    int  m_stopLoop(void);
    bool m_sensorLooperThread(void);
    static int m_sensorEventListener(int fd, int numOfEvents, void* data);

public:
    ASensorEventQueue* m_getSensorEventQueue(void);
    void               m_setLatestTs(nsecs_t ts);
    nsecs_t            m_getLatestTs(void);

private:
    ASensorEventQueue*       m_eventQueue;
    ALooper*                 m_looper;
    sp<SensorListenerThread> m_thread;
    bool                     m_flagThreadRun;

    nsecs_t                  m_lastTs;
};

#endif /* EXYNOS_CAMERA_SENSOR_LISTENER_ASENSOR_H */
