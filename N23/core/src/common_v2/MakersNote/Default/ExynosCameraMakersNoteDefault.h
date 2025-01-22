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

#ifndef EXYNOS_CAMERA_MAKERS_NOTE_DEFAULT_H
#define EXYNOS_CAMERA_MAKERS_NOTE_DEFAULT_H

#include "ExynosCameraMakersNote.h"

class ExynosCameraMakersNoteDefault : public ExynosCameraMakersNote
{
protected:
    ExynosCameraMakersNoteDefault(): ExynosCameraMakersNote()
    {
        m_init();
    }

    ExynosCameraMakersNoteDefault(int cameraId): ExynosCameraMakersNote(cameraId)
    {
        m_init();
    }

    /*
     * This constructor is protected.
     * because this only new() by ExynosCameraMakersNoteFactory::newMakersNote()
     */
    friend class ExynosCameraMakersNoteFactory;

public:
    virtual ~ExynosCameraMakersNoteDefault();

    // user API
    virtual unsigned int getSize(void);
    virtual status_t     fillMakersNote(Args *args);

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

private:
    void             m_init(void);
};

#endif /* EXYNOS_CAMERA_MAKERS_NOTE_DEFAULT_H */
