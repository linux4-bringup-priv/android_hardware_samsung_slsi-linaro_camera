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

#define LOG_TAG "ExynosCameraBufferManager"
#include "ExynosCameraBufferManager.h"
#include "ExynosGraphicBuffer.h"

namespace android {

status_t ExynosCameraBufferManager::create(const char *name, int cameraId, void *defaultAllocator)
{
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    setCameraId(cameraId);
    strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    if (defaultAllocator == NULL) {
        if (m_createDefaultAllocator(false) != NO_ERROR) {
            CLOGE("m_createDefaultAllocator failed");
            return INVALID_OPERATION;
        }
    } else {
        if (m_setDefaultAllocator(defaultAllocator) != NO_ERROR) {
            CLOGE("m_setDefaultAllocator failed");
            return INVALID_OPERATION;
        }
    }

    return ret;
}

status_t ExynosCameraBufferManager::create(const char *name, void *defaultAllocator)
{
    return create(name, 0, defaultAllocator);
}

void ExynosCameraBufferManager::deinit(void)
{
    if (m_flagAllocated == false) {
        CLOGI("OUT.. Buffer is not allocated");
        return;
    }

    CLOGD("IN..");

#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        int bufIndex;
        std::vector<int>::iterator r;
        for (r = m_allocatedBufferIndexQ.begin(); r != m_allocatedBufferIndexQ.end(); ++r) {
            bufIndex = *r;
            cancelBuffer(bufIndex);
        }
    } else
#endif
    {
        for (int bufIndex = 0; bufIndex < m_allocatedBufCount; bufIndex++)
            cancelBuffer(bufIndex);
    }

    if (m_free() != NO_ERROR)
        CLOGE("free failed");

    if (m_defaultAllocator != NULL && m_isCreateDefaultAllocator == true) {
        delete m_defaultAllocator;
        m_defaultAllocator = NULL;
    }

    m_reservedMemoryCount = 0;
    m_flagSkipAllocation = false;
    CLOGD("OUT..");
}

status_t ExynosCameraBufferManager::setAllocator(void *allocator)
{
    Mutex::Autolock lock(m_lock);

    if (allocator == NULL) {
        CLOGE("m_allocator equals NULL");
        return INVALID_OPERATION;
    }

    return m_setAllocator(allocator);
}

void ExynosCameraBufferManager::m_resetSequenceQ()
{
    Mutex::Autolock lock(m_availableBufferIndexQLock);
    m_availableBufferIndexQ.clear();
#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        int bufIndex;
        std::vector<int>::iterator r;
        for (r = m_allocatedBufferIndexQ.begin(); r!= m_allocatedBufferIndexQ.end(); ++r) {
            bufIndex = *r;
            m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
        }
    } else
#endif
    {
        for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++)
            m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
    }
    for (int bufIndex = 0; bufIndex < VIDEO_MAX_FRAME; bufIndex++)
        m_availableBatchBufferIndexQ.push_back(bufIndex);

    return;
}

void ExynosCameraBufferManager::setContigBufCount(int reservedMemoryCount)
{
    CLOGI("reservedMemoryCount(%d)", reservedMemoryCount);
    m_reservedMemoryCount = reservedMemoryCount;
    return;
}

int ExynosCameraBufferManager::getContigBufCount(void)
{
    return m_reservedMemoryCount;
}

status_t ExynosCameraBufferManager::putBatchBufferIndex(int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    List<int>::iterator r;
    bool found = false;

    if (bufIndex < 0 || VIDEO_MAX_FRAME <= bufIndex) {
        CLOGE("buffer Index in out of bound [bufIndex=%d], allocatedBufCount(%d)",
             bufIndex, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBatchBufferIndexQ.begin(); r != m_availableBatchBufferIndexQ.end(); r++) {
        if (bufIndex == *r) {
            found = true;
            break;
        }
    }
    m_availableBufferIndexQLock.unlock();

    if (found == true) {
        CLOGV("bufIndex=%d is already in (available state)", bufIndex);
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    m_availableBatchBufferIndexQ.push_back(bufIndex);
    m_availableBufferIndexQLock.unlock();

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::getBatchBufferIndex(int &bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    List<int>::iterator r;

    if (m_allocatedBufCount == 0) {
        CLOGE("m_allocatedBufCount equals zero");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    if (m_availableBatchBufferIndexQ.empty() == false) {
        r = m_availableBatchBufferIndexQ.begin();
        bufIndex = *r;
        m_availableBatchBufferIndexQ.erase(r);
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGI("available buffer [index=%d]...", bufIndex);
#endif
    } else {
        CLOGI("not exist bufferIndex ...");
    }
    m_availableBufferIndexQLock.unlock();

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::increase(int increaseCount)
{
    CLOGD("increaseCount(%d) function invalid. Do nothing", increaseCount);
    return NO_ERROR;
}

status_t ExynosCameraBufferManager::cancelBuffer(int bufIndex)
{
    return putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
}

int ExynosCameraBufferManager::getBufStride(void)
{
    return 0;
}

status_t ExynosCameraBufferManager::updateStatus(
        int bufIndex,
        int driverValue,
        enum EXYNOS_CAMERA_BUFFER_POSITION   position,
        enum EXYNOS_CAMERA_BUFFER_PERMISSION permission)
{
    if (bufIndex < 0) {
        CLOGE("Invalid buffer index %d", bufIndex);
        return BAD_VALUE;
    }

    m_buffer[bufIndex].index = bufIndex;
    m_buffer[bufIndex].status.driverReturnValue = driverValue;
    m_buffer[bufIndex].status.position          = position;
    m_buffer[bufIndex].status.permission        = permission;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::getStatus(
        int bufIndex,
        struct ExynosCameraBufferStatus *bufStatus)
{
    *bufStatus = m_buffer[bufIndex].status;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::getIndexByFd(int fd, int *index)
{
    if (fd < 0) {
        CLOGE("Invalid FD %d", fd);
        return BAD_VALUE;
    }

    *index = -1;
    for (int bufIndex = m_indexOffset; bufIndex < m_reqBufCount + m_indexOffset; bufIndex++) {
        if (m_buffer[bufIndex].fd[0] == fd) {
            *index = bufIndex;
            break;
        }
    }

    if (*index < 0 || *index > m_allowedMaxBufCount + m_indexOffset) {
        CLOGE("Invalid buffer index %d. fd %d", *index, fd);

        *index = -1;
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

bool ExynosCameraBufferManager::isAllocated(void)
{
    return m_flagAllocated;
}

bool ExynosCameraBufferManager::isAvailable(int bufIndex)
{
    bool ret = false;

    switch (m_buffer[bufIndex].status.permission) {
    case EXYNOS_CAMERA_BUFFER_PERMISSION_NONE:
    case EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE:
        ret = true;
        break;

    case EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS:
    default:
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("buffer is not available");
        dump();
#endif
        ret = false;
        break;
    }

    return ret;
}

status_t ExynosCameraBufferManager::m_setDefaultAllocator(void *allocator)
{
    m_defaultAllocator = (ExynosCameraIonAllocator *)allocator;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::m_createDefaultAllocator(bool isCached)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    m_defaultAllocator = new ExynosCameraIonAllocator();
    m_isCreateDefaultAllocator = true;
    if (m_defaultAllocator->init(isCached) != NO_ERROR) {
        CLOGE("m_defaultAllocator->init failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

int ExynosCameraBufferManager::getAllocatedBufferCount(void)
{
    return m_allocatedBufCount;
}

int ExynosCameraBufferManager::getNumOfAvailableAndNoneBuffer(void)
{
    int numAvailable = 0;

    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        if (m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE ||
            m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_NONE)
            numAvailable++;
    }

    return numAvailable;
}

void ExynosCameraBufferManager::printBufferInfo(
        __unused const char *funcName,
        __unused const int lineNum,
        int bufIndex,
        int planeIndex)
{
    CLOGI("[m_buffer[%d].fd[%d]=%d] .addr=%p .size=%d]",
        bufIndex, planeIndex,
        m_buffer[bufIndex].fd[planeIndex],
        m_buffer[bufIndex].addr[planeIndex],
        m_buffer[bufIndex].size[planeIndex]);

    return;
}

void ExynosCameraBufferManager::dump(int fd)
{
    if (fd > 0) {
        /* TODO: Add code to write information if needed */
    }

    CLOGD("----- dump buffer status -----");
    printBufferState();
    printBufferQState();

    return;
}

status_t ExynosCameraBufferManager::setBufferCount(__unused int bufferCount)
{
    CLOGD("");

    return NO_ERROR;
}

int ExynosCameraBufferManager::getBufferCount(void)
{
    CLOGV("");

    return 0;
}

ExynosCameraFence::ExynosCameraFence()
{
    m_fenceType = EXYNOS_CAMERA_FENCE_TYPE_BASE;

    m_acquireFence = -1;
    m_releaseFence = -1;

    m_fence = 0;

    m_flagSwfence = false;
}

ExynosCameraFence::ExynosCameraFence(
        enum EXYNOS_CAMERA_FENCE_TYPE fenceType,
        int acquireFence,
        int releaseFence)
{
    /* default setting */
    m_fenceType = EXYNOS_CAMERA_FENCE_TYPE_BASE;
    m_acquireFence = -1;
    m_releaseFence = -1;
    m_fence = 0;
    m_flagSwfence = false;

    /* we will set from here */
    if (fenceType <= EXYNOS_CAMERA_FENCE_TYPE_BASE ||
            EXYNOS_CAMERA_FENCE_TYPE_MAX <= fenceType) {
        ALOGE("ERR(%s[%d]):Invalid fenceType(%d)",
                __FUNCTION__, __LINE__, fenceType);
        return;
    }

    m_fenceType = fenceType;
    m_acquireFence = acquireFence;
    m_releaseFence = releaseFence;

    if (0 <= m_acquireFence || 0 <= m_releaseFence) {
        ALOGV("DEBUG(%s[%d]):m_acquireFence(%d), m_releaseFence(%d)",
                __FUNCTION__, __LINE__, m_acquireFence, m_releaseFence);
    }

#ifdef USE_SW_FENCE
    m_flagSwfence = true;
#endif
    if (m_flagSwfence == true) {
        switch (m_fenceType) {
        case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
            m_fence = new Fence(acquireFence);
            break;
        case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
            m_fence = new Fence(releaseFence);
            break;
        default:
            ALOGE("ERR(%s[%d]):invalid m_fenceType(%d)",
                    __FUNCTION__, __LINE__, m_fenceType);
            break;
        }
    }
}

ExynosCameraFence::~ExynosCameraFence()
{
    /* delete sp<Fence> addr */
    m_fence = 0;
#ifdef FORCE_CLOSE_ACQUIRE_FD
    static uint64_t closeCnt = 0;
    if(m_acquireFence >= FORCE_CLOSE_ACQUIRE_FD_THRESHOLD) {
        if(closeCnt++ % 1000 == 0) {
            CLOGW2("Attempt to close acquireFence[%d], %ld th close.",
                     m_acquireFence, (long)closeCnt);
        }
        ::close(m_acquireFence);
    }
#endif
}

int ExynosCameraFence::getFenceType(void)
{
    return m_fenceType;
}

int ExynosCameraFence::getAcquireFence(void)
{
    return m_acquireFence;
}

int ExynosCameraFence::getReleaseFence(void)
{
    return m_releaseFence;
}

bool ExynosCameraFence::isValid(void)
{
    bool ret = false;

    if (m_flagSwfence == true) {
        if (m_fence == NULL) {
            CLOGE2("m_fence == NULL. so, fail");
            ret = false;
        } else {
            ret = m_fence->isValid();
        }
    } else {
        switch (m_fenceType) {
        case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
            if (0 <= m_acquireFence)
              ret = true;
            break;
        case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
            if (0 <= m_releaseFence)
              ret = true;
            break;
        default:
            CLOGE2("invalid m_fenceType(%d)",
                 m_fenceType);
            break;
        }
    }

    return ret;
}

status_t ExynosCameraFence::wait(int time)
{
    status_t ret = NO_ERROR;

    if (this->isValid() == false) {
        CLOGE2("this->isValid() == false. so, fail!! fencType(%d)",
                m_fenceType);
        return INVALID_OPERATION;
    }

    if (m_flagSwfence == false) {
        CLOGW2("m_flagSwfence == false. so, fail!! fencType(%d)",
                m_fenceType);

        return INVALID_OPERATION;
    }

    int waitTime = time;
    if (waitTime < 0)
        waitTime = 1000; /* wait 1 sec */

    int fenceFd = -1;

    switch (m_fenceType) {
    case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
        fenceFd = m_acquireFence;
        break;
    case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
        fenceFd = m_releaseFence;
        break;
    default:
        CLOGE2("invalid m_fenceType(%d)",
             m_fenceType);
        break;
    }

    ret = m_fence->wait(waitTime);
    if (ret == TIMED_OUT) {
        CLOGE2("Fence timeout. so, fail!! fenceFd(%d), fencType(%d)",
             fenceFd, m_fenceType);

        return INVALID_OPERATION;
    } else if (ret != OK) {
        CLOGE2("Fence wait error. so, fail!! fenceFd(%d), fencType(%d)",
             fenceFd, m_fenceType);

        return INVALID_OPERATION;
    }

    return ret;
}

} // namespace android
