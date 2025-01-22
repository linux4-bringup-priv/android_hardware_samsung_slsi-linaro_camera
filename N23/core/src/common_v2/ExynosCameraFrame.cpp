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
#define LOG_TAG "ExynosCameraFrame"
#include <log/log.h>

#include "ExynosCameraFrame.h"

namespace android {

#ifdef DEBUG_FRAME_MEMORY_LEAK
unsigned long long ExynosCameraFrame::m_checkLeakCount;
unsigned long long ExynosCameraFrame::m_checkLeakFrameCount;
Mutex ExynosCameraFrame::m_countLock;
#endif

ExynosCameraFrame::ExynosCameraFrame(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *params,
        uint32_t frameCount,
        uint32_t frameType)
{
    m_cameraId = cameraId;
    m_cameraIds[OUTPUT_NODE] = cameraId;
    m_configurations = configurations;
    m_parameters = params;
    m_frameCount = frameCount;
    m_frameType = frameType;

    CLOGV(" create frame type(%d), frameCount(%d)", frameType, frameCount);

    m_init();
}

ExynosCameraFrame::ExynosCameraFrame(int cameraId)
{
    m_cameraId = cameraId;
    m_cameraIds[OUTPUT_NODE] = cameraId;
    m_configurations = NULL;
    m_parameters = NULL;
    m_frameCount = 0;
    m_frameType = FRAME_TYPE_OTHERS;

    m_init();
}

ExynosCameraFrame::~ExynosCameraFrame()
{
    m_deinit();
}

#ifdef DEBUG_FRAME_MEMORY_LEAK
long long int ExynosCameraFrame::getCheckLeakCount()
{
    return m_privateCheckLeakCount;
}
#endif

status_t ExynosCameraFrame::addSiblingEntity(
        __unused ExynosCameraFrameEntity *curEntity,
        ExynosCameraFrameEntity *newEntity)
{
    Mutex::Autolock l(m_linkageLock);
    m_linkageList.push_back(newEntity);

    return NO_ERROR;
}

status_t ExynosCameraFrame::addChildEntity(
        ExynosCameraFrameEntity *parentEntity,
        ExynosCameraFrameEntity *newEntity)
{
    status_t ret = NO_ERROR;

    if (parentEntity == NULL) {
        CLOGE("parentEntity is NULL");
        return BAD_VALUE;
    }

    /* TODO: This is not suit in case of newEntity->next != NULL */
    ExynosCameraFrameEntity *tmpEntity;

    tmpEntity = parentEntity->getNextEntity();
    ret = parentEntity->setNextEntity(newEntity);
    if (ret != NO_ERROR) {
        CLOGE("setNextEntity fail, ret(%d)", ret);
        return ret;
    }
    newEntity->setNextEntity(tmpEntity);

    return ret;
}

status_t ExynosCameraFrame::addChildEntity(
        ExynosCameraFrameEntity *parentEntity,
        ExynosCameraFrameEntity *newEntity,
        int parentPipeId)
{
    status_t ret = NO_ERROR;

    if (parentEntity == NULL) {
        CLOGE("parentEntity is NULL");
        return BAD_VALUE;
    }

    /* TODO: This is not suit in case of newEntity->next != NULL */
    ExynosCameraFrameEntity *tmpEntity;

    tmpEntity = parentEntity->getNextEntity();
    ret = parentEntity->setNextEntity(newEntity);
    if (ret != NO_ERROR) {
        CLOGE("setNextEntity fail, ret(%d)", ret);
        return ret;
    }

    if (0 <= parentPipeId) {
        ret = newEntity->setParentPipeId((enum pipeline)parentPipeId);
        if (ret != NO_ERROR) {
            CLOGE("setParentPipeId(%d) fail, ret(%d)", parentPipeId, ret);
            return ret;
        }
    } else {
        CLOGW("parentPipeId(%d) < 0. you may set parentPipeId which connect between parent(%d) and child(%d)",
             parentPipeId, parentEntity->getPipeId(), newEntity->getPipeId());
    }

    newEntity->setNextEntity(tmpEntity);
    return ret;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFirstEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *firstEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        firstEntity = NULL;
        return firstEntity;
    }

    r = m_linkageList.begin()++;
    m_currentEntity = r;
    firstEntity = *r;

    return firstEntity;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getNextEntity(void)
{
    ExynosCameraFrameEntity *nextEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    m_currentEntity++;

    if (m_currentEntity == m_linkageList.end()) {
        return nextEntity;
    }

    nextEntity = *m_currentEntity;

    return nextEntity;
}
/* Unused, but useful */
/*
ExynosCameraFrameEntity *ExynosCameraFrame::getChildEntity(ExynosCameraFrameEntity *parentEntity)
{
    ExynosCameraFrameEntity *childEntity = NULL;

    if (parentEntity == NULL) {
        CLOGE("parentEntity is NULL");
        return childEntity;
    }

    childEntity = parentEntity->getNextEntity();

    return childEntity;
}
*/

ExynosCameraFrameEntity *ExynosCameraFrame::searchEntityByPipeId(uint32_t pipeId)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            CLOGE("curEntity is NULL, index(%d), linkageList size(%d)",
                 i, listSize);
            return NULL;
        }

        while (curEntity != NULL) {
            if (curEntity->getPipeId() == pipeId)
                return curEntity;
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    CLOGD("Cannot find matched entity, frameCount(%d), pipeId(%d), type(%d)", getFrameCount(), pipeId, getFrameType());

    return NULL;
}

status_t ExynosCameraFrame::setSrcBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer srcBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcBuf(srcBuf, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set src buffer, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer dstBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set dst buffer, ret(%d)", ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcBuf(dstBuf);
        if (ret != NO_ERROR) {
            CLOGE("Could not set dst buffer, ret(%d)", ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::setDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer dstBuf,
                                         uint32_t nodeIndex,
                                         int      parentPipeId)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set dst buffer, ret(%d)", ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        bool flagSetChildEntity = false;

        /*
         * it  will set child's source buffer
         * when no specific parent set. (for backward compatibility)
         * when specific parent only. (for MCPipe)
         */
        if (entity->flagSpecficParent() == true) {
            if (parentPipeId == entity->getParentPipeId()) {
                flagSetChildEntity = true;
            } else {
                CLOGV("parentPipeId(%d) != entity->getParentPipeId()(%d). so skip setting child src Buf",
                     parentPipeId, entity->getParentPipeId());
            }
        } else {
            /* this is for backward compatiblity */
            flagSetChildEntity = true;
        }

        /* child mode need to setting next */
        if (flagSetChildEntity == true) {
            ret = entity->setSrcBuf(dstBuf);
            if (ret != NO_ERROR) {
                CLOGE("Could not set dst buffer, ret(%d)", ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer *srcBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->getSrcBuf(srcBuf, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not get src buffer, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer *dstBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->getDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not get dst buffer, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setSrcRect(uint32_t pipeId, ExynosRect srcRect, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcRect(srcRect, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set src rect, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setDstRect(uint32_t pipeId, ExynosRect dstRect, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstRect(dstRect, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set dst rect, ret(%d)", ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcRect(dstRect, nodeIndex);
        if (ret != NO_ERROR) {
            CLOGE("Could not set dst rect, ret(%d)", ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcRect(uint32_t pipeId, ExynosRect *srcRect, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->getSrcRect(srcRect, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not get src rect, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getDstRect(uint32_t pipeId, ExynosRect *dstRect, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->getDstRect(dstRect, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not get dst rect, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t *state,
                                         uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    *state = entity->getSrcBufState(nodeIndex);

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t *state,
                                         uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    *state = entity->getDstBufState(nodeIndex);

    return NO_ERROR;
}

status_t ExynosCameraFrame::setSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcBufState(state, nodeIndex);

    return ret;
}

status_t ExynosCameraFrame::setDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBufState(state, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Could not set dst buffer, ret(%d)", ret);
        return ret;
    }

    /* Set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcBufState(state);
        if (ret != NO_ERROR) {
            CLOGE("Could not set src buffer, ret(%d)", ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::ensureSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    entity_buffer_state_t curState;

    do {
        ret = getSrcBufferState(pipeId, &curState);
        if (ret != NO_ERROR)
            continue;

        if (state == curState) {
            ret = OK;
            break;
        } else {
            ret = BAD_VALUE;
            usleep(100);
        }

        retry++;
        if (retry == 10)
            ret = TIMED_OUT;
    } while (ret != OK && retry < 100);

    CLOGV(" retry count %d", retry);

    return ret;
}

status_t ExynosCameraFrame::ensureDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    entity_buffer_state_t curState;

    do {
        ret = getDstBufferState(pipeId, &curState);
        if (ret != NO_ERROR)
            continue;

        if (state == curState) {
            ret = OK;
            break;
        } else {
            ret = BAD_VALUE;
            usleep(100);
        }

        retry++;
        if (retry == 10)
            ret = TIMED_OUT;
    } while (ret != OK && retry < 100);

    CLOGV(" retry count %d", retry);

    return ret;
}

int ExynosCameraFrame::getNumOfSrcBuffer(uint32_t pipeId)
{
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return -1;
    }

    return entity->getNumOfSrcBuf();
}

int ExynosCameraFrame::getNumOfDstBuffer(uint32_t pipeId)
{
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return -1;
    }

    return entity->getNumOfDstBuf();
}

status_t ExynosCameraFrame::setEntityState(uint32_t pipeId,
                                           entity_state_t state)
{
    Mutex::Autolock lock(m_entityLock);
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    if (entity->getEntityState() == ENTITY_STATE_COMPLETE &&
        state != ENTITY_STATE_REWORK) {
        return NO_ERROR;
    }

    if (state == ENTITY_STATE_COMPLETE) {
        m_updateStatusForResultUpdate((enum pipeline)pipeId);

        m_numCompletePipe++;
        if (m_numCompletePipe >= m_numRequestPipe)
            setFrameState(FRAME_STATE_COMPLETE);
    }

    entity->setEntityState(state);

    return NO_ERROR;
}

status_t ExynosCameraFrame::getEntityState(uint32_t pipeId,
                                           entity_state_t *state)
{
    Mutex::Autolock lock(m_entityLock);
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    *state = entity->getEntityState();
    return NO_ERROR;
}

status_t ExynosCameraFrame::getEntityBufferType(uint32_t pipeId,
                                                entity_buffer_type_t *type)
{
    Mutex::Autolock lock(m_entityLock);
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    *type = entity->getBufType();
    return NO_ERROR;
}

void ExynosCameraFrame::setParameters(ExynosCameraParameters *params)
{
    m_parameters = params;
}

ExynosCameraParameters * ExynosCameraFrame::getParameters()
{
    return m_parameters;
}

uint32_t ExynosCameraFrame::getFrameCount(void)
{
    return m_frameCount;
}

int32_t ExynosCameraFrame::getMetaFrameCount(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return 0;
    }

    return getMetaDmRequestFrameCount(&m_metaData[srcNodeIndex]);
}

status_t ExynosCameraFrame::setNumRequestPipe(uint32_t num)
{
    m_numRequestPipe = num;
    return NO_ERROR;
}

uint32_t ExynosCameraFrame::getNumRequestPipe(void)
{
    return m_numRequestPipe;
}

uint32_t ExynosCameraFrame::getNumRemainPipe(void)
{
    int numRemainPipe = m_numRequestPipe - m_numCompletePipe;

    return (numRemainPipe > 0) ? numRemainPipe : 0;
}

bool ExynosCameraFrame::isComplete(void)
{
    return checkFrameState(FRAME_STATE_COMPLETE);
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_REWORK:
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneEntity(uint32_t pipeID)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL && pipeID == curEntity->getPipeId()) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_REWORK:
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneFirstEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_REWORK:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            case ENTITY_STATE_FRAME_DONE:
                return curEntity;
                break;
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_COMPLETE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneFirstEntity(uint32_t pipeID)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_REWORK:
                if (curEntity->getPipeId() == pipeID)
                    return curEntity;

                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getPipeId() == pipeID)
                    return curEntity;

                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_COMPLETE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFirstEntityNotComplete(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_COMPLETE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            default:
                return curEntity;
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

status_t ExynosCameraFrame::skipFrame(void)
{
    Mutex::Autolock lock(m_frameStateLock);
    m_frameState = FRAME_STATE_SKIPPED;

    return NO_ERROR;
}

bool ExynosCameraFrame::isSkipPreview(void)
{
    bool ret = false;

    ////////////////////////////////////////////////
    // get condition for skip preview
    const struct camera2_shot_ext *shot_ext = this->getConstMeta();

    if (shot_ext) {
#ifdef SKIP_HDR_PREVIEW
        if (shot_ext->shot.dm.aa.vendor_previewSkipFrame == 1) {
            ret = true;
        }
#endif
    }

    ////////////////////////////////////////////////
    // log
    if (ret == true) {
        CLOGD("[%d][FRM:%d(CAM:%d,DRV:%d,T:%d)][REQ:%d]:skip preview frame",
            this->getFactoryType(),
            this->getFrameCount(),
            this->getCameraId(),
            this->getMetaFrameCount(),
            this->getFrameType(),
            this->getRequestKey());
    }

    ////////////////////////////////////////////////

    return ret;
}

void ExynosCameraFrame::setFrameState(frame_status_t state)
{
    Mutex::Autolock lock(m_frameStateLock);

#ifdef USE_DEBUG_PROPERTY
    if (state == FRAME_STATE_COMPLETE && state != m_frameState) {
        m_completeTimestamp = systemTime(SYSTEM_TIME_BOOTTIME);

        CLOG_PERFORMANCE(FPS, getCameraId(),
                getFactoryType(), DURATION,
                FRAME_COMPLETE, 0, getRequestKey(), this);
    }
    m_previousFrameState = m_frameState;
#endif

    // To get tid who changed this frame state to SKIPPED
    if (m_frameState != FRAME_STATE_SKIPPED &&
            state == FRAME_STATE_SKIPPED)
        CLOGW("[F%d,T%d] %d -> %d", m_frameCount, m_frameType, m_frameState, state);

    /* TODO: We need state machine */
    if (state > FRAME_STATE_INVALID)
        m_frameState = FRAME_STATE_INVALID;
    else
        m_frameState = state;
}

frame_status_t ExynosCameraFrame::getFrameState(void)
{
    Mutex::Autolock lock(m_frameStateLock);
    return m_frameState;
}

bool ExynosCameraFrame::checkFrameState(frame_status_t state)
{
    Mutex::Autolock lock(m_frameStateLock);
    return (m_frameState == state) ? true : false;
}

#ifdef USE_DEBUG_PROPERTY
void ExynosCameraFrame::getEntityInfoStr(String8 &str)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) return;

    str.append("Entity[");
    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) return;

        str.appendFormat("(P%d(%d))",
             curEntity->getPipeId(), curEntity->getEntityState());

        curEntity = curEntity->getNextEntity();
        while (curEntity != NULL) {
            str.appendFormat("~>(P%d(%d))",
                    curEntity->getPipeId(), curEntity->getEntityState());
            curEntity = curEntity->getNextEntity();
        }

        if ((i + 1) < listSize) str.append("->");
        r++;
    }
    str.append("]");

    return;
}
#endif

void ExynosCameraFrame::printEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    CLOGD(" FrameCount(%d(%d)), type(%d), state(%d), request(%d), complete(%d)",
            getFrameCount(), getMetaFrameCount(), getFrameType(), getFrameState(),
            m_numRequestPipe, m_numCompletePipe);

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            CLOGE("curEntity is NULL, index(%d)", i);
            return;
        }

        int pipeId = curEntity->getPipeId();
        CLOGD("[Req%d] sibling id(%d), state(%d)",
                getRequest(pipeId), pipeId, curEntity->getEntityState());

        curEntity = curEntity->getNextEntity();
        while (curEntity != NULL) {
            pipeId = curEntity->getPipeId();
            CLOGD("----- [Req%d] Child id(%d), state(%d)",
                    getRequest(pipeId), pipeId, curEntity->getEntityState());
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    return;
}

void ExynosCameraFrame::printNotDoneEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        CLOGE("m_linkageList is empty");
        return;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    CLOGD(" FrameCount(%d(%d)), type(%d), state(%d), request(%d), complete(%d)",
             getFrameCount(), getMetaFrameCount(), getFrameType(), getFrameState(),
             m_numRequestPipe, m_numCompletePipe);

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            CLOGE("curEntity is NULL, index(%d)", i);
            return;
        }

        int pipeId = curEntity->getPipeId();
        if (curEntity->getEntityState() != ENTITY_STATE_COMPLETE) {
            CLOGD("[Req:%d] sibling id(%d), state(%d)",
                    getRequest(pipeId), pipeId, curEntity->getEntityState());
        }

        curEntity = curEntity->getNextEntity();
        while (curEntity != NULL) {
            pipeId = curEntity->getPipeId();
            if (curEntity->getEntityState() != ENTITY_STATE_COMPLETE) {
                CLOGD("----- [Req:%d] Child id(%d), state(%d)",
                        getRequest(pipeId), pipeId, curEntity->getEntityState());
            }
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    return;
}

void ExynosCameraFrame::dump(void)
{
    printEntity();

    for (int i = 0; i < MAX_NUM_PIPES; i ++) {
        if (m_request[INDEX(i)] == true)
            CLOGI("pipeId(%d)'s request is ture", i);
    }
}

void ExynosCameraFrame::frameLock(void)
{
    m_frameLocked = true;
}

void ExynosCameraFrame::frameUnlock(void)
{
    m_frameLocked = false;
}

bool ExynosCameraFrame::getFrameLockState(void)
{
    return m_frameLocked;
}

status_t ExynosCameraFrame::initMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    status_t ret = NO_ERROR;

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    if (shot != NULL) {
        CLOGV(" initialize shot_ext");
        memcpy(&m_metaData[srcNodeIndex], shot, sizeof(struct camera2_shot_ext));
    }

    if (m_parameters != NULL) {
        ret = m_parameters->duplicateCtrlMetadata(&m_metaData[srcNodeIndex]);
        if (ret != NO_ERROR) {
            CLOGE("duplicate Ctrl metadata fail");
            return INVALID_OPERATION;
        }
    }

    return ret;
}

/* for const read operation without memcpy */
const struct camera2_shot_ext *ExynosCameraFrame::getConstMeta(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return &m_metaData[0];
    }

    return &m_metaData[srcNodeIndex];
}

status_t ExynosCameraFrame::getMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    char *srcAddr = NULL;
    char *dstAddr = NULL;
    struct camera2_entry_ctl vendorEntry;
    size_t size = 0;

    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    /* camera2_entry_ctl back-up */
    memcpy(&vendorEntry, &shot->shot.ctl.vendor_entry, sizeof(camera2_entry_ctl));

    /* camera2_stream region should not be overwritten */
    srcAddr = ((char *) &m_metaData[srcNodeIndex])  + sizeof(struct camera2_stream);
    dstAddr = ((char *) shot)                       + sizeof(struct camera2_stream);
    size    = sizeof(struct camera2_shot_ext)       - sizeof(struct camera2_stream);
    memcpy(dstAddr, srcAddr, size);
    memcpy(&shot->shot.ctl.vendor_entry, &vendorEntry, sizeof(camera2_entry_ctl));

    return NO_ERROR;
}

status_t ExynosCameraFrame::setMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }


    memcpy(&m_metaData[srcNodeIndex], shot, sizeof(struct camera2_shot_ext));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeShotExtMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    ////////////////////////////////////////////////
    // you can add the field of camera2_shot_ext on here.
    for (int i = 0; i < MCSC_PORT_MAX; i++) {
        m_metaData[srcNodeIndex].mcsc_flip[i]        = shot->mcsc_flip[i];
        m_metaData[srcNodeIndex].mcsc_flip_result[i] = shot->mcsc_flip_result[i];
    }

    // this is move in ExynosCameraPipeVRA.cpp. so, comment out here.
    //memcpy(m_metaData[srcNodeIndex].vra_ext, shot->vra_ext, sizeof(struct vra_ext_meta);

    m_metaData[srcNodeIndex].thermal = shot->thermal;
    memcpy(&m_metaData[srcNodeIndex].user, &shot->user, sizeof(struct camera2_shot_ext_user));

    ////////////////////////////////////////////////

    return NO_ERROR;
}

status_t ExynosCameraFrame::setNFDInfo(struct fd_info *fdInfo)
{
    if(fdInfo == NULL){
        CLOGE(" m_fdInfo is NULL ");
        return BAD_VALUE;
    }
    memcpy(&m_fdInfo, fdInfo, sizeof(struct fd_info));
    return NO_ERROR;
}

status_t ExynosCameraFrame::getNFDInfo(struct fd_info *fdInfo)
{
    memcpy(fdInfo, &m_fdInfo, sizeof(struct fd_info));
    return NO_ERROR;
}

status_t ExynosCameraFrame::storeDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    if (getMetaDmRequestFrameCount(shot) == 0)
        CLOGW(" DM Frame count is ZERO");

    memcpy(&m_metaData[srcNodeIndex].shot.dm, &shot->shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeDynamicMeta(struct camera2_dm *dm, uint32_t srcNodeIndex)
{
    if (dm == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    if (getMetaDmRequestFrameCount(dm) == 0)
        CLOGW(" DM Frame count is ZERO");

    memcpy(&m_metaData[srcNodeIndex].shot.dm, dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeUserDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&m_metaData[srcNodeIndex].shot.udm, &shot->shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeUserDynamicMeta(struct camera2_udm *udm, uint32_t srcNodeIndex)
{
    if (udm == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&m_metaData[srcNodeIndex].shot.udm, udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&shot->shot.dm, &m_metaData[srcNodeIndex].shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::setDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&m_metaData[srcNodeIndex].shot.dm, &shot->shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDynamicMeta(struct camera2_dm *dm, uint32_t srcNodeIndex)
{
    if (dm == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(dm, &m_metaData[srcNodeIndex].shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getUserDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex)
{
    if (shot == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&shot->shot.udm, &m_metaData[srcNodeIndex].shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getUserDynamicMeta(struct camera2_udm *udm, uint32_t srcNodeIndex)
{
    if (udm == NULL) {
        CLOGE(" buffer is NULL");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(udm, &m_metaData[srcNodeIndex].shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::setMetaDataEnable(bool flag, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_metaDataEnable[srcNodeIndex] = flag;
    return NO_ERROR;
}

bool ExynosCameraFrame::getMetaDataEnable(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return false;
    }

    long count = 0;

    while (count < DM_WAITING_COUNT) {
        if (m_metaDataEnable[srcNodeIndex] == true) {
            if (0 < count)
                CLOGD(" metadata enable count(%ld) ", count);

            break;
        }

        count++;
        usleep(WAITING_TIME);
    }

    return m_metaDataEnable[srcNodeIndex];
}

status_t ExynosCameraFrame::getZoomRect(ExynosRect *zoomRect, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    zoomRect->x = m_zoomRect[srcNodeIndex].x;
    zoomRect->y = m_zoomRect[srcNodeIndex].y;
    zoomRect->w = m_zoomRect[srcNodeIndex].w;
    zoomRect->h = m_zoomRect[srcNodeIndex].h;

    return NO_ERROR;
}

status_t ExynosCameraFrame::setZoomRect(ExynosRect zoomRect, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_zoomRect[srcNodeIndex].x = zoomRect.x;
    m_zoomRect[srcNodeIndex].y = zoomRect.y;
    m_zoomRect[srcNodeIndex].w = zoomRect.w;
    m_zoomRect[srcNodeIndex].h = zoomRect.h;

    return NO_ERROR;
}

status_t ExynosCameraFrame::getActiveZoomRect(ExynosRect *zoomRect, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    zoomRect->x = m_activeZoomRect[srcNodeIndex].x;
    zoomRect->y = m_activeZoomRect[srcNodeIndex].y;
    zoomRect->w = m_activeZoomRect[srcNodeIndex].w;
    zoomRect->h = m_activeZoomRect[srcNodeIndex].h;

    return NO_ERROR;
}

status_t ExynosCameraFrame::setActiveZoomRect(ExynosRect zoomRect, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_activeZoomRect[srcNodeIndex].x = zoomRect.x;
    m_activeZoomRect[srcNodeIndex].y = zoomRect.y;
    m_activeZoomRect[srcNodeIndex].w = zoomRect.w;
    m_activeZoomRect[srcNodeIndex].h = zoomRect.h;

    return NO_ERROR;
}

float ExynosCameraFrame::getZoomRatio(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return m_zoomRatio[srcNodeIndex];
}

status_t ExynosCameraFrame::setZoomRatio(float zoomRatio, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_zoomRatio[srcNodeIndex] = zoomRatio;

    return NO_ERROR;
}

float ExynosCameraFrame::getActiveZoomRatio(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return m_activeZoomRatio[srcNodeIndex];
}

status_t ExynosCameraFrame::setActiveZoomRatio(float zoomRatio, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_activeZoomRatio[srcNodeIndex] = zoomRatio;

    return NO_ERROR;
}

float ExynosCameraFrame::getAppliedZoomRatio(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return m_appliedZoomRatio[srcNodeIndex];
}

status_t ExynosCameraFrame::setAppliedZoomRatio(float zoomRatio, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_appliedZoomRatio[srcNodeIndex] = zoomRatio;

    return NO_ERROR;
}

int ExynosCameraFrame::getActiveZoomMargin(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return m_activeZoomMargin[srcNodeIndex];
}

status_t ExynosCameraFrame::setActiveZoomMargin(int zoomMargin, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_activeZoomMargin[srcNodeIndex] = zoomMargin;

    return NO_ERROR;
}

uint32_t ExynosCameraFrame::getSetfile(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return m_metaData[srcNodeIndex].setfile;
}

status_t ExynosCameraFrame::setSetfile(uint32_t setfile, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    m_metaData[srcNodeIndex].setfile = setfile;

    return NO_ERROR;
}

status_t ExynosCameraFrame::getNodeGroupInfo(struct camera2_node_group *node_group, int index, uint32_t srcNodeIndex)
{
    if (node_group == NULL) {
        CLOGE(" node_group is NULL");
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        CLOGE(" index is bigger than PERFRAME_NODE_GROUP_MAX");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(node_group, &m_node_gorup[srcNodeIndex][index], sizeof(struct camera2_node_group));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getNodeGroupInfo(struct camera2_node_group *node_group, int index, float *zoomRatio, uint32_t srcNodeIndex)
{
    getNodeGroupInfo(node_group, index, srcNodeIndex);
    *zoomRatio = m_zoomRatio[srcNodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeNodeGroupInfo(struct camera2_node_group *node_group, int index, uint32_t srcNodeIndex)
{
    if (node_group == NULL) {
        CLOGE(" node_group is NULL");
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        CLOGE(" index is bigger than PERFRAME_NODE_GROUP_MAX");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&m_node_gorup[srcNodeIndex][index], node_group, sizeof(struct camera2_node_group));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeNodeGroupInfo(struct camera2_node_group *node_group, int index, float zoomRatio, uint32_t srcNodeIndex)
{
    storeNodeGroupInfo(node_group, index, srcNodeIndex);
    m_zoomRatio[srcNodeIndex] = zoomRatio;

    return NO_ERROR;
}

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
status_t ExynosCameraFrame::getVirtualNodeInfo(struct camera2_virtual_node_group *node_group, int index, uint32_t srcNodeIndex)
{
    if (node_group == NULL) {
        CLOGE(" node_group is NULL");
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        CLOGE(" index is bigger than PERFRAME_NODE_GROUP_MAX");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(node_group, &m_virtualnode_gorup[srcNodeIndex][index], sizeof(struct camera2_virtual_node_group));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeVirtualNodeInfo(struct camera2_virtual_node_group *node_group, int index, uint32_t srcNodeIndex)
{
    if (node_group == NULL) {
        CLOGE(" node_group is NULL");
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        CLOGE(" index is bigger than PERFRAME_NODE_GROUP_MAX");
        return BAD_VALUE;
    }

    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    memcpy(&m_virtualnode_gorup[srcNodeIndex][index], node_group, sizeof(struct camera2_virtual_node_group));

    return NO_ERROR;
}
#endif

void ExynosCameraFrame::dumpNodeGroupInfo(const char *name, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return;
    }

    if (name != NULL)
        CLOGD("(%s)++++++++++++++++++++ frameCount(%d)", name, m_frameCount);
    else
        CLOGD("()++++++++++++++++++++ frameCount(%d)", m_frameCount);

    for (int i = 0; i < PERFRAME_NODE_GROUP_MAX; i ++) {
        CLOGI("Leader[%d] (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
            i,
            m_node_gorup[srcNodeIndex][i].leader.input.cropRegion[0],
            m_node_gorup[srcNodeIndex][i].leader.input.cropRegion[1],
            m_node_gorup[srcNodeIndex][i].leader.input.cropRegion[2],
            m_node_gorup[srcNodeIndex][i].leader.input.cropRegion[3],
            m_node_gorup[srcNodeIndex][i].leader.output.cropRegion[0],
            m_node_gorup[srcNodeIndex][i].leader.output.cropRegion[1],
            m_node_gorup[srcNodeIndex][i].leader.output.cropRegion[2],
            m_node_gorup[srcNodeIndex][i].leader.output.cropRegion[3],
            m_node_gorup[srcNodeIndex][i].leader.request,
            m_node_gorup[srcNodeIndex][i].leader.vid);

        for (int j = 0; j < CAPTURE_NODE_MAX; j ++) {
            CLOGI("Capture[%d][%d] (%d, %d, %d, %d)(%d, %d, %d, %d)(%d, %d)",
                i,
                j,
                m_node_gorup[srcNodeIndex][i].capture[j].input.cropRegion[0],
                m_node_gorup[srcNodeIndex][i].capture[j].input.cropRegion[1],
                m_node_gorup[srcNodeIndex][i].capture[j].input.cropRegion[2],
                m_node_gorup[srcNodeIndex][i].capture[j].input.cropRegion[3],
                m_node_gorup[srcNodeIndex][i].capture[j].output.cropRegion[0],
                m_node_gorup[srcNodeIndex][i].capture[j].output.cropRegion[1],
                m_node_gorup[srcNodeIndex][i].capture[j].output.cropRegion[2],
                m_node_gorup[srcNodeIndex][i].capture[j].output.cropRegion[3],
                m_node_gorup[srcNodeIndex][i].capture[j].request,
                m_node_gorup[srcNodeIndex][i].capture[j].vid);
        }

        if (name != NULL)
            CLOGD("(%s)------------------------ ", name);
        else
            CLOGD("()------------------------ ");
    }

    if (name != NULL)
        CLOGD("(%s)++++++++++++++++++++", name);
    else
        CLOGD("()++++++++++++++++++++");

    return;
}

void ExynosCameraFrame::setCameraId(int cameraId, int32_t srcNodeIndex)
{
    if (srcNodeIndex == OUTPUT_NODE_1)
        m_cameraId = cameraId;

    m_cameraIds[srcNodeIndex] = cameraId;
#ifdef USE_DEBUG_PROPERTY
    m_previousFrameState = FRAME_STATE_READY;
    m_createTimestamp = systemTime(SYSTEM_TIME_BOOTTIME);
    m_completeTimestamp = 0LL;
#endif
}

int ExynosCameraFrame::getCameraId(int32_t srcNodeIndex)
{
    if (srcNodeIndex < 0 || srcNodeIndex >= SRC_BUFFER_COUNT_MAX)
        return  -1;

    return m_cameraIds[srcNodeIndex];
}

void ExynosCameraFrame::setFactoryType(enum FRAME_FACTORY_TYPE factoryType)
{
    m_factoryType = factoryType;
}

enum FRAME_FACTORY_TYPE ExynosCameraFrame::getFactoryType(void) const
{
    return m_factoryType;
}

void ExynosCameraFrame::setRequestKey(uint32_t requestKey)
{
    m_requestKey = requestKey;
}

uint32_t ExynosCameraFrame::getRequestKey(void) const
{
    return m_requestKey;
}

void ExynosCameraFrame::setFrameIndex(int index)
{
    m_frameIndex = index;
}

int ExynosCameraFrame::getFrameIndex(void)
{
    return m_frameIndex;
}

void ExynosCameraFrame::setMaxFrameIndex(int index)
{
    m_frameMaxIndex = index;
}

int ExynosCameraFrame::getMaxFrameIndex(void)
{
    return m_frameMaxIndex;
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCameraFrame::setBokehAnchorFrameIndex(int index)
{
    if (index > getMaxFrameIndex()) {
        CLOGE("[F%d]invalid index(%d)", getFrameCount(), index);
        return BAD_VALUE;
    }
    m_bokehAnchorFrameIndex = index;

    return NO_ERROR;
}

int ExynosCameraFrame::getBokehAnchorFrameIndex(void)
{
    return m_bokehAnchorFrameIndex;
}
#endif

void ExynosCameraFrame::setJpegEncodeSize(int pipeId, int jpegEncodeSize)
{
    int index = getJpegPipeIdIndex(pipeId);

    if (index >= 0) {
        m_jpegInfo[index].jpegEncodeSize = jpegEncodeSize;
        CLOGV("[F%d]Jpeg: index = %d, size(%#x)", getFrameCount(), index, jpegEncodeSize);
    } else {
        CLOGE("[F%d]invalid pipeId = %d", getFrameCount(), pipeId);
    }
}

int ExynosCameraFrame::getJpegEncodeSize(int pipeId)
{
    int index = getJpegPipeIdIndex(pipeId);

    if (index >= 0) {
        CLOGV("[F%d]Jpeg: index = %d, size(%#x)", getFrameCount(), index, m_jpegInfo[index].jpegEncodeSize);
        return m_jpegInfo[index].jpegEncodeSize;
    } else {
        CLOGE("[F%d]invalid pipeId = %d", getFrameCount(), pipeId);
        return -1;
    }
}

void ExynosCameraFrame::setJpegPipeInfo(int pipeId, ExynosRect jpegRect, camera2_shot* shot)
{
    int index = getJpegPipeIdIndex(pipeId);

    if (index >= 0) {
        m_jpegInfo[index].jpegRect            = jpegRect;
        m_jpegInfo[index].jpegQuality         = shot->ctl.jpeg.quality;
        m_jpegInfo[index].thumbnailSize.w     = shot->ctl.jpeg.thumbnailSize[0];
        m_jpegInfo[index].thumbnailSize.h     = shot->ctl.jpeg.thumbnailSize[1];
        m_jpegInfo[index].thumbnailQuality    = shot->ctl.jpeg.thumbnailQuality;
        m_jpegInfo[index].rotation            = getRotation(pipeId);

        CLOGV("[F%d]Jpeg: index = %d, rect(%dx%d) quality(%d) rotation(%d), Thumbnail: size(%dx%d) quality(%d)",
             getFrameCount(), index,
             m_jpegInfo[index].jpegRect.w, m_jpegInfo[index].jpegRect.h,
             m_jpegInfo[index].jpegQuality, m_jpegInfo[index].rotation,
             m_jpegInfo[index].thumbnailSize.w, m_jpegInfo[index].thumbnailSize.h,
             m_jpegInfo[index].thumbnailQuality);
    } else {
        CLOGE("[F%d]invalid pipeId = %d", getFrameCount(), pipeId);
    }
}

ExynosCameraFrame::jpeg_pipe_info ExynosCameraFrame::getJpegPipeInfo(int pipeId)
{
    jpeg_pipe_info jpegPipeInfo = {0};
    int index = getJpegPipeIdIndex(pipeId);

    if (index >= 0) {
        jpegPipeInfo = m_jpegInfo[index];
    } else {
        CLOGE("[F%d]invalid pipeId = %d", getFrameCount(), pipeId);
    }

    return m_jpegInfo[index];
}

int64_t ExynosCameraFrame::getTimeStamp(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return (int64_t)getMetaDmSensorTimeStamp(&m_metaData[srcNodeIndex]);
}

int64_t ExynosCameraFrame::getTimeStampBoot(uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return BAD_VALUE;
    }

    return (int64_t)getMetaUdmSensorTimeStampBoot(&m_metaData[srcNodeIndex]);
}

void ExynosCameraFrame::getFpsRange(uint32_t *min, uint32_t *max, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return;
    }

    getMetaCtlAeTargetFpsRange(&m_metaData[srcNodeIndex], min, max);
}

void ExynosCameraFrame::setFpsRange(uint32_t min, uint32_t max, uint32_t srcNodeIndex)
{
    if (srcNodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE("Invalid srcNodeIndex index, max(%d) index(%d)", SRC_BUFFER_COUNT_MAX, srcNodeIndex);
        return;
    }

    setMetaCtlAeTargetFpsRange(&m_metaData[srcNodeIndex], min, max);
    setMetaCtlSensorFrameDuration(&m_metaData[srcNodeIndex], (uint64_t)((1000 * 1000 * 1000) / (uint64_t)max));
}

status_t ExynosCameraFrame::setBackupRequest(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId, bool val)
{
    status_t ret = NO_ERROR;

    if ((pipeId % 100) >= MAX_NUM_PIPES) {
        CLOGE("[F%d]Invalid pipeId(%d) mode(%d)", m_frameCount, pipeId, mode);
    } else {
        m_backupRequest[mode][INDEX(pipeId)] = val;
    }

    return ret;
}

bool ExynosCameraFrame::getBackupRequest(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId)
{
    bool request = false;

    if ((pipeId % 100) >= MAX_NUM_PIPES) {
        CLOGE("[F%d]Invalid pipeId(%d) mode(%d)", m_frameCount, pipeId, mode);
    } else {
        request = m_backupRequest[mode][INDEX(pipeId)];
    }

    return request;
}

void ExynosCameraFrame::backupRequest(REQUEST_BACKUP_MODE_T mode)
{
    for (int i = 0; i < MAX_NUM_PIPES; i++)
        m_backupRequest[mode][i] = m_request[i];
}

void ExynosCameraFrame::restoreRequest(REQUEST_BACKUP_MODE_T mode)
{
    for (int i = 0; i < MAX_NUM_PIPES; i++)
        m_request[i] = m_backupRequest[mode][i];
}

/*
 * Based on registered request values, this API will control specific request to set.
 * and also reverse another request(true) to set by !val.
 *  - This function must be called after backupRequest().
 *  - This function returns the flag that whether another request changed or not.
 *
 * ex. existed settting.
 *    PIPE_3AC_REPROCESSING = true
 *    PIPE_JPEG_REPROCESSING = true
 *    PIPE_THUMB_REPROCESSING = true
 *
 *   If it happened calling reverseSpecificRequest(PIPE_3AC_REPROCESSING, true),
 *    PIPE_3AC_REPROCESSING = true
 *    PIPE_JPEG_REPROCESSING = false  (!true)
 *    PIPE_THUMB_REPROCESSING = false (!true)
 *
 *   If it happened calling reverseSpecificRequest(PIPE_3AC_REPROCESSING, false),
 *    PIPE_3AC_REPROCESSING = false
 *    PIPE_JPEG_REPROCESSING = true  (!false)
 *    PIPE_THUMB_REPROCESSING = true (!false)
 */
bool ExynosCameraFrame::reverseExceptForSpecificReq(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId, bool val)
{
    bool changed = false;

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (INDEX(pipeId) == INDEX(i)) {
            m_request[i] = val;
        } else if (m_backupRequest[mode][i]) {
            m_request[i] = !val;
            changed = true;
        }
    }

    return changed;
}

void ExynosCameraFrame::setMode(FRAME_MODE_T mode, bool enable)
{
    m_modeLock.lock();
    if (mode < FRAME_MODE_MAX) {
        m_mode[mode] = enable;
    } else {
        CLOGE("[F%d]Invalid mode(%d) enable(%d)", m_frameCount, mode, enable);
    }
    m_modeLock.unlock();
}

void ExynosCameraFrame::setModeValue(FRAME_MODE_VALUE_T modeValue, int value)
{
    m_modeValueLock.lock();
    if (modeValue < FRAME_MODE_VALUE_MAX) {
        m_modeValue[modeValue] = value;
    } else {
        CLOGE("[F%d]Invalid modeValue(%d) value(%d)", m_frameCount, modeValue, value);
    }
    m_modeValueLock.unlock();
}

bool ExynosCameraFrame::getMode(FRAME_MODE_T mode)
{
    bool ret = false;
    m_modeLock.lock();
    if (mode < FRAME_MODE_MAX) {
        ret = m_mode[mode];
    } else {
        CLOGE("[F%d]Invalid mode(%d)", m_frameCount, mode);
    }
    m_modeLock.unlock();
    return ret;
}

int ExynosCameraFrame::getModeValue(FRAME_MODE_VALUE_T modeValue)
{
    int ret = FRAME_MODE_VALUE_INVALID;
    m_modeValueLock.lock();
    if (modeValue < FRAME_MODE_VALUE_MAX) {
        ret = m_modeValue[modeValue];
    } else {
        CLOGE("[F%d]Invalid modeValue(%d)", m_frameCount, modeValue);
    }
    m_modeValueLock.unlock();
    return ret;
}

void ExynosCameraFrame::setRequest(uint32_t pipeId, bool val)
{
    if ((pipeId % 100) >= MAX_NUM_PIPES) {
        CLOGE("[F%d]Invalid pipeId(%d)", m_frameCount, pipeId);
    } else {
        m_request[INDEX(pipeId)] = val;
    }
}

bool ExynosCameraFrame::getRequest(uint32_t pipeId)
{
    bool request = false;

    if ((pipeId % 100) >= MAX_NUM_PIPES) {
        CLOGE("[F%d]Invalid pipeId(%d)", m_frameCount, pipeId);
    } else {
        request = m_request[INDEX(pipeId)];
    }

    return request;
}

uint32_t ExynosCameraFrame::getUniqueKey(void)
{
    return m_uniqueKey;
}

status_t ExynosCameraFrame::setUniqueKey(uint32_t uniqueKey)
{
    m_uniqueKey = uniqueKey;
    return NO_ERROR;
}

status_t ExynosCameraFrame::setFrameInfo(ExynosCameraConfigurations *configurations, ExynosCameraParameters *params,
                                         uint32_t frameCount, uint32_t frameType)
{
    status_t ret = NO_ERROR;

    m_configurations = configurations;
    m_parameters = params;
    m_frameCount = frameCount;
    m_frameType = frameType;
    return ret;
}

status_t ExynosCameraFrame::setFrameMgrInfo(frame_key_queue_t *queue)
{
    status_t ret = NO_ERROR;

    m_frameQueue = queue;

    return ret;
}

void ExynosCameraFrame::setFrameType(frame_type_t frameType)
{
    m_frameType = frameType;
}

uint32_t ExynosCameraFrame::getFrameType()
{
    return m_frameType;
}

status_t ExynosCameraFrame::m_init()
{
    for (int i = 1; i < SRC_BUFFER_COUNT_MAX; i++)
        m_cameraIds[i] = -1;

    m_frameIndex = 0;
    m_frameMaxIndex = 0;
#ifdef USE_DUAL_CAMERA
    m_isPrepareBokehAnchorFrame = false;
    m_bokehAnchorFrameIndex = -1;
#endif
    memset(m_name, 0x00, sizeof(m_name));
    memset(&m_fdInfo, 0x00, sizeof(struct fd_info));

    m_numRequestPipe = 0;
    m_numCompletePipe = 0;
    m_frameState = FRAME_STATE_READY;
    m_frameLocked = false;

    for (int i = 0; i < SRC_BUFFER_COUNT_MAX; i++) {
        m_metaDataEnable[i] = false;
        m_zoomRatio[i] = 1.0f;
        m_activeZoomRatio[i] = 1.0f;
        m_appliedZoomRatio[i]= 1.0f;
        m_activeZoomMargin[i]= 0;

        memset(&m_zoomRect[i], 0x0, sizeof(ExynosRect));
        memset(&m_activeZoomRect[i], 0x0, sizeof(ExynosRect));
        memset(&m_metaData[i], 0x0, sizeof(struct camera2_shot_ext));

        for (int j = 0; j < PERFRAME_NODE_GROUP_MAX; j++) {
            memset(&m_node_gorup[i][j], 0x0, sizeof(struct camera2_node_group));
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            memset(&m_virtualnode_gorup[i][j], 0x0, sizeof(struct camera2_virtual_node_group));
#endif
        }
    }

    memset(m_jpegInfo, 0x00, sizeof(m_jpegInfo));

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        m_request[i] = false;
    }

    m_backupRequest.fill({false});

    for(int i = 0 ; i < FRAME_MODE_MAX; i++){
        m_mode[i] = false;
    }

    for(int i = 0 ; i < FRAME_MODE_VALUE_MAX; i++){
        m_modeValue[i] = 0;
    }

    m_uniqueKey = 0;
    m_yuvStallPortEnable = false;

    for (int i = 0; i < STREAM_TYPE_MAX; i++) {
        m_stream[i] = false;
    }

    m_refCount = 1;
    CLOGV(" Generate frame type(%d), frameCount(%d)", m_frameType, m_frameCount);

#ifdef DEBUG_FRAME_MEMORY_LEAK
    m_privateCheckLeakCount = 0;
    m_countLock.lock();
    m_checkLeakCount++;
    m_checkLeakFrameCount++;
    m_privateCheckLeakCount = m_checkLeakCount;
    CLOGI("[HalFrmCnt:F%d][LeakFrmCnt:F%lld] CONSTRUCTOR (%lld)",
           m_frameCount, m_privateCheckLeakFrameCount, m_checkLeakCount);
    m_countLock.unlock();
#endif

    m_dupBufferInfo.streamID = 0;
    m_dupBufferInfo.extScalerPipeID = 0;

    m_frameQueue = NULL;

#ifdef CORRECT_TIMESTAMP_FOR_SENSORFUSION
    m_adjustedTimestampFlag = false;
#endif

    m_specialCaptureStep = 0;
    m_hasRequest = false;
    m_updateResult = false;

    m_scenario.clear();
    m_flipHorizontalMap.clear();
    m_flipVerticalMap.clear();

    for (int i = 0; i < RESULT_UPDATE_TYPE_MAX; i++) {
        m_resultUpdatePipeId[i] = MAX_PIPE_NUM;
        m_resultUpdateStatus[i] = RESULT_UPDATE_STATUS_NONE;
    }

    m_stateInSelector = FRAME_STATE_IN_SELECTOR_BASE;

    m_needDynamicBayer = false;
    m_releaseDepthBufferFlag = true;
    m_streamTimeStamp = 0;
    m_bufferDondeIndex = -1;
    m_useOnePortFlag = false;
    m_internalBufTagPipeId = -1;
    m_onePortId = -1;
    m_secondPortId = -1;
    m_physStreamOnly = false;

    m_factoryType = FRAME_FACTORY_TYPE_CAPTURE_PREVIEW;
    m_requestKey  = 0;

#ifdef USE_DEBUG_PROPERTY
    m_createTimestamp = 0LL;
    m_completeTimestamp = 0LL;
    m_previousFrameState = FRAME_STATE_INVALID;
#endif

    m_vendorMeta = NULL;

    m_dualOperationMode = DUAL_OPERATION_MODE_NONE;
    m_isFallback        = false;
    m_displayCameraId   = m_cameraId;

    return NO_ERROR;
}

status_t ExynosCameraFrame::m_deinit()
{
    CLOGV(" Delete frame type(%d), frameCount(%d)", m_frameType, m_frameCount);
#ifdef DEBUG_FRAME_MEMORY_LEAK
    m_countLock.lock();
    m_checkLeakCount --;
    CLOGI("[HalFrmCnt:F%d][LeakFrmCnt:F%lld] DESTRUCTOR (%lld)",
           m_frameCount, m_privateCheckLeakCount, m_checkLeakCount);
    m_countLock.unlock();
#endif

    if (isComplete() == true && isCompleteForResultUpdate() == false) {
        m_dumpStatusForResultUpdate();
    }

    if (m_frameQueue != NULL)
        m_frameQueue->pushProcessQ(&m_uniqueKey);

    m_vendorMeta = NULL;

    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    ExynosCameraFrameEntity *tmpEntity = NULL;

    {
        Mutex::Autolock l(m_linkageLock);
        while (!m_linkageList.empty()) {
            r = m_linkageList.begin()++;
            if (*r) {
                curEntity = *r;

                while (curEntity != NULL) {
                    tmpEntity = curEntity->getNextEntity();
                    CLOGV("PipeId:%d", curEntity->getPipeId());

                    delete curEntity;
                    curEntity = tmpEntity;
                }

            }
            m_linkageList.erase(r);
        }
    }

    m_selectorTagQueueLock.lock();
    m_selectorTagQueue.clear();
    m_selectorTagQueueLock.unlock();

    return NO_ERROR;
}

void ExynosCameraFrame::m_updateStatusForResultUpdate(enum pipeline pipeId)
{
    for (int i = RESULT_UPDATE_TYPE_PARTIAL; i < RESULT_UPDATE_TYPE_MAX; i++) {
        if (m_resultUpdatePipeId[i] == pipeId
            && m_resultUpdateStatus[i] == RESULT_UPDATE_STATUS_REQUIRED) {
            switch (i) {
            case RESULT_UPDATE_TYPE_PARTIAL:
            case RESULT_UPDATE_TYPE_ALL:
                m_resultUpdateStatus[i] = RESULT_UPDATE_STATUS_READY;
                break;
            case RESULT_UPDATE_TYPE_BUFFER:
                m_resultUpdateStatus[i] = RESULT_UPDATE_STATUS_DONE;
                break;
            default:
                /* No operation */
                break;
            }
        }
    }
}

void ExynosCameraFrame::m_dumpStatusForResultUpdate(void)
{
    for (int i = RESULT_UPDATE_TYPE_PARTIAL; i < RESULT_UPDATE_TYPE_MAX; i++) {
        CLOGD("[F%d T%d]ResultType %2d PipeId %3d ResultUpdateStatus %2d",
                m_frameCount, m_frameType,
                i, m_resultUpdatePipeId[i], m_resultUpdateStatus[i]);
    }

    printNotDoneEntity();
}

status_t ExynosCameraFrame::setRotation(uint32_t pipeId, int rotation)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    ret = entity->setRotation(rotation);
    if (ret != NO_ERROR) {
        CLOGE("pipeId(%d)->setRotation(%d) fail", pipeId, rotation);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getRotation(uint32_t pipeId)
{
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        CLOGE("Could not find entity, pipeID(%d)", pipeId);
        return BAD_VALUE;
    }

    return entity->getRotation();
}

status_t ExynosCameraFrame::setFlipHorizontal(uint32_t pipeId, int flipHorizontal)
{
    status_t ret = NO_ERROR;
    m_flipHorizontalMap[pipeId] = flipHorizontal;

    return ret;
}

int ExynosCameraFrame::getFlipHorizontal(uint32_t pipeId)
{
    return m_flipHorizontalMap[pipeId];
}

status_t ExynosCameraFrame::setFlipVertical(uint32_t pipeId, int flipVertical)
{
    status_t ret = NO_ERROR;
    m_flipVerticalMap[pipeId] = flipVertical;
    return ret;
}

int ExynosCameraFrame::getFlipVertical(uint32_t pipeId)
{
    return m_flipVerticalMap[pipeId];
}

void ExynosCameraFrame::setPPScenario(int pipeId, int scenario)
{
    switch (pipeId) {
#ifdef USE_SLSI_PLUGIN
    case PIPE_FUSION:
    case PIPE_FUSION_REPROCESSING:
    case PIPE_PLUGIN_BASE ... PIPE_PLUGIN_MAX:
    case PIPE_PLUGIN_BASE_REPROCESSING ... PIPE_PLUGIN_MAX_REPROCESSING:
#endif
    {
        Mutex::Autolock l(m_scenarioLock);
        m_scenario[pipeId] = scenario;
    }
        break;
    default:
        CLOGE("setScenario failed, pipeID(%d) scenario(%d)", pipeId, scenario);
        break;
    }

}

int32_t ExynosCameraFrame::getPPScenario(int pipeId)
{
    int ret = -1;
    map<int, int>::iterator iter;

    switch (pipeId) {
#ifdef USE_SLSI_PLUGIN
    case PIPE_FUSION:
    case PIPE_FUSION_REPROCESSING:
    case PIPE_PLUGIN_BASE ... PIPE_PLUGIN_MAX:
    case PIPE_PLUGIN_BASE_REPROCESSING ... PIPE_PLUGIN_MAX_REPROCESSING:
#endif
    {
        Mutex::Autolock l(m_scenarioLock);

        iter = m_scenario.find(pipeId);
        if (iter != m_scenario.end()) {
            ret = iter->second;
        } else {
            CLOGE("find failed, pipeId(%d)", pipeId);
        }
    }
        break;
    default:
        CLOGE("getScenario failed, pipeId(%d)", pipeId);
        break;
    }

    return ret;
}

int32_t ExynosCameraFrame::getPPScenarioIndex(int scenario)
{
    int ret = -1;

    Mutex::Autolock l(m_scenarioLock);
    map<int, int>::iterator iter;

    for(iter = m_scenario.begin(); iter != m_scenario.end(); iter++)
    {
        if (scenario == iter->second) {
            ret = iter->first;
        }
    }

    return ret;
}

bool ExynosCameraFrame::hasPPScenario(int scenario)
{
    int ret = false;

    Mutex::Autolock l(m_scenarioLock);
    map<int, int>::iterator iter;

    for(iter = m_scenario.begin(); iter != m_scenario.end(); iter++)
    {
        if (scenario == iter->second) {
            ret = true;
        }
    }

    return ret;
}


bool ExynosCameraFrame::isLastPPScenarioPipe(int pipeId)
{
    map<int, int>::iterator iter;
    int ret = false;
    switch (pipeId) {
#ifdef USE_SLSI_PLUGIN
    case PIPE_PLUGIN1:
    case PIPE_PLUGIN_PRE1_REPROCESSING:
    case PIPE_PLUGIN_POST1_REPROCESSING:
#endif
    {
        Mutex::Autolock l(m_scenarioLock);

        /* check current pipe id and next pipeId */
        iter = m_scenario.find(pipeId);
        if (iter != m_scenario.end()) {
            iter = m_scenario.find(pipeId+1);
            if (iter == m_scenario.end()) {
                ret = true;
            }
        }
    }
        break;
#ifdef USE_SLSI_PLUGIN
    case PIPE_PLUGIN2:
    case PIPE_PLUGIN_PRE2_REPROCESSING:
    case PIPE_PLUGIN_POST2_REPROCESSING:
#endif
    {
        Mutex::Autolock l(m_scenarioLock);
        /* last Pipe id check current pipie id */
        iter = m_scenario.find(pipeId);
        if (iter != m_scenario.end()) {
            ret = true;
        }
    }
        break;
    default:
        CLOGE("getScenario failed, pipeId(%d)", pipeId);
        break;
    }

    return ret;
}

int32_t ExynosCameraFrame::getLastPPScenarioIndex()
{
    Mutex::Autolock l(m_scenarioLock);

    return (int32_t) m_scenario.size();
}

int32_t ExynosCameraFrame::getNumberOfPPScenarios()
{
    Mutex::Autolock l(m_scenarioLock);

    return (int32_t)m_scenario.size();
}

void ExynosCameraFrame::setStreamRequested(int stream, bool flag)
{
    if (stream < STREAM_TYPE_MAX) {
        m_stream[stream] = flag;
    }
}

bool ExynosCameraFrame::getStreamRequested(int stream)
{
    if (stream < STREAM_TYPE_MAX) {
        return m_stream[stream];
    } else {
        return false;
    }
}

void ExynosCameraFrame::setPipeIdForResultUpdate(result_update_type_t resultType, enum pipeline pipeId)
{
    m_resultUpdatePipeId[resultType] = pipeId;
}

int ExynosCameraFrame::getPipeIdForResultUpdate(result_update_type_t resultType)
{
    return m_resultUpdatePipeId[resultType];
}

void ExynosCameraFrame::setStatusForResultUpdate(result_update_type_t resultType, result_update_status_t resultStatus)
{
    m_resultUpdateStatus[resultType] = resultStatus;
}

int ExynosCameraFrame::getStatusForResultUpdate(result_update_type_t resultType)
{
    return m_resultUpdateStatus[resultType];
}

bool ExynosCameraFrame::isReadyForResultUpdate(result_update_type_t resultType)
{
    bool isNeedResultUpdate = false;

    isNeedResultUpdate = (m_resultUpdateStatus[resultType] == RESULT_UPDATE_STATUS_READY);

    CLOGV("[F%d]ResultType %d ResultStatus %d Update %d",
            m_frameCount, resultType, m_resultUpdateStatus[resultType], isNeedResultUpdate);

    return isNeedResultUpdate;
}

bool ExynosCameraFrame::isCompleteForResultUpdate(void)
{
    bool isPartialResultDone = false;
    bool isBufferDone = false;
    bool isAllResultDone = false;

    isPartialResultDone = (m_resultUpdateStatus[RESULT_UPDATE_TYPE_PARTIAL] == RESULT_UPDATE_STATUS_NONE)
                          || (m_resultUpdateStatus[RESULT_UPDATE_TYPE_PARTIAL] == RESULT_UPDATE_STATUS_DONE);
    isBufferDone        = (m_resultUpdateStatus[RESULT_UPDATE_TYPE_BUFFER] == RESULT_UPDATE_STATUS_NONE)
                          || (m_resultUpdateStatus[RESULT_UPDATE_TYPE_BUFFER] == RESULT_UPDATE_STATUS_DONE);
    isAllResultDone     = (m_resultUpdateStatus[RESULT_UPDATE_TYPE_ALL] == RESULT_UPDATE_STATUS_NONE)
                          || (m_resultUpdateStatus[RESULT_UPDATE_TYPE_ALL] == RESULT_UPDATE_STATUS_DONE);

    return (isPartialResultDone && isBufferDone && isAllResultDone);
}

enum DUAL_OPERATION_MODE ExynosCameraFrame::convertDualModeByFrameType(frame_type_t frameType)
{
    enum DUAL_OPERATION_MODE dualOperationMode = DUAL_OPERATION_MODE_NONE;

    switch (frameType) {
    case FRAME_TYPE_PREVIEW_DUAL_MASTER:
    case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
    case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
    case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
        dualOperationMode = DUAL_OPERATION_MODE_SYNC;
        break;
    default:
        if (ExynosCameraFrame::isSlaveFrame(frameType)) {
            dualOperationMode = DUAL_OPERATION_MODE_SLAVE;
        } else {
            dualOperationMode = DUAL_OPERATION_MODE_MASTER;
        }
        break;
    }

    return dualOperationMode;
}

bool ExynosCameraFrame::isSlaveFrame(frame_type_t frameType)
{
    switch (frameType) {
#ifdef USE_DUAL_CAMERA
        case FRAME_TYPE_PREVIEW_SLAVE:
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_SLAVE:
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
        case FRAME_TYPE_INTERNAL_SLAVE:
        case FRAME_TYPE_TRANSITION_SLAVE:
            return true;
#endif
        default:
            return false;
    }
    return false;
}

#ifdef USE_DUAL_CAMERA
bool ExynosCameraFrame::isSlaveFrame(void)
{
    bool isSlaveFrameFlag;

    isSlaveFrameFlag = isSlaveFrame((frame_type_t)m_frameType);

    return isSlaveFrameFlag;
}
#endif

/* selector tag helper functions */
void ExynosCameraFrame::lockSelectorTagList(void)
{
    m_selectorTagQueueLock.lock();
}

void ExynosCameraFrame::unlockSelectorTagList(void)
{
    m_selectorTagQueueLock.unlock();
}

status_t ExynosCameraFrame::addSelectorTag(int selectorId, int pipeId, int bufPos, bool isSrc)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    return addRawSelectorTag(selectorId, pipeId, bufPos, isSrc);
}

bool ExynosCameraFrame::findSelectorTag(ExynosCameraFrameSelectorTag *compareTag)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    return findRawSelectorTag(compareTag);
}

bool ExynosCameraFrame::removeSelectorTag(ExynosCameraFrameSelectorTag *removeTag)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    return removeRawSelectorTag(removeTag);
}

bool ExynosCameraFrame::getFirstSelectorTag(ExynosCameraFrameSelectorTag *tag)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    return getFirstRawSelectorTag(tag);
}

void ExynosCameraFrame::setStateInSelector(FRAME_STATE_IN_SELECTOR_T state)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    setRawStateInSelector(state);
}

FRAME_STATE_IN_SELECTOR_T ExynosCameraFrame::getStateInSelector(void)
{
    Mutex::Autolock l(m_selectorTagQueueLock);

    return getRawStateInSelector();
}

/*
 * It needs lock.
 * It appends the new tag to this frame.
 */
status_t ExynosCameraFrame::addRawSelectorTag(int selectorId, int pipeId, int bufPos, bool isSrc)
{
    ExynosCameraFrameSelectorTag tempTag;

    tempTag.selectorId = selectorId;
    tempTag.pipeId = pipeId;
    tempTag.bufPos = bufPos;
    tempTag.isSrc = isSrc;

    m_selectorTagQueue.push_back(tempTag);

    return NO_ERROR;
}

/*
 * It needs lock.
 * It returns first tag to match with compareTag.
 */
bool ExynosCameraFrame::findRawSelectorTag(ExynosCameraFrameSelectorTag *compareTag)
{
    selector_tag_queue_t::iterator r;
    bool found = false;
    selector_tag_queue_t *list = getSelectorTagList();

    if (list->empty()) {
        CLOGW("list->empty(). so cannot find RawBuffer");
        return false;
    }

    r = list->begin()++;

    do {
        if ((r->selectorId == compareTag->selectorId || compareTag->selectorId == 0) // SELECTOR_ID_BASE
                && (r->pipeId == compareTag->pipeId || compareTag->pipeId < 0)
                && (r->bufPos == compareTag->bufPos || compareTag->bufPos < 0)
                && (r->isSrc == compareTag->isSrc || compareTag->isSrc < 0)) {
            *compareTag = *r;
            found = true;
            break;
        }

        r++;
    } while (r != list->end());

    return found;
}

/*
 * It needs lock.
 * It removes the existing tag from this frame.
 */
bool ExynosCameraFrame::removeRawSelectorTag(ExynosCameraFrameSelectorTag *removeTag)
{
    selector_tag_queue_t::iterator r;
    bool removed = false;
    selector_tag_queue_t *list = getSelectorTagList();

    if (list->empty())
        return false;

    r = list->begin()++;

    do {
        if (*r == *removeTag) {
            r = list->erase(r);
            removed = true;
            break;
        }

        r++;
    } while (r != list->end());

    return removed;
}

/*
 * It needs lock.
 * It returns the first tag from this frame.
 */
bool ExynosCameraFrame::getFirstRawSelectorTag(ExynosCameraFrameSelectorTag *tag)
{
    selector_tag_queue_t::iterator r;
    selector_tag_queue_t *list = getSelectorTagList();

    if (list->empty())
        return false;

    r = list->begin()++;
    *tag = *r;

    return true;
}

/*
 * It needs lock.
 * It updates the state which this frame is processing on the selector.
 */
void ExynosCameraFrame::setRawStateInSelector(FRAME_STATE_IN_SELECTOR_T state)
{
    m_stateInSelector = state;
}

/*
 * It needs lock.
 * It returns the state which this frame is processing on the selector.
 */
FRAME_STATE_IN_SELECTOR_T ExynosCameraFrame::getRawStateInSelector(void)
{
    return m_stateInSelector;
}

selector_tag_queue_t *ExynosCameraFrame::getSelectorTagList(void)
{
    return &m_selectorTagQueue;
}

status_t ExynosCameraFrame::setVendorMeta(sp<ExynosCameraVendorMetaData> vendorMeta)
{
    status_t ret = NO_ERROR;
    m_vendorMeta = vendorMeta;
    return ret;
}

sp<ExynosCameraVendorMetaData> ExynosCameraFrame::getVendorMeta()
{
    return m_vendorMeta;
}

void ExynosCameraFrame::setNeedDynamicBayer(bool set)
{
    m_needDynamicBayer = set;
}

bool ExynosCameraFrame::getNeedDynamicBayer(void)
{
    return m_needDynamicBayer;
}

void ExynosCameraFrame::setServiceBufferPipeId(int32_t streamId, int32_t pipeId)
{
    m_streamIdPipeIdMap[streamId] = pipeId;
}

int32_t ExynosCameraFrame::getServiceBufferPipeId(int32_t streamId)
{
    return m_streamIdPipeIdMap[streamId];
}

bool ExynosCameraFrame::hasServiceBuffer(int32_t streamId)
{
    return m_streamIdPipeIdMap.count(streamId);
}

void ExynosCameraFrame::setServiceBufferPosition(int32_t streamId, int32_t pos)
{
    m_streamIdPosMap[streamId] = pos;
}

int32_t ExynosCameraFrame::getServiceBufferPosition(int32_t streamId)
{
    return m_streamIdPosMap[streamId];
}

void ExynosCameraFrame::copySizeInfo(ExynosCameraFrameSP_sptr_t otherFrame)
{
    if (otherFrame == nullptr) return;

    frame_size_info_t info;
    for (int i = FRAME_SIZE_BASE + 1; i < FRAME_SIZE_MAX; i++) {
        if (otherFrame->getSizeInfo((frame_size_scenario_t)i, info)) {
            setSizeInfo((frame_size_scenario_t)i, info, otherFrame->getCameraId());
        }
    }
}

void ExynosCameraFrame::setSizeInfo(frame_size_scenario_t scenario, const frame_size_info_t &info, int32_t cameraId)
{
    if (cameraId < 0) cameraId = m_cameraId;

    Mutex::Autolock l(m_frameLock);

    m_frameSizeInfoMap[cameraId][scenario] = info;
}

bool ExynosCameraFrame::getSizeInfo(frame_size_scenario_t scenario, frame_size_info_t &info, int32_t cameraId)
{
    if (cameraId < 0) cameraId = m_cameraId;

    Mutex::Autolock l(m_frameLock);

    if (m_frameSizeInfoMap.count(cameraId) == 0) return false;
    auto &subMap = m_frameSizeInfoMap[cameraId];

    if (subMap.count(scenario) == 0) return false;

    info = subMap[scenario];

    return true;
}

const FrameSizeInfoMap_t& ExynosCameraFrame::getSizeInfoMap(int cameraId)
{
    if (cameraId < 0) cameraId = m_cameraId;

    Mutex::Autolock l(m_frameLock);

    return m_frameSizeInfoMap[cameraId];
}

bool ExynosCameraFrame::isOnePortId(int portId)
{
    bool result = false;

    if (portId >= ExynosCameraParameters::YUV_0 && portId < ExynosCameraParameters::YUV_MAX
        && m_onePortId == portId) {
        result = true;
    }

    return result;
}

bool ExynosCameraFrame::isSecondPortId(int portId)
{
    bool result = false;

    if (portId >= ExynosCameraParameters::YUV_0 && portId < ExynosCameraParameters::YUV_MAX
        && m_secondPortId == portId) {
        result = true;
    }

    return result;
}

void ExynosCameraFrame::setPairFrame(ExynosCameraFrameSP_sptr_t newFrame)
{
    Mutex::Autolock l(m_pairFrameLock);
    if (m_pairFrameQ.empty()) {
        m_pairFrameQ.push_back(newFrame);
        return;
    }

    int frameQSize = m_pairFrameQ.size();
    List<ExynosCameraFrameSP_sptr_t>::iterator r = m_pairFrameQ.begin();

    for (int i = 0; i < frameQSize; i++) {
        if (*r) {
            ExynosCameraFrameSP_sptr_t pairFrame = *r;
            if (pairFrame->getUniqueKey() == newFrame->getUniqueKey()) {
                return;
            }
        }
        r++;
    }
    m_pairFrameQ.push_back(newFrame);
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrame::getPairFrame()
{
    Mutex::Autolock l(m_pairFrameLock);
    if (m_pairFrameQ.empty()) {
        return NULL;
    }

    List<ExynosCameraFrameSP_sptr_t>::iterator r = m_pairFrameQ.begin();
    return *r;
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrame::popPairFrame()
{
    Mutex::Autolock l(m_pairFrameLock);
    if (m_pairFrameQ.empty()) {
        return NULL;
    }

    List<ExynosCameraFrameSP_sptr_t>::iterator r = m_pairFrameQ.begin();
    ExynosCameraFrameSP_sptr_t pairFrame = *r;

    m_pairFrameQ.erase(r);

    return pairFrame;
}

int ExynosCameraFrame::getSizeOfPairFrameQ()
{
    Mutex::Autolock l(m_pairFrameLock);
    return m_pairFrameQ.size();
}

void ExynosCameraFrame::releasePairFrameQ()
{
    Mutex::Autolock l(m_pairFrameLock);
    List<ExynosCameraFrameSP_sptr_t>::iterator r;

    while (!m_pairFrameQ.empty()) {
        r = m_pairFrameQ.begin()++;
        if (*r) {
            ExynosCameraFrameSP_sptr_t pairFrame = *r;
            CLOGV("[F%d] delete pair [F%d]",
                    m_frameCount, pairFrame->getFrameCount());
            m_pairFrameQ.erase(r);
        }
    }
}

/*
 * ExynosCameraFrameEntity class
 */

ExynosCameraFrameEntity::ExynosCameraFrameEntity(
        uint32_t pipeId,
        entity_type_t type,
        entity_buffer_type_t bufType)
{
    m_pipeId = pipeId;

    if (m_setEntityType(type) != NO_ERROR)
        CLOGE2("setEntityType fail, pipeId(%d), type(%d)", pipeId, type);

    m_srcBufCnt = 0;
    m_dstBufCnt = 0;

    m_bufferType = bufType;
    m_entityState = ENTITY_STATE_READY;

    m_prevEntity = NULL;
    m_nextEntity = NULL;

    m_flagSpecificParent = false;
    m_parentPipeId = -1;

    m_rotation = 0;

#ifdef USE_DEBUG_PROPERTY
    m_processTimestamp = 0LL;
    m_doneTimestamp = 0LL;
#endif
}

status_t ExynosCameraFrameEntity::m_setEntityType(entity_type_t type)
{
    status_t ret = NO_ERROR;

    m_EntityType = type;

    /* for src */
    for(int i = SRC_BUFFER_DEFAULT; i < SRC_BUFFER_COUNT_MAX; i++) {
        switch (type) {
        case ENTITY_TYPE_INPUT_ONLY:
            m_srcBufState[i] = ENTITY_BUFFER_STATE_REQUESTED;
            break;
        case ENTITY_TYPE_OUTPUT_ONLY:
            m_srcBufState[i] = ENTITY_BUFFER_STATE_NOREQ;
            break;
        case ENTITY_TYPE_INPUT_OUTPUT:
            m_srcBufState[i] = ENTITY_BUFFER_STATE_REQUESTED;
            break;
        default:
            m_srcBufState[i] = ENTITY_BUFFER_STATE_NOREQ;
            m_EntityType = ENTITY_TYPE_INVALID;
            ret = BAD_VALUE;
            break;
        }
    }

    /* for dst */
    for(int i = DST_BUFFER_DEFAULT; i < DST_BUFFER_COUNT_MAX; i++) {
        switch (type) {
        case ENTITY_TYPE_INPUT_ONLY:
            m_dstBufState[i] = ENTITY_BUFFER_STATE_NOREQ;
            break;
        case ENTITY_TYPE_OUTPUT_ONLY:
            m_dstBufState[i] = ENTITY_BUFFER_STATE_REQUESTED;
            break;
        case ENTITY_TYPE_INPUT_OUTPUT:
            m_dstBufState[i] = ENTITY_BUFFER_STATE_REQUESTED;
            break;
        default:
            m_dstBufState[i] = ENTITY_BUFFER_STATE_NOREQ;
            m_EntityType = ENTITY_TYPE_INVALID;
            ret = BAD_VALUE;
            break;
        }
    }

    return ret;
}

uint32_t ExynosCameraFrameEntity::getPipeId(void)
{
    return m_pipeId;
}

status_t ExynosCameraFrameEntity::setSrcBuf(ExynosCameraBuffer buf, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    if (m_srcBufState[nodeIndex] == ENTITY_BUFFER_STATE_COMPLETE) {
        CLOGV2("Buffer completed, state(%d)", m_srcBufState[nodeIndex]);
        return NO_ERROR;
    }

    if (m_bufferType != ENTITY_BUFFER_DELIVERY &&
        m_srcBufState[nodeIndex] != ENTITY_BUFFER_STATE_REQUESTED) {
        CLOGE2("Invalid buffer state(%d)", m_srcBufState[nodeIndex]);
        return INVALID_OPERATION;
    }

    if (this->m_srcBuf[nodeIndex].index <= -1 && buf.index > -1) {
        m_srcBufCnt++;
    }

    this->m_srcBuf[nodeIndex] = buf;

    ret = setSrcBufState(ENTITY_BUFFER_STATE_READY, nodeIndex);

    return ret;
}

status_t ExynosCameraFrameEntity::setDstBuf(ExynosCameraBuffer buf, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    if (m_bufferType != ENTITY_BUFFER_DELIVERY &&
        m_dstBufState[nodeIndex] != ENTITY_BUFFER_STATE_REQUESTED) {
        CLOGE2("Invalid buffer state(%d)", m_dstBufState[nodeIndex]);
        return INVALID_OPERATION;
    }

    if (this->m_dstBuf[nodeIndex].index <= -1 && buf.index > -1) {
        m_dstBufCnt++;
    }

    this->m_dstBuf[nodeIndex] = buf;
    ret = setDstBufState(ENTITY_BUFFER_STATE_READY, nodeIndex);

    return ret;
}

status_t ExynosCameraFrameEntity::getSrcBuf(ExynosCameraBuffer *buf, uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    *buf = this->m_srcBuf[nodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getDstBuf(ExynosCameraBuffer *buf, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    /* Comment out: It was collide with ExynosCamera's dirty dynamic bayer handling routine.
     * (make error log, but no side effect)
     * This code added for block human error.
     * I will add this code after check the side effect closely.
     */
    /*
    if (this->m_dstBuf[nodeIndex].index == -1) {
        CLOGE2("Invalid buffer index(%d)", nodeIndex);
        return BAD_VALUE;
    }
    */

    *buf = this->m_dstBuf[nodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setSrcRect(ExynosRect rect, uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    this->m_srcRect[nodeIndex] = rect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setDstRect(ExynosRect rect, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    this->m_dstRect[nodeIndex] = rect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getSrcRect(ExynosRect *rect, uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    *rect = this->m_srcRect[nodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getDstRect(ExynosRect *rect, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    *rect = this->m_dstRect[nodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setSrcBufState(entity_buffer_state_t state, uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    if (m_srcBufState[nodeIndex] == ENTITY_BUFFER_STATE_COMPLETE) {
        CLOGV2("Buffer completed, state(%d)", m_srcBufState[nodeIndex]);
        return NO_ERROR;
    }

    m_srcBufState[nodeIndex] = state;
    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setDstBufState(entity_buffer_state_t state, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return BAD_VALUE;
    }

    m_dstBufState[nodeIndex] = state;

    return NO_ERROR;
}

entity_buffer_state_t ExynosCameraFrameEntity::getSrcBufState(uint32_t nodeIndex)
{
    if (nodeIndex >= SRC_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return ENTITY_BUFFER_STATE_INVALID;
    }

    return m_srcBufState[nodeIndex];
}

entity_buffer_state_t ExynosCameraFrameEntity::getDstBufState(uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        CLOGE2("Invalid buffer index, index(%d)", nodeIndex);
        return ENTITY_BUFFER_STATE_INVALID;
    }

    return m_dstBufState[nodeIndex];
}

entity_buffer_type_t ExynosCameraFrameEntity::getBufType(void)
{
    return m_bufferType;
}

status_t ExynosCameraFrameEntity::setEntityState(entity_state_t state)
{
#ifdef USE_DEBUG_PROPERTY
    if (m_entityState != state) {
        switch (state) {
        case ENTITY_STATE_PROCESSING:
            m_processTimestamp = systemTime(SYSTEM_TIME_BOOTTIME);
            break;
        case ENTITY_STATE_FRAME_DONE:
            m_doneTimestamp = systemTime(SYSTEM_TIME_BOOTTIME);
            break;
        default:
            break;
        }
    }
#endif
    this->m_entityState = state;

    return NO_ERROR;
}

entity_state_t ExynosCameraFrameEntity::getEntityState(void)
{
    return this->m_entityState;
}

ExynosCameraFrameEntity *ExynosCameraFrameEntity::getPrevEntity(void)
{
    return this->m_prevEntity;
}

ExynosCameraFrameEntity *ExynosCameraFrameEntity::getNextEntity(void)
{
    return this->m_nextEntity;
}

status_t ExynosCameraFrameEntity::setPrevEntity(ExynosCameraFrameEntity *entity)
{
    this->m_prevEntity = entity;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setNextEntity(ExynosCameraFrameEntity *entity)
{
    this->m_nextEntity = entity;

    return NO_ERROR;
}

bool ExynosCameraFrameEntity::flagSpecficParent(void)
{
    return m_flagSpecificParent;
}

status_t ExynosCameraFrameEntity::setParentPipeId(enum pipeline parentPipeId)
{
    if (0 <= m_parentPipeId) {
        CLOGE2("m_parentPipeId(%d) is already set. parentPipeId(%d)",
             m_parentPipeId, parentPipeId);
        return BAD_VALUE;
    }

    m_flagSpecificParent = true;
    m_parentPipeId = parentPipeId;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getParentPipeId(void)
{
    return m_parentPipeId;
}

status_t ExynosCameraFrameEntity::setRotation(int rotation)
{
    m_rotation = rotation;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getRotation(void)
{
    return m_rotation;
}

}; /* namespace android */
