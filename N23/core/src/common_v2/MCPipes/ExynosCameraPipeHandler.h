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

#ifndef EXYNOS_CAMERA_PIPE_HANDLER_H
#define EXYNOS_CAMERA_PIPE_HANDLER_H

#include <array>
#include <list>
#include <map>
#include <log/log.h>

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraNode.h"
#include "ExynosCameraNodeJpegHAL.h"
#include "ExynosCameraFrame.h"
#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraList.h"
#include "ExynosCameraBufferSupplier.h"

#include "ExynosCameraSizeControl.h"
#include "ExynosCameraTimeLogger.h"

namespace android {

using namespace std;

namespace PIPE_HANDLER {
    enum SCENARIO {
        SCENARIO_BASE = -1,
        SCENARIO_REORDER,
        SCENARIO_MAX,
    };

    enum USAGE {
        USAGE_BASE = -1,
        USAGE_PRE_PUSH_FRAMEQ,
        USAGE_PRE_QBUF,
        USAGE_POST_QBUF_ERR,
        USAGE_POST_DQBUF,
        USAGE_MAX,
    };

    enum QUERY {
        QUERY_BASE = -1,
        QUERY_SKIP_OUTPUT, /* skip outputQ for iteration. */
        QUERY_MAX,
    };

};

typedef sp<ExynosCameraFrame>  ExynosCameraFrameSP_sptr_t; /* single ptr */
typedef ExynosCameraList<ExynosCameraFrameSP_sptr_t> frame_queue_t;
typedef list<PIPE_HANDLER::USAGE> UsageList;

class PipeHandler : public RefBase {
private:
    typedef map<PIPE_HANDLER::USAGE, bool>  UsageMap;
    typedef pair<PIPE_HANDLER::USAGE, bool> UsagePair;

public:
    PipeHandler(
        int cameraId,
        uint32_t pipeId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        PIPE_HANDLER::SCENARIO scenario);

    ~PipeHandler();

public:

    status_t setName(const char *name);
    virtual PIPE_HANDLER::SCENARIO getScenario() final;

    virtual status_t    process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status);
    virtual status_t    addUsage(PIPE_HANDLER::USAGE usage) final;
    virtual bool        isUsage(PIPE_HANDLER::USAGE usage) final;
    virtual status_t    setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier) final;
    virtual status_t    setPerframeIndex(int index);
    virtual status_t    setOutputFrameQ(frame_queue_t *queue);
    virtual status_t    setInputFrameQ(frame_queue_t *queue);
    virtual status_t    setFrameDoneQ(frame_queue_t *queue);

protected:
    virtual status_t m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status);
private:
    status_t m_pushItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item);
    status_t m_popItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item);
    status_t m_getItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock, bool &item);
    bool m_findItem(PIPE_HANDLER::USAGE key, UsageMap *list, Mutex *lock);
    status_t m_getKeyList(UsageMap *list, Mutex *lock, UsageList *keylist);

protected:
    int         m_cameraId;
    uint32_t    m_pipeId;
    char        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    ExynosCameraConfigurations         *m_configuration;
    ExynosCameraParameters             *m_parameter;
    ExynosCameraBufferSupplier         *m_bufferSupplier;
    PIPE_HANDLER::SCENARIO              m_scenario;
    UsageMap                            m_usageList;
    mutable Mutex                       m_usageListLock;
    frame_queue_t                      *m_inputFrameQ;
    frame_queue_t                      *m_outputFrameQ;
    frame_queue_t                      *m_frameDoneQ;
    int                                 m_perframeIndex;
};

class ReorderMCSCHandler : public PipeHandler {
public:
    ReorderMCSCHandler(
        int cameraId,
        uint32_t pipeId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        PIPE_HANDLER::SCENARIO scenario)
        : PipeHandler(cameraId, pipeId, configurations, obj_param, scenario)
    {
        m_node = NULL;
        m_pipeInfo = NULL;
        m_virtualNodeNum = NULL;
    }

    virtual ~ReorderMCSCHandler()
    {

    }

    virtual status_t setNodes(ExynosCameraNode ***node);
    virtual status_t setPipeInfo(int pipeId[MAX_NODE], int virtualNodeNum[MAX_NODE]);

protected:
    virtual status_t m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status);

    virtual int32_t getId(int pipeId);
    virtual int getPerframeIdx(int32_t index, camera2_virtual_node_group &node_group_info);

    ExynosCameraNode          **m_node;
    int                        *m_pipeInfo;
    int                        *m_virtualNodeNum;
    mutable Mutex               m_iterationLock;
    list<int>                   m_qWaitingPortList;
    list<int>                   m_dqWaitingPortList;
    mutable Mutex               m_qWaitingPortListLock;
    mutable Mutex               m_dqWaitingPortListLock;
};

class ReorderMCSCHandlerPreview : public PipeHandler {
public:
    ReorderMCSCHandlerPreview(
        int cameraId,
        uint32_t pipeId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        PIPE_HANDLER::SCENARIO scenario)
        : PipeHandler(cameraId, pipeId, configurations, obj_param, scenario)
    {
        m_node = NULL;
        m_pipeInfo = NULL;
        m_virtualNodeNum = NULL;
    }

    virtual ~ReorderMCSCHandlerPreview()
    {

    }

    virtual status_t setNodes(ExynosCameraNode ***node);
    virtual status_t setPipeInfo(int pipeId[MAX_NODE], int virtualNodeNum[MAX_NODE]);

protected:
    virtual status_t m_process(PIPE_HANDLER::USAGE usage, ExynosCameraFrameSP_sptr_t newFrame, PIPE_HANDLER::QUERY &status);

    virtual int32_t getId(int pipeId);
    virtual int getPerframeIdx(int32_t index, camera2_virtual_node_group &node_group_info);

    ExynosCameraNode          **m_node;
    int                        *m_pipeInfo;
    int                        *m_virtualNodeNum;
    mutable Mutex               m_iterationLock;
    list<int>                   m_qWaitingPortList;
    list<int>                   m_dqWaitingPortList;
    mutable Mutex               m_qWaitingPortListLock;
    mutable Mutex               m_dqWaitingPortListLock;
};
}; /* namespace android */

#endif
