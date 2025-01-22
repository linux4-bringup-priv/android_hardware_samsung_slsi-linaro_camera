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

#ifndef EXYNOS_CAMERA_HW_IMPLEMENTATION_H
#define EXYNOS_CAMERA_HW_IMPLEMENTATION_H

#include "ExynosCameraDefine.h"

#include "ExynosCameraRequestManager.h"
#include "ExynosCameraStreamManager.h"
#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraFrameManager.h"
#include "ExynosCameraFrameFactory.h"
#include "ExynosCameraFrameFactoryPreview.h"
#include "ExynosCameraFrameFactoryVision.h"
#include "ExynosCameraFrameReprocessingFactory.h"
#include "ExynosCameraTimeLogger.h"
#include "ExynosCameraFrameSelector.h"

#ifdef USE_DUAL_CAMERA
#include "ExynosCameraFrameFactoryPreviewDual.h"
#include "ExynosCameraFrameReprocessingFactoryDual.h"
#endif

#ifdef USES_SW_VDIS
//class ExynosCameraSolutionSWVdis;
#include "ExynosCameraSolutionSWVdis.h"
#endif

#ifdef USES_SENSOR_LISTENER
#include "ExynosCameraSensorListenerWrapper.h"
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
#include "awb_cal.h"
#include "ExynosCameraLEDCalibrationMap.h"
#endif

#ifdef USES_SENSOR_GYRO_FACTORY_MODE
#include "ExynosCameraFactoryTestFactory.h"
#include "ExynosCameraFactoryTestSensorGyro.h"
#endif

namespace android {

#define SET_STREAM_CONFIG_BIT(_BIT,_STREAM_ID) \
    ((_BIT) |= (1 << ((_STREAM_ID) % HAL_STREAM_ID_MAX)))

#define COMPARE_STREAM_CONFIG_BIT(_BIT1,_BIT2) \
    ((_BIT1) & (_BIT2))

#define IS_OUTPUT_NODE(_FACTORY,_PIPE) \
    (((_FACTORY->getNodeType(_PIPE) >= OUTPUT_NODE) \
        && (_FACTORY->getNodeType(_PIPE) < MAX_OUTPUT_NODE)) ? true : false)

typedef ExynosCameraThread<ExynosCamera> mainCameraThread;
typedef ExynosCameraList<ExynosCameraFrameFactory *> framefactory3_queue_t;

#ifdef USES_VPL_PRELOAD
#define VPL_LIBRARY_PATH "libvpl.so"
#endif

class ExynosCameraResourceManager;
class ExynosCameraOfflineCapture;

class ExynosCameraStreamThread : public ExynosCameraThread <ExynosCameraStreamThread>
{
public:
    ExynosCameraStreamThread(ExynosCamera *cameraObj, const char *name, int pipeId);
    ~ExynosCameraStreamThread();

    frame_queue_t *getFrameDoneQ();
    void setFrameDoneQ(frame_queue_t *frameDoneQ);

private:
    ExynosCamera *m_mainCamera;
    frame_queue_t *m_frameDoneQ;
    int m_pipeId;

private:
    bool m_streamThreadFunc(void);
};

#ifdef ADAPTIVE_RESERVED_MEMORY
enum BUF_TYPE {
    BUF_TYPE_UNDEFINED = 0,
    BUF_TYPE_NORMAL,
    BUF_TYPE_VENDOR,
    BUF_TYPE_REPROCESSING,
    BUF_TYPE_MAX
};

enum BUF_PRIORITY {
    BUF_PRIORITY_FLITE = 0,
    BUF_PRIORITY_ISP,
    BUF_PRIORITY_CAPTURE_CB,
    BUF_PRIORITY_YUV_CAP,
    BUF_PRIORITY_THUMB,
    BUF_PRIORITY_DSCALEYUVSTALL,
    BUF_PRIORITY_FUSION,
    BUF_PRIORITY_VRA,
    BUF_PRIORITY_MCSC_OUT,
    BUF_PRIORITY_SSM,
    BUF_PRIORITY_ISP_RE,
    BUF_PRIORITY_JPEG_INTERNAL,
    BUF_PRIORITY_MAX
};

typedef struct {
    buffer_manager_tag_t bufTag;
    buffer_manager_configuration_t bufConfig;
    int minReservedNum;
    int totalPlaneSize;
    enum BUF_TYPE bufType;
} adaptive_priority_buffer_t;
#endif

#ifdef USE_DUAL_CAMERA
typedef enum DUAL_STANDBY_TRIGGER {
    DUAL_STANDBY_TRIGGER_NONE,
    DUAL_STANDBY_TRIGGER_MASTER_ON,
    DUAL_STANDBY_TRIGGER_MASTER_OFF,
    DUAL_STANDBY_TRIGGER_SLAVE_ON,
    DUAL_STANDBY_TRIGGER_SLAVE_OFF,
    DUAL_STANDBY_TRIGGER_MAX,
} dual_standby_trigger_t;

typedef struct {
    dual_standby_trigger_t mode;
    int32_t camIdIndex;
} dual_standby_trigger_info_t;
#endif

struct create_frame_info {
    ExynosCameraRequestSP_sprt_t    request;
    ExynosCameraFrameFactory        *factory;
    frame_type_t                    frameType;

    create_frame_info(ExynosCameraRequestSP_sprt_t _request,
                        ExynosCameraFrameFactory *_factory,
                        frame_type_t _frameType) : request(_request), factory(_factory), frameType(_frameType) {}
};
typedef ExynosCameraList<create_frame_info *> create_frame_info_queue_t;

typedef struct frame_handle_components{
    ExynosCameraParameters *parameters;
    ExynosCameraFrameFactory *previewFactory;
    ExynosCameraFrameFactory *reprocessingFactory;
    ExynosCameraActivityControl *activityControl;
    ExynosCameraFrameSelector *captureSelector;
    ExynosCameraConfigurations *configuration;
    ExynosCameraStreamManager *streamMgr;
    int currentCameraId;
} frame_handle_components_t;

#ifdef DEBUG_FUSION_CAPTURE_DUMP
static uint32_t captureNum = 0;
#endif

class ExynosCamera {
public:
    explicit ExynosCamera(cameraId_Info *camIdInfo, camera_metadata_t **info);
    virtual             ~ExynosCamera();

    /** Startup */
    status_t    initializeDevice(const camera3_callback_ops *callbackOps);

    /** Stream configuration and buffer registration */
    status_t    configureStreams(camera3_stream_configuration_t *stream_list);

    /** Template request settings provision */
    status_t    construct_default_request_settings(camera_metadata_t **request, int type);

    /** Submission of capture requests to HAL */
    status_t    processCaptureRequest(camera3_capture_request *request);

    /** Vendor metadata registration */
    void        get_metadata_vendor_tag_ops(const camera3_device_t *, vendor_tag_query_ops_t *ops);

    /** Flush all currently in-process captures and all buffers */
    status_t    flush(void);

    status_t    createDevice(cameraId_Info *camIdInfo, camera_metadata_t **info);
    status_t    destroyDevice(void);
    bool        isOfflineCaptureRunning(int cameraSessionId = 0);
    int         getCameraSessionId(void) { return m_cameraSessionId; }

    status_t    setPIPMode(bool enabled, bool isSubCam = false);
    status_t    setDualMode(bool enabled);
    bool        getDualMode(void);
    bool        isCameraRunning(void);

    /** Print out debugging state for the camera device */
    void        dump(int fd = -1);

    /** Stop */
    status_t    releaseDevice(void);

    void        release();

    /* Common functions */
    int         getCameraId() const;
    int         getServiceCameraId() const;
    int         getCameraInfoIdx() const;
    int         getCameraIdx(int32_t camId);
#ifdef USE_DUAL_CAMERA
    int         getSubCameraId() const;
#endif
    /* For Vendor */
    status_t    setParameters(const CameraParameters& params);

    bool        previewStreamFunc(ExynosCameraFrameSP_sptr_t newFrame, int pipeId);

private:
    typedef enum exynos_camera_state {
        EXYNOS_CAMERA_STATE_OPEN        = 0,
        EXYNOS_CAMERA_STATE_INITIALIZE  = 1,
        EXYNOS_CAMERA_STATE_CONFIGURED  = 2,
        EXYNOS_CAMERA_STATE_START       = 3,
        EXYNOS_CAMERA_STATE_RUN         = 4,
        EXYNOS_CAMERA_STATE_FLUSH       = 5,
        EXYNOS_CAMERA_STATE_ERROR       = 6,
        EXYNOS_CAMERA_STATE_SWITCHING_SENSOR = 7,
        EXYNOS_CAMERA_STATE_MAX         = 8,
    } exynos_camera_state_t;

private:
    /* Helper functions for initialization */
    void        m_createThreads(void);
    void        m_createManagers(void);
    void        m_vendorSpecificPreConstructor(int cameraId, int scenario);/* Vendor */
    void        m_vendorSpecificConstructor(void);    /* Vendor */
    void        m_vendorSpecificPreDestructor(void);  /* Vendor */
    void        m_vendorSpecificDestructor(void);     /* Vendor */
    void        m_vendorCreateThreads(void);         /* Vendor */
    status_t    m_reInit(void);
    status_t    m_vendorReInit(void);
    status_t    m_initFrameHoldCount(void);

    /* Helper functions for notification */
    status_t    m_sendJpegStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer, int size);
    status_t    m_sendRawStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer);
    status_t    m_sendVisionStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer);
    status_t    m_sendZslStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer);

    status_t    m_sendDepthStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer, int streamId);

    status_t    m_sendYuvStreamResult(ExynosCameraFrameSP_sptr_t frame,
                                      ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer,
                                      int streamId, bool skipBuffer, uint64_t streamTimeStamp,
                                      ExynosCameraParameters *params);
    status_t    m_sendYuvStreamStallResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer, int streamId);
    status_t    m_sendThumbnailStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer, int streamId);
    status_t    m_sendForceYuvStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraFrameSP_sptr_t frame,
                                                ExynosCameraFrameFactory *factory);
    status_t    m_sendForceStallStreamResult(ExynosCameraRequestSP_sprt_t request);
    status_t    m_sendMeta(ExynosCameraRequestSP_sprt_t request, EXYNOS_REQUEST_RESULT::TYPE type);
    status_t    m_sendPartialMeta(ExynosCameraRequestSP_sprt_t request, EXYNOS_REQUEST_RESULT::TYPE type);
    status_t    m_sendNotifyShutter(ExynosCameraRequestSP_sprt_t request, uint64_t timeStamp = 0);
    status_t    m_sendNotifyError(ExynosCameraRequestSP_sprt_t request, EXYNOS_REQUEST_RESULT::ERROR error,
                                            camera3_stream_t *stream = NULL);

    /* Helper functions of Buffer operation */
    status_t    m_createIonAllocator(ExynosCameraIonAllocator **allocator);

    void        m_getMcscOutputBufferSize(int *width, int *height);

    status_t    m_setBuffers(void);
    status_t    m_setFliteBuffers(bool isRemosaic, int maxSensorW, int maxSensorH);
    status_t    m_setVisionBuffers(void);
    status_t    m_setVendorBuffers();
    status_t    m_setSensorGyroBuffers(int numOfBuf);
    bool        m_flagSensorGyroBuffersAlloc(void);

    status_t    m_setupPreviewFactoryBuffers(const ExynosCameraRequestSP_sprt_t request,
                                                ExynosCameraFrameSP_sptr_t frame,
                                                ExynosCameraFrameFactory *factory);
    status_t    m_setupBatchFactoryBuffers(ExynosCameraRequestSP_sprt_t request,
                                                ExynosCameraFrameSP_sptr_t frame,
                                                ExynosCameraFrameFactory *factory);
    status_t    m_setupVisionFactoryBuffers(const ExynosCameraRequestSP_sprt_t request, ExynosCameraFrameSP_sptr_t frame);
    status_t    m_setupCaptureFactoryBuffers(const ExynosCameraRequestSP_sprt_t request, ExynosCameraFrameSP_sptr_t frame);
    //seperate from m_setupCaptureFactoryBuffers for internal buffer
    status_t    m_setupCaptureFactoryInternalBuffers(ExynosCameraFrameSP_sptr_t frame);

    status_t    m_allocBuffers(
                    const buffer_manager_tag_t tag,
                    const buffer_manager_configuration_t info);

    status_t    m_getBuffer(ExynosCameraFrameSP_dptr_t frame,
                            int pipeId,
                            int dstPipeId,
                            int dstPos,
                            buffer_manager_type_t bufferManagerType);

    status_t    m_putSrcBuffer(ExynosCameraFrameSP_sptr_t frame,
                               int pipeId,
                               int dstPos = 0);

    status_t    m_putDstBuffer(ExynosCameraFrameSP_sptr_t frame,
                               int pipeId,
                               int dstPos = 0);

    status_t    m_checkBuffer(ExynosCameraFrameSP_dptr_t frame,
                            int pipeId,
                            int dstPos);

#ifdef ADAPTIVE_RESERVED_MEMORY
    status_t    m_setupAdaptiveBuffer();
    status_t    m_addAdaptiveBufferInfos(const buffer_manager_tag_t tag,
                                         const buffer_manager_configuration_t info,
                                         enum BUF_PRIORITY buf_prior,
                                         enum BUF_TYPE buf_type);
    status_t    m_allocAdaptiveNormalBuffers();
    status_t    m_allocAdaptiveReprocessingBuffers();
    status_t    m_setAdaptiveReservedBuffers();
#endif

    /* helper functions for set buffer to frame */
    status_t    m_setupEntity(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame,
                                ExynosCameraBuffer *srcBuf = NULL,
                                ExynosCameraBuffer *dstBuf = NULL,
                                int dstNodeIndex = 0,
                                int dstPipeId = -1);
    status_t    m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame, ExynosCameraBuffer *buffer);
    status_t    m_setDstBuffer(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame, ExynosCameraBuffer *buffer,
                                        int nodeIndex = 0, int dstPipeId = -1);

    status_t    m_setVOTFBuffer(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame, uint32_t srcPipeId, uint32_t dstPipeId);
    status_t    m_setHWFCBuffer(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame, uint32_t srcPipeId, uint32_t dstPipeId);
    /* helper functions for frame factory */
    status_t    m_constructFrameFactory(void);
    status_t    m_destroyPreviewFrameFactory(void);
    status_t    m_startFrameFactory(ExynosCameraFrameFactory *factory);
    status_t    m_startReprocessingFrameFactory(ExynosCameraFrameFactory *factory);
    status_t    m_stopFrameFactory(ExynosCameraFrameFactory *factory);
    status_t    m_stopReprocessingFrameFactory(ExynosCameraFrameFactory *factory);
    status_t    m_deinitFrameFactory();
    status_t    m_deinitReprocessingFrameFactory(int cameraSessionId);


    /* frame Generation / Done handler */
    status_t    m_checkMultiCaptureMode(ExynosCameraRequestSP_sprt_t request);
    status_t    m_createInternalFrameFunc(ExynosCameraRequestSP_sprt_t request, bool flagFinishFactoryStart,
                                                        enum Request_Sync_Type syncType,
                                                        frame_type_t frameType = FRAME_TYPE_INTERNAL,
                                                        int cameraId = -1);
    status_t    m_createPreviewFrameFunc(enum Request_Sync_Type syncType, bool flagFinishFactoryStart);
    status_t    m_createVisionFrameFunc(enum Request_Sync_Type syncType, bool flagFinishFactoryStart);
    status_t    m_createCaptureFrameFunc(void);

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    status_t    m_createSensorTransitionFrameFunc(bool toggle);
#endif

    status_t    m_previewFrameHandler(ExynosCameraRequestSP_sprt_t request,
                                      ExynosCameraFrameFactory *targetfactory,
                                      frame_type_t frameType);
    status_t    m_visionFrameHandler(ExynosCameraRequestSP_sprt_t request,
                                      ExynosCameraFrameFactory *targetfactory,
                                      frame_type_t frameType = FRAME_TYPE_VISION);
    status_t    m_captureFrameHandler(ExynosCameraRequestSP_sprt_t request,
                                      ExynosCameraFrameFactory *targetfactory,
                                      frame_type_t frameType);
    bool        m_previewStreamFunc(ExynosCameraFrameSP_sptr_t newFrame, int pipeId);

    status_t    m_updateLatestInfoToShot(struct camera2_shot_ext *shot_ext, frame_type_t frameType,
                                        frame_handle_components_t *components = NULL);
    void        m_updateCropRegion(struct camera2_shot_ext *shot_ext,
                                        frame_handle_components_t *components,
                                        frame_type_t frameType,
                                        bool isReprocessing);
    status_t    m_updateJpegControlInfo(const struct camera2_shot_ext *shot_ext);
    void        m_updateFD(struct camera2_shot_ext *shot_ext, enum facedetect_mode fdMode,
                                int dsInputPortId, bool isReprocessing, bool isEarlyFd = 0);
    void        m_updateVendorInfo(ExynosCameraRequestSP_sprt_t request, struct camera2_shot_ext *shot_ext, bool isReprocessing);
    void        m_updateEdgeNoiseMode(struct camera2_shot_ext *shot_ext, bool isCaptureFrame);
    void        m_updateExposureTime(struct camera2_shot_ext *shot_ext);
    void        m_updateSetfile(struct camera2_shot_ext *shot_ext,
                                        bool isCaptureFrame,
                                        ExynosCameraParameters *parameters);
    void        m_updateMasterCam(struct camera2_shot_ext *shot_ext, bool isReprocessing);
    void        m_setTransientActionInfo(frame_handle_components_t *components);
    void        m_setApertureControl(frame_handle_components_t *components, struct camera2_shot_ext *shot_ext);
    status_t    m_updateYsumValue(ExynosCameraFrameSP_sptr_t frame, ExynosCameraRequestSP_sprt_t request);
    void        m_adjustSlave3AARegion(frame_handle_components_t *components, ExynosCameraFrameSP_sptr_t frame);
    status_t    m_updateDisplayRegion(ExynosCameraFrameSP_sptr_t frame,
                                      ExynosCameraRequestSP_sprt_t request, int streamId);

    /* helper functions for frame */
    status_t    m_generateFrame(ExynosCameraFrameFactory *factory,
                                List<ExynosCameraFrameSP_sptr_t> *list,
                                Mutex *listLock,
                                ExynosCameraFrameSP_dptr_t newFrame,
                                ExynosCameraRequestSP_sprt_t request);
    status_t    m_generateFrame(ExynosCameraFrameFactory *factory,
                                List<ExynosCameraFrameSP_sptr_t> *list,
                                Mutex *listLock,
                                ExynosCameraFrameSP_dptr_t newFrame,
                                ExynosCameraRequestSP_sprt_t request,
                                bool useJpegFlag);
    status_t    m_generateInternalFrame(ExynosCameraFrameFactory *factory,
                                        List<ExynosCameraFrameSP_sptr_t> *list,
                                        Mutex *listLock,
                                        ExynosCameraFrameSP_dptr_t newFrame,
                                        frame_type_t frameType,
                                        ExynosCameraRequestSP_sprt_t request = NULL);
#ifdef USE_DUAL_CAMERA
    status_t    m_generateTransitionFrame(ExynosCameraFrameFactory *factory,
                                          List<ExynosCameraFrameSP_sptr_t> *list,
                                          Mutex *listLock,
                                          ExynosCameraFrameSP_dptr_t newFrame,
                                          frame_type_t frameType,
                                          ExynosCameraRequestSP_sprt_t request);
#endif
    status_t    m_searchFrameFromList(List<ExynosCameraFrameSP_sptr_t> *list,
                                      Mutex *listLock,
                                      uint32_t frameCount,
                                      ExynosCameraFrameSP_dptr_t frame);
    status_t    m_removeFrameFromList(List<ExynosCameraFrameSP_sptr_t> *list,
                                      Mutex *listLock,
                                      ExynosCameraFrameSP_sptr_t frame);

    uint32_t    m_getSizeFromFrameList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock);
    status_t    m_clearList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock);

    uint32_t    m_getSizeFromRequestList(List<ExynosCameraRequestSP_sprt_t> *list, Mutex *listLock);
    status_t    m_clearRequestList(List<ExynosCameraRequestSP_sprt_t> *list, Mutex *listLock);

    status_t    m_removeInternalFrames(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock);
    status_t    m_releaseInternalFrame(ExynosCameraFrameSP_sptr_t frame);
    status_t    m_checkStreamBuffer(ExynosCameraFrameSP_sptr_t frame,
                                    ExynosCameraStream *stream,
                                    ExynosCameraBuffer *buffer,
                                    ExynosCameraRequestSP_sprt_t request,
                                    ExynosCameraFrameFactory *factory);
    status_t    m_checkStreamBufferStatus(ExynosCameraRequestSP_sprt_t request,
                                          ExynosCameraStream *stream, int *status,
                                          bool bufferSkip = false);
    void        m_checkUpdateResult(ExynosCameraFrameSP_sptr_t frame, uint32_t streamConfigBit);
    status_t    m_updateTimestamp(ExynosCameraRequestSP_sprt_t request, ExynosCameraFrameSP_sptr_t frame,
                                ExynosCameraBuffer *timestampBuffer);
    status_t    m_handleYUVPhysStreamResult(ExynosCameraFrameSP_sptr_t frame,
                                        ExynosCameraBuffer *buffer,
                                        ExynosCameraRequestSP_sprt_t request,
                                        int32_t camID,
                                        camera3_buffer_status_t streamBufferState);
    status_t    m_handlePreviewFrame(ExynosCameraFrameSP_sptr_t frame, int pipeId, ExynosCameraFrameFactory *factory);
    status_t    m_handleVisionFrame(ExynosCameraFrameSP_sptr_t frame, int pipeId, ExynosCameraFrameFactory *factory);
    status_t    m_handleInternalFrame(ExynosCameraFrameSP_sptr_t frame, int pipeId, ExynosCameraFrameFactory *factory);
    status_t    m_handleYuvCaptureFrame(ExynosCameraFrameSP_sptr_t frame);
    status_t    m_handleNV21CaptureFrame(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId);
    status_t    m_resizeToDScaledYuvStall(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId, int nodePipeId);
    status_t    m_resizeToScaledYuv(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId);
    status_t    m_handleJpegFrame(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId);
    status_t    m_handleBayerBuffer(ExynosCameraFrameSP_sptr_t frame, ExynosCameraRequestSP_sprt_t request, int pipeId, bool needSync = false);
#ifdef DEBUG_RAWDUMP
    status_t    m_dumpPreFlashBayerBuffer(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer buffer, ExynosCameraActivityFlash *flashMgr);
    status_t    m_dumpMainFlashBayerBuffer(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer buffer, ExynosCameraActivityFlash *flashMgr);
#endif
#ifdef DEBUG_RAWDUMP
    void        m_dumpRawBuffer(ExynosCameraBuffer* bayerBuffer, ExynosCameraFrameSP_sptr_t bayerFrame, ExynosCameraFrameSP_sptr_t frame);
#endif
#ifdef SUPPORT_DEPTH_MAP || defined (SUPPORT_PD_IMAGE)
    status_t    m_handleDepthBuffer(ExynosCameraFrameSP_sptr_t frame, ExynosCameraRequestSP_sprt_t request);
#endif
    status_t    m_handleSensorGyroBuffer(ExynosCameraFrameSP_sptr_t frame);
#ifdef USES_SW_VDIS
    status_t    m_handleVdisFrame(ExynosCameraFrameSP_sptr_t frame, ExynosCameraRequestSP_sprt_t request, int pipeId, ExynosCameraFrameFactory * factory);
#endif
#ifdef USE_DUAL_CAMERA
    status_t    m_checkDualOperationMode(ExynosCameraRequestSP_sprt_t request,
                                         bool isNeedModeChange, bool isReprocessing, bool flagFinishFactoryStart, bool skipCaptureLock = false);
    status_t    m_checkDualOperationMode_vendor(ExynosCameraRequestSP_sprt_t request,
                                                bool isNeedModeChange, bool isReprocessing, bool flagFinishFactoryStart,
                                                const DualTransitionInfo &transInfo,
                                                enum DUAL_OPERATION_MODE &newOperationMode,
                                                enum DUAL_OPERATION_SENSORS &newOperationSensor);
    status_t    m_setStandbyModeTrigger(bool flagFinishFactoryStart,
                                        enum DUAL_OPERATION_MODE newDualOperationMode,
                                        enum DUAL_OPERATION_MODE oldDualOperationMode,
                                        bool isNeedModeChange,
                                        bool *isTriggered,
                                        enum DUAL_OPERATION_SENSORS newOperationSensor,
                                        enum DUAL_OPERATION_SENSORS oldOperationSensor,
                                        bool &isTriggerSkipped);
    status_t    m_setBokehRefocusFusionYuvBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        int fusionPipeId, int srcNodeId, ExynosCameraBuffer &buffer);
    status_t    m_setBokehRefocusFusionDepthBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        int fusionPipeId, int depthNodeIndex, ExynosCameraBuffer *srcBuffer);
    status_t    m_setBokehRefocusJpegSrcBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        int srcPipeId, frame_handle_components_t components);
    status_t    m_returnBokehRefocusFusionDstBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        int fusionPipeId);
    status_t    m_composeBokehRefocusJpegBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        int bokehJpegPipeId,
                                        int piepIdDepth,
                                        ExynosCameraBuffer *jpegServiceBuffer);
    status_t    m_setBokehRefocusBuffer(bool isRemosaic = false);
#endif
#ifdef USES_COMBINE_PLUGIN
    status_t    m_handleCombinePreviewFrame(ExynosCameraFrameSP_sptr_t frame,
                                            ExynosCameraFrameFactory *factory,
                                            int srcPipeId, NODE_TYPE srcNodeType,
                                            int dstPipeId);
#endif
    status_t    m_handleSwmcscPreviewFrame(ExynosCameraFrameSP_sptr_t frame,
                                            ExynosCameraFrameFactory *factory,
                                            int srcPipeId, NODE_TYPE srcNodeType,
                                            int dstPipeId);
    /* helper functions for request */
    status_t    m_pushServiceRequest(camera3_capture_request *request, ExynosCameraRequestSP_dptr_t req,
                                                bool skipRequest = false);
    status_t    m_popServiceRequest(ExynosCameraRequestSP_dptr_t request);
    status_t    m_pushRunningRequest(ExynosCameraRequestSP_dptr_t request_in);
    status_t    m_updateResultShot(ExynosCameraFrameSP_sptr_t frame,
                                   ExynosCameraRequestSP_sprt_t request, struct camera2_shot_ext *src_ext,
                                   enum metadata_type metaType = PARTIAL_NONE, frame_type_t frameType = FRAME_TYPE_BASE,
                                   int32_t physCamID = -1);
    status_t    m_updateJpegPartialResultShot(ExynosCameraRequestSP_sprt_t request, struct camera2_shot_ext *dst_ext, ExynosCameraParameters *parameters);
    status_t    m_setFactoryAddr(ExynosCameraRequestSP_dptr_t request);
    status_t    m_updateFDAEMetadata(ExynosCameraFrameSP_sptr_t frame);
#ifdef USES_CAMERA_EXYNOS_VPL
    status_t    m_updateNFDInfo(ExynosCameraFrameSP_sptr_t frame);
    status_t    m_updateFaceDetectMeta(ExynosCameraFrameSP_sptr_t frame);
#endif

    /* helper functions for configuration options */
    uint32_t    m_getBayerPipeId(void);
    uint32_t    m_getSensorGyroPipeId(void);
    status_t    m_setFrameManager();
    status_t    m_setupFrameFactoryToRequest();
    status_t    m_setupStreamInfoToRequestManager(void);
    status_t    m_setConfigInform();
    status_t    m_setStreamInfo(camera3_stream_configuration *streamList);
    status_t    m_setExtraStreamInfo(camera3_stream_configuration *streamList);
    status_t    m_enumStreamInfo(camera3_stream_t *stream);
    status_t    m_enumExtraStreamInfo(camera3_stream_t *stream);
    status_t    m_checkStreamInfo(void);
    void        m_checkRequestStreamChanged(const char *handlerName, uint32_t currentStreamBit);
    int         m_getCurrentCamIdx(bool isReprocessing, int camType);
    int         m_getCurrentCamIdx(bool isReprocessing, int camType, frame_handle_components_t *components);


    status_t    m_getBayerServiceBuffer(ExynosCameraFrameSP_sptr_t frame,
                                        ExynosCameraBuffer *buffer,
                                        ExynosCameraRequestSP_sprt_t request);
    status_t    m_releaseSelectorTagBuffers(ExynosCameraFrameSP_sptr_t bayerFrame);
    status_t    m_getBayerBuffer(uint32_t pipeId,
                                 uint32_t frameCount,
                                 ExynosCameraBuffer *buffer,
                                 ExynosCameraFrameSelector *selector,
                                 frame_type_t frameType,
                                 ExynosCameraFrameSP_sptr_t captureFrame,
                                 ExynosCameraFrameSP_dptr_t frame
#ifdef SUPPORT_DEPTH_MAP
                                 , ExynosCameraBuffer *depthMapBuffer = NULL
#endif
                                            );
    status_t    m_checkBufferAvailable(uint32_t pipeId, int managerType = BUFFER_MANAGER_ION_TYPE);
    bool        m_checkValidBayerBufferSize(struct camera2_stream *stream, ExynosCameraFrameSP_sptr_t frame, bool flagForceRecovery);
    bool        m_isRequestEssential(ExynosCameraRequestSP_dptr_t request);

    status_t                m_transitState(exynos_camera_state_t state);
    exynos_camera_state_t   m_getState(void);
    bool                    m_isSkipBurstCaptureBuffer(frame_type_t frameType);
#ifdef USE_DUAL_CAMERA
    int32_t     m_getCameraIdFromOperationSensor(enum DUAL_OPERATION_SENSORS id,
                                                 int32_t *camId0, int32_t *camId1);
#endif
    /* s/w processing functions */
#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    void        m_reverseProcessingApplyCalcRGBgain(int32_t *inRGBgain, int32_t *invRGBgain, int32_t *pedestal);
    int         m_reverseProcessingApplyGammaProc(int pixelIn, int32_t *lutX, int32_t *lutY);
    status_t    m_reverseProcessingBayer(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer *inBuf, ExynosCameraBuffer *outBuf);
#endif

    /* debuger functions */
#ifdef FPS_CHECK
    void        m_debugFpsCheck(enum pipeline pipeId);
#endif
    // by Frame
    void        m_getFrameHandleComponentsWrapper(ExynosCameraFrameSP_sptr_t frame, frame_handle_components_t *components, int cameraSessionId = 0);
    // by FrameType and CameraId
    void        m_getFrameHandleComponentsWrapper(frame_type_t frameType, frame_handle_components_t *components, int cameraId = -1, int cameraSessionId = 0);
    void        m_setupSlaveCamIdForRequest(ExynosCameraRequestSP_sprt_t request);

    void        m_getOnePortId(ExynosCameraRequestSP_sprt_t request, bool *isOnlyPhysStreams, int32_t *onePortId = NULL, int32_t *secondPortId = NULL);

#ifdef SUPPORT_ME
    status_t    m_handleMeBuffer(ExynosCameraFrameSP_sptr_t frame, int mePos, const int meLeaderPipeId = -1);
#endif

#ifdef WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA
    status_t    m_waitDualStandbyOnForRemosaicCapture(int currentCameraId);
#endif

#ifdef SUPPORT_SENSOR_MODE_CHANGE
private:
    void        m_startSensorModeTransition(void);
    void        m_stopSensorModeTransition(void);

    status_t    m_switchSensorMode(ExynosCameraRequestSP_sprt_t request, bool toggle, bool streamOff = true);
#endif

    void        m_getFrameHandleComponents(frame_type_t frameType, frame_handle_components_t *components, int cameraId, int cameraSessionId);

    bool        m_checkPureBayerReprocessingFrom(ExynosCameraFrameSP_sptr_t frame);

    void        m_createCaptureStreamQ(void);
    void        m_releaseCaptureStreamQ(void);
    void        m_destroyCaptureStreamQ(void);
    void        m_captureThreadStopAndInputQ(void);
    status_t    m_stopPipeline(void);

    int         m_getCameraSessionId(ExynosCameraFrameSP_sptr_t frame);
    int         m_getCameraSessionId(ExynosCameraRequestSP_sprt_t request);

    ExynosCameraRequestManager* m_getRequestManager(ExynosCameraFrameSP_sptr_t frame);
    ExynosCameraRequestManager* m_getRequestManager(ExynosCameraRequestSP_sprt_t request);

    void        m_setFrameDoneQtoThread(void);

#ifdef USES_OFFLINE_CAPTURE
    status_t    m_rearrangeResources(int cameraSessionId);
#endif

private:
    ExynosCameraRequestManager      *m_requestMgr;
    ExynosCameraMetadataConverter   *m_metadataConverter;
    ExynosCameraConfigurations      *m_configurations;
    ExynosCameraParameters          *m_parameters[CAMERA_ID_MAX];
    ExynosCameraStreamManager       *m_streamManager;
    ExynosCameraFrameManager        *m_frameMgr;
    ExynosCameraIonAllocator        *m_ionAllocator;
    ExynosCameraBufferSupplier      *m_bufferSupplier;
    ExynosCameraActivityControl     *m_activityControl[CAMERA_ID_MAX];

    ExynosCameraFrameFactory        *m_frameFactory[FRAME_FACTORY_TYPE_MAX];
    framefactory3_queue_t           *m_frameFactoryQ;
    framefactory3_queue_t           *m_dualFrameFactoryQ;

    ExynosCameraFrameSelector       *m_captureSelector[CAMERA_ID_MAX];
    ExynosCameraFrameSelector       *m_captureZslSelector;

    sp<ExynosCameraResourceManager> m_resourceManager;
#ifdef USES_OFFLINE_CAPTURE
    sp<ExynosCameraOfflineCapture>  m_offlineCapture;
#endif

protected:
    cameraId_Info                   m_camIdInfo;

private:
    int                             m_cameraIds[MAX_NUM_SENSORS];
    int                             m_cameraId;
    bool                            m_isLogicalCam;
    map<int32_t, int32_t>           m_camIdMap;

#ifdef ADAPTIVE_RESERVED_MEMORY
    adaptive_priority_buffer_t      m_adaptiveBufferInfos[BUF_PRIORITY_MAX];
#endif

    uint32_t                        m_scenario;
    char                            m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    mutable Condition               m_captureResultDoneCondition;
    mutable Mutex                   m_captureResultDoneLock;
#ifdef USE_DUAL_CAMERA
    mutable Mutex                   m_createReprocessingFrameLock;
#endif
    uint64_t                        m_lastFrametime;
    int                             m_prepareFrameCount;

    mutable Mutex                   m_frameCompleteLock;
    mutable Mutex                   m_updateShotInfoLock;

    int                             m_shutterSpeed;
    int                             m_gain;
    int                             m_irLedWidth;
    int                             m_irLedDelay;
    int                             m_irLedCurrent;
    int                             m_irLedOnTime;

    int                             m_visionFps;

    exynos_camera_state_t           m_state;
    Mutex                           m_stateLock;

    bool                            m_captureStreamExist;
    bool                            m_rawStreamExist;
    bool                            m_videoStreamExist;

    bool                            m_recordingEnabled;
    uint32_t                        m_prevStreamBit;

    struct ExynosConfigInfo         *m_exynosconfig;

    struct camera2_shot_ext         *m_currentPreviewShot[CAMERA_ID_MAX];
    struct camera2_shot_ext         *m_currentInternalShot[CAMERA_ID_MAX];
    struct camera2_shot_ext         *m_currentCaptureShot[CAMERA_ID_MAX];
    struct camera2_shot_ext         *m_currentVisionShot[CAMERA_ID_MAX];

#ifdef MONITOR_LOG_SYNC
    static uint32_t                 cameraSyncLogId;
    int                             m_syncLogDuration;
    uint32_t                        m_getSyncLogId(void);
#endif
    /* process queue */
    List<ExynosCameraFrameSP_sptr_t >   m_processList;
    mutable Mutex                       m_processLock;
    List<ExynosCameraFrameSP_sptr_t >   m_captureProcessList;
    mutable Mutex                       m_captureProcessLock;
    frame_queue_t                       *m_pipeFrameDoneQ[MAX_PIPE_NUM];

    /* capture Queue */
    frame_queue_t                   *m_selectBayerQ;
    frame_queue_t                   *m_bayerStreamQ;
    frame_queue_t                   *m_captureQ;
    frame_queue_t                   *m_yuvCaptureDoneQ;
    frame_queue_t                   *m_reprocessingDoneQ;
#if defined(USES_CAMERA_EXYNOS_LEC)
    frame_queue_t                   *m_pipeFrameLecDoneQ;
#endif

    ExynosCameraList<ExynosCameraFrameSP_sptr_t> *m_shotDoneQ;
    status_t                        m_waitShotDone(enum Request_Sync_Type syncType, ExynosCameraFrameSP_dptr_t reqSyncFrame);
#ifdef USE_DUAL_CAMERA
#ifdef USE_DUAL_BAYER_SYNC
    frame_queue_t                   *m_syncBayerFrameDoneQ;
#endif
    frame_queue_t                   *m_selectDualSlaveBayerQ;
    create_frame_info_queue_t       *m_createReprocessingFrameQ;
    ExynosCameraList<ExynosCameraFrameSP_sptr_t> *m_slaveShotDoneQ;
    ExynosCameraList<dual_standby_trigger_info_t> *m_dualStandbyTriggerQ;
    uint32_t                        m_dualTransitionCount;
    uint32_t                        m_dualCaptureLockCount;
    mutable Mutex                   m_dualOperationModeLock;
    bool                            m_dualMultiCaptureLockflag;
    uint32_t                        m_earlyTriggerRequestKey;

    List<ExynosCameraRequestSP_sprt_t>  m_essentialRequestList;

    enum DUAL_OPERATION_MODE        m_earlyDualOperationMode;
    enum DUAL_OPERATION_SENSORS     m_earlyOperationSensor;
    int32_t                         m_earlyMasterCameraId;
    int32_t                         m_earlyDualRequestKey;
    enum DUAL_OPERATION_SENSORS     m_dualTransitionOperationSensor;

    volatile int32_t                m_needSlaveDynamicBayerCount;

    frame_queue_t                   *m_prepareBokehAnchorCaptureQ;
    frame_queue_t                   *m_prepareBokehAnchorMasterCaptureDoneQ;
    frame_queue_t                   *m_prepareBokehAnchorSlaveCaptureDoneQ;
    map<int, ExynosCameraFrameSP_sptr_t> m_prepareBokehAnchorMasterFrames;
    map<int, ExynosCameraFrameSP_sptr_t> m_prepareBokehAnchorSlaveFrames;
#endif
    List<ExynosCameraRequestSP_sprt_t>  m_latestRequestList;
    mutable Mutex                   m_latestRequestListLock;
    mutable Mutex                   m_needDynamicBayerCountLock;
    List<ExynosCameraRequestSP_sprt_t>  m_requestPreviewWaitingList;
    List<ExynosCameraRequestSP_sprt_t>  m_requestCaptureWaitingList;
    mutable Mutex                   m_requestPreviewWaitingLock;
    mutable Mutex                   m_requestCaptureWaitingLock;

    uint32_t                        m_firstRequestFrameNumber;
    int                             m_internalFrameCount;

    // TODO: Temporal. Efficient implementation is required.
    mutable Mutex                   m_updateMetaLock;

    mutable Mutex                   m_flushLock;
    bool                            m_flushLockWait;

    /* Thread */
    sp<mainCameraThread>            m_mainPreviewThread;
    bool                            m_mainPreviewThreadFunc(void);

    sp<mainCameraThread>            m_mainCaptureThread;
    bool                            m_mainCaptureThreadFunc(void);

#ifdef USE_DUAL_CAMERA
    bool                            m_flagFinishDualFrameFactoryStartThread;
    sp<mainCameraThread>            m_dualFrameFactoryStartThread; /* master or slave frameFactoryStartThread */
    bool                            m_dualFrameFactoryStartThreadFunc(void);
    int                             m_dualFrameFactoryStartResult;
    sp<mainCameraThread>            m_slaveMainThread;
    bool                            m_slaveMainThreadFunc(void);
    sp<mainCameraThread>            m_dualStandbyThread;
    bool                            m_dualStandbyThreadFunc(void);
    status_t                        m_modulateDualOperationMode(enum DUAL_OPERATION_MODE &newOperationMode,
                                            enum DUAL_OPERATION_MODE oldOperationMode,
                                            enum DUAL_OPERATION_SENSORS &newOperationSensor,
                                            enum DUAL_OPERATION_SENSORS oldOperationSensor);
#endif

    sp<ExynosCameraStreamThread>    m_previewStreamBayerThread;

    sp<ExynosCameraStreamThread>    m_previewStream3AAThread;

    sp<ExynosCameraStreamThread>    m_previewStreamGMVThread;
    bool                            m_previewStreamGMVPipeThreadFunc(void);

    sp<ExynosCameraStreamThread>    m_previewStreamISPThread;

    sp<ExynosCameraStreamThread>    m_previewStreamMCSCThread;

    sp<ExynosCameraStreamThread>    m_previewStreamGDCThread;

    sp<ExynosCameraStreamThread>    m_previewStreamVDISThread;

#ifdef USE_CLAHE_PREVIEW
    sp<ExynosCameraStreamThread>    m_previewStreamCLAHEThread;
#endif
#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    sp<ExynosCameraStreamThread>    m_previewStreamSWMCSCThread;
#endif
#ifdef USES_CAMERA_EXYNOS_VPL
    sp<mainCameraThread>            m_previewStreamNFDThread;
    bool                            m_previewStreamNFDPipeThreadFunc(void);
#endif
#if defined (USE_VRA_FD)
    sp<mainCameraThread>            m_previewStreamVRAThread;
    bool                            m_previewStreamVRAPipeThreadFunc(void);
#endif

#ifdef SUPPORT_HFD
    sp<mainCameraThread>            m_previewStreamHFDThread;
    bool                            m_previewStreamHFDPipeThreadFunc(void);
#endif

    sp<mainCameraThread>            m_previewStreamPPPipeThread;
    bool                            m_previewStreamPPPipeThreadFunc(void);

#ifdef USE_DUAL_CAMERA
    sp<mainCameraThread>            m_previewStreamBayerSyncThread;
    bool                            m_previewStreamBayerSyncPipeThreadFunc(void);

    sp<mainCameraThread>            m_previewStreamSyncThread;
    bool                            m_previewStreamSyncPipeThreadFunc(void);

    sp<mainCameraThread>            m_previewStreamFusionThread;
    bool                            m_previewStreamFusionPipeThreadFunc(void);

    sp<mainCameraThread>            m_selectDualSlaveBayerThread;
    bool                            m_selectDualSlaveBayerThreadFunc(void);

#ifdef USE_DUAL_BAYER_SYNC
    status_t                        m_pushBayerSyncFrame(ExynosCameraFrameSP_sptr_t bayerFrame);
    ExynosCameraFrameSP_sptr_t      m_waitAndPopBayerSyncFrame(frame_handle_components_t components, uint32_t frameCount);
#endif

    sp<mainCameraThread>            m_createReprocessingFrameThread;
    bool                            m_createReprocessingFrameThreadFunc(void);

    sp<mainCameraThread>            m_gscPreviewCbThread;
    bool                            m_gscPreviewCbThreadFunc();

    sp<mainCameraThread>            m_prepareBokehAnchorCaptureThread;
    bool                            m_prepareBokehAnchorCaptureThreadFunc();
    status_t                        m_createBokehSuperNightShotBayerAnchorFrame(ExynosCameraRequestSP_sprt_t request,
                                                                                ExynosCameraFrameFactory *targetfactory,
                                                                                frame_type_t internalframeType,
                                                                                map<int, int> scenarioList,
                                                                                frame_handle_components_t components,
                                                                                frame_queue_t *selectBayerQ,
                                                                                sp<mainCameraThread> selectBayerThread,
                                                                                int captureCount);
#endif

    map<int /* pipeId */, sp<ExynosCameraStreamThread>> m_previewStreamPlugInThreadMap;

    sp<mainCameraThread>            m_selectBayerThread;
    status_t                        m_selectBayerHandler(uint32_t pipeID, ExynosCameraFrameSP_sptr_t frame,
                                                                    ExynosCameraBuffer *bayerBuffer,
                                                                    ExynosCameraFrameSP_sptr_t bayerFrame,
                                                                    ExynosCameraFrameFactory *factory);
    bool                            m_selectBayerThreadFunc(void);
    bool                            m_selectBayer(ExynosCameraFrameSP_sptr_t frame);

    sp<mainCameraThread>            m_bayerStreamThread;
    bool                            m_bayerStreamThreadFunc(void);

    sp<mainCameraThread>            m_captureThread;
    bool                            m_captureThreadFunc(void);

#ifdef USES_VPL_PRELOAD
    sp<mainCameraThread>            m_vplPreloadThread;
    bool                            m_vplPreloadThreadFunc(void);
    void                            *m_vplDl;
    void                            (*m_vplPreLoad)(void);
    void                            (*m_vplUnload)(void);
    bool                            m_vplPreloadInitStatus;
#endif

    sp<mainCameraThread>            m_captureStreamThread;
    bool                            m_captureStreamThreadFunc(void);

    sp<mainCameraThread>            m_setBuffersThread;
    bool                            m_setBuffersThreadFunc(void);

    sp<mainCameraThread>            m_deinitBufferSupplierThread;
    bool                            m_deinitBufferSupplierThreadFunc(void);

    sp<mainCameraThread>            m_framefactoryCreateThread;
    bool                            m_frameFactoryCreateThreadFunc(void);
    int                             m_framefactoryCreateResult;

    sp<mainCameraThread>            m_dualFramefactoryCreateThread;
    bool                            m_dualFrameFactoryCreateThreadFunc(void);
    int                             m_dualFramefactoryCreateResult;

    sp<mainCameraThread>            m_reprocessingFrameFactoryStartThread;
    bool                            m_reprocessingFrameFactoryStartThreadFunc(void);

    sp<mainCameraThread>            m_startPictureBufferThread;
    bool                            m_startPictureBufferThreadFunc(void);

    sp<mainCameraThread>            m_frameFactoryStartThread;
    bool                            m_frameFactoryStartThreadFunc(void);
    int                             m_frameFactoryStartResult;

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    frame_queue_t                   *m_reverseProcessingBayerQ;
    sp<mainCameraThread>            m_reverseProcessingBayerThread;
    bool                            m_reverseProcessingBayerThreadFunc(void);
#endif

#ifdef SUPPORT_HW_GDC
    frame_queue_t                   *m_gdcQ;
    sp<mainCameraThread>            m_gdcThread;
    bool                            m_gdcThreadFunc(void);
#endif

    sp<mainCameraThread>            m_thumbnailCbThread;
    bool                            m_thumbnailCbThreadFunc(void);
    frame_queue_t                   *m_thumbnailCbQ;
    frame_queue_t                   *m_thumbnailPostCbQ;

    frame_queue_t                   *m_resizeDoneQ;
    frame_queue_t                   *m_resizeYuvDoneQ;

    status_t                        m_setSetfile(void);
    status_t                        m_setupPipeline(ExynosCameraFrameFactory *factory);
    status_t                        m_setupVisionPipeline(void);
    status_t                        m_setupReprocessingPipeline(ExynosCameraFrameFactory *factory);

    sp<mainCameraThread>            m_monitorThread;
    bool                            m_monitorThreadFunc(void);

#ifdef BUFFER_DUMP
    buffer_dump_info_queue_t        *m_dumpBufferQ;
    sp<mainCameraThread>            m_dumpThread;
    bool                            m_dumpThreadFunc(void);
#endif
    status_t                        m_setReprocessingBuffer(bool isRemosaic = false);
    status_t                        m_setVendorReprocessingBuffer(bool isRemosaic = false);

#ifdef SUPPORT_REMOSAIC_CAPTURE
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    sp<mainCameraThread>            m_setRemosaicBufferThread;
    sp<mainCameraThread>            m_releaseRemosaicBufferThread;
    bool                            m_releaseRemosaicBuffer(void);
#endif
    bool                            m_setRemosaicBuffer(void);
    status_t                        m_handleRemosaicCaptureFrame(ExynosCameraFrameSP_sptr_t frame, int pipeId);
#endif //SUPPORT_REMOSAIC_CAPTURE

    int32_t                         m_longExposureRemainCount;
    bool                            m_stopLongExposure;
    uint64_t                        m_preLongExposureTime;

    ExynosCameraBuffer              m_newLongExposureCaptureBuffer;

    int                             m_ionClient;

    bool                            m_checkFirstFrameLux;
    void                            m_checkEntranceLux(struct camera2_shot_ext *meta_shot_ext);
    status_t                        m_fastenAeStable(ExynosCameraFrameFactory *factory);
    status_t                        m_checkRestartStream(ExynosCameraRequestSP_sprt_t request);
    status_t                        m_restartStreamInternal();
    int                             m_getMcscLeaderPipeId(frame_handle_components_t *components);
#ifdef SUPPORT_DEPTH_MAP
    status_t                        m_setDepthInternalBuffer();
#endif

    void                            m_checkUseOnePort();
    void                            m_checkVideoStreamPriority();
    bool                            m_compareStreamPriority(int srcStreamId, int dstStreamId);
    void                            m_getOnePortStreamId();
    void                            m_initLockFrameHoldCount();
    status_t                        m_checkLockFrameHoldCount(frame_handle_components_t &components);
    void                            m_recheckModes();

#define PP_SCENARIO_NONE 0
    status_t                        m_setupPreviewGSC(ExynosCameraFrameSP_sptr_t frame,
                                                      ExynosCameraRequestSP_sprt_t request,
                                                      int pipeId, int subPipeId, bool useDstBuffer,
                                                      int pp_scenario = PP_SCENARIO_NONE);

    status_t                        m_copyOnePortBuf2PhysStreamBuf(ExynosCameraRequestSP_sprt_t request,
                                                            ExynosCameraFrameSP_sptr_t frame,
                                                            ExynosCameraBuffer *srcBuf,
                                                            int32_t streamID,
                                                            camera3_buffer_status_t streamBufferState);

    status_t                        m_copySrcBuf2DstBuf(ExynosCameraFrameSP_sptr_t frame,
                                                    ExynosCameraBuffer *srcBuffer,
                                                    ExynosCameraBuffer *dstBuffer,
                                                    ExynosRect *srcRect,
                                                    ExynosRect *dstRect,
                                                    bool flagMemcpy,
                                                    int32_t numOfPlanesSrc,
                                                    int32_t numOfPlanesDst);

void                                m_copyPreviewCbThreadFunc(ExynosCameraRequestSP_sprt_t request,
                                                            ExynosCameraFrameSP_sptr_t frame,
                                                            ExynosCameraBuffer *buffer);

    sp<class ExynosCameraGSCWrapper> m_gscWrapper;
    status_t                        m_copyPreview2Callback(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer *srcBuffer,
	                                                       ExynosCameraBuffer *dstBuffer);
    status_t                        m_copyStreamBuf(ExynosCameraFrameSP_sptr_t frame,
                                                   int32_t srcStreamId,
                                                   int32_t dstStreamId);

#ifdef USE_DUAL_CAMERA
    status_t                        m_handleCaptureFusionPlugin(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId, int nextPipeId);
    status_t                        m_updateAfterForceSwitchSolution(ExynosCameraFrameSP_sptr_t frame);
    status_t                        m_updateBeforeForceSwitchSolution(ExynosCameraFrameSP_sptr_t frame, int pipeId);
    status_t                        m_updateSizeBeforeDualFusion(ExynosCameraFrameSP_sptr_t frame, int pipeId);
    status_t                        m_updateBeforeDualSolution(ExynosCameraFrameSP_sptr_t frame, int pipeId);
    status_t                        m_updateAfterDualSolution(ExynosCameraFrameSP_sptr_t frame);
#endif
#ifdef SUPPORT_PD_IMAGE
    status_t                        m_setPDimageInternalBuffer();
#endif
    mutable Mutex                   m_frameCountLock;

    /* HACK : To prevent newly added member variable corruption
       (May caused by compiler bug??) */
    int                             m_currentMultiCaptureMode;
    int                             m_lastMultiCaptureServiceRequest;
    int                             m_lastMultiCaptureSkipRequest;
    int                             m_lastMultiCaptureNormalRequest;
    int                             m_doneMultiCaptureRequest;
    ExynosCameraDurationTimer       m_firstPreviewTimer;
    bool                            m_flagFirstPreviewTimerOn;

    ExynosCameraDurationTimer       m_previewDurationTimer[2];
    int                             m_previewDurationTime[2];
    uint32_t                        m_captureResultToggle;
    uint32_t                        m_displayPreviewToggle;
    int                             m_burstFps_history[4];

#ifdef SUPPORT_DEPTH_MAP
    bool                            m_flagUseInternalDepthMap;
#endif
#ifdef FPS_CHECK
    int32_t                         m_debugFpsCount[MAX_PIPE_NUM];
    ExynosCameraDurationTimer       m_debugFpsTimer[MAX_PIPE_NUM];
#endif
    struct camera2_stats_dm         m_stats;
    bool                            m_flagUseInternalyuvStall;
    bool                            m_flagVideoStreamPriority;
    bool                            m_flagUseOnePort;

#ifdef USES_SENSOR_LISTENER
    sp<mainCameraThread>            m_sensorListenerThread;
    bool                            m_sensorListenerThreadFunc(void);
    bool                            m_getSensorListenerData(frame_handle_components_t *components);
    sp<mainCameraThread>            m_sensorListenerUnloadThread;
    bool                            m_sensorListenerUnloadThreadFunc(void);

    void                            *m_gyroHandle;
    ExynosCameraSensorListener::Event_t  m_gyroListenerData;

    void                            *m_accelerometerHandle;
    ExynosCameraSensorListener::Event_t  m_accelerometerListenerData;

    void                            *m_rotationHandle;
    ExynosCameraSensorListener::Event_t  m_rotationListenerData;
#endif

/********************************************************************************/
/**                          VENDOR                                            **/
/********************************************************************************/

private:
    void                            m_vendorSpecificPreConstructorInitalize(int cameraId, int scenario);
    void                            m_vendorSpecificConstructorInitalize(void);
    void                            m_vendorSpecificPreDestructorDeinitalize(void);
    void                            m_vendorSpecificDestructorDeinitalize(void);
    void                            m_vendorUpdateExposureTime(struct camera2_shot_ext *shot_ext);
    status_t                        setParameters_vendor(const CameraParameters &params);
    status_t                        m_checkMultiCaptureMode_vendor_update(ExynosCameraRequestSP_sprt_t request);
    status_t                        processCaptureRequest_vendor_initDualSolutionZoom(camera3_capture_request *request, status_t &ret);
    status_t                        processCaptureRequest_vendor_initDualSolutionPortrait(camera3_capture_request *request, status_t &ret);

    status_t                        m_captureFrameHandler_vendor_updateConfigMode(ExynosCameraRequestSP_sprt_t request,
                                                                        ExynosCameraFrameFactory *targetfactory,
                                                                        frame_type_t &frameType);

    status_t                        m_captureFrameHandler_vendor_updateDualROI(ExynosCameraRequestSP_sprt_t request,
                                                                        frame_handle_components_t &components,
                                                                        frame_type_t frameType);

    status_t                        m_captureFrameHandler_vendor_updateIntent(ExynosCameraRequestSP_sprt_t request,
                                                                        frame_handle_components_t &components);

    void                            m_updateCaptureIntent(ExynosCameraRequestSP_sprt_t request,
                                                          frame_handle_components_t& components);

    void                            m_updateMetaDataCaptureIntent(struct camera2_shot_ext *shot_ext, uint32_t frameType);

    status_t                        m_previewFrameHandler_vendor_updateRequest(ExynosCameraFrameFactory *targetfactory);

    status_t                        m_handlePreviewFrame_vendor_handleBuffer(ExynosCameraRequestSP_sprt_t request,
                                                                             ExynosCameraFrameSP_sptr_t frame, int pipeId,
                                                                             ExynosCameraFrameFactory *factory, frame_handle_components_t &components,
                                                                             status_t &ret);

    status_t                        m_checkStreamInfo_vendor(status_t &ret);
    void                            m_updateTnr(struct camera2_shot_ext *shot_ext, ExynosCameraParameters *parameters);
    status_t                        m_checkDynamicBayerMode(void);
    void                            m_checkVendorDynamicBayerMode(bool &useDynamicBayer);

#ifdef USE_SLSI_PLUGIN
    status_t                        m_checkCaptureMode(__unused ExynosCameraRequestSP_sprt_t request, frame_handle_components_t& components);
    status_t                        m_prepareCaptureMode(__unused ExynosCameraRequestSP_sprt_t request,
                                                                __unused frame_type_t frameType,
                                                                frame_handle_components_t& components);
    status_t                        m_prepareCapturePlugin(__unused ExynosCameraFrameFactory *targetfactory,
                                                                __unused frame_type_t frameType,
                                                                __unused list<int> &preSceanrio,
                                                                __unused map<int, int> *scenarioList);
    status_t                        m_setupCapturePlugIn(ExynosCameraFrameSP_sptr_t frame,
                                                         int pipeId, int subPipeId, int pipeId_next,
                                                         ExynosCameraBuffer *srcBuffer = NULL,
                                                         ExynosCameraBuffer *dstBuffer = NULL);
    status_t                        m_handleCaptureFramePlugin(ExynosCameraFrameSP_sptr_t frame,
                                                            int leaderPipeId,
                                                            int &pipeId_next,
                                                            int &nodePipeId);
#endif

    int                             m_highSpeedModeDecision(camera3_stream_configuration *streamList);

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
    status_t                        m_setLedCalibration(struct camera2_shot_ext *shot_ext);
    status_t                        m_startLedCalibration(ExynosCameraFrameFactory *factory);
    status_t                        m_stopLedCalibration(ExynosCameraFrameFactory *factory);
    status_t                        m_dumpLedCalibrationFile(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer bayerBuffer);

    bool                            m_flagSetLedCalibration;
    bool                            m_flagOldSetLedCalibration;
#endif

#ifdef USES_SENSOR_GYRO_FACTORY_MODE
    ExynosCameraFactoryTestSensorGyro *m_sensorGyroTest;
    int                                m_sensorGyroTestIndex;
#endif
    void                            m_updateCropRegion_vendor(struct camera2_shot_ext *shot_ext,
                                                              frame_handle_components_t *components,
                                                              frame_type_t frameType,
                                                              ExynosRect &targetActiveZoomRect,
                                                              int sensorActiveMaxW, int sensorActiveMaxH,
                                                              bool isReprocessing);
    bool                            isSupportedDebugInfoPlane(__unused enum pipeline pipeId, int *planeCount);
private:
#ifdef USES_SW_VDIS
    ExynosCameraSolutionSWVdis      *m_exCameraSolutionSWVdis;
#endif

#ifdef USE_HW_RAW_REVERSE_PROCESSING
    uint32_t                         m_oldSetfile;
#endif

    ExynosCameraBuffer               m_prePlugInDstBuffer;
    ExynosCameraBuffer               m_postPlugInDstBuffer;

    bool                             m_flagThreadCreated;
    int                              m_cameraSessionId;
};

}; /* namespace android */
#endif
