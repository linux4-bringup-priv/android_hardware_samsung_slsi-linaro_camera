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
#define LOG_TAG "ExynosCameraEEPRomMap6B2"

#include "ExynosCameraEEPRomMap6B2.h"

ExynosCameraEEPRomMap6B2::~ExynosCameraEEPRomMap6B2()
{
}

status_t ExynosCameraEEPRomMap6B2::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    // we will to find proper information, on EEPROM

    return ret;
}

void ExynosCameraEEPRomMap6B2::m_destroy(void)
{
}

void ExynosCameraEEPRomMap6B2::m_init(void)
{
    m_excelEEPRomSize = 4;
    this->setName("ExynosCameraEEPRomMap6B2");
}
