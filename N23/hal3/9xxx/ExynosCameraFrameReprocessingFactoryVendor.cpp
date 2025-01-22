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
#define LOG_TAG "ExynosCameraFrameReprocessingFactorySec"
#include <log/log.h>

#include "ExynosCameraFrameReprocessingFactory.h"

namespace android {

ExynosCameraFrameSP_sptr_t ExynosCameraFrameReprocessingFactory::createNewFrame(uint32_t frameCount, bool useJpegFlag)
{
    status_t ret = NO_ERROR;
    uint32_t frametype = FRAME_TYPE_REPROCESSING;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    if (frameCount <= 0) {
        frameCount = m_frameCount;
    }

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        frametype = FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION;
#endif

    ExynosCameraFrameSP_sptr_t frame =  m_frameMgr->createFrame(m_configurations, frameCount, frametype);
    int requestEntityCount = 0;
    int pipeId = -1;
    int parentPipeId = PIPE_3AA_REPROCESSING;

    if (frame == NULL) {
        CLOGE("frame is NULL");
        return NULL;
    }

    m_setBaseInfoToFrame(frame);

    ret = m_initFrameMetadata(frame);
    if (ret != NO_ERROR)
        CLOGE("frame(%d) metadata initialize fail", m_frameCount);

    /* PLUGIN_PRE1 */
    pipeId = PIPE_PLUGIN_PRE1_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
        ////////////////////////////////////////////////
        // FRAME_MODE_MF_STILL means.... it is multi frame mode.
        if (1 < m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT)) {
            frame->setMode(FRAME_MODE_MF_STILL, true);
        }

        ////////////////////////////////////////////////

        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_request[INDEX(PIPE_NFD_REPROCESSING)] == true) {
        pipeId = PIPE_NFD_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    if (m_request[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)] == true) {
        pipeId = PIPE_PLUGIN_LEC_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

#if defined(USES_HIFI)
if (m_configurations->getMode(CONFIGURATION_HIFI_MODE)) {
#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
        int flipHorizontal = 0;
        int flipVertical = 0;

#ifdef SUPPORT_PERFRAME_FLIP
        flipHorizontal  = m_configurations->getModeValue(CONFIGURATION_PERFRAME_FLIP_H_PICTURE);
        flipVertical  = m_configurations->getModeValue(CONFIGURATION_PERFRAME_FLIP_V_PICTURE);
#else
        flipHorizontal  = m_configurations->getModeValue(CONFIGURATION_FLIP_HORIZONTAL);
        flipVertical    = m_configurations->getModeValue(CONFIGURATION_FLIP_VERTICAL);
#endif

        pipeId = PIPE_SW_MCSC_REPEOCESSING;
        if (m_request[INDEX(pipeId)] == true) {
            frame->setMode(FRAME_MODE_SWMCSC, true);

            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            frame->setFlipHorizontal(pipeId, flipHorizontal);
            frame->setFlipVertical(pipeId, flipVertical);
            requestEntityCount++;

            for(int i = PIPE_MCSC0_REPROCESSING ; i < PIPE_MCSC5_REPROCESSING ; i++) {
                if (m_request[INDEX(i)] == true) {
                    pipeId = i;
                    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
                    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
                    requestEntityCount++;
                }
            }
        }
#endif
    } else
#endif
    {
        /* set 3AA pipe to linkageList */
        if (m_supportPureBayerReprocessing == true) {
            pipeId = PIPE_3AA_REPROCESSING;
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            parentPipeId = pipeId;
        }

        /* set ISP pipe to linkageList */
        if (m_supportPureBayerReprocessing == false
            || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
            pipeId = PIPE_ISP_REPROCESSING;

            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            if (m_supportPureBayerReprocessing == true && m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
                requestEntityCount++;
            }
            parentPipeId = pipeId;
        }

        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;

        /* set MCSC pipe to linkageList */
        if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
            pipeId = PIPE_MCSC_REPROCESSING;
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            requestEntityCount++;
        }

#ifdef USE_VRA_FD
        /* set VRA pipe to linkageList */
        if (((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)
            && m_request[INDEX(PIPE_MCSC5_REPROCESSING)] == true && m_request[INDEX(PIPE_VRA_REPROCESSING)] == true) ||
            ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
            m_request[INDEX(PIPE_3AF_REPROCESSING)] == true && m_request[INDEX(PIPE_VRA_REPROCESSING)] == true)) {
            pipeId = PIPE_VRA_REPROCESSING;
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            requestEntityCount++;
        }
#endif

#ifdef USE_CLAHE_REPROCESSING
        /* set CLAHE pipe to linkageList */
        if ((m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) && m_request[INDEX(PIPE_CLAHEC_REPROCESSING)] == true) {
            pipeId = PIPE_CLAHE_REPROCESSING;
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            requestEntityCount++;
        }
#endif
    }

#ifdef USE_DUAL_CAMERA
    /* set Sync pipe to linkageList */
    pipeId = PIPE_SYNC_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

    /* PLUGIN_POST1 */
    pipeId = PIPE_PLUGIN_POST1_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
#ifdef SUPPORT_VENDOR_YUV_STALL
        if (m_configurations->getMode(CONFIGURATION_VENDOR_YUV_STALL)) {
            frame->setMode(FRAME_MODE_VENDOR_YUV_STALL, true);
            frame->setModeValue(FRAME_MODE_VENDOR_YUV_STALL_PORT, m_configurations->getModeValue(CONFIGURATION_VENDOR_YUV_STALL_PORT));
        }
#endif
        ////////////////////////////////////////////////
        // FRAME_MODE_MF_STILL means.... it is multi frame mode.
        if (m_configurations->getMode(CONFIGURATION_HIFI_LLS_MODE)) {
           frame->setMode(FRAME_MODE_MF_STILL, true);
        }

        if (1 < m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT)) {
            frame->setMode(FRAME_MODE_MF_STILL, true);
        }

        ////////////////////////////////////////////////

        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
    if (m_parameters->needGSCForCapture(m_cameraId) == true) {
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    /* set JPEG pipe to linkageList */
    pipeId = PIPE_JPEG_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
    if (useJpegFlag == true && isMcscRequest()) {
        if ((m_flagHWFCEnabled == true && m_configurations->getModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE) == YUV_STALL_USAGE_PICTURE)
            || (m_flagHWFCEnabled == false)) {
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            requestEntityCount++;

            for (int pipeId = PIPE_JPEG0_REPROCESSING ; pipeId <= MAX_PIPE_NUM_JPEG_DST_REPROCESSING ; pipeId++) {
                newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
                frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            }
        }
    }

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING2;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING3;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    ret = m_initPipelines(frame);
    if (ret != NO_ERROR) {
        CLOGE("m_initPipelines fail, ret(%d)", ret);
    }

    frame->setNumRequestPipe(requestEntityCount);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;

    return frame;
}

bool  ExynosCameraFrameReprocessingFactory::isMcscRequest()
{
    bool ret = false;
    for(int i = PIPE_MCSC0_REPROCESSING; i < PIPE_MCSC5_REPROCESSING ; i++) {
        if (m_request[INDEX(i)]) {
            ret = true;
            break;
        }
    }

#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    if (m_request[INDEX(PIPE_MCSC_PP_REPROCESSING)]) {
        ret = true;
    }
#endif

    return ret;
}

status_t ExynosCameraFrameReprocessingFactory::m_setupConfig(void)
{
    CLOGI("");

    int pipeId = -1;

    int node3Paf = -1;
    int node3aa = -1, node3ac = -1, node3ap = -1, node3af = -1, node3ag = -1;
    int nodeIsp = -1, nodeIspc = -1, nodeIspp = -1;
    int nodeMcsc = -1, nodeMcscp0 = -1, nodeMcscp1 = -1;
    int nodeMcscp2 = -1, nodeMcscp3 = -1, nodeMcscp4 = -1;
    int nodeMcscpDs = -1;
    int nodeVra = -1;
    int nodeClh = -1, nodeClhc = -1;
    int previousPipeId = -1;
    int mcscSrcPipeId = -1;
    int vraSrcPipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    bool flagStreamLeader = false;
    bool supportJpeg = false;
#ifdef USE_PAF
    enum pipeline pipeId3Paf = PIPE_PAF_REPROCESSING;
#endif
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    int virtualNodeMCSC[MCSC_PORT_MAX] = {-1};
    int virtualNodeDS = -1;
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    int virtualNodeNv21 = -1;
#endif
#endif

    m_flagFlite3aaOTF = m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA);
#ifdef USE_PAF
    m_flagPaf3aaOTF = m_parameters->getHwConnectionMode(PIPE_PAF_REPROCESSING, PIPE_3AA_REPROCESSING);
#endif

    m_flag3aaIspOTF = m_parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_ISP_REPROCESSING);
    m_flagIspMcscOTF = m_parameters->getHwConnectionMode(PIPE_ISP_REPROCESSING, PIPE_MCSC_REPROCESSING);
    m_flag3aaVraOTF = m_parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_VRA_REPROCESSING);
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_NONE)
        m_flagMcscVraOTF = m_parameters->getHwConnectionMode(PIPE_MCSC_REPROCESSING, PIPE_VRA_REPROCESSING);

#ifdef USE_CLAHE_REPROCESSING
    m_flagMcscClaheOTF = m_parameters->getHwConnectionMode(PIPE_MCSC_REPROCESSING, PIPE_CLAHE_REPROCESSING);
#endif

    m_supportReprocessing = m_parameters->isReprocessing();
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING) {
        m_supportPureBayerReprocessing = m_parameters->getUsePureBayerRemosaicReprocessing();
    } else {
        m_supportPureBayerReprocessing = m_parameters->getUsePureBayerReprocessing();
    }

    m_request[INDEX(PIPE_3AP_REPROCESSING)] = (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M);
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        m_request[INDEX(PIPE_ISPC_REPROCESSING)] = true;
    }

    /* jpeg can be supported in this factory depending on mcsc output port */
    if (m_parameters->getNumOfMcscOutputPorts() > 3) {
        m_request[INDEX(PIPE_MCSC_JPEG_REPROCESSING)] = true;
        m_request[INDEX(PIPE_MCSC_THUMB_REPROCESSING)] = true;
        if (m_flagHWFCEnabled == true) {
            m_request[INDEX(PIPE_HWFC_JPEG_SRC_REPROCESSING)] = true;
            m_request[INDEX(PIPE_HWFC_JPEG_DST_REPROCESSING)] = true;
            m_request[INDEX(PIPE_HWFC_THUMB_SRC_REPROCESSING)] = true;
        }
    }

    switch (m_parameters->getHwChainType()) {
    case HW_CHAIN_TYPE_DUAL_CHAIN:
        node3aa = FIMC_IS_VIDEO_31S_NUM;
        node3ac = FIMC_IS_VIDEO_31C_NUM;
        node3ap = FIMC_IS_VIDEO_31P_NUM;
        node3af = FIMC_IS_VIDEO_31F_NUM;
        nodeIsp = FIMC_IS_VIDEO_I1S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I1C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I1P_NUM;
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
        break;
    case HW_CHAIN_TYPE_SEMI_DUAL_CHAIN:

#if defined(FRONT_CAMERA_3AA_NUM)
        if (isFrontCamera(getCameraId())) {
            if (FRONT_CAMERA_3AA_NUM != FIMC_IS_VIDEO_30S_NUM) {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif
                node3aa = FIMC_IS_VIDEO_30S_NUM;
                node3ac = FIMC_IS_VIDEO_30C_NUM;
                node3ap = FIMC_IS_VIDEO_30P_NUM;
                node3af = FIMC_IS_VIDEO_31F_NUM;
            } else {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif

                node3aa = FIMC_IS_VIDEO_31S_NUM;
                node3ac = FIMC_IS_VIDEO_31C_NUM;
                node3ap = FIMC_IS_VIDEO_31P_NUM;
                node3af = FIMC_IS_VIDEO_31F_NUM;
            }
        } else
#endif
        {

            if ((m_configurations->getMode(CONFIGURATION_PIP_MODE)
#ifdef USE_DUAL_CAMERA
                || m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
#endif
                )
                    && (getCameraId() == CAMERA_ID_BACK_2
                    || getCameraId() == CAMERA_ID_BACK_3 || getCameraId() == CAMERA_ID_FRONT_2)) {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif

                node3aa = FIMC_IS_VIDEO_30S_NUM;
                node3ac = FIMC_IS_VIDEO_30C_NUM;
                node3ap = FIMC_IS_VIDEO_30P_NUM;
                node3af = FIMC_IS_VIDEO_30F_NUM;
            } else

            {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
                node3aa = FIMC_IS_VIDEO_31S_NUM;
                node3ac = FIMC_IS_VIDEO_31C_NUM;
                node3ap = FIMC_IS_VIDEO_31P_NUM;
                node3af = FIMC_IS_VIDEO_31F_NUM;
            }
        }
        nodeIsp = FIMC_IS_VIDEO_I0S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I0C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I0P_NUM;
        break;
    default:
        CLOGE("invalid hw chain type(%d)", m_parameters->getHwChainType());
        break;
    }

#ifdef USE_3AG_CAPTURE
    //HACK: normal scenario : use dng port instead of 3AP
    //      remosaic : use dng port instead of 3AC
    if (getFactoryType() != FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING) {
        if (node3ap == FIMC_IS_VIDEO_30P_NUM ||
            node3ap == FIMC_IS_VIDEO_31P_NUM)
            node3ap = CONVERT_3AP_TO_3AG(node3ap);
    } else {
        if (node3ac == FIMC_IS_VIDEO_30C_NUM ||
            node3ac == FIMC_IS_VIDEO_31C_NUM)
            node3ac = CONVERT_3AC_TO_3AG(node3ac);
    }
#endif

    if (m_parameters->getNumOfMcscInputPorts() > 1) {
#if 0//def USE_DUAL_CAMERA // HACK??
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
#else
        nodeMcsc = FIMC_IS_VIDEO_M1S_NUM;
#endif
    } else {
#if defined(NUM_OF_M2M_MCSC_OUTPUT_PORTS) && (NUM_OF_M2M_MCSC_OUTPUT_PORTS == 1)
        nodeMcsc = FIMC_IS_VIDEO_M1S_NUM;
#else
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
#endif
    }

    nodeMcscp0 = FIMC_IS_VIDEO_M0P_NUM;
    nodeMcscp1 = FIMC_IS_VIDEO_M1P_NUM;
    nodeMcscp2 = FIMC_IS_VIDEO_M2P_NUM;
    nodeMcscp3 = FIMC_IS_VIDEO_M3P_NUM;
    nodeMcscp4 = FIMC_IS_VIDEO_M4P_NUM;
    nodeVra = FIMC_IS_VIDEO_VRA_NUM;
#ifdef USE_CLAHE_REPROCESSING
    nodeClh = FIMC_IS_VIDEO_CLH0S_NUM;
    nodeClhc = FIMC_IS_VIDEO_CLH0C_NUM;
#endif

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    virtualNodeMCSC[MCSC_PORT_0] = nodeMcscp0;
    virtualNodeMCSC[MCSC_PORT_1] = nodeMcscp1;
    virtualNodeMCSC[MCSC_PORT_2] = nodeMcscp2;
    virtualNodeMCSC[MCSC_PORT_3] = nodeMcscp3;
    virtualNodeMCSC[MCSC_PORT_4] = nodeMcscp4;
    virtualNodeDS = FIMC_IS_VIDEO_M5P_NUM;
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    virtualNodeNv21 = FIMC_IS_VIRTUAL_VIDEO_PP;
#endif
#endif

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        nodeMcscpDs = FIMC_IS_VIDEO_M5P_NUM;
        break;
    case 3:
        nodeMcscp3 = FIMC_IS_VIDEO_M1P_NUM;
        nodeMcscp4 = FIMC_IS_VIDEO_M2P_NUM;
        nodeMcscpDs = FIMC_IS_VIDEO_M3P_NUM;
        break;
    case 1:
        nodeMcscp0 = FIMC_IS_VIDEO_M0P_NUM;
        break;
    default:
        CLOGE("invalid output port(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    m_initDeviceInfo(INDEX(PIPE_3AA_REPROCESSING));
    m_initDeviceInfo(INDEX(PIPE_ISP_REPROCESSING));
    m_initDeviceInfo(INDEX(PIPE_MCSC_REPROCESSING));
#ifdef USE_VRA_FD
    m_initDeviceInfo(INDEX(PIPE_VRA_REPROCESSING));
#endif
#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    m_initDeviceInfo(INDEX(PIPE_SW_MCSC_REPEOCESSING));
#endif
#ifdef USE_CLAHE_REPROCESSING
    m_initDeviceInfo(INDEX(PIPE_CLAHE_REPROCESSING));
#endif
    m_initDeviceInfo(INDEX(PIPE_JPEG_REPROCESSING));

    /*
     * 3AA for Reprocessing
     */
    pipeId = INDEX(PIPE_3AA_REPROCESSING);
    previousPipeId = PIPE_FLITE;

    if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {

        /*
         * If dirty bayer is used for reprocessing, the ISP video node is leader in the reprocessing stream.
         */
        if (m_supportPureBayerReprocessing == false) {
            flagStreamLeader = false;
        } else {
            flagStreamLeader = true;
        }

#ifdef USE_PAF
        /* 3PAF */
        nodeType = getNodeType(pipeId3Paf);
        m_deviceInfo[pipeId].pipeId[nodeType]  = pipeId3Paf;
        m_deviceInfo[pipeId].nodeNum[nodeType] = node3Paf;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3PAF_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3PAF_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum), false, flagStreamLeader, m_flagReprocessing);
        previousPipeId = INDEX(pipeId3Paf);
#endif
    }

    /*
     * If dirty bayer is used for reprocessing, the ISP video node is leader in the reprocessing stream.
     */
    if (m_supportPureBayerReprocessing == false
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
        flagStreamLeader = false;
    } else {
        flagStreamLeader = true;
    }

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_3AA_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3aa;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {
#ifdef USE_PAF
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(pipeId3Paf)], m_flagPaf3aaOTF, flagStreamLeader, m_flagReprocessing);
#endif
    } else {
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum), false, flagStreamLeader, m_flagReprocessing);
    }

    // Restore leader flag
    flagStreamLeader = false;

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ac;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_CAPTURE_OPT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_CAPTURE_OPT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ap;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    if (m_parameters->isUse3aaDNG()) {
        /* 3AG */
        node3ag = FIMC_IS_VIDEO_31G_NUM;
        nodeType = getNodeType(PIPE_3AG_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AG_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = node3ag;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_DNG", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_DNG", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AG_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
    }

#ifdef SUPPORT_3AF
 /* 3AF */
    nodeType = getNodeType(PIPE_3AF_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AF_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3af;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_FD", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_FD", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
#endif

#ifdef USE_VRA_FD
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) {
        int vraPipeId = INDEX(PIPE_VRA_REPROCESSING);

        /*
         * VRA
         */
        previousPipeId = pipeId;

        vraSrcPipeId = INDEX(PIPE_3AF_REPROCESSING);
        nodeType = getNodeType(PIPE_VRA_REPROCESSING);
        m_deviceInfo[vraPipeId].pipeId[nodeType]  = PIPE_VRA;
        m_deviceInfo[vraPipeId].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[vraPipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[vraPipeId].nodeName[nodeType], "REMOSAIC_VRA_OUTPUT_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[vraPipeId].nodeName[nodeType], "VRA_OUTPUT_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[vraPipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flag3aaVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

    /*
     * ISP for Reprocessing
     */
    previousPipeId = pipeId;
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = INDEX(PIPE_ISP_REPROCESSING);
    }

    if (m_supportPureBayerReprocessing == false) {
        /* Set leader flag on ISP pipe for Dirty bayer reprocessing */
        flagStreamLeader = true;
    }

    /* ISPS */
    nodeType = getNodeType(PIPE_ISP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISP_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIsp;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_3AP_REPROCESSING)], m_flag3aaIspOTF, flagStreamLeader, m_flagReprocessing);

    // Restore leader flag
    flagStreamLeader = false;

    /*
     * MCSC for Reprocessing
     */
    previousPipeId = pipeId;
    mcscSrcPipeId = PIPE_ISP_REPROCESSING;

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspc;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_CAPTURE_M2M", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_CAPTURE_M2M", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPP_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspp;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_CAPTURE_OTF", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_CAPTURE_OTF", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        pipeId = INDEX(PIPE_MCSC_REPROCESSING);
        mcscSrcPipeId = PIPE_ISPC_REPROCESSING;
    }


    /* MCSC */
    nodeType = getNodeType(PIPE_MCSC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcsc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(mcscSrcPipeId)], m_flagIspMcscOTF, flagStreamLeader, m_flagReprocessing);

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC0_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_0];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC1 */
    nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC1_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_1];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC2 */
    nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC2_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_2];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC3 */
    nodeType = getNodeType(PIPE_MCSC_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_JPEG_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_3];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC4 */
    nodeType = getNodeType(PIPE_MCSC_THUMB_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_THUMB_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_THUMB_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_4];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    /* MCSC NV21 */
    nodeType = getNodeType(PIPE_MCSC_PP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_PP_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_PP_REPROCESSING)] = virtualNodeNv21;
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_PP", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_PP", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
#endif

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC2_REPROCESSING)] = nodeMcscp2;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC1_REPROCESSING)] = nodeMcscp1;
        /* Not break; */
    case 3:
        supportJpeg = true;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_THUMB_REPROCESSING)] = nodeMcscp4;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)] = nodeMcscp3;
        /* Not break; */
    case 1:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC0_REPROCESSING)] = nodeMcscp0;
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if (supportJpeg == true && m_flagHWFCEnabled == true) {
        /* JPEG Src */
        nodeType = getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_JPEG_SRC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_JPEG_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_JPEG_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_JPEG_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Src */
        nodeType = getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_SRC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_THUMBNAIL_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_THUMBNAIL_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* JPEG Dst */
        nodeType = getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_JPEG_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_JPEG_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_JPEG_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_JPEG_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Dst  */
        nodeType = getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_THUMBNAIL_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_THUMBNAIL_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
    }

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        /* MCSC5 */
        nodeType = getNodeType(PIPE_MCSC5_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC5_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC5_REPROCESSING)] = virtualNodeDS;
#endif
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscpDs;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_MCSC_DS_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_DS_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /*
         * VRA
         */
        previousPipeId = pipeId;
        vraSrcPipeId = PIPE_MCSC5_REPROCESSING;
        pipeId = INDEX(PIPE_VRA_REPROCESSING);

        nodeType = getNodeType(PIPE_VRA_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VRA_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        else
            strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flagMcscVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    pipeId = INDEX(PIPE_SW_MCSC_REPEOCESSING);

    nodeType = getNodeType(PIPE_SW_MCSC_REPEOCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_SW_MCSC_REPEOCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_SW_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_YUV0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REPROCESSING_SW_MCSC_YUV0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_YUV1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REPROCESSING_SW_MCSC_YUV1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_YUV2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REPROCESSING_SW_MCSC_YUV2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC3_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC3_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REPROCESSING_SW_MCSC_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC4_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC4_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_REPROCESSING_SW_MCSC_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REPROCESSING_SW_MCSC_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
#endif

#ifdef USE_CLAHE_REPROCESSING
    /*
     * CLAHE for Reprocessing
     */
    previousPipeId = INDEX(PIPE_ISP_REPROCESSING);
    pipeId = INDEX(PIPE_CLAHE_REPROCESSING);

    nodeType = getNodeType(PIPE_CLAHE_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_CLAHE_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeClh;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_CLAHE_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_CLAHE_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)], m_flagMcscClaheOTF, flagStreamLeader, m_flagReprocessing);

    nodeType = getNodeType(PIPE_CLAHEC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_CLAHEC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeClhc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_CLAHE_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_CLAHE_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_CLAHE_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
#endif

    /* JPEG for Reprocessing */
    pipeId = INDEX(PIPE_JPEG_REPROCESSING);

    nodeType = getNodeType(PIPE_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "JPEG_REPROCESSING_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG0_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_CAPTURE0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "JPEG_REPROCESSING_CAPTURE0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG1_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_CAPTURE1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    else
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "JPEG_REPROCESSING_CAPTURE1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    /* GSC for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC2 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING2)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC3 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING3)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::m_constructPipes(void)
{
    CLOGI("");

    int pipeId = -1;

    /* 3AA for Reprocessing */
    pipeId = PIPE_3AA_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_3AA_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_3AA_REPROCESSING");

    /* ISP for Reprocessing */
    pipeId = PIPE_ISP_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_ISP_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_ISP_REPROCESSING");

    /* MCSC for Reprocessing */
    pipeId = PIPE_MCSC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_MCSC_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_MCSC_REPROCESSING");

#ifdef USE_CLAHE_REPROCESSING
    /* CLAHE for Reprocessing */
    pipeId = PIPE_CLAHE_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_CLAHE_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_CLAHE_REPROCESSING");
#endif

#ifdef USE_VRA_FD
    /* VRA */
    pipeId = PIPE_VRA_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeVRA(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_deviceInfo[INDEX(pipeId)].nodeNum,
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_VRA_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_VRA_REPROCESSING");
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    /* NFD */
    pipeId = PIPE_NFD_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(m_cameraId,
                                                                        m_configurations,
                                                                        m_parameters,
                                                                        m_flagReprocessing,
                                                                        m_nodeNums[INDEX(pipeId)],
                                                                        m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_NFD_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_NFD_REPROCESSING");
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    /* LEC */
    pipeId = PIPE_PLUGIN_LEC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(m_cameraId,
                                                                        m_configurations,
                                                                        m_parameters,
                                                                        m_flagReprocessing,
                                                                        m_nodeNums[INDEX(pipeId)],
                                                                        m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_PLUGIN_LEC_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_PLUGIN_LEC_REPROCESSING");
#endif

    /* GSC for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_GSC_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING");

    /* GSC2 for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING2;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_GSC_REPROCESSING2");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING2");

    /* GSC3 for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING3;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_GSC_REPROCESSING3");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING3");

    if (m_flagHWFCEnabled == false || (m_flagHWFCEnabled && m_parameters->isHWFCOnDemand())) {
        /* JPEG for Reprocessing */
        pipeId = PIPE_JPEG_REPROCESSING;
        m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeMultipleJpeg(
                    m_cameraId,
                    m_configurations,
                    m_parameters,
                    m_flagReprocessing,
                    m_nodeNums[INDEX(pipeId)],
                    m_camIdInfo,
                    &m_deviceInfo[INDEX(pipeId)]);
        if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
            m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_JPEG_MULTIPLE_REPROCESSING");
        else
            m_pipes[INDEX(pipeId)]->setPipeName("PIPE_JPEG_MULTIPLE_REPROCESSING");
        m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    }

    /* LLS PlugIn */
    pipeId = PIPE_PLUGIN_PRE1_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(m_cameraId, m_configurations, m_parameters, m_flagReprocessing, m_nodeNums[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)], m_camIdInfo, PLUGIN_SCENARIO_HIFI_REPROCESSING, true);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_PLUGIN_PRE1_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_PLUGIN_PRE1_REPROCESSING");

    pipeId = PIPE_PLUGIN_POST1_REPROCESSING;
    int scenario = PLUGIN_SCENARIO_HIFI_REPROCESSING;
#ifdef USE_DUAL_CAMERA
    if (m_configurations->getFusionCapturePluginScenario() == PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
        scenario = PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING;
    }
#endif
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(m_cameraId, m_configurations, m_parameters, m_flagReprocessing, m_nodeNums[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)], m_camIdInfo, scenario, true);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_PLUGIN_POST1_REPROCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_PLUGIN_POST1_REPROCESSING");

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    pipeId = PIPE_SW_MCSC_REPEOCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeSWMCSC(m_cameraId, m_configurations, m_parameters, m_flagReprocessing, m_nodeNums[pipeId], m_camIdInfo, &m_deviceInfo[INDEX(pipeId)]);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
        m_pipes[INDEX(pipeId)]->setPipeName("REMOSAIC_PIPE_SW_MCSC_REPEOCESSING");
    else
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_SW_MCSC_REPEOCESSING");
#endif

    CLOGI("pipe ids for reprocessing");
    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            CLOGI("m_pipes[%d] : PipeId(%d) - %s" , i, m_pipes[i]->getPipeId(), m_pipes[i]->getPipeName());
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::m_initFrameMetadata(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;

    if (m_shot_ext == NULL) {
        CLOGE("new struct camera2_shot_ext fail");
        return INVALID_OPERATION;
    }

    memset(m_shot_ext, 0x0, sizeof(struct camera2_shot_ext));

    m_shot_ext->shot.magicNumber = SHOT_MAGIC_NUMBER;

    for (int i = 0; i < INTERFACE_TYPE_MAX; i++) {
        m_shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[i] = MCSC_PORT_NONE;
    }

    frame->setRequest(PIPE_3AC_REPROCESSING, m_request[INDEX(PIPE_3AC_REPROCESSING)]);
    frame->setRequest(PIPE_3AP_REPROCESSING, m_request[INDEX(PIPE_3AP_REPROCESSING)]);
    frame->setRequest(PIPE_3AF_REPROCESSING, m_request[INDEX(PIPE_3AF_REPROCESSING)]);
    frame->setRequest(PIPE_3AG_REPROCESSING, m_request[INDEX(PIPE_3AG_REPROCESSING)]);
    frame->setRequest(PIPE_ISPC_REPROCESSING, m_request[INDEX(PIPE_ISPC_REPROCESSING)]);
    frame->setRequest(PIPE_ISPP_REPROCESSING, m_request[INDEX(PIPE_ISPP_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC0_REPROCESSING, m_request[INDEX(PIPE_MCSC0_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC1_REPROCESSING, m_request[INDEX(PIPE_MCSC1_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC2_REPROCESSING, m_request[INDEX(PIPE_MCSC2_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC_JPEG_REPROCESSING, m_request[INDEX(PIPE_MCSC_JPEG_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC_THUMB_REPROCESSING, m_request[INDEX(PIPE_MCSC_THUMB_REPROCESSING)]);
    frame->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, m_request[INDEX(PIPE_HWFC_JPEG_SRC_REPROCESSING)]);
    frame->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, m_request[INDEX(PIPE_HWFC_JPEG_DST_REPROCESSING)]);
    frame->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, m_request[INDEX(PIPE_HWFC_THUMB_SRC_REPROCESSING)]);
    frame->setRequest(PIPE_MCSC5_REPROCESSING, m_request[INDEX(PIPE_MCSC5_REPROCESSING)]);
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    frame->setRequest(PIPE_MCSC_PP_REPROCESSING, m_request[INDEX(PIPE_MCSC_PP_REPROCESSING)]);
#endif
#ifdef USE_VRA_FD
    frame->setRequest(PIPE_VRA_REPROCESSING, m_request[INDEX(PIPE_VRA_REPROCESSING)]);
#endif
#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    frame->setRequest(PIPE_NFD_REPROCESSING, m_request[INDEX(PIPE_NFD_REPROCESSING)]);
#endif
#if defined(USES_CAMERA_EXYNOS_LEC)
    frame->setRequest(PIPE_PLUGIN_LEC_REPROCESSING, m_request[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]);
#endif
#ifdef USE_DUAL_CAMERA
    frame->setRequest(PIPE_SYNC_REPROCESSING, m_request[INDEX(PIPE_SYNC_REPROCESSING)]);
    frame->setRequest(PIPE_FUSION_REPROCESSING, m_request[INDEX(PIPE_FUSION_REPROCESSING)]);
#endif
#ifdef USE_SLSI_PLUGIN
    frame->setRequest(PIPE_PLUGIN_PRE1_REPROCESSING,  m_request[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)]);
    frame->setRequest(PIPE_PLUGIN_PRE2_REPROCESSING,  m_request[INDEX(PIPE_PLUGIN_PRE2_REPROCESSING)]);
    frame->setRequest(PIPE_PLUGIN_POST1_REPROCESSING, m_request[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)]);
    frame->setRequest(PIPE_PLUGIN_POST2_REPROCESSING, m_request[INDEX(PIPE_PLUGIN_POST2_REPROCESSING)]);
#endif
#ifdef USE_CLAHE_REPROCESSING
    frame->setRequest(PIPE_CLAHEC_REPROCESSING, m_request[INDEX(PIPE_CLAHEC_REPROCESSING)]);
#endif

    /* Reprocessing is not use this */
    m_TNRMode = 0;
    m_bypassFD = 1;

    setMetaTnrMode(m_shot_ext, m_TNRMode);
    setMetaBypassFd(m_shot_ext, m_bypassFD);

    ret = frame->initMetaData(m_shot_ext);
    if (ret != NO_ERROR)
        CLOGE("initMetaData fail");

    return ret;
}

void ExynosCameraFrameReprocessingFactory::extControl(int pipeId, int scenario, int controlType, __unused void *data)
{
    CLOGD("pipeId(%d), scenario(%d), controlType(%d)", pipeId, scenario, controlType);
}
}; /* namespace android */
