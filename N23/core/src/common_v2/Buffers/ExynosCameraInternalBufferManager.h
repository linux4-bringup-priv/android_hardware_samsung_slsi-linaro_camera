/*
 * Copyright 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXYNOS_CAMERA_INTERNAL_BUFFER_MANAGER_H__
#define EXYNOS_CAMERA_INTERNAL_BUFFER_MANAGER_H__

#include "ExynosCameraBufferManager.h"

namespace android {

class InternalExynosCameraBufferManager : protected ExynosCameraBufferManager {
public:
    InternalExynosCameraBufferManager(uint32_t maxBufferCount);
    virtual ~InternalExynosCameraBufferManager();

    status_t increase(int increaseCount);
    bool     m_allocationThreadFunc(void);
    status_t alloc(void);
    status_t resetBuffers(void);

protected:
    status_t m_setAllocator(void *allocator);

    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_constructBufferContainer(int bIndex, int eIndex);
    status_t m_destructBufferContainer(int bIndex, int eIndex);

protected:
    typedef ExynosCameraThread<InternalExynosCameraBufferManager> allocThread;
    sp<allocThread>             m_allocationThread;
};

}
#endif