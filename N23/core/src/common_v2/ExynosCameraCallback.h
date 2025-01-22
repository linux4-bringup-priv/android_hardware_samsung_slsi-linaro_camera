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
 * \file      ExynosCameraCallback.h
 * \brief     header file for ExynosCameraCallback
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/12/17
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_CALLBACK_H
#define EXYNOS_CAMERA_CALLBACK_H

#include <sys/types.h>
#include <log/log.h>
#include <hardware/sensors.h>
#include <utils/Looper.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"
#include "ExynosCameraUtils.h"

using namespace android;

class SensorLooperThread: public Thread {
public:
    SensorLooperThread(Looper* looper) : Thread(false) {
        mLooper = sp<Looper>(looper);
    }
    ~SensorLooperThread() {
        mLooper.clear();
    }

    virtual bool threadLoop() {

        int32_t ret = mLooper->pollOnce(-1);
        return true;
    }

    void wake() {
        mLooper->wake();
    }
private:
    sp<Looper> mLooper;
};

/*
 * Class ExynosCameraCallback
 *
 * you must use sp<ExynosCameraCallback> variable.
 */
class ExynosCameraCallback : public Thread {
public:
    enum CALLBACK_TYPE {
        CALLBACK_TYPE_BASE       = 0,
        CALLBACK_TYPE_REPEAT     = 1,
        CALLBACK_TYPE_USER_COUNT = 2,
        CALLBACK_TYPE_MAX        = 3,
    };

    /*
     * Class Args
     */
    class Args {
    public:
        Args(void) {
            m_init();
        }

        ~Args(void) {
        }

    private:
        typedef void(*callbackFunc) (void* data0, void* data1);

    public:
        void set(callbackFunc func, void *data0 = NULL, void *data1 = NULL);
        void call(void);

        Args& operator =(const Args &other) {
            m_callbackFunc = other.m_callbackFunc;
            m_data0        = other.m_data0;
            m_data1        = other.m_data1;

            return *this;
        }

    private:
        callbackFunc   m_callbackFunc;
        void          *m_data0;
        void          *m_data1;

        void m_init(void) {
            m_callbackFunc = NULL;
            m_data0        = NULL;
            m_data1        = NULL;
        }
    };

private:
    ExynosCameraCallback(void): Thread(false) {
        m_init();
    }

public:
    ExynosCameraCallback(int cameraId, const char *name, int32_t priority = PRIORITY_DEFAULT): Thread(false) {
        m_init();

        m_cameraId = cameraId;
        strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

    ~ExynosCameraCallback(void);

public:
    // user API
    virtual status_t create(void) final;
    virtual void     destroy(void) final;
    virtual bool     flagCreated(void) final;

    void             setCallback(ExynosCameraCallback::Args *args,
                                 ExynosCameraCallback::CALLBACK_TYPE callbackType = CALLBACK_TYPE_REPEAT,
                                 int callbackCount = -1);

    status_t         start(void);
    void             stop(void);
    virtual bool     flagStarted(void) final;

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

private:
    bool             m_flagCreated(void);
    void             m_init(void);

private:
    virtual bool     threadLoop();

private:
    Mutex            m_lock;
    bool             m_flagCreate;

    int              m_cameraId;
    char             m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    CALLBACK_TYPE    m_callbackType;
    int              m_callbackCount;
    int              m_curCallbackCount;

    int              m_priority;
    Args             m_args;

    bool             m_flagStart;
};

#endif /* EXYNOS_CAMERA_CALLBACK_H */
