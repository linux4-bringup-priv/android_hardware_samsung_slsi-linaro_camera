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

#ifndef EXYNOS_CAMERA_METADATA_H
#define EXYNOS_CAMERA_METADATA_H

#include <log/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/List.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/String8.h>

#include <hardware/camera3.h>
#include <CameraMetadata.h>

#include <map>
#include <list>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraSensorInfo.h"
#include "fimc-is-metadata.h"

namespace android {

using namespace std;

namespace VENDORMETA {
    enum KEY {
        YUV_STALL_ON,
    };
};

class ExynosCameraVendorMetaData : public virtual RefBase {
public:
    ExynosCameraVendorMetaData(uint32_t key, uint32_t frameCount);
    ~ExynosCameraVendorMetaData();

    /* API for VendorTag Metadata */
    status_t update(uint32_t tag, const uint8_t *data, size_t data_count);
    status_t update(uint32_t tag, const int32_t *data, size_t data_count);
    status_t update(uint32_t tag, const float *data, size_t data_count);
    status_t update(uint32_t tag, const int64_t *data, size_t data_count);
    status_t update(uint32_t tag, const double *data, size_t data_count);
    status_t update(uint32_t tag, const camera_metadata_rational_t *data, size_t data_count);
    status_t update(uint32_t tag, const String8 &string);
    status_t update(const camera_metadata_ro_entry &entry);
    bool exists(uint32_t tag);
    camera_metadata_entry find(uint32_t tag);
    camera_metadata_ro_entry find(uint32_t tag) const;
    status_t erase(uint32_t tag);

    /* API for custeom config Metadata */
    status_t set(VENDORMETA::KEY key, int item);
    status_t set(VENDORMETA::KEY key, bool item);
    status_t get(VENDORMETA::KEY key, int &item);
    status_t get(VENDORMETA::KEY key, bool &item);
    bool find(VENDORMETA::KEY  key);
    bool remove(VENDORMETA::KEY key);

private:
    bool m_find(int key, map<int, int> &data, Mutex &lock);
    status_t m_insert(int key, int item, map<int, int> &data, Mutex &lock);
    status_t m_remove(int key, map<int, int> &data, Mutex &lock);
    status_t m_get(int key, int &item, map<int, int> &data, Mutex &lock);

private:
    map<int, int>   m_vendorConfig;
    mutable Mutex   m_vendorConfigLock;
    uint32_t        m_key;
    uint32_t        m_frameCount;
    CameraMetadata  m_vendorMeta;
    mutable Mutex   m_vendorMetaLock;
};


}; /* namespace android */
#endif
