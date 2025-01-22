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
#define LOG_TAG "ExynosCameraSolutionVDIS"
#include <log/log.h>

#include "ExynosCameraFrame.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraConfigurations.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraFrameFactory.h"
#ifdef USE_DEBUG_PROPERTY
#include "ExynosCameraProperty.h"
#endif
#include "ExynosCameraSolutionSWVdis.h"

/* #define ENABLE_DEBUG_TIMESTAMP */

#ifdef ENABLE_DEBUG_TIMESTAMP
#define SW_VDIS_TS_LOG CLOGD
#else
#define SW_VDIS_TS_LOG CLOGV
#endif

ExynosCameraSolutionSWVdis::ExynosCameraSolutionSWVdis(int cameraId, int pipeId,
                                       ExynosCameraParameters *parameters,
                                       ExynosCameraConfigurations *configurations)
{
    m_cameraId = cameraId;
    strcpy(m_name, "ExynosCameraSolutionSWVdis");

    CLOGD("+ IN + ");

    m_pipeId = pipeId;
    m_pParameters = parameters;
    m_pConfigurations = configurations;
    m_capturePipeId = -1;
    m_pBufferSupplier = NULL;
    m_pFrameFactory = NULL;

    for(int i = 0; i <= NUM_SW_VDIS_INTERNAL_BUFFERS; i++) {
        m_timeStampList[i] = 0;
    }

    m_frameCountBufferIndexMap.clear();
}

ExynosCameraSolutionSWVdis::~ExynosCameraSolutionSWVdis()
{
    CLOGD("+ IN + ");

    m_frameCountBufferIndexMap.clear();
}

status_t ExynosCameraSolutionSWVdis::configureStream(void)
{
    CLOGD("+ IN + ");

    status_t ret = NO_ERROR;

    ret = m_adjustSize();

    return ret;
}

status_t ExynosCameraSolutionSWVdis::storeOriginalSize(void)
{
    CLOGD("+ IN + ");

    status_t ret = NO_ERROR;

    // store current hw yuv size
    int portId;
    cameraId_Info camIdInfo = m_pConfigurations->getCamIdInfo();
    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        ExynosCameraParameters *parameters = m_pConfigurations->getParameters(camIdInfo.cameraId[i]);
        if (parameters == nullptr) {
            CLOGE("[%d] param[%d] is null!!", i, camIdInfo.cameraId[i]);
            continue;
        }

        portId = parameters->getRecordingPortId();
        parameters->getSize(HW_INFO_MAX_HW_YUV_SIZE,
                (uint32_t *)&m_videoOriginalSize[i].w, (uint32_t *)&m_videoOriginalSize[i].h, portId);
        CLOGD("[%d] Store current RECORDING HwYuvSize. size %dx%d outputPortId %d",
                i, m_videoOriginalSize[i].w, m_videoOriginalSize[i].h, portId);

        portId = parameters->getPreviewPortId();
        parameters->getSize(HW_INFO_MAX_HW_YUV_SIZE,
                (uint32_t *)&m_previewOriginalSize[i].w, (uint32_t *)&m_previewOriginalSize[i].h, portId);
        CLOGD("[%d] Store current PREVIEW HwYuvSize. size %dx%d outputPortId %d",
                i, m_previewOriginalSize[i].w, m_previewOriginalSize[i].h, portId);
    }

    return ret;
}

status_t ExynosCameraSolutionSWVdis::flush(ExynosCameraFrameFactory *frameFactory)
{
    CLOGD("+ IN + ");

    status_t ret = NO_ERROR;
    bool bStart = false;
    frame_queue_t *frameQ = NULL;
    ExynosCameraFrameFactory *pFrameFactory = frameFactory;
    cameraId_Info camIdInfo = m_pConfigurations->getCamIdInfo();

    if (pFrameFactory == NULL)
        pFrameFactory = m_pFrameFactory;


    if(pFrameFactory == NULL) {
        CLOGW("frameFactory is NULL!!");
        return INVALID_OPERATION;
    }

    pFrameFactory->getInputFrameQToPipe(&frameQ, m_pipeId);
    if (frameQ == NULL)
        goto SKIP_FRAMEQ_RELEASE;

    while(frameQ->getSizeOfProcessQ() > 0) {
        CLOGW("VDIS pipe inputQ(%d)", frameQ->getSizeOfProcessQ());
        frameQ->sendCmd(WAKE_UP);
        usleep(DM_WAITING_TIME);
    }

    pFrameFactory->stopThread(m_pipeId);
    pFrameFactory->stopThreadAndWait(m_pipeId);

    frameQ->release();

    // restore previous hwYuvSize
    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        ExynosCameraParameters *parameters = m_pConfigurations->getParameters(camIdInfo.cameraId[i]);
        if (parameters == nullptr) {
            CLOGE("[%d] param[%d] is null!!", i, camIdInfo.cameraId[i]);
            continue;
        }

        int portId = parameters->getRecordingPortId();
        CLOGD("[%d] Restore previous RECORDING HwYuvSize. size %dx%d outputPortId %d",
                i, m_videoOriginalSize[i].w, m_videoOriginalSize[i].h, portId);
        ret = parameters->checkHwYuvSize(m_videoOriginalSize[i].w, m_videoOriginalSize[i].h, portId);
        if (ret != NO_ERROR) {
            CLOGE("[%d] Failed to restore previous RECORDING HwYuvSize. size %dx%d outputPortId %d",
                    i, m_videoOriginalSize[i].w, m_videoOriginalSize[i].h, portId);
        }

        portId = parameters->getPreviewPortId();
        CLOGD("[%d] Restore previous PREVIEW HwYuvSize. size %dx%d outputPortId %d",
                i, m_previewOriginalSize[i].w, m_previewOriginalSize[i].h, portId);
        ret = parameters->checkHwYuvSize(m_previewOriginalSize[i].w, m_previewOriginalSize[i].h, portId);
        if (ret != NO_ERROR) {
            CLOGE("[%d] Failed to restore previous PREVIEW HwYuvSize. size %dx%d outputPortId %d",
                    i, m_previewOriginalSize[i].w, m_previewOriginalSize[i].h, portId);
        }
    }
SKIP_FRAMEQ_RELEASE:

    pFrameFactory->setParameter(m_pipeId, PLUGIN_PARAMETER_KEY_STOP, (void*)&bStart);
    CLOGD("- OUT - ");

    return ret;
}

status_t ExynosCameraSolutionSWVdis::setBuffer(ExynosCameraBufferSupplier* bufferSupplier)
{
    CLOGD("+ IN + ");

    status_t ret = NO_ERROR;
    m_pBufferSupplier = bufferSupplier;
    return ret;
}

int ExynosCameraSolutionSWVdis::getPipeId(void)
{
    return m_pipeId;
}

int ExynosCameraSolutionSWVdis::getCapturePipeId(void)
{
    return m_capturePipeId;
}

void ExynosCameraSolutionSWVdis::checkMode(void)
{
    CLOGD("+ IN + ");

    if (m_pConfigurations->isSupportedFunction(SUPPORTED_FUNCTION_SW_VDIS) == false) {
        CLOGW("Not support SW_VDIS");
        return;
    }

    int videoW = 0, videoH = 0;
    int previewW = 0, previewH = 0;
    m_pConfigurations->getSize(CONFIGURATION_VIDEO_SIZE, (uint32_t *)&videoW, (uint32_t *)&videoH);
    m_pConfigurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);

    int fps;
    fps = m_pConfigurations->getModeValue(CONFIGURATION_RECORDING_FPS);

    if (m_pConfigurations->getMode(CONFIGURATION_RECORDING_MODE) != true) {
        CLOGE("This mode is NOT recording mode");
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, false);
        return;
    }

#ifdef USE_SW_VDIS_UHD_RECORDING
    if (videoW == 3840 && videoH == 2160
#ifdef USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE
        && previewW == 1920 && previewH == 1080
#endif
        ) {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, true);
    } else
#endif
    if (videoW == 1920 && videoH == 1080
#ifdef USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE
        && previewW == 1920 && previewH == 1080
#endif
        ) {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, true);
    } else
#ifdef USE_SW_VDIS_HD30_RECORDING
    if (videoW == 1280 && videoH == 720
#ifdef USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE
        && ((previewW == 1920 && previewH == 1080)
#ifdef USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE_HD
            || (previewW == 1280 && previewH == 720)
#endif
           )
#endif
        ) {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, true);
    } else
#endif
#ifdef USE_SW_VDIS_1920_816_30_RECORDING
    if (videoW == 1920 && videoH == 816
#ifdef USE_SW_VDIS_MATCH_PREVIEW_RECORDING_SIZE
            && previewW == 1920 && previewH == 816
#endif
            ) {
            m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, true);
    } else
#endif
    {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, false);
    }

    if (fps > 30) {
        if (m_pConfigurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE)) {
#ifdef USE_SW_VDIS_FHD60_RECORDING
            if (videoW == 1920 && videoH == 1080 && fps == 60) {
                /* use FHD60 FPS VDIS recording */
            } else
#endif
            {
                m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, false);
            }
        }
    }

#ifdef USE_DUAL_CAMERA
    if (m_pConfigurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
            m_pConfigurations->getScenario() != SCENARIO_DUAL_REAR_ZOOM) {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, false);
    } else
#endif
    if (m_pConfigurations->getMode(CONFIGURATION_PIP_MODE) == true) {
        m_pConfigurations->setMode(CONFIGURATION_VIDEO_STABILIZATION_MODE, false);
    }

    if (m_pConfigurations->getEnabledByProperty(SUPPORTED_FUNCTION_SW_VDIS)
        && m_pConfigurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE)) {
        m_pConfigurations->setModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE, 1);
    }

//EXIT_FUNC:
    CLOGD("set VDIS mode(%d) modeEnable(%d), size(p:%dx%d, r:%dx%d) fps(%d)",
        m_pConfigurations->getMode(CONFIGURATION_VIDEO_STABILIZATION_MODE),
        m_pConfigurations->getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE),
        previewW, previewH, videoW, videoH, fps);
}

status_t ExynosCameraSolutionSWVdis::handleFrame(SOLUTION_PROCESS_TYPE type,
                                                    sp<ExynosCameraFrame> frame,
                                                    int prevPipeId,
                                                    int capturePipeId,
                                                    ExynosCameraFrameFactory *factory)
{
    status_t ret = NO_ERROR;

    switch(type) {
    case SOLUTION_PROCESS_PRE:
        if (m_capturePipeId < 0)
            m_capturePipeId = capturePipeId;

        if (!m_pFrameFactory)
            m_pFrameFactory = factory;

        ret = m_handleFramePreProcess(frame, prevPipeId, capturePipeId, factory);
        break;
    case SOLUTION_PROCESS_POST:
        ret = m_handleFramePostProcess(frame, prevPipeId, capturePipeId, factory);
        break;
    default:
        CLOGW("Unknown SOLUTION_PROCESS_TYPE(%d)", type);
        break;
    }

    return ret;
}

void ExynosCameraSolutionSWVdis::getSize(int& w, int& h)
{
    m_getSize(w, h);
}

uint64_t ExynosCameraSolutionSWVdis::adjustTimeStamp(uint32_t frameCount)
{
    int bufferIndex = m_findBufferIndexWith(frameCount);
    uint64_t timeStamp = 0;

    if (bufferIndex >= 0 && bufferIndex <= NUM_SW_VDIS_INTERNAL_BUFFERS) {
        timeStamp = m_timeStampList[bufferIndex];
    } else {
        CLOGD("Failed to adjust timeStamp F(%d)", frameCount);
    }
    CLOGV("[VDIS] [F:%d | T:%ju]", frameCount, timeStamp);
    return timeStamp;
}

status_t ExynosCameraSolutionSWVdis::m_handleFramePreProcess(sp<ExynosCameraFrame> frame,
                                                                int prevPipeId,
                                                                int capturePipeId,
                                                                ExynosCameraFrameFactory *factory)

{
    CLOGV("+ IN + ");

    status_t ret = NO_ERROR;

    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer meBuffer;
    entity_buffer_state_t bufferState = ENTITY_BUFFER_STATE_INVALID;
    struct camera2_shot_ext *buffer_ext = NULL;

    int pipeId = getPipeId();
    int srcPos = OUTPUT_NODE_1;
    int dstPos = OTF_NODE_1;


    /* 1. Getting VDIS input src buffer from dst buffer of previous pipe */
    ret = frame->getDstBuffer(prevPipeId, &srcBuffer, factory->getNodeType(capturePipeId));
    if (ret != NO_ERROR || srcBuffer.index < 0) {
        CLOGE("Failed to get DST buffer. pipeId %d bufferIndex %d frameCount %d ret %d",
                                        capturePipeId, srcBuffer.index, frame->getFrameCount(), ret);
        return ret;
    }

    ret = frame->getDstBufferState(prevPipeId, &bufferState, factory->getNodeType(capturePipeId));
    if (ret < 0) {
        CLOGE("getDstBufferState fail, pipeId(%d), ret(%d)", prevPipeId, ret);
        return ret;
    }

    if (bufferState == ENTITY_BUFFER_STATE_ERROR) {
        CLOGE("Dst buffer state is error index(%d), framecount(%d), pipeId(%d)",
                            srcBuffer.index, frame->getFrameCount(), prevPipeId);
        return INVALID_OPERATION;
    }

    m_collectTimeStamp(srcBuffer.index, frame->getTimeStampBoot());

    /* Update SrcBuffer Metadata */
    buffer_ext = (struct camera2_shot_ext*)srcBuffer.addr[srcBuffer.getMetaPlaneIndex()];
    if (buffer_ext != NULL) {
        frame->getMetaData(buffer_ext);
    }

    CLOGV("[OSC] Idx(%d) addr : %p", srcBuffer.index, srcBuffer.addr[0]);
    /* 2. Setting VDIS input src buffer to entity PIPE_VDIS of frame */
    ret = frame->setSrcBuffer(pipeId, srcBuffer, srcPos);
    if (ret != NO_ERROR || srcBuffer.index < 0) {
        CLOGE("Failed to set SRC buffer. pipeId %d bufferIndex %d frameCount %d ret %d",
                                        pipeId, srcBuffer.index, frame->getFrameCount(), ret);
        return ret;
    }

#ifdef SUPPORT_ME
    /* 3. Getting ME buffer */
    int mePipeId = PIPE_ME;
    int meLeaderPipe = m_pParameters->getLeaderPipeOfMe();
    if (meLeaderPipe < 0) return INVALID_OPERATION;

    ret = frame->getDstBuffer(meLeaderPipe, &meBuffer, factory->getNodeType(mePipeId));
    if (ret != NO_ERROR || meBuffer.index < 0) {
        CLOGE("Failed to get ME buffer. pipeId %d bufferIndex %d frameCount %d ret %d",
                                        meLeaderPipe, meBuffer.index, frame->getFrameCount(), ret);
        return ret;
    }

    ret = frame->getDstBufferState(meLeaderPipe, &bufferState, factory->getNodeType(mePipeId));
    if (ret < 0) {
        CLOGE("Failed to get ME bufferState, pipeId(%d), ret(%d)", meLeaderPipe, ret);
        return ret;
    }

    if (bufferState == ENTITY_BUFFER_STATE_ERROR) {
        CLOGE("ME buffer state is error index(%d), framecount(%d), pipeId(%d)",
                            meBuffer.index, frame->getFrameCount(), meLeaderPipe);
        return INVALID_OPERATION;
    }

    /* 4. Setting ME buffer to entity PIPE_VDIS of frame */
    ret = frame->setDstBuffer(pipeId, meBuffer, dstPos);
    if (ret != NO_ERROR || meBuffer.index < 0) {
        CLOGE("Failed to srt SRC buffer. pipeId %d bufferIndex %d frameCount %d ret %d",
                                        pipeId, meBuffer.index, frame->getFrameCount(), ret);
        return ret;
    }
#endif

    /* 5. Setting colorFormat */
    ExynosRect rect;

    ret = frame->getSrcRect(pipeId, &rect, srcPos);
    if (ret != NO_ERROR) {
        CLOGE("frame[%d]->getSrcRect(%d) fail", frame->getFrameCount(), pipeId);
    } else if (rect.colorFormat == 0) {
        rect.colorFormat = V4L2_PIX_FMT_NV21M;

        ret = frame->setSrcRect(pipeId, &rect, srcPos);
        if (ret != NO_ERROR) {
            CLOGE("frame[%d]->setSrcRect(%d, %d) fail", frame->getFrameCount(), pipeId, srcPos);
        }
    }

    for (int i = 0; i < 2; i++) {
        ret = frame->getDstRect(pipeId, &rect, i);
        if (ret != NO_ERROR) {
            CLOGE("frame[%d]->getSrcRect(%d) fail", frame->getFrameCount(), pipeId);
        } else if (rect.colorFormat == 0) {
            rect.colorFormat = V4L2_PIX_FMT_NV21M;

            ret = frame->setDstRect(pipeId, &rect, i);
            if (ret != NO_ERROR) {
                CLOGE("frame[%d]->setDstRect(%d, %d) fail", frame->getFrameCount(), pipeId, i);
            }

            ret = frame->setDstRect(pipeId, &rect, i);
            if (ret != NO_ERROR) {
                CLOGE("frame[%d]->setDstRect(%d) fail", frame->getFrameCount(), pipeId, i);
            }
        }
    }


    //for testing
    //ret = -1;
    return ret;
}

status_t ExynosCameraSolutionSWVdis::m_handleFramePostProcess(sp<ExynosCameraFrame> frame,
                                                                int prevPipeId,
                                                                __unused int capturePipeId,
                                                                __unused ExynosCameraFrameFactory *factory)

{
    CLOGV("+ IN + ");

    status_t ret = NO_ERROR;

    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer meBuffer;
    entity_buffer_state_t srcBufferState = ENTITY_BUFFER_STATE_NOREQ;

    /* Getting VDIS input src buffer to release */
    ret = frame->getSrcBufferState(prevPipeId, &srcBufferState);
    if (ret != NO_ERROR) {
        CLOGE("Failed to get src buffer state");
    } else {

        switch(srcBufferState) {
        case ENTITY_BUFFER_STATE_COMPLETE:
            /* Release src buffer */
            ret = frame->getSrcBuffer(prevPipeId, &srcBuffer);
            if (ret != NO_ERROR || srcBuffer.index < 0) {
                CLOGE("Failed to get SRC_BUFFER. pipeId(%d) bufferIndex(%d) frameCount(%d) ret(%d)",
                        prevPipeId, srcBuffer.index, frame->getFrameCount(), ret);
                return ret;
            }

            m_makePair(frame->getFrameCount(), srcBuffer.index);

            if(m_pBufferSupplier != NULL) {
                m_pBufferSupplier->putBuffer(srcBuffer);
                if (ret != NO_ERROR) {
                    CLOGE("Failed to release ME buffer, pipeId(%d), ret(%d), frame(%d) buffer.index(%d)",
                            prevPipeId, ret, frame->getFrameCount(), meBuffer.index);
                    return ret;
                }
                CLOGV("Success to release src buffer(%d)", srcBuffer.index);
            } else {
                CLOGE("bufferSupplier is NULL!!");
            }

            break;
        case ENTITY_BUFFER_STATE_PROCESSING:
            /* Skip to release src buffer */
            CLOGV("src bufferState(ENTITY_BUFFER_STATE_PROCESSING)");
            break;
        case ENTITY_BUFFER_STATE_ERROR:
            CLOGE("src bufferState(ENTITY_BUFFER_STATE_ERROR)");
            break;
        default:
            break;
        }

    }

    entity_buffer_state_t dstBufferState = ENTITY_BUFFER_STATE_NOREQ;
    ret = frame->getDstBufferState(prevPipeId, &dstBufferState);
    if (ret != NO_ERROR) {
        CLOGE("Failed to get src buffer state");
    } else {

        switch(dstBufferState) {
        case ENTITY_BUFFER_STATE_COMPLETE:
            CLOGV("dst bufferState(ENTITY_BUFFER_STATE_COMPLETE)");
            break;
        case ENTITY_BUFFER_STATE_PROCESSING:
            /* Skip to release src buffer */
            CLOGV("dst bufferState(ENTITY_BUFFER_STATE_PROCESSING)");
            break;
        case ENTITY_BUFFER_STATE_ERROR:
            CLOGE("dst bufferState(ENTITY_BUFFER_STATE_ERROR)");
            break;
        default:
            CLOGD("dst bufferState(%d)", dstBufferState);
            break;
        }
    }

#ifdef SUPPORT_ME
    /* Relase ME buffer */
    ret = frame->getDstBuffer(prevPipeId, &meBuffer, OTF_NODE_1);
    if (ret != NO_ERROR) {
        CLOGE("Failed to get ME buffer, pipeId(%d), ret(%d), frame(%d)",
                prevPipeId, ret, frame->getFrameCount());
        return ret;
    }

    if(meBuffer.index < 0) {
        CLOGE("ME buffer index(%d) is invalid", meBuffer.index);
        return ret;
    }

    ret = m_pBufferSupplier->putBuffer(meBuffer);
    if (ret != NO_ERROR) {
        CLOGE("Failed to release ME buffer, pipeId(%d), ret(%d), frame(%d) buffer.index(%d)",
                prevPipeId, ret, frame->getFrameCount(), meBuffer.index);
        return ret;
    }

    CLOGV("Success to release me buffer(%d)", meBuffer.index);
#endif

    return ret;
}

status_t ExynosCameraSolutionSWVdis::m_getSize(int& hwWidth, int& hwHeight)
{
    status_t ret = NO_ERROR;
    int fps = m_pConfigurations->getModeValue(CONFIGURATION_RECORDING_FPS);
    int videoW = 0, videoH = 0;

    bool superEisMode = m_pConfigurations->getMode(CONFIGURATION_SUPER_EIS_MODE);

    m_pConfigurations->getSize(CONFIGURATION_VIDEO_SIZE, (uint32_t *)&videoW, (uint32_t *)&videoH);

#ifdef USE_SW_VDIS_UHD_RECORDING
    if (videoW == 3840 && videoH == 2160) {
        hwWidth = VIDEO_MARGIN_UHD_W;
        hwHeight = VIDEO_MARGIN_UHD_H;
    } else
#endif
    if (videoW == 1920 && videoH == 1080) {
        hwWidth = VIDEO_MARGIN_FHD_W;
        hwHeight = VIDEO_MARGIN_FHD_H;
#ifdef USE_SUPER_EIS
        if (superEisMode) {
            hwWidth = VIDEO_SUPER_EIS_MARGIN_FHD_W;
            hwHeight = VIDEO_SUPER_EIS_MARGIN_FHD_H;
        } else
#endif
        {
            hwWidth = VIDEO_MARGIN_FHD_W;
            hwHeight = VIDEO_MARGIN_FHD_H;
        }
    } else if (videoW == 1920 && videoH == 816){
        hwWidth = VIDEO_MARGIN_1920_W;
        hwHeight = VIDEO_MARGIN_816_H;
    } else if (videoW == 1280 && videoH == 720){
#ifdef USE_SUPER_EIS
        if (superEisMode) {
            hwWidth = VIDEO_SUPER_EIS_MARGIN_HD_W;
            hwHeight = VIDEO_SUPER_EIS_MARGIN_HD_H;
        } else
#endif
        {
            hwWidth = VIDEO_MARGIN_HD_W;
            hwHeight = VIDEO_MARGIN_HD_H;
        }
    } else {
        hwWidth = videoW;
        hwHeight = videoH;
    }

    if (hwWidth == 0 || hwHeight == 0) {
        CLOGE("Not supported VDIS size %dx%d fps %d", videoW, videoH, fps);
        ret = BAD_VALUE;
    }

    if (ret != NO_ERROR) {
        hwWidth = videoW;
        hwHeight = videoH;
    }

    return ret;
}

status_t ExynosCameraSolutionSWVdis::m_adjustSize(void)
{
    CLOGD("+ IN + ");

    status_t ret = NO_ERROR;
    int hwWidth = 0, hwHeight = 0;

    ret = m_getSize(hwWidth, hwHeight);

    cameraId_Info camIdInfo = m_pConfigurations->getCamIdInfo();
    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        ExynosCameraParameters *parameters = m_pConfigurations->getParameters(camIdInfo.cameraId[i]);
        if (parameters == nullptr) {
            CLOGE("[%d] param[%d] is null!!", i, camIdInfo.cameraId[i]);
            continue;
        }

        int portId = parameters->getRecordingPortId();
        ret = parameters->checkHwYuvSize(hwWidth, hwHeight, portId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setHwYuvSize for PREVIEW stream(VDIS). size %dx%d outputPortId %d",
                    hwWidth, hwHeight, portId);
        }

        portId = parameters->getPreviewPortId();
        ret = parameters->checkHwYuvSize(hwWidth, hwHeight, portId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setHwYuvSize for PREVIEW stream(VDIS). size %dx%d outputPortId %d",
                    hwWidth, hwHeight, portId);
        }
    }

    return ret;
}

bool ExynosCameraSolutionSWVdis::m_isEnabledByUser() {
    bool bEnabled = false;
    status_t ret = NO_ERROR;

#ifdef USE_DEBUG_PROPERTY
    ExynosCameraProperty property;

    ret = property.get(ExynosCameraProperty::SOLUTION_VDIS_ENABLE, LOG_TAG, bEnabled);
    if (ret != NO_ERROR) {
        bEnabled = false;
    }
#endif

    return bEnabled;
}

status_t ExynosCameraSolutionSWVdis::m_collectTimeStamp(int index, uint64_t timestamp)
{
    SW_VDIS_TS_LOG("+ IN +");

    status_t ret = NO_ERROR;

    if (index <= NUM_SW_VDIS_INTERNAL_BUFFERS) {
        m_timeStampList[index] = timestamp;
        SW_VDIS_TS_LOG("[VDIS] [I:%d | T:%ju]", index, timestamp);
    }

    return ret;
}

void ExynosCameraSolutionSWVdis::m_makePair(uint32_t frameCount, int bufferIndex) {

    SW_VDIS_TS_LOG("[VDIS] [F:%d | I:%d]", frameCount, bufferIndex);

    Mutex::Autolock l(m_frameCountBufferIndexLock);

	m_frameCountBufferIndexMap.insert(pair<uint32_t, int>(frameCount, bufferIndex));
}

int ExynosCameraSolutionSWVdis::m_findBufferIndexWith(uint32_t frameCount) {
    map<uint32_t, int>::iterator iter;
    int bufferIndex = -1;

    Mutex::Autolock l(m_frameCountBufferIndexLock);

    iter = m_frameCountBufferIndexMap.find(frameCount);
    if (iter != m_frameCountBufferIndexMap.end()) {
        bufferIndex = iter->second;

        m_frameCountBufferIndexMap.erase(iter);
    }

    SW_VDIS_TS_LOG("[VDIS] [F:%d | I:%d]", frameCount, bufferIndex);

    return bufferIndex;
}

