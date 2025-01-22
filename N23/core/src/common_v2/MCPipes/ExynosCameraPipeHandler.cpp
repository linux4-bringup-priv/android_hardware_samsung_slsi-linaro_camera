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
#define LOG_TAG "ExynosCameraPipeHandler"

#include "ExynosCameraPipeHandler.h"

namespace android {

PipeHandler::PipeHandler(
        int cameraId,
        uint32_t pipeId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        PIPE_HANDLER::SCENARIO scenario)
{
    m_cameraId = cameraId;
    m_pipeId = pipeId;
    m_configuration = configurations;
    m_parameter = obj_param;
    m_usageList.clear();
    m_scenario = scenario;
    m_perframeIndex = 0;
    m_bufferSupplier = NULL;
    m_inputFrameQ = NULL;
    m_outputFrameQ = NULL;
    m_frameDoneQ = NULL;
}

PipeHandler::~PipeHandler()
{
    m_usageList.clear();
}

status_t PipeHandler::setName(const char *name)
{
    strncpy(m_name,  name,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    return NO_ERROR;
}

status_t PipeHandler::process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status)
{
    status_t ret = NO_ERROR;
    ret = PipeHandler::m_process(usage, newFrame, status);
    if (ret != NO_ERROR) {
        CLOGE("PipeHandler::m_process() fail");
        return INVALID_OPERATION;
    }

    ret = this->m_process(usage, newFrame, status);
    if (ret != NO_ERROR) {
        CLOGE("this->m_process() fail");
        return INVALID_OPERATION;
    }
    return ret;
}

PIPE_HANDLER::SCENARIO PipeHandler::getScenario()
{
    return m_scenario;
}

status_t PipeHandler::addUsage(PIPE_HANDLER::USAGE usage)
{
    status_t ret = NO_ERROR;
    bool find = m_findItem(usage, &m_usageList, &m_usageListLock);
    if (find == true) {
        /* TODO : error log and skip */
    } else {
        bool item = true;
        ret = m_pushItem(usage, &m_usageList, &m_usageListLock, item);
        if (ret != NO_ERROR) {
            CLOGE("push list failed fail, usage(%d)", usage);
        }
    }
    return ret;
}

bool PipeHandler::isUsage(PIPE_HANDLER::USAGE usage)
{
    bool ret = m_findItem(usage, &m_usageList, &m_usageListLock);
    return ret;
}

status_t PipeHandler::setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier)
{
    CLOGD("");

    m_bufferSupplier = bufferSupplier;

    return NO_ERROR;
}

status_t PipeHandler::setPerframeIndex(int index)
{
    CLOGD("perframeIndex(%d)", index);
    status_t ret = NO_ERROR;
    m_perframeIndex = index;
    return ret;
}

status_t PipeHandler::setInputFrameQ(frame_queue_t *queue)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    m_inputFrameQ = queue;
    return ret;
}

status_t PipeHandler::setOutputFrameQ(frame_queue_t *queue)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    m_outputFrameQ = queue;
    return ret;
}

status_t PipeHandler::setFrameDoneQ(frame_queue_t *queue)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    m_frameDoneQ = queue;
    return ret;
}

status_t PipeHandler::m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status)
{
    status_t ret = NO_ERROR;
    return ret;
}

status_t PipeHandler::m_pushItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item)
{
    status_t ret = NO_ERROR;
    UsageMap::iterator iter;

    lock->lock();

    iter = list->find(key);
    if (iter != list->end()) {
        CLOGE("push list failed fail key(%d)", key);
        ret = INVALID_OPERATION;
    } else {
        UsagePair object(key, item);
        list->insert(object);
    }

    lock->unlock();
    return ret;
}

status_t PipeHandler::m_popItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item)
{
    status_t ret = NO_ERROR;
    UsageMap::iterator iter;

    lock->lock();

    iter = list->find(key);
    if (iter != list->end()) {
        item = iter->second;
        list->erase(iter);
    } else {
        CLOGE("pop list failed fail key(%d)", key);
        ret = INVALID_OPERATION;
    }

    lock->unlock();
    return ret;
}

status_t PipeHandler::m_getItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item)
{
    status_t ret = NO_ERROR;
    UsageMap::iterator iter;

    lock->lock();

    iter = list->find(key);
    if (iter != list->end()) {
        item = iter->second;
    } else {
        CLOGE("get list failed fail key(%d)", key);
        ret = INVALID_OPERATION;
    }

    lock->unlock();
    return ret;
}

bool PipeHandler::m_findItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock)
{
    bool ret = false;
    UsageMap::iterator iter;

    lock->lock();

    iter = list->find(key);
    if (iter != list->end()) {
        ret = true;
    } else {
        ret = false;
    }

    lock->unlock();
    return ret;
}

status_t PipeHandler::m_getKeyList(UsageMap *list, Mutex *lock, UsageList *keylist)
{
    status_t ret = NO_ERROR;
    UsageMap::iterator iter;

    lock->lock();

    for (iter = list->begin(); iter != list->end() ; iter++) {
        keylist->push_back(iter->first);
    }

    lock->unlock();
    return ret;
}

status_t ReorderMCSCHandler::setNodes(ExynosCameraNode ***node)
{
    status_t ret = NO_ERROR;
    m_node = *node;

    for(int i = 0 ; i < MAX_NODE; i++) {
        if (m_node[i] != NULL)
            CLOGV("setNodes m_node[%d] = %p", i, m_node[i]);
    }

    return ret;
}

status_t ReorderMCSCHandler::setPipeInfo(int pipeId[MAX_NODE], int virtualNodeNum[MAX_NODE])
{
    status_t ret = NO_ERROR;

    m_pipeInfo = pipeId;
    m_virtualNodeNum = virtualNodeNum;

    for(int i = 0; i < MAX_NODE; i++) {
        CLOGV("setPipeInfo pipe[%d]=%d virtualNode[%d]=%d", i, m_pipeInfo[i], i, m_virtualNodeNum[i]);
    }

    return ret;
}

status_t ReorderMCSCHandler::m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status)
{
    status_t ret = NO_ERROR;

    switch (usage) {
    case PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ:
    {
        ExynosCameraBuffer buffer;
        struct camera2_shot_ext shot_ext;
        bool enable = false;
        bool needLock = false;
        int curCount = 0;
        int maxCount = 0;
        int maxMcscCount = 0;
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
        bool request[VIRTUAL_MCSC_MAX];
        maxMcscCount = VIRTUAL_MCSC_MAX;
#else
        bool request[MCSC_PORT_MAX];
        maxMcscCount = MCSC_PORT_MAX;
#endif


        int requestCnt = 0;
        int32_t dsport = MCSC_PORT_NONE;
        int pipeId = PIPE_MCSC0_REPROCESSING;

        newFrame->backupRequest(REQUEST_BACKUP_MODE_REORDER);
        newFrame->getMetaData(&shot_ext);
        dsport = shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS];

        for(int i = 0; i < maxMcscCount; i++) {
            request[i] = newFrame->getRequest(PIPE_MCSC0_REPROCESSING+i);
            if (request[i] == true) {
                requestCnt++;
                if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                    m_qWaitingPortList.push_back(i);
                    CLOGV("[F%d] m_qWaitingPortList.push_back(%d), size(%d)", newFrame->getFrameCount(), i, m_qWaitingPortList.size());
                }
            }
        }

        enable = true;

        if (m_parameter->getNumOfMcscOutputPorts() == 1) {
            curCount = 1;
            maxCount = (int)(requestCnt / m_parameter->getNumOfMcscOutputPorts());
            if (requestCnt > m_parameter->getNumOfMcscOutputPorts()
                || request[MCSC_PORT_3] == true
                || request[MCSC_PORT_4] == true) {
                needLock = true;
            }
        } else {
            if (requestCnt > 3) {
                curCount = 1;
                maxCount = 2;
                needLock = true;
            } else {
                if (request[MCSC_PORT_1] == true || request[MCSC_PORT_2] == true) {
                    needLock = true;
                }
            }
        }
        CLOGD("requestCnt(%d) count(%d/%d) enable(%d) needLock(%d) dsport(%d)", requestCnt, maxCount, curCount, enable, needLock, dsport);

        newFrame->setMode(FRAME_MODE_REORDER, enable);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_CUR, curCount);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_MAX, maxCount);
        newFrame->setModeValue(FRAME_MODE_VALUE_DSPORT, dsport);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_LOCK, (needLock == true)?1:0);

        if (enable) {
            CLOGD("frame(%d) original MCSC request(%d %d %d %d %d %d %d)",
                newFrame->getFrameCount(),
                newFrame->getRequest(pipeId+0),
                newFrame->getRequest(pipeId+1),
                newFrame->getRequest(pipeId+2),
                newFrame->getRequest(pipeId+3),
                newFrame->getRequest(pipeId+4),
                newFrame->getRequest(pipeId+5),
                newFrame->getRequest(pipeId+6));
        }

        newFrame->setMetaData(&shot_ext);

        if (curCount != maxCount) {
            m_inputFrameQ->pushProcessQ(&newFrame);
        }
    }
        break;
    case PIPE_HANDLER::USAGE_PRE_QBUF:
    {

        /* 1. init backup request. */

        /* 2-1. set 1st request parameter */

        /* 2-2. set 2nd request parameter */

        status_t ret = NO_ERROR;
        int32_t dsport = MCSC_PORT_NONE;
        ExynosCameraBuffer buffer;
        struct camera2_shot_ext shot_ext;
        camera2_node_group node_group_info;
        camera2_virtual_node_group virtual_node_group_info;
        int perframePosition = 0;
        bool dsRequest = false;
        int perframeIdx = -1;
        int pipeId = PIPE_MCSC0_REPROCESSING;
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;

        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);
        if (supportLock > 0) {
            m_iterationLock.lock();
            CLOGD("Iteration Lock frame(%d) usage(%d)", newFrame->getFrameCount(), usage);
        }
        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        dsport = newFrame->getModeValue(FRAME_MODE_VALUE_DSPORT);

        CLOGD("frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);

        newFrame->getNodeGroupInfo(&node_group_info, m_perframeIndex);
        newFrame->getVirtualNodeInfo(&virtual_node_group_info, m_perframeIndex);

        list<int> portList;
        list<int>::iterator iter;

        list<int> hwPortList;
        list<int>::iterator hwiter;

#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
        bool request[VIRTUAL_MCSC_MAX];
#else
        bool request[MCSC_PORT_MAX+1];
#endif
        int offset = -1;
        int hwport = -1;

        portList.push_back(MCSC_PORT_MAX);
        portList.push_back(MCSC_PORT_0);
        portList.push_back(MCSC_PORT_3);
        portList.push_back(MCSC_PORT_4);
        portList.push_back(MCSC_PORT_1);
        portList.push_back(MCSC_PORT_2);
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
        portList.push_back(VIRTUAL_MCSC_PORT_6);
#endif
        if (mode) {
            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                hwPortList.push_back(MCSC_PORT_0);
            } else {
                hwPortList.push_back(MCSC_PORT_0);
                hwPortList.push_back(MCSC_PORT_3);
                hwPortList.push_back(MCSC_PORT_4);
            }
            /* 1. getRequest and setRequest(false) */
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                request[offset] = newFrame->getBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+offset);
                newFrame->setRequest(pipeId+offset, false);
            }

            /* 2. set request each scenario
                - 1st request / 2nd request.
                - DS port.
            */
            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                if (m_qWaitingPortList.empty() == false) {
                    int portId = m_qWaitingPortList.front();
                    m_dqWaitingPortList.push_back(portId);
                    m_qWaitingPortList.pop_front();
                    CLOGV("[F%d] m_qWaitingPortList.front() %d, size() %d", newFrame->getFrameCount(), portId, m_qWaitingPortList.size());
                    for(int i = 0; i < MCSC_PORT_MAX; i++) {
                        if (portId != i)
                            request[i] = false;
                    }
                }
            } else {
                if (maxCount > 1) {
                    if(curCount != maxCount) {
                        /* 1st frame */
                        request[MCSC_PORT_1] = false;
                        request[MCSC_PORT_2] = false;
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
                        request[VIRTUAL_MCSC_PORT_6] = false;
#endif
                        switch(dsport) {
                            case MCSC_PORT_0:
                            case MCSC_PORT_1:
                                request[MCSC_PORT_MAX] = false;
                                break;
                        }
                    } else {
                        /* 2nd frame */
                        request[MCSC_PORT_0] = false;
                        request[MCSC_PORT_3] = false;
                        request[MCSC_PORT_4] = false;

                        switch(dsport) {
                            case MCSC_PORT_0:
                            case MCSC_PORT_3:
                            case MCSC_PORT_4:
                                request[MCSC_PORT_MAX] = false;
                                break;
                        }
                        newFrame->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, false);
                        newFrame->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, false);
                        newFrame->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, false);
                        newFrame->setRequest(PIPE_HWFC_THUMB_DST_REPROCESSING, false);
                    }
                }
            }

            /* 3. get metadata for update */
            newFrame->getMetaData(&shot_ext);

            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (request[offset]) {
                    switch(offset) {
                    case MCSC_PORT_0:
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        hwiter = find(hwPortList.begin(), hwPortList.end(), offset);
                        if (hwiter != hwPortList.end()) {
                            hwiter = hwPortList.erase(hwiter);
                        }
                        if (dsport == offset) {
                            shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                        }
                        node_group_info.capture[perframeIdx].vid = m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;

                        newFrame->setRequest(pipeId+offset, request[offset]);
                        break;
                    case MCSC_PORT_3:
                    case MCSC_PORT_4:
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                            /* dynamic reorder port : skipped */
                            if (hwPortList.empty() == false) {
                                hwport = hwPortList.front();
                                hwPortList.pop_front();
                            }
                            m_node[getId(pipeId+offset)] = m_node[getId(pipeId+hwport)];
                        } else {
                            hwiter = find(hwPortList.begin(), hwPortList.end(), offset);
                            if (hwiter != hwPortList.end()) {
                                hwiter = hwPortList.erase(hwiter);
                            }
                        }

                        if (dsport == offset) {
                            dsport -= MCSC_PORT_2;
                            shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                        }
                        node_group_info.capture[perframeIdx].vid = m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                        newFrame->setRequest(pipeId+offset, request[offset]);
                        CLOGV("[F%d] perframeIdx(%d), vid(%d), setRequest(pipeId(%d), %d)",
                                newFrame->getFrameCount(), perframeIdx, node_group_info.capture[perframeIdx].vid, pipeId+offset, request[offset]);
                        break;
                    case MCSC_PORT_MAX: /* for ds port */
                        /* ds port reordering */
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        node_group_info.capture[perframeIdx].vid =  m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                        newFrame->setRequest(pipeId+offset, request[offset]);
                        break;
                    case MCSC_PORT_1:
                    case MCSC_PORT_2:
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
                    case VIRTUAL_MCSC_PORT_6:
#endif
                        /* dynamic reorder port : skipped */
                        if (hwPortList.empty() == false) {
                            hwport = hwPortList.front();
                            hwPortList.pop_front();
                        }

                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        if (dsport == offset) {
                            dsport = hwport;

                            switch(hwport) {
                            case MCSC_PORT_0:
                                /* reserved hw port 0 : */
                                break;
                            case MCSC_PORT_3:
                            case MCSC_PORT_4:
                                /* reserved hw port 3,4 : */
                                dsport = dsport- MCSC_PORT_2;
                                break;
                            }
                            shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                        }

                        m_node[getId(pipeId+offset)] = m_node[getId(pipeId+hwport)];
                        node_group_info.capture[perframeIdx].vid =  m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                        newFrame->setRequest(pipeId+offset, request[offset]);
                        break;
                    default:
                        break;
                    }

                    CLOGD("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                } else {
                    perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);
                    node_group_info.capture[perframeIdx].vid = 0;
                    CLOGD("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                }
            }

            /* It is driver requirement
             * - Perframe vid must fill request 0, Driver control DMA disable that reference the vid and request 0.
            */
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (request[offset] == false) {
                    switch(offset) {
                    case MCSC_PORT_0:
                    case MCSC_PORT_3:
                    case MCSC_PORT_4:
                    case MCSC_PORT_1:
                    case MCSC_PORT_2:
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
                    case VIRTUAL_MCSC_PORT_6:
#endif
                        /* dynamic reorder port : skipped */
                        if (hwPortList.empty() == false) {
                            hwport = hwPortList.front();
                            if (m_node[getId(pipeId+offset)]->getNodeNum() == m_node[getId(pipeId+hwport)]->getNodeNum()) {
                                perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);
                                node_group_info.capture[perframeIdx].vid =  m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                                hwPortList.pop_front();
                            }
                        }

                        break;
                    case MCSC_PORT_MAX: /* for ds port */
                    default:
                        break;
                    }

                    CLOGD("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                }
            }

            newFrame->setMetaData(&shot_ext);
        }

        newFrame->storeNodeGroupInfo(&node_group_info, m_perframeIndex);

        CLOGD("frame(%d) MCSC request(%d %d %d %d %d %d %d)",
            newFrame->getFrameCount(),
            newFrame->getRequest(pipeId+0),
            newFrame->getRequest(pipeId+1),
            newFrame->getRequest(pipeId+2),
            newFrame->getRequest(pipeId+3),
            newFrame->getRequest(pipeId+4),
            newFrame->getRequest(pipeId+5),
            newFrame->getRequest(pipeId+6));

        for (int i = 0 ; i < CAPTURE_NODE_MAX ; i++) {
            CLOGV("capture[%d] = vid(%d) request(%d)", i, node_group_info.capture[i].vid, node_group_info.capture[i].request);
        }

    }
        break;
    case PIPE_HANDLER::USAGE_POST_QBUF_ERR:
    {
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;

        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);

        status_t ret = NO_ERROR;
        if (mode) {
            newFrame->restoreRequest(REQUEST_BACKUP_MODE_REORDER);
        }

        if (supportLock > 0) {
            CLOGD("IterationUnLock frame(%d) usage(%d)", newFrame->getFrameCount(), usage);
            m_iterationLock.unlock();
        }

        CLOGE("QBUF error frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);
    }
        break;
    case PIPE_HANDLER::USAGE_POST_DQBUF:
    {
        ExynosCameraBuffer buffer;
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;
        int dsport = -1;
        bool lastFrame = false;
        struct camera2_shot_ext *shot_ext = NULL;

        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);
        dsport = newFrame->getModeValue(FRAME_MODE_VALUE_DSPORT);
        status_t ret = NO_ERROR;

        if (mode) {
            list<int> portList;
            list<int>::iterator iter;

            int pipeId = PIPE_MCSC0_REPROCESSING;

            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                if (m_dqWaitingPortList.empty() == false) {
                    int portId = m_dqWaitingPortList.front();
                    m_dqWaitingPortList.pop_front();
                    portList.push_back(portId);
                    CLOGV("[F%d] m_dqWaitingPortList.front() %d, size() %d",
                            newFrame->getFrameCount(), portId, m_dqWaitingPortList.size());
                    if(curCount < maxCount) {
                        status = PIPE_HANDLER::QUERY_SKIP_OUTPUT;
                        curCount++;
                    } else if (curCount == maxCount) {
                        lastFrame = true;
                    }
                } else {
                    portList.push_back(MCSC_PORT_0);
                    portList.push_back(MCSC_PORT_3);
                    portList.push_back(MCSC_PORT_4);
                    lastFrame = true;

                }
            } else {
                if (maxCount > 1) {
                    if (curCount == 1) {
                        status = PIPE_HANDLER::QUERY_SKIP_OUTPUT;
                        portList.push_back(MCSC_PORT_0);
                        portList.push_back(MCSC_PORT_3);
                        portList.push_back(MCSC_PORT_4);
                        curCount = 2;
                    } else if(curCount == 2) {
                        portList.push_back(MCSC_PORT_1);
                        portList.push_back(MCSC_PORT_2);
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
                        portList.push_back(VIRTUAL_MCSC_PORT_6);
#endif
                        lastFrame = true;
                    }
                } else {
                    portList.push_back(MCSC_PORT_0);
                    portList.push_back(MCSC_PORT_3);
                    portList.push_back(MCSC_PORT_4);
                    portList.push_back(MCSC_PORT_1);
                    portList.push_back(MCSC_PORT_2);
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
                    portList.push_back(VIRTUAL_MCSC_PORT_6);
#endif
                    lastFrame = true;
                }
            }

            int offset = -1;
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (dsport == offset) {
                    newFrame->setBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+MCSC_PORT_MAX, newFrame->getRequest(pipeId+MCSC_PORT_MAX));
                }
                newFrame->setBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+offset, newFrame->getRequest(pipeId+offset));
            }

            newFrame->restoreRequest(REQUEST_BACKUP_MODE_REORDER);

            newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_CUR, curCount);
            if (lastFrame) {
                /* restore DS port for last frame */
                ret = newFrame->getSrcBuffer(m_pipeId, &buffer);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CLOGE("[F%d B%d]Failed to getSrcBuffer. pipeId %d, ret %d",
                            newFrame->getFrameCount(), buffer.index, m_pipeId, ret);
                }
                shot_ext = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];
                shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;

                struct camera2_shot_ext frame_shot_ext;
                newFrame->getMetaData(&frame_shot_ext);
                frame_shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                newFrame->setMetaData(&frame_shot_ext);
            }
        }

        if (supportLock > 0) {
            CLOGD("IterationUnLock frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);
            m_iterationLock.unlock();
        }
    }
        break;
    default:
        break;
    }
    return ret;
}

int32_t ReorderMCSCHandler::getId(int pipeId)
{
    int32_t ret = INVALID_NODE;

    for(int i = OUTPUT_NODE ; i < MAX_NODE ; i++) {
        if (m_pipeInfo[i] == pipeId) {
            ret = i;
        }
    }

    return ret;
}

int ReorderMCSCHandler::getPerframeIdx(int32_t index, camera2_virtual_node_group &node_group_info)
{
    int ret = -1;
    if (m_node[index] != NULL) {
        uint32_t videoId = m_virtualNodeNum[index] - FIMC_IS_VIDEO_BAS_NUM;
        for (int perframePosition = 0; perframePosition < CAPTURE_NODE_MAX; perframePosition++) {
            if (node_group_info.virtualVid[perframePosition]== videoId) {
                ret = perframePosition;
                break;
            }
        }
    }
    return ret;
}

status_t ReorderMCSCHandlerPreview::setNodes(ExynosCameraNode ***node)
{
    status_t ret = NO_ERROR;
    m_node = *node;

    for(int i = 0 ; i < MAX_NODE; i++) {
        if (m_node[i] != NULL)
            CLOGV("setNodes m_node[%d] = %p", i, m_node[i]);
    }

    return ret;
}

status_t ReorderMCSCHandlerPreview::setPipeInfo(int pipeId[MAX_NODE], int virtualNodeNum[MAX_NODE])
{
    status_t ret = NO_ERROR;

    m_pipeInfo = pipeId;
    m_virtualNodeNum = virtualNodeNum;

    for(int i = 0; i < MAX_NODE; i++) {
        CLOGV("setPipeInfo pipe[%d]=%d virtualNode[%d]=%d", i, m_pipeInfo[i], i, m_virtualNodeNum[i]);
    }

    return ret;
}

status_t ReorderMCSCHandlerPreview::m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status)
{
    status_t ret = NO_ERROR;

    switch (usage) {
    case PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ:
    {
        ExynosCameraBuffer buffer;
        struct camera2_shot_ext shot_ext;
        bool enable = false;
        bool needLock = false;
        int curCount = 0;
        int maxCount = 0;
        int maxMcscCount = 0;
        bool request[MCSC_PORT_MAX];
        maxMcscCount = MCSC_PORT_MAX;

        int requestCnt = 0;
        int32_t dsport = MCSC_PORT_NONE;
        int pipeId = PIPE_MCSC0;

        newFrame->backupRequest(REQUEST_BACKUP_MODE_REORDER);
        newFrame->getMetaData(&shot_ext);
        dsport = shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS];

        for(int i = 0; i < maxMcscCount; i++) {
            request[i] = newFrame->getRequest(PIPE_MCSC0+i);
            if (request[i] == true) {
                requestCnt++;
                if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                    Mutex::Autolock lock(m_qWaitingPortListLock);
                    m_qWaitingPortList.push_back(i);
                    CLOGV("[F%d] m_qWaitingPortList.push_back(%d), size(%d)", newFrame->getFrameCount(), i, m_qWaitingPortList.size());
                }
            }
        }

        enable = true;

        if (m_parameter->getNumOfMcscOutputPorts() == 1) {
            curCount = 1;
            maxCount = (int)(requestCnt / m_parameter->getNumOfMcscOutputPorts());
            if (requestCnt > m_parameter->getNumOfMcscOutputPorts()
                || request[MCSC_PORT_1] == true
                || request[MCSC_PORT_2] == true) {
                needLock = true;
            }
        }
        CLOGV("requestCnt(%d) count(%d/%d) enable(%d) needLock(%d) dsport(%d)", requestCnt, maxCount, curCount, enable, needLock, dsport);

        newFrame->setMode(FRAME_MODE_REORDER, enable);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_CUR, curCount);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_MAX, maxCount);
        newFrame->setModeValue(FRAME_MODE_VALUE_DSPORT, dsport);
        newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_LOCK, (needLock == true)?1:0);

        if (enable) {
            CLOGV("frame(%d) original MCSC request(%d %d %d %d %d %d %d)",
                newFrame->getFrameCount(),
                newFrame->getRequest(pipeId+0),
                newFrame->getRequest(pipeId+1),
                newFrame->getRequest(pipeId+2),
                newFrame->getRequest(pipeId+3),
                newFrame->getRequest(pipeId+4),
                newFrame->getRequest(pipeId+5),
                newFrame->getRequest(pipeId+6));
        }

        newFrame->setMetaData(&shot_ext);

        if (curCount != maxCount) {
            m_inputFrameQ->pushProcessQ(&newFrame);
        }
    }
        break;
    case PIPE_HANDLER::USAGE_PRE_QBUF:
    {

        /* 1. init backup request. */

        /* 2-1. set 1st request parameter */

        /* 2-2. set 2nd request parameter */

        status_t ret = NO_ERROR;
        int32_t dsport = MCSC_PORT_NONE;
        ExynosCameraBuffer buffer;
        struct camera2_shot_ext shot_ext;
        camera2_node_group node_group_info;
        camera2_virtual_node_group virtual_node_group_info;
        int perframePosition = 0;
        bool dsRequest = false;
        int perframeIdx = -1;
        int pipeId = PIPE_MCSC0;
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;

        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);
        if (supportLock > 0) {
            m_iterationLock.lock();
            CLOGV("Iteration Lock frame(%d) usage(%d)", newFrame->getFrameCount(), usage);
        }
        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        dsport = newFrame->getModeValue(FRAME_MODE_VALUE_DSPORT);

        CLOGV("frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);

        newFrame->getNodeGroupInfo(&node_group_info, m_perframeIndex);
        newFrame->getVirtualNodeInfo(&virtual_node_group_info, m_perframeIndex);

        list<int> portList;
        list<int>::iterator iter;

        list<int> hwPortList;
        list<int>::iterator hwiter;

        bool request[MCSC_PORT_MAX+1];
        int offset = -1;
        int hwport = -1;

        portList.push_back(MCSC_PORT_MAX);
        portList.push_back(MCSC_PORT_0);
        portList.push_back(MCSC_PORT_1);
        portList.push_back(MCSC_PORT_2);
        if (mode) {
            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                hwPortList.push_back(MCSC_PORT_0);
            }

            /* 1. getRequest and setRequest(false) */
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                request[offset] = newFrame->getBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+offset);
                newFrame->setRequest(pipeId+offset, false);
            }

            /* 2. set request each scenario
                - 1st request / 2nd request.
                - DS port.
            */
            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                Mutex::Autolock lock(m_qWaitingPortListLock);
                if (m_qWaitingPortList.empty() == false) {
                    int portId = m_qWaitingPortList.front();
                    Mutex::Autolock lock(m_dqWaitingPortListLock);
                    m_dqWaitingPortList.push_back(portId);
                    m_qWaitingPortList.pop_front();
                    CLOGV("[F%d] m_qWaitingPortList.front() %d, size: %d", newFrame->getFrameCount(), portId, m_qWaitingPortList.size());
                    for(int i = 0; i < MCSC_PORT_MAX; i++) {
                        if (portId != i)
                            request[i] = false;
                    }
                }
            }

            /* 3. get metadata for update */
            newFrame->getMetaData(&shot_ext);

            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (request[offset]) {
                    switch(offset) {
                    case MCSC_PORT_0:
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        hwiter = find(hwPortList.begin(), hwPortList.end(), offset);
                        if (hwiter != hwPortList.end()) {
                            hwiter = hwPortList.erase(hwiter);
                        }
                        if (dsport == offset) {
                            shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                        }
                        node_group_info.capture[perframeIdx].vid = m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;

                        newFrame->setRequest(pipeId+offset, request[offset]);
                        break;
                    case MCSC_PORT_1:
                    case MCSC_PORT_2:
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                            /* dynamic reorder port : skipped */
                            if (hwPortList.empty() == false) {
                                hwport = hwPortList.front();
                                hwPortList.pop_front();
                            }
                            m_node[getId(pipeId+offset)] = m_node[getId(pipeId+hwport)];
                        } else {
                            hwiter = find(hwPortList.begin(), hwPortList.end(), offset);
                            if (hwiter != hwPortList.end()) {
                                hwiter = hwPortList.erase(hwiter);
                            }
                        }

                        if (dsport == offset) {
                            dsport -= MCSC_PORT_2;
                            shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                        }
                        node_group_info.capture[perframeIdx].vid = m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                        newFrame->setRequest(pipeId+offset, request[offset]);
                        CLOGV("[F%d] perframeIdx(%d), vid(%d), setRequest(pipeId(%d), %d)",
                                newFrame->getFrameCount(), perframeIdx, node_group_info.capture[perframeIdx].vid,
                                pipeId+offset, request[offset]);
                        break;
                    case MCSC_PORT_MAX: /* for ds port */
                        /* ds port reordering */
                        perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);

                        node_group_info.capture[perframeIdx].vid =  m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                        newFrame->setRequest(pipeId+offset, request[offset]);
                        break;
                    default:
                        break;
                    }

                    CLOGV("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                } else {
                    perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);
                    node_group_info.capture[perframeIdx].vid = 0;
                    CLOGV("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                }
            }

            /* It is driver requirement
             * - Perframe vid must fill request 0, Driver control DMA disable that reference the vid and request 0.
            */
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (request[offset] == false) {
                    switch(offset) {
                    case MCSC_PORT_0:
                    case MCSC_PORT_1:
                    case MCSC_PORT_2:
                        /* dynamic reorder port : skipped */
                        if (hwPortList.empty() == false) {
                            hwport = hwPortList.front();
                            if (m_node[getId(pipeId+offset)]->getNodeNum() == m_node[getId(pipeId+hwport)]->getNodeNum()) {
                                perframeIdx = getPerframeIdx(getId(pipeId+offset), virtual_node_group_info);
                                node_group_info.capture[perframeIdx].vid =  m_node[getId(pipeId+offset)]->getNodeNum() - FIMC_IS_VIDEO_BAS_NUM;
                                hwPortList.pop_front();
                            }
                        }

                        break;
                    case MCSC_PORT_MAX: /* for ds port */
                    default:
                        break;
                    }

                    CLOGV("frame(%d) usage(%d) getId(pipeId(%d) + offset(%d)) = %d perframeIdx(%d) vid(%d) dsport(%d) request(%d)", newFrame->getFrameCount(), usage, pipeId, offset, getId(pipeId+offset), perframeIdx, node_group_info.capture[perframeIdx].vid, dsport, request[offset]);
                }
            }

            newFrame->setMetaData(&shot_ext);
        }

        newFrame->storeNodeGroupInfo(&node_group_info, m_perframeIndex);

        CLOGV("frame(%d) MCSC request(%d %d %d %d %d %d %d)",
            newFrame->getFrameCount(),
            newFrame->getRequest(pipeId+0),
            newFrame->getRequest(pipeId+1),
            newFrame->getRequest(pipeId+2),
            newFrame->getRequest(pipeId+3),
            newFrame->getRequest(pipeId+4),
            newFrame->getRequest(pipeId+5),
            newFrame->getRequest(pipeId+6));

        for (int i = 0 ; i < CAPTURE_NODE_MAX ; i++) {
            CLOGV("capture[%d] = vid(%d) request(%d)", i, node_group_info.capture[i].vid, node_group_info.capture[i].request);
        }

    }
        break;
    case PIPE_HANDLER::USAGE_POST_QBUF_ERR:
    {
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;

        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);

        status_t ret = NO_ERROR;
        if (mode) {
            newFrame->restoreRequest(REQUEST_BACKUP_MODE_REORDER);
        }

        if (supportLock > 0) {
            CLOGV("IterationUnLock frame(%d) usage(%d)", newFrame->getFrameCount(), usage);
            m_iterationLock.unlock();
        }

        CLOGE("QBUF error frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);
    }
        break;
    case PIPE_HANDLER::USAGE_POST_DQBUF:
    {
        ExynosCameraBuffer buffer;
        int curCount = 0;
        int maxCount = 0;
        bool mode = false;
        int supportLock = 0;
        int dsport = -1;
        bool lastFrame = false;
        struct camera2_shot_ext *shot_ext = NULL;

        mode = newFrame->getMode(FRAME_MODE_REORDER);
        curCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_CUR);
        maxCount = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_MAX);
        supportLock = newFrame->getModeValue(FRAME_MODE_VALUE_REORDER_LOCK);
        dsport = newFrame->getModeValue(FRAME_MODE_VALUE_DSPORT);
        status_t ret = NO_ERROR;

        if (mode) {
            list<int> portList;
            list<int>::iterator iter;

            int pipeId = PIPE_MCSC0;

            if (m_parameter->getNumOfMcscOutputPorts() == 1) {
                Mutex::Autolock lock(m_dqWaitingPortListLock);
                if (m_dqWaitingPortList.empty() == false) {
                    int portId = m_dqWaitingPortList.front();
                    m_dqWaitingPortList.pop_front();
                    portList.push_back(portId);
                    CLOGV("[F%d] m_dqWaitingPortList.front() %d, size: %d", newFrame->getFrameCount(), portId, m_dqWaitingPortList.size());
                    if(curCount < maxCount) {
                        status = PIPE_HANDLER::QUERY_SKIP_OUTPUT;
                        curCount++;
                    } else if (curCount == maxCount) {
                        lastFrame = true;
                    }
                } else {
                    portList.push_back(MCSC_PORT_0);
                    portList.push_back(MCSC_PORT_1);
                    portList.push_back(MCSC_PORT_2);
                    lastFrame = true;

                }
            }

            int offset = -1;
            for(iter = portList.begin() ; iter != portList.end(); iter++) {
                offset = *iter;
                if (dsport == offset) {
                    newFrame->setBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+MCSC_PORT_MAX, newFrame->getRequest(pipeId+MCSC_PORT_MAX));
                }
                newFrame->setBackupRequest(REQUEST_BACKUP_MODE_REORDER, pipeId+offset, newFrame->getRequest(pipeId+offset));
            }

            newFrame->restoreRequest(REQUEST_BACKUP_MODE_REORDER);

            newFrame->setModeValue(FRAME_MODE_VALUE_REORDER_CUR, curCount);
            if (lastFrame) {
                /* restore DS port for last frame */
                ret = newFrame->getSrcBuffer(m_pipeId, &buffer);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CLOGE("[F%d B%d]Failed to getSrcBuffer. pipeId %d, ret %d",
                            newFrame->getFrameCount(), buffer.index, m_pipeId, ret);
                }
                shot_ext = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];
                shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;

                struct camera2_shot_ext frame_shot_ext;
                newFrame->getMetaData(&frame_shot_ext);
                frame_shot_ext.shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port)dsport;
                newFrame->setMetaData(&frame_shot_ext);
            }
        }

        if (supportLock > 0) {
            CLOGV("IterationUnLock frame(%d) usage(%d) reorder mode(%d) count(%d / %d) supportLock(%d)", newFrame->getFrameCount(), usage, mode, maxCount, curCount, supportLock);
            m_iterationLock.unlock();
        }
    }
        break;
    default:
        break;
    }
    return ret;
}


int32_t ReorderMCSCHandlerPreview::getId(int pipeId)
{
    int32_t ret = INVALID_NODE;

    for(int i = OUTPUT_NODE ; i < MAX_NODE ; i++) {
        if (m_pipeInfo[i] == pipeId) {
            ret = i;
        }
    }

    return ret;
}

int ReorderMCSCHandlerPreview::getPerframeIdx(int32_t index, camera2_virtual_node_group &node_group_info)
{
    int ret = -1;
    if (m_node[index] != NULL) {
        uint32_t videoId = m_virtualNodeNum[index] - FIMC_IS_VIDEO_BAS_NUM;
        for (int perframePosition = 0; perframePosition < CAPTURE_NODE_MAX; perframePosition++) {
            if (node_group_info.virtualVid[perframePosition]== videoId) {
                ret = perframePosition;
                break;
            }
        }
    }
    return ret;
}

}; /* namespace android */
