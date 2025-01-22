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

#ifndef EXYNOS_CAMERA_FRAME_H
#define EXYNOS_CAMERA_FRAME_H

#include <map>
#include <unordered_map>

#include <utils/List.h>
#include <array>

#include "ExynosCameraConfigurations.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraList.h"
#include "ExynosCameraNode.h"
#include "ExynosCameraVendorMetaData.h"

typedef ExynosCameraList<uint32_t> frame_key_queue_t;

/* #define DEBUG_FRAME_MEMORY_LEAK */

#ifndef MAX_NUM_PIPES
#define MAX_NUM_PIPES \
    GET_MAX_NUM((MAX_PIPE_NUM % 100),(MAX_PIPE_NUM_FRONT % 100),(MAX_PIPE_NUM_REPROCESSING % 100))
#endif

#ifndef INDEX
#define INDEX(x)            ((x % 100) % MAX_NUM_PIPES)
#endif

namespace android {

using namespace std;

class ExynosCameraVendorMetaData;

/* Frame type for debugging */
typedef enum FRAME_TYPE {
    FRAME_TYPE_BASE                     = 0,
    FRAME_TYPE_OTHERS                   = 1,
    FRAME_TYPE_PREVIEW                  = 2,
    FRAME_TYPE_PREVIEW_FRONT            = 3,
    FRAME_TYPE_REPROCESSING             = 4,
    FRAME_TYPE_VISION                   = 6,
    FRAME_TYPE_INTERNAL                 = 7,
    FRAME_TYPE_PREVIEW_SLAVE            = 8,
    FRAME_TYPE_PREVIEW_DUAL_MASTER      = 9,
    FRAME_TYPE_PREVIEW_DUAL_SLAVE       = 10,
    FRAME_TYPE_REPROCESSING_SLAVE       = 11,
    FRAME_TYPE_REPROCESSING_DUAL_MASTER = 12,
    FRAME_TYPE_REPROCESSING_DUAL_SLAVE  = 13,
    FRAME_TYPE_INTERNAL_SLAVE           = 14,
    /*
     * "Transition" means that
     * Master or Slave camera was ready to enter "INTERNAL" mode
     * (post processing standby).
     * This type is necessary to avoid frame drop
     * by changing from "INTERNAL" to "NORMAL" in opposite camera.
     */
    FRAME_TYPE_TRANSITION               = 15,
    FRAME_TYPE_TRANSITION_SLAVE         = 16,
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    FRAME_TYPE_INTERNAL_SENSOR_TRANSITION        = 17,
    FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION    = 18,
    FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION   = 19,
#endif
    FRAME_TYPE_MAX,
} frame_type_t;

#ifdef SUPPORT_SENSOR_MODE_CHANGE
typedef enum FRAME_TYPE_SENSOR_TRANSITION {
    FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_ING = FRAME_TYPE_INTERNAL_SENSOR_TRANSITION,
    FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_START = (FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_ING | (1 << 25)),
    FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_STOP = (FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_ING   | (1 << 26)),
}frame_type_sensor_transition_t;
#endif

typedef enum entity_type {
    ENTITY_TYPE_INPUT_ONLY              = 0, /* Need input buffer only */
    ENTITY_TYPE_OUTPUT_ONLY             = 1, /* Need output buffer only */
    ENTITY_TYPE_INPUT_OUTPUT            = 2, /* Need input/output both */
    ENTITY_TYPE_INVALID,
} entity_type_t;

/* Entity state define */
typedef enum entity_state {
    ENTITY_STATE_NOP                    = 0, /* Do not need operation */
    ENTITY_STATE_READY                  = 1, /* Ready to operation */
    ENTITY_STATE_PROCESSING             = 2, /* Processing stage */
    ENTITY_STATE_FRAME_DONE             = 3, /* Pipe has been done HW operation. */
    ENTITY_STATE_COMPLETE               = 4, /* Complete frame, done for all SW work. */
    ENTITY_STATE_REWORK                 = 5, /* After COMPLETE, but need re-work entity */
    ENTITY_STATE_FRAME_SKIP             = 6, /* Entity has been skipped */
} entity_state_t;

typedef enum entity_buffer_type {
    ENTITY_BUFFER_FIXED                 = 0, /* Buffer is never change */
    ENTITY_BUFFER_DELIVERY              = 1, /* Buffer change is possible */
    ENTITY_BUFFER_INVALID
} entity_buffer_type_t;

typedef enum entity_buffer_state {
    ENTITY_BUFFER_STATE_NOREQ           = 0, /* This buffer is not used */
    ENTITY_BUFFER_STATE_REQUESTED       = 1, /* This buffer is not used */
    ENTITY_BUFFER_STATE_READY           = 2, /* Buffer is ready */
    ENTITY_BUFFER_STATE_PROCESSING      = 3, /* Buffer is being prossed */
    ENTITY_BUFFER_STATE_COMPLETE        = 4, /* Buffer is complete */
    ENTITY_BUFFER_STATE_ERROR           = 5, /* Buffer is complete */
    ENTITY_BUFFER_STATE_INVALID,
} entity_buffer_state_t;

enum DST_BUFFER_COUNT
{
    DST_BUFFER_DEFAULT = 0,
    DST_BUFFER_COUNT_MAX = MAX_NODE, /* this must same with MAX_NODE */
};

enum SRC_BUFFER_COUNT
{
    SRC_BUFFER_DEFAULT = 0,
    SRC_BUFFER_COUNT_MAX = MAX_OUTPUT_NODE,
};

typedef enum STREAM_TYPE {
    STREAM_TYPE_CAPTURE				= 0,
    STREAM_TYPE_RAW					= 1,
    STREAM_TYPE_ZSL_INPUT			= 2,
    STREAM_TYPE_ZSL_OUTPUT			= 3,
    STREAM_TYPE_DEPTH				= 4,
    STREAM_TYPE_DEPTH_STALL			= 5,
    STREAM_TYPE_YUVCB_STALL			= 6,
    STREAM_TYPE_THUMBNAIL_CB		= 7,
    STREAM_TYPE_VISION				= 8,
    STREAM_TYPE_ZSL_YUV_INPUT       = 9,
    STREAM_TYPE_MAX,
} STREAM_TYPE_T;

typedef enum FRAME_STATE_IN_SELECTOR {
    FRAME_STATE_IN_SELECTOR_BASE,
    FRAME_STATE_IN_SELECTOR_PUSHED,
    FRAME_STATE_IN_SELECTOR_REMOVE,
    FRAME_STATE_IN_SELECTOR_MAX,
} FRAME_STATE_IN_SELECTOR_T;

typedef enum FRAME_MODE {
    FRAME_MODE_REORDER,
    FRAME_MODE_SWMCSC,
    FRAME_MODE_VENDOR_YUV_STALL,
    FRAME_MODE_MF_STILL,
    FRAME_MODE_DUAL_ZOOM_SOLUTION,
    FRAME_MODE_DUAL_BOKEH_ANCHOR,
    FRAME_MODE_MAX,
} FRAME_MODE_T;

typedef enum FRAME_MODE_VALUE {
    FRAME_MODE_VALUE_INVALID = -1,
    FRAME_MODE_VALUE_REORDER_CUR,
    FRAME_MODE_VALUE_REORDER_MAX,
    FRAME_MODE_VALUE_REORDER_LOCK,
    FRAME_MODE_VALUE_DSPORT,
    FRAME_MODE_VENDOR_YUV_STALL_PORT,
    FRAME_MODE_VALUE_MAX,
} FRAME_MODE_VALUE_T;

typedef enum REQUEST_BACKUP_MODE {
    REQUEST_BACKUP_MODE_INVALID = -1,
    REQUEST_BACKUP_MODE_DNG,
    REQUEST_BACKUP_MODE_REORDER,
    REQUEST_BACKUP_MODE_MAX,
} REQUEST_BACKUP_MODE_T;

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
struct camera2_virtual_node_group {
    uint32_t virtualVid[CAPTURE_NODE_MAX];
};
#endif

struct ExynosCameraFrameSelectorTag {
    /* Id for extendibility to support multiple selector */
    int   selectorId; // ExynosCameraFrameSelector::SELECTOR_ID_BASE

    /* information to get the specific buffer from frame */

    /* entity pipe Id */
    int   pipeId;

    /* it can be nodeIndex in ExynosCameraFrame */
    int   bufPos;

    /* -1 : invalid, 0 : dst, 1 : src */
    int   isSrc;

#ifdef __cplusplus
    ExynosCameraFrameSelectorTag() {
        selectorId = 0; // ExynosCameraFrameSelector::SELECTOR_ID_BASE
        pipeId = -1;
        bufPos = -1;
        isSrc = -1;
    }

    ExynosCameraFrameSelectorTag& operator =(const ExynosCameraFrameSelectorTag &other) {
        selectorId  = other.selectorId;
        pipeId      = other.pipeId;
        bufPos      = other.bufPos;
        isSrc       = other.isSrc;

        return *this;
    }

    bool operator ==(const ExynosCameraFrameSelectorTag &other) const {
        bool ret = true;

        if (selectorId != other.selectorId
                || pipeId      != other.pipeId
                || bufPos      != other.bufPos
                || isSrc       != other.isSrc) {
            ret = false;
        }

        return ret;
    }

    bool operator !=(const ExynosCameraFrameSelectorTag &other) const {
        return !(*this == other);
    }
#endif
};

typedef List<ExynosCameraFrameSelectorTag> selector_tag_queue_t;

class ExynosCameraFrameEntity {
public:
    ExynosCameraFrameEntity(
        uint32_t pipeId,
        entity_type_t type,
        entity_buffer_type_t bufType);
    uint32_t getPipeId(void);

    status_t setSrcBuf(ExynosCameraBuffer buf, uint32_t nodeIndex = 0);
    status_t setDstBuf(ExynosCameraBuffer buf, uint32_t nodeIndex = 0);

    status_t getSrcBuf(ExynosCameraBuffer *buf, uint32_t nodeIndex = 0);
    status_t getDstBuf(ExynosCameraBuffer *buf, uint32_t nodeIndex = 0);

    status_t setSrcRect(ExynosRect rect, uint32_t nodeIndex = 0);
    status_t setDstRect(ExynosRect rect, uint32_t nodeIndex = 0);

    status_t getSrcRect(ExynosRect *rect, uint32_t nodeIndex = 0);
    status_t getDstRect(ExynosRect *rect, uint32_t nodeIndex = 0);

    status_t setSrcBufState(entity_buffer_state_t state, uint32_t nodeIndex = 0);
    status_t setDstBufState(entity_buffer_state_t state, uint32_t nodeIndex = 0);

    entity_buffer_state_t getSrcBufState(uint32_t nodeIndex = 0);
    entity_buffer_state_t getDstBufState(uint32_t nodeIndex = 0);

    uint32_t getNumOfSrcBuf(void) { return m_srcBufCnt; }
    uint32_t getNumOfDstBuf(void) { return m_dstBufCnt; }

    entity_buffer_type_t getBufType(void);

    status_t        setEntityState(entity_state_t state);
    entity_state_t  getEntityState(void);

    ExynosCameraFrameEntity *getPrevEntity(void);
    ExynosCameraFrameEntity *getNextEntity(void);

    status_t setPrevEntity(ExynosCameraFrameEntity *entity);
    status_t setNextEntity(ExynosCameraFrameEntity *entity);

    bool     flagSpecficParent(void);
    status_t setParentPipeId(enum pipeline parentPipeId);
    int      getParentPipeId(void);

    status_t setRotation(int rotation);
    int      getRotation(void);
#ifdef USE_DEBUG_PROPERTY
    nsecs_t  getProcessTimestamp(void) const { return m_processTimestamp; }
    nsecs_t  getDoneTimestamp(void) const { return m_doneTimestamp; }
#endif
private:
    status_t m_setEntityType(entity_type_t type);

private:
    uint32_t                m_pipeId;
    ExynosCameraBuffer      m_srcBuf[SRC_BUFFER_COUNT_MAX];
    ExynosCameraBuffer      m_dstBuf[DST_BUFFER_COUNT_MAX];

    uint32_t                m_srcBufCnt;
    uint32_t                m_dstBufCnt;

    ExynosRect              m_srcRect[SRC_BUFFER_COUNT_MAX];
    ExynosRect              m_dstRect[DST_BUFFER_COUNT_MAX];

    entity_type_t           m_EntityType;
    entity_buffer_type_t    m_bufferType;

    entity_buffer_state_t   m_srcBufState[SRC_BUFFER_COUNT_MAX];
    entity_buffer_state_t   m_dstBufState[DST_BUFFER_COUNT_MAX];
    entity_state_t          m_entityState;

    ExynosCameraFrameEntity *m_prevEntity;
    ExynosCameraFrameEntity *m_nextEntity;

    bool                     m_flagSpecificParent;
    int                      m_parentPipeId;

    int                      m_rotation;
#ifdef USE_DEBUG_PROPERTY
    nsecs_t                  m_processTimestamp;
    nsecs_t                  m_doneTimestamp;
#endif
};

/* Frame state define */
typedef enum frame_status {
    FRAME_STATE_READY      = 0,    /* Ready to operation */
    FRAME_STATE_RUNNING    = 1,    /* Frame is running */
    FRAME_STATE_COMPLETE   = 2,    /* Complete frame. */
    FRAME_STATE_SKIPPED    = 3,    /* This Frame has been skipped. */
    FRAME_STATE_INVALID    = 4,    /* Invalid state */
} frame_status_t;

typedef struct ExynosCameraPerFrameInfo {
    bool perFrameControlNode;
    int perFrameNodeIndex;
    int perFrameNodeVideID;
} camera_per_fream_into_t;

typedef struct ExynosCameraDupBufferInfo {
    int streamID;
    int extScalerPipeID;
} dup_buffer_info_t;

class ExynosCameraFrame : public RefBase {

    friend class FrameWorker;
    friend class CreateWorker;
    friend class DeleteWorker;
    friend class ExynosCameraFrameManager;

public:
    typedef enum result_update_type {
        RESULT_UPDATE_TYPE_PARTIAL,
        RESULT_UPDATE_TYPE_BUFFER,
        RESULT_UPDATE_TYPE_ALL,
        RESULT_UPDATE_TYPE_MAX,
    } result_update_type_t;

    typedef enum result_update_status {
        RESULT_UPDATE_STATUS_NONE,
        RESULT_UPDATE_STATUS_REQUIRED,
        RESULT_UPDATE_STATUS_READY,
        RESULT_UPDATE_STATUS_DONE,
        RESULT_UPDATE_STATUS_MAX,
    } result_update_status_t;

    struct jpeg_pipe_info {
        ExynosRect  jpegRect;
        int         jpegQuality;
        ExynosRect  thumbnailSize;
        int         thumbnailQuality;
        int         rotation;
        int         jpegEncodeSize;

        jpeg_pipe_info& operator =(const jpeg_pipe_info &other) {
            jpegRect            = other.jpegRect;
            jpegQuality         = other.jpegQuality;
            thumbnailSize       = other.thumbnailSize;
            thumbnailQuality    = other.thumbnailQuality;
            rotation            = other.rotation;
            jpegEncodeSize      = other.jpegEncodeSize;
            return *this;
        }
    };

private:
    ExynosCameraFrame(
            int cameraId,
            ExynosCameraConfigurations *configurations,
            ExynosCameraParameters *params,
            uint32_t frameCount,
            uint32_t frameType = 0);
    ExynosCameraFrame(int cameraId);

    ~ExynosCameraFrame();

public:
    /* If curEntity is NULL, newEntity is added to m_linkageList */
    status_t        addSiblingEntity(
                        ExynosCameraFrameEntity *curEntity,
                        ExynosCameraFrameEntity *newEntity);
    status_t        addChildEntity(
                        ExynosCameraFrameEntity *parentEntity,
                        ExynosCameraFrameEntity *newEntity);
    status_t        addChildEntity(
                        ExynosCameraFrameEntity *parentEntity,
                        ExynosCameraFrameEntity *newEntity,
                        int parentPipeId);

    ExynosCameraFrameEntity *getFirstEntity(void);
    ExynosCameraFrameEntity *getNextEntity(void);
    /* Unused, but useful */
    /* ExynosCameraFrameEntity *getChildEntity(ExynosCameraFrameEntity *parentEntity); */

    ExynosCameraFrameEntity *searchEntityByPipeId(uint32_t pipeId);

    status_t        setSrcBuffer(
                        uint32_t pipeId,
                        ExynosCameraBuffer srcBuf,
                        uint32_t nodeIndex = 0);
    status_t        setDstBuffer(
                        uint32_t pipeId,
                        ExynosCameraBuffer dstBuf,
                        uint32_t nodeIndex = 0);
    status_t        setDstBuffer(
                        uint32_t pipeId,
                        ExynosCameraBuffer dstBuf,
                        uint32_t nodeIndex,
                        int      parentPipeId);

    status_t        getSrcBuffer(
                        uint32_t pipeId,
                        ExynosCameraBuffer *srcBuf,
                        uint32_t nodeIndex = 0);
    status_t        getDstBuffer(
                        uint32_t pipeId,
                        ExynosCameraBuffer *dstBuf,
                        uint32_t nodeIndex = 0);

    status_t        setSrcRect(
                        uint32_t pipeId,
                        ExynosRect srcRect,
                        uint32_t nodeIndex = 0);
    status_t        setDstRect(
                        uint32_t pipeId,
                        ExynosRect dstRect,
                        uint32_t nodeIndex = 0);

    status_t        getSrcRect(
                        uint32_t pipeId,
                        ExynosRect *srcRect,
                        uint32_t nodeIndex = 0);
    status_t        getDstRect(
                        uint32_t pipeId,
                        ExynosRect *dstRect,
                        uint32_t nodeIndex = 0);

    status_t        getSrcBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t *state,
                        uint32_t nodeIndex = 0);
    status_t        getDstBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t *state,
                        uint32_t nodeIndex = 0);

    status_t        setSrcBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t state,
                        uint32_t nodeIndex = 0);
    status_t        setDstBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t state,
                        uint32_t nodeIndex = 0);

    status_t        ensureSrcBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t state);

    status_t        ensureDstBufferState(
                        uint32_t pipeId,
                        entity_buffer_state_t state);

    int             getNumOfSrcBuffer(
                        uint32_t pipeId);

    int             getNumOfDstBuffer(
                        uint32_t pipeId);

    status_t        setEntityState(
                        uint32_t pipeId,
                        entity_state_t state);
    status_t        getEntityState(
                        uint32_t pipeId,
                        entity_state_t *state);

    status_t        getEntityBufferType(
                        uint32_t pipeId,
                        entity_buffer_type_t *type);

    void            setRequest(bool tap,
                               bool tac,
                               bool isp,
                               bool scc,
                               bool dis,
                               bool scp);

    void            setRequest(bool tap,
                               bool tac,
                               bool isp,
                               bool ispp,
                               bool ispc,
                               bool scc,
                               bool dis,
                               bool scp);

    status_t        setBackupRequest(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId, bool val);
    bool            getBackupRequest(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId);
    void            backupRequest(REQUEST_BACKUP_MODE_T mode);
    void            restoreRequest(REQUEST_BACKUP_MODE_T mode);
    bool            reverseExceptForSpecificReq(REQUEST_BACKUP_MODE_T mode, uint32_t pipeId, bool val);

    void            setRequest(uint32_t pipeId, bool val);
    bool            getRequest(uint32_t pipeId);

    void                    setParameters(ExynosCameraParameters *params);
    ExynosCameraParameters *getParameters();

    uint32_t        getFrameCount(void);
    int32_t         getMetaFrameCount(uint32_t srcNodeIndex = 0);
    status_t        setNumRequestPipe(uint32_t num);
    uint32_t        getNumRequestPipe(void);
    uint32_t        getNumRemainPipe(void);

    bool            isComplete(void);
    ExynosCameraFrameEntity *getFrameDoneEntity(void);
    ExynosCameraFrameEntity *getFrameDoneEntity(uint32_t pipeID);
    ExynosCameraFrameEntity *getFrameDoneFirstEntity(void);
    ExynosCameraFrameEntity *getFrameDoneFirstEntity(uint32_t pipeID);
    ExynosCameraFrameEntity *getFirstEntityNotComplete(void);

    status_t        skipFrame(void);
    bool            isSkipPreview(void);

    void            setFrameState(frame_status_t state);
    frame_status_t  getFrameState(void);
    bool            checkFrameState(frame_status_t state);

    void            printEntity(void);
    void            printNotDoneEntity(void);
    void            dump(void);

    void            frameLock(void);
    void            frameUnlock(void);
    bool            getFrameLockState(void);

    status_t        initMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        getMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        setMetaData(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);

    status_t        setNFDInfo(struct fd_info *fdInfo);
    status_t        getNFDInfo(struct fd_info *fdInfo);

    /* for const read operation without memcpy */
    const struct camera2_shot_ext *getConstMeta(uint32_t srcNodeIndex = 0);

    status_t        storeShotExtMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);

    status_t        storeDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        storeDynamicMeta(struct camera2_dm *dm, uint32_t srcNodeIndex = 0);
    status_t        storeUserDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        storeUserDynamicMeta(struct camera2_udm *udm, uint32_t srcNodeIndex = 0);

    status_t        getDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        setDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        getDynamicMeta(struct camera2_dm *dm, uint32_t srcNodeIndex = 0);
    status_t        getUserDynamicMeta(struct camera2_shot_ext *shot, uint32_t srcNodeIndex = 0);
    status_t        getUserDynamicMeta(struct camera2_udm *udm, uint32_t srcNodeIndex = 0);

    status_t        setMetaDataEnable(bool flag, uint32_t srcNodeIndex = 0);
    bool            getMetaDataEnable(uint32_t srcNodeIndex = 0);

    float           getZoomRatio(uint32_t srcNodeIndex = 0);
    status_t        setZoomRatio(float zoomRatio, uint32_t srcNodeIndex = 0);

    status_t        getZoomRect(ExynosRect *zoomRect, uint32_t srcNodeIndex = 0);
    status_t        setZoomRect(ExynosRect zoomRect, uint32_t srcNodeIndex = 0);

    status_t        getActiveZoomRect(ExynosRect *zoomRect, uint32_t srcNodeIndex = 0);
    status_t        setActiveZoomRect(ExynosRect zoomRect, uint32_t srcNodeIndex = 0);

    int             getActiveZoomMargin(uint32_t srcNodeIndex = 0);
    status_t        setActiveZoomMargin(int zoomMargin, uint32_t srcNodeIndex = 0);

    float           getAppliedZoomRatio(uint32_t srcNodeIndex = 0);
    status_t        setAppliedZoomRatio(float zoomRatio, uint32_t srcNodeIndex = 0);

    float           getActiveZoomRatio(uint32_t srcNodeIndex = 0);
    status_t        setActiveZoomRatio(float zoomRatio, uint32_t srcNodeIndex = 0);

    uint32_t        getSetfile(uint32_t srcNodeIndex = 0);
    status_t        setSetfile(uint32_t setfile, uint32_t srcNodeIndex = 0);

    status_t        getNodeGroupInfo(struct camera2_node_group *node_group, int index, uint32_t srcNodeIndex = 0);
    status_t        getNodeGroupInfo(struct camera2_node_group *node_group, int index, float *zoomRatio, uint32_t srcNodeIndex = 0);
    status_t        storeNodeGroupInfo(struct camera2_node_group *node_group, int index, uint32_t srcNodeIndex = 0);
    status_t        storeNodeGroupInfo(struct camera2_node_group *node_group, int index, float zoomRatio, uint32_t srcNodeIndex = 0);
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    status_t        getVirtualNodeInfo(struct camera2_virtual_node_group *node_group, int index, uint32_t srcNodeIndex = 0);
    status_t        storeVirtualNodeInfo(struct camera2_virtual_node_group *node_group, int index, uint32_t srcNodeIndex = 0);
#endif
    void            dumpNodeGroupInfo(const char *name, uint32_t srcNodeIndex = 0);

    void            setCameraId(int cameraId, int32_t srcNodeIndex = 0);
    int             getCameraId(int32_t srcNodeIndex = 0);

    void                    setFactoryType(enum FRAME_FACTORY_TYPE factoryType);
    enum FRAME_FACTORY_TYPE getFactoryType(void) const;

    void            setRequestKey(uint32_t requestKey);
    uint32_t        getRequestKey(void) const;

    /*
     * To use to idenfy continous frames.
     * You can classify the frame group F10~F12 and F13.
     * ex. frame(F10, I0), frame(F11, I1), frame(F12, I3), frame(F13, I0)
     */
    void            setFrameIndex(int index);
    int             getFrameIndex(void);
    void            setMaxFrameIndex(int index);
    int             getMaxFrameIndex(void);
#ifdef USE_DUAL_CAMERA
    status_t        setBokehAnchorFrameIndex(int index);
    int             getBokehAnchorFrameIndex(void);
#endif

    void            setJpegEncodeSize(int pipeId, int jpegEncodeSize);
    int             getJpegEncodeSize(int pipeId);

    void            setJpegPipeInfo(int pipeId, ExynosRect jpegRect, camera2_shot* shot);
    jpeg_pipe_info  getJpegPipeInfo(int pipeId);

    int64_t         getTimeStamp(uint32_t srcNodeIndex = 0);
    int64_t         getTimeStampBoot(uint32_t srcNodeIndex = 0);

    void            getFpsRange(uint32_t *min, uint32_t *max, uint32_t srcNodeIndex = 0);
    void            setFpsRange(uint32_t min, uint32_t max, uint32_t srcNodeIndex = 0);

    uint32_t        getUniqueKey(void);
    status_t        setUniqueKey(uint32_t uniqueKey);
    status_t        setFrameInfo(ExynosCameraConfigurations *configurations, ExynosCameraParameters *params,
                                 uint32_t frameCount, uint32_t frameType);
    ExynosCameraConfigurations *getConfigurations(void) const { return m_configurations; }
    void            setFrameType(frame_type_t frameType);
    uint32_t        getFrameType(void);

    void setFrameYuvStallPortUsage(int usage) {
        m_yuvStallPortEnable = usage;
    }

    int getFrameYuvStallPortUsage(void) {
        return m_yuvStallPortEnable;
    }

    dup_buffer_info_t       getDupBufferInfo() {
        return m_dupBufferInfo;
    }

    void setDupBufferInfo(dup_buffer_info_t dupBufferInfo) {
        m_dupBufferInfo = dupBufferInfo;
        return;
    }

#ifdef DEBUG_FRAME_MEMORY_LEAK
    long long int   getCheckLeakCount();
#endif

    status_t setRotation(uint32_t pipeId, int rotation);
    int      getRotation(uint32_t pipeId);
    status_t setFlipHorizontal(uint32_t pipeId, int flipHorizontal);
    int      getFlipHorizontal(uint32_t pipeId);
    status_t setFlipVertical(uint32_t pipeId, int flipVertical);
    int      getFlipVertical(uint32_t pipeId);

#ifdef CORRECT_TIMESTAMP_FOR_SENSORFUSION
    void setAdjustedTimestampFlag(bool flag) {m_adjustedTimestampFlag = flag;};
    bool getAdjustedTimestampFlag(void) {return m_adjustedTimestampFlag;};
#endif

    void setFrameSpecialCaptureStep(uint32_t state) {m_specialCaptureStep = state;};
    uint32_t getFrameSpecialCaptureStep(void) {return m_specialCaptureStep;};

    void    setPPScenario(int pipeId, int scenario);
    int32_t getPPScenario(int pipeId);
    int32_t getPPScenarioIndex(int scenario);
    bool    hasPPScenario(int scenario);
    bool    isLastPPScenarioPipe(int pipeId);
    int32_t getLastPPScenarioIndex();
    int32_t getNumberOfPPScenarios();

    void setStreamRequested(int stream, bool flag);
    bool getStreamRequested(int stream);

    void setHasRequest(bool hasReques) {m_hasRequest = hasReques;};
    bool getHasRequest(void) {return m_hasRequest;};

    void setPipeIdForResultUpdate(result_update_type_t resultType, enum pipeline pipeId);
    int  getPipeIdForResultUpdate(result_update_type_t resultType);
    void setStatusForResultUpdate(result_update_type_t resultType, result_update_status_t resultStatus);
    int  getStatusForResultUpdate(result_update_type_t resultType);
    bool isReadyForResultUpdate(result_update_type_t resultType);
    bool isCompleteForResultUpdate(void);
#ifdef USE_DUAL_CAMERA
    bool isSlaveFrame(void);
#endif

    status_t setVendorMeta(sp<ExynosCameraVendorMetaData> vendorMeta);
    sp<ExynosCameraVendorMetaData> getVendorMeta();

    static bool           isSlaveFrame(frame_type_t frameType);
    static enum DUAL_OPERATION_MODE convertDualModeByFrameType(frame_type_t frameType);

    void                  lockSelectorTagList(void);
    void                  unlockSelectorTagList(void);
    status_t              addSelectorTag(int selectorId, int pipeId, int bufPos, bool isSrc);
    bool                  findSelectorTag(ExynosCameraFrameSelectorTag *compareTag);
    bool                  removeSelectorTag(ExynosCameraFrameSelectorTag *removeTag);
    bool                  getFirstSelectorTag(ExynosCameraFrameSelectorTag *tag);
    void                  setStateInSelector(FRAME_STATE_IN_SELECTOR_T state);
    FRAME_STATE_IN_SELECTOR_T getStateInSelector(void);
    /* Below selectorTag functions needs lock */
    status_t              addRawSelectorTag(int selectorId, int pipeId, int bufPos, bool isSrc);
    bool                  findRawSelectorTag(ExynosCameraFrameSelectorTag *compareTag);
    bool                  removeRawSelectorTag(ExynosCameraFrameSelectorTag *removeTag);
    bool                  getFirstRawSelectorTag(ExynosCameraFrameSelectorTag *tag);
    void                  setRawStateInSelector(FRAME_STATE_IN_SELECTOR_T state);
    FRAME_STATE_IN_SELECTOR_T getRawStateInSelector(void);
    selector_tag_queue_t *getSelectorTagList(void);

    void setNeedDynamicBayer(bool set);
    bool getNeedDynamicBayer(void);

    void setReleaseDepthBuffer(bool releaseBuffer) {m_releaseDepthBufferFlag = releaseBuffer;};
    bool getReleaseDepthBuffer(void) {return m_releaseDepthBufferFlag;};

    void setStreamTimestamp(uint64_t streamTimeStamp) {m_streamTimeStamp = streamTimeStamp;};
    uint64_t getStreamTimestamp(void) {return m_streamTimeStamp;};

#ifdef USE_DEBUG_PROPERTY
    void    getEntityInfoStr(String8 &str);
    frame_status_t getPreviousFrameState(void) const { return m_previousFrameState; }
    nsecs_t getCreateTimestamp(void) const   { return m_createTimestamp; }
    nsecs_t getCompleteTimestamp(void) const { return m_completeTimestamp; }
#endif

    void setBufferDondeIndex(int bufferDondeIndex) {m_bufferDondeIndex = bufferDondeIndex;};
    int32_t getBufferDondeIndex(void) {return m_bufferDondeIndex;}

    void setUseOnePort(bool useOnePort) {m_useOnePortFlag = useOnePort;};
    bool getUseOnePort(void) {return m_useOnePortFlag;};

    void setServiceBufferPipeId(int32_t streamId, int32_t pipeId);
    int32_t getServiceBufferPipeId(int32_t streamId);
    bool hasServiceBuffer(int32_t streamId);
    void setServiceBufferPosition(int32_t streamId, int32_t pos);
    int32_t getServiceBufferPosition(int32_t streamId);

    void copySizeInfo(ExynosCameraFrameSP_sptr_t otherFrame);
    void setSizeInfo(frame_size_scenario_t scenario, const frame_size_info_t &info, int32_t cameraId = -1);
    bool getSizeInfo(frame_size_scenario_t scenario, frame_size_info_t &info, int32_t cameraId = -1);
    const FrameSizeInfoMap_t& getSizeInfoMap(int cameraId);

    void storeInternalBufTagPipeId(int32_t internalBufTagPipeId) {m_internalBufTagPipeId = internalBufTagPipeId;};
    int32_t getInternalBufTagPipeId(void) {return m_internalBufTagPipeId;};

    void setOnePortId(int portId) {m_onePortId = portId;};
    void setSecondPortId(int portId) {m_secondPortId = portId;};
    bool isOnePortId(int portId);
    bool isSecondPortId(int portId);
    int getOnePortId(void) {return m_onePortId;};
    int getSecondPortId(void) {return m_secondPortId;};
    void setPhysStreamOnlyStatus(bool state) {m_physStreamOnly = state;}
    bool getPhysStreamOnlyStatus(void) {return m_physStreamOnly;}

    void                setMode(FRAME_MODE_T mode, bool enable);
    void                setModeValue(FRAME_MODE_VALUE_T modeValue, int value);
    bool                getMode(FRAME_MODE_T mode);
    int                 getModeValue(FRAME_MODE_VALUE_T modeValue);

    void                        setPairFrame(ExynosCameraFrameSP_sptr_t newFrame);
    ExynosCameraFrameSP_sptr_t  getPairFrame();
    ExynosCameraFrameSP_sptr_t  popPairFrame();
    int                         getSizeOfPairFrameQ();
    void                        releasePairFrameQ();
    void                        setDualOperationMode(enum DUAL_OPERATION_MODE mode) { m_dualOperationMode = mode; }
    enum DUAL_OPERATION_MODE    getDualOperationMode(void) const { return m_dualOperationMode; }
    void                        setFallbackOn(bool isFallback) { m_isFallback = isFallback; }
    bool                        isFallback(void) const { return m_isFallback; }
    void                        setDisplayCameraId(int32_t cameraId) { m_displayCameraId = cameraId; }
    int32_t                     getDisplayCameraId(void) const { return m_displayCameraId; }

    bool usePostScalerZoom(void) { return m_postScalerZoomInfo.enable; };
    void enablePostScalerZoom(void) { m_postScalerZoomInfo.enable = true; };
    void disablePostScalerZoom(void) { m_postScalerZoomInfo.enable = false; };
    void setPostScalerZoomSrcRect(ExynosRect r) {
        m_postScalerZoomInfo.srcRect = r;
    };
    void setPostScalerZoomCropRect(int i, ExynosRect r) {
        m_postScalerZoomInfo.cropRect[i] = r;
    };
    ExynosRect getPostScalerZoomSrcRect(void) {return m_postScalerZoomInfo.srcRect;};
    ExynosRect getPostScalerZoomCropRect(int i) {return m_postScalerZoomInfo.cropRect[i];};
    void setPostScalerZoomPipeId(enum pipeline pipeId) {
        m_postScalerZoomInfo.targetPipeId = pipeId;
    }
    enum pipeline getPostScalerZoomPipeId(void) {
        return m_postScalerZoomInfo.targetPipeId;
    };

private:
    status_t        m_init();
    status_t        m_deinit();

    void            m_updateStatusForResultUpdate(enum pipeline pipeId);
    void            m_dumpStatusForResultUpdate(void);

    /* ACCESS allowed only frameManager */
    status_t        setFrameMgrInfo(frame_key_queue_t *queue);

private:
    int                         m_cameraId;
    int                         m_cameraIds[SRC_BUFFER_COUNT_MAX];
    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    mutable Mutex                        m_linkageLock;
    List<ExynosCameraFrameEntity *>      m_linkageList;
    List<ExynosCameraFrameEntity *>::iterator m_currentEntity;

    ExynosCameraConfigurations *m_configurations;
    ExynosCameraParameters     *m_parameters;
    uint32_t                    m_frameCount;
    uint32_t                    m_frameType;

    frame_status_t              m_frameState;
    mutable Mutex               m_frameStateLock;
    mutable Mutex               m_frameLock;

    uint32_t                    m_numRequestPipe;
    uint32_t                    m_numCompletePipe;

    bool                        m_frameLocked;

    bool                        m_metaDataEnable[SRC_BUFFER_COUNT_MAX];
    struct camera2_shot_ext     m_metaData[SRC_BUFFER_COUNT_MAX];
    struct camera2_node_group   m_node_gorup[SRC_BUFFER_COUNT_MAX][PERFRAME_NODE_GROUP_MAX];
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    struct camera2_virtual_node_group m_virtualnode_gorup[SRC_BUFFER_COUNT_MAX][PERFRAME_NODE_GROUP_MAX];
#endif
    float                       m_zoomRatio[SRC_BUFFER_COUNT_MAX];
    float                       m_activeZoomRatio[SRC_BUFFER_COUNT_MAX];
    float                       m_appliedZoomRatio[SRC_BUFFER_COUNT_MAX];
    int                         m_activeZoomMargin[SRC_BUFFER_COUNT_MAX];
    ExynosRect                  m_zoomRect[SRC_BUFFER_COUNT_MAX];
    ExynosRect                  m_activeZoomRect[SRC_BUFFER_COUNT_MAX];

    jpeg_pipe_info              m_jpegInfo[MAX_PIPE_NUM_JPEG_DST_REPROCESSING - PIPE_JPEG_REPROCESSING];

    bool                        m_request[MAX_NUM_PIPES];
    array<array<bool, MAX_NUM_PIPES>, REQUEST_BACKUP_MODE_MAX> m_backupRequest;

    uint32_t                    m_uniqueKey;
    bool                        m_yuvStallPortEnable;

    mutable Mutex               m_refCountLock;
    int32_t                     m_refCount;
    mutable Mutex               m_entityLock;

#ifdef DEBUG_FRAME_MEMORY_LEAK
    static Mutex                m_countLock;
    /* total remained frame which not deleted */
    static unsigned long long   m_checkLeakCount;
    /* unique frame count to trace */
    static unsigned long long   m_checkLeakFrameCount;
    /* m_checkLeakFrameCount */
    unsigned long long          m_privateCheckLeakCount;
#endif
    dup_buffer_info_t           m_dupBufferInfo;
#ifdef CORRECT_TIMESTAMP_FOR_SENSORFUSION
    bool m_adjustedTimestampFlag;
#endif
    uint32_t                    m_specialCaptureStep;

    frame_key_queue_t          *m_frameQueue;
    bool                        m_hasRequest;
    bool                        m_updateResult;

    map<int, int>               m_scenario;
    mutable Mutex               m_scenarioLock;

    map<int, int>               m_flipHorizontalMap;
    map<int, int>               m_flipVerticalMap;

    int                         m_frameIndex;
    int                         m_frameMaxIndex;
#ifdef USE_DUAL_CAMERA
    bool                        m_isPrepareBokehAnchorFrame;
    int                         m_bokehAnchorFrameIndex;
#endif
    bool                        m_stream[STREAM_TYPE_MAX];

    enum pipeline               m_resultUpdatePipeId[RESULT_UPDATE_TYPE_MAX];
    result_update_status_t      m_resultUpdateStatus[RESULT_UPDATE_TYPE_MAX];

    mutable Mutex               m_selectorTagQueueLock;
    selector_tag_queue_t        m_selectorTagQueue;
    FRAME_STATE_IN_SELECTOR_T   m_stateInSelector;

    bool                        m_needDynamicBayer;

    bool                        m_releaseDepthBufferFlag;
    uint64_t                    m_streamTimeStamp;

    enum FRAME_FACTORY_TYPE     m_factoryType;
    uint32_t                    m_requestKey;

    sp<ExynosCameraVendorMetaData>  m_vendorMeta;

#ifdef USE_DEBUG_PROPERTY
    frame_status_t              m_previousFrameState;
    nsecs_t                     m_createTimestamp;
    nsecs_t                     m_completeTimestamp;
#endif

    int32_t                     m_bufferDondeIndex;
    bool                        m_useOnePortFlag;
    int32_t                     m_internalBufTagPipeId;
    int                         m_onePortId;
    int                         m_secondPortId;
    bool                        m_physStreamOnly;

    bool                        m_mode[FRAME_MODE_MAX];
    int                         m_modeValue[FRAME_MODE_VALUE_MAX];
    mutable Mutex               m_modeLock;
    mutable Mutex               m_modeValueLock;

    struct fd_info              m_fdInfo;

    List<ExynosCameraFrameSP_sptr_t> m_pairFrameQ;
    mutable Mutex               m_pairFrameLock;

    enum DUAL_OPERATION_MODE    m_dualOperationMode;
    bool                        m_isFallback;
    int32_t                     m_displayCameraId;
    std::unordered_map<int32_t, int32_t> m_streamIdPosMap;
    std::unordered_map<int32_t, int32_t> m_streamIdPipeIdMap;
    std::map<int32_t /* cameraId  */, FrameSizeInfoMap_t> m_frameSizeInfoMap;

    struct {
        bool            enable = false;
        ExynosRect      srcRect;
        ExynosRect      cropRect[CAPTURE_NODE_MAX];
        enum pipeline   targetPipeId = PIPE_JPEG_REPROCESSING;
    } m_postScalerZoomInfo;

};

}; /* namespace android */

#endif
