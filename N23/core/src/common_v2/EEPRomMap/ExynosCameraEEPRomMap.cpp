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
#define LOG_TAG "ExynosCameraEEPRomMap"

#include "ExynosCameraEEPRomMap.h"
#define AWB_RATIO 16384

ExynosCameraEEPRomMap::~ExynosCameraEEPRomMap()
{
}

status_t ExynosCameraEEPRomMap::create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == true) {
        CLOGE("It is already created. so, fail");
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // my class m_create
    ret = ExynosCameraEEPRomMap::m_create(eepromData, eepromSize);
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraEEPRomMap::m_create() fail");
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // child class m_create
    ret = this->m_create(eepromData, eepromSize);
    if (ret != NO_ERROR) {
        CLOGE("this->m_create() fail");
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////

    m_flagCreate = true;

    CLOGD("done");

    return ret;
}

void ExynosCameraEEPRomMap::destroy(void)
{
    Mutex::Autolock lock(m_lock);

    ////////////////////////////////////////////////
    // check it is created
    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        return;
    }

    ////////////////////////////////////////////////
    // child class m_destroy
    this->m_destroy();

    ////////////////////////////////////////////////
    // my class m_destroy
    ExynosCameraEEPRomMap::m_destroy();

    ////////////////////////////////////////////////

    m_flagCreate = false;

    CLOGD("done");
}

bool ExynosCameraEEPRomMap::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

char *ExynosCameraEEPRomMap::getSensorName(int *size)
{
    *size = m_sensorNameSize;

    return m_sensorName;
}

char *ExynosCameraEEPRomMap::getManuFactureId(int *size)
{
    *size = m_manufactureIdSize;

    return m_manufactureId;
}

char *ExynosCameraEEPRomMap::getManuFactureLine(void)
{
    return m_manufactureLine;
}

char *ExynosCameraEEPRomMap::getManuFactureDate(int *size)
{
    *size = m_manufactureDateSize;

    return m_manufactureDate;
}

char *ExynosCameraEEPRomMap::getFactoryId(int *size)
{
    *size = m_factoryIdSize;

    return m_factoryId;
}

char *ExynosCameraEEPRomMap::getSerialNumber(int *size)
{
    *size = m_serialNumberSize;

    return m_serialNumber;
}

char *ExynosCameraEEPRomMap::getCalSWVersion(void)
{
    return m_calSWVersion;
}

char *ExynosCameraEEPRomMap::getPartNumber(int *size)
{
    *size = m_partNumberSize;

    return m_partNumber;
}

char *ExynosCameraEEPRomMap::getActuatorId(void)
{
    return m_actuatorId;
}

char *ExynosCameraEEPRomMap::getLensId(void)
{
    return m_lensId;
}

int ExynosCameraEEPRomMap::getAwbCurrentR(void)
{
    return m_awbCurrentR;
}

int ExynosCameraEEPRomMap::getAwbCurrentGr(void)
{
    return m_awbCurrentGr;
}

int ExynosCameraEEPRomMap::getAwbCurrentGb(void)
{
    return m_awbCurrentGb;
}

int ExynosCameraEEPRomMap::getAwbCurrentB(void)
{
    return m_awbCurrentB;
}

int ExynosCameraEEPRomMap::getAwbGoldenR(void)
{
    return m_awbGoldenR;
}

int ExynosCameraEEPRomMap::getAwbGoldenGr(void)
{
    return m_awbGoldenGr;
}

int ExynosCameraEEPRomMap::getAwbGoldenGb(void)
{
    return m_awbGoldenGb;
}

int ExynosCameraEEPRomMap::getAwbGoldenB(void)
{
    return m_awbGoldenB;
}

float ExynosCameraEEPRomMap::getAwbRgRatio(void)
{
    return m_awbRgRatio / AWB_RATIO;
}

float ExynosCameraEEPRomMap::getAwbBgRatio(void)
{
    return m_awbBgRatio / AWB_RATIO;
}

float ExynosCameraEEPRomMap::getAwbGrGbRatio(void)
{
    return m_awbGrGbRatio / AWB_RATIO;
}

int ExynosCameraEEPRomMap::getAwbBlackLevel(void)
{
    return m_awbBlackLevel;
}

int ExynosCameraEEPRomMap::getInfinityDacOrg(void)
{
    return m_infinityDacOrg;
}

int ExynosCameraEEPRomMap::getInfinityDacExt(void)
{
    return m_infinityDacExt;
}

int ExynosCameraEEPRomMap::getMacroDacOrg(void)
{
    return m_macroDacOrg;
}

int ExynosCameraEEPRomMap::getMacroDacExt(void)
{
    return m_macroDacExt;
}

float ExynosCameraEEPRomMap::getFocalLength(void)
{
    return m_focalLength;
}

char* ExynosCameraEEPRomMap::getOisHeaTarget(int *size)
{
    *size = m_oisHeaTargetSize;
    return m_oisHeaTarget;
}

status_t ExynosCameraEEPRomMap::m_create(const char *eepromData, const int eepromSize)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // just default value
    strncpy(m_sensorName, "not defined", EEPROM_MAP_BUF_SIZE);
    m_sensorNameSize = strlen(m_sensorName);

    strncpy(m_calSWVersion, "not defined", EEPROM_MAP_BUF_SIZE);

    strncpy(m_partNumber, "not defined", EEPROM_MAP_BUF_SIZE);
    m_partNumberSize = strlen(m_partNumber);

    strncpy(m_actuatorId, "not defined", EEPROM_MAP_BUF_SIZE);

    strncpy(m_lensId, "not defined", EEPROM_MAP_BUF_SIZE);

    strncpy(m_manufactureId, "not defined", EEPROM_MAP_BUF_SIZE);
    m_manufactureIdSize = strlen(m_manufactureId);

    strncpy(m_manufactureLine, "not defined", EEPROM_MAP_BUF_SIZE);

    strncpy(m_manufactureDate, "2019/01/01", EEPROM_MAP_BUF_SIZE);
    m_manufactureDateSize = strlen(m_manufactureDate);

    strncpy(m_factoryId, "not defined", EEPROM_MAP_BUF_SIZE);
    m_factoryIdSize = strlen(m_factoryId);

    strncpy(m_serialNumber, "not defined", EEPROM_MAP_BUF_SIZE);
    m_serialNumberSize = strlen(m_serialNumber);

    // ois
    memset(m_oisHeaTarget, 0, EEPROM_MAP_BUF_SIZE);
    m_oisHeaTargetSize = strlen(m_oisHeaTarget);

    // AWB
    m_awbCurrentR = 0;
    m_awbCurrentGr = 0;
    m_awbCurrentGb = 0;
    m_awbCurrentB = 0;
    m_awbGoldenR = 0;
    m_awbGoldenGr = 0;
    m_awbGoldenGb = 0;
    m_awbGoldenB = 0;

    m_awbRgRatio = 0.0f;
    m_awbBgRatio = 0.0f;
    m_awbGrGbRatio = 0.0f;

    m_awbBlackLevel = 0;

    return ret;
}

void ExynosCameraEEPRomMap::m_destroy(void)
{
}

bool ExynosCameraEEPRomMap::m_flagCreated(void)
{
    return m_flagCreate;
}

void ExynosCameraEEPRomMap::m_copyEEPRom(char *dataAddr, const char *eeprom,
                                         int offset, int size,
                                         const char *debugInfo)
{
    for (int i = 0; i < size; i++) {
        dataAddr[i] = eeprom[offset + i];
        EEPROM_MAP_DEBUG_LOG("%-20s[%5d + %d]: size(%d)/d(%d)/c(%c)", debugInfo, offset, i, size, (int)dataAddr[i], dataAddr[i]);
    }

    // this is not string. so, do not put NULL.
    //dataAddr[size] = '\0';
}

void ExynosCameraEEPRomMap::m_copyEEPRom(int *dataAddr, const char *eeprom,
                                         int offset, int size,
                                         const char *debugInfo)
{
    char  *ptr1 = NULL;
    short *ptr2 = NULL;
    int   *ptr4 = NULL;

    switch (size) {
    case 1:
        ptr1 = (char *)(eeprom + offset);
        *dataAddr = (int)*ptr1;
        break;
    case 2:
        ptr2 = (short *)(eeprom + offset);
        *dataAddr = (int)*ptr2;
        break;
    case 4:
        ptr4 = (int *)(eeprom + offset);
        *dataAddr = (int)*ptr4;
        break;
    default:
        CLOGE("Invalid size(%d). so, fail. cannot copy from EEPRom, when get %s.", size, debugInfo);
        break;
    }

    EEPROM_MAP_DEBUG_LOG("%-20s[%5d]: size(%d)/d(%d)", debugInfo, offset, size, *dataAddr);
}

void ExynosCameraEEPRomMap::m_copyEEPRom(float *dataAddr, const char *eeprom,
                                         int offset, int size,
                                         const char *debugInfo)
{
    char  *ptr1 = NULL;
    short *ptr2 = NULL;
    int   *ptr4 = NULL;

    switch (size) {
    case 1:
        ptr1 = (char *)(eeprom + offset);
        *dataAddr = (float)*ptr1;
        break;
    case 2:
        ptr2 = (short *)(eeprom + offset);
        *dataAddr = (float)*ptr2;
        break;
    case 4:
        ptr4 = (int *)(eeprom + offset);
        *dataAddr = (float)*ptr4;
        break;
    default:
        CLOGE("Invalid size(%d). so, fail. cannot copy from EEPRom, when get %s.", size, debugInfo);
        break;
    }

    EEPROM_MAP_DEBUG_LOG("%-20s[%5d]: size(%d)/f(%f)", debugInfo, offset, size, *dataAddr);
}

void ExynosCameraEEPRomMap::m_init(void)
{
    m_flagCreate = false;
    m_excelEEPRomSize = 4;

    this->setName("ExynosCameraEEPRomMap");
}
