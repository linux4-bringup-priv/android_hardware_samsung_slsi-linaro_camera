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

#ifndef EXYNOS_CAMERA_EEROM_MAP_H
#define EXYNOS_CAMERA_EEROM_MAP_H

#include <sys/types.h>
#include <log/log.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraObject.h"

using namespace android;

#define EEPROM_MAP_BUF_SIZE (64)

//#define EEPROM_MAP_DEBUG

#ifdef EEPROM_MAP_DEBUG
#define EEPROM_MAP_DEBUG_LOG CLOGD
#else
#define EEPROM_MAP_DEBUG_LOG CLOGV
#endif

struct ExynosCameraEEPRomMap : public ExynosCameraObject
{
protected:
    ExynosCameraEEPRomMap() : ExynosCameraObject()
    {
        for (int i = 0; i < EEPROM_MAP_BUF_SIZE; i++) {
            m_sensorName[i] = 0;
            m_calSWVersion[i] = 0;
            m_partNumber[i] = 0;
            m_actuatorId[i] = 0;
            m_lensId[i] = 0;
            m_manufactureId[i] = 0;
            m_manufactureLine[i] = 0;
            m_manufactureDate[i] = 0;
            m_factoryId[i] = 0;
            m_serialNumber[i] = 0;
            m_oisHeaTarget[i] = 0;
        }

        m_excelEEPRomSize = 0;
        m_sensorNameSize = 0;
        m_partNumberSize = 0;
        m_manufactureIdSize = 0;
        m_manufactureDateSize = 0;
        m_factoryIdSize = 0;
        m_serialNumberSize = 0;
        m_awbCurrentR = 0;
        m_awbCurrentGr = 0;
        m_awbCurrentGb = 0;
        m_awbCurrentB = 0;
        m_awbGoldenR = 0;
        m_awbGoldenGr = 0;
        m_awbGoldenGb = 0;
        m_awbGoldenB = 0;
        m_awbRgRatio = 0;
        m_awbBgRatio = 0;
        m_awbGrGbRatio = 0;
        m_awbBlackLevel = 0;
        m_focalLength = 0;
        m_infinityDacOrg = 0;
        m_infinityDacExt = 0;
        m_macroDacOrg = 0;
        m_macroDacExt = 0;
        m_oisHeaTargetSize = 0;
        m_flagCreate = 0;

        m_init();
    }

    ExynosCameraEEPRomMap(int cameraId) : ExynosCameraObject()
    {
        for (int i = 0; i < EEPROM_MAP_BUF_SIZE; i++) {
            m_sensorName[i] = 0;
            m_calSWVersion[i] = 0;
            m_partNumber[i] = 0;
            m_actuatorId[i] = 0;
            m_lensId[i] = 0;
            m_manufactureId[i] = 0;
            m_manufactureLine[i] = 0;
            m_manufactureDate[i] = 0;
            m_factoryId[i] = 0;
            m_serialNumber[i] = 0;
            m_oisHeaTarget[i] = 0;
        }

        m_excelEEPRomSize = 0;
        m_sensorNameSize = 0;
        m_partNumberSize = 0;
        m_manufactureIdSize = 0;
        m_manufactureDateSize = 0;
        m_factoryIdSize = 0;
        m_serialNumberSize = 0;
        m_awbCurrentR = 0;
        m_awbCurrentGr = 0;
        m_awbCurrentGb = 0;
        m_awbCurrentB = 0;
        m_awbGoldenR = 0;
        m_awbGoldenGr = 0;
        m_awbGoldenGb = 0;
        m_awbGoldenB = 0;
        m_awbRgRatio = 0;
        m_awbBgRatio = 0;
        m_awbGrGbRatio = 0;
        m_awbBlackLevel = 0;
        m_focalLength = 0;
        m_infinityDacOrg = 0;
        m_infinityDacExt = 0;
        m_macroDacOrg = 0;
        m_macroDacExt = 0;
        m_oisHeaTargetSize = 0;
        m_flagCreate = 0;

        m_init();

        setCameraId(cameraId);
    }

public:
    virtual ~ExynosCameraEEPRomMap();

    // user API
    virtual status_t    create(const char *eepromData, const int eepromSize) final;
    virtual void        destroy(void) final;
    virtual bool        flagCreated(void) final;

    virtual char       *getSensorName(int *size) final;
    virtual char       *getManuFactureId(int *size) final;
    virtual char       *getManuFactureLine(void) final;
    virtual char       *getManuFactureDate(int *size) final;
    virtual char       *getFactoryId(int *size) final;
    virtual char       *getSerialNumber(int *size) final;
    virtual char       *getCalSWVersion(void) final;
    virtual char       *getPartNumber(int *size) final;
    virtual char       *getActuatorId(void) final;
    virtual char       *getLensId(void) final;
    virtual int         getInfinityDacOrg(void) final;
    virtual int         getInfinityDacExt(void) final;
    virtual int         getMacroDacOrg(void) final;
    virtual int         getMacroDacExt(void) final;
    virtual float       getFocalLength(void) final;

    // AWB
    virtual int        getAwbCurrentR(void) final;
    virtual int        getAwbCurrentGr(void) final;
    virtual int        getAwbCurrentGb(void) final;
    virtual int        getAwbCurrentB(void) final;
    virtual int        getAwbGoldenR(void) final;
    virtual int        getAwbGoldenGr(void) final;
    virtual int        getAwbGoldenGb(void) final;
    virtual int        getAwbGoldenB(void) final;

    virtual float      getAwbRgRatio(void) final;
    virtual float      getAwbBgRatio(void) final;
    virtual float      getAwbGrGbRatio(void) final;

    virtual int        getAwbBlackLevel(void) final;

    virtual char       *getOisHeaTarget(int *size);

protected:
    virtual status_t m_create(const char *eepromData, const int eepromSize) = 0;
    virtual void     m_destroy(void) = 0;

    void             m_copyEEPRom(char *dataAddr, const char *eeprom,
                                  int offset, int size,
                                  const char *debugInfo);

    void             m_copyEEPRom(int *dataAddr, const char *eeprom,
                                  int offset, int size,
                                  const char *debugInfo);

    void             m_copyEEPRom(float *dataAddr, const char *eeprom,
                                  int offset, int size,
                                  const char *debugInfo);

private:
    bool             m_flagCreated(void);
    void             m_init(void);

protected:
    int              m_excelEEPRomSize;

    char             m_sensorName[EEPROM_MAP_BUF_SIZE];
    int              m_sensorNameSize;

    char             m_calSWVersion[EEPROM_MAP_BUF_SIZE];

    char             m_partNumber[EEPROM_MAP_BUF_SIZE];
    int              m_partNumberSize;

    char             m_actuatorId[EEPROM_MAP_BUF_SIZE];

    char             m_lensId[EEPROM_MAP_BUF_SIZE];

    char             m_manufactureId[EEPROM_MAP_BUF_SIZE];
    int              m_manufactureIdSize;

    char             m_manufactureLine[EEPROM_MAP_BUF_SIZE];

    char             m_manufactureDate[EEPROM_MAP_BUF_SIZE];
    int              m_manufactureDateSize;

    char             m_factoryId[EEPROM_MAP_BUF_SIZE];
    int              m_factoryIdSize;

    char             m_serialNumber[EEPROM_MAP_BUF_SIZE];
    int              m_serialNumberSize;

    // AWB
    int              m_awbCurrentR;
    int              m_awbCurrentGr;
    int              m_awbCurrentGb;
    int              m_awbCurrentB;
    int              m_awbGoldenR;
    int              m_awbGoldenGr;
    int              m_awbGoldenGb;
    int              m_awbGoldenB;

    float            m_awbRgRatio;
    float            m_awbBgRatio;
    float            m_awbGrGbRatio;

    int              m_awbBlackLevel;

    // AF
    float            m_focalLength;

    //actuator position
    int              m_infinityDacOrg;
    int              m_infinityDacExt;
    int              m_macroDacOrg;
    int              m_macroDacExt;

    //ois target position
    char             m_oisHeaTarget[EEPROM_MAP_BUF_SIZE];
    int              m_oisHeaTargetSize;
private:
    Mutex            m_lock;
    bool             m_flagCreate;
};

#endif /* EXYNOS_CAMERA_EEROM_MAP_H */
