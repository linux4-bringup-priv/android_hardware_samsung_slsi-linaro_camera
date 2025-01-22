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
#define LOG_TAG "ExynosCameraEEPRomMap5E9"

#include "ExynosCameraEEPRomMap5E9.h"

ExynosCameraEEPRomMap5E9::~ExynosCameraEEPRomMap5E9()
{
}

status_t ExynosCameraEEPRomMap5E9::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    ret = ExynosCameraEEPRomMapGM1SP::m_create(eepromData, eepromSize);
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraEEPRomMapGM1SP::m_create() fail");
        return ret;
    }

    ////////////////////////////////////////////////
    strncpy(m_sensorName, "5E9", EEPROM_MAP_BUF_SIZE);
    m_sensorNameSize = strnlen(m_sensorName, EEPROM_MAP_BUF_SIZE);

    return ret;
}

void ExynosCameraEEPRomMap5E9::m_destroy(void)
{
    ExynosCameraEEPRomMapGM1SP::m_destroy();
}

void ExynosCameraEEPRomMap5E9::m_init(void)
{
    m_excelEEPRomSize = 5409;
    this->setName("ExynosCameraEEPRomMap5E9");
}
