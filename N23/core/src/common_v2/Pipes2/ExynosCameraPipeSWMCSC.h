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

#ifndef EXYNOS_CAMERA_PIPE_SWMCSC_H
#define EXYNOS_CAMERA_PIPE_SWMCSC_H

#include "ExynosCameraSWPipe.h"
#include "ExynosCameraPipeGSC.h"

namespace android {

class ExynosCameraPipeSWMCSC : protected virtual ExynosCameraSWPipe {
public:
    ExynosCameraPipeSWMCSC()
    {
        m_init(NULL);
    }

    ExynosCameraPipeSWMCSC(
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

    virtual ~ExynosCameraPipeSWMCSC();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);
    virtual status_t        stop(void);
    virtual status_t        stopThread(void);

protected:
    virtual status_t        m_run(void);
    virtual status_t        m_runScalerSerial(ExynosCameraFrameSP_sptr_t frame);
    virtual status_t        m_runScalerParallel(ExynosCameraFrameSP_sptr_t frame);
    virtual bool            m_gscDoneThreadFunc(void);
    virtual status_t        m_handleGscDoneFrame(void);
    virtual uint32_t        getPipeId(void);
    virtual int             getPipeId(enum NODE_TYPE nodeType);

private:
    void                    m_init(camera_device_info_t *m_deviceInfo);
    bool                    m_checkThreadLoop(frame_queue_t *queue);

private:
    sp<Thread>              m_gscDoneThread;
    frame_queue_t          *m_handleGscDoneQ;
    frame_queue_t          *m_gscFrameDoneQ[MAX_NODE];
    ExynosCameraPipe       *m_gscPipes[MAX_NODE];
    camera_device_info_t   *m_deviceInfo;
};

}; /* namespace android */

#endif
