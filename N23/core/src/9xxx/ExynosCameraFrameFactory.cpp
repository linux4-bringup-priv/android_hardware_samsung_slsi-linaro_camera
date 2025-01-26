/*
**
** Copyright 2017, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraFrameFactory"
#include <log/log.h>

#include "ExynosCameraFrameFactory.h"

namespace android {

enum NODE_TYPE ExynosCameraFrameFactory::getNodeType(uint32_t pipeId)
{
    enum NODE_TYPE nodeType = INVALID_NODE;

    switch (pipeId) {
    case PIPE_FLITE:
    case PIPE_FLITE_REPROCESSING:
        nodeType = OUTPUT_NODE;
        break;
    case PIPE_VC0:
    case PIPE_VC0_REPROCESSING:
        nodeType = CAPTURE_NODE_1;
        break;
#if defined(SUPPORT_DEPTH_MAP) || defined(SUPPORT_PD_IMAGE)
    case PIPE_VC1:
        nodeType = CAPTURE_NODE_2;
        break;
#endif
    case PIPE_VC3:
        nodeType = CAPTURE_NODE_12;
        break;
    case PIPE_3AA:
        if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = OUTPUT_NODE;
        } else {
            nodeType = OTF_NODE_7;
        }
#ifdef USE_PAF
        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = OUTPUT_NODE;
        } else {
            nodeType = OTF_NODE_7;
        }
#endif
        break;

#ifdef USE_PAF
    case PIPE_PAF_REPROCESSING:
        nodeType = OUTPUT_NODE;
        break;

    case PIPE_PAF:
        // Currently, m_flagFlite3aaOTF is used for the Flite_2_PAF.
        if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = OUTPUT_NODE;
        } else {
            nodeType = OTF_NODE_4;
        }
        break;
#endif

    case PIPE_3AA_REPROCESSING:
        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
            nodeType = OTF_NODE_5;
        } else {
            nodeType = OUTPUT_NODE;
        }
        break;
    case PIPE_3AC:
    case PIPE_3AC_REPROCESSING:
        nodeType = CAPTURE_NODE_3;
        break;
    case PIPE_3AP:
    case PIPE_3AP_REPROCESSING:
        if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = CAPTURE_NODE_4;
        } else {
            nodeType = OTF_NODE_1;
        }
        break;
    case PIPE_3AF:
    case PIPE_3AF_REPROCESSING:
        nodeType = CAPTURE_NODE_24;
        break;

    case PIPE_3AG:
    case PIPE_3AG_REPROCESSING:
        nodeType = CAPTURE_NODE_25;
        break;

    case PIPE_ISP:
    case PIPE_ISP_REPROCESSING:
        if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = OUTPUT_NODE;
        } else {
            nodeType = OTF_NODE_2;
        }
        break;
    case PIPE_ISPC:
    case PIPE_ISPC_REPROCESSING:
        nodeType = CAPTURE_NODE_5;
        break;
    case PIPE_ISPP:
    case PIPE_ISPP_REPROCESSING:
        if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = CAPTURE_NODE_6;
        } else {
            nodeType = OTF_NODE_3;
        }
        break;
    case PIPE_JPEG0_REPROCESSING:
    case PIPE_HWFC_JPEG_DST_REPROCESSING:
        nodeType = CAPTURE_NODE_7;
        break;
    case PIPE_HWFC_JPEG_SRC_REPROCESSING:
        nodeType = CAPTURE_NODE_8;
        break;
    case PIPE_HWFC_THUMB_SRC_REPROCESSING:
        nodeType = CAPTURE_NODE_9;
        break;
    case PIPE_JPEG1_REPROCESSING:
    case PIPE_HWFC_THUMB_DST_REPROCESSING:
        nodeType = CAPTURE_NODE_10;
        break;
    case PIPE_MCSC:
    case PIPE_MCSC_REPROCESSING:
        if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
            nodeType = OUTPUT_NODE;
        } else {
            nodeType = OTF_NODE_6;
        }
        break;
    case PIPE_MCSC0:
    case PIPE_MCSC0_REPROCESSING:
    case PIPE_FUSION0_REPROCESSING:
        nodeType = CAPTURE_NODE_18;
        break;
    case PIPE_MCSC1:
    case PIPE_MCSC1_REPROCESSING:
    case PIPE_FUSION1_REPROCESSING:
        nodeType = CAPTURE_NODE_19;
        break;
    case PIPE_MCSC2:
    case PIPE_MCSC2_REPROCESSING:
    case PIPE_FUSION2_REPROCESSING:
        nodeType = CAPTURE_NODE_20;
        break;
    case PIPE_MCSC3:
    case PIPE_MCSC3_REPROCESSING:
        nodeType = CAPTURE_NODE_21;
        break;
    case PIPE_MCSC4:
    case PIPE_MCSC4_REPROCESSING:
        nodeType = CAPTURE_NODE_22;
        break;
    case PIPE_MCSC5:
    case PIPE_MCSC5_REPROCESSING:
        nodeType = CAPTURE_NODE_23;
        break;
    case PIPE_MCSC_PP_REPROCESSING:
        nodeType = CAPTURE_NODE_24;
        break;
#ifdef USE_CLAHE_PREVIEW
    case PIPE_CLAHEC:
        nodeType = CAPTURE_NODE_26;
        break;
#endif
#ifdef USE_CLAHE_REPROCESSING
    case PIPE_CLAHEC_REPROCESSING:
        nodeType = CAPTURE_NODE_26;
        break;
#endif
#if (defined(USE_SW_MCSC) && (USE_SW_MCSC == true))
    case PIPE_SW_MCSC:
        nodeType = OUTPUT_NODE;
        break;
#endif
#if (defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true))
    case PIPE_SW_MCSC_REPEOCESSING:
        nodeType = OUTPUT_NODE;
        break;
#endif
#ifdef USES_SW_VDIS
    case PIPE_VDIS_PREVIEW:
#ifdef USE_DUAL_CAMERA
        nodeType = OUTPUT_NODE_2;
#else
        nodeType = MAX_OUTPUT_NODE;
#endif
        break;
    case PIPE_VDIS:
#endif
    case PIPE_VRA:
    case PIPE_VRA_REPROCESSING:
    case PIPE_GDC:
    case PIPE_GSC_REPROCESSING3:
    case PIPE_GSC_REPROCESSING2:
    case PIPE_JPEG_REPROCESSING:
    case PIPE_JPEG:
#ifdef USE_DUAL_CAMERA
    case PIPE_FUSION:
    case PIPE_FUSION_CLONE:
    case PIPE_FUSION_REPROCESSING:
#endif
    case PIPE_GSC:
    case PIPE_GSC_CLONE:
#ifdef USE_CLAHE_PREVIEW
    case PIPE_CLAHE:
#endif
#ifdef USE_CLAHE_REPROCESSING
    case PIPE_CLAHE_REPROCESSING:
#endif
    case PIPE_PLUGIN_BASE ... PIPE_PLUGIN_MAX:
    case PIPE_PLUGIN_CALLBACK:
    case PIPE_PLUGIN_RECORDING:
        nodeType = OUTPUT_NODE;
        break;
    case PIPE_PLUGIN_PREVIEW:
        nodeType = OUTPUT_NODE_2;
        break;
    case PIPE_ME:
        nodeType = CAPTURE_NODE_11;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Unexpected pipe_id(%d), assert!!!!",
             __FUNCTION__, __LINE__, pipeId);
        break;
    }

    return nodeType;
}

void ExynosCameraFrameFactory::extControl(int pipeId, int scenario, int controlType, __unused void *data)
{
    android_printAssert(NULL, LOG_TAG,
            "ASSERT(%s[%d]):Function is NOT implemented! pipeId %d scenario %d controlType %d",
            __FUNCTION__, __LINE__, pipeId, scenario, controlType);
}

void ExynosCameraFrameFactory::connectPPScenario(int pipeId, int scenario)
{
    CLOGD("pipeId(%d), scenario(%d)", pipeId, scenario);


    bool plugInPipe = false;
    plugInPipe = checkPlugin(pipeId);

    if (plugInPipe) {
        ExynosCameraPipePlugIn *pipe;
        pipe = (ExynosCameraPipePlugIn *)(m_pipes[INDEX(pipeId)]);
        if (pipe) {
            pipe->setParameter(PLUGIN_PARAMETER_KEY_PREPARE, nullptr, scenario);
        }
    }
}

void ExynosCameraFrameFactory::startPPScenario(int pipeId, int scenario)
{
    CLOGD("pipeId(%d)", pipeId);

    bool plugInPipe = false;
    plugInPipe = checkPlugin(pipeId);

    if (plugInPipe) {
        ExynosCameraPipePlugIn *pipe;
        pipe = (ExynosCameraPipePlugIn *)(m_pipes[INDEX(pipeId)]);
        if (pipe) {
            pipe->setParameter(PLUGIN_PARAMETER_KEY_START, nullptr, scenario);
        }
    }
}

void ExynosCameraFrameFactory::stopPPScenario(int pipeId, int scenario, bool suspendFlag)
{
    CLOGD("pipeId(%d), suspendFlag(%d)", pipeId, suspendFlag);

    bool plugInPipe = false;
    plugInPipe = checkPlugin(pipeId);

    if (plugInPipe) {
        ExynosCameraPipePlugIn *pipe;
        pipe = (ExynosCameraPipePlugIn *)(m_pipes[INDEX(pipeId)]);
        if (pipe) {
            pipe->setParameter(PLUGIN_PARAMETER_KEY_STOP, nullptr, scenario);
        }
    }
}

int ExynosCameraFrameFactory::getPPScenario(int pipeId)
{
    int scenario = 0;

    bool plugInPipe = false;
    plugInPipe = checkPlugin(pipeId);

    if (plugInPipe) {
        ExynosCameraPipePlugIn *pipe;
        pipe = (ExynosCameraPipePlugIn *)(m_pipes[INDEX(pipeId)]);
        if (pipe) {
            pipe->setParameter(PLUGIN_PARAMETER_KEY_GET_SCENARIO, (void*)&scenario);
        }
    }

    return scenario;
}

bool ExynosCameraFrameFactory::checkPlugin(int pipeId)
{
    bool ret = false;

    int startIndex = 0;
    int maxIndex = 0;
    if(m_flagReprocessing == false) {
        startIndex = PIPE_PLUGIN_BASE;
        maxIndex = PIPE_PLUGIN_MAX;
    } else {
        startIndex = PIPE_PLUGIN_BASE_REPROCESSING;
        maxIndex = PIPE_PLUGIN_MAX_REPROCESSING;
    }

    for (int i = startIndex ; i <= maxIndex ; i++) {
        if (pipeId == i) {
            ret = true;
            break;
        }
    }

    return ret;
}

status_t ExynosCameraFrameFactory::m_initFlitePipe(int sensorW, int sensorH, __unused uint32_t frameRate)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];

    int pipeId = PIPE_FLITE;

    if (m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) != HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_3AA;
    }

    ExynosRect tempRect;

#ifdef DEBUG_RAWDUMP
    int bayerFormat = m_parameters->getBayerFormat(PIPE_FLITE);
    if (m_configurations->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    CLOGI("SensorSize(%dx%d)", sensorW, sensorH);

    /* set BNS ratio */
    int bnsScaleRatio = 1000;
    ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_S_BNS, bnsScaleRatio);
    if (ret != NO_ERROR) {
        CLOGE("Set BNS(%.1f) fail, ret(%d)", (float)(bnsScaleRatio / 1000), ret);
    } else {
        int bnsSize = 0;

        ret = m_pipes[pipeId]->getControl(V4L2_CID_IS_G_BNS_SIZE, &bnsSize);
        if (ret != NO_ERROR) {
            CLOGE("Get BNS size fail, ret(%d)", ret);
        } else {
            int bnsWidth = bnsSize >> 16;
            int bnsHeight = bnsSize & 0xffff;
            CLOGI("BNS scale down ratio(%.1f), size (%dx%d)",
                     (float)(bnsScaleRatio / 1000), bnsWidth, bnsHeight);

            m_parameters->setSize(HW_INFO_HW_BNS_SIZE, bnsWidth, bnsHeight);
        }
    }

#ifdef USE_DUAL_CAMERA
    switch (m_configurations->getScenario()) {
    case SCENARIO_DUAL_REAR_ZOOM:
    case SCENARIO_DUAL_REAR_PORTRAIT:
    case SCENARIO_DUAL_FRONT_PORTRAIT:
    {
        int mainCameraId = m_camIdInfo->cameraId[MAIN_CAM];
        CLOGI("HW_SYNC_CAMERA(%d)", mainCameraId);
        ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_HW_SYNC_CAMERA, mainCameraId);
        if (ret != NO_ERROR) {
            CLOGE("HW_SYNC_CAMERA(%d) fail, ret(%d)", mainCameraId, ret);
        }
        break;
    }
    default:
        break;
    }
#endif

    return NO_ERROR;
}

int ExynosCameraFrameFactory::m_getSensorId(unsigned int nodeNum, unsigned int connectionMode,
                                             bool flagLeader, bool reprocessing, int sensorScenario)
{
    /* sub 100, and make index */
    nodeNum -= FIMC_IS_VIDEO_BAS_NUM;

    unsigned int reprocessingBit = 0;
    unsigned int leaderBit = 0;
    unsigned int sensorPosition = 0;

    if (reprocessing == true)
        reprocessingBit = 1;

    sensorPosition = m_cameraId;

    if (flagLeader == true)
        leaderBit = 1;

    if (sensorScenario < 0 || SENSOR_SCENARIO_MAX <= sensorScenario) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid sensorScenario(%d). assert!!!!",
            __FUNCTION__, __LINE__, sensorScenario);
    }

    return ((sensorScenario     << INPUT_SENSOR_SHIFT)   & INPUT_SENSOR_MASK) |
           ((reprocessingBit    << INPUT_STREAM_SHIFT)   & INPUT_STREAM_MASK) |
           ((sensorPosition     << INPUT_POSITION_SHIFT) & INPUT_POSITION_MASK) |
           ((nodeNum            << INPUT_VINDEX_SHIFT)   & INPUT_VINDEX_MASK) |
           ((connectionMode     << INPUT_MEMORY_SHIFT)   & INPUT_MEMORY_MASK) |
           ((leaderBit          << INPUT_LEADER_SHIFT)   & INPUT_LEADER_MASK);
}

void ExynosCameraFrameFactory::m_init(void)
{
}

status_t ExynosCameraFrameFactory::setParameter(uint32_t pipeId, int key, void *data)
{
    status_t ret = NO_ERROR;

    ret = m_pipes[INDEX(pipeId)]->setParameter(key, data);
    if (ret != NO_ERROR) {
        CLOGE("setParameter fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }

    return ret;
}

status_t ExynosCameraFrameFactory::getParameter(uint32_t pipeId, int key, void *data)
{
    status_t ret = NO_ERROR;

    ret = m_pipes[INDEX(pipeId)]->getParameter(key, data);
    if (ret != NO_ERROR) {
        CLOGE("getParameter fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }

    return ret;
}

status_t ExynosCameraFrameFactory::setParameter(uint32_t pipeId, int key, void *data, int scenario)
{
    status_t ret = NO_ERROR;

    ret = m_pipes[INDEX(pipeId)]->setParameter(key, data, scenario);
    if (ret != NO_ERROR) {
        CLOGE("setParameter fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }

    return ret;
}

status_t ExynosCameraFrameFactory::getParameter(uint32_t pipeId, int key, void *data, int scenario)
{
    status_t ret = NO_ERROR;

    ret = m_pipes[INDEX(pipeId)]->getParameter(key, data, scenario);
    if (ret != NO_ERROR) {
        CLOGE("getParameter fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }

    return ret;
}


}; /* namespace android */
