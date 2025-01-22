/*
 * Copyright @ 2019, Samsung Electronics Co. LTD
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

/*!
 * \file      ExynosCameraPlugInCombineReprocessing.h
 * \brief     header file for ExynosCameraPlugIn
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2019/04/26
 *
 * <b>Revision History: </b>
 * - 2019/06/04 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_PLUGIN_COMBINE_REPROCESSING_H
#define EXYNOS_CAMERA_PLUGIN_COMBINE_REPROCESSING_H

#include "ExynosCameraPlugInCombine.h"

namespace android {

/*
 * Class ExynosCameraPlugInCombineReprocessing
 *
 * This is adjusted "Tempate method pattern"
 */

#define NUM_OF_HDR_MERGE_FRAME (6) // hack

class ExynosCameraPlugInCombineReprocessing : public ExynosCameraPlugInCombine {
public:
    ExynosCameraPlugInCombineReprocessing() : ExynosCameraPlugInCombine() {}
    ExynosCameraPlugInCombineReprocessing(int cameraId, int pipeId, int mode) : ExynosCameraPlugInCombine(cameraId, pipeId, mode) {
        strncpy(m_name, "CombineReprocessingPlugIn", (PLUGIN_NAME_STR_SIZE - 1));
        m_init();
    };
    virtual ~ExynosCameraPlugInCombineReprocessing() { ALOGD("%s", __FUNCTION__); };

    /***********************************/
    /*  function                       */
    /***********************************/
private:
    status_t m_librariesCreate(Map_t *map);
    status_t m_librariesDestory(Map_t *map);
    status_t m_librariesExecute(Map_t *map);

protected:
    // inherit this function.
    virtual status_t m_init(void);
    virtual status_t m_deinit(void);
    virtual status_t m_destroy(void);
    virtual status_t m_process(Map_t *map);

protected:
    status_t m_processNightShotBayer(Map_t *map);
    status_t m_processNightShotYuv(Map_t *map);
    status_t m_processSuperNightShotBayer(Map_t *map);
    status_t m_processHdrBayer(Map_t *map);
    status_t m_processHdrYuv(Map_t *map);
    status_t m_processFlashMultiFrameDenoiseYuv(Map_t *map);
    status_t m_processBeautyFaceYuv(Map_t *map);
    status_t m_processSuperResolution(Map_t *map);
    status_t m_processOisDenoiseYuv(Map_t *map);
    status_t m_processSportsYuv(Map_t *map);

    int      m_getFrameMaxIndex(Map_t *map);
    int      m_getFrameCurIndex(Map_t *map);
    status_t m_setAnalogGain(int frameCurIndex, unsigned int analogGain);
    status_t m_setShutterTime(int frameCurIndex, int64_t shutterTime);

    /***********************************/
    /*  variables                      */
    /***********************************/
protected:
    // Common
    static volatile int32_t m_initCount;

    // NightShot

    // Hdr
    unsigned int m_analogGain[NUM_OF_HDR_MERGE_FRAME];
    int64_t      m_shutterTime[NUM_OF_HDR_MERGE_FRAME];

    // beautyShot

private:
    FakeFusion *m_fusion;
};
}
#endif //EXYNOS_CAMERA_PLUGIN_COMBINE_REPROCESSING_H
