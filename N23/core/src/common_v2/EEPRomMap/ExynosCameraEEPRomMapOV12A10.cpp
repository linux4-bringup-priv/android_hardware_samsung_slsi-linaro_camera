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
#define LOG_TAG "ExynosCameraEEPRomMapOV12A10"

#include "ExynosCameraEEPRomMapOV12A10.h"

ExynosCameraEEPRomMapOV12A10::~ExynosCameraEEPRomMapOV12A10()
{
}

status_t ExynosCameraEEPRomMapOV12A10::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;
	int offset=0;
	
    if (eepromSize < m_excelEEPRomSize) {
        CLOGE("eepromSize(%d) < m_excelEEPRomSize(%d). so, fail", eepromSize, m_excelEEPRomSize);
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    strncpy(m_sensorName, "OV12A10", EEPROM_MAP_BUF_SIZE);
    m_sensorNameSize = strnlen(m_sensorName, EEPROM_MAP_BUF_SIZE);

    ////////////////////////////////////////////////
    snprintf(m_calSWVersion, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[118]);

    ////////////////////////////////////////////////
    m_manufactureDateSize = 3;
    snprintf(m_manufactureDate, EEPROM_MAP_BUF_SIZE, "20%u/%u/%u",
        (uint8_t)eepromData[134], (uint8_t)eepromData[135], (uint8_t)eepromData[136]);

    ////////////////////////////////////////////////
    m_manufactureIdSize = 2;
    if ((uint8_t)eepromData[129] == 'S' && (uint8_t)eepromData[130] == 'U') {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "Sunny");
    } else if ((uint8_t)eepromData[129] == 'O' && (uint8_t)eepromData[130] == 'F') {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "OFilm");
    } else {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "Unknown");
    }
    ////////////////////////////////////////////////
    snprintf(m_manufactureLine, EEPROM_MAP_BUF_SIZE, "%u", (uint8_t)eepromData[133]);

    ////////////////////////////////////////////////
    m_factoryIdSize = 2;
    snprintf(m_factoryId, EEPROM_MAP_BUF_SIZE, "%c%c",
        (uint8_t)eepromData[131], (uint8_t)eepromData[132]);
	
    ////////////////////////////////////////////////
    m_serialNumberSize = 16;
	offset=137;
    snprintf(m_serialNumber, EEPROM_MAP_BUF_SIZE, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        (uint8_t)eepromData[offset+0], (uint8_t)eepromData[offset+1],
        (uint8_t)eepromData[offset+2], (uint8_t)eepromData[offset+3],
        (uint8_t)eepromData[offset+4], (uint8_t)eepromData[offset+5],
        (uint8_t)eepromData[offset+6], (uint8_t)eepromData[offset+7],
        (uint8_t)eepromData[offset+8], (uint8_t)eepromData[offset+9],
        (uint8_t)eepromData[offset+10], (uint8_t)eepromData[offset+11],
        (uint8_t)eepromData[offset+12], (uint8_t)eepromData[offset+13],
        (uint8_t)eepromData[offset+14], (uint8_t)eepromData[offset+15]);
    //force each serial number store as 2 digits
    m_serialNumberSize = m_serialNumberSize * 2;

    ////////////////////////////////////////////////
    m_partNumberSize = 8;
	offset=119;
    snprintf(m_partNumber, EEPROM_MAP_BUF_SIZE, "%c%c%c%c%c%c%c%c",
        (uint8_t)eepromData[offset+0], (uint8_t)eepromData[offset+1],
        (uint8_t)eepromData[offset+2], (uint8_t)eepromData[offset+3],
        (uint8_t)eepromData[offset+4], (uint8_t)eepromData[offset+5],
        (uint8_t)eepromData[offset+6], (uint8_t)eepromData[offset+7]);

    ////////////////////////////////////////////////
    snprintf(m_lensId, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[128]);

    ////////////////////////////////////////////////
    // AWB
    m_copyEEPRom(&m_awbCurrentR, eepromData, 196, 4, "m_awbCurrentR");

    m_copyEEPRom(&m_awbCurrentGr, eepromData, 200, 4, "m_awbCurrentGr");

    m_copyEEPRom(&m_awbCurrentGb, eepromData, 204, 4, "m_awbCurrentGb");

    m_copyEEPRom(&m_awbCurrentB, eepromData, 208, 4, "m_awbCurrentB");

    m_copyEEPRom(&m_awbGoldenR, eepromData, 180, 4, "m_awbGoldenR");

    m_copyEEPRom(&m_awbGoldenGr, eepromData, 184, 4, "m_awbGoldenGr");

    m_copyEEPRom(&m_awbGoldenGb, eepromData, 188, 4, "m_awbGoldenGb");

    m_copyEEPRom(&m_awbGoldenB, eepromData, 192, 4, "m_awbGoldenB");

    m_copyEEPRom(&m_awbRgRatio, eepromData, 226, 2, "m_awbRgRatio");

    m_copyEEPRom(&m_awbBgRatio, eepromData, 228, 2, "m_awbBgRatio");

    m_copyEEPRom(&m_awbGrGbRatio, eepromData, 230, 2, "m_awbGrGbRatio");

    m_copyEEPRom(&m_awbBlackLevel, eepromData, 212, 2, "m_awbBlackLevel");

    ////////////////////////////////////////////////
    //actuator position
    m_copyEEPRom(&m_infinityDacOrg, eepromData, 296, 4, "m_infinityDacOrg");
    m_copyEEPRom(&m_infinityDacExt, eepromData, 296, 4, "m_infinityDacExt");
    m_copyEEPRom(&m_macroDacOrg, eepromData, 312, 4, "m_macroDacOrg");
    m_copyEEPRom(&m_macroDacExt, eepromData, 312, 4, "m_macroDacExt");

    ////////////////////////////////////////////////
    // AF
    m_copyEEPRom(&m_focalLength, eepromData, 316, 4, "m_focalLength");

    ////////////////////////////////////////////////
    // Ois
    //m_oisHeaTargetSize = 4;
    //m_copyEEPRom(m_oisHeaTarget, eepromData, 0x1DF4, 4, "m_oisHeaTarget");

    return ret;
}

void ExynosCameraEEPRomMapOV12A10::m_destroy(void)
{
}

void ExynosCameraEEPRomMapOV12A10::m_init(void)
{
    m_excelEEPRomSize = 12751;
    this->setName("ExynosCameraEEPRomMapOV12A10");
}
