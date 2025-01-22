/*
**
** Copyright 2017, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef EXYNOS_CAMERA_PIPE_GSC_H
#define EXYNOS_CAMERA_PIPE_GSC_H

#include "ExynosCameraSWPipe.h"
#include "csc.h"

#define CSC_HW_PROPERTY_DEFAULT ((CSC_HW_PROPERTY_TYPE)2) /* Not fixed mode */
#define CSC_MEMORY_TYPE         CSC_MEMORY_DMABUF /* (CSC_MEMORY_USERPTR) */

namespace android {

class ExynosCameraGSCWrapper : public RefBase {
public:
    ExynosCameraGSCWrapper(int gscNum):m_gscNum(gscNum) {
        m_csc = nullptr;
        m_property = CSC_HW_PROPERTY_DEFAULT;
    }

    ~ExynosCameraGSCWrapper() {
        destroy();
    }

    status_t create(void) {
        CSC_METHOD cscMethod = CSC_METHOD_HW;

        m_csc = csc_init(cscMethod);
        if (m_csc == NULL) {
            ALOGE("[gscWrapper][%d] csc_init() fail", m_gscNum);
            return INVALID_OPERATION;
        }

        csc_set_hw_property(m_csc, m_property, m_gscNum);
        return NO_ERROR;
    }

    void destroy(void) {
        if (m_csc != NULL)
            csc_deinit(m_csc);
        m_csc = NULL;
    }

    status_t convertWithRotation(ExynosRect srcRect,
                                    ExynosRect dstRect,
                                    ExynosCameraBuffer srcBuffer,
                                    ExynosCameraBuffer dstBuffer,
                                    int rotation = 0,
                                    int flipHorizontal = 0,
                                    int flipVertical = 0) {
        csc_set_src_format(m_csc,
                ALIGN_UP(srcRect.fullW, GSCALER_IMG_ALIGN),
                srcRect.fullH,
                srcRect.x, srcRect.y, srcRect.w, srcRect.h,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat),
                0);

        csc_set_dst_format(m_csc,
                dstRect.fullW, dstRect.fullH,
                dstRect.x, dstRect.y, dstRect.w, dstRect.h,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(dstRect.colorFormat),
                0);

        csc_set_src_buffer(m_csc,
                (void **)srcBuffer.fd, CSC_MEMORY_TYPE);

        csc_set_dst_buffer(m_csc,
                (void **)dstBuffer.fd, CSC_MEMORY_TYPE);

        if (csc_convert_with_rotation(m_csc, rotation, flipHorizontal, flipVertical) != 0) {
            ALOGE("[gscWrapper][%d] csc_convert() fail", m_gscNum);
            return INVALID_OPERATION;
        }

        return NO_ERROR;
    }

protected:
    int                     m_gscNum;
    void                   *m_csc;
    CSC_HW_PROPERTY_TYPE    m_property;
};

class ExynosCameraPipeGSC : protected virtual ExynosCameraSWPipe {
public:
    ExynosCameraPipeGSC()
    {
        m_init(NULL);
    }

    ExynosCameraPipeGSC(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums,
        cameraId_Info *camIdInfo) : ExynosCameraSWPipe(cameraId, configurations, obj_param, isReprocessing, nodeNums, camIdInfo)
    {
        m_init(nodeNums);
    }

    virtual status_t        create(int32_t *sensorIds = NULL);

protected:
    virtual status_t        m_destroy(void);
    virtual status_t        m_run(void);

private:
    void                    m_init(int32_t *nodeNums);

private:
    int                     m_gscNum;
    sp<class ExynosCameraGSCWrapper>    m_gscWrapper;
};

}; /* namespace android */

#endif
