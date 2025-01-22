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

#ifndef EXYNOS_CAMERA_FACTORY_TEST_SENSOR_GYRO_H
#define EXYNOS_CAMERA_FACTORY_TEST_SENSOR_GYRO_H

#include <dlfcn.h>

#include "ExynosCameraFactoryTest.h"

#define SENSOR_GYRO_TEST_MAX_INDEX (1)

class ExynosCameraFactoryTestSensorGyro : public ExynosCameraFactoryTest
{
protected:
    ExynosCameraFactoryTestSensorGyro() : ExynosCameraFactoryTest()
    {
        m_init();
    }

    ExynosCameraFactoryTestSensorGyro(int cameraId) : ExynosCameraFactoryTest(cameraId)
    {
        m_init();
    }

    /*
     * This constructor is protected.
     * because this only new() by ExynosCameraMakersNoteFactory::newMakersNote()
     */
    friend class ExynosCameraFactoryTestFactory;

public:
    virtual ~ExynosCameraFactoryTestSensorGyro();

    // user API
    virtual int      getMaxIndex(void);
    virtual bool     flagValidBuffer(int index);
    virtual status_t setBuffer(int index, ExynosCameraBuffer *buffer);

    virtual bool     flagChecked(void);
    virtual status_t check(void);
    virtual int      getResult(void);

protected:
    virtual status_t m_create(void);
    virtual void     m_destroy(void);

    virtual status_t m_openLibrary(void);
    virtual status_t m_closeLibrary(void);

private:
    void             m_init(void);

private:
    bool               m_flagChecked;
    int                m_result;

    bool               m_flagValidBuffer[SENSOR_GYRO_TEST_MAX_INDEX + 1];
    ExynosCameraBuffer m_buffer[SENSOR_GYRO_TEST_MAX_INDEX + 1];

    /* DL Library APIs */
    void              *m_gyroSTDl;
    int               (*SEC_Gyro_SelfTest)(void* elg1, void* elg2, unsigned char* otp);
};

#endif /* EXYNOS_CAMERA_FACTORY_TEST_SENSOR_GYRO_H */
