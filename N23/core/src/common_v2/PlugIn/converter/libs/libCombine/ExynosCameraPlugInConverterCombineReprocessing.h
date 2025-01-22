/*
 * Copyright (C) 2019, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_REPROCESSING_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_REPROCESSING_H__

#include "ExynosCameraPlugInConverter.h"
#include "ExynosCameraBufferSupplier.h"
#include "ExynosCameraPlugInConverterCombine.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrameSP_sptr_t> frame_queue_t;

class ExynosCameraPlugInConverterCombineReprocessing : public virtual ExynosCameraPlugInConverter {
public:
    ExynosCameraPlugInConverterCombineReprocessing() : ExynosCameraPlugInConverter()
    {
        m_init();
    }

    ExynosCameraPlugInConverterCombineReprocessing(int cameraId, int pipeId) : ExynosCameraPlugInConverter(cameraId, pipeId)
    {
        m_init();
    }

    virtual ~ExynosCameraPlugInConverterCombineReprocessing() { ALOGD("%s", __FUNCTION__); };

protected:
    // inherit this function.
    virtual status_t m_init(void);
    virtual status_t m_deinit(void);
    virtual status_t m_create(Map_t *map);
    virtual status_t m_setup(Map_t *map);
    virtual status_t m_make(Map_t *map);

protected:
    // help function.
    status_t m_makeProcessBeforeCommon(Map_t *map, int scenario);
    status_t m_makeProcessBeforeNightShotBayer(Map_t *map);
    status_t m_makeProcessBeforeNightShotYuv(Map_t *map);
    status_t m_makeProcessBeforeSuperNightShotBayer(Map_t *map);
    status_t m_makeProcessAfterSuperNightShotBayer(Map_t *map);
    status_t m_makeProcessBeforeHdrBayer(Map_t *map);
    status_t m_makeProcessBeforeHdrYuv(Map_t *map);
    status_t m_makeProcessBeforeFlashMultiFrameDenoiseYuv(Map_t *map);
    status_t m_makeProcessBeforeBeautyFaceYuv(Map_t *map);
    status_t m_makeProcessBeforeSuperResolution(Map_t *map);
    status_t m_makeProcessBokehFusionCapture(Map_t *map);
    status_t m_makeProcessBeforeSWRemosaicBayer(Map_t *map);
    status_t m_makeProcessBeforeOisDenoiseYuv(Map_t *map);
    status_t m_makeProcessBeforeSportsYuv(Map_t *map);

private:
    frame_queue_t m_frameQeueue;

    // HDR related
    int           m_aeRegion[4];

    // Sports related
    float         m_gyro[4];
};
}; /* namespace android */
#endif
