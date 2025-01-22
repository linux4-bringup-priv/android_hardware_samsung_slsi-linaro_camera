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
uint64_t ExynosCameraBufferManager::m_sReservedBufferSize = 0;
uint64_t ExynosCameraBufferManager::m_sIonBufferSize = 0;

namespace android {

ExynosCameraBufferManager::ExynosCameraBufferManager(uint32_t maxBufferCount):m_maxBufferCount(maxBufferCount)
{
    m_isDestructor = false;
    m_cameraId = 0;

    m_buffer = nullptr;
    m_buffer = new ExynosCameraBuffer[m_maxBufferCount];
    if (m_buffer == nullptr) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]): fail to allocate m_buffer", __FUNCTION__, __LINE__);
    }

    init();
}

ExynosCameraBufferManager::~ExynosCameraBufferManager()
{
    CLOGD("");
    m_isDestructor = true;

    if (m_buffer) {
        delete[] m_buffer;
        m_buffer = nullptr;
    }
}

void ExynosCameraBufferManager::init(void)
{
    CLOGD("");
    EXYNOS_CAMERA_BUFFER_IN();

    m_flagAllocated = false;
    m_reservedMemoryCount = 0;
    m_reqBufCount  = 0;
    m_allocatedBufCount  = 0;
    m_allowedMaxBufCount = 0;
    m_defaultAllocator = NULL;
    m_isCreateDefaultAllocator = false;
    for (int bufIndex = 0; bufIndex < m_maxBufferCount; bufIndex++) {
        for (int planeIndex = 0; planeIndex < EXYNOS_CAMERA_BUFFER_MAX_PLANES; planeIndex++) {
            m_buffer[bufIndex].fd[planeIndex] = -1;
        }
    }
    m_hasMetaPlane = false;
    m_hasDebugInfoPlane = false;
    memset(m_name, 0x00, sizeof(m_name));
    strncpy(m_name, "none", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_flagSkipAllocation = false;
    m_flagNeedMmap = false;
    m_allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
    m_indexOffset = 0;

    m_bufferManagerType = BUFFER_MANAGER_INVALID_TYPE;

    EXYNOS_CAMERA_BUFFER_OUT();
}

status_t ExynosCameraBufferManager::m_free(void)
{
    EXYNOS_CAMERA_BUFFER_IN();
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    Mutex::Autolock lock(m_lock);

    CLOGD("Free the buffer (m_allocatedBufCount=%d) --- dumpBufferInfo ---", m_allocatedBufCount);
#ifdef EXYNOS_CAMERA_DUMP_BUFFER_INFO
    dumpBufferInfo();
    CLOGD("------------------------------------------------------");
#endif

    status_t ret = NO_ERROR;

    if (m_flagAllocated != false) {
        if (m_free(m_indexOffset, m_allocatedBufCount + m_indexOffset) != NO_ERROR) {
            CLOGE("m_free failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_hasMetaPlane == true) {
            if (m_defaultFree(m_indexOffset, m_allocatedBufCount + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultFree failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        if (m_hasDebugInfoPlane == true) {
            if (m_defaultFree(m_indexOffset, m_allocatedBufCount + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultFree failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }
        m_availableBufferIndexQLock.lock();
        m_availableBufferIndexQ.clear();
        m_availableBufferIndexQLock.unlock();
        m_allocatedBufCount  = 0;
        m_allowedMaxBufCount = 0;
        m_flagAllocated = false;
    }

    CLOGD("Free the buffer succeeded (m_allocatedBufCount=%d)", m_allocatedBufCount);

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::setInfo(buffer_manager_configuration_t info)
{
    return m_setInfo(info, false);
}

/*  If Image buffer color format equals YV12, and buffer has MetaDataPlane..

    planeCount = 4      (set by user)
    size[0] : Image buffer plane Y size
    size[1] : Image buffer plane Cr size
    size[2] : Image buffer plane Cb size

    if (createMetaPlane == true)
        size[3] = EXYNOS_CAMERA_META_PLANE_SIZE;    (set by BufferManager, internally)
*/
status_t ExynosCameraBufferManager::m_setInfo(buffer_manager_configuration_t info, bool isServiceBuffer)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    int totalPlaneCount = 0;
    int metaPlaneIndex  = -1;
    int debugPlaneIndex = -1;
    int index           = 0;

    m_config = info;

    if (m_indexOffset > 0) {
        CLOGD("buffer indexOffset(%d), Index[0 - %d] not used", m_indexOffset, m_indexOffset);
    }
    m_indexOffset = info.startBufIndex;

    if (info.allowedMaxBufCount < info.reqBufCount) {
        CLOGW("abnormal value [reqBufCount=%d, allowedMaxBufCount=%d]",
                info.reqBufCount, info.allowedMaxBufCount);
        info.allowedMaxBufCount = info.reqBufCount;
    }

    if (info.reqBufCount < 0 || m_maxBufferCount < info.reqBufCount) {
        CLOGE("abnormal value [reqBufCount=%d]", info.reqBufCount);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (info.planeCount < 0 || EXYNOS_CAMERA_BUFFER_MAX_PLANES <= info.planeCount) {
        CLOGE("abnormal value [planeCount=%d]", info.planeCount);
        ret = BAD_VALUE;
        goto func_exit;
    }

    totalPlaneCount = m_getTotalPlaneCount(info.planeCount, info.batchSize, info.createMetaPlane, info.createDebugInfoPlane);
    if (totalPlaneCount < 1 || EXYNOS_CAMERA_BUFFER_MAX_PLANES < totalPlaneCount) {
        CLOGE("Failed to getTotalPlaneCount." \
                "totalPlaneCount %d planeCount %d batchSize %d hasMetaPlane %d, hasDebugInfoPlane %d",
                totalPlaneCount, info.planeCount, info.batchSize, info.createMetaPlane,info.createDebugInfoPlane);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    index = info.planeCount-1;
    if (info.createDebugInfoPlane == true) {
        debugPlaneIndex = index;
        info.size[index--] = EXYNOS_CAMERA_DEBUG_INFO_PLANE_SIZE;
        m_hasDebugInfoPlane = true;
    }

    if (info.createMetaPlane == true) {
        metaPlaneIndex =index;
        info.size[index] = EXYNOS_CAMERA_META_PLANE_SIZE;
        m_hasMetaPlane = true;
    }

    for (int bufIndex = m_indexOffset; bufIndex < info.allowedMaxBufCount + m_indexOffset; bufIndex++) {
        for (int planeIndex = 0; planeIndex < info.planeCount; planeIndex++) {
            int curPlaneIndex = planeIndex;
            if (info.size[planeIndex] <= 0) {
                CLOGE("abnormal value [size=%d, planeIndex %d]",
                        info.size[planeIndex], planeIndex);
                ret = BAD_VALUE;
                goto func_exit;
            }

            //Transfer debugInfo plane index in case of batch mode
            if (info.createDebugInfoPlane == true && planeIndex == debugPlaneIndex)
                curPlaneIndex=totalPlaneCount - 1;

            //Transfer meta plane index in case of batch mode
            if(info.createMetaPlane == true && planeIndex == metaPlaneIndex){
               if (info.createDebugInfoPlane == true )
                   curPlaneIndex = totalPlaneCount - 2;
               else
                   curPlaneIndex = totalPlaneCount - 1;
            }
            m_buffer[bufIndex].size[curPlaneIndex]         = info.size[planeIndex];
            m_buffer[bufIndex].bytesPerLine[curPlaneIndex] = info.bytesPerLine[planeIndex];

#ifdef USE_GIANT_MSCL
            if (metaPlaneIndex != planeIndex
                && debugPlaneIndex != planeIndex
                && m_buffer[bufIndex].size[0] >= GIANT_MIN_SIZE
                && isServiceBuffer == false) {

                m_buffer[bufIndex].size[curPlaneIndex] += GIANT_MARIN_SIZE;
            }
#endif
        }

        /* Copy image plane imformation into other planes in batch buffer */
        if (info.batchSize > 1) {
            int imagePlaneCount = m_getImagePlaneCount(info.planeCount);

            for (int batchIndex = 1; batchIndex < info.batchSize; batchIndex++) {
                for (int planeIndex = 0; planeIndex < imagePlaneCount; planeIndex++) {
                    int curPlaneIndex = (batchIndex * imagePlaneCount) + planeIndex;
                    m_buffer[bufIndex].size[curPlaneIndex]          = info.size[planeIndex];
                    m_buffer[bufIndex].bytesPerLine[curPlaneIndex]  = info.bytesPerLine[planeIndex];
                }
            }
        }

        m_buffer[bufIndex].planeCount = info.planeCount;
        m_buffer[bufIndex].type       = info.type;
        m_buffer[bufIndex].batchSize  = info.batchSize;
        m_buffer[bufIndex].bufMgrNm   = m_name; /* for debug */
        m_buffer[bufIndex].hasMetaPlane = m_hasMetaPlane;
        m_buffer[bufIndex].hasDebugInfoPlane = m_hasDebugInfoPlane;
        m_buffer[bufIndex].refCount   = 0;
        m_buffer[bufIndex].v4l2Flags  = info.v4l2Flags;
    }

    m_allowedMaxBufCount    = info.allowedMaxBufCount + info.startBufIndex;
    m_reqBufCount           = info.reqBufCount;
    m_flagNeedMmap          = info.needMmap;
    m_allocMode             = info.allocMode;
    m_reservedMemoryCount   = info.reservedMemoryCount;
func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::increaseBufferRefCnt(int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    Mutex::Autolock lock(m_lock);
    m_buffer[bufIndex].refCount++;

    EXYNOS_CAMERA_BUFFER_OUT();

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::putBuffer(
        int bufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    std::list<int>::iterator r;
    bool found = false;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;

    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;

    if (bufIndex < 0) {
        CLOGE("buffer Index is invalid [bufIndex=%d]", bufIndex);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocMode != BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE && m_allocatedBufCount + m_indexOffset <= bufIndex) {
        CLOGE("buffer Index in out of bound [bufIndex=%d], allocatedBufCount(%d)",
             bufIndex, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        if (bufIndex == *r) {
            found = true;
            break;
        }
    }
    m_availableBufferIndexQLock.unlock();

    if (found == true) {
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("bufIndex=%d is already in (available state)", bufIndex);
#endif
        goto func_exit;
    }
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    else {
        CLOGD("bufIndex=%d return", bufIndex);
    }
#endif

    if (--m_buffer[bufIndex].refCount > 0) {
        CLOGD("skip put buffer. bufRefCount %d, IDX:%d", m_buffer[bufIndex].refCount, bufIndex);
        goto func_exit;
    }

    if (m_buffer[bufIndex].refCount < 0) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]): invalid refCount(%d), index:%d",
                __FUNCTION__, __LINE__, m_buffer[bufIndex].refCount, bufIndex);
    }

    if (updateStatus(bufIndex, 0, position, permission) != NO_ERROR) {
        CLOGE("setStatus failed [bufIndex=%d, position=%d, permission=%d]",
             bufIndex, (int)position, (int)permission);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {

        if (bufIndex >= m_reqBufCount && bufIndex == m_allocatedBufCount - 1) {
            int stIndex = bufIndex;
            std::list<int>::reverse_iterator rit;

            m_availableBufferIndexQLock.lock();
            m_availableBufferIndexQ.sort();

            if (m_availableBufferIndexQ.size() > 0) {
                for (rit = m_availableBufferIndexQ.rbegin(); rit != m_availableBufferIndexQ.rend() && stIndex >= m_reqBufCount; ++rit) {
                    stIndex--;
                    if (stIndex == *rit) {
                        if (*rit == *(m_availableBufferIndexQ.begin()) && stIndex > m_reqBufCount) {
                            stIndex--;
                        }
                        continue;
                    } else {
                        break;
                    }
                }
                stIndex++;

                for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); ) {
                    if (stIndex <= *r) {
                        r = m_availableBufferIndexQ.erase(r);
                        m_allocatedBufCount--;
                    } else {
                        ++r;
                    }
                }
            }

            /* Cur buffer  */
            m_allocatedBufCount--;

            m_availableBufferIndexQLock.unlock();

            CLOGI("Decrease bufIndex(%d - %d), m_reqBufCount(%d), m_allocatedBufCount(%d), m_indexOffset(%d)",
                    stIndex, bufIndex, m_reqBufCount, m_allocatedBufCount, m_indexOffset);

            if (m_free(stIndex, bufIndex + 1) != NO_ERROR) {
                CLOGE("m_free failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }

            if (m_hasMetaPlane == true) {
                if (m_defaultFree(stIndex, bufIndex + 1, m_hasMetaPlane) != NO_ERROR) {
                    CLOGE("m_defaultFree failed");
                    ret = INVALID_OPERATION;
                    goto func_exit;
                }
            }
        } else {
            m_availableBufferIndexQLock.lock();
            m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);

            CLOGI("[available] Decrease bufIndex(%d/%d), m_reqBufCount(%d), m_allocatedBufCount(%d), m_indexOffset(%d), available(%zu)",
                bufIndex, m_buffer[bufIndex].index, m_reqBufCount, m_allocatedBufCount, m_indexOffset,
                m_availableBufferIndexQ.size());
            m_availableBufferIndexQLock.unlock();
        }
    } else {
        m_availableBufferIndexQLock.lock();
        m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
        m_availableBufferIndexQLock.unlock();
    }


func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

/* User Process need to check the index of buffer returned from "getBuffer()" */
status_t ExynosCameraBufferManager::getBuffer(
        int  *reqBufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position,
        struct ExynosCameraBuffer *buffer)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    std::list<int>::iterator r;

    int  bufferIndex;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;

    bufferIndex = *reqBufIndex;
    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_NONE;

    if (m_allocMode != BUFFER_MANAGER_ALLOCATION_ONDEMAND
        && m_allocMode != BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        if (m_allocatedBufCount == 0) {
            CLOGE("m_allocatedBufCount equals zero");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

reDo:
    if (bufferIndex < 0 || m_allocatedBufCount + m_indexOffset <= bufferIndex) {
        /* find availableBuffer */
        m_availableBufferIndexQLock.lock();
        if (m_availableBufferIndexQ.empty() == false) {
            r = m_availableBufferIndexQ.begin();
            bufferIndex = *r;
            m_availableBufferIndexQ.erase(r);
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGI("available buffer [index=%d]...", bufferIndex);
#endif
        }
        m_availableBufferIndexQLock.unlock();
    } else {
        m_availableBufferIndexQLock.lock();
        /* get the Buffer of requested */
        for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
            if (bufferIndex == *r) {
                m_availableBufferIndexQ.erase(r);
                break;
            }
        }
        m_availableBufferIndexQLock.unlock();
    }

    if (0 <= bufferIndex && bufferIndex < m_allocatedBufCount + m_indexOffset) {
        /* found buffer */
        if (isAvailable(bufferIndex) == false) {
            CLOGE("isAvailable failed [bufferIndex=%d]", bufferIndex);
            ret = BAD_VALUE;
            goto func_exit;
        }

        permission = EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS;

        if (updateStatus(bufferIndex, 0, position, permission) != NO_ERROR) {
            CLOGE("setStatus failed [bIndex=%d, position=%d, permission=%d]",
                 bufferIndex, (int)position, (int)permission);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    } else {
        /* do not find buffer */
        if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND
            || m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE
            ) {
            /* increase buffer*/
            ret = m_increase(1);
            if (ret < 0) {
                CLOGE("increase the buffer failed, m_allocatedBufCount %d, bufferIndex %d",
                      m_allocatedBufCount, bufferIndex);
            } else {
                m_availableBufferIndexQLock.lock();
                m_availableBufferIndexQ.push_back(m_allocatedBufCount + m_indexOffset);
                m_availableBufferIndexQLock.unlock();
                bufferIndex = m_allocatedBufCount + m_indexOffset;
                m_allocatedBufCount++;

#ifdef EXYNOS_CAMERA_DUMP_BUFFER_INFO
                dumpBufferInfo();
#endif
                CLOGI("increase the buffer succeeded (bufferIndex=%d)", bufferIndex);
                goto reDo;
            }
        } else {
            if (m_allocatedBufCount == 1 && isAvailable(0))
                bufferIndex = 0;
            else
                ret = INVALID_OPERATION;
        }

        if (ret < 0) {
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGD("find free buffer... failed --- dump ---");
            dump();
            CLOGD("----------------------------------------");
            CLOGD("buffer Index in out of bound [bufferIndex=%d]", bufferIndex);
#endif
            ret = BAD_VALUE;
            goto func_exit;
        }
    }

    m_buffer[bufferIndex].index = bufferIndex;
    m_buffer[bufferIndex].refCount++;
    *reqBufIndex = bufferIndex;
    *buffer      = m_buffer[bufferIndex];

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::m_defaultAlloc(int bIndex, int eIndex, unsigned int extraPlane)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int planeIndexStart = 0;
    int planeIndexEnd   = 0;
    bool mapNeeded      = false;
#ifdef DEBUG_RAWDUMP
    char enableRawDump[PROP_VALUE_MAX];
#endif /* DEBUG_RAWDUMP */

    int mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
    int flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;

    ExynosCameraDurationTimer timer;
    long long    durationTime = 0;
    long long    durationTimeSum = 0;
    unsigned int estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
    unsigned int estimatedTime = 0;
    unsigned int bufferSize = 0;
    int          reservedMaxCount = 0;
    int          bufIndex = 0;
    bool         isMetaPlane  = false;
    bool         isDebugPlane = false;
    bool         isImagePlane = false;

    if (m_defaultAllocator == NULL) {
        CLOGE("m_defaultAllocator equals NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bIndex < 0 || eIndex < 0) {
        CLOGE("Invalid index parameters. bIndex %d eIndex %d", bIndex, eIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    isMetaPlane = ((extraPlane & META_PLANE_MASK) >> META_PLANE_SHIFT) ? true : false;
    isDebugPlane = ((extraPlane & DEBUG_INFO_PLANE_MASK) >> DEBUG_INFO_PLANE_SHIFT) ? true : false;

    //CLOGV("NewPlane: extraPlane=0x%x isMetaPlane %d isDebugPlane %d",extraPlane, isMetaPlane, isDebugPlane);

    if (isMetaPlane == true || isDebugPlane == true) {
        mapNeeded = true;
    } else {
#if defined(DEBUG_RAWDUMP) || defined(DEBUG_DUMP_IMAGE) || defined(YUV_DUMP)
        mapNeeded = true;
#else
        mapNeeded = m_flagNeedMmap;
#endif
        isImagePlane = true;
    }

    for (bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (isMetaPlane == true) {
            planeIndexStart = m_buffer[bufIndex].getMetaPlaneIndex();
            planeIndexEnd   = planeIndexStart + 1;
            mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
            flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
            estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
        } else if (isDebugPlane == true) {
            planeIndexStart = m_buffer[bufIndex].getDebugInfoPlaneIndex();
            planeIndexEnd   = planeIndexStart + 1;
            mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
            flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
            estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
        } else {
            planeIndexStart = 0;
            planeIndexEnd   = m_buffer[bufIndex].getLastImagePlaneIndex() + 1;

            switch (m_buffer[bufIndex].type) {
            case EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE:
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE:
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE:
            /* case EXYNOS_CAMERA_BUFFER_ION_NONCACHED_RESERVED_TYPE: */
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), non-cached",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);
                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d),"
                        "non-cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);
                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE:
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), cached",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED | EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d),"
                        "cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE:
                ALOGD("SYNC_FORCE_CACHED");
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED_SYNC_FORCE;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_RESERVED_TYPE:
                ALOGD("SYNC_FORCE_CACHED_RESERVED");
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), cached",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED | EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d),"
                        "cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED_SYNC_FORCE;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_ION_RESERVED_SECURE_TYPE:
                CLOGI("bufIndex(%d), m_reservedMemoryCount(%d), non-cached",
                    bufIndex, m_reservedMemoryCount);
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_SECURE;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_SECURE;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;

                mapNeeded = m_flagNeedMmap;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_SECURE_TYPE:
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), cached",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_SECURE;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_SECURE | EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d),"
                        "cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_INVALID_TYPE:
            default:
                CLOGE("buffer type is invaild (%d)", (int)m_buffer[bufIndex].type);
                break;
            }
        }

        if (isImagePlane == true) {
            timer.start();
            bufferSize = 0;
        }

        for (int planeIndex = planeIndexStart; planeIndex < planeIndexEnd; planeIndex++) {
            if (m_buffer[bufIndex].fd[planeIndex] >= 0) {
                CLOGE("buffer[%d].fd[%d] = %d already allocated",
                         bufIndex, planeIndex, m_buffer[bufIndex].fd[planeIndex]);
                continue;
            }

            if (m_defaultAllocator->alloc(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    mask,
                    flags,
                    mapNeeded) != NO_ERROR) {
#if defined(RESERVED_MEMORY_ENABLE) && defined(RESERVED_MEMORY_REALLOC_WITH_ION)
                if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE) {
                    CLOGE("Realloc with ion:bufIndex(%d), reserved(req count(%d) / allocated(%lluKB),"
                        " non-cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, m_reservedMemoryCount, (unsigned long long)(m_sReservedBufferSize/(uint64_t)1024));

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
                } else if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE) {
                    CLOGE("Realloc with ion:bufIndex(%d), reserved(req count(%d) / allocated(%lluKB),"
                        " cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, m_reservedMemoryCount, (unsigned long long)(m_sReservedBufferSize/(uint64_t)1024));

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                }  else if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_RESERVED_TYPE) {
                    CLOGE("Realloc with ion:bufIndex(%d), reserved(req count(%d) / allocated(%lluKB),"
                        " force sync cached. so, alloc ion memory instead of reserved memory",
                        bufIndex, m_reservedMemoryCount, (unsigned long long)(m_sReservedBufferSize/(uint64_t)1024));

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED_SYNC_FORCE;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                } else {
                    CLOGE("m_defaultAllocator->alloc(bufIndex=%d, planeIndex=%d, size=%d) failed",
                        bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);

                    ret = INVALID_OPERATION;
                    goto func_exit;
                }

                if (m_defaultAllocator->alloc(
                        m_buffer[bufIndex].size[planeIndex],
                        &(m_buffer[bufIndex].fd[planeIndex]),
                        &(m_buffer[bufIndex].addr[planeIndex]),
                        mask,
                        flags,
                        mapNeeded) != NO_ERROR) {
                    CLOGE("m_defaultAllocator->alloc(bufIndex=%d, planeIndex=%d, size=%d) failed",
                        bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);
                    ret = INVALID_OPERATION;
                    goto func_exit;
                }

#else
                CLOGE("m_defaultAllocator->alloc(bufIndex=%d, planeIndex=%d, size=%d) failed",
                    bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);

                ret = INVALID_OPERATION;
                goto func_exit;
#endif
            } else {
                if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE
                    || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE
                    || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_RESERVED_TYPE) {
                    m_sReservedBufferSize += m_buffer[bufIndex].size[planeIndex];
                } else if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE
                    || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE
                    || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE) {
                    m_sIonBufferSize += m_buffer[bufIndex].size[planeIndex];
                }
            }

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            printBufferInfo(__FUNCTION__, __LINE__, bufIndex, planeIndex);
#endif
            if (isImagePlane == true) {
                bufferSize = bufferSize + m_buffer[bufIndex].size[planeIndex];
            }
        }

        if (isImagePlane == true) {
            timer.stop();
            durationTime = timer.durationMsecs();
            durationTimeSum += durationTime;
            CLOGD("duration time(%5d msec):(type=%d, bufIndex=%d, size=%.2f(%d), batchSize=%d, fd=[%d,%d,%d])",
                 (int)durationTime, m_buffer[bufIndex].type, bufIndex,
                 (float)bufferSize / (float)(1024 * 1024), bufferSize, m_buffer[bufIndex].batchSize,
                 m_buffer[bufIndex].fd[0], m_buffer[bufIndex].fd[1], m_buffer[bufIndex].fd[2]);

            estimatedTime = estimatedBase * bufferSize / EXYNOS_CAMERA_BUFFER_1MB;
            if (estimatedTime < durationTime) {
                CLOGW("estimated time(%5d msec):(type=%d, bufIndex=%d, size=%d)",
                     (int)estimatedTime, m_buffer[bufIndex].type, bufIndex, (int)bufferSize);
            }
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE) != NO_ERROR) {
            CLOGE("setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                 bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    if ((planeIndexEnd - planeIndexStart) == 1) {
        CLOGD("Duration time of buffer(Plane:%d) allocation(%5d msec)", planeIndexStart, (int)durationTimeSum);
    } else if ((planeIndexEnd - planeIndexStart) > 1) {
        CLOGD("Duration time of buffer(Plane:%d~%d) allocation(%5d msec)",planeIndexStart, (planeIndexEnd - 1), (int)durationTimeSum);
    }

    CLOGD("Allocated Buffer Size Ion(%ju), Reserved(%ju)", m_sIonBufferSize, m_sReservedBufferSize);

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    if (bufIndex < eIndex) {
        if (m_defaultFree(0, bufIndex, extraPlane) != NO_ERROR) {
            CLOGE("m_defaultFree failed");
        }
    }
    return ret;
}

status_t ExynosCameraBufferManager::m_defaultFree(int bIndex, int eIndex, unsigned int extraPlane)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int bufIndex = 0;
    int planeIndexStart = 0;
    int planeIndexEnd   = 0;
    bool mapNeeded      = false;
#ifdef DEBUG_RAWDUMP
    char enableRawDump[PROP_VALUE_MAX];
#endif /* DEBUG_RAWDUMP */
    bool isMetaPlane, isDebugPlane;

    isMetaPlane = ((extraPlane & META_PLANE_MASK) >> META_PLANE_SHIFT) ? true : false;
    isDebugPlane = ((extraPlane & DEBUG_INFO_PLANE_MASK) >> DEBUG_INFO_PLANE_SHIFT) ? true : false ;

    //CLOGV("NewPlane:extraPlane=0x%x isMetaPlane %d isDebugPlane %d",extraPlane, isMetaPlane, isDebugPlane);

    if (isMetaPlane == true || isDebugPlane == true) {
        mapNeeded = true;
    } else {
#if defined(DEBUG_RAWDUMP) || defined(DEBUG_DUMP_IMAGE) || defined(YUV_DUMP)
        mapNeeded = true;
#else
        mapNeeded = m_flagNeedMmap;
#endif
  }

    for (bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (isAvailable(bufIndex) == false) {
            CLOGE("buffer [bufIndex=%d] in InProcess state", bufIndex);
            if (m_isDestructor == false) {
                ret = BAD_VALUE;
                continue;
            } else {
                CLOGE("buffer [bufIndex=%d] in InProcess state, but try to forcedly free", bufIndex);
            }
        }

        if (isMetaPlane == true) {
            planeIndexStart = m_buffer[bufIndex].getMetaPlaneIndex();
            planeIndexEnd   = planeIndexStart + 1;
        } else if (isDebugPlane == true) {
            planeIndexStart = m_buffer[bufIndex].getDebugInfoPlaneIndex();
            planeIndexEnd   = planeIndexStart + 1;
        } else {
            planeIndexStart = 0;
            planeIndexEnd   = m_buffer[bufIndex].getLastImagePlaneIndex() + 1;
        }

        if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_RESERVED_SECURE_TYPE) {
            mapNeeded = m_flagNeedMmap;
        }

        for (int planeIndex = planeIndexStart; planeIndex < planeIndexEnd; planeIndex++) {
            if (m_defaultAllocator->free(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    mapNeeded) != NO_ERROR) {
                CLOGE("m_defaultAllocator->free for Imagedata Plane failed." \
                        "bufIndex %d planeIndex %d fd %d addr %p",
                    bufIndex, planeIndex,
                    m_buffer[bufIndex].fd[planeIndex],
                    m_buffer[bufIndex].addr[planeIndex]);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
            if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE
                || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE
                || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_RESERVED_TYPE) {
                m_sReservedBufferSize -= m_buffer[bufIndex].size[planeIndex];
            } else if (m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE
                || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE
                || m_buffer[bufIndex].type == EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE) {
                m_sIonBufferSize -= m_buffer[bufIndex].size[planeIndex];
            }
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("setStatus failed [bIndex=%d, position=NONE, permission=NONE]", bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    CLOGD("Remained Buffer Size Ion(%ju), Reserved(%ju)", m_sIonBufferSize, m_sReservedBufferSize);

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

bool ExynosCameraBufferManager::m_checkInfoForAlloc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();

    bool ret = true;

    if (m_reqBufCount < 0 || m_maxBufferCount < m_reqBufCount) {
        CLOGE("buffer Count in out of bound [m_reqBufCount=%d]", m_reqBufCount);
        ret = false;
        goto func_exit;
    }

    for (int bufIndex = m_indexOffset; bufIndex < m_reqBufCount + m_indexOffset; bufIndex++) {
        if (m_buffer[bufIndex].planeCount < 0
         || VIDEO_MAX_PLANES <= m_buffer[bufIndex].planeCount) {
            CLOGE("plane Count in out of bound [m_buffer[bIndex].planeCount=%d]",
                 m_buffer[bufIndex].planeCount);
            ret = false;
            goto func_exit;
        }

        for (int planeIndex = 0; planeIndex < m_buffer[bufIndex].planeCount; planeIndex++) {
             int curPlaneIndex = planeIndex;
             int imagePlaneCount = m_getImagePlaneCount( m_buffer[bufIndex].planeCount);
             //Convert to metaPlaneIndex.
             if (m_hasMetaPlane == true && planeIndex == imagePlaneCount)
                curPlaneIndex = m_buffer[bufIndex].getMetaPlaneIndex();
             //Convert to debugInfoPlaneIndex.
             if (m_hasDebugInfoPlane == true) {
                if (m_hasMetaPlane == true && planeIndex == (imagePlaneCount+1))
                    curPlaneIndex = m_buffer[bufIndex].getDebugInfoPlaneIndex();
                else if (m_hasMetaPlane == false && planeIndex == imagePlaneCount )
                    curPlaneIndex = m_buffer[bufIndex].getDebugInfoPlaneIndex();
             }
             if (m_buffer[bufIndex].size[curPlaneIndex] == 0) {
                CLOGE("size is empty [m_buffer[%d].size[%d]=%d]",
                    bufIndex, curPlaneIndex, m_buffer[bufIndex].size[curPlaneIndex]);
                ret = false;
                goto func_exit;
             }
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

int ExynosCameraBufferManager::m_getTotalPlaneCount(int planeCount, int batchSize, bool hasMetaPlane, bool hasDebugInfoPlane)
{
    if (planeCount < 1 || batchSize < 1) {
        CLOGE("Invalid values. planeCount %d batchSize %d", planeCount, batchSize);
        return 0;
    }

    int totalPlaneCount = 0;
    int extraPlane = 0;
    int imagePlaneCount = planeCount;

    if (hasDebugInfoPlane == true)
        imagePlaneCount--;

    if (hasMetaPlane == true)
        imagePlaneCount--;

    extraPlane = planeCount - imagePlaneCount;
    totalPlaneCount = imagePlaneCount * batchSize + extraPlane;

    return totalPlaneCount;
}

int ExynosCameraBufferManager::m_getImagePlaneCount(int planeCount)
{
    int imagePlaneCount = planeCount;
    if(planeCount < 1 ){
       CLOGE("Invalid values. planeCount (%d)", planeCount);
       return 0;
    }

    imagePlaneCount -= (m_hasDebugInfoPlane == true) ? 1 : 0;
    imagePlaneCount -= (m_hasMetaPlane == true) ? 1 : 0;

    return imagePlaneCount;
}

int ExynosCameraBufferManager::getAvailableIncreaseBufferCount(void)
{
    CLOGI("this function only applied to ONDEMAND mode (%d)", m_allocMode);
    int numAvailable = 0;

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND || m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE)
        numAvailable += (m_allowedMaxBufCount - m_allocatedBufCount);

    CLOGI("m_allowedMaxBufCount(%d), m_allocatedBufCount(%d), ret(%d)",
         m_allowedMaxBufCount, m_allocatedBufCount, numAvailable);
    return numAvailable;
}

int ExynosCameraBufferManager::getNumOfAvailableBuffer(void)
{
    int numAvailable = 0;

    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        if (m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE)
            numAvailable++;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND || m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE)
        numAvailable += (m_allowedMaxBufCount - m_allocatedBufCount);

    return numAvailable;
}

void ExynosCameraBufferManager::printBufferState(void)
{
    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        CLOGV("m_buffer[%d].fd[0]=%d, position=%d, permission=%d, refCount=%d]",
            i, m_buffer[i].fd[0],
            m_buffer[i].status.position, m_buffer[i].status.permission, m_buffer[i].refCount);
    }

    return;
}

void ExynosCameraBufferManager::printBufferState(int bufIndex, int planeIndex)
{
    CLOGI("m_buffer[%d].fd[%d]=%d, .status.permission=%d, .refCount=%d]",
        bufIndex, planeIndex, m_buffer[bufIndex].fd[planeIndex],
        m_buffer[bufIndex].status.permission,
        m_buffer[bufIndex].refCount);

    return;
}

void ExynosCameraBufferManager::printBufferQState()
{
    std::list<int>::iterator r;
    int  bufferIndex;

    Mutex::Autolock lock(m_availableBufferIndexQLock);

    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        bufferIndex = *r;
        CLOGV("bufferIndex=%d", bufferIndex);
    }

    return;
}

void ExynosCameraBufferManager::dumpBufferInfo(int fd)
{
    CLOGW("m_allocatedBufCount(%d), m_reservedMemoryCount(%d), m_reqBufCount(%d), " \
            "m_flagNeedMmap(%d), m_hasMetaPlane(%d), m_hasDebugInfoPlane(%d),m_indexOffset(%d)",
            m_allocatedBufCount, m_reservedMemoryCount, m_reqBufCount, m_flagNeedMmap, m_hasMetaPlane,m_hasDebugInfoPlane,m_indexOffset);

    if (fd > 0) {
        dprintf(fd, "[%s]\n", m_name);
        dprintf(fd, "\tDims: %d x %d, format 0x%x, type 0x%x\n", m_config.debugInfo.width, m_config.debugInfo.height, m_config.debugInfo.format, m_config.type);
        dprintf(fd, "\tm_allocatedBufCount(%d), m_reservedMemoryCount(%d), m_reqBufCount(%d), " \
            "m_flagNeedMmap(%d), m_hasMetaPlane(%d), m_hasDebugInfoPlane(%d), m_indexOffset(%d)\n",
            m_allocatedBufCount, m_reservedMemoryCount, m_reqBufCount, m_flagNeedMmap, m_hasMetaPlane,m_hasDebugInfoPlane, m_indexOffset);
        dprintf(fd, "\tBuffer size(%d) ([0]: %d, [1]: %d, [2]: %d)\n",
                m_config.size[0] + m_config.size[1] + m_config.size[2],
                m_config.size[0], m_config.size[1], m_config.size[2]);
    }

    for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++) {
        int totalPlaneCount = m_getTotalPlaneCount(m_buffer[bufIndex].planeCount, m_buffer[bufIndex].batchSize, m_hasMetaPlane,m_hasDebugInfoPlane);
        for (int planeIndex = 0; planeIndex < totalPlaneCount; planeIndex++) {
            CLOGE("[m_buffer[%d].fd[%d]=%d] .addr=%p .size=%d .position=%d .permission=%d]",
                    m_buffer[bufIndex].index, planeIndex,
                    m_buffer[bufIndex].fd[planeIndex],
                    m_buffer[bufIndex].addr[planeIndex],
                    m_buffer[bufIndex].size[planeIndex],
                    m_buffer[bufIndex].status.position,
                    m_buffer[bufIndex].status.permission);
        }
    }
    printBufferQState();

    if (fd > 0) dprintf(fd, "\n");

    return;
}

} // namespace android
