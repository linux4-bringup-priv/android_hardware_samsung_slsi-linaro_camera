/*
**
** Copyright 2019, Samsung Electronics Co. LTD
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

#define LOG_TAG "FakeSceneDetect"

#include "FakeSceneDetect.h"

FakeSceneDetect::FakeSceneDetect()
{
    ALOGD("DEBUG(%s[%d]):new FakeSceneDetect object allocated", __FUNCTION__, __LINE__);
}

FakeSceneDetect::~FakeSceneDetect()
{
    ALOGD("DEBUG(%s[%d]):new FakeSceneDetect object deallocated", __FUNCTION__, __LINE__);
}

status_t FakeSceneDetect::create(void)
{
    ALOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t FakeSceneDetect::start(void)
{
    PLUGIN_LOGD("-IN-");

    return NO_ERROR;
}

status_t FakeSceneDetect::stop(void)
{
    PLUGIN_LOGD("-IN-");

    return NO_ERROR;
}

status_t FakeSceneDetect::init(Map_t *map)
{
    return NO_ERROR;
}

status_t FakeSceneDetect::setup(Map_t *map)
{
    return NO_ERROR;
}

status_t FakeSceneDetect::query(Map_t *map)
{
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "FakeSceneDetectLib";
        (*map)[PLUGIN_BUILD_DATE]             = (Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = (Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)1;
    }
    return NO_ERROR;
}

status_t FakeSceneDetect::destroy(void)
{
    ALOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

/* execute library */
status_t FakeSceneDetect::execute(Map_t *map)
{
    status_t ret = NO_ERROR;
    m_sceneType = -1;

    /* target buffer */
    Array_buf_addr_t sourceBuf = (Array_buf_addr_t)(*map)[PLUGIN_DST_BUF_1];
    Array_buf_t sourceBufSize = (Array_buf_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_t rectDimensions = (Array_buf_t)(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t sourceBufWStride = (Array_buf_t)(*map)[PLUGIN_DST_BUF_WSTRIDE];
    Array_buf_t sourceBufHStride = (Array_buf_t)(*map)[PLUGIN_DST_BUF_HSTRIDE];
    Array_buf_t sourceBufV4L2Format = (Array_buf_t)(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    int frameSize_width = (unsigned int)(rectDimensions[PLUGIN_ARRAY_RECT_W]);
    int frameSize_height = (unsigned int)(rectDimensions[PLUGIN_ARRAY_RECT_H]);
    int srcBufWstride = (int)(sourceBufWStride[0]);
    int srcBufHstride = (int)(sourceBufHStride[0]);
    Array_buf_plane_t mapSrcBufFd;
    mapSrcBufFd = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_FD];

    //PLUGIN_LOGD("[scene] frame size width:%d, height: %d, width stride: %d, height stride: %d", frameSize_width, frameSize_height, srcBufWstride, srcBufHstride);
    //m_sceneType = detect();
    m_sceneType = 10;
    if (m_sceneType < 0) {
        ret = UNKNOWN_ERROR;
    }
    (*map)[PLUGIN_SCENE_TYPE] = m_sceneType;

    return ret;
}
