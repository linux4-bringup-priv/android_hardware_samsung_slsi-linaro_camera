/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraVendorMetaData"

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraVendorMetaData.h"

namespace android {

ExynosCameraVendorMetaData::ExynosCameraVendorMetaData(uint32_t key, uint32_t frameCount)
{
    m_key = key;
    m_frameCount = frameCount;
}

ExynosCameraVendorMetaData::~ExynosCameraVendorMetaData()
{
    m_vendorConfig.clear();
    m_vendorMeta.clear();
}

status_t ExynosCameraVendorMetaData::set(VENDORMETA::KEY key, int item)
{
    status_t ret = NO_ERROR;
    ret = m_insert(key, item, m_vendorConfig, m_vendorConfigLock);
    if (ret != NO_ERROR) {
        CLOGE2("[R%d F%d] set API failed key[%d] item[%d]", m_key, m_frameCount, key, item);
    }
    return ret;
}

status_t ExynosCameraVendorMetaData::set(VENDORMETA::KEY key, bool item)
{
    status_t ret = NO_ERROR;
    int value = (item == true)?1:0;
    ret = m_insert(key, value, m_vendorConfig, m_vendorConfigLock);
    if (ret != NO_ERROR) {
        CLOGE2("[R%d F%d] set API failed key[%d] item[%d]", m_key, m_frameCount, key, item);
    }
    return ret;
}

status_t ExynosCameraVendorMetaData::get(VENDORMETA::KEY key, int &item)
{
    status_t ret = NO_ERROR;
    ret = m_get(key, item, m_vendorConfig, m_vendorConfigLock);
    if (ret != NO_ERROR) {
        CLOGE2("[R%d F%d] set API failed key[%d] item[%d]", m_key, m_frameCount, key, item);
    }
    return ret;
}

status_t ExynosCameraVendorMetaData::get(VENDORMETA::KEY key, bool &item)
{
    status_t ret = NO_ERROR;
    int value = 0;
    ret = m_get(key, value, m_vendorConfig, m_vendorConfigLock);
    if (ret != NO_ERROR) {
        CLOGE2("[R%d F%d] set API failed key[%d] item[%d]", m_key, m_frameCount, key, value);
    }
    item = (value == 1)?true:false;
    return ret;
}

bool ExynosCameraVendorMetaData::find(VENDORMETA::KEY key)
{
    bool ret = false;
    ret = m_find(key, m_vendorConfig, m_vendorConfigLock);
    return ret;
}

bool ExynosCameraVendorMetaData::remove(VENDORMETA::KEY key)
{
    bool ret = false;
    ret = m_remove(key, m_vendorConfig, m_vendorConfigLock);
    return ret;
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const uint8_t *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const int32_t *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const float *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const int64_t *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const double *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const camera_metadata_rational_t *data, size_t data_count)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, data, data_count);
}

status_t ExynosCameraVendorMetaData::update(uint32_t tag, const String8 &string)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(tag, string);
}

status_t ExynosCameraVendorMetaData::update(const camera_metadata_ro_entry &entry)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.update(entry);
}

bool ExynosCameraVendorMetaData::exists(uint32_t tag)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.exists(tag);
}

camera_metadata_entry_t ExynosCameraVendorMetaData::find(uint32_t tag)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.find(tag);
}

camera_metadata_ro_entry_t ExynosCameraVendorMetaData::find(uint32_t tag) const
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.find(tag);
}

status_t ExynosCameraVendorMetaData::erase(uint32_t tag)
{
    Mutex::Autolock lock(m_vendorMetaLock);
    return m_vendorMeta.erase(tag);
}

bool ExynosCameraVendorMetaData::m_find(int key, map<int, int> &data, Mutex &lock)
{
    bool ret = true;
    map<int, int>::iterator iter;

    lock.lock();
    iter = data.find(key);
    if (iter == data.end()) {
        ret = false;
    }
    lock.unlock();
    return ret;
}

status_t ExynosCameraVendorMetaData::m_insert(int key, int item, map<int, int> &data, Mutex &lock)
{
    status_t ret = NO_ERROR;
    lock.lock();
    data[key] = item;
    lock.unlock();
    return ret;
}

status_t ExynosCameraVendorMetaData::m_remove(int key, map<int, int> &data, Mutex &lock)
{
    status_t ret = NO_ERROR;
    map<int, int>::iterator iter;
    int item = 0;

    lock.lock();
    iter = data.find(key);
    if (iter != data.end()) {
        item = iter->second;
        data.erase(iter);
    } else {
        ret = BAD_VALUE;
        CLOGE2("[R%d F%d] API failed key[%d] item[%d]", m_key, m_frameCount, key, item);
    }
    lock.unlock();
    return ret;
}

status_t ExynosCameraVendorMetaData::m_get(int key, int &item, map<int, int> &data, Mutex &lock)
{
    status_t ret = NO_ERROR;
    map<int, int>::iterator iter;

    lock.lock();
    iter = data.find(key);
    if (iter != data.end()) {
        item = iter->second;
    } else {
        CLOGE2("[R%d F%d] m_get API failed key[%d] item[%d]", m_key, m_frameCount, key, item);
        ret = BAD_VALUE;
    }
    lock.unlock();
    return ret;
}

};
