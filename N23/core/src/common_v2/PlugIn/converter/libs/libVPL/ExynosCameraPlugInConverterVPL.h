/*
 * copyright (C) 2017, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_VPL_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_VPL_H__

#include "ExynosCameraPlugInConverter.h"
//#include "../../../libs/libVPL/include/libvpl.h"

namespace android {

class ExynosCameraPlugInConverterVPL : public virtual ExynosCameraPlugInConverter {
public:
    ExynosCameraPlugInConverterVPL() : ExynosCameraPlugInConverter()
    {
        m_init();
    }

    ExynosCameraPlugInConverterVPL(int cameraId, int pipeId) : ExynosCameraPlugInConverter(cameraId, pipeId)
    {
        m_init();
    }

    virtual ~ExynosCameraPlugInConverterVPL() { ALOGD("%s", __FUNCTION__); };

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
    Data_int32_t m_detectedFaces;
    Data_uint32_t m_faceId[CAMERA2_MAX_FACES];
    Data_float_t m_faceRect[CAMERA2_MAX_FACES][4];
    Data_float_t m_faceScore[CAMERA2_MAX_FACES];
    Data_float_t m_faceRotation[CAMERA2_MAX_FACES];
    Data_float_t m_faceYaw[CAMERA2_MAX_FACES];
    Data_float_t m_facePitch[CAMERA2_MAX_FACES];

    status_t m_frameConfigVPLBefore(ExynosCameraFrameSP_sptr_t, Map_t *);
    status_t m_frameConfigVPLAfter(ExynosCameraFrameSP_sptr_t, Map_t *);
    // for default converting to send the plugIn
};
}; /* namespace android */
#endif

