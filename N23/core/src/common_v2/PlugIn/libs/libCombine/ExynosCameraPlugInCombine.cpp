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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPlugInCombine"
#include <log/log.h>

#include "ExynosCameraPlugInCombine.h"

namespace android {

/*********************************************/
/*  global definition                        */
/*********************************************/

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInCombine::m_init(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_create(void)
{
    PLUGIN_LOGD("");

    mUser = NULL;
    mDebugLevel = DEBUG_LEVEL_NONE;

    /* Check Debug Level */
    char prop[PROP_VALUE_MAX];
    if (property_get("persist.vendor.Combine.debug", prop, "0") > 0) {
        mDebugLevel = atoi(prop);
        PLUGIN_LOGD("[CombinePlugIn] mDebugLevel %d", mDebugLevel);
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_destroy(void)
{
    PLUGIN_LOGD("");

    if (mUser) {
        Library* lib = (Library*)mUser;

        int ret = lib->destroy();
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombinePlugIn] Plugin deinit failed!!");
        }
        mUser = NULL;
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_setup(Map_t *map)
{
    PLUGIN_LOGD("");

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_process(Map_t *map)
{
    status_t ret = NO_ERROR;

    PLUGIN_LOG_ASSERT("ASSERT(%s[%d]):Child class should make this function, assert!!!!", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCameraPlugInCombine::m_setParameter(int key, void *data)
{
    status_t ret = NO_ERROR;

    switch (key) {
    case PLUGIN_PARAMETER_KEY_PREPARE:
        break;
    case PLUGIN_PARAMETER_KEY_START:
        if (mUser == NULL) {
            PLUGIN_LOGD("[CombinePlugIn] Plugin create");
            Library* lib = new Library();

            ret = lib->create();
            if (ret != NO_ERROR) {
                PLUGIN_LOGE("[CombinePlugIn] Plugin init failed!!");
                delete lib;
                lib = NULL;
            }

            mUser = (void *)lib;
            PLUGIN_LOGD("[CombinePlugIn] Plugin create done");
        }
        break;
    case PLUGIN_PARAMETER_KEY_STOP:
        if (mUser != NULL) {
            PLUGIN_LOGD("[CombinePlugIn] Plugin destroy");
            Library* lib = (Library*)mUser;

            ret = lib->destroy();
            if (ret != NO_ERROR) {
                PLUGIN_LOGE("[CombinePlugIn] Plugin destroy failed!!");
            }
            delete lib;
            lib = NULL;

            mUser = (void *)lib;
            PLUGIN_LOGD("[CombinePlugIn] Plugin destroy done");
        }
        break;
    default:
        PLUGIN_LOGW("Unknown key(%d)", key);
    }

    return ret;
}

status_t ExynosCameraPlugInCombine::m_getParameter(int key, void *data)
{
    PLUGIN_LOGD("");
    return NO_ERROR;
}

void ExynosCameraPlugInCombine::m_dump(void)
{
    PLUGIN_LOGD("");
    /* do nothing */
}

status_t ExynosCameraPlugInCombine::m_query(Map_t *map)
{
    PLUGIN_LOGD("");
    status_t ret = NO_ERROR;
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "SAMSUNG_COMBINE_PLUG_IN";
        (*map)[PLUGIN_BUILD_DATE]             = 0;//(Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = 0;//(Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)3;
    }
    return ret;
}

status_t ExynosCameraPlugInCombine::m_start(void)
{
    PLUGIN_LOGD("");
    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_stop(void)
{
    PLUGIN_LOGD("");
    return NO_ERROR;
}

status_t ExynosCameraPlugInCombine::m_processCommon(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // buffer info
    int               mapSrcBufCnt      = (Data_int32_t     )(*map)[PLUGIN_SRC_BUF_CNT];
    Array_buf_t       mapSrcBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_PLANE_CNT];
    Array_buf_plane_t mapSrcBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_SIZE];
    Array_buf_rect_t  mapSrcRect        = (Array_buf_rect_t )(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_t       mapSrcV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapSrcBufAddr[2];
    mapSrcBufAddr[0] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];
    mapSrcBufAddr[1] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_2];
    Array_buf_plane_t mapSrcBufFd;
    mapSrcBufFd = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_FD];

    int               mapDstBufCnt      = (Data_int32_t     )(*map)[PLUGIN_DST_BUF_CNT];
    Array_buf_t       mapDstBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_PLANE_CNT];
    Array_buf_plane_t mapDstBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_rect_t  mapDstRect        = (Array_buf_rect_t )(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t       mapDstV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapDstBufAddr     = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_1];
    Array_buf_plane_t mapDstBufFd;
    mapDstBufFd = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_FD];

    ////////////////////////////////////////////////
    // log
    if (mDebugLevel >= DEBUG_LEVEL_LOG) {
        for (int i = 0; i < mapSrcBufCnt; i++) {
            PLUGIN_LOGD("[NightShot] src[%d/%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
                i,
                mapSrcBufCnt,
                mapSrcBufAddr[i][0],
                mapSrcBufPlaneCnt[i],
                mapSrcBufSize[i][0],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_X],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_Y],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_W],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_H],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_W],
                mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_H],
                mapSrcV4L2Format[i],
                mapSrcBufFd[i][0]);
        }

        for (int i = 0; i < mapDstBufCnt; i++) {
            PLUGIN_LOGD("[NightShot] dst[%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
                mapDstBufCnt,
                mapDstBufAddr[0],
                mapDstBufPlaneCnt[0],
                mapDstBufSize[0][0],
                mapDstRect[0][PLUGIN_ARRAY_RECT_X],
                mapDstRect[0][PLUGIN_ARRAY_RECT_Y],
                mapDstRect[0][PLUGIN_ARRAY_RECT_W],
                mapDstRect[0][PLUGIN_ARRAY_RECT_H],
                mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_W],
                mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_H],
                mapDstV4L2Format[0],
                mapDstBufFd[i][0]);
        }
    }

    return ret;
}

int ExynosCameraPlugInCombine::m_getFrameCount(Map_t *map)
{
    int frameCount = 0;
    frameCount = (Data_int32_t)(*map)[PLUGIN_SRC_FRAMECOUNT];

    return frameCount;
}

int ExynosCameraPlugInCombine::m_getScenario(Map_t *map)
{
    int scenario = 0;
    scenario = (Data_int32_t)(*map)[PLUGIN_SCENARIO];

    return scenario;
}

bool ExynosCameraPlugInCombine::m_getVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario)
{
    bool flagOn = false;

    ////////////////////////////////////////////////
    // get the current value.
    int scenario = 0;
    scenario = m_getScenario(map);

    ////////////////////////////////////////////////
    // check the bit is 1
    if (scenario & (1 << vendorScenario)) {
        flagOn = true;
    }

    ////////////////////////////////////////////////

    return flagOn;
}

}
