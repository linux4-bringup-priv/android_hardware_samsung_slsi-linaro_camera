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
 * \file      ExynosCameraSensorListenerDummy.h
 * \brief     header file for ExynosCameraSensorListenerDummy
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/12/17
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_SENSOR_LISTENER_DUMMY_H
#define EXYNOS_CAMERA_SENSOR_LISTENER_DUMMY_H

#include <log/log.h>
#include "ExynosCameraSensorListener.h"
#include "ExynosCameraCallback.h"

using namespace android;

/*
 * Class ExynosCameraSensorListenerDummy
 */
class ExynosCameraSensorListenerDummy: public ExynosCameraSensorListener
{
protected:
    ExynosCameraSensorListenerDummy(): ExynosCameraSensorListener()
    {
        m_init();
    }

public:
    ExynosCameraSensorListenerDummy(int cameraId): ExynosCameraSensorListener(cameraId)
    {
        m_init();
    }

    virtual ~ExynosCameraSensorListenerDummy();

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

    virtual status_t m_start(int sensorType, int samplingTimeMsec);
    virtual status_t m_stop(int sensorType);

private:
    void             m_init(void);
    static  void     m_listenerCallbackFunc(void *data0, void *data1);

private:
    sp<ExynosCameraCallback> m_callback;
};

#endif /* EXYNOS_CAMERA_SENSOR_LISTENER_DUMMY_H */
