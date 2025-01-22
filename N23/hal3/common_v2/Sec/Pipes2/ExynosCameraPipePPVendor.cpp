/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPipePPSec"

#include "ExynosCameraPipePP.h"

namespace android {

status_t ExynosCameraPipePP::create(__unused int32_t *sensorIds)
{
    status_t ret = NO_ERROR;
    char SuperClassName[EXYNOS_CAMERA_NAME_STR_SIZE] = {0,};

    CLOGD("m_nodeNum(%d)", m_nodeNum);

    m_pp = ExynosCameraPPFactory::newPP(m_cameraId, m_configurations, m_parameters, m_nodeNum);
    if (m_pp == NULL) {
        CLOGE("ExynosCameraPPFactory::newPP(m_cameraId : %d, m_nodeNum : %d) fail", m_cameraId, m_nodeNum);
        return INVALID_OPERATION;
    }

    if (m_pp->flagCreated() == false) {
        ret = m_pp->create();
        if (ret != NO_ERROR) {
            CLOGE("m_pp->create() fail");
            return INVALID_OPERATION;
        }
    }

    snprintf(SuperClassName, sizeof(SuperClassName), "%s_SWThread", m_name);

    m_mainThread = new ExynosCameraThread<ExynosCameraPipePP>(this, &ExynosCameraPipePP::m_mainThreadFunc, SuperClassName);

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    CLOGI("create() is succeed (%d)", getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipePP::m_destroy(void)
{
    status_t ret = NO_ERROR;

    if (m_pp != NULL) {
        if (m_pp->flagCreated() == true) {
            ret = m_pp->destroy();
            if (ret != NO_ERROR) {
                CLOGE("m_pp->destroy() fail");
            }
        }

        SAFE_DELETE(m_pp);
    }

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    CLOGI("destroy() is succeed (%d)", getPipeId());

    return NO_ERROR;
}

void ExynosCameraPipePP::m_init(int32_t *nodeNums, bool isPreviewFactory)
{
    if (nodeNums == NULL) {
        m_nodeNum = -1;
    } else {
        m_nodeNum = nodeNums[0];
    }

    m_pp = NULL;
}

}; /* namespace android */
