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

#ifndef EXYNOS_CAMERA_BUFFER_MANAGER_H__
#define EXYNOS_CAMERA_BUFFER_MANAGER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/List.h>
#include <utils/threads.h>
#include <cutils/properties.h>

#include <ui/Fence.h>
#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <hardware/exynos/ion.h>

#include <list>

#include "ExynosCameraCommonInclude.h"
#include "fimc-is-metadata.h"

#include "ExynosCameraObject.h"
#include "ExynosCameraList.h"
#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraMemory.h"
#include "ExynosCameraThread.h"

namespace android {

/* #define DUMP_2_FILE */
/* #define EXYNOS_CAMERA_BUFFER_TRACE */

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
#define EXYNOS_CAMERA_BUFFER_IN()   CLOGD("IN..")
#define EXYNOS_CAMERA_BUFFER_OUT()  CLOGD("OUT..")
#else
#define EXYNOS_CAMERA_BUFFER_IN()   ((void *)0)
#define EXYNOS_CAMERA_BUFFER_OUT()  ((void *)0)
#endif

#ifdef USE_GIANT_MSCL
#define GIANT_MARIN_SIZE (256)
#define GIANT_MIN_SIZE   (33177600)  //33177600 = 7680x4320 (8K UHD)
#endif

// Hack: Close Fence FD if the fd is larger than specified number
// Currently, Joon's fence FD is not closed properly
/* #define FORCE_CLOSE_ACQUIRE_FD */
#define FORCE_CLOSE_ACQUIRE_FD_THRESHOLD    700

#define SWBUFFER_MAX_COUNT                  512

typedef enum buffer_manager_type {
    BUFFER_MANAGER_ION_TYPE                 = 0,
    BUFFER_MANAGER_FASTEN_AE_ION_TYPE       = 1,
    BUFFER_MANAGER_SERVICE_GRALLOC_TYPE     = 2,
    BUFFER_MANAGER_ONLY_HAL_USE_ION_TYPE    = 3,
    BUFFER_MANAGER_REMOSAIC_ION_TYPE        = 4,
    BUFFER_MANAGER_REMOSAIC_ONLY_HAL_USE_ION_TYPE    = 5,
    BUFFER_MANAGER_SERVICE_SECURE_TYPE      = 6,
    BUFFER_MANAGER_INVALID_TYPE,
} buffer_manager_type_t;

typedef enum buffer_manager_allocation_mode {
    BUFFER_MANAGER_ALLOCATION_ATONCE   = 0,   /* alloc() : allocation all buffers */
    BUFFER_MANAGER_ALLOCATION_ONDEMAND = 1,   /* alloc() : allocation the number of reqCount buffers, getBuffer() : increase buffers within limits */
    BUFFER_MANAGER_ALLOCATION_SILENT   = 2,   /* alloc() : same as ONDEMAND, increase buffers in background */
    BUFFER_MANAGER_ALLOCATION_ONDEMAND_RELEASE = 3,   /* alloc() : allocation the number of reqCount buffers, getBuffer() : increase buffers within limits */
    BUFFER_MANAGER_ALLOCATION_INVALID_MODE,
} buffer_manager_allocation_mode_t;

typedef struct buffer_manager_configuration {
    typedef struct debug_info {
        int width;
        int height;
        int format;

        debug_info& operator =(const debug_info &other) {
            this->width = other.width;
            this->height = other.height;
            this->format = other.format;
            return *this;
        }

        debug_info& operator =(const int &other) {
            this->width = other;
            this->height = other;
            this->format = other;
            return *this;
        }

    } debug_info_t;

    int planeCount;
    unsigned int size[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int startBufIndex;
    int reqBufCount;
    int allowedMaxBufCount;
    int batchSize;
    exynos_camera_buffer_type_t type;
    buffer_manager_allocation_mode_t allocMode;
    bool createMetaPlane;
    bool createDebugInfoPlane;
    bool needMmap;
    int reservedMemoryCount;
    debug_info_t debugInfo;
    unsigned int v4l2Flags;

    buffer_manager_configuration() {
        planeCount = 0;
        memset(size, 0x00, sizeof(size));
        memset(bytesPerLine, 0x00, sizeof(bytesPerLine));
        startBufIndex = 0;
        reqBufCount = 0;
        allowedMaxBufCount = 0;
        batchSize = 1;
        type = EXYNOS_CAMERA_BUFFER_INVALID_TYPE;
        allocMode = BUFFER_MANAGER_ALLOCATION_INVALID_MODE;
        createMetaPlane = false;
        createDebugInfoPlane = false;
        needMmap = false;
        reservedMemoryCount = 0;
        debugInfo = 0;
        v4l2Flags = 0;
    }

    buffer_manager_configuration& operator =(const buffer_manager_configuration &other) {
        this->planeCount = other.planeCount;
        memcpy(this->size, other.size, sizeof(this->size));
        memcpy(this->bytesPerLine, other.bytesPerLine, sizeof(this->bytesPerLine));
        this->startBufIndex = other.startBufIndex;
        this->reqBufCount = other.reqBufCount;
        this->allowedMaxBufCount = other.allowedMaxBufCount;
        this->batchSize = other.batchSize;
        this->type = other.type;
        this->allocMode = other.allocMode;
        this->createMetaPlane = other.createMetaPlane;
        this->createDebugInfoPlane = other.createDebugInfoPlane;
        this->needMmap = other.needMmap;
        this->reservedMemoryCount = other.reservedMemoryCount;
        this->debugInfo = other.debugInfo;
        this->v4l2Flags = other.v4l2Flags;

        return *this;
    }
} buffer_manager_configuration_t;

class ExynosCameraBufferManager : public ExynosCameraObject {
protected:
    ExynosCameraBufferManager(uint32_t maxBufferCount);

public:
    virtual ~ExynosCameraBufferManager();

    status_t         create(const char *name, void *defaultAllocator);
    status_t         create(const char *name, int cameraId, void *defaultAllocator);

    void             init(void);
    virtual void     deinit(void);
    virtual status_t resetBuffers(void) = 0;

    status_t         setAllocator(void *allocator);

    void             setType(buffer_manager_type_t type) { m_bufferManagerType = type; };

    void             setContigBufCount(int reservedMemoryCount);
    int              getContigBufCount(void);

    virtual status_t setInfo(buffer_manager_configuration_t info);
    virtual status_t alloc(void) = 0;

    virtual status_t increaseBufferRefCnt(int bufIndex);
    virtual status_t putBuffer(
                        int bufIndex,
                        enum EXYNOS_CAMERA_BUFFER_POSITION position);
    virtual status_t getBuffer(
                        int    *reqBufIndex,
                        enum   EXYNOS_CAMERA_BUFFER_POSITION position,
                        struct ExynosCameraBuffer *buffer);

    virtual status_t putBatchBufferIndex(int bufIndex);
    virtual status_t getBatchBufferIndex(int &bufIndex);

    virtual status_t updateStatus(
                        int bufIndex,
                        int driverValue,
                        enum EXYNOS_CAMERA_BUFFER_POSITION   position,
                        enum EXYNOS_CAMERA_BUFFER_PERMISSION permission);
    virtual status_t getStatus(
                        int bufIndex,
                        struct ExynosCameraBufferStatus *bufStatus);

    virtual status_t getIndexByFd(int fd, int *index);

    bool             isAllocated(void);
    virtual bool     isAvailable(int bufIndex);

    void             dump(int fd = -1);
    virtual void     dumpBufferInfo(int fd = -1);
    int              getAllocatedBufferCount(void);
    int              getAvailableIncreaseBufferCount(void);
    virtual int      getNumOfAvailableBuffer(void);
    virtual int      getNumOfAvailableAndNoneBuffer(void);
    int              getNumOfAllowedMaxBuffer(void);
    void             printBufferInfo(
                        const char *funcName,
                        const int lineNum,
                        int bufIndex,
                        int planeIndex);
    void             printBufferQState(void);
    virtual void     printBufferState(void);
    virtual void     printBufferState(int bufIndex, int planeIndex);

    virtual status_t increase(int increaseCount);
    virtual status_t cancelBuffer(int bufIndex);
    virtual status_t setBufferCount(int bufferCount);
    virtual int      getBufferCount(void);
    virtual int      getBufStride(void);

protected:
    virtual bool     m_allocationThreadFunc(void) = 0;
    status_t         m_free(void);

    status_t         m_setDefaultAllocator(void *allocator);
    virtual status_t m_defaultAlloc(int bIndex, int eIndex, unsigned int extraPlane);
    virtual status_t m_defaultFree(int bIndex, int eIndex, unsigned int extraPlane);
    virtual bool     m_checkInfoForAlloc(void);
    status_t         m_createDefaultAllocator(bool isCached = false);
    int              m_getTotalPlaneCount(int planeCount, int batchSize, bool hasMetaPlane, bool hasDebugInfoPlane = false);
    int              m_getImagePlaneCount(int planeCount);

    virtual void     m_resetSequenceQ(void);

    virtual status_t m_setInfo(buffer_manager_configuration_t info, bool isServiceBuffer);

    virtual status_t m_setAllocator(void *allocator) = 0;
    virtual status_t m_alloc(int bIndex, int eIndex) = 0;
    virtual status_t m_free(int bIndex, int eIndex)  = 0;

    virtual status_t m_increase(int increaseCount) = 0;
    virtual status_t m_decrease(void) = 0;

protected:
    bool                        m_flagAllocated;
    int                         m_reservedMemoryCount;
    int                         m_reqBufCount;
    int                         m_allocatedBufCount;
    int                         m_allowedMaxBufCount;
    bool                        m_flagSkipAllocation;
    bool                        m_isDestructor;
    mutable Mutex               m_lock;
    bool                        m_flagNeedMmap;

    bool                        m_hasMetaPlane;
    /* using internal allocator (ION) for MetaData plane */
    bool                        m_hasDebugInfoPlane;

    ExynosCameraIonAllocator    *m_defaultAllocator;
    bool                        m_isCreateDefaultAllocator;
    struct ExynosCameraBuffer   *m_buffer;
    std::list<int>              m_availableBufferIndexQ;
    List<int>                   m_availableBatchBufferIndexQ;
    mutable Mutex               m_availableBufferIndexQLock;

    buffer_manager_allocation_mode_t m_allocMode;
    int                         m_indexOffset;
    static uint64_t             m_sReservedBufferSize;
    static uint64_t             m_sIonBufferSize;
    buffer_manager_configuration_t m_config;
    buffer_manager_type_t       m_bufferManagerType;

    const uint32_t              m_maxBufferCount;
};

class ExynosCameraFence {
public:
    enum EXYNOS_CAMERA_FENCE_TYPE {
        EXYNOS_CAMERA_FENCE_TYPE_BASE = 0,
        EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE,
        EXYNOS_CAMERA_FENCE_TYPE_RELEASE,
        EXYNOS_CAMERA_FENCE_TYPE_MAX,
    };

private:
    ExynosCameraFence();

public:
    ExynosCameraFence(
                enum EXYNOS_CAMERA_FENCE_TYPE fenceType,
                int acquireFence,
                int releaseFence);

    virtual ~ExynosCameraFence();

    int  getFenceType(void);
    int  getAcquireFence(void);
    int  getReleaseFence(void);

    bool isValid(void);
    status_t wait(int time = -1);

private:
    enum EXYNOS_CAMERA_FENCE_TYPE m_fenceType;

    int       m_acquireFence;
    int       m_releaseFence;

    sp<Fence> m_fence;

    bool      m_flagSwfence;
};

}
#endif
