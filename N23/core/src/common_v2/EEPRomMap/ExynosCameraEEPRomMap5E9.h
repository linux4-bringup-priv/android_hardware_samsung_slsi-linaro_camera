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

#ifndef EXYNOS_CAMERA_EEROM_MAP_5E9_H
#define EXYNOS_CAMERA_EEROM_MAP_5E9_H

#include "ExynosCameraEEPRomMapGM1SP.h"

// 5E9's map is same with GM1SP's map
class ExynosCameraEEPRomMap5E9: public ExynosCameraEEPRomMapGM1SP
{
protected:
    ExynosCameraEEPRomMap5E9(): ExynosCameraEEPRomMapGM1SP()
    {
        m_init();
    }

    ExynosCameraEEPRomMap5E9(int cameraId): ExynosCameraEEPRomMapGM1SP(cameraId)
    {
        m_init();
    }

    /*
     * This constructor is protected.
     * because this only new() by ExynosCameraEEPRomMapFactory::newEEPRomMap()
     */
    friend class ExynosCameraEEPRomMapFactory;

public:
    virtual ~ExynosCameraEEPRomMap5E9();

protected:
    virtual status_t m_create(const char *eepromData, const int eepromSize);
    virtual void     m_destroy(void);

private:
    void             m_init(void);
};

#endif /* EXYNOS_CAMERA_EEROM_MAP_5E9_H */
