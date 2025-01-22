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

#define LOG_TAG "ExynosCameraMemoryAllocator"
#include "ExynosCameraMemory.h"
#include <hardware/exynos/dmabuf_container.h>

#include "ExynosGraphicBuffer.h"

using namespace vendor::graphics;

namespace android {

ExynosCameraIonAllocator::ExynosCameraIonAllocator()
{
    m_ionClient   = -1;
    m_ionAlign    = 0;
    m_ionHeapMask = 0;
    m_ionFlags    = 0;
}

ExynosCameraIonAllocator::~ExynosCameraIonAllocator()
{
    close(m_ionClient);
}

status_t ExynosCameraIonAllocator::init(bool isCached)
{
    status_t ret = NO_ERROR;

    if (m_ionClient < 0) {
        m_ionClient = exynos_ion_open();

        if (m_ionClient < 0) {
            CLOGE2("ERR:exynos_ion_open(%d) failed", m_ionClient);
            ret = BAD_VALUE;
            goto func_exit;
        }
    }

    m_ionAlign    = 0;
    m_ionHeapMask = EXYNOS_ION_HEAP_SYSTEM_MASK;
    m_ionFlags    = (isCached == true ?
        (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC ) : 0);

func_exit:

    return ret;
}

status_t ExynosCameraIonAllocator::alloc(
        int size,
        int *fd,
        char **addr,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = 0;
    char *ionAddr = NULL;

    if (m_ionClient < 0) {
        CLOGE2("ERR:allocator is not yet created");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (size == 0) {
        CLOGE2("ERR:size equals zero");
        ret = BAD_VALUE;
        goto func_exit;
    }

    ionFd = exynos_ion_alloc(m_ionClient, size, m_ionHeapMask, m_ionFlags);
    if (ionFd < 0) {
        CLOGE2("ERR:exynos_ion_alloc(fd=%d) failed(%s)", ionFd, strerror(errno));
        ionFd = -1;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (map(size, ionFd, &ionAddr) != NO_ERROR) {
            CLOGE2("ERR:map failed");
        }
    }

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::alloc(
        int size,
        int *fd,
        char **addr,
        int  mask,
        int  flags,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = 0;
    char *ionAddr = NULL;

    if (m_ionClient < 0) {
        CLOGE2("ERR:allocator is not yet created");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (size == 0) {
        CLOGE2("ERR:size equals zero");
        ret = BAD_VALUE;
        goto func_exit;
    }

    ionFd = exynos_ion_alloc(m_ionClient, size, mask, flags);
    if (ionFd < 0) {
        CLOGE2("ERR:exynos_ion_alloc(fd=%d) failed(%s)", ionFd, strerror(errno));
        ionFd = -1;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (map(size, ionFd, &ionAddr) != NO_ERROR) {
            CLOGE2("ERR:map failed");
        }
    }

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::free(
        __unused int size,
        int *fd,
        char **addr,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = *fd;
    char *ionAddr = *addr;

    if (ionFd < 0) {
        CLOGE2("ERR:ion_fd is lower than zero");
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (ionAddr == NULL) {
            CLOGE2("ERR:ion_addr equals NULL");
            ret = BAD_VALUE;
            goto func_close_exit;
        }

        if (munmap(ionAddr, size) < 0) {
            CLOGE2("ERR:munmap failed");
            ret = INVALID_OPERATION;
            goto func_close_exit;
        }
    }

func_close_exit:

    close(ionFd);

    ionFd   = -1;
    ionAddr = NULL;

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::map(int size, int fd, char **addr)
{
    status_t ret = NO_ERROR;
    char *ionAddr = NULL;

    if (size == 0) {
        CLOGE2("ERR:size equals zero");
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (fd < 0) {
        CLOGE2("ERR:fd=%d failed", fd);
        ret = BAD_VALUE;
        goto func_exit;
    }

    ionAddr = (char *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (ionAddr == (char *)MAP_FAILED || ionAddr == NULL) {
        CLOGE2("ERR:ion_map(size=%d) failed, (fd=%d), (%s)", size, fd, strerror(errno));
        close(fd);
        ionAddr = NULL;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    *addr = ionAddr;

    return ret;
}

void ExynosCameraIonAllocator::setIonHeapMask(int mask)
{
    m_ionHeapMask |= mask;
}

void ExynosCameraIonAllocator::setIonFlags(int flags)
{
    m_ionFlags |= flags;
}

status_t ExynosCameraIonAllocator::createBufferContainer(int *fd, int batchSize, int *containerFd)
{
    int bufferContainerFd = -1;
    if (fd == NULL || batchSize < 1 || containerFd == NULL) {
        CLOGE2("ERR:Invalid parameters. fd %p batchSize %d containerFd %p",
                fd, batchSize, containerFd);
        return BAD_VALUE;
    }

    bufferContainerFd = dma_buf_merge(*fd, fd + 1, batchSize - 1);
    if (bufferContainerFd < 0) {
        CLOGE2("ERR:Failed to create BufferContainer. batchSize %d containerFd %d",
                batchSize, bufferContainerFd);
        return INVALID_OPERATION;
    }

    CLOGV2("DEBUG:Success to create BufferContiner. batchSize %d containerFd %d",
            batchSize, bufferContainerFd);

    *containerFd = bufferContainerFd;

    return NO_ERROR;
}

ExynosCameraStreamAllocator::ExynosCameraStreamAllocator(int actualFormat)
{
    m_allocator = NULL;
    m_actualFormat = actualFormat;
}

ExynosCameraStreamAllocator::~ExynosCameraStreamAllocator()
{
}

status_t ExynosCameraStreamAllocator::init(camera3_stream_t *allocator)
{
    status_t ret = NO_ERROR;

    m_allocator = allocator;

    return ret;
}

int ExynosCameraStreamAllocator::lock(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        bool *isLocked,
        int planeCount)
{
    int ret = 0;
    uint64_t usage  = 0;
    uint32_t format = 0;
    void  *grallocAddr[3] = {NULL};
    int   grallocFd[3] = {0};
    ExynosCameraDurationTimer   lockbufferTimer;

    static ExynosGraphicBufferMapper& gmapper(ExynosGraphicBufferMapper::get());

    status_t error = NO_ERROR;
    android_ycbcr ycbcrLayout;
    Rect rect;
    ExynosGraphicBufferMeta gmeta;

    if (bufHandle == NULL) {
        CLOGE2("ERR:bufHandle equals NULL, failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (*bufHandle == NULL) {
        CLOGE2("ERR:*bufHandle == NULL, failed");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    ycbcrLayout.y = NULL;
    ycbcrLayout.cb = NULL;
    ycbcrLayout.cr = NULL;
    ycbcrLayout.ystride = 0;
    ycbcrLayout.cstride = 0;
    ycbcrLayout.chroma_step = 0;

    rect.left = 0;
    rect.top = 0;
    switch (m_actualFormat) {
    case HAL_PIXEL_FORMAT_BLOB:
        //GRALLOC Spec:
        rect.right  = m_allocator->width * m_allocator->height;
        rect.bottom = 1;
        break;
    default:
        rect.right  = m_allocator->width;
        rect.bottom = m_allocator->height;
        break;
    }
    usage  = m_allocator->usage;
    format = m_allocator->format;

    switch (m_actualFormat) {
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_RAW10:
    case HAL_PIXEL_FORMAT_Y16:
    case HAL_PIXEL_FORMAT_Y8:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_BLOB:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        if (planeCount == 1) {
            lockbufferTimer.start();
            error = gmapper.lock(
                    **bufHandle,
                    usage,
                    rect,
                    grallocAddr);
            lockbufferTimer.stop();
            break;
        }
    default:
        lockbufferTimer.start();

    /* In gralloc4,
         HAL uses lockYCbCr64 instead of lockYCbCr to get addr for plane 0,1
         But, to get addr for plane 2, HAL uses the function such as below example.
           ex) grallocFd[2] = ExynosGraphicBufferMeta::get_video_metadata(**bufHandle) */

        error = gmapper.lockYCbCr64(**bufHandle,
                usage,
                rect,
                &ycbcrLayout);

        lockbufferTimer.stop();
        break;
    }

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    CLOGD2("DEBUG:Check grallocHAL lock performance, duration(%ju usec)",
            lockbufferTimer.durationUsecs());
#else
    if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        CLOGW2("WRN:grallocHAL->lock() duration(%ju msec)",
                lockbufferTimer.durationMsecs());
#endif

    gmeta.init(**bufHandle);

    if (error != NO_ERROR) {
        CLOGE2("ERR:grallocHal->lock failed.. ");
        if (**bufHandle == NULL) {
            CLOGE2("ERR:[H%p]private_handle is NULL",
                    **bufHandle);
        } else {
            CLOGE2("ERR:[H%p]fd %d/%d/%d usage %jX(P%jx/C%jx) format %X(I%jx/F%x) rect %d,%d,%d,%d size %d/%d/%d",
                    **bufHandle,
                    gmeta.fd, gmeta.fd1, gmeta.fd2,
                    usage, gmeta.producer_usage, gmeta.consumer_usage,
                    format, gmeta.internal_format, gmeta.frameworkFormat,
                    rect.left, rect.top, rect.right, rect.bottom,
                    gmeta.size, gmeta.size1, gmeta.size2);
        }

        ret = INVALID_OPERATION;
        goto func_exit;
    }

    switch (m_actualFormat) {
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        grallocFd[2] = gmeta.fd2;
        grallocAddr[2] = ycbcrLayout.cb;
        grallocFd[1] = gmeta.fd1;
        grallocAddr[1] = ycbcrLayout.cr;
        grallocFd[0] = gmeta.fd;
        grallocAddr[0] = ycbcrLayout.y;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
#if defined(SUPPORT_EXYNOS_GRAPHIC_USAGE) && (SUPPORT_EXYNOS_GRAPHIC_USAGE == 1)
        if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
            /* Use VideoMeta data */
            grallocFd[2] = gmeta.fd2;
            grallocAddr[2] = ExynosGraphicBufferMeta::get_video_metadata(**bufHandle);
        }
#endif
        grallocFd[1] = gmeta.fd1;
        grallocAddr[1] = ycbcrLayout.cr;
        grallocFd[0] = gmeta.fd;
        grallocAddr[0] = ycbcrLayout.y;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        grallocFd[2] = gmeta.fd2;
        grallocAddr[2] = ycbcrLayout.cr;
        grallocFd[1] = gmeta.fd1;
        grallocAddr[1] = ycbcrLayout.cb;
        grallocFd[0] = gmeta.fd;
        grallocAddr[0] = ycbcrLayout.y;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
        if (planeCount == 3) {
            /* Use VideoMeta data */
            grallocFd[2] = gmeta.fd2;
            grallocAddr[2] = ExynosGraphicBufferMeta::get_video_metadata(**bufHandle);
        }
        grallocFd[1] = gmeta.fd1;
        grallocAddr[1] = ycbcrLayout.cb;
        grallocFd[0] = gmeta.fd;
        grallocAddr[0] = ycbcrLayout.y;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
#if defined(SUPPORT_EXYNOS_GRAPHIC_USAGE) && (SUPPORT_EXYNOS_GRAPHIC_USAGE == 1)
        if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
            /* Use VideoMeta data */
            grallocFd[2] = gmeta.fd2;
            grallocAddr[2] = ExynosGraphicBufferMeta::get_video_metadata(**bufHandle);
        }
#endif
        grallocFd[1] = gmeta.fd1;
        grallocAddr[1] = ycbcrLayout.cb;
        grallocFd[0] = gmeta.fd;
        grallocAddr[0] = ycbcrLayout.y;
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_BLOB:
    case HAL_PIXEL_FORMAT_Y8:
    case HAL_PIXEL_FORMAT_Y16:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I:
    case HAL_PIXEL_FORMAT_EXYNOS_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_EXYNOS_CrYCbY_422_I:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
    default:
        grallocFd[0] = gmeta.fd;
        if (grallocAddr[0] == NULL) {
            grallocAddr[0] = ycbcrLayout.y;
        }
        break;
    }

    *isLocked    = true;

func_exit:
    switch (planeCount) {
    case 3:
        fd[2]   = grallocFd[2];
        addr[2] = (char *)grallocAddr[2];
    case 2:
        fd[1]   = grallocFd[1];
        addr[1] = (char *)grallocAddr[1];
    case 1:
        fd[0]   = grallocFd[0];
        addr[0] = (char *)grallocAddr[0];
        break;
    default:
        break;
    }

    return ret;
}

int ExynosCameraStreamAllocator::unlock(buffer_handle_t *bufHandle)
{
    int ret = 0;
    int releaseFence = -1;
    static ExynosGraphicBufferMapper& gmapper(ExynosGraphicBufferMapper::get());

    if (bufHandle == NULL) {
        CLOGE2("ERR:bufHandle equals NULL");
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    releaseFence = gmapper.unlock(*bufHandle);

func_exit:
    return ret;
}

void ExynosCameraStreamAllocator::dump(int fd)
{
    CLOGD2("max request count : %d", m_allocator->max_buffers);
    if (fd > 0) {
        dprintf(fd, "max request count : %d\n", m_allocator->max_buffers);
    }
}

}
