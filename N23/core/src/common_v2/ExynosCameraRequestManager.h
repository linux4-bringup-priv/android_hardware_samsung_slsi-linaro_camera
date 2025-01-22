/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXYNOS_CAMERA_REQUEST_MANAGER_H__
#define EXYNOS_CAMERA_REQUEST_MANAGER_H__
#define CALLBACK_FPS_CHECK

#include <log/log.h>
#include <utils/RefBase.h>
#include <hardware/camera3.h>
#include <CameraMetadata.h>
#include <map>
#include <list>
#include <array>
#include <android/sync.h>

#include "ExynosCameraDefine.h"
#include "ExynosCameraObject.h"
#include "ExynosCameraStreamManager.h"
#include "ExynosCameraFrameFactory.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraTimeLogger.h"
#include "ExynosCameraVendorMetaData.h"

namespace android {

using namespace std;

namespace EXYNOS_REQUEST_RESULT {
    enum TYPE {
        CALLBACK_INVALID         = -1,
        CALLBACK_NOTIFY_ONLY     = 0x00,
        CALLBACK_BUFFER_ONLY     = 0x01,
        CALLBACK_PARTIAL_3AA     = 0x02,
        CALLBACK_ALL_RESULT      = 0x03,
        CALLBACK_NOTIFY_ERROR    = 0x04,
        CALLBACK_PARTIAL_SHUTTER = 0x05,
        CALLBACK_MAX             = 0x06,
    };

    enum ERROR {
        ERROR_UNKNOWN,

        /* An error has occurred in processing a request. No output (metadata or buffers) will be produced for this request. */
        ERROR_REQUEST,

        /* An error has occurred in producing an output result metadata buffer for a request, but output stream buffers for it will still be available. */
        ERROR_RESULT,

        /* An error has occurred in placing an output buffer into a stream for a request. The frame metadata and other buffers may still be available. */
        ERROR_BUFFER,
    };
};

namespace EXYNOS_LIST_OPER {
    enum MODE {
        SINGLE_BACK  = 0,
        SINGLE_FRONT = 1,
        SINGLE_ORDER = 2,
        MULTI_GET    = 3
    };
};

#define PHYSCAM_ID_SIZE     4

typedef list< camera3_stream_buffer_t* >                  StreamBufferList;

class ExynosCamera;
class ExynosCameraRequest;
class ExynosCameraFrameFactory;

typedef sp<ExynosCameraRequest> ExynosCameraRequestSP_sprt_t;
typedef sp<ExynosCameraRequest>& ExynosCameraRequestSP_dptr_t;

typedef ExynosCameraList<uint32_t>                        stream_callback_queue_t;
typedef ExynosCameraThread<ExynosCameraRequestManager>    callbackThread;

typedef list< int >                                       StreamIDList;
typedef list< EXYNOS_REQUEST_RESULT::TYPE >               ResultTypeList;
typedef list< EXYNOS_REQUEST_RESULT::TYPE >::iterator     ResultTypeListIter;


typedef bool (ExynosCamera::*factory_donehandler_t)();

class ExynosCameraRequestResult : public virtual RefBase {
public:
    ExynosCameraRequestResult(uint32_t key, uint32_t requestKey, uint32_t frameCount, EXYNOS_REQUEST_RESULT::TYPE type);
    ~ExynosCameraRequestResult();

    virtual uint32_t getFrameCount();
    virtual uint32_t getRequestKey();
    virtual uint32_t getKey();

    virtual EXYNOS_REQUEST_RESULT::TYPE getType();
    virtual int getStreamId();
    virtual camera3_capture_result_t* getCaptureResult();
     virtual camera3_notify_msg_t* getNotifyMsg();
    virtual camera3_stream_buffer_t* getStreamBuffer();
    virtual void     setStreamTimeStamp(uint64_t timestamp);
    virtual uint64_t getStreamTimeStamp(void);
    virtual void setupForPhysCamMeta(camera3_capture_result_t *captureResult);

private:
    status_t         m_init();
    status_t         m_deinit();

private:
    uint32_t                     m_key;
    uint32_t                     m_requestKey;
    EXYNOS_REQUEST_RESULT::TYPE  m_type;
    uint32_t                     m_frameCount;
    camera3_capture_result_t     m_captureResult;
    camera3_notify_msg_t         m_notityMsg;
    camera3_stream_buffer_t      m_streamBuffer;
    uint64_t                     m_streamTimeStamp;
    /* following memebers added to avoid dynamic allocations for small blocks */
    char                         m_physCamIds[MAX_NUM_SENSORS][PHYSCAM_ID_SIZE]; //it is used for only IDS
    char                        *m_physCamIdsPointer[MAX_NUM_SENSORS];
    camera_metadata_t           *m_physCamMetadata[MAX_NUM_SENSORS];
    int                          m_streamId;
};

namespace EXYNOS_REQUEST {
    enum STATE {
        STATE_INVALID = -1,            /* invalid state */
        STATE_ERROR,                   /* request error callbck state */
        STATE_SERVICE,                 /* request success callbck state */
        STATE_RUNNING,                 /* request running state */
    };
};

typedef sp<ExynosCameraRequestResult>                            ResultRequest;
typedef sp<ExynosCameraRequestResult>&                           ResultRequest_dptr_t;
typedef list< uint32_t >                                         ResultRequestkeys;
typedef list< uint32_t >::iterator                               ResultRequestkeysIterator;
typedef list<ResultRequest>                                      ResultList;
typedef list<ResultRequest>::iterator                            ResultListIter;
typedef map< int, ResultList* >                                  ResultMap;
typedef map< int, ResultList* >::iterator                        ResultMapIter;
typedef map< int, ResultRequest >                                ResultMapS;
typedef map< int, ResultRequest >::iterator                      ResultMapSIter;

typedef map< int32_t, ExynosCameraFrameFactory* >               FrameFactoryMap;
typedef map< int32_t, ExynosCameraFrameFactory* >::iterator     FrameFactoryMapIterator;
typedef list<ExynosCameraFrameFactory*>                         FrameFactoryList;
typedef list<ExynosCameraFrameFactory*>::iterator               FrameFactoryListIterator;

class ExynosCameraRequest : public virtual RefBase {
public:
    ExynosCameraRequest(camera3_capture_request_t* request,
        CameraMetadata &previousMeta, CameraMetadata *previousMetaPhysCam);
    ~ExynosCameraRequest();

    virtual uint32_t                       getKey();
    virtual void                           setFrameCount(uint32_t frameCount);
    virtual uint32_t                       getFrameCount();

    virtual camera3_capture_request_t*     getServiceRequest();

    virtual CameraMetadata*                getServiceMeta();
    virtual CameraMetadata*                getServiceMetaPhysCam(int32_t physCamInternalID);
    virtual struct camera2_shot_ext*       getServiceShot();
    virtual struct camera2_shot_ext*       getServiceShotPhysCam(int32_t physCamInternalID);
    virtual struct CameraMetaParameters*   getMetaParameters();
    virtual struct CameraMetaParameters*   getMetaParamPhysCam(int32_t physCamInternalID);

    virtual void                           setRequestLock();
    virtual void                           setRequestUnlock();

    virtual status_t                       setRequestState(EXYNOS_REQUEST::STATE state);
    virtual EXYNOS_REQUEST::STATE          getRequestState(void);

    virtual uint64_t                       getSensorTimestamp();
    virtual void                           setSensorTimestamp(uint64_t timeStamp);
    virtual uint32_t                       getNumOfInputBuffer();
    virtual camera3_stream_buffer_t*       getInputBuffer();

    virtual uint32_t                       getNumOfOutputBuffer();
    virtual uint32_t                       getNumOfPhysCamSettings();
    virtual int32_t                        getSizeOfFactory(enum FRAME_FACTORY_TYPE factoryType);
    virtual int32_t *                      getAllPhysCamInternalIDs();
    virtual const camera3_stream_buffer_t* getOutputBuffers();
    virtual const camera3_stream_buffer_t* getOutputBuffer(int streamId, bool exact);

    virtual status_t                       pushFrameFactory(int StreamID, ExynosCameraFrameFactory* factory);
    virtual ExynosCameraFrameFactory*      getFrameFactory(int streamID);
    virtual bool                           findFrameFactory(int streamID);

    virtual status_t                       getFrameFactoryList(FrameFactoryList *list);
    virtual status_t                       setFactoryAddrList(ExynosCameraFrameFactory *factoryAddr[FRAME_FACTORY_TYPE_MAX]);
    virtual status_t                       getFactoryAddrList(enum FRAME_FACTORY_TYPE factoryType, FrameFactoryList *list);

    virtual void                           increaseCompleteBufferCount(void);
    virtual void                           resetCompleteBufferCount(void);
    virtual int                            getCompleteBufferCount(void);
    virtual bool                           isAllBufferCallbackDone(void);

    virtual void                           setRequestId(int reqId);
    virtual int                            getRequestId();
    virtual status_t                       getAllRequestOutputStreams(List<int> **list);
    virtual status_t                       pushRequestOutputStreams(int requestStreamId);
    virtual status_t                       getAllRequestInputStreams(List <int> **list);
    virtual status_t                       pushRequestInputStreams(int requestStreamId);

    virtual status_t                       setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag);
    virtual bool                           getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType);
    virtual status_t                       setCallbackStreamDone(int streamId, bool flag);
    virtual bool                           getCallbackStreamDone(int streamId);
    virtual void                           printCallbackDoneState();
    virtual bool                           isComplete();

    virtual int                            getStreamIdwithBufferIdx(int bufferIndex);
    virtual bool                           hasStream(int streamIdx);
    virtual int                            getStreamIdwithStreamIdx(int index);
    virtual int                            getBufferIndex(int streamId);

    virtual void                           increasePipelineDepth(void);
    virtual void                           updatePipelineDepth(void);
    virtual CameraMetadata                 get3AAResultMeta(void);
    virtual CameraMetadata                 getShutterMeta();
    virtual void                           get3AAResultMetaVendor(CameraMetadata &minimal_resultMeta);

    virtual status_t                       setStreamBufferStatus(int streamId, camera3_buffer_status_t bufferStatus);
    virtual camera3_buffer_status_t        getStreamBufferStatus(int streamId);

    virtual void                           setSkipMetaResult(bool skip);
    virtual bool                           getSkipMetaResult(void);

    virtual void                           setSkipCaptureResult(bool skip);
    virtual bool                           getSkipCaptureResult(void);

    virtual void                           setDsInputPortId(int dsInputPortId);
    virtual int                            getDsInputPortId(void);

    virtual void                           setYsumValue(struct ysum_data *ysumdata);
    virtual void                           getYsumValue(struct ysum_data *ysumdata);

    virtual void                           setStreamPipeId(int streamId, int pipeId);
    virtual int                            getStreamPipeId(int streamId);

    virtual void                           setParentStreamPipeId(int streamId, int pipeId);
    virtual int                            getParentStreamPipeId(int streamId);

    virtual status_t                       setAcquireFenceDone(buffer_handle_t *handle, bool done);
    virtual bool                           getAcquireFenceDone(buffer_handle_t *handle);

    virtual void                           setFlipHorizontal(int streamId, bool flip);
    virtual bool                           getFlipHorizontal(int streamId);

    virtual void                           setFlipVertical(int streamId, bool flip);
    virtual bool                           getFlipVertical(int streamId);

    virtual void                           setRotation(int streamId, bool rotation);
    virtual bool                           getRotation(int streamId);

    virtual void                           setDisplayCameraId(int32_t cameraId) { m_displayCameraId = cameraId; }
    virtual int32_t                        getDisplayCameraId(void) const { return m_displayCameraId; }
    virtual void                           setSizeInfo(frame_size_scenario_t scenario, const frame_size_info_t &info, int32_t cameraId = -1);
    virtual bool                           getSizeInfo(frame_size_scenario_t scenario, frame_size_info_t &info, int32_t cameraId = -1);

    virtual status_t                       setSlaveCamId(int32_t slaveCamId, int32_t slaveCamIdx, bool isReprocessing = false);
    virtual int32_t                        getSlaveCamId(bool isReprocessing = false);
    virtual void                           getPhysCamIDString(char **physCamIDString);

    virtual sp<ExynosCameraVendorMetaData>          getVendorMeta();


private:
    virtual status_t                       m_init();
    virtual status_t                       m_deinit();

    virtual status_t                       m_push(int key, ExynosCameraFrameFactory* item, FrameFactoryMap *list, Mutex *lock);
    virtual status_t                       m_pop(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock);
    virtual status_t                       m_get(int streamID, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock);
    virtual bool                           m_find(int streamID, FrameFactoryMap *list, Mutex *lock);
    virtual status_t                       m_getList(FrameFactoryList *factorylist, FrameFactoryMap *list, Mutex *lock);
    virtual bool                           m_isInputStreamId(int streamId);
    virtual int                            m_getOutputBufferIndex(int streamId);
    virtual status_t                       m_setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag, Mutex *lock);
    virtual bool                           m_getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, Mutex *lock);
    virtual status_t                       m_setCallbackStreamDone(int streamId, bool flag, Mutex *lock);
    virtual bool                           m_getCallbackStreamDone(int streamId, Mutex *lock);
    virtual status_t                       m_setStreamBufferStatus(int streamId, camera3_buffer_status_t status, Mutex *lock);
    virtual camera3_buffer_status_t        m_getStreamBufferStatus(int streamId, Mutex *lock);
    virtual void                           m_copyAndEraseTagFromeServiceMetaDataU8(uint32_t tag, CameraMetadata &resultMeta);
    virtual void                           m_copyAndEraseTagFromeServiceMetaDataI32(uint32_t tag, CameraMetadata &resultMeta);

private:
    uint32_t                      m_key;
    uint32_t                      m_frameCount;
    camera3_capture_request_t     *m_request;
    CameraMetadata                m_serviceMeta;
    CameraMetadata                m_serviceMetaPhysCam[MAX_NUM_SENSORS];
    int32_t                       m_physCamInternalID[MAX_NUM_SENSORS];
    map<int32_t, int32_t>         m_physCamInternalIDMap;
    char                          m_physCamIDString[MAX_NUM_SENSORS][PHYSCAM_ID_SIZE];

    struct camera2_shot_ext       m_serviceShot;
    struct camera2_shot_ext       m_serviceShotPhysCam[MAX_NUM_SENSORS];
    int                           m_streamIdList[HAL_STREAM_ID_MAX];
    struct CameraMetaParameters   m_metaParameters;
    struct CameraMetaParameters   m_metaParamPhysCam[MAX_NUM_SENSORS];

    EXYNOS_REQUEST::STATE         m_requestState;
    int                           m_requestId;

    map<buffer_handle_t *, bool>  m_acquireFenceDoneMap;
    mutable Mutex                 m_acquireFenceDoneLock;

    bool                          m_resultStatus[EXYNOS_REQUEST_RESULT::CALLBACK_MAX];
    mutable Mutex                 m_resultStatusLock;

    bool                          m_inputStreamDone;
    bool                          m_outputStreamDone[HAL_STREAM_ID_MAX];

    camera3_buffer_status_t       m_inputStreamStatus;
    camera3_buffer_status_t       m_outputStreamStatus[HAL_STREAM_ID_MAX];
    int                           m_streamPipeId[HAL_STREAM_ID_MAX];
    int                           m_streamParentPipeId[HAL_STREAM_ID_MAX];

    mutable Mutex                 m_streamStatusLock;

    int                           m_numOfOutputBuffers;
    int                           m_numPhysCamSettings;
    int                           m_numOfCompleteBuffers;
    List<int>                     m_requestOutputStreamList;
    List<int>                     m_requestInputStreamList;

    FrameFactoryMap               m_factoryMap;
    mutable Mutex                 m_factoryMapLock;

    unsigned int                  m_pipelineDepth;

    bool                          m_isSkipMetaResult;
    mutable Mutex                 m_SkipMetaResultLock;

    bool                          m_isSkipCaptureResult;
    mutable Mutex                 m_SkipCaptureResultLock;

    uint64_t                      m_sensorTimeStampBoot;

    mutable Mutex                 m_requestLock;
    mutable Mutex                 m_sizeInfoLock;

    FrameFactoryList              m_previewFactoryAddrList;
    FrameFactoryList              m_captureFactoryAddrList;

    int                           m_dsInputPortId;
    struct ysum_data              m_ysumValue;

    bool                          m_horizontalFlip[HAL_STREAM_ID_MAX];
    bool                          m_verticalFlip[HAL_STREAM_ID_MAX];

    bool                          m_rotation[HAL_STREAM_ID_MAX];

    int32_t                       m_displayCameraId;
    int32_t                       m_slaveCamId;
    int32_t                       m_slaveCamIdx;
    int32_t                       m_reprocessSlaveCamId;
    int32_t                       m_reprocessSlaveCamIdx;

    sp<ExynosCameraVendorMetaData>  m_vendorMeta;

    typedef std::map<frame_size_scenario_t, frame_size_info_t> FrameSizeInfoMap_t;
    std::map<int32_t /* cameraId  */, FrameSizeInfoMap_t> m_frameSizeInfoMap;
};

typedef list< uint32_t >           CallbackListkeys;
typedef list< uint32_t >::iterator CallbackListkeysIter;

class ResultCallback : public virtual RefBase {
public:
    ResultCallback(ExynosCameraRequestSP_sprt_t request, ResultRequest result);
    ~ResultCallback();

    ExynosCameraRequestSP_sprt_t getRequest();
    ResultRequest getResult();

private:
    ExynosCameraRequestSP_sprt_t    m_request;
    ResultRequest                   m_result;
};

typedef sp<ResultCallback>          ResultItem;

class ExynosCameraCallbackSequencer : public virtual RefBase {
public:
    ExynosCameraCallbackSequencer(const char *name, int cameraId);
    ~ExynosCameraCallbackSequencer();

    status_t        pushKey(uint32_t key);
    status_t        getFrontKey(uint32_t &key);
    uint32_t        getKeySize();

    status_t        pushResult(uint32_t key, ResultRequest result);
    status_t        popResult(uint32_t key, ResultRequest_dptr_t result);
    status_t        getResult(uint32_t key, ResultRequest_dptr_t result);
    uint32_t        getResultSize();
    bool            isExistResult(uint32_t key);

    status_t        getKeyList(CallbackListkeys *list);
    status_t        deleteKey(uint32_t key);
    void            dumpList();
    status_t        flush();

private:
    status_t        m_init();
    status_t        m_deinit();
    status_t        m_pushKey(EXYNOS_LIST_OPER::MODE operMode, uint32_t key, CallbackListkeys &keyList, Mutex *lock);
    status_t        m_popKey(EXYNOS_LIST_OPER::MODE operMode, CallbackListkeys &keyList, uint32_t &key, Mutex *lock);
    status_t        m_getKey(EXYNOS_LIST_OPER::MODE operMode, CallbackListkeys &keyList, uint32_t &key, Mutex *lock);
    status_t        m_pushResult(uint32_t key, ResultRequest item, ResultMapS &resultMap, Mutex *lock);
    status_t        m_popResult(uint32_t key, ResultRequest_dptr_t item, ResultMapS &resultMap, Mutex *lock);
    status_t        m_getResult(uint32_t key, ResultRequest_dptr_t item, ResultMapS &resultMap, Mutex *lock);
    bool            m_findResult(uint32_t key, ResultMapS &resultMap, Mutex *lock);
    status_t        m_delete(uint32_t key, CallbackListkeys &keyList, ResultMapS &resultMap, Mutex *lock);

private:
    CallbackListkeys m_runningRequestKeys;
    ResultMapS       m_resultMap;
    mutable Mutex    m_requestCbListLock;
    char             m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    int              m_cameraId;
};

typedef ExynosCameraList<uint32_t> result_queue_t;
typedef ExynosCameraList<ResultItem> result_callback_queue_t;

typedef sp<ExynosCameraCallbackSequencer>   callback_sequence_t;
typedef map<int, callback_sequence_t>       stream_callback_sequence_t;
typedef map<int, callback_sequence_t>::iterator stream_callback_sequence_iter_t;


class ExynosCameraRequestManager : public ExynosCameraObject, public virtual RefBase {
public:
    /* Constructor */
    ExynosCameraRequestManager(cameraId_Info *camIdInfo, ExynosCameraConfigurations *configurations);

    /* Destructor */
    virtual ~ExynosCameraRequestManager();

public:
    /* related to camera3 device operations */
    status_t             constructDefaultRequestSettings(int type, camera_metadata_t **request);

    /* Android meta data translation functions */
    ExynosCameraRequestSP_sprt_t           registerToServiceList(camera3_capture_request *srcRequest, bool skipRequest = false);
    ExynosCameraRequestSP_sprt_t           registerToServiceList(ExynosCameraRequestSP_sprt_t request);
    ExynosCameraRequestSP_sprt_t           eraseFromServiceList(void);
    status_t                               registerToRunningList(ExynosCameraRequestSP_sprt_t request_in);
    ExynosCameraRequestSP_sprt_t           getRunningRequest(uint32_t frameCount);
    status_t                               getRunningRequest(uint32_t requestCount, ExynosCameraRequestSP_dptr_t request);


    status_t                       setMetaDataConverter(ExynosCameraMetadataConverter *converter);
    ExynosCameraMetadataConverter* getMetaDataConverter();


    status_t                       setRequestsInfo(int key, ExynosCameraFrameFactory *factory);
    void                           clearFrameFactory(void);

    status_t                       setStreamInfos(list<int> &streamIdList /* WRN: only support output stream */);
    void                           flushStreamSequencer(void);
    void                           clearStreamSequencer(void);

    void                           notifyDeviceError(void);

    status_t                       pushResultRequest(ResultRequest result);

    status_t                       flush(bool reconfigure = false);

    /* other helper functions */
    status_t                       isPrevRequest(void);
    status_t                       clearPrevRequest(void);

    status_t                       setCallbackOps(const camera3_callback_ops *callbackOps);
    ResultRequest                  createResultRequest(uint32_t requestKey, uint32_t frameCount,
                                                       EXYNOS_REQUEST_RESULT::TYPE type);
    uint32_t                       getAllRequestCount(void);
    uint32_t                       getServiceRequestCount(void);
    uint32_t                       getRunningRequestCount(void);
    void                           getRunningRequestKeyList(CallbackListkeys &list);


    status_t                       setFrameCount(uint32_t frameCount, uint32_t requestKey);
    status_t                       waitforRequestflush();

    int32_t                        getResultRenew(void);
    void                           incResultRenew(void);
    void                           resetResultRenew(void);
    void                           dump(void);

private:
    typedef map<uint32_t, ExynosCameraRequestSP_sprt_t>           RequestInfoMap;
    typedef map<uint32_t, ExynosCameraRequestSP_sprt_t>::iterator RequestInfoMapIterator;
    typedef list<ExynosCameraRequestSP_sprt_t>                    RequestInfoList;
    typedef list<ExynosCameraRequestSP_sprt_t>::iterator          RequestInfoListIterator;
    typedef map<uint32_t, uint32_t>                       RequestFrameCountMap;
    typedef map<uint32_t, uint32_t>::iterator             RequestFrameCountMapIterator;

    status_t                       m_pushBack(ExynosCameraRequestSP_sprt_t item, RequestInfoList *list, Mutex *lock);
    status_t                       m_popBack(ExynosCameraRequestSP_dptr_t item, RequestInfoList *list, Mutex *lock);
    status_t                       m_pushFront(ExynosCameraRequestSP_sprt_t item, RequestInfoList *list, Mutex *lock);
    status_t                       m_popFront(ExynosCameraRequestSP_dptr_t item, RequestInfoList *list, Mutex *lock);
    status_t                       m_get(uint32_t frameCount, ExynosCameraRequestSP_dptr_t item, RequestInfoList *list, Mutex *lock);


    status_t                       m_push(ExynosCameraRequestSP_sprt_t item, RequestInfoMap *list, Mutex *lock);
    status_t                       m_pop(uint32_t frameCount, ExynosCameraRequestSP_dptr_t item, RequestInfoMap *list, Mutex *lock);
    status_t                       m_get(uint32_t frameCount, ExynosCameraRequestSP_dptr_t item, RequestInfoMap *list, Mutex *lock);
    bool                           m_exist(uint32_t key, RequestInfoMap *list, Mutex *lock);

    void                           m_printAllServiceRequestInfo(void);
    void                           m_printAllRequestInfo(RequestInfoMap *map, Mutex *lock);
    void                           m_printAllResultInfo(camera3_capture_result_t *result, EXYNOS_REQUEST_RESULT::TYPE type);

    status_t                       m_removeFromRunningList(uint32_t requestKey);

    status_t                       m_pushFactory(int key, ExynosCameraFrameFactory* item, FrameFactoryMap *list, Mutex *lock);
    status_t                       m_popFactory(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock);
    status_t                       m_getFactory(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock);

    status_t                       m_callbackOpsCaptureResult(camera3_capture_result_t *result, EXYNOS_REQUEST_RESULT::TYPE type);
    status_t                       m_callbackOpsNotify(camera3_notify_msg_t *msg);

    status_t                       m_getKey(uint32_t *key, uint32_t frameCount);
    status_t                       m_popKey(uint32_t *key, uint32_t frameCount);
    uint32_t                       m_generateResultKey();

    status_t                       m_releaseCameraMetadata(ExynosCameraRequestSP_sprt_t request, ResultRequest result);
    status_t                       m_sendCallbackResult(ExynosCameraRequestSP_sprt_t request, ResultRequest result);

    status_t                       m_increasePipelineDepth(RequestInfoMap *map, Mutex *lock);

    void                           m_debugCallbackFPS();

    status_t                       m_resultCallback(ResultTypeList typeList, ResultRequest result);
    bool                           m_resultNotifyCallbackThreadFunc(void);
    bool                           m_resultStreamCallbackThreadFunc(void);
    bool                           m_resultMetaCallbackThreadFunc(void);
    bool                           m_resultCallbackThreadFunc(void);

    bool                           m_notifyCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result = NULL);
    bool                           m_partialMetaCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result = NULL, bool flagFlush = false);
    bool                           m_allMetaCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result = NULL, bool flagFlush = false);
    bool                           m_bufferOnlyCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result = NULL, bool flagFlush = false);
    bool                           m_notifyErrorCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result = NULL, bool flagFlush = false);

    bool                           m_requestDeleteFunc(ExynosCameraRequestSP_sprt_t curRequest);

    void                           m_setFlushFlag(bool falg);
    bool                           m_getFlushFlag(void);
    void                           m_waitFlushDone(void);

    void                           m_adjustFaceDetectMetadata(ExynosCameraRequestSP_sprt_t request);

#if 0
    /* Other helper functions */
    status_t        initShotData(void);
    status_t        checkAvailableStreamFormat(int format);
    uint32_t        getFrameNumber(void);
#endif
private:
    bool                          m_flushFlag;
    mutable Mutex                 m_flushLock;
    cameraId_Info                 *m_camIdInfo;

    RequestInfoList               m_serviceRequests;
    RequestInfoMap                m_runningRequests;
    mutable Mutex                 m_requestLock;

    camera_metadata_t             *m_defaultRequestTemplate[CAMERA3_TEMPLATE_COUNT];
    CameraMetadata                m_previousMeta;
    CameraMetadata                m_previousMetaPhysCam[CAMERA_ID_MAX];

    ExynosCameraConfigurations    *m_configurations;
    ExynosCameraMetadataConverter *m_converter;

    const camera3_callback_ops_t  *m_callbackOps;

    int32_t                       m_requestResultKey;
    mutable Mutex                 m_requestResultKeyLock;

    FrameFactoryMap               m_factoryMap;
    mutable Mutex                 m_factoryMapLock;

    RequestFrameCountMap          m_requestFrameCountMap;
    mutable Mutex                 m_requestFrameCountMapLock;

    ExynosCameraDurationTimer     m_callbackFlushTimer;

#ifdef CALLBACK_FPS_CHECK
    int32_t                       m_callbackFrameCnt = 0;
    ExynosCameraDurationTimer     m_callbackDurationTimer;
#endif

    result_queue_t                m_resultNotifyCallbackQ;
    sp<callbackThread>            m_resultNotifyCallbackThread;

    result_queue_t                m_resultStreamCallbackQ;
    sp<callbackThread>            m_resultStreamCallbackThread;

    result_queue_t                m_resultMetaCallbackQ;
    sp<callbackThread>            m_resultMetaCallbackThread;

    result_callback_queue_t       m_callbackQ;
    sp<callbackThread>            m_callbackThread;

    callback_sequence_t           m_notifySequencer;
    callback_sequence_t           m_partialSequencer;
    callback_sequence_t           m_allMetaSequencer;
    stream_callback_sequence_t    m_streamsSequencerList;
    mutable Mutex                 m_streamsSequencerListLock;
    mutable Mutex                 m_callbackQLock;

    array<uint32_t, EXYNOS_REQUEST_RESULT::CALLBACK_MAX>  m_callbackCounts;

    int32_t                       m_resultRenew;
    uint32_t                      m_printInterval;
    int32_t                       m_lastResultKey[EXYNOS_REQUEST_RESULT::CALLBACK_MAX];

    struct FaceDetectMeta {
#ifdef USE_DUAL_CAMERA
        enum aa_cameraMode      cameraMode;
        enum aa_sensorPlace     masterCamera;
#endif
        enum facedetect_mode    faceDetectMode;
        uint32_t    frameCount;
        uint32_t    faceIds[CAMERA2_MAX_FACES];
        uint32_t    faceLandmarks[CAMERA2_MAX_FACES][6];
        uint32_t    faceRectangles[CAMERA2_MAX_FACES][4];
        uint8_t     faceScores[CAMERA2_MAX_FACES];
        struct vra_ext_meta vraExt;
    };
    struct FaceDetectMeta         m_faceDetectMeta;

    const camera_metadata_t       *m_physicalIdsMetaData[MAX_NUM_SENSORS];
};

}; /* namespace android */
#endif
