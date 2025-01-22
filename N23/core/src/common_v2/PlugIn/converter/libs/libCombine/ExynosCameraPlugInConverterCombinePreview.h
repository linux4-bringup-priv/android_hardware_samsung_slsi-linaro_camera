/*
 * copyright (C) 2019, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_PREVIEW_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_PREVIEW_H__

#include "ExynosCameraPlugInConverter.h"

namespace android {

class ExynosCameraPlugInConverterCombinePreview : public virtual ExynosCameraPlugInConverter {
public:
    ExynosCameraPlugInConverterCombinePreview() : ExynosCameraPlugInConverter()
    {
        m_init();
    }

    ExynosCameraPlugInConverterCombinePreview(int cameraId, int pipeId) : ExynosCameraPlugInConverter(cameraId, pipeId)
    {
        m_init();
    }

    virtual ~ExynosCameraPlugInConverterCombinePreview() { ALOGD("%s", __FUNCTION__); };

protected:
    // inherit this function.
    virtual status_t m_init(void);
    virtual status_t m_deinit(void);
    virtual status_t m_create(Map_t *map);
    virtual status_t m_setup(Map_t *map);
    virtual status_t m_make(Map_t *map);

protected:
    // help function.

private:
    Data_int32_t m_resultSceneType;

    status_t m_frameConfigCombinePreviewBefore(ExynosCameraFrameSP_sptr_t frame, Map_t *);
    status_t m_frameConfigCombinePreviewAfter(ExynosCameraFrameSP_sptr_t frame, Map_t *);
    // for default converting to send the plugIn
};
}; /* namespace android */
#endif

