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
#define LOG_TAG "ExynosCameraPlugInConverterHifi"

#include "ExynosCameraPlugInConverterHifi.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterHifi::m_init(void)
{
    strncpy(m_name, "CoverterHIFI", (PLUGIN_NAME_STR_SIZE - 1));
    m_frameQeueue.release();
    memset(m_shutterspeed, 0x00, sizeof(m_shutterspeed));
    memset(m_iso, 0x00, sizeof(m_iso));
    memset(&m_bcrop, 0x00, sizeof(m_bcrop));
    memset(m_faceRect, 0x00, sizeof(m_faceRect));
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterHifi::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterHifi::m_create(Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterHifi::m_setup(Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterHifi::m_make(Map_t *map)
{
    status_t ret = NO_ERROR;

#if 1
    enum PLUGIN_CONVERT_TYPE_T type;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext shot_ext;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;
    ExynosCameraParameters *parameter = NULL;
    plugin_rect_t rect;
    ExynosCameraBufferSupplier  *bufferSupplier = NULL;

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];
    parameter = (ExynosCameraParameters *)(*map)[PLUGIN_CONVERT_PARAMETER];
    bufferSupplier = (ExynosCameraBufferSupplier *)(*map)[PLUGIN_CONVERT_BUFFERSUPPLIER];

    camera2_node_group node_group_info_bcrop;

    switch (type) {
    case PLUGIN_CONVERT_PROCESS_BEFORE:
    {
        int noiseMode = 0, edgeMode = 0;
        int noiseStrength = 0, edgeStrength = 0;

        if (frame == NULL) {
            CLOGE("frame is NULL!! type(%d), pipeId(%d)", type, m_pipeId);
            goto func_exit;
        }

        frame->getMetaData(&shot_ext);

        noiseMode = (int)shot_ext.shot.ctl.noise.mode;
        noiseStrength = (int)shot_ext.shot.ctl.noise.strength;

        edgeMode = (int)shot_ext.shot.ctl.edge.mode;
        edgeStrength = (int)shot_ext.shot.ctl.edge.strength;

        /* meta data setting */
        (*map)[PLUGIN_SRC_FRAMECOUNT]           = (Map_data_t)frame->getMetaFrameCount();
        (*map)[PLUGIN_NOISE_CONTROL_MODE]       = (Map_data_t)noiseMode;
        (*map)[PLUGIN_NOISE_CONTROL_STRENGTH]   = (Map_data_t)noiseStrength;
        (*map)[PLUGIN_EDGE_CONTROL_MODE]        = (Map_data_t)edgeMode;
        (*map)[PLUGIN_EDGE_CONTROL_STRENGTH]    = (Map_data_t)edgeStrength;
        CLOGD("[F%d D%d] HIFI control Noise mode=%d strength=%d Edge mode=%d strength=%d)", frame->getFrameCount(), frame->getMetaFrameCount(), noiseMode, noiseStrength, edgeMode, edgeStrength);
    }
        break;
    case PLUGIN_CONVERT_PROCESS_AFTER:
        break;
    case PLUGIN_CONVERT_SETUP_AFTER:
        (*map)[PLUGIN_SRC_FRAMECOUNT] = (Map_data_t)frame->getMetaFrameCount();
        break;
    default:
        CLOGE("invalid convert type(%d)!! pipeId(%d)", type, m_pipeId);
        goto func_exit;
    }

    func_exit:
#endif
    return NO_ERROR;
}
}; /* namespace android */
