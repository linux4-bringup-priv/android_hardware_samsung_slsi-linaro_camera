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

#ifndef EXYNOS_CAMERA_SERVICE_BUFFER_MANAGER_H__
#define EXYNOS_CAMERA_SERVICE_BUFFER_MANAGER_H__

#include "ExynosCameraBufferManager.h"

namespace android {

class ServiceExynosCameraBufferManager : protected ExynosCameraBufferManager {
public:
    ServiceExynosCameraBufferManager(int actualFormat);
    virtual ~ServiceExynosCameraBufferManager();

    bool     m_allocationThreadFunc(void);
    status_t setInfo(buffer_manager_configuration_t info);
    status_t resetBuffers(void);
    status_t alloc(void);
    status_t putBuffer(int bufIndex,
                       enum EXYNOS_CAMERA_BUFFER_POSITION position);
    status_t getBuffer(int    *reqBufIndex,
                       enum   EXYNOS_CAMERA_BUFFER_POSITION position,
                       struct ExynosCameraBuffer *buffer);

    void     dumpBufferInfo(int fd = -1);

    /*
     * The H/W fence sequence is
     * 1. On putBuffer (call by processCaptureRequest()),
     *    And, save acquire_fence, release_fence value.
     *    S/W fence : Make Fence class with acquire_fence on output_buffer.
     *
     * 2. when getBuffer,
     *    H/W fence : save acquire_fence, release_fence to ExynosCameraBuffer.
     *    S/W fence : wait Fence class that allocated at step 1.
     *
     *    (step 3 ~ step 4 is on ExynosCameraNode::m_qBuf())
     * 3. During H/W qBuf,
     *    give ExynosCameraBuffer.acquireFence (gotten step2) to v4l2
     *    v4l2_buffer.flags = V4L2_BUF_FLAG_USE_SYNC;
     *    v4l2_buffer.reserved = ExynosCameraBuffer.acquireFence;
     * 4. after H/W qBuf
     *    v4l2_buffer.reserved is changed to release_fence value.
     *    So,
     *    ExynosCameraBuffer.releaseFence = static_cast<int>(v4l2_buffer.reserved)
     *
     * 5. (step5 is on ExynosCamera::m_setResultBufferInfo())
     *    after H/W dqBuf, we meet handlePreview().
     *    we can set final value.
     *    result_buffer_info_t.acquire_fence = -1. (NOT original ExynosCameraBuffer.acquireFence)
     *    result_buffer_info_t.release_fence = ExynosCameraBuffer.releaseFence. (gotten by driver at step3)
     *    (service will look this release_fence.)
     *
     * 6  skip bufferManger::putBuffer().
     *     (we don't need to call putBuffer. because, buffer came from service.)
     *
     * 7. repeat from 1.
     */

protected:
    status_t m_setAllocator(void *allocator);
    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_getBufferInfoFromHandle(buffer_handle_t handle, int planeCount, /* out */ int fd[]);
    status_t m_checkBufferInfo(const buffer_handle_t handle, const ExynosCameraBuffer* buffer);

    virtual status_t m_waitFence(ExynosCameraFence *fence);

    status_t m_constructBufferContainer(int bufIndex);
    status_t m_destructBufferContainer(int bufIndex);

private:
    ExynosCameraStreamAllocator     *m_allocator;
    bool                            m_handleIsLocked[VIDEO_MAX_FRAME];

    List<ExynosCameraFence *>       m_fenceList;
    mutable Mutex                   m_fenceListLock;

    typedef ExynosCameraThread<ServiceExynosCameraBufferManager> allocThread;
    sp<allocThread>             m_allocationThread;

};


}
#endif
