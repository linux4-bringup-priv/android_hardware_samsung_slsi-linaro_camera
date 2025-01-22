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
#define LOG_TAG "ExynosCameraCallback"

#include "ExynosCameraCallback.h"

void ExynosCameraCallback::Args::set(callbackFunc func, void *data0, void *data1)
{
    m_callbackFunc = func;
    m_data0 = data0;
    m_data1 = data1;
}

void ExynosCameraCallback::Args::call(void)
{
    if (m_callbackFunc == NULL) {
        ALOGE("m_callbackFunc == NULL. so, fail");
    } else {
        m_callbackFunc(m_data0, m_data1);
    }
}

ExynosCameraCallback::~ExynosCameraCallback(void)
{
    CLOGD("requestExitAndWait() start");
    this->requestExitAndWait();
    CLOGD("requestExitAndWait() end");
}

status_t ExynosCameraCallback::create(void)
{
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
    // my class m_create
    ret = ExynosCameraCallback::m_create();
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraCallback::m_create() fail");
        ret = INVALID_OPERATION;
        goto done;
    }

    m_flagCreate = true;

    ////////////////////////////////////////////////
done:

    return ret;
}

void ExynosCameraCallback::destroy(void)
{
    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        return;
    }

    if (flagStarted() == true) {
        this->stop();
    }

    ////////////////////////////////////////////////
    // my class m_destroy
    ExynosCameraCallback::m_destroy();

    m_flagCreate = false;
    ////////////////////////////////////////////////
}

bool ExynosCameraCallback::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

void ExynosCameraCallback::setCallback(ExynosCameraCallback::Args *args,
                                       ExynosCameraCallback::CALLBACK_TYPE callbackType,
                                       int callbackCount)
{
    m_args             = *args;
    m_callbackType     = callbackType;
    m_callbackCount    = callbackCount;
    m_curCallbackCount = m_callbackCount;
}

status_t ExynosCameraCallback::start(void)
{
    m_flagStart = true;
    m_curCallbackCount = m_callbackCount;

    return Thread::run(m_name, m_priority);
}

bool ExynosCameraCallback::flagStarted(void)
{
    return m_flagStart;
}

void ExynosCameraCallback::stop(void)
{
    m_flagStart = false;

    // need to put time checking.
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    this->join();
}

bool ExynosCameraCallback::threadLoop() {
    bool ret = false;

    ////////////////////////////////////////////////
    // after start(), call callbackFunc.
    m_args.call();

    ////////////////////////////////////////////////
    // after stop(), stop this thread.
    if (m_flagStart == false) {
        ret = false;
        goto done;
    }

    ////////////////////////////////////////////////
    // decide repeate thread, or not.
    switch (m_callbackType) {
    case CALLBACK_TYPE_REPEAT:
        ret = true;
        break;
    case CALLBACK_TYPE_USER_COUNT:
        if (0 < m_curCallbackCount) {
            m_curCallbackCount--;
            ret = true;
        } else {
            ret = false;
        }
        break;
    default:
        CLOGE("Invalid m_callbackType(%d)",m_callbackType );
        break;
    }

    ////////////////////////////////////////////////
done:

    return ret;
}

status_t ExynosCameraCallback::m_create(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

void ExynosCameraCallback::m_destroy(void)
{
    status_t ret = NO_ERROR;

    this->requestExit();
}

bool ExynosCameraCallback::m_flagCreated(void)
{
    return m_flagCreate;
}

void ExynosCameraCallback::m_init(void)
{
    m_flagCreate = false;

    m_cameraId = -1;

    m_callbackType = CALLBACK_TYPE_REPEAT;
    m_callbackCount = 1;
    m_curCallbackCount = 1;
    m_priority = PRIORITY_URGENT_DISPLAY;

    m_flagStart = false;
}