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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPlugInCombinePreview"
#include <log/log.h>

#include "ExynosCameraPlugInCombinePreview.h"

#include "FakeFusion.h"
#include "FakeSceneDetect.h"

namespace android {

volatile int32_t ExynosCameraPlugInCombinePreview::initCount = 0;

DECLARE_CREATE_PLUGIN_SYMBOL(ExynosCameraPlugInCombinePreview);

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInCombinePreview::m_init(void)
{
    int count = android_atomic_inc(&initCount);

    CLOGD("count(%d)", count);

    if (count == 1) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_deinit(void)
{
    int count = android_atomic_dec(&initCount);

    CLOGD("count(%d)", count);

    if (count == 0) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_create(void)
{
    CLOGD("");
    strncpy(m_name, "CombinePreviewPlugIn", (PLUGIN_NAME_STR_SIZE - 1));

    // internal plugIn
    m_fakeFusion = std::make_shared<FakeFusion>();
    m_fakeFusion->create();
    m_fakeSceneDetect = std::make_shared<FakeSceneDetect>();
    m_fakeSceneDetect->create();

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_destroy(void)
{
    CLOGD("");

    // internal plugIn
    m_fakeFusion->destroy();
    m_fakeSceneDetect->destroy();

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_setup(Map_t *map)
{
    CLOGD("");

    // internal plugIn
    m_fakeSceneDetect->setup(map);

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_process(Map_t *map)
{
    status_t ret = NO_ERROR;

    // internal plugIn
    m_fakeFusion->execute(map);
    m_fakeSceneDetect->execute(map);

    return ret;
}

status_t ExynosCameraPlugInCombinePreview::m_setParameter(int key, void *data)
{
    status_t ret = NO_ERROR;

    switch(key) {
    case PLUGIN_PARAMETER_KEY_START:
        ret = this->m_start();
        break;
    case PLUGIN_PARAMETER_KEY_STOP:
        ret = this->m_stop();
        break;
    default:
        CLOGW("Unknown key(%d)", key);
    }
    return ret;
}

status_t ExynosCameraPlugInCombinePreview::m_getParameter(int key, void *data)
{
    CLOGD("");
    return NO_ERROR;
}

void ExynosCameraPlugInCombinePreview::m_dump(void)
{
    CLOGD("");
    /* do nothing */
}

status_t ExynosCameraPlugInCombinePreview::m_query(Map_t *map)
{
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "CombinePreviewLib";
        (*map)[PLUGIN_BUILD_DATE]             = (Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = (Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)1;
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_start(void)
{
    CLOGD("");

    // internal plugIn
    m_fakeSceneDetect->start();

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombinePreview::m_stop(void)
{
    CLOGD("");

    // internal plugIn
    m_fakeSceneDetect->stop();

    return NO_ERROR;
}
}
