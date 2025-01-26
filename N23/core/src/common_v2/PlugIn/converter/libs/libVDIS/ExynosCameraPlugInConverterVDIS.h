/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_VDIS_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_VDIS_H__

#include "ExynosCameraPlugInConverter.h"

namespace android {

class ExynosCameraPlugInConverterVDIS : public virtual ExynosCameraPlugInConverter {
public:
    ExynosCameraPlugInConverterVDIS() : ExynosCameraPlugInConverter()
    {
        m_init();
    }

    ExynosCameraPlugInConverterVDIS(int cameraId, int pipeId) : ExynosCameraPlugInConverter(cameraId, pipeId)
    {
        m_init();
    }

    virtual ~ExynosCameraPlugInConverterVDIS() { ALOGD("%s", __FUNCTION__); };

protected:
    // inherit this function.
    virtual status_t m_init(void);
    virtual status_t m_deinit(void);
    virtual status_t m_create(__unused Map_t *map);
    virtual status_t m_setup(Map_t *map);
    virtual status_t m_make(Map_t *map);

protected:
    // help function.

private:
    // for default converting to send the plugIn
    //float m_zoomRatio[1];
    plugin_rect_t m_bcrop;
    int32_t m_previewBufPos;
    int32_t m_recordingBufPos;
    plugin_rect_t m_previewCrop;

    int32_t m_inputSizeW;
    int32_t m_inputSizeH;
    int32_t m_previewSizeW;
    int32_t m_previewSizeH;
    int32_t m_recordingSizeW;
    int32_t m_recordingSizeH;
};
}; /* namespace android */
#endif
