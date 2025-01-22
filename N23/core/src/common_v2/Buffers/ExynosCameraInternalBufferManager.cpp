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

#define LOG_TAG "ExynosCameraInternalBufferManager"
#include "ExynosCameraInternalBufferManager.h"

InternalExynosCameraBufferManager::InternalExynosCameraBufferManager(uint32_t maxBufferCount):ExynosCameraBufferManager(maxBufferCount)
{
    m_allocationThread = new allocThread(this, &InternalExynosCameraBufferManager::m_allocationThreadFunc, "allocationThreadFunc");
}

InternalExynosCameraBufferManager::~InternalExynosCameraBufferManager()
{
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        m_allocationThread->join();
        CLOGI("allocationThread is finished");
    }

    ExynosCameraBufferManager::deinit();
}

status_t InternalExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    return m_setDefaultAllocator(allocator);
}

bool InternalExynosCameraBufferManager::m_allocationThreadFunc(void)
{
    status_t ret = NO_ERROR;
    int increaseCount = 1;

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("increase buffer silently - start - "
          "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d, m_reqBufCount=%d)",
        m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);

    increaseCount = m_allowedMaxBufCount - m_reqBufCount;

    /* increase buffer*/
    for (int count = 0; count < increaseCount; count++) {
        ret = m_increase(1);
        if (ret < 0) {
            CLOGE("increase the buffer failed");
        } else {
            Mutex::Autolock lock(m_lock);
            m_allocatedBufCount++;
#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
            int bufferIndex;
            int bufCount = 0;
            int increaseCount = 1;
            std::vector<int>::reverse_iterator r;
            if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
                m_availableBufferIndexQLock.lock();
                for (r = m_allocatedBufferIndexQ.rbegin(); r != m_allocatedBufferIndexQ.rend() && bufCount < increaseCount; ++r) {
                    bufferIndex = *r;
                    m_availableBufferIndexQ.push_back(bufferIndex);
                    m_allocatedBufCount++;
                    bufCount++;
                }
                m_availableBufferIndexQLock.unlock();
            } else
#endif
            {
                m_availableBufferIndexQLock.lock();
                m_availableBufferIndexQ.push_back(m_buffer[m_allocatedBufCount + m_indexOffset].index);
                m_availableBufferIndexQLock.unlock();
            }
        }

    }
#ifdef EXYNOS_CAMERA_DUMP_BUFFER_INFO
    dumpBufferInfo();
#endif
    CLOGI("increase buffer silently - end - (increaseCount=%d)"
          "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d, m_reqBufCount=%d)",
         increaseCount, m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);

    /* false : Thread run once */
    return false;
}

status_t InternalExynosCameraBufferManager::resetBuffers(void)
{
    /* same as deinit */
    /* clear buffers except releasing the memory */
    status_t ret = NO_ERROR;

    if (m_flagAllocated == false) {
        CLOGI("OUT.. Buffer is not allocated");
        return ret;
    }

    CLOGD("IN..");

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        m_allocationThread->join();
        CLOGI("allocationThread is finished");
    }

#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        int bufIndex;
        std::vector<int> allocatedBufferIndexQ_backup;
        std::vector<int>::iterator r;
        allocatedBufferIndexQ_backup.assign(m_allocatedBufferIndexQ.begin(),m_allocatedBufferIndexQ.end());

        for (r = allocatedBufferIndexQ_backup.begin(); r != allocatedBufferIndexQ_backup.end(); ++r) {
            bufIndex = *r;
            int32_t refCount = m_buffer[bufIndex].refCount;
            while (refCount > 0) {
                cancelBuffer(bufIndex);
                if (refCount == m_buffer[bufIndex].refCount) {
                    CLOGE("Can't decrease refCount");
                    break;
                }
                refCount = m_buffer[bufIndex].refCount;
            }
        }
    } else
#endif
    {
        for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++) {
            int32_t refCount = m_buffer[bufIndex].refCount;
            while (refCount > 0) {
                cancelBuffer(bufIndex);
                if (refCount == m_buffer[bufIndex].refCount) {
                    CLOGE("Can't decrease refCount");
                    break;
                }
                refCount = m_buffer[bufIndex].refCount;
            }
        }
    }

    m_resetSequenceQ();
    m_flagSkipAllocation = true;

    return ret;
}

status_t InternalExynosCameraBufferManager::alloc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    if (m_flagSkipAllocation == true) {
        CLOGI("skip to allocate memory (m_flagSkipAllocation=%d)", (int)m_flagSkipAllocation);
        m_flagAllocated = true;
        goto func_exit;
    }

    if (m_checkInfoForAlloc() == false) {
        CLOGE("m_checkInfoForAlloc failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocatedBufCount != 0) {
        CLOGD("m_allocatedBufCount(%d), skip alloc", m_allocatedBufCount);
        m_flagAllocated = true;
        goto func_exit;
    }

#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        int bufIndex;
        int bufCount = 0;
        std::vector<int>::iterator r;

        m_availableBufferIndexQLock.lock();
        for (r = m_unAllocatedBufferIndexQ.begin(); r != m_unAllocatedBufferIndexQ.end() && bufCount < m_reqBufCount; ) {
            bufIndex = *r;
            if (m_hasDebugInfoPlane == true) {
                if (m_defaultAlloc(bufIndex, bufIndex+1, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultAlloc failed");
                    ret = INVALID_OPERATION;
                    m_availableBufferIndexQLock.unlock();
                    goto func_exit;
                }
            }

            if (m_hasMetaPlane == true) {
                if (m_defaultAlloc(bufIndex, bufIndex+1, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultAlloc failed");
                    ret = INVALID_OPERATION;
                    m_availableBufferIndexQLock.unlock();
                    goto func_exit;
                }
            }

            /* allocate image buffer */
            if (m_alloc(bufIndex, bufIndex+1) != NO_ERROR) {
                CLOGE("m_alloc failed");

                if (m_hasMetaPlane == true) {
                    CLOGD("Free metadata plane. bufferCount %d", m_reqBufCount);
                    if (m_defaultFree(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                        CLOGE("m_defaultFree failed");
                    }
                }

                if (m_hasDebugInfoPlane == true) {
                    CLOGD("Free metadata plane. bufferCount %d", m_reqBufCount);
                    if (m_defaultFree(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                        CLOGE("m_defaultFree failed");
                    }
                }
                ret = INVALID_OPERATION;
                m_availableBufferIndexQLock.unlock();
                goto func_exit;
            }

            r = m_unAllocatedBufferIndexQ.erase(r);
            m_allocatedBufferIndexQ.push_back(bufIndex);
            bufCount += 1;
        }

        m_availableBufferIndexQLock.unlock();
    } else
#endif
    {
        if (m_hasDebugInfoPlane == true) {
            if (m_defaultAlloc(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultAlloc failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        if (m_hasMetaPlane == true) {
            if (m_defaultAlloc(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultAlloc failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        /* allocate image buffer */
        if (m_alloc(m_indexOffset, m_reqBufCount + m_indexOffset) != NO_ERROR) {
            CLOGE("m_alloc failed");

            if (m_hasMetaPlane == true) {
                CLOGD("Free metadata plane. bufferCount %d", m_reqBufCount);
                if (m_defaultFree(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultFree failed");
                }
            }

            if (m_hasDebugInfoPlane == true) {
                CLOGD("Free metadata plane. bufferCount %d", m_reqBufCount);
                if (m_defaultFree(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultFree failed");
                }
            }

            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    m_allocatedBufCount = m_reqBufCount;
    m_resetSequenceQ();
    m_flagAllocated = true;

    CLOGD("Allocate the buffer succeeded "
          "(m_allocatedBufCount=%d, m_reqBufCount=%d, m_allowedMaxBufCount=%d) --- dumpBufferInfo ---",
        m_allocatedBufCount, m_reqBufCount, m_allowedMaxBufCount);
#ifdef EXYNOS_CAMERA_DUMP_BUFFER_INFO
    dumpBufferInfo();
    CLOGD("------------------------------------------------------------------");
#endif

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        /* run the allocationThread */
        m_allocationThread->run(PRIORITY_DEFAULT);
        CLOGI("allocationThread is started");
    }

func_exit:

    m_flagSkipAllocation = false;
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t InternalExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    status_t ret = NO_ERROR;

    ret = m_defaultAlloc(bIndex, eIndex, 0);
    if (ret != NO_ERROR) {
        CLOGE("Failed to alloc. bIndex %d eIndex %d ret %d",
                bIndex, eIndex, ret);
        return ret;
    }

    if (m_buffer[0].batchSize > 1) {
        ret = m_constructBufferContainer(bIndex, eIndex);
        if (ret != NO_ERROR) {
            CLOGE("Failed to constructBufferContainer. bIndex %d eIndex %d ret %d",
                    bIndex, eIndex, ret);
            return ret;
        }
    }

    return NO_ERROR;
}

status_t InternalExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    status_t ret = NO_ERROR;

    if (m_buffer[0].batchSize > 1) {
        ret = m_destructBufferContainer(bIndex, eIndex);
        if (ret != NO_ERROR) {
            CLOGE("Failed to destructBufferContainer. bIndex %d eIndex %d ret %d",
                    bIndex, eIndex, ret);
            return ret;
        }
    } else {
        ret = m_defaultFree(bIndex, eIndex, 0);
        if (ret != NO_ERROR) {
            CLOGE("Failed to free. bIndex %d eIndex %d ret %d",
                    bIndex, eIndex, ret);
            return ret;
        }
    }

    return NO_ERROR;
}

status_t InternalExynosCameraBufferManager::m_increase(int increaseCount)
{
    CLOGD("IN..");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;

    if (m_allowedMaxBufCount <= m_allocatedBufCount) {
        CLOGD("BufferManager can't increase the buffer "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d)",
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allowedMaxBufCount < m_allocatedBufCount + increaseCount) {
        CLOGI("change the increaseCount (%d->%d) --- "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d + increaseCount=%d)",
             increaseCount, m_allowedMaxBufCount - m_allocatedBufCount,
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount, increaseCount);
        increaseCount = m_allowedMaxBufCount - m_allocatedBufCount;
    }

#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
        int bufIndex;
        int headIndex = 0;
        int bufCount = 0;
        std::vector<int>::iterator r;
        m_availableBufferIndexQLock.lock();

        if (m_allocatedBufferIndexQ.size() > 0) {
            headIndex = m_allocatedBufferIndexQ.front();
        }

        for (r = m_unAllocatedBufferIndexQ.begin(); r != m_unAllocatedBufferIndexQ.end() && bufCount < increaseCount; ) {
            bufIndex = *r;
            for (int planeIndex = 0; planeIndex < m_buffer[0].planeCount; planeIndex++) {
                if (m_buffer[headIndex].size[planeIndex] == 0) {
                    CLOGE("abnormal value [size=%d]",
                        m_buffer[0].size[planeIndex]);
                    ret = BAD_VALUE;
                    m_availableBufferIndexQLock.unlock();
                    goto func_exit;
                }
                m_buffer[bufIndex].size[planeIndex]         = m_buffer[headIndex].size[planeIndex];
                m_buffer[bufIndex].bytesPerLine[planeIndex] = m_buffer[headIndex].bytesPerLine[planeIndex];
            }
            m_buffer[bufIndex].planeCount = m_buffer[headIndex].planeCount;
            m_buffer[bufIndex].type       = m_buffer[headIndex].type;

            if (m_alloc(bufIndex,bufIndex+1) != NO_ERROR) {
                CLOGE("m_alloc failed");
                ret = INVALID_OPERATION;
                m_availableBufferIndexQLock.unlock();
                goto func_exit;
            }

            if (m_hasMetaPlane == true) {
                if (m_defaultAlloc(bufIndex,bufIndex+1, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultAlloc failed");
                    ret = INVALID_OPERATION;
                    m_availableBufferIndexQLock.unlock();
                    goto func_exit;
                }
            }

            if (m_hasDebugInfoPlane == true) {
                if (m_defaultAlloc(bufIndex,bufIndex+1, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                    CLOGE("m_defaultAlloc failed");
                    ret = INVALID_OPERATION;
                    m_availableBufferIndexQLock.unlock();
                    goto func_exit;
                }
            }
            r = m_unAllocatedBufferIndexQ.erase(r);
            m_allocatedBufferIndexQ.push_back(bufIndex);
            bufCount +=1 ;
            CLOGD("Increase the buffer succeeded (m_allocatedBufCount=%d, increaseCount=%d)",
                m_allocatedBufCount + m_indexOffset, increaseCount);
        }
        m_availableBufferIndexQLock.unlock();
    } else
#endif
    {
        /* set the buffer information */
        for (int bufIndex = m_allocatedBufCount + m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset + increaseCount; bufIndex++) {
            for (int planeIndex = 0; planeIndex < m_buffer[0].planeCount; planeIndex++) {
                if (m_buffer[0].size[planeIndex] == 0) {
                    CLOGE("abnormal value [size=%d]",
                        m_buffer[0].size[planeIndex]);
                    ret = BAD_VALUE;
                    goto func_exit;
                }
                m_buffer[bufIndex].size[planeIndex]         = m_buffer[0].size[planeIndex];
                m_buffer[bufIndex].bytesPerLine[planeIndex] = m_buffer[0].bytesPerLine[planeIndex];
            }
            m_buffer[bufIndex].planeCount = m_buffer[0].planeCount;
            m_buffer[bufIndex].type       = m_buffer[0].type;
            m_buffer[bufIndex].refCount   = 0;
        }

        if (m_alloc(m_allocatedBufCount + m_indexOffset, m_allocatedBufCount + m_indexOffset + increaseCount) != NO_ERROR) {
            CLOGE("m_alloc failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_hasMetaPlane == true) {
            if (m_defaultAlloc(m_allocatedBufCount + m_indexOffset, m_allocatedBufCount + m_indexOffset + increaseCount, 1 << META_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultAlloc failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        if (m_hasDebugInfoPlane == true) {
            if (m_defaultAlloc(m_allocatedBufCount + m_indexOffset, m_allocatedBufCount + m_indexOffset + increaseCount, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
                CLOGE("m_defaultAlloc failed");
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        CLOGD("Increase the buffer succeeded (m_allocatedBufCount=%d, increaseCount=%d)",
            m_allocatedBufCount + m_indexOffset, increaseCount);
    }

func_exit:

    CLOGD("OUT..");

    return ret;
}

status_t InternalExynosCameraBufferManager::m_decrease(void)
{
    CLOGD("IN..");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = true;
    std::list<int>::iterator r;

    int  bufferIndex = -1;

    if (m_allocatedBufCount <= m_reqBufCount) {
        CLOGD("BufferManager can't decrease the buffer "
              "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d <= m_reqBufCount=%d)",
            m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }
    bufferIndex = m_allocatedBufCount;

    if (m_free(bufferIndex-1 + m_indexOffset, bufferIndex + m_indexOffset) != NO_ERROR) {
        CLOGE("m_free failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultFree(bufferIndex-1 + m_indexOffset, bufferIndex + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
            CLOGE("m_defaultFree failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    if (m_hasDebugInfoPlane == true) {
        if (m_defaultFree(bufferIndex-1 + m_indexOffset, bufferIndex + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
            CLOGE("m_defaultFree failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        if ((bufferIndex + m_indexOffset) == *r) {
            m_availableBufferIndexQ.erase(r);
            break;
        }
    }
    m_availableBufferIndexQLock.unlock();
    m_allocatedBufCount--;

    CLOGD("Decrease the buffer succeeded (m_allocatedBufCount=%d)" ,
         m_allocatedBufCount);

func_exit:

    CLOGD("OUT..");

    return ret;
}


status_t InternalExynosCameraBufferManager::m_constructBufferContainer(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer timer;

    if (m_defaultAllocator == NULL) {
        CLOGE("defaultAllocator is NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bIndex < 0 || eIndex < 0) {
        CLOGE("Invalid parameters. bIndex %d eIndex %d",
                bIndex, eIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        /* Get buffer container FD for same size planes */
        int imagePlaneCount = m_getImagePlaneCount(m_buffer[bufIndex].planeCount);

        timer.start();
        for (int planeIndex = 0; planeIndex < imagePlaneCount; planeIndex++) {
            /* Gather single FDs */
            int fds[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
            for (int batchIndex = 0; batchIndex < m_buffer[bufIndex].batchSize; batchIndex++) {
                fds[batchIndex] = m_buffer[bufIndex].fd[(batchIndex * imagePlaneCount) + planeIndex];
            }

            ret = m_defaultAllocator->createBufferContainer(fds,
                                                            m_buffer[bufIndex].batchSize,
                                                            &m_buffer[bufIndex].containerFd[planeIndex]);
            if (ret != NO_ERROR) {
                CLOGE("[B%d P%d]Failed to createBufferContainer. batchSize %d ret %d",
                        bufIndex, planeIndex, m_buffer[bufIndex].batchSize, ret);
                ret = INVALID_OPERATION;
                goto func_exit;
            }

            /* Release single buffer FD.
             * Buffer container will have reference for each single buffer.
             */
            for (int batchIndex = 0; batchIndex < m_buffer[bufIndex].batchSize; batchIndex++) {
                int curPlaneIndex = (batchIndex * imagePlaneCount) + planeIndex;
                ret = m_defaultAllocator->free(m_buffer[bufIndex].size[curPlaneIndex],
                                            &(m_buffer[bufIndex].fd[curPlaneIndex]),
                                            &(m_buffer[bufIndex].addr[curPlaneIndex]),
                                            m_flagNeedMmap);
                if (ret != NO_ERROR) {
                    CLOGE("[B%d P%d FD%d]Failed to free. ret %d",
                            bufIndex, planeIndex, m_buffer[bufIndex].fd[curPlaneIndex], ret);
                    /* continue */
                }
            }
        }

        timer.stop();
        CLOGD("duration time(%5d msec):(bufIndex=%d, batchSize=%d, fd=%d/%d/%d)",
                (int)timer.durationMsecs(), bufIndex, m_buffer[bufIndex].batchSize,
                m_buffer[bufIndex].containerFd[0], m_buffer[bufIndex].containerFd[1], m_buffer[bufIndex].containerFd[2]);
    }

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t InternalExynosCameraBufferManager::m_destructBufferContainer(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    char *dummyAddr = NULL;

    if (m_defaultAllocator == NULL) {
        CLOGE("defaultAllocator is NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bIndex < 0 || eIndex < 0) {
        CLOGE("Invalid parameters. bIndex %d eIndex %d",
                bIndex, eIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        int imagePlaneCount = m_getImagePlaneCount(m_buffer[bufIndex].planeCount);

        for (int planeIndex = 0; planeIndex < imagePlaneCount; planeIndex++) {
            ret = m_defaultAllocator->free(m_buffer[bufIndex].size[planeIndex],
                                           &(m_buffer[bufIndex].containerFd[planeIndex]),
                                           &dummyAddr, /*mapNeeded */false);
            if (ret != NO_ERROR) {
                CLOGE("[B%d P%d FD%d]Failed to free containerFd. ret %d",
                        bufIndex, planeIndex, m_buffer[bufIndex].containerFd[planeIndex], ret);
                /* continue */
            }
        }
    }

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t InternalExynosCameraBufferManager::increase(int increaseCount)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    Mutex::Autolock lock(m_lock);
    status_t ret = NO_ERROR;

    CLOGI("m_allocatedBufCount(%d), m_allowedMaxBufCount(%d), increaseCount(%d)",
         m_allocatedBufCount, m_allowedMaxBufCount, increaseCount);

    /* increase buffer*/
    ret = m_increase(increaseCount);
    if (ret < 0) {
        CLOGE("increase the buffer failed, m_allocatedBufCount(%d), m_allowedMaxBufCount(%d), increaseCount(%d)",
              m_allocatedBufCount, m_allowedMaxBufCount, increaseCount);
    } else {
#ifdef USE_BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE_NEW
        int bufferIndex;
        int bufCount = 0;
        std::vector<int>::reverse_iterator r;

        if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE) {
            m_availableBufferIndexQLock.lock();
            for (r = m_allocatedBufferIndexQ.rbegin(); r != m_allocatedBufferIndexQ.rend() && bufCount < increaseCount; ++r) {
                bufferIndex = *r;
                m_availableBufferIndexQ.push_back(bufferIndex);
                bufCount += 1;
            }
            m_availableBufferIndexQLock.unlock();
        } else
#endif
        {
            for (int bufferIndex = m_allocatedBufCount + m_indexOffset; bufferIndex < m_allocatedBufCount + m_indexOffset + increaseCount; bufferIndex++) {
                m_availableBufferIndexQLock.lock();
                m_availableBufferIndexQ.push_back(bufferIndex);
                m_availableBufferIndexQLock.unlock();
            }
        }
        m_allocatedBufCount += increaseCount;

#ifdef EXYNOS_CAMERA_DUMP_BUFFER_INFO
        dumpBufferInfo();
#endif
        CLOGI("increase the buffer succeeded (increaseCount(%d))",increaseCount);
    }

    return ret;
}
