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
#define LOG_TAG "ExynosCameraPipeMultipleJpeg"
#include "ExynosCameraPipeMultipleJpeg.h"

namespace android {

ExynosCameraPipeMultipleJpeg::~ExynosCameraPipeMultipleJpeg()
{
    this->destroy();
}

status_t ExynosCameraPipeMultipleJpeg::create(__unused int32_t *sensorIds)
{
    status_t ret = NO_ERROR;

    ret = ExynosCameraSWPipe::create(sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("SWPipe creation fail! ret %d", ret);
        return ret;
    }

    for (int nodeType = CAPTURE_NODE, pipeIndex = 0; nodeType < MAX_CAPTURE_NODE; nodeType++) {
        if (m_deviceInfo->pipeId[nodeType] <= -1) {
            continue;
        }

        if (pipeIndex >= m_maxJpegPipeNum) {
            CLOGE("Internal Jpeg Pipe is over. pipeIndex(%d), MAX jpeg pipe Num(%d)",
                    pipeIndex, m_maxJpegPipeNum);
            return BAD_VALUE;
        }

        m_jpegPipes[pipeIndex] = new ExynosCameraPipeJpeg(m_cameraId, m_configurations, m_parameters, m_reprocessing, nullptr, m_camIdInfo);
        m_jpegPipes[pipeIndex]->setPipeId(m_deviceInfo->pipeId[nodeType]);
        m_jpegPipes[pipeIndex]->setPipeName(m_deviceInfo->secondaryNodeName[nodeType]);

        ret = m_jpegPipes[pipeIndex]->create(sensorIds);
        if (ret != NO_ERROR) {
            CLOGE("Internal Jpeg Pipe(%d) creation fail! ret %d",
                    m_deviceInfo->pipeId[nodeType], ret);
            return ret;
        }
        m_jpegFrameDoneQ[pipeIndex] = new frame_queue_t(m_jpegDoneThread);
        pipeIndex++;
    }

    m_handleJpegDoneQ = new frame_queue_t(m_jpegDoneThread);
    m_jpegDoneThread = new ExynosCameraThread<ExynosCameraPipeMultipleJpeg>(this, &ExynosCameraPipeMultipleJpeg::m_jpegDoneThreadFunc, "jpegDoneThread");

    return NO_ERROR;
}

status_t ExynosCameraPipeMultipleJpeg::destroy(void)
{
    status_t ret = NO_ERROR;

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL)
            continue;

        ret = m_jpegPipes[i]->destroy();
        if (ret != NO_ERROR)
            CLOGE("Internal Jpeg Pipe(%d) detstroy fail!", i);

        SAFE_DELETE(m_jpegFrameDoneQ[i]);
        SAFE_DELETE(m_jpegPipes[i]);
    }

    SAFE_DELETE(m_handleJpegDoneQ);
    m_jpegDoneThread = NULL;

    ExynosCameraSWPipe::m_destroy();

    return NO_ERROR;
}

status_t ExynosCameraPipeMultipleJpeg::stop(void)
{
    status_t ret = NO_ERROR;

    m_flagTryStop = true;

    stopThreadAndInputQ(m_mainThread, 1, m_inputFrameQ);
    stopThreadAndInputQ(m_jpegDoneThread, 1, m_handleJpegDoneQ);

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL)
            continue;

        ret = m_jpegPipes[i]->stop();
        if (ret != NO_ERROR)
            CLOGE("Internal Jpeg Pipe stop fail!");

        m_jpegFrameDoneQ[i]->release();
    }

    m_flagTryStop = false;

    CLOGI("stop() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraPipeMultipleJpeg::m_checkValidBuffers(ExynosCameraFrameSP_sptr_t frame,
                                                                ExynosCameraBuffer (&srcBuffers)[m_maxJpegPipeNum],
                                                                ExynosCameraBuffer (&dstBuffers)[m_maxJpegPipeNum])
{
    int validSrcBufCnt = 0, validDstBufCnt = 0;
    int numOfSrcBuffer = frame->getNumOfSrcBuffer(getPipeId());
    int numOfDstBuffer = frame->getNumOfDstBuffer(getPipeId());

    if ((numOfSrcBuffer != numOfDstBuffer)
        || (numOfSrcBuffer > m_maxJpegPipeNum)
        || (numOfDstBuffer > m_maxJpegPipeNum)) {
        CLOGE("invalid buffer number. numOfSrcBuffer(%d), numOfDstBuffer(%d), numOfMaxJpeg(%d).",
                numOfSrcBuffer, numOfDstBuffer, m_maxJpegPipeNum);
        return BAD_VALUE;
    }

    for (int nodeIndex = OUTPUT_NODE; nodeIndex < MAX_OUTPUT_NODE; nodeIndex++) {
        ExynosCameraBuffer buffer;
        status_t ret = frame->getSrcBuffer(getPipeId(), &buffer, nodeIndex);
        if (ret != NO_ERROR || buffer.index < 0) {
            CLOGV("getSrcBuffer fail, ret(%d)", ret);
            continue;
        }

        entity_buffer_state_t srcBufferState;
        ret = frame->getSrcBufferState(getPipeId(), &srcBufferState, nodeIndex);
        if (srcBufferState == ENTITY_BUFFER_STATE_ERROR
            && frame->getFrameType() != FRAME_TYPE_INTERNAL) {
            CLOGE("Invalid SrcBuffer(index:%d, state:%d)",
                    buffer.index, srcBufferState);
            continue;
        }

        srcBuffers[validSrcBufCnt++] = buffer;
        if (validSrcBufCnt == numOfSrcBuffer) {
            break;
        }
    }

    for (int nodeIndex = OUTPUT_NODE; nodeIndex < MAX_CAPTURE_NODE; nodeIndex++) {
        ExynosCameraBuffer buffer;
        status_t ret = frame->getDstBuffer(getPipeId(), &buffer, nodeIndex);
        if (ret != NO_ERROR || buffer.index < 0) {
            CLOGV("getDstBuffer fail, ret(%d)", ret);
            continue;
        }

        entity_buffer_state_t dstBufferState;
        ret = frame->getDstBufferState(getPipeId(), &dstBufferState, nodeIndex);
        if (dstBufferState == ENTITY_BUFFER_STATE_ERROR
            && frame->getFrameType() != FRAME_TYPE_INTERNAL) {
            CLOGE("Invalid DstBuffer(index:%d, state:%d)",
                    buffer.index, dstBufferState);
            continue;
        }

        dstBuffers[validDstBufCnt++] = buffer;
        if (validDstBufCnt == numOfDstBuffer) {
            break;
        }
    }

    if ((validSrcBufCnt != numOfSrcBuffer) || (validDstBufCnt != numOfDstBuffer)) {
        CLOGE("validSrcBufCnt(%d), validDstBufCnt(%d), numOfSrcBuffer(%d), numOfDstBuffer(%d)",
                validSrcBufCnt, validDstBufCnt, numOfSrcBuffer, numOfDstBuffer);
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeMultipleJpeg::m_run(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    ExynosCameraBuffer srcBuffers[m_maxJpegPipeNum], dstBuffers[m_maxJpegPipeNum];

    ret = m_inputFrameQ->waitAndPopProcessQ(&frame);
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

    if (frame == NULL) {
        CLOGE("new frame is NULL");
        return BAD_VALUE;
    }

    /* Check buffer state */

    if (frame->getFrameState() == FRAME_STATE_SKIPPED
        || frame->getFrameState() == FRAME_STATE_INVALID) {
        if (frame->getFrameType() != FRAME_TYPE_INTERNAL) {
            CLOGE("New frame is INVALID, frameCount(%d)",
                     frame->getFrameCount());
        }
        goto FUNC_EXIT;
    }

    ret = m_checkValidBuffers(frame, srcBuffers, dstBuffers);
    if (ret != NO_ERROR) {
        CLOGE("[F%d] fail to m_checkValidMultipleBuffers. ret = %d",
                frame->getFrameCount(), ret);
        goto FUNC_EXIT;
    }

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL) {
            continue;
        }

        int jpegPipeId = m_jpegPipes[i]->getPipeId();
        if ((srcBuffers[i].index > -1) && (dstBuffers[i].index > -1)) {
            frame->setRequest(jpegPipeId, true);
        } else {
            frame->setRequest(jpegPipeId, false);
        }
    }

    m_runJpegs(frame, srcBuffers, dstBuffers);

    return NO_ERROR;

FUNC_EXIT:
    if (frame != NULL) {
        ret = frame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
        if (ret != NO_ERROR) {
            CLOGE("set entity state fail, ret(%d)", ret);
            /* TODO: doing exception handling */
            return INVALID_OPERATION;
        }

        for (int i = 0; i < m_maxJpegPipeNum; i++) {
            if (m_jpegPipes[i] != NULL) {
                ret = frame->setEntityState(m_jpegPipes[i]->getPipeId(), ENTITY_STATE_FRAME_DONE);
                if (ret != NO_ERROR) {
                    CLOGE("set entity state fail, ret(%d)", ret);
                    /* TODO: doing exception handling */
                    return INVALID_OPERATION;
                }
            }
        }
        m_outputFrameQ->pushProcessQ(&frame);
    }

    return NO_ERROR;
}

void ExynosCameraPipeMultipleJpeg::m_runJpegs(ExynosCameraFrameSP_sptr_t frame,
                                                    ExynosCameraBuffer srcBuffers[m_maxJpegPipeNum],
                                                    ExynosCameraBuffer dstBuffers[m_maxJpegPipeNum])
{
    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL) {
            continue;
        }

        int jpegPipeId = m_jpegPipes[i]->getPipeId();
        if (frame->getRequest(jpegPipeId) == false) {
            continue;
        }

        if (srcBuffers[i].index <= -1 || dstBuffers[i].index <= -1) {
            CLOGE("[F%d] skip to push Jpeg. srcBuffers[%d].index = %d, dstBuffers[%d].index = %d",
                    frame->getFrameCount(), i, srcBuffers[i].index, i, dstBuffers[i].index);
            continue;
        }

        status_t ret = frame->setSrcBuffer(jpegPipeId, srcBuffers[i]);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to setSrcBuffer. pipeId %d. ret %d",
                    frame->getFrameCount(), srcBuffers[i].index,
                    jpegPipeId, ret);
            continue;
        }

        ret = frame->setDstBuffer(jpegPipeId, dstBuffers[i]);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to setDstBuffer. pipeId %d. ret %d",
                    frame->getFrameCount(), dstBuffers[i].index,
                    jpegPipeId, ret);
            continue;
        }

        m_jpegPipes[i]->setOutputFrameQ(m_jpegFrameDoneQ[i]);
        m_jpegPipes[i]->pushFrame(frame);
    }

    m_handleJpegDoneQ->pushProcessQ(&frame);
    if (m_jpegDoneThread->isRunning() == false) {
        m_jpegDoneThread->run(m_name);
    }
}

bool ExynosCameraPipeMultipleJpeg::m_checkThreadLoop(frame_queue_t *queue)
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

bool ExynosCameraPipeMultipleJpeg::m_jpegDoneThreadFunc(void)
{
    status_t ret = NO_ERROR;

    ret = m_handleJpegDoneFrame();
    if (ret != NO_ERROR) {
        if (ret != TIMED_OUT)
            CLOGE("m_putBuffer fail, ret(%d)", ret);

        /* TODO: doing exception handling */
    }

    return m_checkThreadLoop(m_handleJpegDoneQ);
}

status_t ExynosCameraPipeMultipleJpeg::m_handleJpegDoneFrame()
{
    ExynosCameraFrameSP_sptr_t requestFrame = NULL;

    status_t ret = m_handleJpegDoneQ->waitAndPopProcessQ(&requestFrame);
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

    /* Wait and Pop frame from GSC output Q */
    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL) {
            continue;
        }

        int jpegPipeId = m_jpegPipes[i]->getPipeId();
        if (requestFrame->getRequest(jpegPipeId) == false) {
            continue;
        }

        CLOGD("wait JPEG(%d) output", i);

        ExynosCameraFrameSP_sptr_t doneFrame = NULL;
        int waitCount = 0;
        do {
            ret = m_jpegFrameDoneQ[i]->waitAndPopProcessQ(&doneFrame);
            waitCount++;

        } while (ret == TIMED_OUT && waitCount < 10);

        if (ret != NO_ERROR)
            CLOGW("JPEG wait and pop error, waitCount(%d) ret(%d)", waitCount, ret);

        if (doneFrame == NULL) {
            CLOGE("JPEG frame is NULL");
            continue;
        }

        if (requestFrame->getFrameCount() != doneFrame->getFrameCount()) {
            CLOGW("FrameCount mismatch, Push(%d) Pop(%d)",
                    requestFrame->getFrameCount(), doneFrame->getFrameCount());
        }

        CLOGD("[F%d] Get frame from JPEG Pipe(%d)", doneFrame->getFrameCount(), jpegPipeId);
    }

    ret = requestFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret != NO_ERROR) {
        CLOGE("Set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    m_outputFrameQ->pushProcessQ(&requestFrame);

    return NO_ERROR;
}

void ExynosCameraPipeMultipleJpeg::m_init(camera_device_info_t *deviceInfo)
{
    m_reprocessing = true;
    m_deviceInfo = (deviceInfo != NULL) ? deviceInfo : NULL;
    m_handleJpegDoneQ = NULL;

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        m_jpegFrameDoneQ[i] = NULL;
    }

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        m_jpegPipes[i] = NULL;
    }
}

status_t ExynosCameraPipeMultipleJpeg::setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier)
{
    status_t ret = NO_ERROR;

    m_bufferSupplier = bufferSupplier;

    for (int i = 0; i < m_maxJpegPipeNum; i++) {
        if (m_jpegPipes[i] == NULL)
            continue;

        ret = m_jpegPipes[i]->setBufferSupplier(bufferSupplier);
        if (ret != NO_ERROR)
            CLOGE("Internal Jpeg Pipe(%d) detstroy fail!", i);
    }

    return NO_ERROR;
}

}; /* namespace android */
