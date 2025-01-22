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

#ifndef EXYNOS_CAMERA_MAKERS_NOTE_H
#define EXYNOS_CAMERA_MAKERS_NOTE_H

#include <sys/types.h>
#include <log/log.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"
#include "ExynosCameraEEPRomMap.h"
#include "fimc-is-metadata.h"

#include "ExynosCameraUtils.h"
#include "ExynosCameraVendorUtils.h"

using namespace android;

//#define MAKERS_NOTE_DEBUG

#ifdef MAKERS_NOTE_DEBUG
#define MAKERS_NOTE_DEBUG_LOG CLOGD
#else
#define MAKERS_NOTE_DEBUG_LOG CLOGV
#endif


class ExynosCameraMakersNote : public ExynosCameraObject
{
public:
    class Args {
    public:
        ExynosCameraEEPRomMap      *eepromMap;
        camera2_shot_ext           *shot_ext;
        ExynosCameraSensorInfoBase *sensorInfo;
        char                       *buf;
        int                         bufSize;

        Args() {
            eepromMap  = NULL;
            shot_ext   = NULL;
            sensorInfo = NULL;
            buf        = NULL;
            bufSize = 0;
        }
    };

protected:
    ExynosCameraMakersNote() : ExynosCameraObject()
    {
        m_init();
    }

    ExynosCameraMakersNote(int cameraId) : ExynosCameraObject()
    {
        m_init();

        setCameraId(cameraId);
    }

public:
    virtual ~ExynosCameraMakersNote();

    // user API
    virtual status_t     create(void) final;
    virtual void         destroy(void) final;
    virtual bool         flagCreated(void) final;

    virtual unsigned int getSize(void);
    virtual status_t     fillMakersNote(Args *args);

protected:
    virtual status_t m_create(void) = 0;
    virtual void     m_destroy(void) = 0;

    virtual void     m_setTag(const char *tagName,
                              char *tagBuf,
                              int   tagBufSize,
                              char *eepromData);

    virtual void     m_setTag(const char *tagName,
                              char *tagBuf,
                              int   tagBufSize,
                              int   value);

private:
    bool             m_flagCreated(void);
    void             m_init(void);

private:
    Mutex            m_lock;
    bool             m_flagCreate;
};

#endif /* EXYNOS_CAMERA_MAKERS_NOTE_H */
