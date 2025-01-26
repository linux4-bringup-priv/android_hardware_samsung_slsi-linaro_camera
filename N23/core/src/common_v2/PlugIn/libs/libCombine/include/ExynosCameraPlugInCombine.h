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
 * \file      ExynosCameraPlugInCombine.h
 * \brief     header file for ExynosCameraPlugIn
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2019/04/26
 *
 * <b>Revision History: </b>
 * - 2019/06/04 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_PLUGIN_COMBINE_H
#define EXYNOS_CAMERA_PLUGIN_COMBINE_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/threads.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>

#include <linux/videodev2_exynos_media.h>
#include "ExynosCameraCommonDefine.h" /* just refer to CLOG */

#include "ExynosCameraPlugIn.h"
#include "PlugInCommon.h"

#include "FakeMultiFrame.h" /* external lib header */
#include "FakeFusion.h" /* external lib header */

namespace android {

/*
 * Class ExynosCameraPlugInCombine
 *
 * This is adjusted "Tempate method pattern"
 */

enum DEBUG_LEVEL {
    DEBUG_LEVEL_NONE = 0,
    DEBUG_LEVEL_LOG,
};

typedef FakeMultiFrame Library;

class ExynosCameraPlugInCombine : public ExynosCameraPlugIn {
protected:
    ////////////////////////////////////////////////
    // this should same with ExynosCameraPlugInConverterCombine.h
    enum VENDOR_SCENARIO {
        VENDOR_SCENARIO_BASE = 0,
        VENDOR_SCENARIO_NIGHT_SHOT_BAYER,
        VENDOR_SCENARIO_NIGHT_SHOT_YUV,
        VENDOR_SCENARIO_SUPER_NIGHT_SHOT_BAYER,
        VENDOR_SCENARIO_HDR_BAYER,
        VENDOR_SCENARIO_HDR_YUV,
        VENDOR_SCENARIO_FLASH_MULTI_FRAME_DENOISE_YUV,
        VENDOR_SCENARIO_BEAUTY_FACE_YUV,
        VENDOR_SCENARIO_SUPER_RESOLUTION,
        VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE,
        VENDOR_SCENARIO_SW_REMOSAIC,
        VENDOR_SCENARIO_OIS_DENOISE_YUV,
        VENDOR_SCENARIO_SPORTS_YUV,
        VENDOR_SCENARIO_MAX,
    };

public:
    ExynosCameraPlugInCombine() : ExynosCameraPlugIn() {}
    ExynosCameraPlugInCombine(int cameraId, int pipeId, int mode) : ExynosCameraPlugIn(cameraId, pipeId, mode) {
        strncpy(m_name, "CombinePlugIn", (PLUGIN_NAME_STR_SIZE - 1));
        m_init();
        mUser = NULL;
        mDebugLevel = 0;
    };
    virtual ~ExynosCameraPlugInCombine() { ALOGD("%s", __FUNCTION__); };

    /***********************************/
    /*  function                       */
    /***********************************/
protected:
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

protected:
    status_t m_processCommon(Map_t *map);

    int      m_getFrameCount(Map_t *map);

    int      m_getScenario(Map_t *map);
    bool     m_getVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario);

    /***********************************/
    /*  variables                      */
    /***********************************/
protected:
    // common
    void *mUser;
    int   mDebugLevel;
};
}
#endif //EXYNOS_CAMERA_PLUGIN_COMBINE_H
