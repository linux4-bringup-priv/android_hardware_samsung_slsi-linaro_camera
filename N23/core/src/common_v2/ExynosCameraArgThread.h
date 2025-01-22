/*
**
** Copyright 2017, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef EXYNOS_CAMERA_ARG_THREAD_H
#define EXYNOS_CAMERA_ARG_THREAD_H

#include "ExynosCameraThread.h"

using namespace android;

template<typename T>
class ExynosCameraArgThread : public ExynosCameraThread<T> {

typedef bool (T::*arg_thread_loop)(void *);

public:
    ExynosCameraArgThread(
        T *hw,
        arg_thread_loop loop,
        const char *name,
        void *data,
        int32_t priority = PRIORITY_DEFAULT) :
        ExynosCameraThread<T>{ hw, nullptr, name, priority },
        m_argThreadLoop{ loop },
        m_data{ data }
    {
    }

    virtual status_t setup(
        T *hw,
        arg_thread_loop loop,
        const char *name,
        void *data,
        int32_t priority = PRIORITY_DEFAULT)
    {
        ExynosCameraThread<T>::setup(hw, nullptr, name, priority);

        m_argThreadLoop = loop;
        m_data       = data;

        return NO_ERROR;
    }

    virtual void setData(void *data) {
        m_data = data;
    }

protected:
    virtual bool threadLoop() {
        bool ret = (ExynosCameraThread<T>::m_hardware->*m_argThreadLoop)(m_data);

        if (ExynosCameraThread<T>::m_flatStart == false)
            ret = ExynosCameraThread<T>::m_flatStart;

        return ret;
    }

protected:
    arg_thread_loop m_argThreadLoop;
    void           *m_data;
};

#endif
