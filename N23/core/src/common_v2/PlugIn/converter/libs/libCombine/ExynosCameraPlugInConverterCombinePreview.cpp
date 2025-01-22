/*
 * Copyright (C) 2019, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraPlugInConverterCombinePreview"

#include "ExynosCameraPlugInConverterCombinePreview.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterCombinePreview::m_init(void)
{
    strncpy(m_name, "ConverterCombinePreview", (PLUGIN_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombinePreview::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombinePreview::m_create(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombinePreview::m_setup(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombinePreview::m_make(__unused Map_t *map)
{
    enum PLUGIN_CONVERT_TYPE_T type;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    status_t ret = NO_ERROR;

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];

    switch (type) {
    case PLUGIN_CONVERT_PROCESS_BEFORE:
        /* CombinePreview specific information update */
        ret = m_frameConfigCombinePreviewBefore(frame, map);
        if (ret != NO_ERROR) {
            CLOGE("m_frameConfigCombinePreviewBefore(%d, %d, %d) fail",
                    m_pipeId, type, (frame != NULL) ? frame->getFrameCount() : -1);
            goto func_exit;
        }
        break;
    case PLUGIN_CONVERT_PROCESS_AFTER:
        ret = m_frameConfigCombinePreviewAfter(frame, map);
        if (ret != NO_ERROR) {
            CLOGE("m_frameConfigCombinePreviewAfter(%d, %d, %d) fail",
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

status_t ExynosCameraPlugInConverterCombinePreview::m_frameConfigCombinePreviewBefore(
            ExynosCameraFrameSP_sptr_t frame, Map_t *map)
{
    status_t ret = NO_ERROR;
    bool isDualMode = false;
    int scenario = -1;

    ExynosCameraConfigurations *configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];
    if (configurations) {
        isDualMode = configurations->getMode(CONFIGURATION_DUAL_MODE);
        switch (configurations->getScenario()) {
        case SCENARIO_DUAL_REAR_ZOOM:
            scenario = PLUGIN_DUAL_SCENARIO_ZOOM;
            break;
        case SCENARIO_DUAL_REAR_PORTRAIT:
        case SCENARIO_DUAL_FRONT_PORTRAIT:
            scenario = PLUGIN_DUAL_SCENARIO_BOKEH;
            break;
        default:
            break;
        }
        (*map)[PLUGIN_DUAL_MODE] = (Map_data_t)(&isDualMode);
        (*map)[PLUGIN_DUAL_SCENARIO] = (Map_data_t)(&scenario);
    }

    m_resultSceneType = 0;
    // for scene detector lib result
    (*map)[PLUGIN_SCENE_TYPE] = (Map_data_t)(&m_resultSceneType);

    /* Update face info */
    const int masterNodeIndex = OUTPUT_NODE_1, slaveNodeIndex = OUTPUT_NODE_2;
    int numOfDetectedFaces = 0;
    const camera2_shot_ext *metaData[MAX_OUTPUT_NODE] = {NULL};

    metaData[masterNodeIndex] = frame->getConstMeta(masterNodeIndex);
    (*map)[PLUGIN_MASTER_FACE_RECT] = (Map_data_t)(&metaData[masterNodeIndex]->shot.dm.stats.faceRectangles);

    for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
        if (metaData[masterNodeIndex]->shot.dm.stats.faceIds[i] <= 0) {
            continue;
         }

        numOfDetectedFaces++;
        CLOGV("faceId %d, faceScores %d, Rectangles %d, %d, %d, %d",
                metaData[masterNodeIndex]->shot.dm.stats.faceIds[i],
                metaData[masterNodeIndex]->shot.dm.stats.faceScores[i],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][0],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][1],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][2],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][3]);
     }

    if (isDualMode == true && frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
        metaData[slaveNodeIndex] = frame->getConstMeta(slaveNodeIndex);
        (*map)[PLUGIN_SLAVE_FACE_RECT] = (Map_data_t)(&metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles);

        int slaveNumOfDetectedFaces = 0;
        for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
            if (metaData[slaveNodeIndex]->shot.dm.stats.faceIds[i] <= 0) {
                continue;
            }

            slaveNumOfDetectedFaces++;
            CLOGV("Slave: faceId %d, faceScores %d, Rectangles %d, %d, %d, %d",
                metaData[slaveNodeIndex]->shot.dm.stats.faceIds[i],
                metaData[slaveNodeIndex]->shot.dm.stats.faceScores[i],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][0],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][1],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][2],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][3]);
        }

        numOfDetectedFaces = MAX(numOfDetectedFaces, slaveNumOfDetectedFaces);
    }

    (*map)[PLUGIN_FACE_NUM] = (Map_data_t)numOfDetectedFaces;

    return ret;
}

status_t ExynosCameraPlugInConverterCombinePreview::m_frameConfigCombinePreviewAfter(
            ExynosCameraFrameSP_sptr_t frame, Map_t *map)
{
    status_t ret = NO_ERROR;
    m_resultSceneType = (Data_int32_t)(*map)[PLUGIN_SCENE_TYPE];
    sp<ExynosCameraVendorMetaData> vendorMeta = frame->getVendorMeta();
    if (vendorMeta != NULL) {
        vendorMeta->update(EXYNOS_ANDROID_VENDOR_SCENE_TYPE, &m_resultSceneType, 1);
        CLOGV("[R%d F%d D%d] update vendorMeta EXYNOS_ANDROID_VENDOR_SCENE_TYPE (%d)", frame->getRequestKey(), frame->getFrameCount(), frame->getMetaFrameCount(), m_resultSceneType);
    }
    return ret;
}
}; /* namespace android */

