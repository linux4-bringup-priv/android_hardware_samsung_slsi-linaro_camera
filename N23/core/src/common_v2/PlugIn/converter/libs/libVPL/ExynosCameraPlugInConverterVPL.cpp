/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraPlugInConverterVPL"

#include "ExynosCameraPlugInConverterVPL.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterVPL::m_init(void)
{
    strncpy(m_name, "ConverterVPL", (PLUGIN_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVPL::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVPL::m_create(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVPL::m_setup(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVPL::m_make(__unused Map_t *map)
{
    enum PLUGIN_CONVERT_TYPE_T type;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    status_t ret = NO_ERROR;

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];

    switch (type) {
   case PLUGIN_CONVERT_PROCESS_BEFORE:
        /* VPL specific information update */
        ret = m_frameConfigVPLBefore(frame, map);
        if (ret != NO_ERROR) {
            CLOGE("m_frameConfigVPLBefore(%d, %d, %d) fail",
                    m_pipeId, type, (frame != NULL) ? frame->getFrameCount() : -1);
            goto func_exit;
        }
        break;
    case PLUGIN_CONVERT_PROCESS_AFTER:
        ret = m_frameConfigVPLAfter(frame, map);
        if (ret != NO_ERROR) {
            CLOGE("m_frameConfigVPLAfter(%d, %d, %d) fail",
                    m_pipeId, type, (frame != NULL) ? frame->getFrameCount() : -1);
            goto func_exit;
        }
        break;
    default:
        break;
    }
func_exit:
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVPL::m_frameConfigVPLBefore(
            ExynosCameraFrameSP_sptr_t frame, Map_t *map)
{
    status_t ret = NO_ERROR;

    m_detectedFaces = 0;
    (*map)[PLUGIN_NFD_DETECTED_FACES]   = (Map_data_t)m_detectedFaces;
    (*map)[PLUGIN_NFD_FACE_ID]          = (Map_data_t)(&m_faceId);
    (*map)[PLUGIN_NFD_FACE_RECT]        = (Map_data_t)(&m_faceRect);
    (*map)[PLUGIN_NFD_FACE_SCORE]       = (Map_data_t)(&m_faceScore);
    (*map)[PLUGIN_NFD_FACE_ROTATION]    = (Map_data_t)(&m_faceRotation);
    (*map)[PLUGIN_NFD_FACE_YAW]         = (Map_data_t)(&m_faceYaw);
    (*map)[PLUGIN_NFD_FACE_PITCH]       = (Map_data_t)(&m_facePitch);

err_exit:
    return ret;

}

status_t ExynosCameraPlugInConverterVPL::m_frameConfigVPLAfter(
            ExynosCameraFrameSP_sptr_t frame, Map_t *map) {

    status_t ret = NO_ERROR;

    fd_info fdInfo = {0};
    int i = 0;

    fdInfo.frame_count = frame->getMetaFrameCount();

    fdInfo.face_num = (Data_int32_t)(*map)[PLUGIN_NFD_DETECTED_FACES];
    for (int index = 0; index < fdInfo.face_num; index++) {
        fdInfo.id[index]            =   ((Array_vpl_int32_t)(*map)[PLUGIN_NFD_FACE_ID])[index];
        fdInfo.score[index]         =   ((Array_vpl_float_t)(*map)[PLUGIN_NFD_FACE_SCORE])[index];
        fdInfo.rect[index].offset_x =   ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][0];
        fdInfo.rect[index].offset_y =   ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][1];
        fdInfo.rect[index].width    =   ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][2] - ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][0];
        fdInfo.rect[index].height   =   ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][3] - ((Array_buf_rect_t)(*map)[PLUGIN_NFD_FACE_RECT])[index][1];
        fdInfo.rotation[index]      =   ((Array_vpl_float_t)(*map)[PLUGIN_NFD_FACE_ROTATION])[index];
        fdInfo.yaw[index]           =   ((Array_vpl_float_t)(*map)[PLUGIN_NFD_FACE_YAW])[index];
        fdInfo.pitch[index]         =   ((Array_vpl_float_t)(*map)[PLUGIN_NFD_FACE_PITCH])[index];
        CLOGV("[F:%d]face_num(%d), id(%d), score(%f), rotation(%f), faceRect(%f,%f,%fx%f)",
                fdInfo.frame_count, fdInfo.face_num, fdInfo.id[index], fdInfo.score[index], fdInfo.rotation[index],
                fdInfo.rect[index].offset_x, fdInfo.rect[index].offset_y, fdInfo.rect[index].width, fdInfo.rect[index].height);
    }

    for (i = 0; i < m_srcCurBufCnt; i++) {
        fdInfo.crop_x = m_srcBufRect[i][PLUGIN_ARRAY_RECT_X];
        fdInfo.crop_y = m_srcBufRect[i][PLUGIN_ARRAY_RECT_Y];
        fdInfo.crop_w = m_srcBufRect[i][PLUGIN_ARRAY_RECT_W];
        fdInfo.crop_h = m_srcBufRect[i][PLUGIN_ARRAY_RECT_H];
        fdInfo.width  = m_srcBufRect[i][PLUGIN_ARRAY_RECT_FULL_W];
        fdInfo.height = m_srcBufRect[i][PLUGIN_ARRAY_RECT_FULL_H];
        CLOGV("[F:%d][%d]imgRect(%d,%d, %dx%d),(%dx%d)",
                fdInfo.frame_count, i, fdInfo.crop_x, fdInfo.crop_y, fdInfo.crop_w, fdInfo.crop_h, fdInfo.width, fdInfo.height);
    }

#ifdef TEST_INTERFACE
    i = 0;
    fdInfo.id[i]            = 1;
    fdInfo.score[i]         = 1000; /* 900 - 51, 1000 - 91 */
    fdInfo.rect[i].offset_x = 10;
    fdInfo.rect[i].offset_y = 10;
    fdInfo.rect[i].width    = (MAX_VRA_INPUT_WIDTH / 10);
    fdInfo.rect[i].height   = (MAX_VRA_INPUT_WIDTH / 10);
    fdInfo.rotation[i]      = 0;
    fdInfo.yaw[i]           = 0;
    fdInfo.pitch[i]         = 0;

    CLOGV("[F:%d]face_num(%d), id(%d), score(%f), rotation(%f), faceRectangles(%f,%f, %fx%f)",
            fdInfo.frame_count, fdInfo.face_num, fdInfo.id[i], fdInfo.score[i], fdInfo.rotation[i],
            fdInfo.rect[i].offset_x, fdInfo.rect[i].offset_y, fdInfo.rect[i].width, fdInfo.rect[i].height);
#endif

    ret = frame->setNFDInfo(&fdInfo);

    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to storeDynamicMeta to frame. ret %d",
                frame->getFrameCount(), ret);
    }
    return ret;
}
}; /* namespace android */

