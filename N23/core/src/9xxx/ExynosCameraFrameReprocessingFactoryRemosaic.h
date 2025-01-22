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

#ifndef EXYNOS_CAMERA_FRAME_REPROCESSING_FACTORY_REMOSAIC_H
#define EXYNOS_CAMERA_FRAME_REPROCESSING_FACTORY_REMOSAIC_H

#include "ExynosCameraFrameReprocessingFactory.h"

namespace android {

class ExynosCameraFrameReprocessingFactoryRemosaic : public ExynosCameraFrameReprocessingFactory {
public:
    ExynosCameraFrameReprocessingFactoryRemosaic()
    {
    }

    ExynosCameraFrameReprocessingFactoryRemosaic(int cameraId,
                                                ExynosCameraConfigurations *configurations,
                                                ExynosCameraParameters *param,
                                                cameraId_Info *camIdInfo)
        : ExynosCameraFrameReprocessingFactory(cameraId, configurations, param, camIdInfo)
    {

    }

    virtual ~ExynosCameraFrameReprocessingFactoryRemosaic();

    virtual status_t        initPipes(void);
    virtual status_t        stopPipes(void);

    virtual ExynosCameraFrameSP_sptr_t createNewFrame(uint32_t frameCount, bool useJpegFlag = false);

    virtual void            setRequest(int pipeId, bool enable);

protected:
    virtual status_t        m_setupConfig(void);
    virtual status_t        m_constructPipes(void);
    virtual status_t        m_fillNodeGroupInfo(ExynosCameraFrameSP_sptr_t frame);

};

}; /* namespace android */

#endif //EXYNOS_CAMERA_FRAME_REPROCESSING_FACTORY_REMOSAIC_H