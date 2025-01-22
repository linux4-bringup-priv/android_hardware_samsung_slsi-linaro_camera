/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraPipeSWMCSC"
#include "ExynosCameraPipeSWMCSC.h"

namespace android {

ExynosCameraPipeSWMCSC::~ExynosCameraPipeSWMCSC()
{
    this->destroy();
}

status_t ExynosCameraPipeSWMCSC::create(__unused int32_t *sensorIds)
{
    status_t ret = NO_ERROR;

    ret= ExynosCameraSWPipe::create(sensorIds);

    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_flagValidInt(m_deviceInfo->secondaryNodeNum[i]) == true) {
            switch (m_deviceInfo->secondaryNodeNum[i]) {
            case PREVIEW_GSC_NODE_NUM: {
                int32_t nodeNums[MAX_NODE] = {0};
                nodeNums[OUTPUT_NODE] = m_deviceInfo->secondaryNodeNum[i];

                m_gscPipes[i] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_configurations, m_parameters, true, &nodeNums[OUTPUT_NODE], m_camIdInfo);
                m_gscPipes[i]->setPipeId(m_deviceInfo->pipeId[i]);
                m_gscPipes[i]->setPipeName(m_deviceInfo->secondaryNodeName[i]);
                break;
            }
            default: {
                break;
            }
            }
        }
    }

    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        ret = m_gscPipes[i]->create(sensorIds);
        if (ret != NO_ERROR) {
            CLOGE("Internal GSC Pipe(%d) creation fail!", i);
            return ret;
        }
        m_gscFrameDoneQ[i] = new frame_queue_t(m_gscDoneThread);
    }
    m_handleGscDoneQ = new frame_queue_t(m_gscDoneThread);

    m_gscDoneThread = new ExynosCameraThread<ExynosCameraPipeSWMCSC>(this, &ExynosCameraPipeSWMCSC::m_gscDoneThreadFunc, "gscDoneThread");

    return NO_ERROR;
}

status_t ExynosCameraPipeSWMCSC::destroy(void)
{
    status_t ret = NO_ERROR;

    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        ret = m_gscPipes[i]->destroy();
        if (ret != NO_ERROR)
            CLOGE("Internal GSC Pipe(%d) detstroy fail!", i);
        if (m_gscFrameDoneQ[i]) {
            delete m_gscFrameDoneQ[i];
            m_gscFrameDoneQ[i] = NULL;
        }
        delete m_gscPipes[i];
        m_gscPipes[i] = NULL;
    }

    delete m_handleGscDoneQ;
    m_handleGscDoneQ = NULL;
    /* sp<Thread> */
    m_gscDoneThread = NULL;

    ExynosCameraSWPipe::m_destroy();

    return NO_ERROR;
}

status_t ExynosCameraPipeSWMCSC::stop(void)
{
    status_t ret = NO_ERROR;

    m_flagTryStop = true;

    m_mainThread->requestExitAndWait();
    m_gscDoneThread->requestExitAndWait();

    m_inputFrameQ->release();
    m_handleGscDoneQ->release();

    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        ret = m_gscPipes[i]->stop();
        if (ret != NO_ERROR)
            CLOGE("Internal GSC Pipe stop fail!");

        m_gscFrameDoneQ[i]->release();
    }

    m_flagTryStop = false;

    CLOGI("stop() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraPipeSWMCSC::stopThread(void)
{
    status_t ret = NO_ERROR;

    m_gscDoneThread->requestExit();
    m_handleGscDoneQ->sendCmd(WAKE_UP);
    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        m_gscFrameDoneQ[i]->sendCmd(WAKE_UP);
    }

    ret = ExynosCameraPipe::stopThread();
    if (ret != NO_ERROR)
        CLOGE("Internal GSC Pipe stopThread fail!");

    CLOGI("stopThread is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraPipeSWMCSC::m_run(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraBuffer srcBuffer;

    struct camera2_stream *streamMeta = NULL;
    uint32_t *mcscOutputCrop = NULL;

    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("new frame is NULL");
        return BAD_VALUE;
    }

    if (newFrame->getFrameState() == FRAME_STATE_SKIPPED
        || newFrame->getFrameState() == FRAME_STATE_INVALID) {
        if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL) {
            CLOGE("New frame is INVALID, frameCount(%d)",
                     newFrame->getFrameCount());
        }
        goto FUNC_EXIT;
    }

    /* Get scaler source buffer */
    ret = newFrame->getSrcBuffer(getPipeId(), &srcBuffer);
    if (ret != NO_ERROR) {
        CLOGE("getSrcBuffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto FUNC_EXIT;
    }

    entity_buffer_state_t srcBufferState;
    ret = newFrame->getSrcBufferState(getPipeId(), &srcBufferState);
    if (srcBuffer.index < 0
        || srcBufferState == ENTITY_BUFFER_STATE_ERROR) {
        if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL) {
            CLOGE("Invalid SrcBuffer(index:%d, state:%d)",
                    srcBuffer.index, srcBufferState);
        }
        goto FUNC_EXIT;
    }

    /* Get size from metadata */
    streamMeta = (struct camera2_stream*)srcBuffer.addr[srcBuffer.getMetaPlaneIndex()];
    if (streamMeta == NULL) {
        CLOGE("srcBuffer.addr is NULL pipeId(%d)", getPipeId());
        goto FUNC_EXIT;
    }

    mcscOutputCrop = streamMeta->output_crop_region;

    /* Run scaler based source size */
    /*
    if (mcscOutputCrop[2] * mcscOutputCrop[3] > FHD_PIXEL_SIZE) {
        ret = m_runScalerSerial(newFrame);
    } else {
    */
        ret = m_runScalerParallel(newFrame);
    //}

    if (ret != NO_ERROR) {
        CLOGE("m_runScaler() fail!, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto FUNC_EXIT;
    }

    return NO_ERROR;

FUNC_EXIT:
    if (newFrame != NULL) {
        ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
        if (ret != NO_ERROR) {
            CLOGE("set entity state fail, ret(%d)", ret);
            /* TODO: doing exception handling */
            return INVALID_OPERATION;
        }

        m_outputFrameQ->pushProcessQ(&newFrame);
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeSWMCSC::m_runScalerSerial(ExynosCameraFrameSP_sptr_t frame)
{
    return NO_ERROR;
}

status_t ExynosCameraPipeSWMCSC::m_runScalerParallel(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    ExynosRect srcRect, dstRect;
    camera2_node_group node_group_info;
    int pipeId = -1;
    int base = -1;
    int bufferIndex = -1;
    int perframeInfoIdx = -1;
    int flipHorizontal = 0;
    int flipVertical = 0;

    if (m_reprocessing == false) {
        perframeInfoIdx = PERFRAME_INFO_SWMCSC;
        base = PIPE_MCSC0;
    } else {
        perframeInfoIdx = PERFRAME_INFO_REPROCESSING_SWMCSC;
        base = PIPE_MCSC0_REPROCESSING;
    }

    /* Get scaler source buffer */
    ret = frame->getSrcBuffer(getPipeId(), &srcBuffer);
    if (ret != NO_ERROR) {
        CLOGE("getSrcBuffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return INVALID_OPERATION;
    }

    frame->getNodeGroupInfo(&node_group_info, perframeInfoIdx);

    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        status_t funcRet = NO_ERROR;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("getPipeId(%d) fail", i);
            return BAD_VALUE;
        }

        /* Get scaler destination buffer */
        if (frame->getRequest(pipeId) == false && node_group_info.capture[pipeId - base].request == false)
            continue;

        if ((m_configurations->getOnePortId() > -1) &&
                (pipeId == (m_configurations->getOnePortId() % ExynosCameraParameters::YUV_MAX) + PIPE_MCSC0)) {
            CLOGV("[F%d] pipeId(%d) is not output of SWMCSC", frame->getFrameCount(), pipeId);
            continue;
        }

        dstBuffer.index = -1;

        if (m_bufferSupplier == NULL) {
            CLOGE("[F%d]BufferSupplier is NULL, i(%d), piepId(%d)",
                    frame->getFrameCount(), i, pipeId);
            continue;
        }

        ret = frame->getDstBuffer(getPipeId(), &dstBuffer, i);
        if (ret != NO_ERROR) {
            CLOGE("getDstBuffer fail. pipeId(%d), frameCount(%d), ret(%d)",
                    pipeId, frame->getFrameCount(), ret);
            continue;
        }

        if (dstBuffer.index < 0) {
            buffer_manager_tag_t bufTag;
            bufTag.pipeId[0] = pipeId;
            bufTag.managerType = m_deviceInfo->bufferManagerType[i];
            ret = m_bufferSupplier->getBuffer(bufTag, &dstBuffer);
            if (ret != NO_ERROR) {
                CLOGE("[%s][F%d]Failed to getBuffer from BufferSupplier. ret %d",
                        m_deviceInfo->nodeName[i], frame->getFrameCount(), ret);
                frame->dump();
                frame->setRequest(pipeId, false);
                continue;
            }
        } else {
            CLOGV("Skip to get buffer from bufferMgr.\
                    pipeId(%d), frameCount(%d) bufferIndex %d)",
                    pipeId, frame->getFrameCount(), dstBuffer.index);
        }

        /* Set source buffer to Scaler */
        funcRet = frame->setSrcBuffer(pipeId, srcBuffer);
        if (funcRet != NO_ERROR) {
            CLOGE("setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            ret |= funcRet;
        }

        /* Set destination buffer to Scaler */
        funcRet = frame->setDstBuffer(pipeId, dstBuffer);
        if (funcRet != NO_ERROR) {
            CLOGE("setDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            ret |= funcRet;
        }

        /* Set scaler source size */
        srcRect.colorFormat = node_group_info.leader.pixelformat;
        srcRect.x = node_group_info.capture[pipeId - base].input.cropRegion[0];
        srcRect.y = node_group_info.capture[pipeId - base].input.cropRegion[1];
        srcRect.w = node_group_info.capture[pipeId - base].input.cropRegion[2];
        srcRect.h = node_group_info.capture[pipeId - base].input.cropRegion[3];
        srcRect.fullW = ALIGN_UP(node_group_info.leader.input.cropRegion[2], CAMERA_16PX_ALIGN);
#if GRALLOC_CAMERA_64BYTE_ALIGN
        if (srcRect.colorFormat == V4L2_PIX_FMT_NV21M)
            srcRect.fullW = ALIGN_UP(node_group_info.leader.input.cropRegion[2], CAMERA_64PX_ALIGN);
#endif
        srcRect.fullH = node_group_info.leader.input.cropRegion[3];

        /* Set scaler destination size */
        dstRect.colorFormat = node_group_info.capture[pipeId - base].pixelformat;
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = node_group_info.capture[pipeId - base].output.cropRegion[2];
        dstRect.h = node_group_info.capture[pipeId - base].output.cropRegion[3];
        dstRect.fullW = node_group_info.capture[pipeId - base].output.cropRegion[2];
#if GRALLOC_CAMERA_64BYTE_ALIGN
        if (dstRect.colorFormat == V4L2_PIX_FMT_NV21M)
            dstRect.fullW = ALIGN_UP(node_group_info.capture[pipeId - base].output.cropRegion[2], CAMERA_64PX_ALIGN);
#endif
        dstRect.fullH = node_group_info.capture[pipeId - base].output.cropRegion[3];



        ret |= frame->setSrcRect(pipeId, srcRect);
        ret |= frame->setDstRect(pipeId, dstRect);

        ////////////////////////////////////////////////
        // get each pipeId's flip info.
        flipHorizontal = frame->getFlipHorizontal(pipeId);
        flipVertical   = frame->getFlipVertical(pipeId);

        ////////////////////////////////////////////////

        CLOGV("[F%d] Scaling(MCSC%d) pipeId(%d):(flipH:%d, flipV:%d) \
                srcBuf.index(%d) dstBuf.index(%d) \
                (size:%d, %d, %d x %d) format(%c%c%c%c) actual(%x) fd(%d) addr(%p)-> \
                (size:%d, %d, %d x %d) format(%c%c%c%c) actual(%x) fd(%d) addr(%p)",
                frame->getFrameCount(),
                i,  pipeId,
                flipHorizontal, flipVertical,
                srcBuffer.index, dstBuffer.index,
                srcRect.x, srcRect.y, srcRect.w, srcRect.h,
                v4l2Format2Char(srcRect.colorFormat, 0),
                v4l2Format2Char(srcRect.colorFormat, 1),
                v4l2Format2Char(srcRect.colorFormat, 2),
                v4l2Format2Char(srcRect.colorFormat, 3),
                V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat),
                srcBuffer.fd[0], srcBuffer.addr[0],
                dstRect.x, dstRect.y, dstRect.w, dstRect.h,
                v4l2Format2Char(dstRect.colorFormat, 0),
                v4l2Format2Char(dstRect.colorFormat, 1),
                v4l2Format2Char(dstRect.colorFormat, 2),
                v4l2Format2Char(dstRect.colorFormat, 3),
                V4L2_PIX_2_HAL_PIXEL_FORMAT(dstRect.colorFormat),
                dstBuffer.fd[0], dstBuffer.addr[0]);

        /* TODO: doing exception handling */
        if (ret != NO_ERROR) {
            if (m_bufferSupplier != NULL) {
                ret = m_bufferSupplier->putBuffer(dstBuffer);
                if (ret != NO_ERROR) {
                    CLOGE("[%s][F%d B%d]Failed to putBuffer. ret %d",
                            m_deviceInfo->nodeName[i],
                            frame->getFrameCount(),
                            dstBuffer.index,
                            ret);
                }
            }

            ret = frame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
            if (ret != NO_ERROR) {
                CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                        getPipeId(), frame->getFrameCount(), ret);
            }

            frame->setRequest(pipeId, false);
            continue;
        }

        /* Copy metadata to destination buffer */
        memcpy(dstBuffer.addr[dstBuffer.getMetaPlaneIndex()], srcBuffer.addr[srcBuffer.getMetaPlaneIndex()], dstBuffer.size[dstBuffer.getMetaPlaneIndex()]);

        /* Push frame to scaler Pipe */
        m_gscPipes[i]->setOutputFrameQ(m_gscFrameDoneQ[i]);
        m_gscPipes[i]->pushFrame(frame);
    }

    m_handleGscDoneQ->pushProcessQ(&frame);
    if (m_gscDoneThread->isRunning() == false) {
        m_gscDoneThread->run(m_name);
    }

    return NO_ERROR;
}

bool ExynosCameraPipeSWMCSC::m_checkThreadLoop(frame_queue_t *queue)
{
    Mutex::Autolock lock(m_pipeframeLock);
    bool loop = false;

    if (m_isReprocessing() == false)
        loop = true;

    if (queue->getSizeOfProcessQ() > 0)
        loop = true;

    if (m_flagTryStop == true)
        loop = false;

    return loop;
}

bool ExynosCameraPipeSWMCSC::m_gscDoneThreadFunc(void)
{
    status_t ret = NO_ERROR;

    ret = m_handleGscDoneFrame();
    if (ret != NO_ERROR) {
        if (ret != TIMED_OUT)
            CLOGE("m_putBuffer fail, ret(%d)", ret);

        /* TODO: doing exception handling */
    }

    return m_checkThreadLoop(m_handleGscDoneQ);
}

status_t ExynosCameraPipeSWMCSC::m_handleGscDoneFrame()
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t requestFrame = NULL;
    ExynosCameraFrameSP_sptr_t doneFrame = NULL;
    int base = -1;
    int pipeId = -1;
    int waitCount = 0;
    int perframeInfoIdx = -1;
    ExynosCameraBuffer dstBuffer;
    entity_buffer_state_t dstBufferState = ENTITY_BUFFER_STATE_NOREQ;

    ret = m_handleGscDoneQ->waitAndPopProcessQ(&requestFrame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (requestFrame == NULL) {
        CLOGE("Request Frame is NULL");
        return BAD_VALUE;
    }

    if (m_reprocessing == false) {
        perframeInfoIdx = PERFRAME_INFO_SWMCSC;
        base = PIPE_MCSC0;
    } else {
        perframeInfoIdx = PERFRAME_INFO_REPROCESSING_SWMCSC;
        base = PIPE_MCSC0_REPROCESSING;
    }
    camera2_node_group node_group_info;
    requestFrame->getNodeGroupInfo(&node_group_info, perframeInfoIdx);

    /* Wait and Pop frame from GSC output Q */
    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_gscPipes[i] == NULL)
            continue;

        status_t funcRet = NO_ERROR;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("getPipeId(%d) fail", i);
            return BAD_VALUE;
        }

        /* Get scaler destination buffer */
        //if (requestFrame->getRequest(pipeId) == false) {
        if (requestFrame->getRequest(pipeId) == false && node_group_info.capture[pipeId - base].request == false) {
            continue;
        }

        if ((m_configurations->getOnePortId() > -1) &&
                (pipeId == (m_configurations->getOnePortId() % ExynosCameraParameters::YUV_MAX) + PIPE_MCSC0)) {
            CLOGV("[F%d] pipeId(%d) is not output of SWMCSC", requestFrame->getFrameCount(), pipeId);
            continue;
        }

        CLOGV("wait GSC(%d) output", i);

        waitCount = 0;
        doneFrame = NULL;
        do {
            ret = m_gscFrameDoneQ[i]->waitAndPopProcessQ(&doneFrame);
            waitCount++;

        } while (ret == TIMED_OUT && waitCount < 10);

        if (ret != NO_ERROR)
            CLOGW("GSC wait and pop error, ret(%d)", ret);

        if (doneFrame == NULL) {
            CLOGE("gscFrame is NULL");
            requestFrame->setRequest(pipeId, false);
            continue;
        }

        if (requestFrame->getFrameCount() != doneFrame->getFrameCount()) {
            CLOGW("FrameCount mismatch, Push(%d) Pop(%d)",
                    requestFrame->getFrameCount(), doneFrame->getFrameCount());
        }

        CLOGV("Get frame from GSC Pipe(%d), frameCount(%d)",
                i, doneFrame->getFrameCount());

        /* Check dstBuffer state */
        dstBufferState = ENTITY_BUFFER_STATE_NOREQ;
        ret = requestFrame->getDstBufferState(pipeId, &dstBufferState);
        if (ret != NO_ERROR || dstBufferState == ENTITY_BUFFER_STATE_ERROR) {
            CLOGE("getDstBufferState fail, pipeId(%d), entityState(%d) frame(%d)",
                    pipeId, dstBufferState, requestFrame->getFrameCount());
            requestFrame->setRequest(pipeId, false);
            continue;
        }

        dstBuffer.index = -1;
        ret = requestFrame->getDstBuffer(pipeId, &dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("getDstBuffer fail, pipeId(%d), ret(%d) frame(%d)",
                    pipeId, ret, requestFrame->getFrameCount());
            requestFrame->setRequest(pipeId, false);
            continue;
        }

        if (dstBuffer.index < 0) {
            CLOGE("Invalid Dst buffer index(%d), frame(%d)",
                    dstBuffer.index, requestFrame->getFrameCount());
            requestFrame->setRequest(pipeId, false);
            continue;
        }

        /* Sync dst Node index with MCSC0 */
        funcRet = requestFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
        if (funcRet != NO_ERROR) {
            CLOGE("setdst Buffer state failed(%d) frame(%d)",
                    ret, requestFrame->getFrameCount());
            ret |= funcRet;
        }

        funcRet = requestFrame->setDstBuffer(getPipeId(), dstBuffer, i);
        if (funcRet != NO_ERROR) {
            CLOGE("setdst Buffer failed(%d) frame(%d)",
                    ret, requestFrame->getFrameCount());
            ret |= funcRet;
        }

        funcRet = requestFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE, i);
        if (funcRet != NO_ERROR) {
            CLOGE("setdst Buffer state failed(%d) frame(%d)",
                    ret, requestFrame->getFrameCount());
            ret |= funcRet;
        }

        /* TODO: doing exception handling */
        if (ret != NO_ERROR) {
            /* TODO: doing exception handling */
            if (m_bufferSupplier != NULL) {
                ret = m_bufferSupplier->putBuffer(dstBuffer);
                if (ret != NO_ERROR) {
                    CLOGE("[%s][F%d B%d]Failed to putBuffer. ret %d",
                            m_deviceInfo->nodeName[i],
                            requestFrame->getFrameCount(),
                            dstBuffer.index,
                            ret);
                }
            }

            ret = requestFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
            if (ret != NO_ERROR) {
                CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                        pipeId, requestFrame->getFrameCount(), ret);
            }

            requestFrame->setRequest(pipeId, false);
            continue;
        }

        funcRet = requestFrame->setEntityState(pipeId, ENTITY_STATE_COMPLETE);
        if (funcRet < 0) {
            CLOGE("setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                 pipeId, ENTITY_STATE_COMPLETE, ret);
            ret |= funcRet;
        }
    }

    ret = requestFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret != NO_ERROR) {
        CLOGE("Set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    requestFrame->setMetaDataEnable(true);

    m_outputFrameQ->pushProcessQ(&requestFrame);

    if (m_frameDoneQ != NULL)
        m_frameDoneQ->pushProcessQ(&requestFrame);

    return NO_ERROR;
}

void ExynosCameraPipeSWMCSC::m_init(camera_device_info_t *deviceInfo)
{
    if (deviceInfo != NULL)
        m_deviceInfo = deviceInfo;
    else
        m_deviceInfo = NULL;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        m_gscFrameDoneQ[i] = NULL;
        m_gscPipes[i] = NULL;
    }

    m_handleGscDoneQ = NULL;

}

int ExynosCameraPipeSWMCSC::getPipeId(enum NODE_TYPE nodeType)
{
    if (nodeType < OUTPUT_NODE || MAX_NODE <= nodeType) {
        CLOGE("Invalid nodeType(%d). so, fail", nodeType);
        return -1;
    }

    return m_deviceInfo->pipeId[nodeType];
}

uint32_t ExynosCameraPipeSWMCSC::getPipeId(void)
{
    return (uint32_t)this->getPipeId(OUTPUT_NODE);
}

}; /* namespace android */
