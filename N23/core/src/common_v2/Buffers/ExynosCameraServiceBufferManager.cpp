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

#define LOG_TAG "ExynosCameraServiceBufferManager"
#include "ExynosCameraServiceBufferManager.h"
#include "ExynosGraphicBuffer.h"

using vendor::graphics::ExynosGraphicBufferMeta;

ServiceExynosCameraBufferManager::ServiceExynosCameraBufferManager(int actualFormat):ExynosCameraBufferManager(VIDEO_MAX_FRAME)
{
    CLOGD("");

    m_allocationThread = new allocThread(this, &ServiceExynosCameraBufferManager::m_allocationThreadFunc, "allocationThreadFunc");
    m_allocator = new ExynosCameraStreamAllocator(actualFormat);

    for (int bufIndex = 0; bufIndex < VIDEO_MAX_FRAME; bufIndex++) {
        m_handleIsLocked[bufIndex] = false;
    }
}

ServiceExynosCameraBufferManager::~ServiceExynosCameraBufferManager()
{
    if (m_allocator != NULL) {
        delete m_allocator;
        m_allocator = NULL;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        m_allocationThread->join();
        CLOGI("allocationThread is finished");
    }

    ExynosCameraBufferManager::deinit();
}

status_t ServiceExynosCameraBufferManager::setInfo(buffer_manager_configuration_t info)
{
    return m_setInfo(info, true);
}

bool ServiceExynosCameraBufferManager::m_allocationThreadFunc(void)
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

            m_availableBufferIndexQLock.lock();
            m_availableBufferIndexQ.push_back(m_buffer[m_allocatedBufCount + m_indexOffset].index);
            m_availableBufferIndexQLock.unlock();
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

status_t ServiceExynosCameraBufferManager::resetBuffers(void)
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

    for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++)
        cancelBuffer(bufIndex);

    m_resetSequenceQ();
    m_flagSkipAllocation = true;

    return ret;
}

status_t ServiceExynosCameraBufferManager::alloc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    if (m_flagSkipAllocation == true) {
        CLOGI("skip to allocate memory (m_flagSkipAllocation=%d)", (int)m_flagSkipAllocation);
        goto func_exit;
    }

    if (m_checkInfoForAlloc() == false) {
        CLOGE("m_checkInfoForAlloc failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultAlloc(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << META_PLANE_SHIFT) != NO_ERROR) {
            CLOGE("m_defaultAlloc failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }
    if (m_hasDebugInfoPlane == true) {
        if (m_defaultAlloc(m_indexOffset, m_reqBufCount + m_indexOffset, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
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

status_t ServiceExynosCameraBufferManager::putBuffer(
        int bufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    std::list<int>::iterator r;
    bool found = false;
    int totalPlaneCount = 0;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;

    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;

    if (bufIndex < 0 || m_allocatedBufCount + m_indexOffset <= bufIndex) {
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
        CLOGV("bufIndex=%d is already in (available state)", bufIndex);
        goto func_exit;
    }

    if (--m_buffer[bufIndex].refCount > 0) {
        CLOGD("skip put buffer. bufRefCount %d, IDX:%d", m_buffer[bufIndex].refCount, bufIndex);
        goto func_exit;
    }

    /* Clear Image Plane Information */
    totalPlaneCount = m_getTotalPlaneCount(m_buffer[bufIndex].planeCount,
                                           m_buffer[bufIndex].batchSize,
                                           m_hasMetaPlane, m_hasDebugInfoPlane);

    totalPlaneCount -= (m_hasDebugInfoPlane == true) ? 1 : 0;
    totalPlaneCount -= (m_hasMetaPlane == true) ? 1 : 0;

    for (int i = 0; i < totalPlaneCount; i++) {
        m_buffer[bufIndex].fd[i] = -1;
        m_buffer[bufIndex].addr[i] = NULL;
        m_buffer[bufIndex].handle[i] = NULL;
        m_buffer[bufIndex].acquireFence[i] = -1;
        m_buffer[bufIndex].releaseFence[i] = -1;
    }

    if (m_buffer[bufIndex].batchSize > 1) {
        ret = m_destructBufferContainer(bufIndex);
        if (ret != NO_ERROR) {
            CLOGE("[B%d]Failed to destructBufferContainer. ret %d", bufIndex, ret);
            /* continue */
        }
    }

    if (updateStatus(bufIndex, 0, position, permission) != NO_ERROR) {
        CLOGE("setStatus failed [bufIndex=%d, position=%d, permission=%d]",
             bufIndex, (int)position, (int)permission);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
    m_availableBufferIndexQLock.unlock();

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::getBuffer(
        int  *reqBufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position,
        struct ExynosCameraBuffer *buffer)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    std::list<int>::iterator r;

    int  bufferIndex;
    int planeCount;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;
    ExynosCameraFence* fence = NULL;

    bufferIndex = *reqBufIndex;
    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_NONE;

    if (m_allocatedBufCount == 0) {
        CLOGE("m_allocatedBufCount equals zero");
        ret = INVALID_OPERATION;
        goto func_exit;
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
        if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND) {
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

    for (int batchIndex = 0; batchIndex < m_buffer[bufferIndex].batchSize; batchIndex++) {
        if (buffer->handle[batchIndex] == NULL) {
            CLOGW("[B%d-%d]Handle is NULL. Skip it", bufferIndex, batchIndex);
            continue;
        }

        fence = new ExynosCameraFence(
                ExynosCameraFence::EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE,
                buffer->acquireFence[batchIndex],
                buffer->releaseFence[batchIndex]);

        /* Wait fence */
#ifdef USE_CAMERA2_USE_FENCE
        if (fence != NULL) {
#ifdef USE_SW_FENCE
            /* wait before give buffer to hardware */
            if (fence->isValid() == true) {
                ret = m_waitFence(fence);
                if (ret != NO_ERROR) {
                    CLOGE("[B%d-%d]m_waitFence() fail", bufferIndex, batchIndex);
                    goto loop_continue;
                }
            } else {
                CLOGV("[B%d-%d]Fence is invalid", bufferIndex, batchIndex);
            }
            /* Initialize fence */
            buffer->acquireFence[batchIndex] = -1;
            buffer->releaseFence[batchIndex] = -1;
#else
            /* give fence to H/W */
            buffer->acquireFence[batchIndex] = ptrFence->getAcquireFence();
            buffer->releaseFence[batchIndex] = ptrFence->getReleaseFence();
#endif
        }
#endif

        planeCount = m_getImagePlaneCount(m_buffer[bufferIndex].planeCount);

        if (m_bufferManagerType == BUFFER_MANAGER_SERVICE_GRALLOC_TYPE) {
            ret = m_checkBufferInfo(*(buffer->handle[batchIndex]), &m_buffer[bufferIndex]);
            if (ret != NO_ERROR) {
                CLOGE("[B%d]Invalid buffer info.", bufferIndex);
                goto func_exit;
            }
        }

        if (m_flagNeedMmap == true) {
            /* Get FD/VA from handle */
            ret = m_allocator->lock(
                    &(buffer->handle[batchIndex]),
                    &(m_buffer[bufferIndex].fd[batchIndex * planeCount]),
                    &(m_buffer[bufferIndex].addr[batchIndex * planeCount]),
                    &m_handleIsLocked[bufferIndex],
                    planeCount);
            if (ret != NO_ERROR) {
                CLOGE("[B%d]Failed to grallocHal->lock buffer_handle", bufferIndex);
                goto loop_continue;
            }

            /* Sync cache operation */
            ret = m_allocator->unlock(buffer->handle[batchIndex]);
            if (ret != NO_ERROR) {
                CLOGE("[B%d]Failed to grallocHal->unlock buffer_handle", bufferIndex);
                goto loop_continue;
            }
        } else {
            /* Get FD from handle */
            ret = m_getBufferInfoFromHandle(*(buffer->handle[batchIndex]),
                                            planeCount,
                                            &(m_buffer[bufferIndex].fd[batchIndex * planeCount]));
            if (ret != NO_ERROR) {
                CLOGE("[B%d]Failed to getBufferInfoFromHandle. ret %d", bufferIndex, ret);
                goto loop_continue;
            }
        }

        m_buffer[bufferIndex].handle[batchIndex] = buffer->handle[batchIndex];
        m_handleIsLocked[bufferIndex] = false;

loop_continue:
        if (fence != NULL) {
            delete fence;
            fence = NULL;
        }
    }

    if (m_buffer[bufferIndex].batchSize > 1) {
        ret = m_constructBufferContainer(bufferIndex);
        if (ret != NO_ERROR) {
            CLOGE("[B%d]Failed to constructBufferContainer. ret %d", bufferIndex, ret);
            /* continue */
        }
    }

    m_buffer[bufferIndex].refCount++;
    *reqBufIndex = bufferIndex;
    *buffer      = m_buffer[bufferIndex];

func_exit:

    if (fence != NULL) {
        delete fence;
        fence = NULL;
    }
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    if (m_allocator == NULL) {
        CLOGE("m_allocator equals NULL");
        goto func_exit;
    }

    m_allocator->init((camera3_stream_t *)allocator);

func_exit:
    return NO_ERROR;
}

status_t ServiceExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = OK;
    CLOGD("");

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("setStatus failed [bufIndex=%d, position=SERVICE, permission=NONE]", bufIndex);
            ret = INVALID_OPERATION;
            break;
        }
    }

    /*
     * service buffer is given by service.
     * so, initial state is all un-available.
     */
    m_availableBufferIndexQLock.lock();
    m_availableBufferIndexQ.clear();
    m_availableBufferIndexQLock.unlock();

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    CLOGD("IN");
    dump();

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (m_handleIsLocked[bufIndex] == false) {
            CLOGV("buffer [bufIndex=%d] already free", bufIndex);
            continue;
        }

        m_handleIsLocked[bufIndex] = false;

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

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_increase(__unused int increaseCount)
{
    CLOGD("IN..");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    int refIndex, sIndex, eIndex;

    refIndex = m_indexOffset;
    sIndex = m_allocatedBufCount + m_indexOffset;
    eIndex = sIndex + increaseCount;

    if (m_allocatedBufCount == 0) {
        CLOGE("Can't support reqBufCount = 0 case");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allowedMaxBufCount <= m_allocatedBufCount) {
        CLOGE("BufferManager can't increase the buffer "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d)",
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allowedMaxBufCount < eIndex) {
        CLOGW("change the increaseCount (%d->%d) --- "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d + increaseCount=%d)",
             increaseCount, m_allowedMaxBufCount - m_allocatedBufCount,
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount, increaseCount);
        increaseCount = m_allowedMaxBufCount - m_allocatedBufCount;
    }

    // TODO: Can't support batch buffer until now
    if (m_buffer[refIndex].batchSize > 1) {
        CLOGE("Can't support batch buffer");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    /* set the buffer information */
    for (int bufIndex = sIndex; bufIndex < eIndex; bufIndex++) {
        // copy first buffer to current
        m_buffer[bufIndex] = m_buffer[refIndex];

        // init buffer
        m_buffer[bufIndex].refCount = 0;
        for (int i = 0; i < m_buffer[bufIndex].planeCount; i++) {
            m_buffer[bufIndex].fd[i] = -1;
            m_buffer[bufIndex].addr[i] = NULL;
            m_buffer[bufIndex].handle[i] = NULL;
            m_buffer[bufIndex].acquireFence[i] = -1;
            m_buffer[bufIndex].releaseFence[i] = -1;
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("setStatus failed [bufIndex=%d, position=SERVICE, permission=NONE]", bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultAlloc(sIndex, eIndex, 1 << META_PLANE_SHIFT) != NO_ERROR) {
            CLOGE("m_defaultAlloc failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    if (m_hasDebugInfoPlane == true) {
        if (m_defaultAlloc(sIndex, eIndex, 1 << DEBUG_INFO_PLANE_SHIFT) != NO_ERROR) {
            CLOGE("m_defaultAlloc failed");
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    CLOGD("Increase the buffer succeeded (m_allocatedBufCount=%d, increaseCount=%d, s=%d/e=%d)",
            m_allocatedBufCount + m_indexOffset, increaseCount, sIndex, eIndex);

func_exit:

    CLOGD("OUT..");

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_decrease(void)
{
    return INVALID_OPERATION;
}


/* typedef const native_handle_t* buffer_handle_t; (refer header in libcutils/include/cutils/native_handle.h) */
status_t ServiceExynosCameraBufferManager::m_getBufferInfoFromHandle(buffer_handle_t handle,
                                                                     int planeCount,
                                                                     int fd[])
{
    int curPlaneCount = planeCount;

    if (handle->version != sizeof(native_handle_t)) {
        android_printAssert(NULL, LOG_TAG,
                            "ASSERT(%s):native_handle_t size mismatch. local %zu != framework %d",
                            __FUNCTION__, sizeof(native_handle_t), handle->version);

        return BAD_VALUE;
    }

    if (handle->numFds != planeCount) {
        curPlaneCount = (handle->numFds < planeCount)? handle->numFds : planeCount;
        CLOGW("planeCount mismatch. local %d != handle %d. Use %d.",
                planeCount, handle->numFds, curPlaneCount);
    }

    for (int i = 0; i < curPlaneCount; i++) {
        if (handle->data[i] < 0) {
            CLOGE("Invalid FD %d for plane %d",
                    handle->data[i], i);
            continue;
        }

        fd[i] = handle->data[i];
    }

    return NO_ERROR;
}

status_t ServiceExynosCameraBufferManager::m_checkBufferInfo(const buffer_handle_t handle,
                                                             const ExynosCameraBuffer* buffer)
{
    if (handle == NULL || buffer == NULL) {
        CLOGE("Invalid arguments. handle %p buffer %p", handle, buffer);
        return BAD_VALUE;
    }

    ExynosGraphicBufferMeta gmeta(handle);
    int imagePlaneCount = m_getImagePlaneCount(buffer->planeCount);
    int planeIndex = imagePlaneCount;
    bool isInvalidBuffer = false;

    /* The buffer which has smaller size than the size that V4L2 knows
     * will be failed to do qbuf().
     */
    switch (imagePlaneCount) {
    case 3:
        planeIndex -= 1;

        /* In the preview stream,
           if video meta plane size (gmeta.size2) get 0 from gralloc,
           the below code get size to check buffer size */
        if (!gmeta.size2) {
            if (m_allocator->getActualFormat() == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M
                && ExynosGraphicBufferMeta::get_video_metadata(handle)) {
                gmeta.size2 = ExynosGraphicBufferMeta::get_metadata_size(handle);
            }
        }

        if ((unsigned int)gmeta.size2 < buffer->size[planeIndex]) {
            isInvalidBuffer |= true;
            CLOGE("[B%d P%d]fd %d size %d < %d",
                    buffer->index, planeIndex,
                    gmeta.fd2,
                    gmeta.size2, buffer->size[planeIndex]);
        }
    case 2:
        planeIndex -= 1;
        if ((unsigned int)gmeta.size1 < buffer->size[planeIndex]) {
            isInvalidBuffer |= true;
            CLOGE("[B%d P%d]fd %d size %d < %d",
                    buffer->index, planeIndex,
                    gmeta.fd1,
                    gmeta.size1, buffer->size[planeIndex]);
        }
    case 1:
    default:
        planeIndex -= 1;
        if ((unsigned int)gmeta.size < buffer->size[planeIndex]) {
            isInvalidBuffer |= true;
            CLOGE("[B%d P%d]fd %d size %d < %d",
                    buffer->index, planeIndex,
                    gmeta.fd,
                    gmeta.size, buffer->size[planeIndex]);
        }
        break;
    }

    if (isInvalidBuffer == true) {
        CLOGE("[B%d]format %X imageSize %dx%d usage %ju|%ju ionFlags %X",
                buffer->index,
                gmeta.format,
                gmeta.width, gmeta.height,
                gmeta.producer_usage, gmeta.consumer_usage,
                gmeta.flags);

        android_printAssert(NULL, LOG_TAG,
                            "ASSERT(%s):[B%d]Invalid Service Buffer!",
                            __FUNCTION__, buffer->index);

        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ServiceExynosCameraBufferManager::m_waitFence(ExynosCameraFence *fence)
{
    status_t ret = NO_ERROR;

#if 0
    /* reference code */
    sp<Fence> bufferAcquireFence = new Fence(buffer->acquire_fence);
    ret = bufferAcquireFence->wait(1000); /* 1 sec */
    if (ret == TIMED_OUT) {
        CLOGE("Fence timeout(%d)!!", request->frame_number);
        return INVALID_OPERATION;
    } else if (ret != OK) {
        CLOGE("Waiting on Fence error(%d)!!", request->frame_number);
        return INVALID_OPERATION;
    }
#endif

    if (fence == NULL) {
        CLOGE("fence == NULL. so, fail");
        return INVALID_OPERATION;
    }

    if (fence->isValid() == false) {
        CLOGE("fence->isValid() == false. so, fail");
        return INVALID_OPERATION;
    }

    CLOGV("Valid fence");


    ret = fence->wait();
    if (ret != NO_ERROR) {
        CLOGE("fence->wait() fail");
        return INVALID_OPERATION;
    } else {
        CLOGV("fence->wait() succeed");
    }

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_constructBufferContainer(int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer timer;
    int imagePlaneCount = 0;

    if (m_defaultAllocator == NULL) {
        CLOGE("defaultAllocator is NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bufIndex < 0) {
        CLOGE("Invalid parameters. bufIndex %d", bufIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    /* Get buffer container FD for same size planes */
    imagePlaneCount = m_getImagePlaneCount(m_buffer[bufIndex].planeCount);

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
    }

    timer.stop();
    CLOGV("duration time(%5d msec):(bufIndex=%d, batchSize=%d, fd=%d/%d/%d)",
            (int)timer.durationMsecs(), bufIndex, m_buffer[bufIndex].batchSize,
            m_buffer[bufIndex].containerFd[0], m_buffer[bufIndex].containerFd[1], m_buffer[bufIndex].containerFd[2]);

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_destructBufferContainer(int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int imagePlaneCount = 0;
    char* dummyAddr = NULL;

    if (m_defaultAllocator == NULL) {
        CLOGE("defaultAllocator is NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bufIndex < 0) {
        CLOGE("Invalid parameters. bufIndex %d", bufIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    imagePlaneCount = m_getImagePlaneCount(m_buffer[bufIndex].planeCount);

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

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

void ServiceExynosCameraBufferManager::dumpBufferInfo(int fd)
{
    if (fd > 0) dprintf(fd, "[%s] ", m_name);

    m_allocator->dump(fd);
    ExynosCameraBufferManager::dumpBufferInfo(fd);

    return;
}
