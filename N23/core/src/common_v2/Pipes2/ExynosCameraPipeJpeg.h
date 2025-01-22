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

#ifndef EXYNOS_CAMERA_PIPE_JPEG_H
#define EXYNOS_CAMERA_PIPE_JPEG_H

#include "ExynosCameraSWPipe.h"
#include "ExynosJpegEncoderForCamera.h"
#include "ExynosCameraPipeGSC.h"

namespace android {

class ExynosCameraPipeJpeg : public virtual ExynosCameraSWPipe {
public:
    ExynosCameraPipeJpeg()
    {
        m_init();
    }

    ExynosCameraPipeJpeg(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums,
        cameraId_Info *camIdInfo) : ExynosCameraSWPipe(cameraId, configurations, obj_param, isReprocessing, nodeNums, camIdInfo)
    {
        m_init();
    }

    ~ExynosCameraPipeJpeg()
    {
        if (m_shot_ext) {
            delete m_shot_ext;
            m_shot_ext = NULL;
        }
    }

    virtual status_t        stop(void);

protected:
    virtual status_t        m_destroy(void);
    virtual status_t        m_run(void);

private:
    void                    m_init(void);

    status_t                m_prepareYuv(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer srcBuf,
                                        ExynosCameraBuffer *mainBuf,
                                        ExynosCameraBuffer *thumbBuf,
                                        ExynosRect srcRect,
                                        ExynosRect *mainRect,
                                        ExynosRect *thumbRect);

    status_t                m_getMainImage(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer srcBuf,
                                        ExynosCameraBuffer *dstBuf,
                                        ExynosRect srcRect,
                                        ExynosRect *dstRect);

    status_t                m_getThumbImage(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer srcBuf,
                                        ExynosCameraBuffer *dstBuf,
                                        ExynosRect srcRect,
                                        ExynosRect *dstRect);

    bool                    m_rotate(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer *srcBuf,
                                        ExynosCameraBuffer *dstBuf,
                                        ExynosRect *srcRect,
                                        ExynosRect *dstRect);

    bool                    m_giantScale(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer srcBuf,
                                        ExynosCameraBuffer dstBuf,
                                        ExynosRect srcRect,
                                        ExynosRect dstRect,
                                        int rotation = 0);

    bool                    m_handlePostScalerZoom(ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraBuffer srcBuf,
                                        ExynosCameraBuffer *dstBuf,
                                        ExynosRect srcRect,
                                        ExynosRect *dstRect,
                                        int rotation = 0);

private:
    ExynosJpegEncoderForCamera  m_jpegEnc;
    struct camera2_shot_ext    *m_shot_ext;

    sp<class ExynosCameraGSCWrapper>    m_gscWrapper;
};

}; /* namespace android */

#endif
