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

#ifndef EXYNOS_CAMERA_MCPIPE_H
#define EXYNOS_CAMERA_MCPIPE_H

#include "ExynosCameraPipeFlite.h"
#ifdef SUPPORT_POST_SCALER_ZOOM
#include "ExynosCameraPipeGSC.h"
#endif
#include <array>
namespace android {

using namespace std;

class ExynosCameraMCPipe : public ExynosCameraPipeFlite {
public:
    ExynosCameraMCPipe()
    {
        m_init(NULL);
    }

    ExynosCameraMCPipe(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        camera_device_info_t *deviceInfo,
        cameraId_Info *camIdInfo)
        : ExynosCameraPipeFlite(cameraId, configurations, obj_param, isReprocessing, NULL, camIdInfo)
    {
        m_init(deviceInfo);
    }

    virtual ~ExynosCameraMCPipe();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        precreate(int32_t *sensorIds = NULL);
    virtual status_t        postcreate(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);
    virtual status_t        prepare(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual bool            flagStart(void);

    virtual status_t        startThread(void);
    virtual status_t        stopThread(void);
    virtual status_t        stopThreadAndWait(int sleep, int times);

    virtual bool            flagStartThread(void);

    virtual status_t        sensorStream(bool on);
    virtual status_t        sensorStandby(bool on);
    virtual enum SENSOR_STANDBY_STATE getSensorStandbyState(void);
    virtual status_t        forceDone(unsigned int cid, int value);
    virtual status_t        setControl(int cid, int value);
    virtual status_t        getControl(int cid, int *value);
    virtual status_t        setExtControl(struct v4l2_ext_controls *ctrl);
    virtual status_t        setParam(struct v4l2_streamparm streamParam);

    virtual status_t        pushFrame(ExynosCameraFrameSP_dptr_t newFrame);

    virtual status_t        instantOn(int32_t numFrames);
    virtual status_t        instantOff(void);

    virtual status_t        getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition);
    virtual status_t        setPipeId(uint32_t id);
    virtual uint32_t        getPipeId(void);
    virtual status_t        setPipeId(enum NODE_TYPE nodeType, uint32_t id);
    virtual int             getPipeId(enum NODE_TYPE nodeType);

    virtual status_t        setPipeName(const char *pipeName);
    virtual char           *getPipeName(void);

    virtual status_t        clearInputFrameQ(void);
    virtual status_t        getInputFrameQ(frame_queue_t **inputQ);
    virtual status_t        setOutputFrameQ(frame_queue_t *outputQ);
    virtual status_t        getOutputFrameQ(frame_queue_t **outputQ);

    virtual status_t        setBoosting(bool isBoosting);

    virtual bool            isThreadRunning(void);

    virtual status_t        getThreadState(int **threadState);
    virtual status_t        getThreadInterval(uint64_t **timeInterval);
    virtual status_t        getThreadRenew(int **timeRenew);
    virtual status_t        incThreadRenew(void);
    virtual status_t        resetThreadRenew(void);
    virtual status_t        setStopFlag(void);

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    virtual status_t        setHandler(sp<PipeHandler> handler);
    virtual status_t        getHandler(PIPE_HANDLER::USAGE usage, sp<PipeHandler> &handler);
    virtual status_t        getHandler(PIPE_HANDLER::SCENARIO scenario, sp<PipeHandler> &handler);
    virtual status_t        getNodes(ExynosCameraNode **node[MAX_NODE]);
    virtual status_t        setShreadNode(int32_t src, int32_t dst);
#endif

    virtual int             getRunningFrameCount(void);

    virtual void            dump(void);

    /* only for debugging */
    virtual status_t        dumpFimcIsInfo(bool bugOn);
//#ifdef MONITOR_LOG_SYNC
    virtual status_t        syncLog(uint32_t syncId);
//#endif

/* MC Pipe include buffer manager, so FrameFactory(ExynosCamera) must set buffer manager to pipe.
 * Add interface for set buffer manager to pipe.
 */
    virtual status_t        setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier);
    virtual status_t        setBufferManager(ExynosCameraBufferManager **bufferManager);

/* MC Pipe have several nodes. It need to specify nodes.
 * Add interface set/get control with specify nodes.
 */
    virtual status_t        setControl(int cid, int value, enum NODE_TYPE nodeType);
    virtual status_t        getControl(int cid, int *value, enum NODE_TYPE nodeType);
    virtual status_t        setExtControl(struct v4l2_ext_controls *ctrl, enum NODE_TYPE nodeType);

/* Set map buffer is makes node operation faster at first start.
 * Thereby map buffer before start, reduce map buffer time at start.
 * It use first Exynos5430/5433 in old pipe.
 */
    virtual status_t        setMapBuffer(ExynosCameraBuffer *srcBuf = NULL, ExynosCameraBuffer *dstBuf = NULL);

/* MC Pipe have two output queue.
 * If you want push frame to FrameDoneQ in ExynosCamera explicitly, use this interface.
 */

    virtual status_t        setFrameDoneQ(frame_queue_t *frameDoneQ);
    virtual status_t        getFrameDoneQ(frame_queue_t **frameDoneQ);

    virtual status_t        setNodeInfos(camera_node_objects_t *nodeObjects, bool flagReset = false);
    virtual status_t        getNodeInfos(camera_node_objects_t *nodeObjects);

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    virtual void            needSerialization(bool enable);
#endif

    virtual status_t        setDeviceInfo(camera_device_info_t *deviceInfo);

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    virtual status_t        flush(void);
#endif //SUPPORT_SENSOR_MODE_CHANGE

protected:
    virtual bool            m_putBufferThreadFunc(void);
    virtual bool            m_getBufferThreadFunc(void);
#ifdef DEBUG_DUMP_IMAGE
    virtual bool            m_dumpBufferThreadFunc(void);
    virtual int32_t         m_prepareBufferForDump(ExynosCameraBuffer *srcBuffer,
                                                         uint32_t cameraId,
                                                         uint32_t frameCount,
                                                         int32_t pipeId,
                                                         enum NODE_TYPE nodeType,
                                                         camera2_node_group *nodeGroupInfo,
                                                         ExynosCameraFrameSP_sptr_t newFrame);
    virtual int32_t         m_getPerFrameIdxFromCaptureNodeType(camera2_node_group *nodeGroupInfo,
                                                                                int32_t captureNodeType);
    virtual int32_t         m_dumpBufferUtils(ExynosCameraFrameSP_sptr_t newFrame);
    virtual bool            m_handleSequentialRawDump(ExynosCameraFrameSP_sptr_t newFrame);
#endif
    virtual status_t        m_putBuffer(void);
    virtual status_t        m_getBuffer(void);
    virtual status_t        m_frameErrorHandler(ExynosCameraFrameSP_sptr_t newFrame);

    virtual status_t        m_updateMetadataToFrame(void *metadata, int index, ExynosCameraFrameSP_sptr_t frame = NULL, enum NODE_TYPE nodeLocation = OUTPUT_NODE);
    virtual status_t        m_getFrameByIndex(ExynosCameraFrameSP_dptr_t frame, int index, enum NODE_TYPE nodeLocation = OUTPUT_NODE);
    virtual status_t        m_completeFrame(ExynosCameraFrameSP_sptr_t frame, bool isValid = true);

    virtual status_t        m_setInput(ExynosCameraNode *nodes[], int32_t *nodeNums, int32_t *sensorIds);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);
    virtual status_t        m_setNodeInfo(ExynosCameraNode *node, camera_pipe_info_t *pipeInfos,
                                          uint32_t planeCount, enum YUV_RANGE yuvRange,
                                          bool flagBayer = false);
    virtual status_t        m_sensorStandby(bool on);
    virtual status_t        m_forceDone(ExynosCameraNode *node, unsigned int cid, int value);

    virtual status_t        m_startNode(void);
    virtual status_t        m_stopNode(void);
    virtual status_t        m_clearNode(void);

    virtual status_t        m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, char *name, camera2_node *oldNode, camera2_node *newNode);
    virtual status_t        m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, char *name, int index, camera2_node *oldNode, camera2_node *newNode);
    virtual status_t        m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, int index, camera2_node *oldNode, camera2_node *newNode);

    virtual void            m_dumpRunningFrameList(void);

    virtual void            m_dumpPerframeNodeGroupInfo(const char *name, camera_pipe_perframe_node_group_info_t nodeInfo);
    virtual void            m_dumpPerframeShotInfo(const char *name, int frameCount, camera2_shot_ext *shot_ext);

    virtual void            m_configDvfs(void);
    virtual bool            m_flagValidInt(int num);
    virtual bool            m_checkThreadLoop(frame_queue_t *frameQ);

    virtual status_t        m_preCreate(void);
    virtual status_t        m_postCreate(int32_t *sensorIds = NULL);

    virtual status_t        m_checkShotDone(struct camera2_shot_ext *shot_ext);
    virtual status_t        m_updateMetadataFromFrame(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer *buffer, int perframeInfoIndex = -1);

    virtual status_t        m_getPerframePosition(int *perframePosition, uint32_t pipeId);

    virtual status_t        m_setSetfile(ExynosCameraNode *node, uint32_t pipeId);

    virtual status_t        m_setMapBuffer(int nodeIndex);
    virtual status_t        m_setMapBuffer(ExynosCameraNode *node, ExynosCameraBuffer *buffer);

    virtual status_t        m_setJpegInfo(int nodeType, ExynosCameraFrameSP_sptr_t frame);

    status_t                m_checkPolling(ExynosCameraNode *node);

private:
    void                    m_init(camera_device_info_t *deviceInfo);

    status_t                m_createSensorNode(int32_t *sensorIds);

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    int                     m_getRunningFrameMinCount(void);
#endif //SUPPORT_SENSOR_MODE_CHANGE

#ifdef SUPPORT_POST_SCALER_ZOOM
    void                    m_handlePostScalerZoom(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer *srcBuffer,
                                    int i);

    int                     m_postScalerZoom(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer srcBuffer,
                                    ExynosCameraBuffer *dstBuffer,
                                    ExynosRect srcRect,
                                    ExynosRect dstRect);
#endif
    void                    m_setSizeInfo(ExynosCameraFrameSP_sptr_t frame, camera2_shot_ext *shot_ext);

protected:
    typedef ExynosCameraThread<ExynosCameraMCPipe> MCPipeThread;
    sp<MCPipeThread>            m_putBufferThread;
    sp<MCPipeThread>            m_getBufferThread;
    String8                     m_putBufferThreadName;
    String8                     m_getBufferThreadName;
#ifdef DEBUG_DUMP_IMAGE
    sp<MCPipeThread>            m_dumpBufferThread;
    uint32_t                    m_dumpBufferCount;
#endif
    array<array<ExynosCameraFrameSP_sptr_t, MAX_BUFFERS>, MAX_NODE> m_runningFrameList;
    uint32_t                    m_numOfRunningFrame[MAX_NODE];

    uint32_t                    m_pipeIdArr[MAX_NODE];
    uint32_t                    m_numBuffers[MAX_NODE];

    camera_pipe_perframe_node_group_info_t m_perframeMainNodeGroupInfo[MAX_NODE];

    camera_device_info_t       *m_deviceInfo;

    frame_queue_t              *m_requestFrameQ;
#ifdef DEBUG_DUMP_IMAGE
    ExynosCameraList<ExynosCameraImageDumpInfo_t> *m_dumpImageQ;
    frame_queue_t               m_RawFrameHoldList;
    uint32_t                    m_sequentialDumpCount;
#endif

    ExynosCameraBuffer          m_skipBuffer[MAX_NODE];
    bool                        m_skipPutBuffer[MAX_NODE];
    bool                        m_skipBufferHint;

    Mutex                       m_sensorStandbyLock;
    Mutex                       m_putBufferLock;
    Mutex                       m_runningFrameDumpLock;
    enum SENSOR_STANDBY_STATE   m_flagSensorStandby;
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    bool                        m_serializeOperation;
    static Mutex                g_serializationLock[ExynosCameraParameters::CRITICAL_SECTION_TYPE_END];
    void                        m_lockSerializeOperation(enum pipeline pipeId);
    void                        m_unlockSerializeOperation(enum pipeline pipeId);
#endif

    int                         m_sensorNodeIndex;
#ifdef SUPPORT_DEPTH_MAP
    int                         m_depthVcNodeIndex;
#endif // SUPPORT_DEPTH_MAP

    /* for print internal frame log duration */
    int                         m_putInternalFrameLogCnt;
    int                         m_getInternalFrameLogCnt;

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    array<sp<PipeHandler>, PIPE_HANDLER::SCENARIO_MAX> m_handler;
    bool                        m_sharedNode[MAX_NODE];
#endif

#ifdef SUPPORT_POST_SCALER_ZOOM
    sp<class ExynosCameraGSCWrapper>    m_gscWrapper;
#endif
    std::map<int32_t, int32_t>  m_nodeIdToPipeIdMap;
    std::map<int32_t, int32_t>  m_videoIdToPipeIdMap;
};

}; /* namespace android */

#endif
