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
#define LOG_TAG "ExynosCameraPlugInLowLightShot"
#include <log/log.h>

#include "ExynosCameraPlugInLowLightShot.h"

namespace android {

volatile int32_t ExynosCameraPlugInLowLightShot::initCount = 0;

DECLARE_CREATE_PLUGIN_SYMBOL(ExynosCameraPlugInLowLightShot);

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInLowLightShot::m_init(void)
{
    int count = android_atomic_inc(&initCount);

    CLOGD("count(%d)", count);

    if (count == 1) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInLowLightShot::m_deinit(void)
{
    int count = android_atomic_dec(&initCount);

    CLOGD("count(%d)", count);

    if (count == 0) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInLowLightShot::m_create(void)
{
    CLOGD("");

    lls = new LowLightShot();
    lls->create();
    strncpy(m_name, "LowLightShotPL", (PLUGIN_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

status_t ExynosCameraPlugInLowLightShot::m_destroy(void)
{
    CLOGD("");

    if (lls) {
        lls->destroy();
        delete lls;
        lls = NULL;
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInLowLightShot::m_setup(Map_t *map)
{
    CLOGD("");

    return lls->setup(map);
}

status_t ExynosCameraPlugInLowLightShot::m_process(Map_t *map)
{
    return lls->execute(map);
}

status_t ExynosCameraPlugInLowLightShot::m_setParameter(int key, void *data)
{
    /* do nothing */
    return NO_ERROR;
}

status_t ExynosCameraPlugInLowLightShot::m_getParameter(int key, void *data)
{
    /* do nothing */
    return NO_ERROR;
}

void ExynosCameraPlugInLowLightShot::m_dump(void)
{
    /* do nothing */
}

status_t ExynosCameraPlugInLowLightShot::m_query(Map_t *map)
{
    return lls->query(map);
}
}
