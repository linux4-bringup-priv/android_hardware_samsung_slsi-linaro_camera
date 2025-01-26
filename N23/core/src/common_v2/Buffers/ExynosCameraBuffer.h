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

/*!
 * \file      ExynosCameraBuffer.h
 * \brief     hearder file for ExynosCameraBuffer
 * \author    Sunmi Lee(carrotsm.lee@samsung.com)
 * \date      2013/07/17
 *
 */

#ifndef EXYNOS_CAMERA_BUFFER_H__
#define EXYNOS_CAMERA_BUFFER_H__


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <linux/videodev2.h>
#include "videodev2_exynos_camera.h"

#include "ExynosCameraMemory.h"
#include "fimc-is-metadata.h"

namespace android {

/* release a single-use dmabuf at DQ */
#define V4L2_BUF_FLAG_DISPOSAL  0x10000000

/* metadata plane : non-cached buffer */
/* image plane (default) : non-cached buffer */
#define EXYNOS_CAMERA_BUFFER_1MB                        (1024*1024)
#define EXYNOS_CAMERA_BUFFER_WARNING_TIME_MARGIN        (100)  /* 0.1ms per 1MB */

#define EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED         (EXYNOS_ION_HEAP_SYSTEM_MASK)
#define EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED         (0)
#define EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED (1600 + EXYNOS_CAMERA_BUFFER_WARNING_TIME_MARGIN)  /* 1.6ms per 1MB */

#define EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED            (EXYNOS_ION_HEAP_SYSTEM_MASK)
#define EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED            (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC)
#define EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED_SYNC_FORCE (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC | ION_FLAG_SYNC_FORCE)
#define EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED    (670 + EXYNOS_CAMERA_BUFFER_WARNING_TIME_MARGIN)  /* 0.67ms per 1MB */

#define EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED          (EXYNOS_ION_HEAP_CAMERA_MASK)
#define EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED          (0)
#define EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED  (50)  /* 0.05ms */

#define EXYNOS_CAMERA_BUFFER_GRALLOC_WARNING_TIME       (3300 + EXYNOS_CAMERA_BUFFER_WARNING_TIME_MARGIN)  /* 3.3ms per 1MB */

#define EXYNOS_CAMERA_BUFFER_ION_MASK_SECURE            (EXYNOS_ION_HEAP_SECURE_CAMERA_MASK)
#define EXYNOS_CAMERA_BUFFER_ION_FLAG_SECURE            (ION_FLAG_PROTECTED)

typedef enum exynos_camera_buffer_type {
    EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE = 0,
    EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE    = 1,
    EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE  = 2,
    EXYNOS_CAMERA_BUFFER_ION_NONCACHED_RESERVED_TYPE = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE,
    EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE = 3,
    EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE = 4,
    EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_RESERVED_TYPE = 5,
    EXYNOS_CAMERA_BUFFER_ION_RESERVED_SECURE_TYPE = 6,
    EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_SECURE_TYPE = 7,
    EXYNOS_CAMERA_BUFFER_INVALID_TYPE,
} exynos_camera_buffer_type_t;

enum EXYNOS_CAMERA_BUFFER_POSITION {
    EXYNOS_CAMERA_BUFFER_POSITION_NONE = 0,
    EXYNOS_CAMERA_BUFFER_POSITION_IN_DRIVER,
    EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL,
    EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE,
    EXYNOS_CAMERA_BUFFER_POSITION_MAX
};

enum EXYNOS_CAMERA_BUFFER_PERMISSION {
    EXYNOS_CAMERA_BUFFER_PERMISSION_NONE = 0,
    EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE,
    EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS,
    EXYNOS_CAMERA_BUFFER_PERMISSION_MAX
};

struct ExynosCameraBufferStatus {
    int  driverReturnValue;
    enum EXYNOS_CAMERA_BUFFER_POSITION   position;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;
#ifdef __cplusplus
    ExynosCameraBufferStatus() {
        driverReturnValue = 0;
        position   = EXYNOS_CAMERA_BUFFER_POSITION_NONE;
        permission = EXYNOS_CAMERA_BUFFER_PERMISSION_NONE;
    }

    ExynosCameraBufferStatus& operator =(const ExynosCameraBufferStatus &other) {
        driverReturnValue = other.driverReturnValue;
        position   = other.position;
        permission = other.permission;

        return *this;
    }

    bool operator ==(const ExynosCameraBufferStatus &other) const {
        bool ret = true;

        if (driverReturnValue != other.driverReturnValue
        || position   != other.position
        || permission != other.permission) {
            ret = false;
        }

        return ret;
    }

    bool operator !=(const ExynosCameraBufferStatus &other) const {
        return !(*this == other);
    }
#endif
};

typedef struct buffer_manager_tag {
    int pipeId[5];
    int managerType;
    bool multiCameraSharable;
    int cameraId[5];
    int camId;

    // hidden identifier
    union {
        int32_t i32;
    } reserved;

    buffer_manager_tag() {
        int length = sizeof(pipeId) / sizeof(pipeId[0]);
        for (int i = 0; i < length; i++) {
            pipeId[i] = -1;
            cameraId[i] = -1;
        }
        managerType = -1;
        reserved.i32 = -1;
        managerType  = -1;
        multiCameraSharable = true;
        camId = -1;
    }

    buffer_manager_tag& operator =(const buffer_manager_tag &other) {
        int length = sizeof(pipeId) / sizeof(pipeId[0]);
        for (int i = 0; i < length; i++) {
            this->pipeId[i] = other.pipeId[i];
            this->cameraId[i] = other.cameraId[i];
        }
        this->managerType = other.managerType;
        this->reserved.i32 = other.reserved.i32;
        this->multiCameraSharable = other.multiCameraSharable;
        this->camId = other.camId;

        return *this;
    }

    bool operator ==(const buffer_manager_tag &other) const {
        if (this->managerType != other.managerType) {
            return false;
        }

        if (this->multiCameraSharable != other.multiCameraSharable) {
            return false;
        }

        if (this->multiCameraSharable == false) {
            bool cameraIdMatch = false;
            for (int i = 0; i < 5 && this->cameraId[i] > -1; i++) {
                for (int j = 0; j < 5 && other.cameraId[j] > -1; j++) {
                    if (this->cameraId[i] == other.cameraId[j]) {
                        cameraIdMatch = true;
                    }
                }
            }

            if (cameraIdMatch == false) {
                return false;
            }
        }

        for (int i = 0; i < 5 && this->pipeId[i] > -1; i++) {
            for (int j = 0; j < 5 && other.pipeId[j] > -1; j++) {
                if (this->pipeId[i] == other.pipeId[j]) {
                    return true;
                }
            }
        }

        return false;
    }

    bool operator !=(const buffer_manager_tag &other) const {
        return !(*this == other);
    }
} buffer_manager_tag_t;

struct ExynosCameraBuffer {
    int                             index;
    int                             planeCount;
    int                             batchSize;
    int                             fd[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int                             containerFd[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    unsigned int                    size[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    unsigned int                    bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    char                            *addr[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    struct ExynosCameraBufferStatus status;
    exynos_camera_buffer_type_t     type;   /* this value in effect exclude metadataPlane*/
    buffer_manager_tag_t            tag;
    bool                            hasMetaPlane;
    bool                            hasDebugInfoPlane;

    buffer_handle_t                 *handle[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int                             acquireFence[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int                             releaseFence[EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int                             refCount;

    const char      *bufMgrNm;
    void            *manager;
    unsigned int                    v4l2Flags;
    bool                            dynamicV4l2Index;
    union {
        uint8_t u8[4];
        uint32_t u32;
    } reserved;

#ifdef __cplusplus
    ExynosCameraBuffer() {
        index = -1;
        planeCount = 0;
        batchSize = 1;
        hasMetaPlane = false;
        hasDebugInfoPlane = false;
        for (int planeIndex = 0; planeIndex < EXYNOS_CAMERA_BUFFER_MAX_PLANES; planeIndex++) {
            fd[planeIndex]   = -1;
            containerFd[planeIndex] = -1;
            size[planeIndex] = 0;
            bytesPerLine[planeIndex] = 0;
            addr[planeIndex] = NULL;
            handle[planeIndex] = NULL;
            acquireFence[planeIndex] = -1;
            releaseFence[planeIndex] = -1;
        }
        status.driverReturnValue = 0;
        status.position          = EXYNOS_CAMERA_BUFFER_POSITION_NONE;
        status.permission        = EXYNOS_CAMERA_BUFFER_PERMISSION_NONE;
        type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;

        refCount = 0;
        bufMgrNm = NULL;
        manager = NULL;
        v4l2Flags = 0;
        dynamicV4l2Index = false;
        reserved.u32 = 0;
    }

    int getLastImagePlaneIndex() {
        int lastIndex = ((planeCount - (hasMetaPlane + hasDebugInfoPlane)) * batchSize) - 1;
        if (lastIndex < 0) {
            LOG_ALWAYS_FATAL("ASSERT(%s[%d]): invalid index(%d)", __FUNCTION__, __LINE__, lastIndex);
        }
        return lastIndex;
    }

    int getMetaPlaneIndex() {
        int metadataPlaneIndex = -1;

        if(hasDebugInfoPlane == true)
           metadataPlaneIndex = (planeCount - 2) * batchSize;
        else
           metadataPlaneIndex = (planeCount - 1) * batchSize;

        if (size[metadataPlaneIndex] != EXYNOS_CAMERA_META_PLANE_SIZE) {
            android_printAssert(NULL, LOG_TAG,
                                "ASSERT(%s):Invalid access to metadata plane. planeIndex %d size %d",
                                __FUNCTION__, metadataPlaneIndex, size[metadataPlaneIndex]);
        }

        return metadataPlaneIndex;
    }

    int getDebugInfoPlaneIndex() {
        int debugInfoPlaneIndex = -1;

        if (hasMetaPlane == true)
           debugInfoPlaneIndex = (planeCount - 2) * batchSize+1;
        else
           debugInfoPlaneIndex = (planeCount - 1) * batchSize;

        if (size[debugInfoPlaneIndex] != EXYNOS_CAMERA_DEBUG_INFO_PLANE_SIZE) {
            android_printAssert(NULL, LOG_TAG,
                                "ASSERT(%s):Invalid access to metadata plane. planeIndex %d size %d",
                                __FUNCTION__, debugInfoPlaneIndex, size[debugInfoPlaneIndex]);
        }

        return debugInfoPlaneIndex;
    }

    ExynosCameraBuffer& operator =(const ExynosCameraBuffer &other) {
        index      = other.index;
        planeCount = other.planeCount;
        batchSize  = other.batchSize;
        hasMetaPlane = other.hasMetaPlane;
        hasDebugInfoPlane = other.hasDebugInfoPlane;
        for (int i = 0; i < EXYNOS_CAMERA_BUFFER_MAX_PLANES; i++) {
            fd[i]           = other.fd[i];
            containerFd[i]  = other.containerFd[i];
            size[i]         = other.size[i];
            bytesPerLine[i] = other.bytesPerLine[i];
            addr[i]         = other.addr[i];
            handle[i]       = other.handle[i];
            acquireFence[i] = other.acquireFence[i];
            releaseFence[i] = other.releaseFence[i];
        }
        status     = other.status;
        type       = other.type;
        tag        = other.tag;
        bufMgrNm   = other.bufMgrNm;

        manager = other.manager;
        refCount = other.refCount;
        reserved.u32 = other.reserved.u32;
        v4l2Flags = other.v4l2Flags;
        dynamicV4l2Index = other.dynamicV4l2Index;

        return *this;
    }

    bool operator ==(const ExynosCameraBuffer &other) const {
        bool ret = true;

        if (index != other.index
        || planeCount != other.planeCount
        || batchSize != other.batchSize
        || status != other.status
        || type   != other.type
        || manager != other.manager
        || v4l2Flags != other.v4l2Flags
        || dynamicV4l2Index != other.dynamicV4l2Index
        || reserved.u32 != other.reserved.u32) {
            ret = false;
        }

        for (int i = 0; i < EXYNOS_CAMERA_BUFFER_MAX_PLANES; i++) {
            if (fd[i]  != other.fd[i]
            || containerFd[i] != other.containerFd[i]
            || size[i] != other.size[i]
            || bytesPerLine[i] != other.bytesPerLine[i]
            || addr[i] != other.addr[i]
            || handle[i] != other.handle[i]
            || acquireFence[i] != other.acquireFence[i]
            || releaseFence[i] != other.releaseFence[i]
            || v4l2Flags != other.v4l2Flags) {
                ret = false;
                break;
            }
        }

        return ret;
    }

    bool operator !=(const ExynosCameraBuffer &other) const {
        return !(*this == other);
    }

    // implemeted on cpp file
    void dump();
#endif
};

typedef struct buffer_dump_info {
    ExynosCameraBuffer  buffer;
    int                 format;
    int                 width;
    int                 height;
    uint32_t            frameCount;
    int                 camId;
    int                 pipeId;
    char                name[128];

    buffer_dump_info() {
        format = -1;
        width = -1;
        height = -1;
        frameCount = 0;
        camId = -1;
        pipeId = -1;
        for (size_t i = 0; i < sizeof(name)/sizeof(name[0]); i++) {
            name[i] = '\0';
        }
    }

    buffer_dump_info& operator =(const buffer_dump_info &other) {
        this->buffer = other.buffer;
        this->format = other.format;
        this->width = other.width;
        this->height = other.height;
        this->frameCount = other.frameCount;
        this->camId = other.camId;
        this->pipeId = other.pipeId;
        memcpy(this->name, other.name, sizeof(name));

        return *this;
    }

    char* makeFileName() {
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        snprintf(name, sizeof(name),
                "%s%02d%02d%02d_%02d%02d%02d_CAM%d_P%d_F%d_%dx%d.raw",
                CAMERA_DATA_PATH, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
                timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                camId, pipeId, frameCount, width, height);
        return name;
    }

} buffer_dump_info_t;

}
#endif
