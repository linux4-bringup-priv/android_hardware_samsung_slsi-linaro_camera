/*
**
** Copyright 2018, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_FACTORY_TEST_H
#define EXYNOS_CAMERA_FACTORY_TEST_H

#include <sys/types.h>
#include <log/log.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"
#include "fimc-is-metadata.h"

#include "ExynosCameraUtils.h"
#include "ExynosCameraVendorUtils.h"

using namespace android;

//#define FACTORY_TEST_DEBUG

#ifdef FACTORY_TEST_DEBUG
#define FACTORY_TEST_DEBUG_LOG CLOGD
#else
#define FACTORY_TEST_DEBUG_LOG CLOGV
#endif


class ExynosCameraFactoryTest : public ExynosCameraObject
{
public:
    class Args {
    public:
        char                       *buf;
        int                         bufSize;

        Args() {
            buf        = NULL;
            bufSize = 0;
        }
    };

protected:
    ExynosCameraFactoryTest() : ExynosCameraObject()
    {
        m_init();
    }

    ExynosCameraFactoryTest(int cameraId) : ExynosCameraObject()
    {
        m_init();

        setCameraId(cameraId);
    }

public:
    virtual ~ExynosCameraFactoryTest();

    // user API
    virtual status_t     create(void) final;
    virtual void         destroy(void) final;
    virtual bool         flagCreated(void) final;

protected:
    virtual status_t m_create(void) = 0;
    virtual void     m_destroy(void) = 0;

private:
    bool             m_flagCreated(void);
    void             m_init(void);

private:
    Mutex            m_lock;
    bool             m_flagCreate;
};

#endif /* EXYNOS_CAMERA_FACTORY_TEST_H */
