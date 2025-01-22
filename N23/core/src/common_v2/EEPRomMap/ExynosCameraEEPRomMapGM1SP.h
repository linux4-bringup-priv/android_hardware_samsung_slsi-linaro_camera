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

#ifndef EXYNOS_CAMERA_EEROM_MAP_GM1SP_H
#define EXYNOS_CAMERA_EEROM_MAP_GM1SP_H

#include "ExynosCameraEEPRomMap.h"

class ExynosCameraEEPRomMapGM1SP : public ExynosCameraEEPRomMap
{
protected:
    ExynosCameraEEPRomMapGM1SP(): ExynosCameraEEPRomMap()
    {
        m_init();
    }

    ExynosCameraEEPRomMapGM1SP(int cameraId): ExynosCameraEEPRomMap(cameraId)
    {
        m_init();
    }

    /*
     * This constructor is protected.
     * because this only new() by ExynosCameraEEPRomMapFactory::newEEPRomMap()
     */
    friend class ExynosCameraEEPRomMapFactory;

public:
    virtual ~ExynosCameraEEPRomMapGM1SP();

protected:
    virtual status_t m_create(const char *eepromData, const int eepromSize);
    virtual void     m_destroy(void);

private:
    void             m_init(void);
};

#endif /* EXYNOS_CAMERA_EEROM_MAP_GM1SP_H */
