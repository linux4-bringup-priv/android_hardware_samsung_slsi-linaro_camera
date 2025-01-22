/*
**
** Copyright 2016, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PIPE_MULTIPLE_JPEG_H
#define EXYNOS_CAMERA_PIPE_MULTIPLE_JPEG_H

#include "ExynosCameraSWPipe.h"
#include "ExynosCameraPipeJpeg.h"

namespace android {

class ExynosCameraPipeMultipleJpeg : public ExynosCameraSWPipe {
private:
    const static int        m_maxJpegPipeNum = MAX_PIPE_NUM_JPEG_DST_REPROCESSING - PIPE_JPEG_REPROCESSING;

public:
    ExynosCameraPipeMultipleJpeg()
    {
        m_init(NULL);
    }

    ExynosCameraPipeMultipleJpeg(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums,
        cameraId_Info *camIdInfo,
        camera_device_info_t *deviceInfo) : ExynosCameraSWPipe(cameraId, configurations, obj_param, isReprocessing, nodeNums, camIdInfo)
    {
        m_init(deviceInfo);
    }

    virtual ~ExynosCameraPipeMultipleJpeg();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);
    virtual status_t        stop(void);
    virtual status_t        setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier);

private:
    virtual status_t        m_run(void);
    bool                    m_checkThreadLoop(frame_queue_t *queue);
    void                    m_init(camera_device_info_t *deviceInfo);

    status_t                m_checkValidBuffers(ExynosCameraFrameSP_sptr_t frame,
                                        ExynosCameraBuffer (&srcBuffers)[m_maxJpegPipeNum],
                                        ExynosCameraBuffer (&dstBuffers)[m_maxJpegPipeNum]);
    virtual void            m_runJpegs(ExynosCameraFrameSP_sptr_t frame,
                                        ExynosCameraBuffer srcBuffers[m_maxJpegPipeNum],
                                        ExynosCameraBuffer dstBuffers[m_maxJpegPipeNum]);
    virtual bool            m_jpegDoneThreadFunc(void);
    virtual status_t        m_handleJpegDoneFrame(void);

private:
    /* Thread & Q for main multiple Jpeg pipe */
    sp<Thread>              m_jpegDoneThread;
    frame_queue_t           *m_handleJpegDoneQ;

    /* Pipe & Q for internal Jpeg pipe */
    ExynosCameraPipeJpeg    *m_jpegPipes[m_maxJpegPipeNum];
    frame_queue_t           *m_jpegFrameDoneQ[m_maxJpegPipeNum];

    camera_device_info_t    *m_deviceInfo;

};

}; /* namespace android */

#endif
