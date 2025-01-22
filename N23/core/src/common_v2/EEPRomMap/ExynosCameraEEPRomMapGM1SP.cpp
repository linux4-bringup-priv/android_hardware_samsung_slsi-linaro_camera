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
#define LOG_TAG "ExynosCameraEEPRomMapGM1SP"

#include "ExynosCameraEEPRomMapGM1SP.h"

ExynosCameraEEPRomMapGM1SP::~ExynosCameraEEPRomMapGM1SP()
{
}

status_t ExynosCameraEEPRomMapGM1SP::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    if (eepromSize < m_excelEEPRomSize) {
        CLOGE("eepromSize(%d) < m_excelEEPRomSize(%d). so, fail", eepromSize, m_excelEEPRomSize);
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    strncpy(m_sensorName, "GM1SP", EEPROM_MAP_BUF_SIZE);
    m_sensorNameSize = strnlen(m_sensorName, EEPROM_MAP_BUF_SIZE);

    ////////////////////////////////////////////////
    snprintf(m_calSWVersion, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[102]);

    ////////////////////////////////////////////////
    m_partNumberSize = 8;
    snprintf(m_partNumber, EEPROM_MAP_BUF_SIZE, "%c%c%c%c%c%c%c%c",
        (uint8_t)eepromData[103], (uint8_t)eepromData[104], (uint8_t)eepromData[105],
        (uint8_t)eepromData[106], (uint8_t)eepromData[107], (uint8_t)eepromData[108],
        (uint8_t)eepromData[109], (uint8_t)eepromData[110]);

    ////////////////////////////////////////////////
    snprintf(m_actuatorId, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[111]);
    snprintf(m_lensId, EEPROM_MAP_BUF_SIZE, "0x%x", (uint8_t)eepromData[112]);

    ////////////////////////////////////////////////
    m_manufactureIdSize = 2;
    if ((uint8_t)eepromData[113] == 'S' && (uint8_t)eepromData[114] == 'E') {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "SEMCO");
    } else {
        snprintf(m_manufactureId, EEPROM_MAP_BUF_SIZE, "Unknown");
    }

    ////////////////////////////////////////////////
    m_factoryIdSize = 2;
    snprintf(m_factoryId, EEPROM_MAP_BUF_SIZE, "%c%c",
        (uint8_t)eepromData[115], (uint8_t)eepromData[116]);

    ////////////////////////////////////////////////
    snprintf(m_manufactureLine, EEPROM_MAP_BUF_SIZE, "%u", (uint8_t)eepromData[117]);

    ////////////////////////////////////////////////
    m_manufactureDateSize = 3;
    snprintf(m_manufactureDate, EEPROM_MAP_BUF_SIZE, "20%u/%u/%u",
        (uint8_t)eepromData[118], (uint8_t)eepromData[119], (uint8_t)eepromData[120]);

    ////////////////////////////////////////////////
    m_serialNumberSize = 8;
    char tmp_serialNumber[EEPROM_MAP_BUF_SIZE];
    m_copyEEPRom(tmp_serialNumber, eepromData, 121, m_serialNumberSize, "m_serialNumber");
    snprintf(m_serialNumber, EEPROM_MAP_BUF_SIZE, "%02x%02x%02x%02x%02x%02x%02x%02x",
        (uint8_t)tmp_serialNumber[0], (uint8_t)tmp_serialNumber[1],
        (uint8_t)tmp_serialNumber[2], (uint8_t)tmp_serialNumber[3],
        (uint8_t)tmp_serialNumber[4], (uint8_t)tmp_serialNumber[5],
        (uint8_t)tmp_serialNumber[6], (uint8_t)tmp_serialNumber[7]);
    //force each serial number store as 2 digits
    m_serialNumberSize = m_serialNumberSize * 2;

    ////////////////////////////////////////////////
    // AWB
    m_copyEEPRom(&m_awbCurrentR, eepromData, 164, 4, "m_awbCurrentR");

    m_copyEEPRom(&m_awbCurrentGr, eepromData, 168, 4, "m_awbCurrentGr");

    m_copyEEPRom(&m_awbCurrentGb, eepromData, 172, 4, "m_awbCurrentGb");

    m_copyEEPRom(&m_awbCurrentB, eepromData, 176, 4, "m_awbCurrentB");

    m_copyEEPRom(&m_awbGoldenR, eepromData, 148, 4, "m_awbGoldenR");

    m_copyEEPRom(&m_awbGoldenGr, eepromData, 152, 4, "m_awbGoldenGr");

    m_copyEEPRom(&m_awbGoldenGb, eepromData, 156, 4, "m_awbGoldenGb");

    m_copyEEPRom(&m_awbGoldenB, eepromData, 160, 4, "m_awbGoldenB");

    m_copyEEPRom(&m_awbRgRatio, eepromData, 190, 2, "m_awbRgRatio");

    m_copyEEPRom(&m_awbBgRatio, eepromData, 192, 2, "m_awbBgRatio");

    m_copyEEPRom(&m_awbGrGbRatio, eepromData, 194, 2, "m_awbGrGbRatio");

    m_copyEEPRom(&m_awbBlackLevel, eepromData, 196, 2, "m_awbBlackLevel");

    ////////////////////////////////////////////////
    //actuator position
    m_copyEEPRom(&m_infinityDacOrg, eepromData, 216, 4, "m_infinityDacOrg");
    m_copyEEPRom(&m_infinityDacExt, eepromData, 216, 4, "m_infinityDacExt");
    m_copyEEPRom(&m_macroDacOrg, eepromData, 232, 4, "m_macroDacOrg");
    m_copyEEPRom(&m_macroDacExt, eepromData, 232, 4, "m_macroDacExt");

    ////////////////////////////////////////////////
    // AF
    m_copyEEPRom(&m_focalLength, eepromData, 236, 4, "m_focalLength");

    ////////////////////////////////////////////////
    // Ois
    m_oisHeaTargetSize = 4;
    m_copyEEPRom(m_oisHeaTarget, eepromData, 0x1DF4, 4, "m_oisHeaTarget");

    return ret;
}

void ExynosCameraEEPRomMapGM1SP::m_destroy(void)
{
}

void ExynosCameraEEPRomMapGM1SP::m_init(void)
{
    m_excelEEPRomSize = 12777;
    this->setName("ExynosCameraEEPRomMapGM1SP");
}
