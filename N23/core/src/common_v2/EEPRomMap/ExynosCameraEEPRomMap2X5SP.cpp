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
#define LOG_TAG "ExynosCameraEEPRomMap2X5SP"

#include "ExynosCameraEEPRomMap2X5SP.h"

ExynosCameraEEPRomMap2X5SP::~ExynosCameraEEPRomMap2X5SP()
{
}

status_t ExynosCameraEEPRomMap2X5SP::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    if (eepromSize < m_excelEEPRomSize) {
        CLOGE("eepromSize(%d) < m_excelEEPRomSize(%d). so, fail", eepromSize, m_excelEEPRomSize);
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    strncpy(m_sensorName, "2X5SP", EEPROM_MAP_BUF_SIZE);
    m_sensorNameSize = strnlen(m_sensorName, EEPROM_MAP_BUF_SIZE);

    ////////////////////////////////////////////////
    snprintf(m_calSWVersion, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[16259]);

    ////////////////////////////////////////////////
    m_manufactureDateSize = 3;
    snprintf(m_manufactureDate, EEPROM_MAP_BUF_SIZE, "20%u/%u/%u",
        (uint8_t)eepromData[16275], (uint8_t)eepromData[16276], (uint8_t)eepromData[16277]);

    ////////////////////////////////////////////////
    m_manufactureIdSize = 2;
    if ((uint8_t)eepromData[16270] == 'S' && (uint8_t)eepromData[16271] == 'U') {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "Sunny");
    } else if ((uint8_t)eepromData[16270] == 'O' && (uint8_t)eepromData[16271] == 'F') {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "OFilm");
    } else {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "Unknown");
    }
    ////////////////////////////////////////////////
    snprintf(m_manufactureLine, EEPROM_MAP_BUF_SIZE, "%u", (uint8_t)eepromData[16274]);

    ////////////////////////////////////////////////
    m_factoryIdSize = 2;
    snprintf(m_factoryId, EEPROM_MAP_BUF_SIZE, "%c%c",
        (uint8_t)eepromData[16272], (uint8_t)eepromData[16273]);
    ////////////////////////////////////////////////
    m_serialNumberSize = 6;
    snprintf(m_serialNumber, EEPROM_MAP_BUF_SIZE, "%02x%02x%02x%02x%02x%02x",
        (uint8_t)eepromData[16278], (uint8_t)eepromData[16279],
        (uint8_t)eepromData[16280], (uint8_t)eepromData[16281],
        (uint8_t)eepromData[16282], (uint8_t)eepromData[16283]);

    ////////////////////////////////////////////////
    m_partNumberSize = 8;
    snprintf(m_partNumber, EEPROM_MAP_BUF_SIZE, "%c%c%c%c%c%c%c%c",
        (uint8_t)eepromData[16260], (uint8_t)eepromData[16261], (uint8_t)eepromData[16262],
        (uint8_t)eepromData[16263], (uint8_t)eepromData[16264], (uint8_t)eepromData[16265],
        (uint8_t)eepromData[16266], (uint8_t)eepromData[16267]);

    ////////////////////////////////////////////////
    snprintf(m_lensId, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[16269]);

    ////////////////////////////////////////////////
    // AWB
    m_copyEEPRom(&m_awbCurrentR, eepromData, 16302, 2, "m_awbCurrentR");

    m_copyEEPRom(&m_awbCurrentGr, eepromData, 16304, 2, "m_awbCurrentGr");

    m_copyEEPRom(&m_awbCurrentGb, eepromData, 16306, 2, "m_awbCurrentGb");

    m_copyEEPRom(&m_awbCurrentB, eepromData, 16308, 2, "m_awbCurrentB");

    m_copyEEPRom(&m_awbGoldenR, eepromData, 16288, 2, "m_awbGoldenR");

    m_copyEEPRom(&m_awbGoldenGr, eepromData, 16290, 2, "m_awbGoldenGr");

    m_copyEEPRom(&m_awbGoldenGb, eepromData, 16292, 2, "m_awbGoldenGb");

    m_copyEEPRom(&m_awbGoldenB, eepromData, 16294, 2, "m_awbGoldenB");

    m_copyEEPRom(&m_awbRgRatio, eepromData, 16310, 2, "m_awbRgRatio");

    m_copyEEPRom(&m_awbBgRatio, eepromData, 16312, 2, "m_awbBgRatio");

    m_copyEEPRom(&m_awbGrGbRatio, eepromData, 16314, 2, "m_awbGrGbRatio");

    // not in excel.
    //m_copyEEPRom(&m_awbBlackLevel, eepromData, 196, 2, "m_awbBlackLevel");

    ////////////////////////////////////////////////

    return ret;
}

void ExynosCameraEEPRomMap2X5SP::m_destroy(void)
{
}

void ExynosCameraEEPRomMap2X5SP::m_init(void)
{
    m_excelEEPRomSize = 16383;
    this->setName("ExynosCameraEEPRomMap2X5SP");
}
