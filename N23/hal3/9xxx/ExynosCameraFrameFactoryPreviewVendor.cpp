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
#define LOG_TAG "ExynosCameraFrameFactoryPreviewSec"
#include <log/log.h>

#include "ExynosCameraFrameFactoryPreview.h"

namespace android {

ExynosCameraFrameSP_sptr_t ExynosCameraFrameFactoryPreview::createNewFrame(uint32_t frameCount, __unused bool useJpegFlag)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    ExynosCameraFrameSP_sptr_t frame = NULL;

    int requestEntityCount = 0;
    int pipeId = -1;
    int parentPipeId = -1;

    if (frameCount <= 0) {
        frameCount = m_frameCount;
    }

    frame = m_frameMgr->createFrame(m_configurations, frameCount, FRAME_TYPE_PREVIEW);

    if (frame == NULL) {
        CLOGE("frame is NULL");
        return NULL;
    }

    m_setBaseInfoToFrame(frame);

    ret = m_initFrameMetadata(frame);
    if (ret != NO_ERROR)
        CLOGE("frame(%d) metadata initialize fail", m_frameCount);

    if (m_flagPipeXNeedToRun(PIPE_FLITE) == true) {
        /* set flite pipe to linkageList */
        pipeId = PIPE_FLITE;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        requestEntityCount++;
    }

    /* set 3AA pipe to linkageList */
    pipeId = PIPE_3AA;
    newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[pipeId]);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
    requestEntityCount++;
    parentPipeId = pipeId;

#ifdef USE_DUAL_CAMERA
    /* set Sync pipe to linkageList */
    pipeId = PIPE_BAYER_SYNC;
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_request[pipeId] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }
#endif

#ifdef SUPPORT_GMV
    if (m_request[PIPE_GMV] == true) {
        pipeId = PIPE_GMV;
        newEntity[PIPE_GMV] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        requestEntityCount++;
    }
#endif

    /* set ISP pipe to linkageList */
    pipeId = PIPE_ISP;
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M
        && m_request[pipeId] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
        parentPipeId = pipeId;
    }

    /* set MCSC pipe to linkageList */
    pipeId = PIPE_MCSC;
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M
        && m_request[pipeId] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

#ifdef USE_CLAHE_PREVIEW
    /* set CLAHE pipe to linkageList */
    pipeId = PIPE_CLAHE;
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M
            && m_request[PIPE_CLAHEC] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }
#endif

#ifdef USE_DUAL_CAMERA
    /* set Sync pipe to linkageList */
    pipeId = PIPE_SYNC;
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_request[pipeId] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    /* set Fusion pipe to linkageList */
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
        m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
        pipeId = PIPE_FUSION;
        if (m_request[pipeId] == true) {
            newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[pipeId]);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            requestEntityCount++;
        }
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
        int flipHorizontal = 0;
        int flipVertical = 0;

#ifdef SUPPORT_PERFRAME_FLIP
        flipHorizontal = m_configurations->getModeValue(CONFIGURATION_PERFRAME_FLIP_H_PICTURE);
        flipVertical = m_configurations->getModeValue(CONFIGURATION_PERFRAME_FLIP_V_PICTURE);
#else
        flipHorizontal = m_configurations->getModeValue(CONFIGURATION_FLIP_HORIZONTAL);
        flipVertical = m_configurations->getModeValue(CONFIGURATION_FLIP_VERTICAL);
#endif

        pipeId = PIPE_SW_MCSC;
        if (m_request[pipeId] == true) {
            frame->setMode(FRAME_MODE_SWMCSC, true);

            newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[pipeId]);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            frame->setFlipHorizontal(pipeId, flipHorizontal);
            frame->setFlipVertical(pipeId, flipVertical);
            requestEntityCount++;
            for(int i = PIPE_MCSC0 ; i < PIPE_MCSC5; i++) {
                if (m_configurations->getSecondPortId() == i - PIPE_MCSC0) {
                    pipeId = i;
                    newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
                    frame->addSiblingEntity(NULL, newEntity[pipeId]);
                    requestEntityCount++;
                }
            }
        }
#endif

    /* set GDC pipe to linkageList */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_GDC) == true
        && m_request[PIPE_GDC] == true) {
        pipeId = PIPE_GDC;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_request[i] == false) continue;
        if (m_pipes[i] == NULL) {
            CLOGW("there's no pipe[%d]", i);
            continue;
        }

        pipeId = i;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }
#endif

#ifdef USE_VRA_FD
    /* set VRA pipe to linkageList */
    if (((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)
        && m_request[PIPE_MCSC5] == true && m_request[PIPE_VRA] == true) ||
        ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
        m_request[PIPE_3AF] == true && m_request[PIPE_VRA] == true)) {
        pipeId = PIPE_VRA;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        requestEntityCount++;

#ifdef SUPPORT_HFD
        if (m_request[PIPE_HFD] == true) {
            pipeId = PIPE_HFD;
            newEntity[PIPE_HFD] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[pipeId]);
            requestEntityCount++;
        }
#endif
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    if (m_request[PIPE_NFD] == true) {
        pipeId = PIPE_NFD;
        newEntity[PIPE_NFD] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        requestEntityCount++;
    }
#endif

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC;
    newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[pipeId]);

    /* set GSC for Video pipe to linkageList */
    pipeId = PIPE_GSC_VIDEO;
    newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[pipeId]);

    if (m_supportReprocessing == false) {
        /* set GSC for Capture pipe to linkageList */
        pipeId = PIPE_GSC_PICTURE;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);

        /* set JPEG pipe to linkageList */
        pipeId = PIPE_JPEG;
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
    }

#ifdef USES_SW_VDIS
    pipeId = PIPE_VDIS;

    if (m_request[pipeId] == true) {
        newEntity[pipeId] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[pipeId]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }
#endif

    ret = m_initPipelines(frame);
    if (ret != NO_ERROR) {
        CLOGE("m_initPipelines fail, ret(%d)", ret);
    }

    /* TODO: make it dynamic */
    frame->setNumRequestPipe(requestEntityCount);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;

    return frame;
}

status_t ExynosCameraFrameFactoryPreview::m_setupConfig()
{
    CLOGI("");

    status_t ret = NO_ERROR;

    m_flagFlite3aaOTF = m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA);
    m_flag3aaIspOTF = m_parameters->getHwConnectionMode(PIPE_3AA, PIPE_ISP);
    m_flagIspMcscOTF = m_parameters->getHwConnectionMode(PIPE_ISP, PIPE_MCSC);
    m_flag3aaVraOTF = m_parameters->getHwConnectionMode(PIPE_3AA, PIPE_VRA);
#ifdef USE_PAF
    m_flagPaf3aaOTF = m_parameters->getHwConnectionMode(PIPE_PAF, PIPE_3AA);
#endif
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_NONE)
        m_flagMcscVraOTF = m_parameters->getHwConnectionMode(PIPE_MCSC, PIPE_VRA);
#ifdef USE_CLAHE_PREVIEW
    m_flagMcscClaheOTF = m_parameters->getHwConnectionMode(PIPE_MCSC, PIPE_CLAHE);
#endif

    m_supportReprocessing = m_parameters->isReprocessing();
    m_supportPureBayerReprocessing = m_parameters->getUsePureBayerReprocessing();
    m_useBDSOff = m_parameters->isUse3aaBDSOff();
    CLOGD("m_useBDSOff: %d m_flag3aaVraOTF: %d", m_useBDSOff, m_flag3aaVraOTF);

    m_request[PIPE_3AP] = (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M);
    m_request[PIPE_ISPC] = (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M);
    m_request[PIPE_3AF] = false;
    m_request[PIPE_3AG] = false;

    /* FLITE ~ MCSC */
    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("m_setDeviceInfo() fail, ret(%d)", ret);
        return ret;
    }

#if defined(SUPPORT_HW_GDC)
    /* GDC */
    m_nodeNums[INDEX(PIPE_GDC)][OUTPUT_NODE] = FIMC_IS_VIDEO_GDC_NUM;
#endif

#ifdef SUPPORT_HFD
    /* HFD */
    m_nodeNums[INDEX(PIPE_HFD)][OUTPUT_NODE] = -1;
#endif
#ifdef SUPPORT_GMV
    /* GMV */
    m_nodeNums[INDEX(PIPE_GMV)][OUTPUT_NODE] = -1;
#endif

    /* GSC */
    m_nodeNums[PIPE_GSC][OUTPUT_NODE] = PREVIEW_GSC_NODE_NUM;

    /* GSC for Recording */
    m_nodeNums[PIPE_GSC_VIDEO][OUTPUT_NODE] = VIDEO_GSC_NODE_NUM;

    /* GSC for Capture */
    m_nodeNums[PIPE_GSC_PICTURE][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* JPEG */
    m_nodeNums[PIPE_JPEG][OUTPUT_NODE] = -1;

#ifdef USES_SW_VDIS
    m_nodeNums[INDEX(PIPE_VDIS)][OUTPUT_NODE] = 0;
    m_nodeNums[INDEX(PIPE_VDIS)][CAPTURE_NODE_1] = -1;
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_constructPipes()
{
    CLOGI("");

    int pipeId = -1;

    /* FLITE */
    pipeId = PIPE_FLITE;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_FLITE");

    /* 3AA */
    pipeId = PIPE_3AA;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_3AA");

#ifdef SUPPORT_GMV
    if (m_parameters->getGmvMode() == true) {
        pipeId = PIPE_GMV;
        m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeGMV(
                    m_cameraId,
                    m_configurations,
                    m_parameters,
                    m_flagReprocessing,
                    m_nodeNums[pipeId],
                    m_camIdInfo);
        m_pipes[pipeId]->setPipeId(pipeId);
        m_pipes[pipeId]->setPipeName("PIPE_GMV");
    }
#endif

    /* ISP */
    pipeId = PIPE_ISP;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_ISP");

    /* MCSC */
    pipeId = PIPE_MCSC;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_MCSC");

#ifdef USE_CLAHE_PREVIEW
    /* CLAHE */
    pipeId = PIPE_CLAHE;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
            m_cameraId,
            m_configurations,
            m_parameters,
            m_flagReprocessing,
            &m_deviceInfo[pipeId],
            m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_CLAHE");
#endif

#if defined(SUPPORT_HW_GDC)
    /* GDC */
    pipeId = PIPE_GDC;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipePP(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeName("PIPE_GDC");
    m_pipes[pipeId]->setPipeId(PIPE_GDC);
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    /* NFD */
    pipeId = PIPE_NFD;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo,
                pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_NFD");
    m_pipes[pipeId]->setPipeId(PIPE_NFD);
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    pipeId = PIPE_SW_MCSC;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeSWMCSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo,
                &m_deviceInfo[pipeId]);
    m_pipes[pipeId]->setPipeName("PIPE_SW_MCSC");
    m_pipes[pipeId]->setPipeId(PIPE_SW_MCSC);
#endif


#ifdef USE_VRA_FD
    /* VRA */
    pipeId = PIPE_VRA;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeVRA(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_deviceInfo[pipeId].nodeNum,
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_VRA");

    if (m_flag3aaVraOTF != HW_CONNECTION_MODE_NONE) {
        m_pipes[pipeId]->setPipeStreamLeader(true);
        m_pipes[pipeId]->setPrevPipeID(PIPE_3AA);
        m_pipes[pipeId]->setUseLatestFrame(true);
    }
#endif

#ifdef SUPPORT_HFD
    /* HFD */
    if (m_parameters->getHfdMode() == true) {
        pipeId = PIPE_HFD;
        m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeHFD(
                    m_cameraId,
                    m_configurations,
                    m_parameters,
                    m_flagReprocessing,
                    m_nodeNums[pipeId],
                    m_camIdInfo);
        m_pipes[pipeId]->setPipeId(pipeId);
        m_pipes[pipeId]->setPipeName("PIPE_HFD");
    }
#endif

    /* GSC */
    pipeId = PIPE_GSC;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_GSC");

    /* GSC for Recording */
    pipeId = PIPE_GSC_VIDEO;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_GSC_VIDEO");

    /* GSC for Capture */
    pipeId = PIPE_GSC_PICTURE;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters, m_flagReprocessing, m_nodeNums[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_GSC_PICTURE");

#ifdef USE_VIDEO_HQISP
    /* JPEG */
    pipeId = PIPE_JPEG;
    m_pipes[pipeId] = (ExynosCameraPipe*)new ExynosCameraPipeJpeg(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[pipeId],
                m_camIdInfo);
    m_pipes[pipeId]->setPipeId(pipeId);
    m_pipes[pipeId]->setPipeName("PIPE_JPEG");
#endif

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        std::string name;
        name.append("PIPE_PLUGIN");
        name.append(std::to_string(i - PIPE_PLUGIN_BASE + 1));
        m_pipes[i] = (ExynosCameraPipe*)new ExynosCameraPipePlugIn(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[i],
                m_camIdInfo,
                0, /* Not specific scenario */
                true /* SupportMultiLib */);
        m_pipes[i]->setPipeId(i);
        m_pipes[i]->setPipeName(name.c_str());
    }
#endif
#ifdef USES_SW_VDIS
    /* VDIS */
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_SW_VDIS)
#ifdef USE_DUAL_CAMERA
        && (m_camIdInfo->cameraId[MAIN_CAM] == m_cameraId)
#endif
        ) {
        ExynosCameraPipePlugIn *vdisPipe = new ExynosCameraPipePlugIn(m_cameraId, m_configurations, m_parameters, m_flagReprocessing, m_nodeNums[INDEX(PIPE_VDIS)], m_camIdInfo, PLUGIN_SCENARIO_SWVDIS_PREVIEW);
        m_pipes[INDEX(PIPE_VDIS)] = (ExynosCameraPipe*)vdisPipe;
        m_pipes[INDEX(PIPE_VDIS)]->setPipeId(PIPE_VDIS);
        m_pipes[INDEX(PIPE_VDIS)]->setPipeName("PIPE_VDIS");
    } else {
        CLOGD("SW VDIS is disabled");
        m_pipes[INDEX(PIPE_VDIS)] = NULL;
    }
#endif

    CLOGI("pipe ids for preview");
    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            CLOGI("m_pipes[%d] : PipeId(%d) - %s" , i, m_pipes[i]->getPipeId(), m_pipes[i]->getPipeName());
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_initFrameMetadata(ExynosCameraFrameSP_sptr_t frame)
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

    frame->setRequest(PIPE_VC0, m_request[PIPE_VC0]);
    frame->setRequest(PIPE_VC1, m_request[PIPE_VC1]);
    frame->setRequest(PIPE_VC3, m_request[PIPE_VC3]);
    frame->setRequest(PIPE_3AC, m_request[PIPE_3AC]);
    frame->setRequest(PIPE_3AP, m_request[PIPE_3AP]);
    frame->setRequest(PIPE_3AF, m_request[PIPE_3AF]);
    frame->setRequest(PIPE_3AG, m_request[PIPE_3AG]);
    frame->setRequest(PIPE_ISPC, m_request[PIPE_ISPC]);
    frame->setRequest(PIPE_ISPP, m_request[PIPE_ISPP]);
    frame->setRequest(PIPE_ME,   m_request[PIPE_ME]);
    frame->setRequest(PIPE_MCSC0, m_request[PIPE_MCSC0]);
    frame->setRequest(PIPE_MCSC1, m_request[PIPE_MCSC1]);
    frame->setRequest(PIPE_MCSC2, m_request[PIPE_MCSC2]);
    frame->setRequest(PIPE_MCSC_JPEG, m_request[PIPE_MCSC_JPEG]);
    frame->setRequest(PIPE_MCSC_THUMB, m_request[PIPE_MCSC_THUMB]);
    frame->setRequest(PIPE_GSC, m_request[PIPE_GSC]);
    frame->setRequest(PIPE_GDC, m_request[PIPE_GDC]);
    frame->setRequest(PIPE_MCSC5, m_request[PIPE_MCSC5]);
#ifdef USE_CLAHE_PREVIEW
    frame->setRequest(PIPE_CLAHEC, m_request[PIPE_CLAHEC]);
#endif
#if defined (USE_SW_MCSC) && (USE_SW_MCSC == true)
    frame->setRequest(PIPE_SW_MCSC, m_request[PIPE_SW_MCSC]);
#endif
#ifdef USE_VRA_FD
    frame->setRequest(PIPE_VRA, m_request[PIPE_VRA]);
#endif
    frame->setRequest(PIPE_HFD, m_request[PIPE_HFD]);
    frame->setRequest(PIPE_GMV, m_request[PIPE_GMV]);
#ifdef USES_CAMERA_EXYNOS_VPL
    frame->setRequest(PIPE_NFD, m_request[PIPE_NFD]);
#endif
#ifdef USE_DUAL_CAMERA
    frame->setRequest(PIPE_BAYER_SYNC, m_request[PIPE_BAYER_SYNC]);
    frame->setRequest(PIPE_SYNC, m_request[PIPE_SYNC]);
    frame->setRequest(PIPE_FUSION, m_request[PIPE_FUSION]);
#endif
#ifdef USE_SLSI_PLUGIN
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        frame->setRequest(i, m_request[INDEX(i)]);
    }
#endif
#ifdef USES_SW_VDIS
    frame->setRequest(PIPE_VDIS, m_request[PIPE_VDIS]);
#endif

    setMetaTnrMode(m_shot_ext, m_TNRMode);
    setMetaBypassFd(m_shot_ext, m_bypassFD);

    ret = frame->initMetaData(m_shot_ext);
    if (ret != NO_ERROR)
        CLOGE("initMetaData fail");

    return ret;
}

void ExynosCameraFrameFactoryPreview::extControl(int pipeId, int scenario, int controlType, __unused void *data)
{
    CLOGD("pipeId(%d), scenario(%d), controlType(%d)", pipeId, scenario, controlType);
}

status_t ExynosCameraFrameFactoryPreview::startPipes(void)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_pipes[i] == NULL) continue;

        ret = m_pipes[i]->start();
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN%d start fail, ret(%d)", i, ret);
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USES_SW_VDIS
    if (m_configurations->getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE) > 0) {
        if (m_pipes[INDEX(PIPE_VDIS)] != NULL) {
            ret = m_pipes[INDEX(PIPE_VDIS)]->start();
            if (ret < 0) {
                CLOGE("PIPE_VDIS start fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    if (m_pipes[INDEX(PIPE_NFD)] != NULL) {
        ret = m_pipes[INDEX(PIPE_NFD)]->start();
        if (ret < 0) {
            CLOGE("PIPE_NFD start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif
#ifdef SUPPORT_HFD
    if (m_parameters->getHfdMode() == true) {
        ret = m_pipes[PIPE_HFD]->start();
        if (ret != NO_ERROR) {
            CLOGE("HFD start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
        if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
            if (m_pipes[PIPE_FUSION] != NULL) {
                ret = m_pipes[PIPE_FUSION]->start();
                if (ret < 0) {
                    CLOGE("PIPE_FUSION start fail, ret(%d)", ret);
                    /* TODO: exception handling */
                    return INVALID_OPERATION;
                }
            }
        }

        if (m_pipes[PIPE_SYNC] != NULL) {
            ret = m_pipes[PIPE_SYNC]->start();
            if (ret < 0) {
                CLOGE("PIPE_SYNC start fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

#if defined (USE_SW_MCSC) && (USE_SW_MCSC == true)
    if (m_configurations->getSecondPortId() > -1) {
        if (m_pipes[PIPE_SW_MCSC] != NULL) {
            ret = m_pipes[PIPE_SW_MCSC]->start();
            if (ret < 0) {
                CLOGE("PIPE_SW_MCSC start fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

#ifdef USE_VRA_FD
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) ||
        (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = m_pipes[PIPE_VRA]->start();
        if (ret != NO_ERROR) {
            CLOGE("VRA start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_CLAHE]->start();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_MCSC]->start();
        if (ret != NO_ERROR) {
            CLOGE("MCSC start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_ISP]->start();
        if (ret != NO_ERROR) {
            CLOGE("ISP start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

#ifdef SUPPORT_GMV
    if (m_parameters->getGmvMode() == true) {
        ret = m_pipes[PIPE_GMV]->start();
        if (ret != NO_ERROR) {
            CLOGE("GMV start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_pipes[PIPE_BAYER_SYNC] != NULL) {
        ret = m_pipes[PIPE_BAYER_SYNC]->start();
        if (ret < 0) {
            CLOGE("PIPE_BAYER_SYNC start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

    ret = m_pipes[PIPE_3AA]->start();
    if (ret != NO_ERROR) {
        CLOGE("3AA start fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    int pipeId = PIPE_3AA;
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
        ret = m_pipes[PIPE_FLITE]->start();
        if (ret != NO_ERROR) {
            CLOGE("FLITE start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_needSensorStreamOn == true) {
        ret = m_pipes[pipeId]->prepare();
        if (ret != NO_ERROR) {
            CLOGE("pipeId %d prepare fail, ret(%d)", pipeId, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        ret = m_pipes[pipeId]->sensorStream(true);
        if (ret != NO_ERROR) {
            CLOGE("pipeId %d sensorStream on fail, ret(%d)", pipeId, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* change state to streamOn(standby off) */
        m_sensorStandby = false;
    } else {
        CLOGI("skip the Sensor Stream");
    }

    ret = m_transitState(FRAME_FACTORY_STATE_RUN);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState. ret %d", ret);
        return ret;
    }

    int32_t bigMinLock, littleMinLock;
    bool lockFlag = m_configurations->getMinLockFreq(bigMinLock, littleMinLock);
    if (lockFlag) {
        CLOGI("CPU lock(%d) big(0x%x) little(0x%x)", lockFlag, bigMinLock, littleMinLock);
        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER1, bigMinLock);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER1 setControl fail, ret(%d)", ret);
        }

        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER0, littleMinLock);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER0 setControl fail, ret(%d)", ret);
        }
    }

    CLOGI("Starting Success!");

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::stopPipes(void)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;
    uint32_t sensorPipeId = (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) ? PIPE_FLITE : PIPE_3AA;

    if (m_pipes[PIPE_3AA]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_3AA]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("3AA stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_pipes[PIPE_BAYER_SYNC] != NULL
        && m_pipes[PIPE_BAYER_SYNC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_BAYER_SYNC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("BAYER_SYNC stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef SUPPORT_GMV
    if (m_pipes[PIPE_GMV]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_GMV]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_GMV stopThread fail, ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

    if (m_pipes[PIPE_ISP] != NULL && m_pipes[PIPE_ISP]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_ISP]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("ISP stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_pipes[PIPE_MCSC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_MCSC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("MCSC stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_CLAHE_PREVIEW
    if (m_pipes[PIPE_CLAHE]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_CLAHE]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

    if (m_flagPipeXNeedToRun(PIPE_FLITE) == true) {
        ret = m_pipes[PIPE_FLITE]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("FLITE stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#if defined(SUPPORT_HW_GDC)
    if (m_pipes[PIPE_GDC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_GDC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("GDC stopThread fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    if (m_pipes[PIPE_NFD]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_NFD]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("NFD stopThread fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    if (m_pipes[PIPE_SW_MCSC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_SW_MCSC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("SW_MCSC stopThread fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#ifdef USE_VRA_FD
    if (m_pipes[PIPE_VRA] != NULL && m_pipes[PIPE_VRA]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_VRA]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_VRA stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#ifdef SUPPORT_HFD
    if (m_parameters->getHfdMode() == true
        && m_pipes[PIPE_HFD]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_HFD]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_HFD stopThread fail, ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_pipes[i] == NULL) continue;
        if (m_pipes[i]->isThreadRunning() == false) continue;

        ret = m_pipes[i]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_PLUGIN%d stopThread fail, ret(%d)", i, ret);
            funcRet |= ret;
        }
    }
#endif

    if (m_pipes[PIPE_SYNC] != NULL
        && m_pipes[PIPE_SYNC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_SYNC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("SYNC stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
        m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
        if (m_pipes[PIPE_FUSION] != NULL
            && m_pipes[PIPE_FUSION]->isThreadRunning() == true) {
            ret = stopThread(PIPE_FUSION);
            if (ret != NO_ERROR) {
                CLOGE("PIPE_FUSION stopThread fail, ret(%d)", ret);
                /* TODO: exception handling */
                funcRet |= ret;
            }
        }
    }
#endif

    if (m_pipes[PIPE_GSC]->isThreadRunning() == true) {
        ret = m_pipes[PIPE_GSC]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_GSC stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USES_SW_VDIS
    if (m_pipes[INDEX(PIPE_VDIS)] != NULL) {
        if (m_pipes[PIPE_VDIS]->isThreadRunning() == true) {
            ret = m_pipes[PIPE_VDIS]->stopThread();
            if (ret != NO_ERROR) {
                CLOGE("PIPE_VDIS stopThread fail, ret(%d)", ret);
                /* TODO: exception handling */
                funcRet |= ret;
            }
        }
    }
#endif

    ret = m_pipes[sensorPipeId]->sensorStream(false);
    if (ret != NO_ERROR) {
        CLOGE("FLITE sensorStream off fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

    /* default settting */
    m_sensorStandby = true;
    m_needSensorStreamOn = true;

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_FLITE]->stop();
        if (ret != NO_ERROR) {
            CLOGE("FLITE stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* 3AA force done */
    ret = m_pipes[PIPE_3AA]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_3AA force done fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

    /* stream off for 3AA */
    ret = m_pipes[PIPE_3AA]->stop();
    if (ret != NO_ERROR) {
        CLOGE("3AA stop fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

    if (m_pipes[PIPE_BAYER_SYNC] != NULL) {
        ret = m_pipes[PIPE_BAYER_SYNC]->stop();
        if (ret != NO_ERROR) {
            CLOGE("BAYER_SYNC stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef SUPPORT_GMV
    if (m_parameters->getGmvMode() == true) {
        ret = m_pipes[PIPE_GMV]->stop();
        if (ret != NO_ERROR) {
            CLOGE("GMV stop fail, ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

    if (m_pipes[PIPE_ISP] != NULL && m_pipes[PIPE_ISP]->flagStart() == true) {
        /* ISP force done */
        ret = m_pipes[PIPE_ISP]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_ISP force done fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }

        /* stream off for ISP */
        ret = m_pipes[PIPE_ISP]->stop();
        if (ret != NO_ERROR) {
            CLOGE("ISP stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_pipes[PIPE_MCSC]->flagStart() == true) {
        /* MCSC force done */
        ret = m_pipes[PIPE_MCSC]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_MCSC force done fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }

        ret = m_pipes[PIPE_MCSC]->stop();
        if (ret != NO_ERROR) {
            CLOGE("MCSC stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_CLAHE_PREVIEW
    if (m_pipes[PIPE_CLAHE]->flagStart() == true) {
        /* CLAHE force done */
        ret = m_pipes[PIPE_CLAHE]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_CLAHE force done fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }

        ret = m_pipes[PIPE_CLAHE]->stop();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_pipes[i] == NULL) continue;

        ret = m_pipes[i]->stop();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_PLUGIN%d stop fail, ret(%d)", i, ret);
            funcRet |= ret;
        }
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    if (m_pipes[PIPE_NFD] != NULL) {
        ret = m_pipes[PIPE_NFD]->stop();
        CLOGV("previewVendor: m_pipes[PIPE_NFD]->stop() is called");
        if (ret != NO_ERROR) {
            CLOGE("NFD stop fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    if (m_pipes[PIPE_SW_MCSC] != NULL) {
        ret = m_pipes[PIPE_SW_MCSC]->stop();
        CLOGV("previewVendor: m_pipes[PIPE_SW_MCSC]->stop() is called");
        if (ret != NO_ERROR) {
            CLOGE("SW_MCSC stop fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#ifdef USE_VRA_FD
    if (m_pipes[PIPE_VRA] != NULL && m_pipes[PIPE_VRA]->flagStart() == true) {
        /* VRA force done */
        ret = m_pipes[PIPE_VRA]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_VRA force done fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }

        ret = m_pipes[PIPE_VRA]->stop();
        if (ret != NO_ERROR) {
            CLOGE("VRA stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#ifdef SUPPORT_HFD
    if (m_parameters->getHfdMode() == true) {
        ret = m_pipes[PIPE_HFD]->stop();
        if (ret != NO_ERROR) {
            CLOGE("HFD stop fail, ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(SUPPORT_HW_GDC)
    ret = stopThreadAndWait(PIPE_GDC);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_GDC stopThreadAndWait fail, ret(%d)", ret);
        funcRet |= ret;
    }
#endif

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_pipes[i] == NULL) continue;

        ret = stopThreadAndWait(i);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_PLUGIN%d stopThreadAndWait fail, ret(%d)", i, ret);
            funcRet |= ret;
        }
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    ret = stopThreadAndWait(PIPE_NFD);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_NFD stopThreadAndWait fail, ret(%d)", ret);
        funcRet |= ret;
    }
#endif

    if (m_pipes[PIPE_SYNC] != NULL) {
        ret = m_pipes[PIPE_SYNC]->stop();
        if (ret != NO_ERROR) {
            CLOGE("SYNC stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
        m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
        if (m_pipes[PIPE_FUSION] != NULL) {
            ret = m_pipes[PIPE_FUSION]->stop();
            if (ret < 0) {
                CLOGE("m_pipes[PIPE_FUSION]->stop() fail, ret(%d)", ret);
                /* TODO: exception handling */
                funcRet |= ret;
            }
        }
    }
#endif

    ret = m_pipes[PIPE_GSC]->stop();
    if (ret != NO_ERROR) {
        CLOGE("PIPE_GSC stop fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

#ifdef USES_SW_VDIS
    if (m_configurations->getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE) > 0
        && m_pipes[INDEX(PIPE_VDIS)] != NULL) {
        ret = m_pipes[INDEX(PIPE_VDIS)]->stop();
        if (ret < 0) {
            CLOGE("m_pipes[INDEX(PIPE_VDIS)]->stop() fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

    ret = m_transitState(FRAME_FACTORY_STATE_CREATE);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState. ret %d",
                 ret);
        funcRet |= ret;
    }

    int32_t bigMinLock, littleMinLock;
    bool lockFlag = m_configurations->getMinLockFreq(bigMinLock, littleMinLock);
    if (lockFlag) {
        CLOGI("CPU unlock(%d) big(0x%x) little(0x%x)", lockFlag, bigMinLock, littleMinLock);
        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER1, 0);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER1 setControl fail, ret(%d)", ret);
        }
        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER0, 0);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER0 setControl fail, ret(%d)", ret);
        }
    }

    CLOGI("Stopping  Success!");

    return funcRet;
}
}; /* namespace android */
