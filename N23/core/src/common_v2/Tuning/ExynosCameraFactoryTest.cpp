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
#define LOG_TAG "ExynosCameraFactoryTest"

#include "ExynosCameraFactoryTest.h"

ExynosCameraFactoryTest::~ExynosCameraFactoryTest()
{
}

status_t ExynosCameraFactoryTest::create(void)
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
    ret = ExynosCameraFactoryTest::m_create();
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraFactoryTest::m_create() fail");
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////
    // child class m_create
    ret = this->m_create();
    if (ret != NO_ERROR) {
        CLOGE("this->m_create() fail");
        return INVALID_OPERATION;
    }

    ////////////////////////////////////////////////

    m_flagCreate = true;

    CLOGD("done");

    return ret;
}

void ExynosCameraFactoryTest::destroy(void)
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
    ExynosCameraFactoryTest::m_destroy();

    ////////////////////////////////////////////////
    // my class m_destroy
    this->m_destroy();

    ////////////////////////////////////////////////

    m_flagCreate = false;

    CLOGD("done");
}

bool ExynosCameraFactoryTest::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

status_t ExynosCameraFactoryTest::m_create(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

void ExynosCameraFactoryTest::m_destroy(void)
{
}

bool ExynosCameraFactoryTest::m_flagCreated(void)
{
    return m_flagCreate;
}

void ExynosCameraFactoryTest::m_init(void)
{
    m_flagCreate = false;

    this->setName("ExynosCameraFactoryTest");
}
