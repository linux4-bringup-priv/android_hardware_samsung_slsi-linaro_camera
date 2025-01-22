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
#define LOG_TAG "ExynosCameraFactoryTestSensorGyro"

#include "ExynosCameraFactoryTestSensorGyro.h"
#include "libGyroST/include/Gyro_SelfTest.h"

#define SENSOR_GYRO_GYRO_ST_LIBRARY_PATH "/vendor/lib/libGyroST.so"
#define SENSOR_GYRO_USE_DL_OPEN

ExynosCameraFactoryTestSensorGyro::~ExynosCameraFactoryTestSensorGyro()
{
}

int ExynosCameraFactoryTestSensorGyro::getMaxIndex(void)
{
    return SENSOR_GYRO_TEST_MAX_INDEX;
}

bool ExynosCameraFactoryTestSensorGyro::flagValidBuffer(int index)
{
    ////////////////////////////////////////////////
    // check argument
    if (index < 0 || getMaxIndex() < index) {
        CLOGD("index(%d) < 0 || getMaxIndex(%d) < index(%d). so, fail", index, getMaxIndex(), index);
        return false;
    }

    ////////////////////////////////////////////////

    return m_flagValidBuffer[index];
}

status_t ExynosCameraFactoryTestSensorGyro::setBuffer(int index, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;

    char bit0 = 0;
    char bit1 = 0;

    ////////////////////////////////////////////////
    // check argument
    if (index < 0 || getMaxIndex() < index) {
        CLOGD("index(%d) < 0 || getMaxIndex(%d) < index(%d). so, fail", index, getMaxIndex(), index);
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // check it already filled
    if (m_flagValidBuffer[index] == true) {
        CLOGD("%d buffer is already filled (m_flagValidBuffer[%d] == true). so, fail", index, index);
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // checking argument
    bit0 = *(buffer->addr[0]);
    bit1 = *(buffer->addr[0] + 1);

    if (0xCA != bit0 || 0xFE != bit1) {
        CLOGE("0xCA != bit0(0x%X) || 0xFE != bit1(0x%X). so, fail", bit0, bit1);
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // alloc & memcpy
    for (int i = 0; i < EXYNOS_CAMERA_BUFFER_MAX_PLANES; i++) {
        if (buffer->addr[i] != NULL &&
            buffer->size[i] != 0) {
            int bufSize = buffer->size[i];

            m_buffer[index].size[i] = bufSize;
            m_buffer[index].addr[i] = new char[bufSize];

            memcpy(m_buffer[index].addr[i], buffer->addr[i], bufSize);
        }
    }

    ////////////////////////////////////////////////
    // check it is valid
    m_flagValidBuffer[index] = true;

    ////////////////////////////////////////////////
done:
    CLOGD("m_flagValidBuffer[%d](%d)", index, m_flagValidBuffer[index]);

    return ret;
}

bool ExynosCameraFactoryTestSensorGyro::flagChecked(void)
{
    return m_flagChecked;
}


#ifdef SENSOR_GYRO_USE_DL_OPEN
#else
/*
int SEC_Gyro_SelfTest(void* elg1, void* elg2, unsigned char* otp)
{
    return 0x01;
}
*/
#endif

status_t ExynosCameraFactoryTestSensorGyro::check(void)
{
    status_t ret = NO_ERROR;
#ifdef USES_SENSOR_GYRO_FACTORY_MODE
    struct camera2_shot_ext *shot_ext = NULL;
    unsigned char otp[3] = { 0, 0, 0 };

    ////////////////////////////////////////////////
    // checking argument
    for (int i = 0; i < 2; i++) {
        char bit0 = *(m_buffer[i].addr[0]);
        char bit1 = *(m_buffer[i].addr[0] + 1);

        if (0xCA != bit0 || 0xFE != bit1) {
            CLOGE("i(%d) 0xCA != bit0(0x%X) || 0xFE != bit1(0x%X). so, fail", i, bit0, bit1);
            m_result = 0xFE;
            goto done;
        }
    }

    shot_ext = (struct camera2_shot_ext *)m_buffer[1].addr[1];
    if (shot_ext == NULL) {
        CLOGE("shot_ext == NULL. so, fail");
        goto done;
    }

    ////////////////////////////////////////////////
    // call API
    otp[0] = (unsigned char)shot_ext->user.gyro_info.x;
    otp[1] = (unsigned char)shot_ext->user.gyro_info.y;
    otp[2] = (unsigned char)shot_ext->user.gyro_info.z;

#ifdef SENSOR_GYRO_USE_DL_OPEN
    m_result = (*SEC_Gyro_SelfTest)((void*)m_buffer[0].addr[0],
                                    (void*)m_buffer[1].addr[0],
                                    otp);
#else
    m_result = SEC_Gyro_SelfTest((void*)m_buffer[0].addr[0],
                                 (void*)m_buffer[1].addr[0],
                                  otp);
#endif

    if (m_result == 1) { // pass
        m_result = 0x01;
    } else {             // fail
        CLOGE("m_result(%d). so, SEC_Gyro_SelfTest() fail", m_result);
        m_result = 0xFF;
    }


    // 0x01 : OK
    // 0xFF : NG
    // 0xFE : Gyro communation error;

done:
    CLOGD("m_buffer[0](addr(%p), size(%d)), m_buffer[1].(addr(%p), size(%d)) otp(%d, %d, %d), m_result(0x%X)",
        m_buffer[0].addr[0], m_buffer[0].size[0],
        m_buffer[1].addr[0], m_buffer[1].size[0],
        otp[0], otp[1], otp[2],
        m_result);

    m_flagChecked = true;

    ////////////////////////////////////////////////
#endif
    return ret;
}


int ExynosCameraFactoryTestSensorGyro::getResult(void)
{
    // 0x01 : OK
    // 0xFF : NG
    // 0xFE : Gyro communation error;

    return m_result;

}

status_t ExynosCameraFactoryTestSensorGyro::m_create(void)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // init value
    m_gyroSTDl    = NULL;
    m_flagChecked = false;
    m_result = 0xFE;

    for (int i = 0; i <= getMaxIndex(); i++) {
        m_flagValidBuffer[i] = false;
    }

#ifdef SENSOR_GYRO_USE_DL_OPEN
    ////////////////////////////////////////////////
    // open dl library
    ret = m_openLibrary();
    if (ret != NO_ERROR) {
        CLOGE("m_openLibrary() fail");
    }

#endif

    ////////////////////////////////////////////////

    return ret;
}

void ExynosCameraFactoryTestSensorGyro::m_destroy(void)
{
    status_t ret = NO_ERROR;

#ifdef SENSOR_GYRO_USE_DL_OPEN
    ////////////////////////////////////////////////
    // close dl library
    ret = m_closeLibrary();
    if (ret != NO_ERROR) {
        CLOGE("m_closeLibrary() fail");
    }
#endif

    ////////////////////////////////////////////////
    // check vendor meta tag
    for (int i = 0; i <= getMaxIndex(); i++) {
        for (int j = 0; j < EXYNOS_CAMERA_BUFFER_MAX_PLANES; j++) {
            if (m_buffer[i].addr[j] != NULL) {
                SAFE_DELETE(m_buffer[i].addr[j]);
                m_buffer[i].size[j] = 0;
            }
        }

        m_flagValidBuffer[i] = false;
    }

    ////////////////////////////////////////////////
}

status_t ExynosCameraFactoryTestSensorGyro::m_openLibrary(void)
{
    CLOGD("start");

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // dlopen
    m_gyroSTDl = dlopen(SENSOR_GYRO_GYRO_ST_LIBRARY_PATH, RTLD_NOW);
    if (m_gyroSTDl == NULL) {
        CLOGE("dlopen(%s, RTLD_NOW) fail", SENSOR_GYRO_GYRO_ST_LIBRARY_PATH);
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////
    // function open
    SEC_Gyro_SelfTest = (int(*)(void* elg1, void* elg2, unsigned char* otp)) dlsym(m_gyroSTDl, "SEC_Gyro_SelfTest");
    if (dlerror() != NULL && SEC_Gyro_SelfTest == NULL) {
        CLOGE("dlsym(SEC_Gyro_SelfTest) fail. dlerror : %s", dlerror());
        ret = INVALID_OPERATION;
        goto done;
    }

    ////////////////////////////////////////////////s

done:
    if (ret != NO_ERROR) {
        if (m_gyroSTDl != NULL) {
            dlclose(m_gyroSTDl);
            m_gyroSTDl = NULL;
        }
    }

    CLOGD("done");

    return ret;
}

status_t ExynosCameraFactoryTestSensorGyro::m_closeLibrary(void)
{
    CLOGD("start");

    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // dlclose
    if (m_gyroSTDl != NULL) {
        dlclose(m_gyroSTDl);
        m_gyroSTDl = NULL;
    }

    ////////////////////////////////////////////////

    CLOGD("done");

    return ret;
}

void ExynosCameraFactoryTestSensorGyro::m_init(void)
{
    this->setName("ExynosCameraFactoryTestSensorGyro");

    m_flagChecked = false;
    m_result = 0xFE;
    m_gyroSTDl	  = NULL;
    SEC_Gyro_SelfTest = NULL;
}
