/*
 * Copyright@ Samsung Electronics Co. LTD
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


#ifndef EXYNOS_CAMERA_PLUGIN_COMBINE_PREVIEW_H
#define EXYNOS_CAMERA_PLUGIN_COMBINE_PREVIEW_H

#include "ExynosCameraPlugInCombine.h"

class FakeFusion;
class FakeSceneDetect;

namespace android {

class ExynosCameraPlugInCombinePreview : public ExynosCameraPlugInCombine {
public:
    ExynosCameraPlugInCombinePreview() : ExynosCameraPlugInCombine() {}
    ExynosCameraPlugInCombinePreview(int cameraId, int pipeId, int scenario) : ExynosCameraPlugInCombine(cameraId, pipeId, scenario) {
        strncpy(m_name, "CombinePreviewPlugIn", (PLUGIN_NAME_STR_SIZE - 1));
        m_init();
    };
    virtual ~ExynosCameraPlugInCombinePreview() { ALOGD("%s", __FUNCTION__); };

protected:
    static volatile int32_t initCount;

    // inherit this function.
    virtual status_t m_init(void);
    virtual status_t m_deinit(void);
    virtual status_t m_create(void);
    virtual status_t m_destroy(void);
    virtual status_t m_setup(Map_t *map);
    virtual status_t m_process(Map_t *map);
    virtual status_t m_setParameter(int key, void *data);
    virtual status_t m_getParameter(int key, void *data);
    virtual void     m_dump(void);
    virtual status_t m_query(Map_t *map);
    virtual status_t m_start(void);
    virtual status_t m_stop(void);


    /***********************************/
    /*  variables                      */
    /***********************************/
private:
    std::shared_ptr<FakeFusion> m_fakeFusion;
    std::shared_ptr<FakeSceneDetect> m_fakeSceneDetect;
};
}
#endif //EXYNOS_CAMERA_PLUGIN_COMBINE_PREVIEW_H
