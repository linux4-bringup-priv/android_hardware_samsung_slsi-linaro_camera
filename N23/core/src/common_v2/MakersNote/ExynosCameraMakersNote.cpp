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
#define LOG_TAG "ExynosCameraMakersNote"

#include "ExynosCameraMakersNote.h"

ExynosCameraMakersNote::~ExynosCameraMakersNote()
{
}

status_t ExynosCameraMakersNote::create(void)
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
    ret = ExynosCameraMakersNote::m_create();
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraMakersNote::m_create() fail");
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

void ExynosCameraMakersNote::destroy(void)
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
    ExynosCameraMakersNote::m_destroy();

    ////////////////////////////////////////////////
    // my class m_destroy
    this->m_destroy();

    ////////////////////////////////////////////////

    m_flagCreate = false;

    CLOGD("done");
}

bool ExynosCameraMakersNote::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

unsigned int ExynosCameraMakersNote::getSize(void)
{
    return sizeof(*this);
}

status_t ExynosCameraMakersNote::fillMakersNote(Args *args)
{
    status_t ret = NO_ERROR;

    return ret;
}

status_t ExynosCameraMakersNote::m_create(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

void ExynosCameraMakersNote::m_destroy(void)
{
}

void ExynosCameraMakersNote::m_setTag(const char *tagName,
                                      char *tagBuf,
                                      int   tagBufSize,
                                      char *eepromData)
{
    if (tagName == NULL) {
        CLOGE("tagName == NULL. so, fail");
    }

    if (tagBuf == nullptr) {
        CLOGE("tagBuf == NULL. so, fail");
        return;
    }

    if (eepromData == nullptr) {
        CLOGE("eepromData == NULL. so, fail");
        return;
    }

    memcpy(tagBuf, eepromData, tagBufSize);
    tagBuf[tagBufSize] = '\0';

    MAKERS_NOTE_DEBUG_LOG("%s(%s), tagBufSize(%d)",
        tagName, tagBuf, tagBufSize);
}

void ExynosCameraMakersNote::m_setTag(const char *tagName,
                                      char *tagBuf,
                                      int   tagBufSize,
                                      int   value)
{
    if (tagName == NULL) {
        CLOGE("tagName == NULL. so, fail");
    }

    if (tagBuf == nullptr) {
        CLOGE("tagBuf == NULL. so, fail");
        return;
    }

    snprintf(tagBuf, tagBufSize, "%d", value);

    MAKERS_NOTE_DEBUG_LOG("%s(%s), tagBufSize(%d)",
        tagName, tagBuf, 1);
}

bool ExynosCameraMakersNote::m_flagCreated(void)
{
    return m_flagCreate;
}

void ExynosCameraMakersNote::m_init(void)
{
    m_flagCreate = false;

    this->setName("ExynosCameraMakersNote");
}
