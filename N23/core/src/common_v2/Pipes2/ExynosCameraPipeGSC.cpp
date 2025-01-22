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
#define LOG_TAG "ExynosCameraPipeGSC"

#include "ExynosCameraPipeGSC.h"

namespace android {

status_t ExynosCameraPipeGSC::create(__unused int32_t *sensorIds)
{
    if (m_gscWrapper->create() != NO_ERROR) {
        CLOGE("gscWrapper create fail!");
        return INVALID_OPERATION;
    }

    ExynosCameraSWPipe::create(sensorIds);

    return NO_ERROR;
}

status_t ExynosCameraPipeGSC::m_destroy(void)
{
    m_gscWrapper->destroy();
    m_gscWrapper = nullptr;

    ExynosCameraSWPipe::m_destroy();

    return NO_ERROR;
}

status_t ExynosCameraPipeGSC::m_run(void)
{
    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    ExynosRect srcRect;
    ExynosRect dstRect;
    ExynosCameraFrameEntity *entity = NULL;

    int ret = 0;
    int rotation = 0;
    int flipHorizontal = 0;
    int flipVertical = 0;

    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
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
        return NO_ERROR;
    }

    entity = newFrame->searchEntityByPipeId(getPipeId());
    if (entity == NULL || entity->getSrcBufState() == ENTITY_BUFFER_STATE_ERROR) {
        CLOGE("frame(%d) entityState(ENTITY_BUFFER_STATE_ERROR), skip msc", newFrame->getFrameCount());
        goto func_exit;
    }

    rotation = newFrame->getRotation(getPipeId());
    CLOGV(" getPipeId(%d), rotation(%d)", getPipeId(), rotation);

    flipHorizontal = newFrame->getFlipHorizontal(getPipeId());
    flipVertical = newFrame->getFlipVertical(getPipeId());

    ret = newFrame->getSrcRect(getPipeId(), &srcRect);
    ret = newFrame->getDstRect(getPipeId(), &dstRect);

    switch (srcRect.colorFormat) {
    case V4L2_PIX_FMT_NV21:
        srcRect.fullH = ALIGN_UP(srcRect.fullH, 2);
        break;
    default:
        srcRect.fullH = ALIGN_UP(srcRect.fullH, GSCALER_IMG_ALIGN);
        break;
    }

    ret = newFrame->getSrcBuffer(getPipeId(), &srcBuffer);
    if (ret < 0) {
        CLOGE("frame get src buffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return OK;
    }

    ret = newFrame->getDstBuffer(getPipeId(), &dstBuffer);
    if (ret < 0) {
        CLOGE("frame get dst buffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return OK;
    }

    ret = m_gscWrapper->convertWithRotation(srcRect, dstRect, srcBuffer, dstBuffer, rotation, flipHorizontal, flipVertical);
    if (ret != NO_ERROR) {
        CLOGE("convertWithRoration fail, ret(%d)", ret);
        newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, 0);
    }

    CLOGV("Rotation(%d), flip horizontal(%d), vertical(%d)",
             rotation, flipHorizontal, flipVertical);

    CLOGV("CSC(%d) converting done", m_gscNum);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return OK;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

    return NO_ERROR;

func_exit:

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return OK;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);
    return NO_ERROR;
}

void ExynosCameraPipeGSC::m_init(int32_t *nodeNums)
{
    if (nodeNums == NULL)
        m_gscNum = -1;
    else
        m_gscNum = nodeNums[0];

    m_gscWrapper = new ExynosCameraGSCWrapper(m_gscNum);
    if (m_gscWrapper == nullptr) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Fail to create new gscWrapper instance. so, assert!!!!",
                    __FUNCTION__, __LINE__);
    }
}

}; /* namespace android */
