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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera"
#include <log/log.h>
#include <ui/Fence.h>

#include "ExynosCamera.h"
#include "ExynosCameraProperty.h"

#include "ExynosCameraResourceManager.h"

#include "ExynosGraphicBuffer.h"

#ifdef USES_OFFLINE_CAPTURE
#include "OfflineProcessing/ExynosCameraOfflineCapture.h"
#endif

using vendor::graphics::BufferUsage;
using vendor::graphics::ExynosGraphicBufferUsage;
using vendor::graphics::ExynosGraphicBufferMeta;

namespace android {

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera::cameraSyncLogId = 0;
#endif

ExynosCamera::ExynosCamera(cameraId_Info *camIdInfo, camera_metadata_t **info):
    m_requestMgr(NULL),
    m_streamManager(NULL)
{
    BUILD_DATE();

    memset(m_cameraIds, -1, sizeof(m_cameraIds));
    m_camIdInfo = *camIdInfo;

    m_resourceManager = NULL;

    m_yuvCaptureDoneQ = NULL;
    m_reprocessingDoneQ = NULL;
    m_bayerStreamQ = NULL;
    m_captureQ = NULL;

    /* Camera Scenario */
    m_scenario = m_camIdInfo.scenario;

    /* Main Camera/Sub Camera.... */
    m_cameraId = m_camIdInfo.cameraId[MAIN_CAM];

    if (m_camIdInfo.numOfSensors > MAX_NUM_SENSORS) {
        CLOGE("numOfSensors (%d) > MAX_NUM_SENSORS(%d)", m_camIdInfo.numOfSensors, MAX_NUM_SENSORS);
        m_camIdInfo.numOfSensors = MAX_NUM_SENSORS;
    }
    
#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    m_reverseProcessingBayerQ = NULL;
#endif

#ifdef USE_DUAL_CAMERA
    m_prepareBokehAnchorCaptureQ = NULL;
    m_prepareBokehAnchorMasterCaptureDoneQ = NULL;
    m_prepareBokehAnchorSlaveCaptureDoneQ = NULL;

    m_createReprocessingFrameQ = NULL;
    m_selectDualSlaveBayerQ = NULL;
#ifdef USE_DUAL_BAYER_SYNC
    m_syncBayerFrameDoneQ = NULL;
#endif
#endif

#ifdef USES_OFFLINE_CAPTURE
    m_offlineCapture = NULL;
#endif

    m_flagThreadCreated = false;
    m_cameraSessionId = 0;

    m_flagFirstPreviewTimerOn = false;
    m_flagUseInternalyuvStall = false;
    m_flagVideoStreamPriority = false;
    m_flagUseOnePort = false;
    m_captureResultToggle = 0;
    m_displayPreviewToggle = 0;
#ifdef SUPPORT_DEPTH_MAP
    m_flagUseInternalDepthMap = false;
#endif
#ifdef USES_SENSOR_LISTENER
    m_gyroHandle = NULL;
    m_rotationHandle = NULL;
#endif
#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
    m_flagSetLedCalibration = false;
    m_flagOldSetLedCalibration = false;
#endif
#ifdef USES_SENSOR_GYRO_FACTORY_MODE
    m_sensorGyroTest = NULL;
    m_sensorGyroTestIndex = -1;
#endif
#ifdef USES_SW_VDIS
    m_exCameraSolutionSWVdis = NULL;
#endif
#ifdef USE_HW_RAW_REVERSE_PROCESSING
    m_oldSetfile = 0;
#endif
    memset(&m_stats, 0, sizeof(m_stats));

#ifdef USES_VPL_PRELOAD
    m_vplDl = nullptr;
    m_vplPreLoad = nullptr;
    m_vplUnload = nullptr;
#endif
}

status_t ExynosCamera::createDevice(cameraId_Info *camIdInfo, camera_metadata_t **info)
{
    CLOGD("");

    status_t ret = NO_ERROR;

    for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
        m_camIdMap.insert(pair<int32_t, int32_t>(m_camIdInfo.cameraId[i], i));

        m_cameraIds[i] = m_camIdInfo.cameraId[i];
        CLOGI("CameraId_%d (%d)", i, m_camIdInfo.cameraId[i]);
    }
    m_isLogicalCam = isLogicalCam(m_camIdInfo.serviceCameraId);

    if (getCamName(m_cameraId, m_name, sizeof(m_name)) != NO_ERROR) {
        memset(m_name, 0x00, sizeof(m_name));
        CLOGE("Invalid camera ID(%d)", m_cameraId);
    }

    if (m_camIdInfo.numOfSensors > 1) {
        CLOGI("Multi cameara mode is enabled! : m_scenario(%d) m_isLogicalCam(%d) serviceCameraId(%d)",
            m_scenario, m_isLogicalCam, m_camIdInfo.serviceCameraId);
    } else {
        CLOGI("Single cameara enabled! CameraId (%d) m_scenario(%d)", m_cameraIds[0], m_scenario);
    }

    /* Initialize pointer variables */
    m_ionAllocator = NULL;

    m_bufferSupplier = NULL;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_parameters[i] = NULL;
        m_captureSelector[i] = NULL;
        m_activityControl[i] = NULL;

        m_currentPreviewShot[i] = NULL;
        m_currentInternalShot[i] = NULL;
        m_currentCaptureShot[i] = NULL;
        m_currentVisionShot[i] = NULL;
    }

    m_captureZslSelector = NULL;

    m_vendorSpecificPreConstructor(m_cameraId, m_scenario);

    if (m_resourceManager == NULL) {
        m_resourceManager = new ExynosCameraResourceManager(m_cameraId, m_name);
    } else {
        CLOGD("[resourceMgr] exist resourceManager for this camera(%d)", m_cameraId);
    }

    m_resourceManager->initResources(&m_camIdInfo, &m_configurations, m_parameters, &m_metadataConverter);
    if (m_configurations == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):configuration is NULL", __FUNCTION__, __LINE__);
    }

#ifdef USES_OFFLINE_CAPTURE
    if (m_offlineCapture == NULL) {
        m_offlineCapture = new ExynosCameraOfflineCapture(m_cameraId);
    } else {
        //android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):m_offlineCapture is already allocated", __FUNCTION__, __LINE__);
    }
#endif

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_parameters[i] != NULL) {
            m_activityControl[i] = m_parameters[i]->getActivityControl();
        } else {
            if (m_parameters[m_cameraId] == NULL) {
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):m_parameters[%d] is NOT allocated", __FUNCTION__, __LINE__, i);
            }
        }
    }


    /* Create related classes */
    switch (m_scenario) {
#ifdef USE_DUAL_CAMERA
    case SCENARIO_DUAL_REAR_ZOOM:
    case SCENARIO_DUAL_REAR_PORTRAIT:
    case SCENARIO_DUAL_FRONT_PORTRAIT:
        for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
            m_currentPreviewShot[m_camIdInfo.cameraId[i]] = new struct camera2_shot_ext;
            memset(m_currentPreviewShot[m_camIdInfo.cameraId[i]], 0x00, sizeof(struct camera2_shot_ext));

            m_currentInternalShot[m_camIdInfo.cameraId[i]] = new struct camera2_shot_ext;
            memset(m_currentInternalShot[m_camIdInfo.cameraId[i]], 0x00, sizeof(struct camera2_shot_ext));

            m_currentCaptureShot[m_camIdInfo.cameraId[i]] = new struct camera2_shot_ext;
            memset(m_currentCaptureShot[m_camIdInfo.cameraId[i]], 0x00, sizeof(struct camera2_shot_ext));
        }
        /* No break: m_parameters[0] is same with normal */
#endif
    case SCENARIO_NORMAL:
    default:
        m_currentPreviewShot[m_cameraId] = new struct camera2_shot_ext;
        memset(m_currentPreviewShot[m_cameraId], 0x00, sizeof(struct camera2_shot_ext));

        m_currentInternalShot[m_cameraId] = new struct camera2_shot_ext;
        memset(m_currentInternalShot[m_cameraId], 0x00, sizeof(struct camera2_shot_ext));

        m_currentCaptureShot[m_cameraId] = new struct camera2_shot_ext;
        memset(m_currentCaptureShot[m_cameraId], 0x00, sizeof(struct camera2_shot_ext));

        break;
    }

    m_resourceManager->initManagerResources(camIdInfo, &m_requestMgr, nullptr, nullptr);
    m_requestMgr->setMetaDataConverter(m_metadataConverter);

    /* Create managers */
    m_createManagers();

    /* Create queue for preview path. If you want to control pipeDone in ExynosCamera, try to create frame queue here */
    m_shotDoneQ = new ExynosCameraList<ExynosCameraFrameSP_sptr_t>();
    m_shotDoneQ->setWaitTime(4000000000);
    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        switch (i) {
        case PIPE_FLITE:
        case PIPE_3AA:
        case PIPE_ISP:
        case PIPE_MCSC:
#ifdef USE_CLAHE_PREVIEW
        case PIPE_CLAHE:
#endif
#ifdef SUPPORT_HW_GDC
        case PIPE_GDC:
#endif
#ifdef USE_SLSI_PLUGIN
        case PIPE_PLUGIN_BASE ... PIPE_PLUGIN_MAX:
#endif
#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
        case PIPE_SW_MCSC:
#endif
        case PIPE_VDIS:
            m_pipeFrameDoneQ[i] = new frame_queue_t;
            break;
        default:
            m_pipeFrameDoneQ[i] = NULL;
            break;
        }
    }

#ifdef USES_CAMERA_EXYNOS_VPL
    m_pipeFrameDoneQ[PIPE_NFD] = new frame_queue_t(m_previewStreamNFDThread);
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    m_pipeFrameLecDoneQ = new frame_queue_t();
#endif

    /* Create threads */
    if (m_flagThreadCreated == false) {
        m_createThreads();
    } else {
        CLOGI("Threads are created");
    }

    m_setFrameDoneQtoThread();

    if (m_configurations->getMode(CONFIGURATION_GMV_MODE) == true) {
        m_pipeFrameDoneQ[PIPE_GMV] = new frame_queue_t(m_previewStreamGMVThread);
    }
#ifdef USE_VRA_FD
    m_pipeFrameDoneQ[PIPE_VRA] = new frame_queue_t(m_previewStreamVRAThread);
#endif
#ifdef SUPPORT_HFD
    if (m_parameters[m_cameraId]->getHfdMode() == true) {
        m_pipeFrameDoneQ[PIPE_HFD] = new frame_queue_t(m_previewStreamHFDThread);
    }
#endif

#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        m_slaveShotDoneQ = new ExynosCameraList<ExynosCameraFrameSP_sptr_t>();
        m_slaveShotDoneQ->setWaitTime(4000000000);
        m_dualStandbyTriggerQ = new ExynosCameraList<dual_standby_trigger_info_t>();
#ifdef USE_DUAL_BAYER_SYNC
        m_pipeFrameDoneQ[PIPE_BAYER_SYNC] = new frame_queue_t;
#endif
        m_pipeFrameDoneQ[PIPE_SYNC] = new frame_queue_t;
        m_pipeFrameDoneQ[PIPE_FUSION] = new frame_queue_t;
        m_pipeFrameDoneQ[PIPE_GSC] = new frame_queue_t;
        m_dualTransitionCount = 0;
        m_dualCaptureLockCount = 0;
        m_dualMultiCaptureLockflag = false;
        m_earlyTriggerRequestKey = 0;
        m_flagFinishDualFrameFactoryStartThread = false;
    } else {
        m_slaveShotDoneQ = NULL;
        m_dualStandbyTriggerQ = NULL;
        m_pipeFrameDoneQ[PIPE_BAYER_SYNC] = NULL;
        m_pipeFrameDoneQ[PIPE_SYNC] = NULL;
        m_pipeFrameDoneQ[PIPE_FUSION] = NULL;
        m_dualTransitionCount = 0;
        m_dualCaptureLockCount = 0;
        m_dualMultiCaptureLockflag = false;
        m_earlyTriggerRequestKey = 0;
        m_flagFinishDualFrameFactoryStartThread = false;
    }

    m_earlyOperationSensor = m_dualTransitionOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    m_earlyDualOperationMode = DUAL_OPERATION_MODE_NONE;
    m_earlyMasterCameraId = m_cameraId;
    m_earlyDualRequestKey = -1;
#endif

    for(int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++)
        m_frameFactory[i] = NULL;

    m_frameFactoryQ = new framefactory3_queue_t;
    m_dualFrameFactoryQ = new framefactory3_queue_t;
    m_selectBayerQ = new frame_queue_t();

    m_createCaptureStreamQ();

#ifdef SUPPORT_HW_GDC
    m_gdcQ = new frame_queue_t(m_gdcThread);
#endif

    /* construct static meta data information */
    if (*info == NULL) {
        if (ExynosCameraMetadataConverter::constructStaticInfo(&m_camIdInfo, info, NULL))
            CLOGE("Create static meta data failed!!");
    }

    m_metadataConverter->setStaticInfo(&m_camIdInfo, *info);

    m_streamManager->setYuvStreamMaxCount(m_parameters[m_cameraId]->getYuvStreamMaxNum());

    m_setFrameManager();

    m_setConfigInform();

    /* init infomation of fd orientation*/
    m_configurations->setModeValue(CONFIGURATION_DEVICE_ORIENTATION, 0);
    m_configurations->setModeValue(CONFIGURATION_FD_ORIENTATION, 0);
    ExynosCameraActivityUCTL *uctlMgr = m_activityControl[m_cameraId]->getUCTLMgr();
    if (uctlMgr != NULL) {
        uctlMgr->setDeviceRotation(m_configurations->getModeValue(CONFIGURATION_FD_ORIENTATION));
    }

    m_state = EXYNOS_CAMERA_STATE_OPEN;

    m_visionFps = 0;

    m_flushLockWait = false;
    m_captureStreamExist = false;
    m_rawStreamExist = false;
    m_videoStreamExist = false;

    m_recordingEnabled = false;
    m_prevStreamBit = 0;

    m_firstRequestFrameNumber = 0;
    m_internalFrameCount = 1;
#ifdef MONITOR_LOG_SYNC
    m_syncLogDuration = 0;
#endif
    m_lastFrametime = 0;
    m_prepareFrameCount = 0;
#ifdef BUFFER_DUMP
    m_dumpBufferQ = new buffer_dump_info_queue_t(m_dumpThread);
#endif

    m_ionClient = exynos_ion_open();
    if (m_ionClient < 0) {
        ALOGE("ERR(%s):m_ionClient ion_open() fail", __func__);
        m_ionClient = -1;
    }

    m_framefactoryCreateResult = NO_ERROR;
    m_dualFramefactoryCreateResult = NO_ERROR;
    m_flagFirstPreviewTimerOn = false;
    m_gscWrapper = NULL;

    /* Construct vendor */
    m_vendorSpecificConstructor();

#ifdef USES_SW_VDIS //Need to move at configureStream() : Instanciate VDIS solution object after checking VDIS running condition
    m_exCameraSolutionSWVdis = new ExynosCameraSolutionSWVdis(m_cameraId, PIPE_VDIS, m_parameters[m_cameraId], m_configurations);
#endif

    return ret;
}

status_t ExynosCamera::destroyDevice()
{
    CLOGD("");

    status_t ret = NO_ERROR;

    m_deinitFrameFactory();

    m_resourceManager->deInitResources(&m_camIdInfo, &m_configurations, m_parameters, &m_metadataConverter);

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_captureSelector[i] != NULL) {
            m_captureSelector[i] = NULL;
        }
    }

    if (m_captureZslSelector != NULL) {
        m_captureZslSelector = NULL;
    }

    return ret;
}

status_t  ExynosCamera::m_setConfigInform() {
    struct ExynosConfigInfo exynosConfig;
    memset((void *)&exynosConfig, 0x00, sizeof(exynosConfig));

    exynosConfig.mode = CONFIG_MODE::NORMAL;

    /* Internal buffers */
#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_sensor_buffers = DUAL_NUM_SENSOR_BUFFERS;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_3aa_buffers = DUAL_NUM_3AA_BUFFERS;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_hwdis_buffers = DUAL_NUM_HW_DIS_BUFFERS;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.dual_num_fusion_buffers = DUAL_NUM_SYNC_FUSION_BUFFERS;
    } else
#endif
    {
        if (isFrontCamera(getCameraId())) {
            exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_sensor_buffers = FRONT_NUM_SENSOR_BUFFERS;
            exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_3aa_buffers = FRONT_NUM_3AA_BUFFERS;
        } else {
            exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_sensor_buffers = NUM_SENSOR_BUFFERS;
            exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_3aa_buffers = NUM_3AA_BUFFERS;
        }
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_hwdis_buffers = NUM_HW_DIS_BUFFERS;
    }
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_nv21_picture_buffers = NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    /* v4l2_reqBuf() values for stream buffers */
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
    /* Required stream buffers for HAL */
#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_preview_buffers = DUAL_NUM_REQUEST_PREVIEW_BUFFER;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_callback_buffers = DUAL_NUM_REQUEST_CALLBACK_BUFFER;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_video_buffers = DUAL_NUM_REQUEST_VIDEO_BUFFER;
    } else
#endif
    {
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_preview_buffers = NUM_REQUEST_PREVIEW_BUFFER;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_callback_buffers = NUM_REQUEST_CALLBACK_BUFFER;
        exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_video_buffers = NUM_REQUEST_VIDEO_BUFFER;
    }
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_raw_buffers = NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_bayer_buffers = NUM_REQUEST_BAYER_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_burst_capture_buffers = NUM_REQUEST_BURST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_capture_buffers = NUM_REQUEST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_batch_buffers = 1;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_FLITE] = PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_3AA] = PIPE_3AA_PREPARE_COUNT;

    /* Config HIGH_SPEED 60 buffer & pipe info */
    /* Internal buffers */
#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_sensor_buffers = DUAL_NUM_SENSOR_BUFFERS;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_3aa_buffers = DUAL_NUM_3AA_BUFFERS;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_hwdis_buffers = DUAL_NUM_HW_DIS_BUFFERS;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.dual_num_fusion_buffers = DUAL_NUM_SYNC_FUSION_BUFFERS;
    } else
#endif
    {
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_sensor_buffers = NUM_SENSOR_BUFFERS;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_3aa_buffers = NUM_3AA_BUFFERS;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_hwdis_buffers = NUM_HW_DIS_BUFFERS;
    }
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_nv21_picture_buffers = NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    /* v4l2_reqBuf() values for stream buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
    /* Required stream buffer for HAL */
#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_preview_buffers = DUAL_NUM_REQUEST_PREVIEW_BUFFER;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_callback_buffers = DUAL_NUM_REQUEST_CALLBACK_BUFFER;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_video_buffers = DUAL_NUM_REQUEST_VIDEO_BUFFER;
    } else
#endif
    {
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_preview_buffers = NUM_REQUEST_PREVIEW_BUFFER;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_callback_buffers = NUM_REQUEST_CALLBACK_BUFFER;
        exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_video_buffers = NUM_REQUEST_VIDEO_BUFFER;
    }
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_raw_buffers = NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_bayer_buffers = NUM_REQUEST_BAYER_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_burst_capture_buffers = NUM_REQUEST_BURST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_request_capture_buffers = NUM_REQUEST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_batch_buffers = 1;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_FLITE] = PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_3AA] = PIPE_3AA_PREPARE_COUNT;

#if (USE_HIGHSPEED_RECORDING)
    /* Config HIGH_SPEED 120 buffer & pipe info */
    /* Internal buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_sensor_buffers = FPS120_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_3aa_buffers = FPS120_NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_hwdis_buffers = FPS120_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_nv21_picture_buffers = NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    /* v4l2_reqBuf() values for stream buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
    /* Required stream buffers for HAL */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_raw_buffers = FPS120_NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_bayer_buffers = FPS120_NUM_REQUEST_BAYER_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_preview_buffers = FPS120_NUM_REQUEST_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_callback_buffers = FPS120_NUM_REQUEST_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_video_buffers = FPS120_NUM_REQUEST_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_burst_capture_buffers = FPS120_NUM_REQUEST_BURST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_capture_buffers = FPS120_NUM_REQUEST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_batch_buffers = 2;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_FLITE] = FPS120_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_3AA] = FPS120_PIPE_3AA_PREPARE_COUNT;

    /* Config HIGH_SPEED 240 buffer & pipe info */
    /* Internal buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_sensor_buffers = FPS240_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_3aa_buffers = FPS240_NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_hwdis_buffers = FPS240_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_nv21_picture_buffers = NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    /* v4l2_reqBuf() values for stream buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
    /* Required stream buffers for HAL */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_raw_buffers = FPS240_NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_bayer_buffers = FPS240_NUM_REQUEST_BAYER_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_preview_buffers = FPS240_NUM_REQUEST_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_callback_buffers = FPS240_NUM_REQUEST_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_video_buffers = FPS240_NUM_REQUEST_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_burst_capture_buffers = FPS240_NUM_REQUEST_BURST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_request_capture_buffers = FPS240_NUM_REQUEST_CAPTURE_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_batch_buffers = 4;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_FLITE] = FPS240_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_3AA] = FPS240_PIPE_3AA_PREPARE_COUNT;

    /* Config HIGH_SPEED 480 buffer & pipe info */
    /* Internal buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_sensor_buffers = FPS480_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_3aa_buffers = FPS480_NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_hwdis_buffers = FPS480_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.reprocessing_bayer_hold_count = REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_nv21_picture_buffers = NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    /* v4l2_reqBuf() values for stream buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
    /* Required stream buffers for HAL */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_raw_buffers = FPS480_NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_bayer_buffers = NUM_REQUEST_BAYER_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_preview_buffers = FPS480_NUM_REQUEST_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_callback_buffers = FPS480_NUM_REQUEST_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_video_buffers = FPS480_NUM_REQUEST_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_request_jpeg_buffers = FPS480_NUM_REQUEST_JPEG_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].bufInfo.num_batch_buffers = 8;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].pipeInfo.prepare[PIPE_FLITE] = FPS480_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_480].pipeInfo.prepare[PIPE_3AA] = FPS480_PIPE_3AA_PREPARE_COUNT;
#endif

    m_configurations->setConfig(&exynosConfig);
    m_exynosconfig = m_configurations->getConfig();

    return NO_ERROR;
}

void ExynosCamera::m_createThreads(void)
{

#ifdef USES_VPL_PRELOAD
    m_vplPreloadInitStatus = false;
    m_vplPreloadThread = new mainCameraThread(this, &ExynosCamera::m_vplPreloadThreadFunc, "vplPreloadThreadFunc");
    CLOGD("vplPreload thread is created");
    m_vplPreloadThread->run(PRIORITY_URGENT_DISPLAY);
    CLOGD("vplPreload thread is started");
#endif
    m_mainPreviewThread = new mainCameraThread(this, &ExynosCamera::m_mainPreviewThreadFunc, "m_mainPreviewThreadFunc");
    CLOGD("Main Preview thread created");

    m_mainCaptureThread = new mainCameraThread(this, &ExynosCamera::m_mainCaptureThreadFunc, "m_mainCaptureThreadFunc");
    CLOGD("Main Capture thread created");

    /* m_previewStreamXXXThread is for seperated frameDone each own handler */
    m_previewStreamBayerThread = new ExynosCameraStreamThread(this, "PreviewBayerThread", PIPE_FLITE);
    CLOGD("Bayer Preview stream thread created");

    m_previewStream3AAThread = new ExynosCameraStreamThread(this, "Preview3AAThread", PIPE_3AA);
    CLOGD("3AA Preview stream thread created!!!");

#ifdef SUPPORT_GMV
    if (m_configurations->getMode(CONFIGURATION_GMV_MODE) == true) {
        m_previewStreamGMVThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamGMVPipeThreadFunc, "PreviewGMVThread");
        CLOGD("GMV Preview stream thread created");
    }
#endif

    m_previewStreamISPThread = new ExynosCameraStreamThread(this, "PreviewISPThread", PIPE_ISP);
    CLOGD("ISP Preview stream thread created");

    m_previewStreamMCSCThread = new ExynosCameraStreamThread(this, "PreviewMCSCThread", PIPE_MCSC);
    CLOGD("MCSC Preview stream thread created");

#ifdef USE_CLAHE_PREVIEW
    m_previewStreamCLAHEThread = new ExynosCameraStreamThread(this, "PreviewCLAHEThread", PIPE_CLAHE);
    CLOGD("CLAHE Preview stream thread created");
#endif

#ifdef SUPPORT_HW_GDC
    m_previewStreamGDCThread = new ExynosCameraStreamThread(this, "PreviewGDCThread", PIPE_GDC);
    CLOGD("GDC Preview stream thread created");
#endif

#ifdef USE_SLSI_PLUGIN
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        sp<ExynosCameraStreamThread> plugInThread = new ExynosCameraStreamThread(this, "PreviewPlugInThread", i);
        m_previewStreamPlugInThreadMap[i] = plugInThread;
        CLOGD("Preview PlugIn%d stream thread created", i);
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    m_previewStreamSWMCSCThread = new ExynosCameraStreamThread(this, "PreviewSWMCSCThread", PIPE_SW_MCSC);
    CLOGD("SWMCSC Preview stream thread created");
#endif

#ifdef USES_SW_VDIS
    m_previewStreamVDISThread = new ExynosCameraStreamThread(this, "PreviewVDISThread", PIPE_VDIS);
    CLOGD("VDIS Preview stream thread created");
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    m_previewStreamNFDThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamNFDPipeThreadFunc, "PreviewNFDThread");
    CLOGD("NFD Preview stream thread created(%p)", m_pipeFrameDoneQ[PIPE_NFD]);
#endif

#ifdef USE_VRA_FD
    m_previewStreamVRAThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamVRAPipeThreadFunc, "PreviewVRAThread");
    CLOGD("VRA Preview stream thread created");
#endif

#ifdef SUPPORT_HFD
    if (m_parameters[m_cameraId]->getHfdMode() == true) {
        m_previewStreamHFDThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamHFDPipeThreadFunc, "PreviewHFDThread");
        CLOGD("HFD Preview stream thread created");
    }
#endif

#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        m_slaveMainThread = new mainCameraThread(this, &ExynosCamera::m_slaveMainThreadFunc, "m_slaveMainThreadFunc");
        CLOGD("Dual Slave Main thread created");

        m_dualStandbyThread = new mainCameraThread(this, &ExynosCamera::m_dualStandbyThreadFunc, "m_setStandbyThreadFunc");
        CLOGD("Dual Standby thread created");

#ifdef USE_DUAL_BAYER_SYNC
        m_previewStreamBayerSyncThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamBayerSyncPipeThreadFunc, "PreviewBayerSyncThread");
        CLOGD("Bayer Sync for bayer buffer thread created");
#endif

        m_previewStreamSyncThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamSyncPipeThreadFunc, "PreviewSyncThread");
        CLOGD("Sync Preview stream thread created");

        m_previewStreamFusionThread = new mainCameraThread(this, &ExynosCamera::m_previewStreamFusionPipeThreadFunc, "PreviewFusionThread");
        CLOGD("Fusion Preview stream thread created");

        m_dualFrameFactoryStartThread = new mainCameraThread(this, &ExynosCamera::m_dualFrameFactoryStartThreadFunc, "DualFrameFactoryStartThread");
        CLOGD("DualFrameFactoryStartThread created");

        m_selectDualSlaveBayerThread = new mainCameraThread(this, &ExynosCamera::m_selectDualSlaveBayerThreadFunc, "SelectDualSlaveBayerThreadFunc");
        CLOGD("SelectDualSlaveBayerThread created");

        m_createReprocessingFrameThread = new mainCameraThread(this, &ExynosCamera::m_createReprocessingFrameThreadFunc, "m_createReprocessingFrameThreadFunc", PRIORITY_URGENT_DISPLAY);
        CLOGD("m_createReprocessingMasterFrameThread created");

        m_gscPreviewCbThread = new mainCameraThread(this, &ExynosCamera::m_gscPreviewCbThreadFunc, "m_gscPreviewCbThread");
        CLOGD("m_gscPreviewCbThread created");

        m_prepareBokehAnchorCaptureThread = new mainCameraThread(this, &ExynosCamera::m_prepareBokehAnchorCaptureThreadFunc, "m_prepareBokehAnchorCaptureThread");
        CLOGD("m_prepareBokehAnchorCaptureThread created");
    }
#endif

    m_selectBayerThread = new mainCameraThread(this, &ExynosCamera::m_selectBayerThreadFunc, "SelectBayerThreadFunc");
    CLOGD("SelectBayerThread created");

    m_bayerStreamThread = new mainCameraThread(this, &ExynosCamera::m_bayerStreamThreadFunc, "BayerStreamThread");
    CLOGD("Bayer stream thread created");

    m_captureThread = new mainCameraThread(this, &ExynosCamera::m_captureThreadFunc, "CaptureThreadFunc");
    CLOGD("CaptureThread created");

    m_captureStreamThread = new mainCameraThread(this, &ExynosCamera::m_captureStreamThreadFunc, "CaptureThread");
    CLOGD("Capture stream thread created");

    m_setBuffersThread = new mainCameraThread(this, &ExynosCamera::m_setBuffersThreadFunc, "setBuffersThread");
    CLOGD("Buffer allocation thread created");

    m_framefactoryCreateThread = new mainCameraThread(this, &ExynosCamera::m_frameFactoryCreateThreadFunc, "FrameFactoryCreateThread");
    CLOGD("FrameFactoryCreateThread created");

    m_dualFramefactoryCreateThread = new mainCameraThread(this, &ExynosCamera::m_dualFrameFactoryCreateThreadFunc, "dualFrameFactoryCreateThread");
    CLOGD("DualFrameFactoryCreateThread created");

    m_reprocessingFrameFactoryStartThread = new mainCameraThread(this, &ExynosCamera::m_reprocessingFrameFactoryStartThreadFunc, "m_reprocessingFrameFactoryStartThread");
    CLOGD("m_reprocessingFrameFactoryStartThread created");

    m_startPictureBufferThread = new mainCameraThread(this, &ExynosCamera::m_startPictureBufferThreadFunc, "startPictureBufferThread");
    CLOGD("startPictureBufferThread created");

    m_frameFactoryStartThread = new mainCameraThread(this, &ExynosCamera::m_frameFactoryStartThreadFunc, "FrameFactoryStartThread");
    CLOGD("FrameFactoryStartThread created");

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    m_reverseProcessingBayerThread = new mainCameraThread(this, &ExynosCamera::m_reverseProcessingBayerThreadFunc, "reverseProcessingBayerThread");
    CLOGD("reverseProcessingBayerThread created");
#endif

#ifdef SUPPORT_HW_GDC
    m_gdcThread = new mainCameraThread(this, &ExynosCamera::m_gdcThreadFunc, "GDCThread");
    CLOGD("GDCThread created");
#endif

    m_monitorThread = new mainCameraThread(this, &ExynosCamera::m_monitorThreadFunc, "MonitorThread");
    CLOGD("MonitorThread created");

#ifdef BUFFER_DUMP
    m_dumpThread = new mainCameraThread(this, &ExynosCamera::m_dumpThreadFunc, "m_dumpThreadFunc");
    CLOGD("DumpThread created");
#endif

#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    m_setRemosaicBufferThread = new mainCameraThread(this, &ExynosCamera::m_setRemosaicBuffer, "SetRemosaicBufThread");
    CLOGD("SetRemosaicBufThread created");

    m_releaseRemosaicBufferThread = new mainCameraThread(this, &ExynosCamera::m_releaseRemosaicBuffer, "ReleaseRemosaicBufThread");
    CLOGD("ReleaseRemosaicBufThread created");
#endif

    m_vendorCreateThreads();

    m_flagThreadCreated = true;
}

ExynosCamera::~ExynosCamera()
{
    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, DESTRUCTOR_START, 0);

    this->destroyDevice();

    this->release();

#ifdef TIME_LOGGER_CLOSE_ENABLE
    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, DESTRUCTOR_END, 0);
    TIME_LOGGER_SAVE(m_cameraId);
#endif
}

void ExynosCamera::release()
{
    CLOGI("-IN-");

    m_vendorSpecificPreDestructor();
    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, PRE_DESTRUCTOR_END, 0);

    m_captureThreadStopAndInputQ();

    m_destroyCaptureStreamQ();

    if (m_shotDoneQ != NULL) {
        delete m_shotDoneQ;
        m_shotDoneQ = NULL;
    }

#ifdef USE_DUAL_CAMERA
    if (m_slaveShotDoneQ != NULL) {
        delete m_slaveShotDoneQ;
        m_slaveShotDoneQ = NULL;
    }

    if (m_dualStandbyTriggerQ != NULL) {
        delete m_dualStandbyTriggerQ;
        m_dualStandbyTriggerQ = NULL;
    }
#endif

    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        if (m_pipeFrameDoneQ[i] != NULL) {
            delete m_pipeFrameDoneQ[i];
            m_pipeFrameDoneQ[i] = NULL;
        }
    }

#if defined(USES_CAMERA_EXYNOS_LEC)
    if (m_pipeFrameLecDoneQ != NULL) {
        delete m_pipeFrameLecDoneQ;
        m_pipeFrameLecDoneQ = NULL;
    }
#endif

    if (m_frameFactoryQ != NULL) {
        delete m_frameFactoryQ;
        m_frameFactoryQ = NULL;
    }

    if (m_dualFrameFactoryQ != NULL) {
        delete m_dualFrameFactoryQ;
        m_dualFrameFactoryQ = NULL;
    }

#ifdef SUPPORT_HW_GDC
    if (m_gdcQ != NULL) {
        delete m_gdcQ;
        m_gdcQ = NULL;
    }
#endif
#ifdef BUFFER_DUMP
    if (m_dumpBufferQ != NULL) {
        delete m_dumpBufferQ;
        m_dumpBufferQ = NULL;
    }
#endif

    if (m_frameMgr != NULL) {
        //deleted by resoureManager
        m_frameMgr = NULL;
    }

    if (m_streamManager != NULL) {
        //delete by resourceManager
        m_streamManager = NULL;
    }

    if (m_requestMgr!= NULL) {
        //deleted by resourceManager;
        m_requestMgr = NULL;
    }

#ifdef USES_VPL_PRELOAD
    if (m_vplPreloadThread->isRunning() == true) {
        m_vplPreloadThread->join();
    }

    if (m_vplPreloadInitStatus) {
        if (m_vplUnload != nullptr) {
            (*m_vplUnload)();
            m_vplUnload = nullptr;
        }

        if (m_vplDl != nullptr) {
            dlclose(m_vplDl);
            m_vplDl = nullptr;
        }
        m_vplPreloadInitStatus = false;
    }
#endif

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_currentPreviewShot[i] != NULL) {
            delete m_currentPreviewShot[i];
            m_currentPreviewShot[i] = NULL;
        }

        if (m_currentInternalShot[i] != NULL) {
            delete m_currentInternalShot[i];
            m_currentInternalShot[i] = NULL;
        }

        if (m_currentCaptureShot[i] != NULL) {
            delete m_currentCaptureShot[i];
            m_currentCaptureShot[i] = NULL;
        }

        if (m_currentVisionShot[i] != NULL) {
            delete m_currentVisionShot[i];
            m_currentVisionShot[i] = NULL;
        }
    }

    if (m_ionClient >= 0) {
        close(m_ionClient);
        m_ionClient = -1;
    }

    if (m_gscWrapper != NULL) {
        m_gscWrapper->destroy();
        m_gscWrapper = nullptr;
    }

    if (isOfflineCaptureRunning() != true) {
        TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, BUFFER_SUPPLIER_DEINIT_JOIN_START, 0);
        if (m_deinitBufferSupplierThread != NULL) {
            m_deinitBufferSupplierThread->join();
        }
        m_deinitBufferSupplierThreadFunc(); /* try again for failure to run thread*/
        TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, BUFFER_SUPPLIER_DEINIT_JOIN_END, 0);
    }

    // TODO: clean up
    // m_resultBufferVectorSet
    // m_processList
    // m_postProcessList
    // m_pipeFrameDoneQ

    m_vendorSpecificDestructor();
    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, POST_DESTRUCTOR_END, 0);

#ifdef USES_SENSOR_GYRO_FACTORY_MODE
    if (m_sensorGyroTest != NULL) {
        if (m_sensorGyroTest->flagCreated() == true) {
            m_sensorGyroTest->destroy();
        }

        SAFE_DELETE(m_sensorGyroTest);
    }
#endif

#ifdef USES_SW_VDIS
    if (m_exCameraSolutionSWVdis != NULL) {
        delete m_exCameraSolutionSWVdis;
        m_exCameraSolutionSWVdis = NULL;
    }
#endif
    m_camIdMap.clear();
    CLOGI("-OUT-");
}

status_t ExynosCamera::initializeDevice(const camera3_callback_ops *callbackOps)
{
    status_t ret = NO_ERROR;
    CLOGD("");

    /* set callback ops */
    m_requestMgr->setCallbackOps(callbackOps);

    if (m_parameters[m_cameraId]->isReprocessing() == true) {
        ExynosCameraResourceManager::capture_property_t captureProperty;

        memset(&captureProperty, 0, sizeof(ExynosCameraResourceManager::capture_property_t));

        m_resourceManager->initCaptureProperty(&m_camIdInfo, &captureProperty);

        if (captureProperty.frameSelector[m_cameraId] == NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):captureProperty.frameSelector[%d] is NULL",
                                __FUNCTION__, __LINE__, m_cameraId);
        }

        for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
            if (m_captureSelector[m_camIdInfo.cameraId[i]] == NULL) {
                m_captureSelector[m_camIdInfo.cameraId[i]] = captureProperty.frameSelector[m_camIdInfo.cameraId[i]];
            } else {
                CLOGW("m_captureSelector[%d] is not NULL, But resource manager allocates it", m_camIdInfo.cameraId[i]);
            }
        }

        ret = m_initFrameHoldCount();
        if (ret != NO_ERROR) {
            CLOGE("init farme hold count is fail. ret(%d)", ret);
        }

        if (captureProperty.zslFrameSelector == NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):captureProperty.zslFrameSelector is NULL",
                                __FUNCTION__, __LINE__);
        }

        if (m_captureZslSelector == NULL) {
            m_captureZslSelector = captureProperty.zslFrameSelector;
            ret = m_captureZslSelector->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
            if (ret < 0) {
                CLOGE("m_captureZslSelector setFrameHoldCount(%d) is fail",
                     REPROCESSING_BAYER_HOLD_COUNT);
            }
        }
    }

    m_frameMgr->start();

    ret = m_transitState(EXYNOS_CAMERA_STATE_INITIALIZE);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState into INITIALIZE. ret %d", ret);
    }

    return ret;
}

status_t ExynosCamera::m_reInit(void)
{
    m_vendorReInit();

    return NO_ERROR;
}

status_t ExynosCamera::m_initFrameHoldCount(void)
{
    status_t ret = NO_ERROR;

    ret = m_captureSelector[m_cameraId]->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
    if (ret != NO_ERROR) {
        CLOGE("setFrameHoldCount is fail. ret = %d", ret);
        return ret;
    }

    ret = m_captureSelector[m_cameraId]->clearList();
    if (ret != NO_ERROR) {
        CLOGE("clearList is fail. ret = %d", ret);
        return ret;
    }

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) != DUAL_PREVIEW_MODE_OFF) {
        for (int slave_idx = 1; slave_idx < m_camIdInfo.numOfSensors; slave_idx++) {
            ret = m_captureSelector[m_cameraIds[slave_idx]]->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
            if (ret != NO_ERROR) {
                CLOGE("setFrameHoldCount is fail. ret = %d", ret);
                return ret;
            }

            ret = m_captureSelector[m_cameraIds[slave_idx]]->clearList();
            if (ret != NO_ERROR) {
                CLOGE("clearList is fail. ret = %d", ret);
                return ret;
            }
        }
    }
#endif

    return ret;
}

status_t ExynosCamera::construct_default_request_settings(camera_metadata_t **request, int type)
{
    CLOGD("Type %d", type);
    if ((type < 0) || (type >= CAMERA3_TEMPLATE_COUNT)) {
        CLOGE("Unknown request settings template: %d", type);
        return NO_INIT;
    }

    m_requestMgr->constructDefaultRequestSettings(type, request);

    CLOGI("out");
    return NO_ERROR;
}

void ExynosCamera::get_metadata_vendor_tag_ops(const camera3_device_t *, vendor_tag_query_ops_t *ops)
{
    if (ops == NULL) {
        CLOGE("ops is NULL");
        return;
    }
}

void ExynosCamera::dump(int fd)
{
    ExynosCameraFrameFactory *factory = NULL;

    CLOGI("");

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    if (factory == NULL) {
        CLOGE("frameFactory is NULL");
        return;
    }
    m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW]->dump();
    m_bufferSupplier->dump(fd);
}

int ExynosCamera::getCameraIdx(int32_t camId)
{
    map<int32_t, int32_t>::iterator it;

    it = m_camIdMap.find(camId);
    if (it == m_camIdMap.end()) {
        CLOGE("getCameraIdx is failed:  camId (%d) it (%d)", camId, it);
        return -1;
    }

   return it->second;
}

int ExynosCamera::getCameraId() const
{
    return m_cameraId;
}

int ExynosCamera::getServiceCameraId() const
{
    return m_camIdInfo.serviceCameraId;
}

int ExynosCamera::getCameraInfoIdx() const
{
    return m_camIdInfo.camInfoIndex;
}

#ifdef USE_DUAL_CAMERA
int ExynosCamera::getSubCameraId() const
{
    return m_cameraIds[1];
}
#endif

status_t ExynosCamera::setPIPMode(bool enabled, bool isSubCam)
{
    m_configurations->setMode(CONFIGURATION_PIP_MODE, enabled);
    if (isSubCam)
        m_configurations->setMode(CONFIGURATION_PIP_SUB_CAM_MODE, enabled);

    return NO_ERROR;
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCamera::setDualMode(bool enabled)
{
    m_configurations->setMode(CONFIGURATION_DUAL_MODE, enabled);

#ifdef USES_BOKEH_REFOCUS_CAPTURE
    if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) {
        m_configurations->setMode(CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE, enabled);
    }
#endif

#ifdef USES_DUAL_REAR_ALWAYS_FORCE_SWITCHING
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        && m_cameraId == CAMERA_SERVICE_ID_DUAL_REAR_ZOOM_MAIN) {
        m_configurations->setMode(CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE, true);
    } else {
        m_configurations->setMode(CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE, false);
    }
#endif

    return NO_ERROR;
}

bool ExynosCamera::getDualMode(void)
{
    return m_configurations->getMode(CONFIGURATION_DUAL_MODE);
}
#endif

bool ExynosCamera::isCameraRunning(void)
{
    exynos_camera_state_t state;

    state = m_getState();

    return (state == EXYNOS_CAMERA_STATE_RUN) ? true : false;
}


bool ExynosCamera::m_mainPreviewThreadFunc(void)
{
    status_t ret = NO_ERROR;

    if (m_getState() != EXYNOS_CAMERA_STATE_RUN) {
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        if (m_getState() != EXYNOS_CAMERA_STATE_SWITCHING_SENSOR) {
#endif
            CLOGI("Wait to run FrameFactory. state(%d)", m_getState());

            if (m_getState() == EXYNOS_CAMERA_STATE_ERROR) {
                return false;
            }

            usleep(1000);
            return true;
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        }
#endif
    }

    if (m_configurations->getMode(CONFIGURATION_VISION_MODE) == false) {
        ret = m_createPreviewFrameFunc(REQ_SYNC_WITH_3AA, true /* flagFinishFactoryStart */);
    } else {
        ret = m_createVisionFrameFunc(REQ_SYNC_WITH_3AA, true /* flagFinishFactoryStart */);
    }
    if (ret != NO_ERROR) {
        CLOGE("Failed to createPreviewFrameFunc. Shotdone");
    }

    return true;
}

bool ExynosCamera::m_mainCaptureThreadFunc(void)
{
    status_t ret = NO_ERROR;

    m_reprocessingFrameFactoryStartThread->join();

    if (m_getState() != EXYNOS_CAMERA_STATE_RUN) {
        CLOGI("Wait to run FrameFactory. state(%d)", m_getState());

        if (m_getState() == EXYNOS_CAMERA_STATE_ERROR) {
            return false;
        }

        usleep(10000);
        return true;
    }

    if (m_getSizeFromRequestList(&m_requestCaptureWaitingList, &m_requestCaptureWaitingLock) > 0) {
        ret = m_createCaptureFrameFunc();
        if (ret != NO_ERROR) {
            CLOGE("Failed to createCaptureFrameFunc. Shotdone");
        }

        return true;
    } else {
        return false;
    }
}

#ifdef USE_DUAL_CAMERA
#ifdef USE_DUAL_BAYER_SYNC
status_t ExynosCamera::m_pushBayerSyncFrame(ExynosCameraFrameSP_sptr_t bayerFrame)
{
    ExynosCameraFrameSP_sptr_t syncFrame = bayerFrame->getPairFrame();

    if (syncFrame == NULL) {
        CLOGE("syncFrame is NULL");
        return INVALID_OPERATION;
    }

    CFLOGD(syncFrame, "Push bayer sync frame");
    m_syncBayerFrameDoneQ->pushProcessQ(&syncFrame);

    return NO_ERROR;
}

ExynosCameraFrameSP_sptr_t ExynosCamera::m_waitAndPopBayerSyncFrame(frame_handle_components_t components,
                                                                                uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t bayerFrame;
    int waitCount = 0;
    const int timeOut = 20;
    do {
        ret = m_syncBayerFrameDoneQ->waitAndPopProcessQ(&bayerFrame);
        if (ret != NO_ERROR && waitCount++) {
            CLOGW("fail to waitAndPopProcessQ. ret(%d) waitCount(%d)", ret, waitCount);
        }
    } while (ret == TIMED_OUT && waitCount < timeOut);

    if (ret != NO_ERROR) {
        CLOGE("fail to waitAndPopProcessQ. ret(%d) waitCount(%d)", ret, waitCount);
        return components.captureSelector->selectCaptureFrames(1, frameCount, timeOut);
    }

    CFLOGD(bayerFrame, "get bayer sync frame");

    return bayerFrame;
}
#endif

bool ExynosCamera::m_slaveMainThreadFunc(void)
{
    ExynosCameraFrameSP_sptr_t frame = NULL;
    bool loop = false;
    status_t ret = NO_ERROR;
    enum DUAL_OPERATION_MODE dualOperationMode = DUAL_OPERATION_MODE_NONE;
    enum DUAL_OPERATION_MODE earlyDualOperationMode = DUAL_OPERATION_MODE_NONE;
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
    enum DUAL_OPERATION_SENSORS dualOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    enum DUAL_OPERATION_SENSORS earlyDualOperationSensor = DUAL_OPERATION_SENSOR_MAX;

    int32_t masterCamId, slaveCamId;
    int32_t earlyMasterCamId, earlySlaveCamId;
    int32_t camIdIndex;

    frame_type_t frameType;
    ExynosCameraFrameFactory *factory = NULL;
    factory_handler_t frameCreateHandler;
    ExynosCameraRequestSP_sprt_t request = NULL;
    List<ExynosCameraRequestSP_sprt_t>::iterator r;

    if (m_getState() != EXYNOS_CAMERA_STATE_RUN) {
        CLOGI("Wait to run FrameFactory. state(%d), m_earlyDualOperationMode(%d) m_earlyOperationSensor(%d)",
            m_getState(), m_earlyDualOperationMode, m_earlyOperationSensor);

        if (m_getState() == EXYNOS_CAMERA_STATE_ERROR) {
            return false;
        }

        usleep(1000);
        return true;
    }

    /* 1. Wait the shot done with the latest frame */
    ret = m_slaveShotDoneQ->waitAndPopProcessQ(&frame);
    if (ret < 0) {
        if (ret == TIMED_OUT)
            CLOGW("wait timeout");
        else
            CLOGE("wait and pop fail, ret(%d)", ret);
        loop = true;
        goto p_exit;
    } else {
        Mutex::Autolock lock(m_dualOperationModeLock);
        /*
         * Is it proper to take DualOperationMode from m_configurations?
         * It is not always sync to the request.
         * TODO: check if any better way.
         */

        earlyDualOperationMode = m_earlyDualOperationMode;
        earlyDualOperationSensor = m_earlyOperationSensor;

        switch (frame->getFrameType()) {
        case FRAME_TYPE_TRANSITION_SLAVE:
            /*
             * The dualOperationMode might have not been changed by my standby.
             * So I want to refer to m_earlyDualOperationMode in advance.
             */
            dualOperationMode = m_earlyDualOperationMode;
            dualOperationSensor = m_earlyOperationSensor;
            break;
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
            /*
             * Refer to current dualOperationMode
             */
            dualOperationMode = m_configurations->getDualOperationMode();
            dualOperationSensor = m_configurations->getDualOperationSensor(false);
            break;
        case FRAME_TYPE_PREVIEW_SLAVE:
        case FRAME_TYPE_INTERNAL_SLAVE:
            /*
             * Refer to current dualOperationMode
             * But, in this generation of frame,
             * DUAL_SLAVE frame may be created in m_createPreviewFrameFunc()
             */
            dualOperationMode = m_configurations->getDualOperationMode();
            dualOperationSensor = m_configurations->getDualOperationSensor(false);
            break;
        default:
            CLOG_ASSERT("invalid frameType(%d)", frame->getFrameType());
            break;
        }
    }

    m_getCameraIdFromOperationSensor(m_earlyOperationSensor, &earlyMasterCamId, &earlySlaveCamId);
    m_getCameraIdFromOperationSensor(dualOperationSensor, &masterCamId, &slaveCamId);

    CLOGV("[F%d, T%d] dualOperationMode: %d / %d dualOperationSensor: %d / %d"
          "earlyMasterCamId = %d earlySlaveCamId = %d masterCamId = %d slaveCamId = %d",
            frame->getFrameCount(), frame->getFrameType(),
            m_earlyDualOperationMode, m_configurations->getDualOperationMode(),
            m_earlyOperationSensor, m_configurations->getDualOperationSensor(false),
            earlyMasterCamId, earlySlaveCamId, masterCamId, slaveCamId);

    switch (dualOperationMode) {
    case DUAL_OPERATION_MODE_MASTER:
        if (earlyDualOperationMode == DUAL_OPERATION_MODE_SYNC &&
                m_parameters[earlySlaveCamId]->getStandbyState() == DUAL_STANDBY_STATE_OFF) {
            /* create slave frame for sync mode */
            camIdIndex = getCameraIdx(earlySlaveCamId);
            if (camIdIndex < 0) {
                CLOGE("Wrong earlySlaveCamId = %d camIdIndex = %d", earlySlaveCamId, camIdIndex);
                camIdIndex = 1;
            }
            frameType = FRAME_TYPE_PREVIEW_DUAL_SLAVE;
            factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + camIdIndex];
            frameCreateHandler = factory->getFrameCreateHandler();

            /* get the latest request */
            m_latestRequestListLock.lock();
            if (m_essentialRequestList.size() > 0) {
                r = m_essentialRequestList.begin();
                request = *r;
                m_essentialRequestList.erase(r);
            } else {
                if (m_latestRequestList.size() > 0) {
                    r = m_latestRequestList.begin();
                    request = *r;
                }
            }
            m_latestRequestListLock.unlock();

            if (request != NULL) {
                FrameFactoryList previewFactoryAddrList;

                previewFactoryAddrList.clear();
                request->getFactoryAddrList(FRAME_FACTORY_TYPE_CAPTURE_PREVIEW, &previewFactoryAddrList);

                if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION && previewFactoryAddrList.empty() == false
                    && camIdIndex == SUB_CAM) {
                    (this->*frameCreateHandler)(request, factory, frameType);
                } else {
                    m_createInternalFrameFunc(NULL, true, REQ_SYNC_NONE, FRAME_TYPE_INTERNAL_SLAVE);
                }
            } else {
                m_createInternalFrameFunc(NULL, true, REQ_SYNC_NONE, FRAME_TYPE_INTERNAL_SLAVE);
            }
        }
        loop = true;
        goto p_exit;
    case DUAL_OPERATION_MODE_SLAVE:
        ret = m_createPreviewFrameFunc(REQ_SYNC_NONE, true);
        loop = true;
        goto p_exit;
    case DUAL_OPERATION_MODE_SYNC:
        /* create slave frame for sync mode */
        frameType = FRAME_TYPE_PREVIEW_DUAL_SLAVE;
        camIdIndex = getCameraIdx(slaveCamId);
        if (camIdIndex < 0) {
            CLOGE("Wrong slaveCamId = %d camIdIndex = %d", slaveCamId, camIdIndex);
            camIdIndex = 1;
        }
        factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + camIdIndex];
        frameCreateHandler = factory->getFrameCreateHandler();

        /* get the latest request */
        m_latestRequestListLock.lock();
        if (m_essentialRequestList.size() > 0) {
            r = m_essentialRequestList.begin();
            request = *r;
            m_essentialRequestList.erase(r);
        } else {
            if (m_latestRequestList.size() > 0) {
                r = m_latestRequestList.begin();
                request = *r;
            }
        }
        m_latestRequestListLock.unlock();

        if (request != NULL) {
            FrameFactoryList previewFactoryAddrList;

            previewFactoryAddrList.clear();
            request->getFactoryAddrList(FRAME_FACTORY_TYPE_CAPTURE_PREVIEW, &previewFactoryAddrList);
            if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION && previewFactoryAddrList.empty() == false
                    && (camIdIndex == SUB_CAM || camIdIndex == SUB_CAM2)) {
                (this->*frameCreateHandler)(request, factory, frameType);
            } else {
                m_createInternalFrameFunc(NULL, true, REQ_SYNC_NONE, FRAME_TYPE_INTERNAL_SLAVE);
            }
        } else {
            m_createInternalFrameFunc(NULL, true, REQ_SYNC_NONE, FRAME_TYPE_INTERNAL_SLAVE);
        }

        loop = true;
        goto p_exit;
    default:
        goto p_exit;
    }

p_exit:

    return loop;
}
#endif

#ifdef SUPPORT_SENSOR_MODE_CHANGE
status_t ExynosCamera::m_createSensorTransitionFrameFunc(bool toggle)
{
    bool flagFinishFactoryStart = false;
    enum Request_Sync_Type syncType = REQ_SYNC_NONE;
    frame_type_t internalFrameType;

    if (toggle == true)
        internalFrameType = FRAME_TYPE_INTERNAL_SENSOR_TRANSITION;
    else
        internalFrameType = FRAME_TYPE_INTERNAL;

    CLOGD("Create sensor transition frame");
    return m_createInternalFrameFunc(NULL, flagFinishFactoryStart, syncType, internalFrameType);
}
#endif

status_t ExynosCamera::m_createPreviewFrameFunc(enum Request_Sync_Type syncType, __unused bool flagFinishFactoryStart)
{
    status_t ret = NO_ERROR;
    status_t waitRet = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    struct camera2_shot_ext *service_shot_ext = NULL;
    FrameFactoryList previewFactoryAddrList;
    ExynosCameraFrameFactory *factory = NULL;
    FrameFactoryListIterator factorylistIter;
    factory_handler_t frameCreateHandler;
    List<ExynosCameraRequestSP_sprt_t>::iterator r;
    frame_type_t frameType = FRAME_TYPE_PREVIEW;
    uint32_t waitingListSize;
    uint32_t requiredRequestCount = -1;
    bool isNeedRequestFrame = false;
    ExynosCameraFrameSP_sptr_t reqSyncFrame = NULL;
#ifdef USE_DUAL_CAMERA
    ExynosCameraFrameFactory *subFactory = NULL;
    frame_type_t subFrameType = FRAME_TYPE_PREVIEW;
    enum DUAL_OPERATION_MODE dualOperationMode = m_configurations->getDualOperationMode();
    int32_t dualOperationModeLockCount = 0;
    bool isDualMode = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    int32_t slaveCamIdx = 1;
    int32_t slaveCamIdxReprocess = 1;
#endif

#ifdef DEBUG_STREAM_CONFIGURATIONS
    CLOGD("DEBUG_STREAM_CONFIGURATIONS::[R] generate request frame");
#endif

    waitRet = m_waitShotDone(syncType, reqSyncFrame);
    if (waitRet < 0) {
        /*
         * ShotDone can be timedout when the sensor in STANDBY_ON_MODE (DUAL_camera case).
         * Internal frame should be created in such scenario.
         * If Internal frame is created per every timeout case, there will be
         * so many pending frames for the STNDBY_ON sensor.
         */

        return NO_ERROR;
    }

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    if (m_getState() == EXYNOS_CAMERA_STATE_SWITCHING_SENSOR) {
        CLOGW("sensor mode is changing, so ignore creating new preview frame");
        return NO_ERROR;
    }

    if ( (syncType == REQ_SYNC_WITH_3AA) && (reqSyncFrame != NULL)
        && ((reqSyncFrame->getFrameType() & FRAME_TYPE_INTERNAL_SENSOR_TRANSITION) == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION)) {
        switch(reqSyncFrame->getFrameType()) {
        case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_ING:
            CLOGD("[SENSOR TRANSITION] FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_ING");
            return ret;
            break;
        case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_START:
            CLOGD("[SENSOR TRANSITION] switchSensorMode FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_START");
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
            if (m_configurations->getMode(CONFIGURATION_DYNAMIC_REMOSAIC_BUFFER_ALLOC_MODE) !=
                    DYNAMIC_REMOSAIC_BUFFER_ALLOC_OFF) {
                m_releaseRemosaicBufferThread->requestExitAndWait();
                m_setRemosaicBufferThread->run();
            }
#endif
            m_switchSensorMode(NULL, true);

            CLOGD("[SENSOR TRANSITION] m_shotDoneQ(%d)", m_shotDoneQ->getSizeOfProcessQ());
            m_shotDoneQ->release();

            if (m_frameFactory[FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING]->isRunning() == false) {
                CLOGD("Remosaic Reprocessing FrameFactory is run");
                ret = m_startReprocessingFrameFactory(m_frameFactory[FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING]);
                if (ret != NO_ERROR) {
                    CLOGE("startReprocessingFrameFactory is fail, ret = %d", ret);
                    return ret;
                }
            } else {
                CLOGW("Remosaic Reprocessing FrameFactory is already running");
            }

            return NO_ERROR;
            break;
        case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_STOP:
            CLOGD("[SENSOR TRANSITION] switchSensorMode FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_STOP");
            m_switchSensorMode(NULL, false);

            CLOGD("[SENSOR TRANSITION] m_shotDoneQ(%d)", m_shotDoneQ->getSizeOfProcessQ());
            m_shotDoneQ->release();

            return NO_ERROR;
            break;
        default:
            CLOGD("[T%d] unknown internal sensor transit frame type", reqSyncFrame->getFrameType());
            break;
        }
    } else
#endif //SUPPORT_SENSOR_MODE_CHANGE

    /* 1. Update the current shot */
    {
        Mutex::Autolock l(m_requestPreviewWaitingLock);

        enum pipeline controlPipeId = (enum pipeline) m_parameters[m_cameraId]->getPerFrameControlPipe();
        waitingListSize = m_requestPreviewWaitingList.size();
        if (m_parameters[m_cameraId]->useServiceBatchMode() == true) {
            requiredRequestCount = 1;
        } else {
            requiredRequestCount = m_parameters[m_cameraId]->getBatchSize(controlPipeId);
        }

        if (waitingListSize < requiredRequestCount) {
            isNeedRequestFrame = false;
        } else {
            isNeedRequestFrame = true;

            r = m_requestPreviewWaitingList.begin();
            request = *r;

            /* 2. Update the entire shot_ext structure */
            service_shot_ext = request->getServiceShot();
            if (service_shot_ext == NULL) {
                CLOGE("[R%d] Get service shot is failed", request->getKey());
            } else {
                m_latestRequestListLock.lock();
                m_latestRequestList.clear();
                m_latestRequestList.push_front(request);
                m_latestRequestListLock.unlock();

                *m_currentPreviewShot[m_cameraId] = *service_shot_ext;
            }

            /* 3. Initial (SENSOR_REQUEST_DELAY + 1) frames must be internal frame to
                * secure frame margin for sensor control.
                * If sensor control delay is 0, every initial frames must be request frame.
                */
            if (m_parameters[m_cameraId]->getSensorControlDelay() > 0
                && m_internalFrameCount < (m_parameters[m_cameraId]->getSensorControlDelay() + 2)) {
                isNeedRequestFrame = false;
                request = NULL;
                CLOGD("[F%d]Create Initial Frame", m_internalFrameCount);

#ifdef SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
                if (flagFinishFactoryStart == false
                    && m_currentPreviewShot[m_cameraId]->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE_EXPOSURE_DYNAMIC_SHOT) {
                    /* Do not use the LEC Parameter for internal frame */
                    int vendorModeValue = m_configurations->getModeValue(CONFIGURATION_SESSION_MODE_VALUE);

                    switch (vendorModeValue) {
                    case EXYNOS_SESSION_MODE_PRO:
                        m_currentPreviewShot[m_cameraId]->shot.ctl.aa.captureIntent = AA_CAPTURE_INTENT_PREVIEW;
                        m_currentPreviewShot[m_cameraId]->shot.ctl.aa.vendor_captureExposureTime = 0;
                        m_currentPreviewShot[m_cameraId]->shot.ctl.aa.vendor_captureCount = 0;

                        CLOGD("Initialized captureIntent(%d), ExposureTime(%u), captureCount(%d)",
                                m_currentPreviewShot[m_cameraId]->shot.ctl.aa.captureIntent,
                                m_currentPreviewShot[m_cameraId]->shot.ctl.aa.vendor_captureExposureTime,
                                m_currentPreviewShot[m_cameraId]->shot.ctl.aa.vendor_captureCount);
                        break;
                    default:
                        break;
                    }
                } else {
                    CLOGW("flagFinishFactoryStart(%d), captureIntent(%d)",
                            flagFinishFactoryStart, m_currentPreviewShot[m_cameraId]->shot.ctl.aa.captureIntent);
                }
#endif
            } else {
#ifdef USE_DUAL_CAMERA
                if (isDualMode == true) {
                    ret = m_checkDualOperationMode(request, true, false, flagFinishFactoryStart);
                    if (ret != NO_ERROR) {
                        CLOGE("m_checkDualOperationMode fail! flagFinishFactoryStart(%d) ret(%d)", flagFinishFactoryStart, ret);
                    }

                    dualOperationMode = m_configurations->getDualOperationMode();
                    dualOperationModeLockCount = m_configurations->getDualOperationModeLockCount();
                    slaveCamIdx = m_getCurrentCamIdx(false, SUB_CAM);

                    switch (dualOperationMode) {
                    case DUAL_OPERATION_MODE_SLAVE:
                        /* source from master */
                        if (syncType == REQ_SYNC_WITH_3AA) {
                            if (reqSyncFrame != NULL) {
                                /*
                                 * Early SensorStandby-off case in MASTER only.
                                 * The request to trigger Early SensorStandby-off has not been yet arrived here.
                                 * So we generate Transition frame to keep the count of queued buffers.
                                 * We don't care Early SensorStandby-off in SLAVE cause of independent
                                 * slaveMainThread.
                                 */
                                if ((m_earlyTriggerRequestKey > request->getKey() &&
                                     m_parameters[m_cameraId]->getStandbyState() == DUAL_STANDBY_STATE_OFF) ||
                                        dualOperationModeLockCount > 0) {
                                    CLOGI("generate transition frame more due to early sensorStandby off [%d, %d] lockCnt(%d)",
                                            m_earlyTriggerRequestKey,
                                            request->getKey(),
                                            dualOperationModeLockCount);
                                    ret = m_createInternalFrameFunc(NULL, flagFinishFactoryStart, REQ_SYNC_NONE, FRAME_TYPE_TRANSITION);
                                    if (ret != NO_ERROR) {
                                        CLOGE("Failed to createInternalFrameFunc(%d)", ret);
                                    }
                                } else {
                                    CLOGW("[R%d F%d T%d]skip this request for slave",
                                            request->getKey(),
                                            reqSyncFrame->getFrameCount(),
                                            reqSyncFrame->getFrameType());
                                }
                            } else {
                                CLOGW("skip this request for slave");
                            }
                            return ret;
                        }
                        break;
                    case DUAL_OPERATION_MODE_SYNC:
                    case DUAL_OPERATION_MODE_MASTER:
                        /* source from slave */
                        if (syncType == REQ_SYNC_NONE && flagFinishFactoryStart == true) {
                            if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
                                /* create slave frame for sync mode */
                                frameType = FRAME_TYPE_PREVIEW_DUAL_SLAVE;
                                factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + slaveCamIdx];
                                frameCreateHandler = factory->getFrameCreateHandler();

                                (this->*frameCreateHandler)(request, factory, frameType);
                            } else {
                                m_createInternalFrameFunc(NULL, flagFinishFactoryStart, REQ_SYNC_NONE, FRAME_TYPE_INTERNAL_SLAVE);
                            }

                            return ret;
                        }
                        break;
                    default:
                        break;
                    }
                }
#endif

#ifdef USE_DUAL_CAMERA
                /* save the latest request */
                m_latestRequestListLock.lock();
                if (isDualMode == true
                    && m_isRequestEssential(request) == true) {
                    if (m_essentialRequestList.size() > 10) {
                        CLOGW("Essential request list(%zu)", m_essentialRequestList.size());
                    }
                    m_essentialRequestList.push_back(request);
                }
                m_latestRequestListLock.unlock();
#endif
                m_requestPreviewWaitingList.erase(r);
            }
        }
    }

    CLOGV("Create New Frame %d needRequestFrame %d waitingSize %d",
            m_internalFrameCount, isNeedRequestFrame, waitingListSize);

    /* 4. Select the frame creation logic between request frame and internal frame */
    if (isNeedRequestFrame == true) {
        previewFactoryAddrList.clear();
        request->getFactoryAddrList(FRAME_FACTORY_TYPE_CAPTURE_PREVIEW, &previewFactoryAddrList);
        m_configurations->updateMetaParameter(request->getMetaParameters());

        /* Call the frame create handler for each frame factory */
        /* Frame creation handler calling sequence is ordered in accordance with
           its frame factory type, because there are dependencies between frame
           processing routine.
         */

        if (previewFactoryAddrList.empty() == false) {
            ExynosCameraRequestSP_sprt_t serviceRequest = NULL;
            m_popServiceRequest(serviceRequest);
            serviceRequest = NULL;

            for (factorylistIter = previewFactoryAddrList.begin(); factorylistIter != previewFactoryAddrList.end(); ) {
                factory = *factorylistIter;
                CLOGV("frameFactory (%p)", factory);

                switch(factory->getFactoryType()) {
                    case FRAME_FACTORY_TYPE_CAPTURE_PREVIEW:
                        frameType = FRAME_TYPE_PREVIEW;
                        break;
                    case FRAME_FACTORY_TYPE_REPROCESSING:
                        frameType = FRAME_TYPE_REPROCESSING;
                        break;
                    case FRAME_FACTORY_TYPE_VISION:
                        frameType = FRAME_TYPE_VISION;
                        break;
                    default:
                        CLOGE("[R%d] Factory type is not available", request->getKey());
                        break;
                }

#ifdef USE_DUAL_CAMERA
                if (isDualMode == true) {
                    switch (factory->getFactoryType()) {
                    case FRAME_FACTORY_TYPE_CAPTURE_PREVIEW:
                        if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
                            frameType = FRAME_TYPE_PREVIEW_DUAL_MASTER;
                        } else if (dualOperationMode == DUAL_OPERATION_MODE_SLAVE) {
                            factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + slaveCamIdx];
                            frameType = FRAME_TYPE_PREVIEW_SLAVE;
                        }
                        break;
                    case FRAME_FACTORY_TYPE_REPROCESSING:
                        slaveCamIdxReprocess = m_getCurrentCamIdx(true, SUB_CAM);

                        if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
                            subFrameType = FRAME_TYPE_REPROCESSING_DUAL_SLAVE;
                            subFactory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING + slaveCamIdxReprocess];
                            frameCreateHandler = subFactory->getFrameCreateHandler();
                            (this->*frameCreateHandler)(request, subFactory, subFrameType);

                            frameType = FRAME_TYPE_REPROCESSING_DUAL_MASTER;
                        } else if (dualOperationMode == DUAL_OPERATION_MODE_SLAVE) {
                            factory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING + slaveCamIdxReprocess];
                            frameType = FRAME_TYPE_REPROCESSING_SLAVE;
                        }
                        break;
                    default:
                        CLOGE("[R%d] Factory type is not available", request->getKey());
                        break;
                    }
                }
#endif
                frameCreateHandler = factory->getFrameCreateHandler();
                (this->*frameCreateHandler)(request, factory, frameType);

                factorylistIter++;
            }
        } else {
            frame_type_t internalFrameType = FRAME_TYPE_INTERNAL;
#ifdef USE_DUAL_CAMERA
            if (isDualMode == true && dualOperationMode == DUAL_OPERATION_MODE_SLAVE &&
                    syncType == REQ_SYNC_NONE) {
                internalFrameType = FRAME_TYPE_INTERNAL_SLAVE;
            }
#endif
            m_createInternalFrameFunc(request, flagFinishFactoryStart, syncType, internalFrameType);
        }
    } else {
        frame_type_t internalFrameType = FRAME_TYPE_INTERNAL;
#ifdef USE_DUAL_CAMERA
        if (isDualMode == true && dualOperationMode == DUAL_OPERATION_MODE_SLAVE &&
                syncType == REQ_SYNC_NONE) {
            internalFrameType = FRAME_TYPE_INTERNAL_SLAVE;
        }
#endif
        m_createInternalFrameFunc(NULL, flagFinishFactoryStart, syncType, internalFrameType);
    }

    previewFactoryAddrList.clear();

    return ret;
}

status_t ExynosCamera::m_createVisionFrameFunc(enum Request_Sync_Type syncType, __unused bool flagFinishFactoryStart)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    struct camera2_shot_ext *service_shot_ext = NULL;
    FrameFactoryList visionFactoryAddrList;
    ExynosCameraFrameFactory *factory = NULL;
    FrameFactoryListIterator factorylistIter;
    factory_handler_t frameCreateHandler;
    List<ExynosCameraRequestSP_sprt_t>::iterator r;
    frame_type_t frameType = FRAME_TYPE_VISION;
    uint32_t watingListSize = 0;
    bool isNeedRequestFrame = false;
    ExynosCameraFrameSP_sptr_t reqSyncFrame = NULL;

#ifdef DEBUG_STREAM_CONFIGURATIONS
    CLOGD("DEBUG_STREAM_CONFIGURATIONS::[R%d] generate request frame", request->getKey());
#endif

    m_waitShotDone(syncType, reqSyncFrame);

    /* 1. Update the current shot */
    {
        Mutex::Autolock l(m_requestPreviewWaitingLock);

        watingListSize = m_requestPreviewWaitingList.size();
        if (m_requestPreviewWaitingList.empty() == true) {
            isNeedRequestFrame = false;
        } else {
            isNeedRequestFrame = true;

            r = m_requestPreviewWaitingList.begin();
            request = *r;

            /* Update the entire shot_ext structure */
            service_shot_ext = request->getServiceShot();
            if (service_shot_ext == NULL) {
                CLOGE("[R%d] Get service shot is failed", request->getKey());
            } else {
                m_latestRequestListLock.lock();
                m_latestRequestList.clear();
                m_latestRequestList.push_front(request);
                m_latestRequestListLock.unlock();
            }

            /* 2. Initial (SENSOR_CONTROL_DELAY + 1) frames must be internal frame to
             * secure frame margin for sensor control.
             * If sensor control delay is 0, every initial frames must be request frame.
             */
            if (m_parameters[m_cameraId]->getSensorControlDelay() > 0
                && m_internalFrameCount < (m_parameters[m_cameraId]->getSensorControlDelay() + 2)) {
                isNeedRequestFrame = false;
                request = NULL;
            } else {
                m_requestPreviewWaitingList.erase(r);
            }
        }
    }

    CLOGV("Create New Frame %d needRequestFrame %d waitingSize %d",
            m_internalFrameCount, isNeedRequestFrame, watingListSize);

    /* 3. Select the frame creation logic between request frame and internal frame */
    if (isNeedRequestFrame == true) {
        visionFactoryAddrList.clear();
        request->getFactoryAddrList(FRAME_FACTORY_TYPE_VISION, &visionFactoryAddrList);

        /* Call the frame create handler fro each frame factory */
        /* Frame createion handler calling sequence is ordered in accordance with
           its frame factory type, because there are dependencies between frame
           processing routine.
         */

        if (visionFactoryAddrList.empty() == false) {
            ExynosCameraRequestSP_sprt_t serviceRequest = NULL;
            m_popServiceRequest(serviceRequest);
            serviceRequest = NULL;

            for (factorylistIter = visionFactoryAddrList.begin(); factorylistIter != visionFactoryAddrList.end(); ) {
                factory = *factorylistIter;
                CLOGV("frameFactory (%p)", factory);

                switch(factory->getFactoryType()) {
                    case FRAME_FACTORY_TYPE_CAPTURE_PREVIEW:
                        frameType = FRAME_TYPE_PREVIEW;
                        break;
                    case FRAME_FACTORY_TYPE_REPROCESSING:
                        frameType = FRAME_TYPE_REPROCESSING;
                        break;
                    case FRAME_FACTORY_TYPE_VISION:
                        frameType = FRAME_TYPE_VISION;
                        break;
                    default:
                        CLOGE("[R%d] Factory type is not available", request->getKey());
                        break;
                }
                frameCreateHandler = factory->getFrameCreateHandler();
                (this->*frameCreateHandler)(request, factory, frameType);

                factorylistIter++;
            }
        } else {
            m_createInternalFrameFunc(request, flagFinishFactoryStart, syncType);
        }
    } else {
        m_createInternalFrameFunc(NULL, flagFinishFactoryStart, syncType);
    }

    visionFactoryAddrList.clear();

    return ret;
}

status_t ExynosCamera::m_waitShotDone(enum Request_Sync_Type syncType, ExynosCameraFrameSP_dptr_t reqSyncFrame)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t frame = NULL;

    if (syncType == REQ_SYNC_WITH_3AA && m_shotDoneQ != NULL) {
        /* 1. Wait the shot done with the latest framecount */
        ret = m_shotDoneQ->waitAndPopProcessQ(&frame);
        if (ret < 0) {
            if (ret == TIMED_OUT)
                CLOGW("wait timeout");
            else
                CLOGE("wait and pop fail, ret(%d)", ret);
            return ret;
        }
    }

    reqSyncFrame = frame;

    return ret;
}

status_t ExynosCamera::m_sendJpegStreamResult(ExynosCameraRequestSP_sprt_t request,
                                               ExynosCameraBuffer *buffer, int size)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t *streamBuffer = NULL;
    ResultRequest resultRequest = NULL;
    camera3_capture_result_t *requestResult = NULL;
    int streamId = HAL_STREAM_ID_JPEG;
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        for (uint32_t index = 0; index < request->getNumOfOutputBuffer(); index++) {
            const camera3_stream_buffer_t *bufferList = request->getOutputBuffers();
            const camera3_stream_buffer_t *streamBuffer = &(bufferList[index]);
            int id = request->getStreamIdwithBufferIdx(index);

            switch (id % HAL_STREAM_ID_MAX) {
                case HAL_STREAM_ID_JPEG:
                    streamId = id;
                    break;
                default:
                    break;
            }
        }
    }
#endif

    ret = m_streamManager->getStream(streamId, &stream);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getStream from StreamMgr. streamId HAL_STREAM_ID_JPEG");
        return ret;
    }

    if (stream == NULL) {
        CLOGE("stream is NULL");
        return INVALID_OPERATION;
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(),
                                        EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. streamId HAL_STREAM_ID_JPEG",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    requestResult = resultRequest->getCaptureResult();
    if (requestResult == NULL) {
        CLOGE("[R%d F%d] getCaptureResult fail. streamId HAL_STREAM_ID_JPEG",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    streamBuffer = resultRequest->getStreamBuffer();
    if (streamBuffer == NULL) {
        CLOGE("[R%d F%d] getStreamBuffer fail. streamId HAL_STREAM_ID_JPEG",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = stream->getStream(&(streamBuffer->stream));
    if (ret != NO_ERROR) {
        CLOGE("Failed to getStream from ExynosCameraStream. streamId HAL_STREAM_ID_JPEG");
        return ret;
    }

    streamBuffer->buffer = buffer->handle[0];

    ret = m_checkStreamBufferStatus(request, stream, &streamBuffer->status);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d S%d B%d]Failed to checkStreamBufferStatus.",
            request->getKey(), request->getFrameCount(),
            HAL_STREAM_ID_JPEG, buffer->index);
        return ret;
    }

    streamBuffer->acquire_fence = -1;
    streamBuffer->release_fence = -1;

    camera3_jpeg_blob_t jpeg_bolb;
    jpeg_bolb.jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
    jpeg_bolb.jpeg_size = size;
    int32_t jpegBufferSize = ExynosGraphicBufferMeta::get_width(*streamBuffer->buffer);
    memcpy(buffer->addr[0] + jpegBufferSize - sizeof(camera3_jpeg_blob_t), &jpeg_bolb, sizeof(camera3_jpeg_blob_t));
    //int32_t jpegBufferSize = ((private_handle_t *)(*(streamBuffer->buffer)))->width;
    //memcpy(buffer->addr[0] + jpegBufferSize - sizeof(camera3_jpeg_blob_t), &jpeg_bolb, sizeof(camera3_jpeg_blob_t));

    /* update jpeg size */
    request->setRequestLock();
    CameraMetadata *setting = request->getServiceMeta();
    int32_t jpegsize = size;
    ret = setting->update(ANDROID_JPEG_SIZE, &jpegsize, 1);
    if (ret < 0) {
        CLOGE("ANDROID_JPEG_SIZE update failed(%d)", ret);
    }
    request->setRequestUnlock();

    CLOGD("Set JPEG result Done. streamId %d, frameCount %d", streamId, request->getFrameCount());

    /* construct result for service */
    requestResult->frame_number = request->getKey();
    requestResult->result = NULL;
    requestResult->num_output_buffers = 1;
    requestResult->output_buffers = streamBuffer;
    requestResult->input_buffer = request->getInputBuffer();
    requestResult->partial_result = 0;

    CLOGV("frame number(%d), #out(%d)",
            requestResult->frame_number, requestResult->num_output_buffers);

    requestMgr->pushResultRequest(resultRequest);

    ret = m_bufferSupplier->putBuffer(*buffer);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d B%d]Failed to putBuffer. ret %d",
                request->getKey(), request->getFrameCount(), buffer->index, ret);
    }

    return ret;
}

status_t ExynosCamera::m_sendRawStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t *streamBuffer = NULL;
    ResultRequest resultRequest = NULL;
    camera3_capture_result_t *requestResult = NULL;

    /* 1. Get stream object for RAW */
    ret = m_streamManager->getStream(HAL_STREAM_ID_RAW, &stream);
    if (ret < 0) {
        CLOGE("getStream is failed, from streammanager. Id error:HAL_STREAM_ID_RAW");
        return ret;
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(),
            EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. streamId HAL_STREAM_ID_RAW",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    requestResult = resultRequest->getCaptureResult();
    if (requestResult == NULL) {
        CLOGE("[R%d F%d] getCaptureResult fail. streamId HAL_STREAM_ID_RAW",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    streamBuffer = resultRequest->getStreamBuffer();
    if (streamBuffer == NULL) {
        CLOGE("[R%d F%d] getStreamBuffer fail. streamId HAL_STREAM_ID_RAW",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    /* 2. Get camera3_stream structure from stream object */
    ret = stream->getStream(&(streamBuffer->stream));
    if (ret < 0) {
        CLOGE("getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_RAW");
        return ret;
    }

    /* 3. Get the service buffer handle */
    streamBuffer->buffer = buffer->handle[0];

    /* 4. Update the remained buffer info */
    ret = m_checkStreamBufferStatus(request, stream, &streamBuffer->status);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d S%d B%d]Failed to checkStreamBufferStatus.",
            request->getKey(), request->getFrameCount(),
            HAL_STREAM_ID_RAW, buffer->index);
        return ret;
    }

    streamBuffer->acquire_fence = -1;
    streamBuffer->release_fence = -1;

    /* 5. Create new result for RAW buffer */
    requestResult->frame_number = request->getKey();
    requestResult->result = NULL;
    requestResult->num_output_buffers = 1;
    requestResult->output_buffers = streamBuffer;
    requestResult->input_buffer = NULL;
    requestResult->partial_result = 0;

    CLOGV("frame number(%d), #out(%d)", requestResult->frame_number, requestResult->num_output_buffers);

    /* 6. Request to callback the result to request manager */
    requestMgr->pushResultRequest(resultRequest);

    ret = m_bufferSupplier->putBuffer(*buffer);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d B%d]Failed to putBuffer. ret %d",
                request->getKey(), request->getFrameCount(), buffer->index, ret);
    }

    CLOGV("request->frame_number(%d), request->getNumOfOutputBuffer(%d)"
            " request->getCompleteBufferCount(%d) frame->getFrameCapture(%d)",
            request->getKey(),
            request->getNumOfOutputBuffer(),
            request->getCompleteBufferCount(),
            request->getFrameCount());

    CLOGV("streamBuffer info: stream (%p), handle(%p)",
            streamBuffer->stream, streamBuffer->buffer);

    return ret;
}

status_t ExynosCamera::m_sendZslStreamResult(ExynosCameraRequestSP_sprt_t request, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t *streamBuffer = NULL;
    ResultRequest resultRequest = NULL;
    camera3_capture_result_t *requestResult = NULL;
    List<int> *outputStreamList = NULL;

    /* 1. Get stream object for ZSL */
    /* Refer to the ZSL stream ID requested by the service request */
    request->getAllRequestOutputStreams(&outputStreamList);
    for (List<int>::iterator iter = outputStreamList->begin(); iter != outputStreamList->end(); iter++) {
        if ((*iter % HAL_STREAM_ID_MAX) == HAL_STREAM_ID_ZSL_OUTPUT) {
            CLOGV("[R%d F%d] request ZSL Stream ID %d",
                    request->getKey(), request->getFrameCount(), *iter);
            ret = m_streamManager->getStream(*iter, &stream);
            if (ret < 0) {
                CLOGE("getStream is failed, from streammanager. Id error:HAL_STREAM_ID_ZSL");
                return ret;
            }
            break;
        }
    }

    if (stream == NULL) {
        CLOGE("can not find the ZSL stream ID in the current request");
        return NAME_NOT_FOUND;
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(),
            EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. streamId HAL_STREAM_ID_ZSL",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    requestResult = resultRequest->getCaptureResult();
    if (requestResult == NULL) {
        CLOGE("[R%d F%d] getCaptureResult fail. streamId HAL_STREAM_ID_ZSL",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    streamBuffer = resultRequest->getStreamBuffer();
    if (streamBuffer == NULL) {
        CLOGE("[R%d F%d] getStreamBuffer fail. streamId HAL_STREAM_ID_ZSL",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    /* 2. Get camera3_stream structure from stream object */
    ret = stream->getStream(&(streamBuffer->stream));
    if (ret < 0) {
        CLOGE("getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_ZSL");
        return ret;
    }

    /* 3. Get zsl buffer */
    streamBuffer->buffer = buffer->handle[0];

    /* 4. Update the remained buffer info */
    ret = m_checkStreamBufferStatus(request, stream, &streamBuffer->status);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d S%d]Failed to checkStreamBufferStatus.",
            request->getKey(), request->getFrameCount(),
            HAL_STREAM_ID_ZSL_OUTPUT);
        return ret;
    }

    streamBuffer->acquire_fence = -1;
    streamBuffer->release_fence = -1;

    /* construct result for service */
    requestResult->frame_number = request->getKey();
    requestResult->result = NULL;
    requestResult->num_output_buffers = 1;
    requestResult->output_buffers = streamBuffer;
    requestResult->input_buffer = request->getInputBuffer();
    requestResult->partial_result = 0;

    CLOGV("frame number(%d), #out(%d)",
            requestResult->frame_number, requestResult->num_output_buffers);

    /* 6. Request to callback the result to request manager */
    requestMgr->pushResultRequest(resultRequest);

    ret = m_bufferSupplier->putBuffer(*buffer);
    if (ret != NO_ERROR) {
        CLOGE("[R%d F%d B%d]Failed to putBuffer. ret %d",
                request->getKey(), request->getFrameCount(), buffer->index, ret);
    }

    CLOGV("request->frame_number(%d), request->getNumOfOutputBuffer(%d)"
            "request->getCompleteBufferCount(%d) frame->getFrameCapture(%d)",
            request->getKey(), request->getNumOfOutputBuffer(),
            request->getCompleteBufferCount(), request->getFrameCount());

    CLOGV("streamBuffer info: stream (%p), handle(%p)",
            streamBuffer->stream, streamBuffer->buffer);

    return ret;

}

status_t ExynosCamera::m_sendMeta(ExynosCameraRequestSP_sprt_t request, EXYNOS_REQUEST_RESULT::TYPE type)
{
    ResultRequest resultRequest = NULL;
    uint32_t frameNumber = 0;
    camera3_capture_result_t *requestResult = NULL;
    status_t ret = OK;

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(), type);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. ALL_META",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    requestResult = resultRequest->getCaptureResult();
    if (requestResult == NULL) {
        CLOGE("[R%d F%d] getCaptureResult fail. ALL_META",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    frameNumber = request->getKey();
    request->setRequestLock();
    request->updatePipelineDepth();
    request->setRequestUnlock();

    /* CameraMetadata will be released by RequestManager
     * to keep the order of request
     */
    requestResult->frame_number = frameNumber;
    requestResult->result = NULL;
    requestResult->num_output_buffers = 0;
    requestResult->output_buffers = NULL;
    requestResult->input_buffer = NULL;
    requestResult->partial_result = 2;

    CLOGV("framecount %d request %d", request->getFrameCount(), request->getKey());

    requestMgr->pushResultRequest(resultRequest);

    return ret;
}


status_t ExynosCamera::m_sendNotifyShutter(ExynosCameraRequestSP_sprt_t request, uint64_t timeStamp)
{
    camera3_notify_msg_t *notify = NULL;
    ResultRequest resultRequest = NULL;
    uint32_t requestKey = 0;

    requestKey = request->getKey();
    if (timeStamp <= 0) {
        timeStamp = request->getSensorTimestamp();
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. Notify Callback",
                request->getKey(), request->getFrameCount());
        return INVALID_OPERATION;
    }

    notify = resultRequest->getNotifyMsg();
    if (notify == NULL) {
        CLOGE("[R%d F%d] getNotifyResult fail. Notify Callback",
                request->getKey(), request->getFrameCount());
        return INVALID_OPERATION;
    }

    CLOGV("[R%d] SHUTTER frame t(%ju)", requestKey, timeStamp);

    notify->type = CAMERA3_MSG_SHUTTER;
    notify->message.shutter.frame_number = requestKey;
    notify->message.shutter.timestamp = timeStamp;

    requestMgr->pushResultRequest(resultRequest);

    return OK;
}

status_t ExynosCamera::m_sendNotifyError(ExynosCameraRequestSP_sprt_t request,
                                          EXYNOS_REQUEST_RESULT::ERROR resultError,
                                          camera3_stream_t *stream)
{
    ResultRequest resultRequest = NULL;
    camera3_notify_msg_t *notify = NULL;
    ExynosCameraStream *streamInfo = NULL;
    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR;
    int streamId = 0;
    uint32_t requestKey = 0;

    if (request->getRequestState() == EXYNOS_REQUEST::STATE_ERROR) {
        CLOGV("[F%d] already send notify Error by ERROR_REQUEST", request->getFrameCount());
        return OK;
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(request);

    resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(), cbType);
    if (resultRequest == NULL) {
        CLOGE("[R%d F%d] createResultRequest fail. Notify Callback",
                request->getKey(), request->getFrameCount());
        return INVALID_OPERATION;
    }

    requestKey = request->getKey();
    notify = resultRequest->getNotifyMsg();
    if (notify == NULL) {
        CLOGE("[R%d F%d] getNotifyResult fail. Notify Callback",
                requestKey, request->getFrameCount());
        return INVALID_OPERATION;
    }

    notify->type = CAMERA3_MSG_ERROR;
    notify->message.error.frame_number = requestKey;

    switch (resultError) {
    case EXYNOS_REQUEST_RESULT::ERROR_REQUEST:
        CLOGE("[R%d F%d] REQUEST ERROR", requestKey, request->getFrameCount());
        notify->message.error.error_code = CAMERA3_MSG_ERROR_REQUEST;

        /* The error state of the request is set only if the error message is ERROR_REQUEST. */
        request->setRequestState(EXYNOS_REQUEST::STATE_ERROR);
        break;
    case EXYNOS_REQUEST_RESULT::ERROR_RESULT:
        CLOGE("[R%d F%d] RESULT ERROR", requestKey, request->getFrameCount());
        notify->message.error.error_code = CAMERA3_MSG_ERROR_RESULT;
        break;
    case EXYNOS_REQUEST_RESULT::ERROR_BUFFER:
        if (stream == NULL) {
            CLOGE("[R%d F%d] BUFFER ERROR. But stream value is NULL", requestKey, request->getFrameCount());
        } else {
            streamInfo = static_cast<ExynosCameraStream*>(stream->priv);
            streamInfo->getID(&streamId);
            CLOGE("[R%d F%d S%d] BUFFER ERROR",
                    requestKey, request->getFrameCount(), streamId);
        }
        notify->message.error.error_code = CAMERA3_MSG_ERROR_BUFFER;
        notify->message.error.error_stream = stream;
        break;
    case EXYNOS_REQUEST_RESULT::ERROR_UNKNOWN:
        CLOGE("[R%d F%d] UNKNOWN ERROR", requestKey, request->getFrameCount());
        return INVALID_OPERATION;
    }

    requestMgr->pushResultRequest(resultRequest);

    return OK;
}

status_t ExynosCamera::m_searchFrameFromList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock, uint32_t frameCount, ExynosCameraFrameSP_dptr_t frame)
{
    ExynosCameraFrameSP_sptr_t curFrame = NULL;
    List<ExynosCameraFrameSP_sptr_t>::iterator r;

    Mutex::Autolock l(listLock);
    if (list->empty()) {
        CLOGD("list is empty");
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE("curFrame is empty");
            return INVALID_OPERATION;
        }

        if (frameCount == curFrame->getFrameCount()) {
            CLOGV("frame count match: expected(%d)", frameCount);
            frame = curFrame;
            return NO_ERROR;
        }
        r++;
    } while (r != list->end());

    CLOGV("Cannot find match frame, frameCount(%d)", frameCount);

    return OK;
}

status_t ExynosCamera::m_removeFrameFromList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock, ExynosCameraFrameSP_sptr_t frame)
{
    ExynosCameraFrameSP_sptr_t curFrame = NULL;
    uint32_t frameUniqueKey = 0;
    uint32_t curFrameUniqueKey = 0;
    int frameCount = 0;
    int curFrameCount = 0;
    List<ExynosCameraFrameSP_sptr_t>::iterator r;

    if (frame == NULL) {
        CLOGE("frame is NULL");
        return BAD_VALUE;
    }

    Mutex::Autolock l(listLock);
    if (list->empty()) {
        CLOGE("list is empty");
        return INVALID_OPERATION;
    } else if (list->size() > MAX_BUFFERS) {
        CLOGW("There are too many frames(%zu) in this list. frame(F%d, T%d)",
                list->size(), frame->getFrameCount(), frame->getFrameType());
    }

    frameCount = frame->getFrameCount();
    frameUniqueKey = frame->getUniqueKey();
    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE("curFrame is empty");
            return INVALID_OPERATION;
        }

        curFrameCount = curFrame->getFrameCount();
        curFrameUniqueKey = curFrame->getUniqueKey();
        if (frameCount == curFrameCount
            && frameUniqueKey == curFrameUniqueKey) {
            CLOGV("frame count match: expected(%d, %u), current(%d, %u)",
                 frameCount, frameUniqueKey, curFrameCount, curFrameUniqueKey);
            list->erase(r);
            return NO_ERROR;
        }

        if (frameCount != curFrameCount
            && frameUniqueKey != curFrameUniqueKey) {
#ifndef USE_DUAL_CAMERA // HACK: Remove!
            CLOGW("frame count mismatch: expected(%d, %u), current(%d, %u)",
                 frameCount, frameUniqueKey, curFrameCount, curFrameUniqueKey);
            /* curFrame->printEntity(); */
            curFrame->printNotDoneEntity();
#endif
        }
        r++;
    } while (r != list->end());

#ifdef USE_DUAL_CAMERA // HACK: Remove!
    return NO_ERROR;
#else
    CLOGE("[F%d U%d]Cannot find match frame!!!", frameCount, frameUniqueKey);

    return INVALID_OPERATION;
#endif
}

uint32_t ExynosCamera::m_getSizeFromFrameList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock)
{
    uint32_t count = 0;
    Mutex::Autolock l(listLock);

    count = list->size();

    return count;
}

status_t ExynosCamera::m_clearList(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock)
{
    ExynosCameraFrameSP_sptr_t curFrame = NULL;
    List<ExynosCameraFrameSP_sptr_t>::iterator r;

    CLOGD("remaining frame(%zu), we remove them all", list->size());

    Mutex::Autolock l(listLock);
    while (!list->empty()) {
        r = list->begin()++;
        curFrame = *r;
        if (curFrame != NULL) {
            CLOGV("remove frame count(%d)", curFrame->getFrameCount());
            curFrame = NULL;
        }
        list->erase(r);
    }

    return OK;
}

status_t ExynosCamera::m_removeInternalFrames(List<ExynosCameraFrameSP_sptr_t> *list, Mutex *listLock)
{
    ExynosCameraFrameSP_sptr_t curFrame = NULL;
    List<ExynosCameraFrameSP_sptr_t>::iterator r;

    CLOGD("remaining frame(%zu), we remove internal frames",
             list->size());

    Mutex::Autolock l(listLock);
    while (!list->empty()) {
        r = list->begin()++;
        curFrame = *r;
        if (curFrame != NULL) {
            if (curFrame->getFrameType() == FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
                || curFrame->getFrameType() == FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
                || curFrame->getFrameType() == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif
                ) {
                CLOGV("remove internal frame(%d)",
                         curFrame->getFrameCount());
                m_releaseInternalFrame(curFrame);
            } else {
                CLOGW("frame(%d) is NOT internal frame and will be remained in List",
                         curFrame->getFrameCount());
            }
        }
        list->erase(r);
        curFrame = NULL;
    }

    return OK;
}

status_t ExynosCamera::m_releaseInternalFrame(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer buffer;
    ExynosCameraFrameFactory *factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    if (frame == NULL) {
        CLOGE("frame is NULL");
        return BAD_VALUE;
    }
    if (frame->getFrameType() != FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
        && frame->getFrameType() != FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        && frame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif
        ) {
        CLOGE("frame(%d) is NOT internal frame",
                 frame->getFrameCount());
        return BAD_VALUE;
    }

    /* Return bayer buffer */
    if (frame->getRequest(PIPE_VC0) == true) {
        int pipeId = m_getBayerPipeId();
        int dstPos = factory->getNodeType(PIPE_VC0);

        ret = frame->getDstBuffer(pipeId, &buffer, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("getDstBuffer(pipeId(%d), dstPos(%d)) fail", pipeId, dstPos);
        } else if (buffer.index >= 0) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer for VC0. ret %d",
                        frame->getFrameCount(), buffer.index, ret);
            }
        }
    }

    /* Return sensor gyro buffer */
    int sensorGyroPipeId = m_getSensorGyroPipeId();

    if (frame->getRequest(sensorGyroPipeId) == true) {
        int dstPos = factory->getNodeType(sensorGyroPipeId);

        ret = frame->getDstBuffer(sensorGyroPipeId, &buffer, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("getDstBuffer(sensorGyroPipeId(%d), dstPos(%d)) fail", sensorGyroPipeId, dstPos);
        } else if (buffer.index >= 0) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer for VC3. ret %d",
                    frame->getFrameCount(), buffer.index, ret);
            }
        }
    }

    ///////////////////////////////////////////////

    /* Return 3AS buffer */
    ret = frame->getSrcBuffer(PIPE_3AA, &buffer);
    if (ret != NO_ERROR) {
        CLOGE("getSrcBuffer failed. PIPE_3AA, ret(%d)", ret);
    } else if (buffer.index >= 0) {
        ret = m_bufferSupplier->putBuffer(buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to putBuffer for 3AS. ret %d",
                    frame->getFrameCount(), buffer.index, ret);
        }
    }

    /* Return 3AP buffer */
    if (frame->getRequest(PIPE_3AP) == true) {
        ret = frame->getDstBuffer(PIPE_3AA, &buffer);
        if (ret != NO_ERROR) {
            CLOGE("getDstBuffer failed. PIPE_3AA, ret %d",
                     ret);
        } else if (buffer.index >= 0) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer for 3AP. ret %d",
                        frame->getFrameCount(), buffer.index, ret);
            }
        }
    }

    frame = NULL;

    return ret;
}

uint32_t ExynosCamera::m_getSizeFromRequestList(List<ExynosCameraRequestSP_sprt_t> *list, Mutex *listLock)
{
    uint32_t count = 0;
    Mutex::Autolock l(listLock);

    count = list->size();

    return count;
}

status_t ExynosCamera::m_clearRequestList(List<ExynosCameraRequestSP_sprt_t> *list, Mutex *listLock)
{
    ExynosCameraRequestSP_sprt_t curRequest = NULL;
    List<ExynosCameraRequestSP_sprt_t>::iterator r;

    CLOGD("remaining request(%zu), we remove them all", list->size());

    Mutex::Autolock l(listLock);
    while (!list->empty()) {
        r = list->begin()++;
        curRequest = *r;
        if (curRequest != NULL) {
            CLOGV("remove request key(%d), fcount(%d)",
                   curRequest->getKey(), curRequest->getFrameCount());
            curRequest = NULL;
        }
        list->erase(r);
    }

    return OK;
}

status_t ExynosCamera::m_checkStreamBufferStatus(ExynosCameraRequestSP_sprt_t request, ExynosCameraStream *stream,
                                                  int *status, bool bufferSkip)
{
    status_t ret = NO_ERROR;
    int streamId = 0;
    camera3_stream_t *serviceStream = NULL;

    if (stream == NULL) {
        CLOGE("stream is NULL");
        return BAD_VALUE;
    }

    stream->getID(&streamId);

    if (request->getRequestState() == EXYNOS_REQUEST::STATE_ERROR) {
        /* If the request state is error, the buffer status is always error. */
        request->setStreamBufferStatus(streamId, CAMERA3_BUFFER_STATUS_ERROR);
    } else {
        if (request->getStreamBufferStatus(streamId) == CAMERA3_BUFFER_STATUS_ERROR) {
            /* Temp code */
            {
                ret = stream->getStream(&serviceStream);
                if (ret != NO_ERROR) {
                    CLOGE("Failed to getStream from ExynosCameraStream. streamId(%d)", streamId);
                    return ret;
                }

                ret = m_sendNotifyError(request, EXYNOS_REQUEST_RESULT::ERROR_BUFFER, serviceStream);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d R%d S%d] sendNotifyError fail. ret %d",
                            request->getFrameCount(), request->getKey(), streamId, ret);
                    return ret;
                }
            }
        } else {
            if (bufferSkip == true) {
                /* The state of the buffer is normal,
                 * but if you want to skip the buffer, set the buffer status to ERROR.
                 * NotifyError is not given. */
                request->setStreamBufferStatus(streamId, CAMERA3_BUFFER_STATUS_ERROR);
            }
        }
    }

    *status = request->getStreamBufferStatus(streamId);

    CLOGV("[R%d F%d S%d] buffer status(%d)",
            request->getKey(), request->getFrameCount(), streamId, *status);

    return ret;
}

void ExynosCamera::m_checkUpdateResult(ExynosCameraFrameSP_sptr_t frame, uint32_t streamConfigBit)
{
    enum FRAME_TYPE frameType;
    bool previewFlag = false;
    bool captureFlag= false;
    bool zslInputFlag = false;
    uint32_t targetBit = 0;
    frame_handle_components_t components;

    frameType = (enum FRAME_TYPE) frame->getFrameType();
    m_getFrameHandleComponentsWrapper(frame, &components);
    switch (frameType) {
        case FRAME_TYPE_PREVIEW:
#ifdef USE_DUAL_CAMERA
        case FRAME_TYPE_PREVIEW_SLAVE:
        case FRAME_TYPE_PREVIEW_DUAL_MASTER:
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
#endif
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_PREVIEW);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_VIDEO);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_CALLBACK);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_PREVIEW_VIDEO);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_ZSL_OUTPUT);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_DEPTHMAP);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_CALLBACK_PHYSICAL);
            previewFlag = (COMPARE_STREAM_CONFIG_BIT(streamConfigBit, targetBit) > 0) ? true : false;

            if (previewFlag == true) {
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
            }

            targetBit = 0;
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_RAW);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_ZSL_INPUT);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_JPEG);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_CALLBACK_STALL);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_DEPTHMAP_STALL);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_THUMBNAIL_CALLBACK);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_YUV_INPUT);
            captureFlag = (COMPARE_STREAM_CONFIG_BIT(streamConfigBit, targetBit) > 0) ? true : false;

            if (captureFlag == false) {
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
            }

            break;
        case FRAME_TYPE_REPROCESSING:
#ifdef USE_DUAL_CAMERA
        case FRAME_TYPE_REPROCESSING_SLAVE:
        case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
#endif
#ifdef SUPPORT_REMOSAIC_CAPTURE
        case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
#endif //SUPPORT_REMOSAIC_CAPTURE
            targetBit = 0;
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_RAW);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_ZSL_INPUT);
            if (components.parameters->isUseVideoHQISP() == false)
                SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_JPEG);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_CALLBACK_STALL);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_DEPTHMAP_STALL);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_THUMBNAIL_CALLBACK);
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_YUV_INPUT);
            zslInputFlag = (COMPARE_STREAM_CONFIG_BIT(streamConfigBit, SET_BIT(HAL_STREAM_ID_ZSL_INPUT)) > 0) ? true : false;
            if (zslInputFlag == false && COMPARE_STREAM_CONFIG_BIT(streamConfigBit, SET_BIT(HAL_STREAM_ID_YUV_INPUT)) > 0) {
                zslInputFlag = true;
            }
            captureFlag = (COMPARE_STREAM_CONFIG_BIT(streamConfigBit, targetBit) > 0) ? true : false;

            /*
             * Partial update will be done by internal frame or preview frame
             * However, if the ZSL input is present, the meta value will not be known in the preview stream.
             * So partial update will be performed after the capture stream is finished.
             */

            if (zslInputFlag == true) {
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
            }

            if (captureFlag == true) {
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
            }

            break;
        case FRAME_TYPE_VISION:
            targetBit = 0;
            SET_STREAM_CONFIG_BIT(targetBit, HAL_STREAM_ID_VISION);
            previewFlag = (COMPARE_STREAM_CONFIG_BIT(streamConfigBit, targetBit) > 0) ? true : false;

            if (previewFlag == true) {
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
                frame->setStatusForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL,
                                                ExynosCameraFrame::RESULT_UPDATE_STATUS_REQUIRED);
            }

            break;
        default:
            CLOGE("[F%d]Unsupported frame type %d", frame->getFrameCount(), frameType);
            break;
    }

    CLOGV("[F%d T%d]flags %d/%d streamConfig %x",
            frame->getFrameCount(),
            frame->getFrameType(),
            previewFlag, captureFlag,
            streamConfigBit);

    return;
}

status_t ExynosCamera::m_setFrameManager()
{
    sp<FrameWorker> worker;

    m_resourceManager->initManagerResources(&m_camIdInfo, nullptr, nullptr, &m_frameMgr);

    worker = new CreateWorker("CREATE FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT, 300, 200);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::CREATE, worker);

    worker = new RunWorker("RUNNING FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT, 100, 300);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::RUNNING, worker);

    sp<KeyBox> key = new KeyBox("FRAME KEYBOX", m_cameraId);

    m_frameMgr->setKeybox(key);

    return NO_ERROR;
}

bool ExynosCamera::m_frameFactoryCreateThreadFunc(void)
{
    CLOGI("");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    bool loop = false;
    status_t ret = NO_ERROR;

    ExynosCameraFrameFactory *framefactory = NULL;


    m_framefactoryCreateResult = NO_ERROR;
    ret = m_frameFactoryQ->waitAndPopProcessQ(&framefactory);
    if (ret < 0) {
        CLOGE("wait and pop fail, ret(%d)", ret);
        goto func_exit;
    }

    if (framefactory == NULL) {
        CLOGE("framefactory is NULL");
        goto func_exit;
    }

    if (framefactory->isCreated() == false) {
        CLOGD("framefactory create");
        TIME_LOGGER_UPDATE(m_cameraId, framefactory->getFactoryType(), framefactory->getCameraId(), SUB_CUMULATIVE_CNT, FACTORY_CREATE_START, 0);
        ret = framefactory->create();
        TIME_LOGGER_UPDATE(m_cameraId, framefactory->getFactoryType(), framefactory->getCameraId(), SUB_CUMULATIVE_CNT, FACTORY_CREATE_END, 0);
        if (ret != NO_ERROR) {
            CLOGD("failed to create framefactory (%d)", ret);
            m_framefactoryCreateResult = NO_INIT;
            return false;
        }
    } else {
        CLOGD("framefactory already create");
    }

func_exit:
    if (0 < m_frameFactoryQ->getSizeOfProcessQ()) {
        loop = true;
    }


    return loop;
}

bool ExynosCamera::m_dualFrameFactoryCreateThreadFunc(void)
{
    CLOGI("");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    bool loop = false;
    status_t ret = NO_ERROR;

    ExynosCameraFrameFactory *framefactory = NULL;

    m_dualFramefactoryCreateResult = NO_ERROR;
    ret = m_dualFrameFactoryQ->waitAndPopProcessQ(&framefactory);
    if (ret < 0) {
        CLOGE("wait and pop fail, ret(%d)", ret);
        goto func_exit;
    }

    if (framefactory == NULL) {
        CLOGE("framefactory is NULL");
        goto func_exit;
    }

    if (framefactory->isCreated() == false) {
        CLOGD("framefactory create");
        TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, FACTORY_CREATE_START, 0);
        ret = framefactory->create();
        TIME_LOGGER_UPDATE(m_cameraId, 0, 0, CUMULATIVE_CNT, FACTORY_CREATE_END, 0);
        if (ret != NO_ERROR) {
            CLOGD("failed to create framefactory (%d)", ret);
            m_dualFramefactoryCreateResult = NO_INIT;
            return false;
        }
    } else {
        CLOGD("framefactory already create");
    }

func_exit:
    if (0 < m_dualFrameFactoryQ->getSizeOfProcessQ()) {
        loop = true;
    }


    return loop;
}

status_t ExynosCamera::m_startFrameFactory(ExynosCameraFrameFactory *factory)
{
    status_t ret = OK;

    if (factory == NULL) {
        CLOGE("frameFactory is NULL");
        return BAD_VALUE;
    }

    /* prepare pipes */
    ret = factory->preparePipes();
    if (ret != NO_ERROR) {
        CLOGW("Failed to prepare FLITE");
    }

    /* stream on pipes */
    ret = factory->startPipes();
    if (ret != NO_ERROR) {
        CLOGE("startPipe fail");
        return ret;
    }

    /* start all thread */
    ret = factory->startInitialThreads();
    if (ret != NO_ERROR) {
        CLOGE("startInitialThreads fail");
        return ret;
    }

    return ret;
}

status_t ExynosCamera::m_stopFrameFactory(ExynosCameraFrameFactory *factory)
{
    int ret = 0;

    CLOGD("");
    if (factory != NULL  && factory->isRunning() == true) {
        ret = factory->stopPipes();
        if (ret < 0) {
            CLOGE("stopPipe fail");
            return ret;
        }
    }

    return ret;
}

status_t ExynosCamera::m_deinitFrameFactory()
{
    CLOGI("-IN-");

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *frameFactory = NULL;

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (m_frameFactory[i] != NULL) {
            frameFactory = m_frameFactory[i];

            if(isOfflineCaptureRunning()) {
                if(frameFactory->isForReprocessing()) {
                    CLOGD("[OFFLINE] skip deinit reprocessing factory");
                    continue;
                }
            }

            for (int k = i + 1; k < FRAME_FACTORY_TYPE_MAX; k++) {
               if (frameFactory == m_frameFactory[k]) {
                   CLOGD("m_frameFactory index(%d) and index(%d) are same instance, "
                        "set index(%d) = NULL", i, k, k);
                   m_frameFactory[k] = NULL;
               }
            }

            ret = m_stopFrameFactory(m_frameFactory[i]);
            if (ret < 0)
                CLOGE("m_frameFactory[%d] stopPipes fail", i);

            CLOGD("m_frameFactory[%d] stopPipes", i);

            if (m_frameFactory[i]->isCreated() == true) {
                TIME_LOGGER_UPDATE(m_cameraId, m_frameFactory[i]->getFactoryType(), m_frameFactory[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_DESTROY_START, 0);
                ret = m_frameFactory[i]->destroy();
                TIME_LOGGER_UPDATE(m_cameraId, m_frameFactory[i]->getFactoryType(), m_frameFactory[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_DESTROY_END, 0);
                if (ret < 0)
                    CLOGE("m_frameFactory[%d] destroy fail", i);

                SAFE_DELETE(m_frameFactory[i]);

                CLOGD("m_frameFactory[%d] destroyed", i);
            }
        }
    }

    if (m_resourceManager) {
        m_resourceManager->registerFrameFactory(m_frameFactory);
    } else {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):m_resourceManager is NULL!!", __FUNCTION__, __LINE__);
    }

    CLOGI("-OUT-");

    return ret;

}

status_t ExynosCamera::m_deinitReprocessingFrameFactory(int cameraSessionId)
{
    CLOGI("-IN-");

    ExynosCameraResourceManager::resources_t* resources = m_resourceManager->getResource(cameraSessionId);

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *frameFactory = NULL;

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (resources->frameFactories[i] != NULL) {
            frameFactory = resources->frameFactories[i];

            for (int k = i + 1; k < FRAME_FACTORY_TYPE_MAX; k++) {
               if (frameFactory == resources->frameFactories[k]) {
                   CLOGD("m_frameFactory index(%d) and index(%d) are same instance, "
                        "set index(%d) = NULL", i, k, k);
                   resources->frameFactories[k] = NULL;
               }
            }

            if (frameFactory->isForReprocessing() == false) {
               CLOGD("skip deinit preview factory");
               continue;
            }

            ret = m_stopFrameFactory(resources->frameFactories[i]);
            if (ret < 0)
                CLOGE("m_frameFactory[%d] stopPipes fail", i);

            CLOGD("m_frameFactory[%d] stopPipes", i);

            if (resources->frameFactories[i]->isCreated() == true) {
                TIME_LOGGER_UPDATE(m_cameraId, resources->frameFactories[i]->getFactoryType(), resources->frameFactories[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_DESTROY_START, 0);
                ret = resources->frameFactories[i]->destroy();
                TIME_LOGGER_UPDATE(m_cameraId, resources->frameFactories[i]->getFactoryType(), resources->frameFactories[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_DESTROY_END, 0);
                if (ret < 0)
                    CLOGE("m_frameFactory[%d] destroy fail", i);

                SAFE_DELETE(resources->frameFactories[i]);

                CLOGD("m_frameFactory[%d] destroyed", i);
            }
        }
    }

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        resources->frameFactories[i] = NULL;
    }

    CLOGI("-OUT-");

    return ret;

}


status_t ExynosCamera::m_setupPipeline(ExynosCameraFrameFactory *factory)
{
    status_t ret = NO_ERROR;

    if (factory == NULL) {
        CLOGE("Frame factory is NULL!!");
        return BAD_VALUE;
    }

    int pipeId = MAX_PIPE_NUM;
    int nodePipeId = MAX_PIPE_NUM;
    int32_t cameraId = factory->getCameraId();
    int32_t reprocessingBayerMode = m_parameters[cameraId]->getReprocessingBayerMode();
    bool flagFlite3aaM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M);
    bool flag3aaIspM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_3AA, PIPE_ISP) == HW_CONNECTION_MODE_M2M);
    bool flagIspMcscM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_ISP, PIPE_MCSC) == HW_CONNECTION_MODE_M2M);
    bool flag3aaVraM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_3AA, PIPE_VRA) == HW_CONNECTION_MODE_M2M);
    bool flagMcscVraM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_MCSC, PIPE_VRA) == HW_CONNECTION_MODE_M2M);
#ifdef USE_CLAHE_PREVIEW
    bool flagMcscClaheM2M = (m_parameters[cameraId]->getHwConnectionMode(PIPE_MCSC, PIPE_CLAHE) == HW_CONNECTION_MODE_M2M);
#endif
#ifdef USE_DUAL_CAMERA
    bool isDualMode = m_configurations->getMode(CONFIGURATION_DUAL_MODE);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif
    int flipHorizontal = 0;
    int flipVertical = 0;
    enum NODE_TYPE nodeType;

    if (flag3aaVraM2M)
        flagMcscVraM2M = false;

    m_setSetfile();

    /* Setting default Bayer DMA-out request flag */
    switch (reprocessingBayerMode) {
    case REPROCESSING_BAYER_MODE_NONE : /* Not using reprocessing */
        CLOGD("Use REPROCESSING_BAYER_MODE_NONE");
        factory->setRequest(PIPE_VC0, false);
        factory->setRequest(PIPE_3AC, false);
        break;
    case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON :
        CLOGD("Use REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON");
        factory->setRequest(PIPE_VC0, true);
        factory->setRequest(PIPE_3AC, false);
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON :
        CLOGD("Use REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON");
        factory->setRequest(PIPE_VC0, false);
        factory->setRequest(PIPE_3AC, true);
        break;
    case REPROCESSING_BAYER_MODE_PURE_DYNAMIC :
        CLOGD("Use REPROCESSING_BAYER_MODE_PURE_DYNAMIC");
        factory->setRequest(PIPE_VC0, false);
        factory->setRequest(PIPE_3AC, false);
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC :
        CLOGD("Use REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC");
        factory->setRequest(PIPE_VC0, false);
        factory->setRequest(PIPE_3AC, false);
        break;
    default :
        CLOGE("Unknown dynamic bayer mode");
        factory->setRequest(PIPE_3AC, false);
        break;
    }

    if (flagFlite3aaM2M == true) {
        factory->setRequest(PIPE_VC0, true);
    }

    if (flagMcscVraM2M == true) {
        factory->setRequest(PIPE_MCSC5, true);
    }

    if (flag3aaVraM2M)
        factory->setRequest(PIPE_3AF, true);

    /* Setting bufferSupplier based on H/W pipeline */
    if (flagFlite3aaM2M == true) {
        pipeId = PIPE_FLITE;

        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }

        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
    }

    pipeId = PIPE_3AA;

    if (flag3aaIspM2M == true) {
        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }

        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);

        if (m_configurations->getMode(CONFIGURATION_GMV_MODE) == true) {
            pipeId = PIPE_GMV;
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
        }

        pipeId = PIPE_ISP;
    }

    if (flagIspMcscM2M == true) {
        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }

        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);

        pipeId = PIPE_MCSC;
    }

    ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setBufferSupplierToPipe into pipeId %d ", pipeId);
        return ret;
    }

    /* Setting OutputFrameQ/FrameDoneQ to Pipe */
    factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);

    if (cameraId == m_cameraId) {
        flipHorizontal  = m_configurations->getModeValue(CONFIGURATION_FLIP_HORIZONTAL);
        flipVertical    = m_configurations->getModeValue(CONFIGURATION_FLIP_VERTICAL);

        for (nodePipeId = PIPE_MCSC0; nodePipeId <= PIPE_MCSC2; nodePipeId++) {
            nodeType = factory->getNodeType(nodePipeId);

            factory->setControl(V4L2_CID_HFLIP, 0, pipeId, nodeType);
            factory->setControl(V4L2_CID_VFLIP, 0, pipeId, nodeType);
        }

        if (m_flagVideoStreamPriority == true) {
            nodePipeId = m_streamManager->getOutputPortId(HAL_STREAM_ID_PREVIEW);
        } else {
            nodePipeId = m_streamManager->getOutputPortId(HAL_STREAM_ID_VIDEO);
        }

        if (nodePipeId != -1) {
            nodePipeId = nodePipeId + PIPE_MCSC0;
            nodeType = factory->getNodeType(nodePipeId);

            ret = factory->setControl(V4L2_CID_HFLIP, flipHorizontal, pipeId, nodeType);
            if (ret < 0)
                CLOGE("V4L2_CID_HFLIP fail, ret(%d)", ret);

            ret = factory->setControl(V4L2_CID_VFLIP, flipVertical, pipeId, nodeType);
            if(ret < 0)
                CLOGE("V4L2_CID_VFLIP fail, ret(%d)", ret);

            CLOGD("set flipHorizontal(%d) flipVertical(%d) Recording nodePipeId(%d)",
                    flipHorizontal, flipVertical, nodePipeId);
        }
    }

#ifdef USE_CLAHE_PREVIEW
    pipeId = PIPE_CLAHE;

    if (flagMcscClaheM2M == true) {
        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }

        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
    }
#endif

#if defined (USES_CAMERA_EXYNOS_VPL) || defined (USE_VRA_FD)
    if (flagMcscVraM2M || flag3aaVraM2M) {
#ifdef USE_VRA_FD
        pipeId = PIPE_VRA;
#else
        pipeId = PIPE_NFD;
#endif

        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }

        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
#ifdef SUPPORT_HFD
        if (m_parameters[m_cameraId]->getHfdMode() == true) {
            pipeId = PIPE_HFD;
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
        }
#endif
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    pipeId = PIPE_SW_MCSC;
    ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
        return ret;
    }

    /* Setting OutputFrameQ/FrameDoneQ to Pipe */
    factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
#endif

#ifdef USE_DUAL_CAMERA
    if (cameraId == m_cameraId && isDualMode == true) {
#ifdef USE_DUAL_BAYER_SYNC
        pipeId = PIPE_BAYER_SYNC;
        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }
        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
#endif

        pipeId = PIPE_SYNC;
        ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
            return ret;
        }
        /* Setting OutputFrameQ/FrameDoneQ to Pipe */
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);

        if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION) {
            pipeId = PIPE_FUSION;

            ret = factory->setBufferSupplierToPipe(m_bufferSupplier, pipeId);
            if (ret != NO_ERROR) {
                CLOGE("Failed to setBufferSupplierToPipe into pipeId %d", pipeId);
                return ret;
            }

            /* Setting OutputFrameQ/FrameDoneQ to Pipe */
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
        }
    }
#endif

#ifdef SUPPORT_HW_GDC
    pipeId = PIPE_GDC;
    factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
#endif

#ifdef USE_SLSI_PLUGIN
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[i], i);
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    pipeId = PIPE_NFD;
    factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[pipeId], pipeId);
#endif

#ifdef USES_SENSOR_GYRO_FACTORY_MODE
    ////////////////////////////////////////////////
    // set ioctl to driver, to let the step now.
    if (m_configurations->getOisTestReqired(CONFIGURATION_OIS_TEST_GEA) == true) {
        CLOGD("[MotFactory] m_sensorGyroTestIndex : %d", m_sensorGyroTestIndex);

        // driver need to get step1 / step2 (not step0, step1)
        int realSensorGyroTestIndex = m_sensorGyroTestIndex + 1;

        ret = factory->setControl(V4L2_CID_IS_TUNE_FACTORY_GYRO_SELF_TEST, realSensorGyroTestIndex, PIPE_3AA);
        if (ret != NO_ERROR) {
            CLOGE("[MotFactory] V4L2_CID_IS_TUNE_FACTORY_GYRO_SELF_TEST(realSensorGyroTestIndex(%d), PIPE_3AA) fail", realSensorGyroTestIndex);

            // just ignore.
            ret = NO_ERROR;
        }

        CLOGD("[MotFactory] V4L2_CID_IS_TUNE_FACTORY_GYRO_SELF_TEST(realSensorGyroTestIndex(%d), PIPE_3AA) done", realSensorGyroTestIndex);
    }
    ////////////////////////////////////////////////
#endif

    return ret;
}

status_t ExynosCamera::m_updateLatestInfoToShot(struct camera2_shot_ext *shot_ext,
                                                frame_type_t frameType,
                                                frame_handle_components_t *components)
{
    status_t ret = NO_ERROR;
    bool isReprocessing = false;

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return BAD_VALUE;
    }

    switch (frameType) {
    case FRAME_TYPE_VISION:
        isReprocessing = false;
        break;
    case FRAME_TYPE_PREVIEW:
    case FRAME_TYPE_INTERNAL:
#ifdef USE_DUAL_CAMERA
    case FRAME_TYPE_PREVIEW_SLAVE:
    case FRAME_TYPE_PREVIEW_DUAL_MASTER:
    case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
    case FRAME_TYPE_INTERNAL_SLAVE:
    case FRAME_TYPE_TRANSITION:
    case FRAME_TYPE_TRANSITION_SLAVE:
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION:
#endif
        isReprocessing = false;
        m_updateSetfile(shot_ext, isReprocessing, components->parameters);
#if 0
        m_updateTnr(shot_ext, components->parameters);
#endif
        break;
    case FRAME_TYPE_REPROCESSING:
#ifdef USE_DUAL_CAMERA
    case FRAME_TYPE_REPROCESSING_SLAVE:
    case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
    case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
#endif //SUPPORT_REMOSAIC_CAPTURE
        isReprocessing = true;
        m_updateJpegControlInfo(shot_ext);
        m_updateSetfile(shot_ext, isReprocessing, components->parameters);
        break;
    default:
        CLOGE("Invalid frame type(%d)", frameType);
        break;
    }

    /* This section is common function */
    m_updateCropRegion(shot_ext, components, frameType, isReprocessing);
    m_updateMasterCam(shot_ext, isReprocessing);
    m_updateEdgeNoiseMode(shot_ext, isReprocessing);

    return ret;
}

void ExynosCamera::m_updateCropRegion(struct camera2_shot_ext *shot_ext,
                                frame_handle_components_t *components,
                                frame_type_t frameType,
                                bool isReprocessing)
{
    int sensorMaxW = 0, sensorMaxH = 0;
    int cropRegionMinW = 0, cropRegionMinH = 0;
    int maxZoom = 0;
    float activeZoomRatio = 1.0f;
    ExynosRect targetActiveZoomRect = {0, };
    ExynosCameraParameters *parameters;

    /*
     * TODO : This function needs detailed review. something fishy.
     * calling sequence : xxxFrameHandler-> m_updateLatestInfoToShot -> m_updateCropRegion
     * It is called for all the frame_types. (Ex: PREVIEW, PREVIEW_SLAVE, DUAL_MASTER, DUAL_SLAVE)
     * when the function is called for FRAME_TYPE_PREVIEW, both master and slave CropRegion is getting updated.
     * similarly, for FRAME_TYPE_SLAVE,  both master and slave CropRegion is getting updated.
     * Is it really intended????
     */

    if (components == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):components is null. so, assert!!!!",
                            __FUNCTION__, __LINE__);
    }

    parameters = components->parameters; //it can be for the master or slave;
    parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&sensorMaxW, (uint32_t *)&sensorMaxH);

    shot_ext->shot.ctl.scaler.cropRegion[0] = ALIGN_DOWN(shot_ext->shot.ctl.scaler.cropRegion[0], 2);
    shot_ext->shot.ctl.scaler.cropRegion[1] = ALIGN_DOWN(shot_ext->shot.ctl.scaler.cropRegion[1], 2);
    shot_ext->shot.ctl.scaler.cropRegion[2] = ALIGN_UP(shot_ext->shot.ctl.scaler.cropRegion[2], 2);
    shot_ext->shot.ctl.scaler.cropRegion[3] = ALIGN_UP(shot_ext->shot.ctl.scaler.cropRegion[3], 2);

    targetActiveZoomRect.x = shot_ext->shot.ctl.scaler.cropRegion[0];
    targetActiveZoomRect.y = shot_ext->shot.ctl.scaler.cropRegion[1];
    targetActiveZoomRect.w = shot_ext->shot.ctl.scaler.cropRegion[2];
    targetActiveZoomRect.h = shot_ext->shot.ctl.scaler.cropRegion[3];

#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
        // vendor specific logic
        m_updateCropRegion_vendor(shot_ext, components, frameType, targetActiveZoomRect, sensorMaxW, sensorMaxH, isReprocessing);
    } else if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        parameters->setActiveZoomMargin(0);

        targetActiveZoomRect.x = 0;
        targetActiveZoomRect.y = 0;
        targetActiveZoomRect.w = sensorMaxW;
        targetActiveZoomRect.h = sensorMaxH;
    } else
#endif /* USE_DUAL_CAMERA */
    {
        parameters->setActiveZoomMargin(0);
    }


    /*  Check the validation of the crop size(width x height).
     The width and height of the crop region cannot be set to be smaller than
     floor( activeArraySize.width / android.scaler.availableMaxDigitalZoom )
     and floor( activeArraySize.height / android.scaler.availableMaxDigitalZoom )
     */
    maxZoom = (int) (parameters->getMaxZoomRatio() / 1000);
    cropRegionMinW = (int)ceil((float)sensorMaxW / maxZoom);
    cropRegionMinH = (int)ceil((float)sensorMaxH / maxZoom);

    cropRegionMinW = ALIGN_UP(cropRegionMinW, 2);
    cropRegionMinH = ALIGN_UP(cropRegionMinH, 2);

    if (cropRegionMinW > (int) targetActiveZoomRect.w
        || cropRegionMinH > (int) targetActiveZoomRect.h){
        CLOGE("FT(%d) Invalid Crop Size(%d, %d),Change to cropRegionMin(%d, %d)",
                frameType,
                targetActiveZoomRect.w,
                targetActiveZoomRect.h,
                cropRegionMinW, cropRegionMinH);

        targetActiveZoomRect.w = cropRegionMinW;
        targetActiveZoomRect.h = cropRegionMinH;
    }

    /* 1. Check the validation of the crop size(width x height).
     * The crop size must be smaller than sensor max size.
     */
    if (sensorMaxW < (int) targetActiveZoomRect.w
        || sensorMaxH < (int)targetActiveZoomRect.h) {
        CLOGE("FT(%d) Invalid Crop Size(%d, %d), sensorMax(%d, %d)",
                frameType,
                targetActiveZoomRect.w,
                targetActiveZoomRect.h,
                sensorMaxW, sensorMaxH);
        targetActiveZoomRect.w = sensorMaxW;
        targetActiveZoomRect.h = sensorMaxH;
    }

    /* 2. Check the validation of the crop offset.
     * Offset coordinate + width or height must be smaller than sensor max size.
     */
    if (sensorMaxW < (int) targetActiveZoomRect.x
            + (int) targetActiveZoomRect.w) {
        CLOGE("Invalid Crop Region, offsetX(%d), width(%d) sensorMaxW(%d)",
                targetActiveZoomRect.x,
                targetActiveZoomRect.w,
                sensorMaxW);
        targetActiveZoomRect.x = sensorMaxW - targetActiveZoomRect.w;
    }

    if (sensorMaxH < (int) targetActiveZoomRect.y
            + (int) targetActiveZoomRect.h) {
        CLOGE("Invalid Crop Region, offsetY(%d), height(%d) sensorMaxH(%d)",
                targetActiveZoomRect.y,
                targetActiveZoomRect.h,
                sensorMaxH);
        targetActiveZoomRect.y = sensorMaxH - targetActiveZoomRect.h;
    }

    if (isReprocessing) {
        parameters->setPictureCropRegion(targetActiveZoomRect.x,
                                         targetActiveZoomRect.y,
                                         targetActiveZoomRect.w,
                                         targetActiveZoomRect.h);

        /* set main active zoom rect */
        parameters->setPictureActiveZoomRect(targetActiveZoomRect);

        /* set main active zoom ratio */
        activeZoomRatio = (float)(sensorMaxW) / (float)(targetActiveZoomRect.w);
        parameters->setPictureActiveZoomRatio(activeZoomRatio);
    } else {
        parameters->setCropRegion(targetActiveZoomRect.x,
                                  targetActiveZoomRect.y,
                                  targetActiveZoomRect.w,
                                  targetActiveZoomRect.h);

        /* set main active zoom rect */
        parameters->setActiveZoomRect(targetActiveZoomRect);

        /* set main active zoom ratio */
        activeZoomRatio = (float)(sensorMaxW) / (float)(targetActiveZoomRect.w);
        parameters->setActiveZoomRatio(activeZoomRatio);
    }
}

void ExynosCamera::m_updateFD(struct camera2_shot_ext *shot_ext, enum facedetect_mode fdMode,
                              int dsInputPortId, bool isReprocessing, bool isEarlyFd)
{
#ifdef USE_ALWAYS_FD_OFF
    {
        shot_ext->fd_bypass = 1;
        shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_OFF;
        shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = MCSC_PORT_NONE;
        return;
    }
#endif
    if (fdMode <= FACEDETECT_MODE_OFF) {
        shot_ext->fd_bypass = 1;
        shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_OFF;
        shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = MCSC_PORT_NONE;
    } else {
        if (isReprocessing == true) {
            shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_OFF;
            shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = MCSC_PORT_NONE;
            if (isEarlyFd) {
                // TODO: need to check it.
#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
                shot_ext->fd_bypass = 0;
                shot_ext->shot.ctl.stats.faceDetectMode = fdMode;
#else
                shot_ext->fd_bypass = 1;
#endif
            } else if (dsInputPortId < MCSC_PORT_MAX) {
                shot_ext->fd_bypass = 1;
            } else {
#ifdef CAPTURE_FD_SYNC_WITH_PREVIEW
                shot_ext->fd_bypass = 1;
                shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = MCSC_PORT_NONE;
#else
                shot_ext->fd_bypass = 0;
                shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port) (dsInputPortId % MCSC_PORT_MAX);
#endif
                shot_ext->shot.ctl.stats.faceDetectMode = fdMode;
            }
        } else {
            shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_OFF;
            shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = MCSC_PORT_NONE;

#ifdef SUPPORT_PIP_LIMITTATION_FD
            /* The sub camera do not support FD in PIP mode */
            if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == true && isBackCamera(getCameraId())) {
                shot_ext->fd_bypass = 1;
            } else
#endif
            {
                if (isEarlyFd) {
                    shot_ext->fd_bypass = 0;
                    shot_ext->shot.ctl.stats.faceDetectMode = fdMode;
                } else if (dsInputPortId < MCSC_PORT_MAX) {
                    shot_ext->fd_bypass = 0;
                    shot_ext->shot.ctl.stats.faceDetectMode = fdMode;
                    shot_ext->shot.uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS] = (enum mcsc_port) dsInputPortId ;
                } else {
                    shot_ext->fd_bypass = 1;
                }
            }
        }
    }
}

void ExynosCamera::m_updateVendorInfo(ExynosCameraRequestSP_sprt_t request, struct camera2_shot_ext *shot_ext, bool isReprocessing)
{
    /* flip control */
#ifdef SUPPORT_PERFRAME_FLIP
    if (isReprocessing) {
        int streamId = HAL_STREAM_ID_JPEG;
        bool flipH = request->getFlipHorizontal(streamId);
        bool flipV = request->getFlipVertical(streamId);

        m_configurations->setModeValue(CONFIGURATION_PERFRAME_FLIP_H_PICTURE, (flipH)?1:0);
        m_configurations->setModeValue(CONFIGURATION_PERFRAME_FLIP_V_PICTURE, (flipV)?1:0);

        enum camera_flip_mode flipMode = CAM_FLIP_MODE_NORMAL;

        if (flipH == true && flipV == true) {
            flipMode = CAM_FLIP_MODE_HORIZONTAL_VERTICAL;
        } else if (flipH == true) {
            flipMode = CAM_FLIP_MODE_HORIZONTAL;
        } else if(flipV == true) {
            flipMode = CAM_FLIP_MODE_VERTICAL;
        } else {
            flipMode = CAM_FLIP_MODE_NORMAL;
        }

        for (int i = 0 ; i < MCSC_PORT_MAX ; i++) {
            shot_ext->mcsc_flip[i] = flipMode;
        }
    }
#endif

}

void ExynosCamera::m_updateEdgeNoiseMode(struct camera2_shot_ext *shot_ext, bool isCaptureFrame)
{
    /*
    // when LSI processing mode cannot support,
    // this code calirates to simple mode.
    enum processing_mode mode = PROCESSING_MODE_FAST;

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return;
    }

    if (isCaptureFrame == true) {
        mode = PROCESSING_MODE_OFF;
    } else {
        mode = PROCESSING_MODE_FAST;
    }

    if (shot_ext->shot.ctl.noise.mode
        == (enum processing_mode) FIMC_IS_METADATA(ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG)) {
        shot_ext->shot.ctl.noise.mode = mode;
    }
    if (shot_ext->shot.ctl.edge.mode
        == (enum processing_mode) FIMC_IS_METADATA(ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG)) {
        shot_ext->shot.ctl.edge.mode = mode;
    }
    */
}

status_t ExynosCamera::m_updateJpegControlInfo(const struct camera2_shot_ext *shot_ext)
{
    status_t ret = NO_ERROR;

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return BAD_VALUE;
    }

    for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
        ret = m_parameters[m_camIdInfo.cameraId[i]]->checkThumbnailSize(
                shot_ext->shot.ctl.jpeg.thumbnailSize[0],
                shot_ext->shot.ctl.jpeg.thumbnailSize[1]);
        if (ret != NO_ERROR) {
            CLOGE("m_camIdInfo.cameraId[%d]=(%d), Failed to checkThumbnailSize. size %dx%d",
                    i,
                    m_camIdInfo.cameraId[i],
                    shot_ext->shot.ctl.jpeg.thumbnailSize[0],
                    shot_ext->shot.ctl.jpeg.thumbnailSize[1]);
        }
    }

    return ret;
}

void ExynosCamera::m_updateSetfile(struct camera2_shot_ext *shot_ext,
                                    bool isCaptureFrame,
                                    ExynosCameraParameters *parameters)
{
    int yuvRange = 0;
    int setfile = 0;

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return;
    }

    if (isCaptureFrame == true) {
        parameters->setSetfileYuvRange();
    }

    parameters->getSetfileYuvRange(isCaptureFrame, &setfile, &yuvRange);
    shot_ext->setfile = setfile;
}

void ExynosCamera::m_setupSlaveCamIdForRequest(ExynosCameraRequestSP_sprt_t request)
{
    int32_t slaveCamIdx, slaveCamId;

    if (request == NULL)
        return;

    slaveCamIdx = m_getCurrentCamIdx(false, SUB_CAM);
    slaveCamId = m_camIdInfo.cameraId[slaveCamIdx];
    request->setSlaveCamId(slaveCamId, slaveCamIdx, false);

    slaveCamIdx = m_getCurrentCamIdx(true, SUB_CAM);
    slaveCamId = m_camIdInfo.cameraId[slaveCamIdx];
    request->setSlaveCamId(slaveCamId, slaveCamIdx, true);

    return;
}

status_t ExynosCamera::m_generateFrame(ExynosCameraFrameFactory *factory, List<ExynosCameraFrameSP_sptr_t> *list,
                                        Mutex *listLock, ExynosCameraFrameSP_dptr_t newFrame,
                                        ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = OK;
    newFrame = NULL;
    uint32_t frameCount = request->getFrameCount();

    CLOGV("(%d)", frameCount);
#if 0 /* Use the same frame count in dual camera, lls capture scenario */
    ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
    if (ret < 0) {
        CLOGE("searchFrameFromList fail");
        return INVALID_OPERATION;
    }
#endif

    if (newFrame == NULL) {
        CLOG_PERFORMANCE(FPS, factory->getCameraId(),
                         factory->getFactoryType(), DURATION,
                         FRAME_CREATE, 0, request->getKey(), nullptr);

        newFrame = factory->createNewFrame(frameCount);
        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            return UNKNOWN_ERROR;
        }

        /* debug */
        if (request != nullptr) {
            newFrame->setRequestKey(request->getKey());
            newFrame->setVendorMeta(request->getVendorMeta());
        }

        listLock->lock();
        list->push_back(newFrame);
        listLock->unlock();
    }

    newFrame->setHasRequest(true);

    CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(), "");

    return ret;
}

status_t ExynosCamera::m_generateFrame(ExynosCameraFrameFactory *factory, List<ExynosCameraFrameSP_sptr_t> *list,
                                       Mutex *listLock, ExynosCameraFrameSP_dptr_t newFrame,
                                       ExynosCameraRequestSP_sprt_t request, bool useJpegFlag)
{
    status_t ret = OK;
    newFrame = NULL;
    uint32_t frameCount = request->getFrameCount();

    CLOGV("(%d)", frameCount);
#if 0 /* Use the same frame count in dual camera, lls capture scenario */
    ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
    if (ret < 0) {
        CLOGE("searchFrameFromList fail");
        return INVALID_OPERATION;
    }
#endif

    if (newFrame == NULL) {
        CLOG_PERFORMANCE(FPS, factory->getCameraId(),
                         factory->getFactoryType(), DURATION,
                         FRAME_CREATE, 0, request->getKey(), nullptr);

        newFrame = factory->createNewFrame(frameCount, useJpegFlag);
        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            return UNKNOWN_ERROR;
        }

        /* debug */
        if (request != nullptr) {
            newFrame->setRequestKey(request->getKey());
            newFrame->setVendorMeta(request->getVendorMeta());
        }

        listLock->lock();
        list->push_back(newFrame);
        listLock->unlock();
    }

    newFrame->setHasRequest(true);

    CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(), "");

    return ret;
}

status_t ExynosCamera::m_setupEntity(uint32_t pipeId,
                                      ExynosCameraFrameSP_sptr_t newFrame,
                                      ExynosCameraBuffer *srcBuf,
                                      ExynosCameraBuffer *dstBuf,
                                      int dstNodeIndex,
                                      int dstPipeId)
{
    status_t ret = OK;
    entity_buffer_state_t entityBufferState;

    CLOGV("(%d)", pipeId);
    /* set SRC buffer */
    ret = newFrame->getSrcBufferState(pipeId, &entityBufferState);
    if (ret < 0) {
        CLOGE("getSrcBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setSrcBuffer(pipeId, newFrame, srcBuf);
        if (ret < 0) {
            CLOGE("m_setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }
    }

    /* set DST buffer */
    ret = newFrame->getDstBufferState(pipeId, &entityBufferState, dstNodeIndex);
    if (ret < 0) {
        CLOGE("getDstBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setDstBuffer(pipeId, newFrame, dstBuf, dstNodeIndex, dstPipeId);
        if (ret < 0) {
            CLOGE("m_setDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }
    }

    ret = newFrame->setEntityState(pipeId, ENTITY_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)",
             pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera::m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrameSP_sptr_t newFrame, ExynosCameraBuffer *buffer)
{
    status_t ret = OK;
    ExynosCameraBuffer srcBuffer;

    CLOGV("(%d)", pipeId);
    if (buffer == NULL) {
        buffer_manager_tag_t bufTag;
        bufTag.pipeId[0] = pipeId;
        bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
        switch (newFrame->getFrameType()) {
        case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION:
        case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
            if (pipeId != PIPE_3AA) // hack
            bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
            break;
        default:
            break;
        }
#endif
        buffer = &srcBuffer;

        ret = m_bufferSupplier->getBuffer(bufTag, buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getBuffer. pipeId %d ret %d",
                    newFrame->getFrameCount(), pipeId, ret);
            return ret;
        }
    }

    /* set buffers */
    ret = newFrame->setSrcBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE("[F%d B%d]Failed to setSrcBuffer. pipeId %d ret %d",
                newFrame->getFrameCount(), buffer->index, pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera::m_setDstBuffer(uint32_t pipeId,
                                       ExynosCameraFrameSP_sptr_t newFrame,
                                       ExynosCameraBuffer *buffer,
                                       int nodeIndex,
                                       __unused int dstPipeId)
{
    status_t ret = OK;

    if (buffer == NULL) {
        CLOGE("[F%d]Buffer is NULL", newFrame->getFrameCount());
        return BAD_VALUE;
    }

    /* set buffers */
    ret = newFrame->setDstBuffer(pipeId, *buffer, nodeIndex);
    if (ret < 0) {
        CLOGE("[F%d B%d]Failed to setDstBuffer. pipeId %d nodeIndex %d ret %d",
                newFrame->getFrameCount(), buffer->index, pipeId, nodeIndex, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera::m_setVOTFBuffer(uint32_t pipeId,
                                       ExynosCameraFrameSP_sptr_t newFrame,
                                       uint32_t srcPipeId,
                                       uint32_t dstPipeId)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer buffer;
    const buffer_manager_tag_t initBufTag;
    buffer_manager_tag_t bufTag;
    int srcPos = -1;
    int dstPos = -1;
    /*
       COUTION: getNodeType is common function, So, use preview frame factory.
       If, getNodeType is seperated, frame factory must seperate.
     */
    ExynosCameraFrameFactory *factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    if (newFrame == NULL) {
        CLOGE("New frame is NULL!!");
        return BAD_VALUE;
    }

    /* Get buffer from buffer supplier */
    ret = m_checkBufferAvailable(srcPipeId);
    if (ret != NO_ERROR) {
        CLOGE("Waiting buffer for Pipe(%d) timeout. ret %d", srcPipeId, ret);
        return INVALID_OPERATION;
    }

    bufTag = initBufTag;
    bufTag.pipeId[0] = srcPipeId;
    bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
    buffer.index = -2;

    ret = m_bufferSupplier->getBuffer(bufTag, &buffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to get Buffer(Pipeid:%d). ret %d",
                newFrame->getFrameCount(), srcPipeId, ret);
        return INVALID_OPERATION;
    }

    if (buffer.index < 0) {
        CLOGE("[F%d]Invalid Buffer index(%d), Pipeid %d. ret %d",
                newFrame->getFrameCount(), buffer.index, srcPipeId, ret);
        return INVALID_OPERATION;
    }

    /* Set Buffer to srcPosition in Frame */
    if (newFrame->getRequest(srcPipeId) == true) {
        srcPos = factory->getNodeType(srcPipeId);
        ret = newFrame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED, srcPos);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setDstBufferState. pipeId %d(%d) pos %d",
                    pipeId, srcPipeId, srcPos);
            newFrame->setRequest(srcPipeId, false);
        } else {
            ret = newFrame->setDstBuffer(pipeId, buffer, srcPos);
            if (ret != NO_ERROR) {
                CLOGE("Failed to setDstBuffer. pipeId %d(%d) pos %d",
                        pipeId, srcPipeId, srcPos);
                newFrame->setRequest(srcPipeId, false);
            }
        }
    }

    /* Set Buffer to dstPosition in Frame */
    if (newFrame->getRequest(srcPipeId) == true) {
        dstPos = factory->getNodeType(dstPipeId);
        ret = newFrame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setDstBufferState. pipeId %d(%d) pos %d",
                    pipeId, srcPipeId, dstPos);
            newFrame->setRequest(srcPipeId, false);
        } else {
            ret = newFrame->setDstBuffer(pipeId, buffer, dstPos);
            if (ret != NO_ERROR) {
                CLOGE("Failed to setDstBuffer. pipeId %d(%d) pos %d",
                        pipeId, srcPipeId, dstPos);
                newFrame->setRequest(srcPipeId, false);
            }
        }
    }

    /* If setBuffer failed, return buffer */
    if (newFrame->getRequest(srcPipeId) == false) {
        ret = m_bufferSupplier->putBuffer(buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to put Buffer(Pipeid:%d). ret %d",
                    newFrame->getFrameCount(), srcPipeId, ret);
            ret = INVALID_OPERATION;
        }
    }

    return ret;
}

status_t ExynosCamera::m_setHWFCBuffer(uint32_t pipeId,
                                       ExynosCameraFrameSP_sptr_t newFrame,
                                       uint32_t srcPipeId,
                                       uint32_t dstPipeId)
{
    /* This function is same logic with m_setVOTFBuffer */
    return m_setVOTFBuffer(pipeId, newFrame, srcPipeId, dstPipeId);
}

status_t ExynosCamera::m_createIonAllocator(ExynosCameraIonAllocator **allocator)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    do {
        retry++;
        CLOGI("try(%d) to create IonAllocator", retry);
        *allocator = new ExynosCameraIonAllocator();
        ret = (*allocator)->init(false);
        if (ret < 0)
            CLOGE("create IonAllocator fail (retryCount=%d)", retry);
        else {
            CLOGD("m_createIonAllocator success (allocator=%p)", *allocator);
            break;
        }
    } while ((ret < 0) && (retry < 3));

    if ((ret < 0) && (retry >=3)) {
        CLOGE("create IonAllocator fail (retryCount=%d)", retry);
        ret = INVALID_OPERATION;
    }

    return ret;
}

#ifdef USES_VPL_PRELOAD
bool ExynosCamera::m_vplPreloadThreadFunc()
{
    char vplLibPath[] = VPL_LIBRARY_PATH;

    CLOGD("");

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    m_vplDl = dlopen(vplLibPath, RTLD_NOW);
    if (m_vplDl == NULL) {
        CLOGE("Failed to open VPL Library. path %s, err %s", vplLibPath, dlerror());
        return false;
    }

    m_vplPreloadInitStatus = true;
    m_vplPreLoad = (void(*)(void)) dlsym(m_vplDl, "vplPreLoad");
    if (dlerror() != NULL && m_vplPreLoad == NULL) {
        CLOGE("Failed to dlsym m_vplPreLoad : err %s", dlerror());
        goto err_exit;
    }


    m_vplUnload = (void(*)(void)) dlsym(m_vplDl, "vplUnload");
    if (dlerror() != NULL && m_vplUnload == NULL) {
        CLOGE("Failed to dlsym m_vplUnLoad : err %s", dlerror());
        // goto err_exit;
    }

    (*m_vplPreLoad)();

err_exit:

    return false;
}
#endif

bool ExynosCamera::m_captureThreadFunc()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    ExynosCameraRequestSP_sprt_t request = NULL;
    ExynosCameraFrameEntity *entity = NULL;
    uint32_t pipeId = 0;
    int retryCount = 1;
    frame_handle_components_t components;

    ret = m_captureQ->waitAndPopProcessQ(&frame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGV("Wait timeout");
        } else {
            CLOGE("Failed to wait&pop captureQ, ret %d", ret);
            /* TODO: doing exception handling */
        }
        goto CLEAN;
    } else if (frame == NULL) {
        CLOGE("frame is NULL!!");
        goto FUNC_EXIT;
    }

    entity = frame->getFirstEntityNotComplete();
    pipeId = (entity == NULL) ? -1 : entity->getPipeId();

    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

    CFLOGI(frame, "[P%d] sessionId:%d", pipeId, m_getCameraSessionId(frame));

    m_captureStreamThread->run(PRIORITY_DEFAULT);

    if (frame->getStreamRequested(STREAM_TYPE_CAPTURE) == false
#ifdef USE_DUAL_CAMERA
        && frame->getFrameType() != FRAME_TYPE_REPROCESSING_DUAL_SLAVE
#endif
        && frame->getStreamRequested(STREAM_TYPE_RAW) == false) {
        ret = frame->setEntityState(pipeId, ENTITY_STATE_FRAME_DONE);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to setEntityState(FRAME_DONE). pipeId %d ret %d",
                    frame->getFrameCount(), pipeId, ret);
            /* continue */
        }

        m_yuvCaptureDoneQ->pushProcessQ(&frame);
    } else {
        if (components.reprocessingFactory == NULL) {
            CLOGE("frame factory is NULL");
            goto FUNC_EXIT;
        }

        while (components.reprocessingFactory->isRunning() == false) {
            CLOGD("[F%d]Wait to start reprocessing stream", frame->getFrameCount());

            if (m_getState() == EXYNOS_CAMERA_STATE_FLUSH) {
                CLOGD("[F%d]Flush is in progress.", frame->getFrameCount());
                goto FUNC_EXIT;
            }

            usleep(5000);
        }

#ifdef USE_HW_RAW_REVERSE_PROCESSING
        /* To let the 3AA reprocessing pipe twice */
        if (components.parameters->isUseRawReverseReprocessing() == true &&
                frame->getStreamRequested(STREAM_TYPE_RAW) == true) {
            frame->backupRequest(REQUEST_BACKUP_MODE_DNG);

            if (frame->getRequest(PIPE_3AC_REPROCESSING) == true) {
                bool twiceRun = true;

                if (!twiceRun) {
                    /* just only one loop for raw */
                    frame->reverseExceptForSpecificReq(REQUEST_BACKUP_MODE_DNG, PIPE_3AC_REPROCESSING, true);

                    /* for raw capture setfile index */
                    frame->setSetfile(ISS_SUB_SCENARIO_STILL_CAPTURE_DNG_REPROCESSING);
                } else  {
                    ////////////////////////////////////////////////
                    // make DNG, then, make JPEG
                    frame->reverseExceptForSpecificReq(REQUEST_BACKUP_MODE_DNG, PIPE_3AC_REPROCESSING, true);

                    ////////////////////////////////////////////////
                    // save the oldSetfile
                    m_oldSetfile = frame->getSetfile();

                    frame->setSetfile(ISS_SUB_SCENARIO_STILL_CAPTURE_DNG_REPROCESSING);

                    ////////////////////////////////////////////////
                }

                CLOGD("[F%d] This frame will be processed %d th time", frame->getFrameCount(), twiceRun ? 2 : 1);
            }
        }
#endif
        CFLOGI(frame, "[P%d] start reprocessingFactory(CAM:%d,TYPE:%d)",
                pipeId, components.reprocessingFactory->getCameraId(),
                components.reprocessingFactory->getFactoryType());

        components.reprocessingFactory->pushFrameToPipe(frame, pipeId);

        /* When enabled SCC capture or pureBayerReprocessing, we need to start bayer pipe thread */
        if (components.parameters->isReprocessing() == true)
            components.reprocessingFactory->startThread(pipeId);

        /* Wait reprocesisng done */
        do {
            ret = m_reprocessingDoneQ->waitAndPopProcessQ(&frame);
        } while (ret == TIMED_OUT && retryCount-- > 0);

        if (ret != NO_ERROR) {
            CLOGW("[F%d]Failed to waitAndPopProcessQ to reprocessingDoneQ. ret %d",
                    frame->getFrameCount(), ret);
        }
        CLOGI("[F%d]Wait reprocessing done", frame->getFrameCount());
    }

    /* Thread can exit only if timeout or error occured from inputQ (at CLEAN: label) */
    return true;

CLEAN:
    if (frame != NULL && m_getState() != EXYNOS_CAMERA_STATE_FLUSH) {
        ret = m_removeFrameFromList(&m_captureProcessList, &m_captureProcessLock, frame);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to removeFrameFromList for captureProcessList. ret %d",
                    frame->getFrameCount(), ret);
        }

        frame->printEntity();
        CLOGD("[F%d]Delete frame from captureProcessList", frame->getFrameCount());
        frame = NULL;
    }

FUNC_EXIT:
    {
        Mutex::Autolock l(m_captureProcessLock);
        if (m_captureQ->getSizeOfProcessQ() > 0 || m_captureProcessList.size() > 0) {
            CLOGD("captureQSize(%d) captureProcessSize(%d)",
                    m_captureQ->getSizeOfProcessQ(), m_captureProcessList.size());
            return true;
        } else {
#ifdef USE_DUAL_CAMERA
            if (m_prepareBokehAnchorCaptureQ != NULL && m_prepareBokehAnchorCaptureQ->getSizeOfProcessQ() > 0) {
                CLOGD("prepareBokehAnchorCaptureQSize(%d)",
                        m_prepareBokehAnchorCaptureQ->getSizeOfProcessQ());
                return true;
            } else
#endif
            {
                return false;
            }
        }
    }
}

status_t ExynosCamera::m_getBayerServiceBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                ExynosCameraBuffer *buffer,
                                                ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t bayerFrame = NULL;
    int retryCount = 30;
    frame_handle_components_t components;

    if (frame == NULL) {
        CLOGE("frame is NULL");
        /* TODO: doing exception handling */
        return BAD_VALUE;
    }

    m_getFrameHandleComponentsWrapper(frame, &components);
    ExynosCameraFrameFactory *factory = components.previewFactory;

    int dstPos = factory->getNodeType(PIPE_VC0);
    int pipeId = -1;

    if (frame->getStreamRequested(STREAM_TYPE_ZSL_YUV_INPUT) == true) {
        pipeId = frame->getFirstEntity()->getPipeId();
    } else
    if (components.parameters->getUsePureBayerReprocessing() == true) {
        pipeId = PIPE_3AA_REPROCESSING;
    } else {
        pipeId = PIPE_ISP_REPROCESSING;
    }

    if (frame->getStreamRequested(STREAM_TYPE_RAW)) {
        m_captureZslSelector->setWaitTime(200000000);
        bayerFrame = m_captureZslSelector->selectDynamicFrames(1, retryCount);
        if (bayerFrame == NULL) {
            CLOGE("bayerFrame is NULL");
            return INVALID_OPERATION;
        }

        if (bayerFrame->getMetaDataEnable() == true)
            CLOGI("Frame metadata is updated. frameCount %d",
                     bayerFrame->getFrameCount());

        ret = bayerFrame->getDstBuffer(m_getBayerPipeId(), buffer, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getDstBuffer. pipeId %d ret %d",
                    m_getBayerPipeId(), ret);
            return ret;
        }

        struct camera2_shot_ext src_ext = {0,};
        struct camera2_shot_ext dst_ext = {0,};
        struct camera2_shot_ext *buffer_ext = NULL;
        bayerFrame->getMetaData(&src_ext);
        frame->getMetaData(&dst_ext);
        buffer_ext = (struct camera2_shot_ext*)buffer->addr[buffer->getMetaPlaneIndex()];
        updateMetadataUser(&src_ext, &dst_ext, buffer_ext);
        frame->setMetaData(&dst_ext);


    } else {
        if (request != NULL) {
            camera3_stream_buffer_t *stream_buffer = request->getInputBuffer();
            buffer_handle_t *handle = stream_buffer->buffer;
            buffer_manager_tag_t bufTag;

            CLOGI("[R%d F%d H%p]Getting Bayer from getRunningRequest",
                    request->getKey(), frame->getFrameCount(), handle);

            bufTag.pipeId[0] = pipeId;
            bufTag.managerType = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
            buffer->handle[0] = handle;
            buffer->acquireFence[0] = stream_buffer->acquire_fence;
            buffer->releaseFence[0] = stream_buffer->release_fence;

            ret = m_bufferSupplier->getBuffer(bufTag, buffer);

            ret = request->setAcquireFenceDone(handle, (buffer->acquireFence[0] == -1) ? true : false);
            if (ret != NO_ERROR) {
                CLOGE("[R%d F%d B%d]Failed to setAcquireFenceDone. ret %d",
                        request->getKey(), frame->getFrameCount(), buffer->index, ret);
                return ret;
            }

            CLOGI("[R%d F%d B%d H%p]Service bayer selected. ret %d",
                    request->getKey(), frame->getFrameCount(), buffer->index, handle, ret);
        } else {
            CLOGE("request if NULL(fcount:%d)", frame->getFrameCount());
            ret = BAD_VALUE;
        }
    }

    return ret;
}

bool ExynosCamera::m_previewStreamGMVPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_GMV]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
            goto retry_thread_loop;
        } else if (ret != NO_ERROR) {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
            goto retry_thread_loop;
        } else if (newFrame == NULL) {
            CLOGE("Frame is NULL");
            goto retry_thread_loop;
        }
    }

    return m_previewStreamFunc(newFrame, PIPE_GMV);

retry_thread_loop:
    if (m_pipeFrameDoneQ[PIPE_GMV]->getSizeOfProcessQ() > 0) {
        return true;
    } else {
        return false;
    }
}

#ifdef USE_VRA_FD
bool ExynosCamera::m_previewStreamVRAPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_VRA]->waitAndPopProcessQ(&newFrame);
    if (ret != NO_ERROR) {
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        goto retry_thread_loop;
    } else if (newFrame == NULL) {
        CLOGE("Frame is NULL");
        goto retry_thread_loop;
    }

    return m_previewStreamFunc(newFrame, PIPE_VRA);

retry_thread_loop:
    if (m_pipeFrameDoneQ[PIPE_VRA]->getSizeOfProcessQ() > 0) {
        return true;
    } else {
        return false;
    }
}
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
bool ExynosCamera::m_previewStreamNFDPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_NFD]->waitAndPopProcessQ(&newFrame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        goto retry_thread_loop;
    } else if (newFrame == NULL) {
        CLOGE("Frame is NULL");
        goto retry_thread_loop;
    }

    return m_previewStreamFunc(newFrame, PIPE_NFD);

retry_thread_loop:
    if (m_pipeFrameDoneQ[PIPE_NFD]->getSizeOfProcessQ() > 0) {
        return true;
    } else {
        return false;
    }
}
#endif

#ifdef SUPPORT_HFD
bool ExynosCamera::m_previewStreamHFDPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_HFD]->waitAndPopProcessQ(&newFrame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        goto retry_thread_loop;
    } else if (newFrame == NULL) {
        CLOGE("Frame is NULL");
        goto retry_thread_loop;
    }

    return m_previewStreamFunc(newFrame, PIPE_HFD);

retry_thread_loop:
    if (m_pipeFrameDoneQ[PIPE_HFD]->getSizeOfProcessQ() > 0) {
        return true;
    } else {
        return false;
    }
}
#endif

#ifdef USE_DUAL_CAMERA
bool ExynosCamera::m_previewStreamBayerSyncPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_BAYER_SYNC]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            enum DUAL_OPERATION_MODE dualOperationMode = m_configurations->getDualOperationMode();
            if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
                CLOGW("wait timeout");
            }
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_BAYER_SYNC);
}

bool ExynosCamera::m_previewStreamSyncPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_SYNC]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            enum DUAL_OPERATION_MODE dualOperationMode = m_configurations->getDualOperationMode();
            if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
                CLOGW("wait timeout");
            }
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_SYNC);
}

bool ExynosCamera::m_previewStreamFusionPipeThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_FUSION]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            enum DUAL_OPERATION_MODE dualOperationMode = m_configurations->getDualOperationMode();
            if (dualOperationMode == DUAL_OPERATION_MODE_SYNC) {
                CLOGW("wait timeout");
            }
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_FUSION);
}
#endif

status_t ExynosCamera::m_allocBuffers(
        const buffer_manager_tag_t tag,
        const buffer_manager_configuration_t info)
{
    status_t ret = NO_ERROR;

    ret = m_bufferSupplier->setInfo(tag, info);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setInfo. ret %d", ret);

        return ret;
    }

    return m_bufferSupplier->alloc(tag);
}

status_t ExynosCamera::m_getBuffer(ExynosCameraFrameSP_dptr_t frame,
                                   int pipeId,
                                   int dstPipeId,
                                   int dstPos,
                                   buffer_manager_type_t bufferManagerType)
{
    status_t ret = NO_ERROR;


    ////////////////////////////////////////////////
    // variable init
    buffer_manager_tag_t bufTag;
    bufTag.pipeId[0]   = dstPipeId;
    bufTag.managerType = bufferManagerType;

    ExynosCameraBuffer buffer;
    buffer.index = -2;

    ////////////////////////////////////////////////
    // get buffer
    ret = m_bufferSupplier->getBuffer(bufTag, &buffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]m_bufferSupplier(%s)->getBuffer(pipeId(%d)) fail",
            frame->getFrameCount(), m_bufferSupplier->getName(bufTag), pipeId);
        goto done;
    }

    if (buffer.index < 0) {
        CLOGW("[F%d B%d]m_bufferSupplier(%s)->getBuffer(pipeId(%d)) return Invalid buffer index. Skip to pushFrame",
            frame->getFrameCount(), buffer.index, m_bufferSupplier->getName(bufTag), pipeId);
        goto done;
    }

    CLOGV("[F%d B%d]Use %s Buffer",
        frame->getFrameCount(), buffer.index, m_bufferSupplier->getName(bufTag));

    ////////////////////////////////////////////////
    // set buffer on pipe
    ret = frame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]frame->setDstBufferState(pipeId(%d), dstPos(%d)) fail",
            frame->getFrameCount(), buffer.index, pipeId, dstPos);

        goto done;
    }

    ret = frame->setDstBuffer(pipeId, buffer, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]frame->setDstBuffer(pipeId(%d), buffer.index(%d), dstPos(%d)) fail",
            frame->getFrameCount(), buffer.index, pipeId, buffer.index, dstPos);

        goto done;
    }

done:
    ////////////////////////////////////////////////
    // error handling. just off it.
    if (ret != NO_ERROR) {
        if (0 < buffer.index) {
            int ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d]m_bufferSupplier(%s)->putBuffer(pipeId(%d)) fail",
                    frame->getFrameCount(), m_bufferSupplier->getName(bufTag), pipeId);
            }
        }

        frame->setRequest(bufTag.pipeId[0], false);
    }

    ////////////////////////////////////////////////

    return ret;

}

status_t ExynosCamera::m_putSrcBuffer(ExynosCameraFrameSP_sptr_t frame, int pipeId, int dstPos)
{
    status_t ret = NO_ERROR;

    ExynosCameraBuffer buffer;

    ret = frame->getSrcBuffer(pipeId, &buffer, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getSrcBuffer. pipeId(%d), dstPos(%d)",
            frame->getFrameCount(), pipeId, dstPos);
    } else {
        ret = m_bufferSupplier->putBuffer(buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to putBuffer. ret %d",
                frame->getFrameCount(), buffer.index, ret);
        }
    }

    return ret;
}

status_t ExynosCamera::m_putDstBuffer(ExynosCameraFrameSP_sptr_t frame, int pipeId, int dstPos)
{
    status_t ret = NO_ERROR;

    ExynosCameraBuffer buffer;

    ret = frame->getDstBuffer(pipeId, &buffer, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getDstBuffer. pipeId(%d), dstPos(%d)",
            frame->getFrameCount(), pipeId, dstPos);
    } else {
        ret = m_bufferSupplier->putBuffer(buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to putBuffer. ret %d",
                frame->getFrameCount(), buffer.index, ret);
        }
    }

    return ret;
}


bool ExynosCamera::m_setBuffersThreadFunc(void)
{
    CLOGI("");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int ret;

    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, SUB_CUMULATIVE_CNT, SET_BUFFER_THREAD_START, 0);

    if (m_configurations->getMode(CONFIGURATION_VISION_MODE) == false) {
#ifdef ADAPTIVE_RESERVED_MEMORY
        ret = m_allocAdaptiveNormalBuffers();
#else
        ret = m_setBuffers();
#endif
    } else {
        ret = m_setVisionBuffers();
    }

    if (ret < 0) {
        CLOGE("m_setBuffers failed");
        m_bufferSupplier->deinit();
        return false;
    }

    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, SUB_CUMULATIVE_CNT, SET_BUFFER_THREAD_END, 0);

    return false;
}

bool ExynosCamera::m_deinitBufferSupplierThreadFunc(void)
{
    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, SUB_CUMULATIVE_CNT, BUFFER_SUPPLIER_DEINIT_START, 0);

    m_resourceManager->deInitBufferSupplier(&m_bufferSupplier, &m_ionAllocator);

    TIME_LOGGER_UPDATE(m_cameraId, 0, 0, SUB_CUMULATIVE_CNT, BUFFER_SUPPLIER_DEINIT_END, 0);
    return false;
}

uint32_t ExynosCamera::m_getBayerPipeId(void)
{
    uint32_t pipeId = 0;

    if (m_parameters[m_cameraId]->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    return pipeId;
}

uint32_t ExynosCamera::m_getSensorGyroPipeId(void)
{
    return PIPE_VC3;
}

status_t ExynosCamera::m_pushServiceRequest(camera3_capture_request *request_in,
                                                        ExynosCameraRequestSP_dptr_t req, bool skipRequest)
{
    status_t ret = OK;

    CLOGV("m_pushServiceRequest frameCnt(%d)", request_in->frame_number);

    req = m_requestMgr->registerToServiceList(request_in, skipRequest);
    if (req == NULL) {
        CLOGE("registerToServiceList failed, FrameNumber = [%d]", request_in->frame_number);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_popServiceRequest(ExynosCameraRequestSP_dptr_t request)
{
    status_t ret = OK;

    CLOGV("m_popServiceRequest ");

    request = m_requestMgr->eraseFromServiceList();
    if (request == NULL) {
        CLOGE("createRequest failed ");
        ret = INVALID_OPERATION;
    }
    m_captureResultDoneCondition.signal();
    return ret;
}

status_t ExynosCamera::m_pushRunningRequest(ExynosCameraRequestSP_dptr_t request_in)
{
    status_t ret = NO_ERROR;

    CLOGV("[R%d F%d] Push Request in runningRequest",
            request_in->getKey(), request_in->getFrameCount());

    ret = m_requestMgr->registerToRunningList(request_in);
    if (ret != NO_ERROR) {
        CLOGE("[R%d] registerToRunningList is failed", request_in->getKey());
        return ret;
    }

    return ret;
}

status_t ExynosCamera::m_setFactoryAddr(ExynosCameraRequestSP_dptr_t request)
{
    status_t ret = NO_ERROR;
    FrameFactoryList factorylist;
    FrameFactoryListIterator factorylistIter;
    ExynosCameraFrameFactory *factory = NULL;
    ExynosCameraFrameFactory *factoryAddr[FRAME_FACTORY_TYPE_MAX] = {NULL, };

    factorylist.clear();
    request->getFrameFactoryList(&factorylist);
    for (factorylistIter = factorylist.begin(); factorylistIter != factorylist.end(); ) {
        factory = *factorylistIter;

        if (factory->getFactoryType() < FRAME_FACTORY_TYPE_MAX && factoryAddr[factory->getFactoryType()] == NULL) {
            factoryAddr[factory->getFactoryType()] = factory;
            CLOGV("list Factory(%p) Type(%d)", factory, factory->getFactoryType());
        }

        factorylistIter++;
    }

    request->setFactoryAddrList(factoryAddr);

    return ret;
}

bool ExynosCamera::m_reprocessingFrameFactoryStartThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrameFactory *factory = NULL;
    ExynosCameraFrameFactory *factoryList[FRAME_FACTORY_TYPE_MAX] = {NULL, };

    if (m_getState() == EXYNOS_CAMERA_STATE_FLUSH) {
        CLOGD("Flush is in progress.");
        return false;
    }

    for (int i = FRAME_FACTORY_TYPE_REPROCESSING; i < FRAME_FACTORY_TYPE_REPROCESSING_MAX; i++) {
        factoryList[i] = m_frameFactory[i];
    }

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_dualFrameFactoryStartThread != NULL) {
        /* reprocessing instance must be create after creation of preview instance */
        CLOGI("m_dualFrameFactoryStartThread join E");
        m_dualFrameFactoryStartThread->join();
        CLOGI("m_dualFrameFactoryStartThread join X");
    }
#endif

#ifdef SUPPORT_REMOSAIC_CAPTURE
    factoryList[FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING] = m_frameFactory[FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING];
#endif //SUPPORT_REMOSAIC_CAPTURE

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        factory = factoryList[i];
        if (factory == NULL)
            continue;

        if (factory->isCreated() == false) {
            CLOGE("Reprocessing FrameFactory is NOT created!");
            return false;
        } else if (factory->isRunning() == true) {
            CLOGW("Reprocessing FrameFactory is already running");
            return false;
        }

        /* Set buffer manager */
        ret = m_setupReprocessingPipeline(factory);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setupReprocessingPipeline. ret %d", ret);
            return false;
        }

        ret = factory->initPipes();
        if (ret < 0) {
            CLOGE("Failed to initPipes. ret %d", ret);
            return false;
        }

#ifdef SUPPORT_REMOSAIC_CAPTURE
        /*
         * when remosaic, it must do the stream-on( == startPipes), during capture-size setting.
         * the stream-on(== startPipes) will happpen in m_sensorModeSwitchThreadFunc();
         */
        if (i != FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING)
#endif
        {
            ret = m_startReprocessingFrameFactory(factory);
            if (ret < 0) {
                CLOGE("Failed to startReprocessingFrameFactory");
                /* TODO: Need release buffers and error exit */
                return false;
            }
        }
    }

    return false;
}

status_t ExynosCamera::m_startReprocessingFrameFactory(ExynosCameraFrameFactory *factory)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = 0;

    CLOGD("- IN -");

    ret = factory->preparePipes();
    if (ret < 0) {
        CLOGE("m_reprocessingFrameFactory preparePipe fail");
        return ret;
    }

    /* stream on pipes */
    ret = factory->startPipes();
    if (ret < 0) {
        CLOGE("m_reprocessingFrameFactory startPipe fail");
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_stopReprocessingFrameFactory(ExynosCameraFrameFactory *factory)
{
    CLOGI("");
    status_t ret = 0;

    if (factory != NULL && factory->isRunning() == true) {
        ret = factory->stopPipes();
        if (ret < 0) {
            CLOGE("m_reprocessingFrameFactory->stopPipe() fail");
        }
    }

    CLOGD("clear m_captureProcessList(Picture) Frame list");
    ret = m_clearList(&m_captureProcessList, &m_captureProcessLock);
    if (ret < 0) {
        CLOGE("m_clearList fail");
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_checkBufferAvailable(uint32_t pipeId, int managerType)
{
    status_t ret = TIMED_OUT;
    buffer_manager_tag_t bufTag;
    int retry = 0;

    bufTag.pipeId[0] = pipeId;
    bufTag.managerType = managerType;

    do {
        ret = -1;
        retry++;
        if (m_bufferSupplier->getNumOfAvailableBuffer(bufTag) > 0) {
            ret = OK;
        } else {
            /* wait available ISP buffer */
            usleep(WAITING_TIME);
        }
        if (retry % 10 == 0)
            CLOGW("retry(%d) setupEntity for pipeId(%d)", retry, pipeId);
    } while(ret < 0 && retry < (TOTAL_WAITING_TIME/WAITING_TIME));

    return ret;
}

bool ExynosCamera::m_checkValidBayerBufferSize(struct camera2_stream *stream, ExynosCameraFrameSP_sptr_t frame, bool flagForceRecovery)
{
    status_t ret = NO_ERROR;
    ExynosRect bayerRect;
    ExynosRect reprocessingInputRect;
    struct camera2_node_group nodeGroupInfo;
    frame_handle_components_t components;
    int perframeIndex = -1;

    if (stream == NULL) {
        CLOGE("[F%d]stream is NULL", frame->getFrameCount());
        return false;
    }

    if (frame == NULL) {
        CLOGE("frame is NULL");
        /* TODO: doing exception handling */
        return false;
    }

#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (frame->getFrameType() == FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION) {
        CLOGW("[F%d] Skip. frame is FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION", frame->getFrameCount());
        return true;
    }
#endif //SUPPORT_REMOSAIC_CAPTURE

    m_getFrameHandleComponentsWrapper(frame, &components);

    bool usePureBayerReprocessing = components.parameters->getUsePureBayerReprocessing();

    if (usePureBayerReprocessing == true) {
        CLOGV("[F%d]Skip. Reprocessing mode is PURE bayer", frame->getFrameCount());
        return true;
    } else {
        perframeIndex = PERFRAME_INFO_DIRTY_REPROCESSING_ISP;
    }

    /* Get per-frame size info from stream.
     * Set by driver.
     */
    bayerRect.x = stream->output_crop_region[0];
    bayerRect.y = stream->output_crop_region[1];
    bayerRect.w = stream->output_crop_region[2];
    bayerRect.h = stream->output_crop_region[3];

    /* Get per-frame size info from frame.
     * Set by FrameFactory.
     */
    ret = frame->getNodeGroupInfo(&nodeGroupInfo, perframeIndex);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getNodeGroupInfo. perframeIndex %d ret %d",
                frame->getFrameCount(), perframeIndex, ret);
        return true;
    }

    reprocessingInputRect.x = nodeGroupInfo.leader.input.cropRegion[0];
    reprocessingInputRect.y = nodeGroupInfo.leader.input.cropRegion[1];
    reprocessingInputRect.w = nodeGroupInfo.leader.input.cropRegion[2];
    reprocessingInputRect.h = nodeGroupInfo.leader.input.cropRegion[3];

    /* Compare per-frame size. */
    if (bayerRect.w != reprocessingInputRect.w
        || bayerRect.h != reprocessingInputRect.h) {
        CLOGW("[F%d(%d)]Bayer size mismatch. Bayer %dx%d Control %dx%d, forceRecovery(%d)",
                frame->getFrameCount(),
                stream->fcount,
                bayerRect.w, bayerRect.h,
                reprocessingInputRect.w, reprocessingInputRect.h,
                flagForceRecovery);

        if (flagForceRecovery) {
            /* recovery based on this node's real perframe size */
            nodeGroupInfo.leader.input.cropRegion[0] = bayerRect.x;
            nodeGroupInfo.leader.input.cropRegion[1] = bayerRect.y;
            nodeGroupInfo.leader.input.cropRegion[2] = bayerRect.w;
            nodeGroupInfo.leader.input.cropRegion[3] = bayerRect.h;

            /* also forcely set the input crop */
            for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
                if (nodeGroupInfo.capture[i].vid) {
                    nodeGroupInfo.capture[i].input.cropRegion[0] = 0;
                    nodeGroupInfo.capture[i].input.cropRegion[1] = 0;
                    nodeGroupInfo.capture[i].input.cropRegion[2] = bayerRect.w;
                    nodeGroupInfo.capture[i].input.cropRegion[3] = bayerRect.h;
                }
            }

            ret = frame->storeNodeGroupInfo(&nodeGroupInfo, perframeIndex);
            if (ret != NO_ERROR) {
                CLOGE("[F%d]Failed to getNodeGroupInfo. perframeIndex %d ret %d",
                        frame->getFrameCount(), perframeIndex, ret);
            }
        } else {
            return false;
        }
    }

    return true;
}

bool ExynosCamera::m_startPictureBufferThreadFunc(void)
{
    CLOGI("");
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int ret = 0;

    if (m_parameters[m_cameraId]->isReprocessing() == true ||
            m_parameters[m_cameraId]->isUseVideoHQISP() == true) {
#ifdef ADAPTIVE_RESERVED_MEMORY
        ret = m_allocAdaptiveReprocessingBuffers();
#else
        ret = m_setReprocessingBuffer();
#endif
        if (ret < 0) {
            CLOGE("m_setReprocessing Buffer fail");
            m_bufferSupplier->deinit();
            return false;
        }
    }

    return false;
}

status_t ExynosCamera::m_generateInternalFrame(ExynosCameraFrameFactory *factory, List<ExynosCameraFrameSP_sptr_t> *list,
                                                Mutex *listLock, ExynosCameraFrameSP_dptr_t newFrame, frame_type_t frameType,
                                                ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = OK;
    newFrame = NULL;
    uint32_t frameCount = 0;
    bool hasReques = false;

    if (request != NULL) {
        frameCount = request->getFrameCount();
        hasReques = true;
    } else {
        m_frameCountLock.lock();
        frameCount = m_internalFrameCount++;
        m_frameCountLock.unlock();
        hasReques = false;
    }

    CLOGV("(%d)", frameCount);
#if 0 /* Use the same frame count in dual camera, lls capture scenario */
    ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
    if (ret < 0) {
        CLOGE("searchFrameFromList fail");
        return INVALID_OPERATION;
    }
#endif

    if (newFrame == NULL) {
        CLOG_PERFORMANCE(FPS, factory->getCameraId(),
                         factory->getFactoryType(), DURATION,
                         FRAME_CREATE, 0, request == nullptr ? 0 : request->getKey(), nullptr);

        newFrame = factory->createNewFrame(frameCount);
        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            return UNKNOWN_ERROR;
        }

        /* debug */
        if (request != nullptr) {
            newFrame->setRequestKey(request->getKey());
            newFrame->setVendorMeta(request->getVendorMeta());
        }

        listLock->lock();
        list->push_back(newFrame);
        listLock->unlock();
    }

    /* Set frame type into FRAME_TYPE_INTERNAL */
    ret = newFrame->setFrameInfo(m_configurations, factory->getParameters(), frameCount, frameType);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setFrameInfo with INTERNAL. frameCount %d", frameCount);
        return ret;
    }

    newFrame->setHasRequest(hasReques);

#ifdef WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA
    /*
     * If the capture frame generation is delayed by WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA,
     * the partial result of the internal frame may be updated before SlaveCamId is set.
     * In case of WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA,
     * set slave camera ID at m_generateInternalFrame because SlaveCamId is needed when updating partial result.
     */
    if (hasReques == true) {
        m_setupSlaveCamIdForRequest(request);
    }
#endif

    CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(), "");

    return ret;
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCamera::m_generateTransitionFrame(ExynosCameraFrameFactory *factory, List<ExynosCameraFrameSP_sptr_t> *list,
                                                Mutex *listLock, ExynosCameraFrameSP_dptr_t newFrame, frame_type_t frameType,
                                                ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;
    newFrame = NULL;
    uint32_t frameCount = m_internalFrameCount;
    bool hasReques = false;

    m_frameCountLock.lock();
    frameCount = m_internalFrameCount++;
    m_frameCountLock.unlock();

    CLOGV("(%d)", frameCount);
#if 0 /* Use the same frame count in dual camera, lls capture scenario */
    ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
    if (ret < 0) {
        CLOGE("searchFrameFromList fail");
        return INVALID_OPERATION;
    }
#endif

    if (newFrame == NULL) {
        CLOG_PERFORMANCE(FPS, factory->getCameraId(),
                         factory->getFactoryType(), DURATION,
                         FRAME_CREATE, 0, 0, nullptr);

        newFrame = factory->createNewFrame(frameCount);
        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            return UNKNOWN_ERROR;
        }

        if (request != nullptr) {
            newFrame->setVendorMeta(request->getVendorMeta());
        }

        listLock->lock();
        list->push_back(newFrame);
        listLock->unlock();
    }

    /* Set frame type into FRAME_TYPE_TRANSITION */
    ret = newFrame->setFrameInfo(m_configurations, factory->getParameters(), frameCount, frameType);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setFrameInfo with INTERNAL. frameCount %d", frameCount);
        return ret;
    }

    newFrame->setHasRequest(hasReques);

#ifdef WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA
    /*
     * If the capture frame generation is delayed by WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA,
     * the partial result of the internal frame may be updated before SlaveCamId is set.
     * In case of WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA,
     * set slave camera ID at m_generateInternalFrame because SlaveCamId is needed when updating partial result.
     */
    if (hasReques == true) {
        m_setupSlaveCamIdForRequest(request);
    }
#endif

    CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(), "");

    return ret;
}

int32_t ExynosCamera::m_getCameraIdFromOperationSensor(enum DUAL_OPERATION_SENSORS id,
                                                       int32_t *camId0, int32_t *camId1)
{
    return m_configurations->getCameraIdFromOperationSensor(id, camId0, camId1);
}

status_t ExynosCamera::m_modulateDualOperationMode(enum DUAL_OPERATION_MODE &newOperationMode,
                                        enum DUAL_OPERATION_MODE oldOperationMode,
                                        enum DUAL_OPERATION_SENSORS &newOperationSensor,
                                        enum DUAL_OPERATION_SENSORS oldOperationSensor)
{
    status_t ret = NO_ERROR;
    int masterId = -1, slaveId = -1;
    int oldMasterId = -1, oldSlaveId = -1;

    if ((oldOperationMode == newOperationMode)
        && (newOperationSensor == oldOperationSensor)) {
        goto DUAL_OPERATION_MODE_END;
    }


    //if oldOperationMode is DUAL_OPERATION_MODE_NONE, any new mode is OK.
    if (oldOperationMode == DUAL_OPERATION_MODE_NONE) {
        goto DUAL_OPERATION_MODE_END;
    }

    //TODO: need to consider the current STANDBY_STATES
#ifdef USE_MASTER_SLAVE_SWITCHING
    /*
     * Allowed cases :
     * Slave_mode (SWIDE) <-> Sync_mode (WIDE_SWIDE) <-> Master_mode (WIDE) <-> Sync_Mode (WIDE_TELE) <-> Slave_mode (TELE)
                              Master_mode (WIDE)                                Master_mode (WIDE)
     */

    /*
     * if oldOperationMode is DUAL_OPERATION_MODE_MASTER,
     * 1) Any SYNC mode is OK. DUAL_OPERATION_SENSOR_BACK_MAIN_SUB or DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2.
     * 2) if newOperationMode is SLAVE, set the corresponding SYNC mode (M + S).
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_MASTER) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_SLAVE:
        case DUAL_OPERATION_MODE_MASTER:
        case DUAL_OPERATION_MODE_SYNC:
        default:
            goto DUAL_OPERATION_MODE_END;
            break;
        }
     }

     /*
     * if oldOperationMode is DUAL_OPERATION_MODE_SYNC,
     * 1) new mode : MASTER  (ok)
     * 2) new mode : SYNC (DIFFERENT_SENSOR) ==> MASTER.
     * 3) new mode : SLAVE :
     *     case 1: oldSlaveId == slaveId  : OK
     *     Case 2: else : SLAVE => MASTER
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_SYNC) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_SYNC:
            if (newOperationSensor != oldOperationSensor) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                newOperationMode = DUAL_OPERATION_MODE_MASTER;
            }
            goto DUAL_OPERATION_MODE_END;
            break;
        case DUAL_OPERATION_MODE_SLAVE:
            m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
            m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
            if ((oldSlaveId < 0) || (slaveId < 0)) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (oldSlaveId != slaveId) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                newOperationMode = DUAL_OPERATION_MODE_MASTER;
            }
            goto DUAL_OPERATION_MODE_END;
            break;

        case DUAL_OPERATION_MODE_MASTER:
        default:
            goto DUAL_OPERATION_MODE_END;
        }
     }

      /*
     * if oldOperationMode is DUAL_OPERATION_MODE_SLAVE,
     * 1) new mode : MASTER  => SYNC (MASTER + OLD SLAVE)
     * 2) new mode : SYNC
     *     case 1: oldSlaveId == slaveId  : OK
           case 2 : else..SYNC (MASTER + OLD SLAVE)
     * 3) new mode : SLAVE:
     *     case 1: oldSlaveId == slaveId  : OK
     *     Case 2: else: SYNC (MASTER + OLD SLAVE)
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_SLAVE) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_MASTER:
            goto DUAL_OPERATION_MODE_END;
            break;
        case DUAL_OPERATION_MODE_SYNC:
        case DUAL_OPERATION_MODE_SLAVE:
            m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
            m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
            if ((oldSlaveId < 0) || (slaveId < 0)) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (oldSlaveId != slaveId) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                newOperationMode = DUAL_OPERATION_MODE_MASTER;
            }

            goto DUAL_OPERATION_MODE_END;
            break;
        default:
            goto DUAL_OPERATION_MODE_END;
        }
     }

#else
    /*
     * Allowed cases :
     * Slave_mode (SWIDE) <-> Sync_mode (WIDE_SWIDE) <-> Master_mode (WIDE) <-> Sync_Mode (WIDE_TELE) <-> Slave_mode (TELE)
     */

    /*
     * if oldOperationMode is DUAL_OPERATION_MODE_MASTER,
     * 1) Any SYNC mode is OK. DUAL_OPERATION_SENSOR_BACK_MAIN_SUB or DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2.
     * 2) if newOperationMode is SLAVE, set the corresponding SYNC mode (M + S).
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_MASTER) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_SLAVE:
            m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
            if (slaveId < 0) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (slaveId == m_camIdInfo.cameraId[SUB_CAM]) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
                //DUAL_OPERATION_SENSOR_BACK_SUB

            } else {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2;
                //DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2
            }
            newOperationMode = DUAL_OPERATION_MODE_SYNC;
            goto DUAL_OPERATION_MODE_END;
            break;

        case DUAL_OPERATION_MODE_MASTER:
        case DUAL_OPERATION_MODE_SYNC:
        default:
            goto DUAL_OPERATION_MODE_END;
            break;
        }
     }

     /*
     * if oldOperationMode is DUAL_OPERATION_MODE_SYNC,
     * 1) new mode : MASTER  (ok)
     * 2) new mode : SYNC (DIFFERENT_SENSOR) ==> MASTER.
     * 3) new mode : SLAVE :
     *     case 1: oldSlaveId == slaveId  : OK
     *     Case 2: else : SLAVE => MASTER
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_SYNC) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_SYNC:
            if (newOperationSensor != oldOperationSensor) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                newOperationMode = DUAL_OPERATION_MODE_MASTER;
            }
            goto DUAL_OPERATION_MODE_END;
            break;
        case DUAL_OPERATION_MODE_SLAVE:
            m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
            m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
            if ((oldSlaveId < 0) || (slaveId < 0)) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (oldSlaveId != slaveId) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                newOperationMode = DUAL_OPERATION_MODE_MASTER;
            }
            goto DUAL_OPERATION_MODE_END;
            break;

        case DUAL_OPERATION_MODE_MASTER:
        default:
            goto DUAL_OPERATION_MODE_END;
        }
     }

      /*
     * if oldOperationMode is DUAL_OPERATION_MODE_SLAVE,
     * 1) new mode : MASTER  => SYNC (MASTER + OLD SLAVE)
     * 2) new mode : SYNC
     *     case 1: oldSlaveId == slaveId  : OK
           case 2 : else..SYNC (MASTER + OLD SLAVE)
     * 3) new mode : SLAVE:
     *     case 1: oldSlaveId == slaveId  : OK
     *     Case 2: else: SYNC (MASTER + OLD SLAVE)
     */

     if (oldOperationMode == DUAL_OPERATION_MODE_SLAVE) {
        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_MASTER:
            m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
            if (oldSlaveId < 0) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (oldSlaveId == m_camIdInfo.cameraId[SUB_CAM]) {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
                //DUAL_OPERATION_SENSOR_BACK_SUB

            } else {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2;
                //DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2
            }
            newOperationMode = DUAL_OPERATION_MODE_SYNC;
            goto DUAL_OPERATION_MODE_END;
            break;
        case DUAL_OPERATION_MODE_SYNC:
        case DUAL_OPERATION_MODE_SLAVE:
            m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
            m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
            if ((oldSlaveId < 0) || (slaveId < 0)) {
                CLOGE("WRONG STATE!!!(%d %d %d %d)", oldOperationMode, newOperationMode,
                                oldOperationSensor, newOperationSensor);
                ret = BAD_VALUE;
                goto DUAL_OPERATION_MODE_END;
            }

            if (oldSlaveId != slaveId) {
                if (oldSlaveId == m_camIdInfo.cameraId[SUB_CAM]) {
                    newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
                    //DUAL_OPERATION_SENSOR_BACK_SUB

                } else {
                    newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2;
                    //DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2
                }
            }
            newOperationMode = DUAL_OPERATION_MODE_SYNC;
            goto DUAL_OPERATION_MODE_END;
            break;
        default:
            goto DUAL_OPERATION_MODE_END;
        }
     }
#endif

DUAL_OPERATION_MODE_END:

#ifdef RECHECK_DUAL_MODE_BASED_ON_STANDBY_STATE
    m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterId, &oldSlaveId);
    m_getCameraIdFromOperationSensor(newOperationSensor, &masterId, &slaveId);
    if ((slaveId != -1) && (slaveId != oldSlaveId)) {
        for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
            if (m_camIdInfo.cameraId[i] != slaveId) {
                if (m_parameters[m_camIdInfo.cameraId[i]]->getStandbyState() >= DUAL_STANDBY_STATE_OFF) {
                    //other slave cam is active. we can't enable the new slave now.
                    CLOGD("Forcing (slave (%d %d): OperationMode(%d => %d) OperationSensor(%d => %d)",
                    oldSlaveId, slaveId,
                    newOperationMode, oldOperationMode,
                    newOperationSensor, oldOperationSensor);

                    newOperationSensor = oldOperationSensor;
                    newOperationMode = oldOperationMode;
                    break;
                }
            }
        }
    }
#endif
    CLOGD("After: OperationMode(%d => %d) OperationSensor(%d => %d)",
                    oldOperationMode, newOperationMode,
                    oldOperationSensor, newOperationSensor);

    return ret;

}

status_t ExynosCamera::m_checkDualOperationMode(ExynosCameraRequestSP_sprt_t request,
                                                 bool isNeedModeChange, bool isReprocessing, bool flagFinishFactoryStart, bool skipCaptureLock)
{
    Mutex::Autolock lock(m_dualOperationModeLock);

    status_t ret = NO_ERROR;
    flagFinishFactoryStart &= m_flagFinishDualFrameFactoryStartThread;

    if (request == NULL) {
        CLOGE("request is NULL!!");
        return BAD_VALUE;
    }

    float zoomRatio = request->getMetaParameters()->m_zoomRatio; /* zoom ratio by current request */
    enum DUAL_OPERATION_MODE oldOperationModeReprocessing = m_configurations->getDualOperationModeReprocessing();
    enum DUAL_OPERATION_SENSORS oldOperationReprocessingSensor = m_configurations->getDualOperationSensor(true);
    enum DUAL_OPERATION_MODE newOperationMode = DUAL_OPERATION_MODE_NONE;
    enum DUAL_OPERATION_SENSORS newOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    enum DUAL_OPERATION_MODE oldOperationMode = m_configurations->getDualOperationMode();
    enum DUAL_OPERATION_SENSORS oldOperationSensor = m_configurations->getDualOperationSensor(false);
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
    int32_t masterCamId, slaveCamId;
    int32_t oldMasterCamId, oldSlaveCamId;
    int32_t dualOperationModeLockCount = m_configurations->getDualOperationModeLockCount();
    int32_t captureProcessSize = 0;
    bool needCheckMasterStandby = false;
    bool needCheckSlaveStandby = false;
    bool isTriggered = false;
    bool isTriggerSkipped = false;

    {
        Mutex::Autolock l(m_captureProcessLock);
        captureProcessSize = m_captureProcessList.size();
    }

    m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterCamId, &oldSlaveCamId);

    if (isReprocessing) {
        enum DUAL_OPERATION_MODE newOperationModeReprocessing = DUAL_OPERATION_MODE_NONE;
        enum DUAL_OPERATION_SENSORS newOperationReprocessingSensor = DUAL_OPERATION_SENSOR_MAX;

        if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
            if (m_dualCaptureLockCount) {
                // Currently reprocessing mode is also updated at every processCaptureRequest()
                // But it can't be updated during capture for stability
                DUAL_OPERATION_MODE fallbackOperationMode = DUAL_OPERATION_MODE_NONE;
                DUAL_OPERATION_SENSORS fallbackOperationSensor = DUAL_OPERATION_SENSOR_MAX;
                DUAL_OPERATION_SENSORS fallbackOperationReprSensor = DUAL_OPERATION_SENSOR_MAX;
                bool fallback = m_configurations->getFallbackState(fallbackOperationMode,
                        fallbackOperationSensor,
                        fallbackOperationReprSensor);

                CLOGI("[R%d] Now capture is working(zoom:%f,cnt:%d).. keep current mode (preview:%d,%d, capture:%d,%d, fallback(%d):%d,%d,%d)",
                        request->getKey(),
                        zoomRatio,
                        m_dualCaptureLockCount,
                        oldOperationMode,
                        oldOperationSensor,
                        oldOperationModeReprocessing,
                        oldOperationReprocessingSensor,
                        fallback,
                        fallbackOperationMode,
                        fallbackOperationSensor,
                        fallbackOperationReprSensor);

                return NO_ERROR;
            }
           /* reprocessingDualMode */
           if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_SWITCHING) {
                if (zoomRatio >= DUAL_SWITCHING_SYNC_MODE_MAX_ZOOM_RATIO) {
                    newOperationModeReprocessing = DUAL_OPERATION_MODE_SLAVE;
                    newOperationReprocessingSensor = DUAL_OPERATION_SENSOR_BACK_SUB;
                } else {
                    newOperationModeReprocessing = DUAL_OPERATION_MODE_MASTER;
                    newOperationReprocessingSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                }
            } else {
                if (m_camIdInfo.dualTransitionInfo.empty()) {
                    CLOG_ASSERT("m_camIdInfo->dualTransitionInfo is null!!");
                }
                auto &transInfo = m_camIdInfo.dualTransitionInfo.lower_bound(zoomRatio)->second;
                ret = m_checkDualOperationMode_vendor(request, isNeedModeChange, isReprocessing, flagFinishFactoryStart,
                        transInfo, newOperationModeReprocessing, newOperationReprocessingSensor);
                if (ret < 0) {
                    CLOGE("[R%d] checkDualOperationMode_vendor fail(%f)", request->getKey(), zoomRatio);
                }

                if (m_currentMultiCaptureMode == MULTI_CAPTURE_MODE_BURST
                    || m_currentMultiCaptureMode == MULTI_CAPTURE_MODE_AGIF) {
                    if (m_dualMultiCaptureLockflag == false) {
                        m_dualMultiCaptureLockflag = true;
                    } else {
                        if (oldOperationModeReprocessing != newOperationModeReprocessing) {
                            newOperationModeReprocessing = oldOperationModeReprocessing;
                            newOperationReprocessingSensor = oldOperationReprocessingSensor;
                        }
                    }
                }
            }

            if (dualOperationModeLockCount > 0 &&
                    oldOperationModeReprocessing != DUAL_OPERATION_MODE_NONE &&
                    oldOperationMode != DUAL_OPERATION_MODE_SYNC) {
                /* keep this dualOperationModeReprocessing with current dualOperationMode */
                if (oldOperationMode != newOperationModeReprocessing) {
                    CLOGW("[R%d] lock reprocessing operation mode(%d / %d) for capture, lock(%d)",
                            request->getKey(), newOperationModeReprocessing, oldOperationMode,
                            dualOperationModeLockCount);
                    newOperationModeReprocessing = oldOperationMode;
                    newOperationReprocessingSensor = oldOperationSensor;
                }
            }

            // set capture lockCount
            if (skipCaptureLock == false)
                m_dualCaptureLockCount = DUAL_CAPTURE_LOCK_COUNT;
        } else if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
            newOperationModeReprocessing = DUAL_OPERATION_MODE_SYNC;
        }

        if (m_isLogicalCam && m_configurations->isPhysStreamExist()) {
            /* When Physical streams are enabled, It is required to enabled SYNC mode for logical camera */
            newOperationModeReprocessing = DUAL_OPERATION_MODE_SYNC;
            newOperationReprocessingSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
        }

        m_getCameraIdFromOperationSensor(newOperationReprocessingSensor, &masterCamId, &slaveCamId);
        if (newOperationModeReprocessing == DUAL_OPERATION_MODE_SLAVE) {
            masterCamId = slaveCamId;
        }
        m_configurations->setDualOperationModeReprocessing(newOperationModeReprocessing);
        m_configurations->setDualOperationSensor(newOperationReprocessingSensor, true);
        m_configurations->setDualCamId(masterCamId, slaveCamId, true);
   } else {
        if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
            if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_SWITCHING) {
                if (zoomRatio >= DUAL_SWITCHING_SYNC_MODE_MAX_ZOOM_RATIO) {
                    newOperationMode = DUAL_OPERATION_MODE_SLAVE;
                    newOperationSensor = DUAL_OPERATION_SENSOR_BACK_SUB;
                } else {
                    newOperationMode = DUAL_OPERATION_MODE_MASTER;
                    newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                }
            } else {
                if (m_camIdInfo.dualTransitionInfo.empty()) {
                    CLOG_ASSERT("m_camIdInfo->dualTransitionInfo is null!!");
                }
                auto &transInfo = m_camIdInfo.dualTransitionInfo.lower_bound(zoomRatio)->second;
                ret = m_checkDualOperationMode_vendor(request, isNeedModeChange, isReprocessing, flagFinishFactoryStart,
                        transInfo, newOperationMode, newOperationSensor);
                if (ret < 0) {
                    CLOGE("[R%d] checkDualOperationMode_vendor fail(%f)", request->getKey(), zoomRatio);
                }
            }
        } else if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
            newOperationMode = DUAL_OPERATION_MODE_SYNC;
            if (m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
                newOperationSensor = DUAL_OPERATION_SENSOR_FRONT_MAIN_SUB;
            } else {
                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
            }
        }

        if (m_isLogicalCam && m_configurations->isPhysStreamExist()) {
            /* When Physical streams are enabled, It is required to enabled SYNC mode for logical camera */
            newOperationMode = DUAL_OPERATION_MODE_SYNC;
            newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
        }

        if (m_dualTransitionCount > 0) {
            if (m_dualTransitionOperationSensor == DUAL_OPERATION_SENSOR_MAX) {
                CLOGE("[R%d] transitionCount(%d) m_dualTransitionOperationSensor (%d)",
                            request->getKey(), m_dualTransitionCount, m_dualTransitionOperationSensor);
                m_dualTransitionCount = 0;
                return NO_ERROR;
            }

            if ((oldOperationMode == DUAL_OPERATION_MODE_SYNC) &&
                (oldOperationSensor == m_dualTransitionOperationSensor)) {
                if (isNeedModeChange == true) {
                    CLOGD("[R%d]Return for dual transition, transitionCount(%d)",
                            request->getKey(), m_dualTransitionCount);
                }

                m_dualTransitionCount--;
                return NO_ERROR;
            }

           if ((newOperationMode != DUAL_OPERATION_MODE_SYNC) ||
                (newOperationSensor != m_dualTransitionOperationSensor)) {
                /* keep previous sync for at least remained m_dualTransitionCount */
                /* DUAL_OPERATION_MODE_SYNC */
                newOperationMode = DUAL_OPERATION_MODE_SYNC;
                newOperationSensor = m_dualTransitionOperationSensor;
            }
        }

        // earlyDualMode update
        if (((int32_t)request->getKey()) > m_earlyDualRequestKey) {
            m_earlyDualOperationMode = newOperationMode;
            m_earlyOperationSensor = newOperationSensor;
            // get current reprocessing sensor mode for master camera
            m_configurations->getDualCamId(masterCamId, slaveCamId, true);
            if (masterCamId < 0) {
                masterCamId = slaveCamId;
            }
            m_earlyMasterCameraId = masterCamId;
            m_earlyDualRequestKey = request->getKey();
        }

        if ((newOperationMode != oldOperationMode) ||
            (newOperationSensor != oldOperationSensor)) {
            if ((m_scenario == SCENARIO_DUAL_REAR_ZOOM) && (m_camIdInfo.numOfSensors > 2)) {
                m_modulateDualOperationMode(newOperationMode, oldOperationMode, newOperationSensor, oldOperationSensor);
            } else {
                switch (oldOperationMode) {
                case DUAL_OPERATION_MODE_MASTER:
                case DUAL_OPERATION_MODE_SLAVE:
                    if (newOperationMode != DUAL_OPERATION_MODE_SYNC) {
                        newOperationMode = DUAL_OPERATION_MODE_SYNC;
                        if (m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
                            newOperationSensor = DUAL_OPERATION_SENSOR_FRONT_MAIN_SUB;
                        } else if (m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) {
                            newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
                        } else {
                            // SCENARIO_DUAL_REAR_ZOOM and numOfSensors == 2
                            if (m_configurations->getMode(CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE) == true) {
                                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2;
                            } else {
                                newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN_SUB;
                            }
                        }
                    }
                    break;
                case DUAL_OPERATION_MODE_NONE:
                default:
                    break;
                }
            }

            if (isNeedModeChange == true) {
                bool lockStandby = false;
                {
                    /* don't off alive sensor cause of not finished capture */
                    Mutex::Autolock l(m_captureProcessLock);
                    lockStandby = (m_dualCaptureLockCount > 0 || m_captureProcessList.size() > 0);
                }

                if (dualOperationModeLockCount > 0)
                    lockStandby = true;

                m_getCameraIdFromOperationSensor(newOperationSensor, &masterCamId, &slaveCamId);
                CLOGD("[R%d] Change dual mode old(%d), new(%d), finishFactoryStart(%d), standby(M:%d) count(%d, %d) capturelock(%d) lock(%d)"
                            "dualOperationSensor (%d %d) masterCamId (%d %d) slaveCamId(%d %d)",
                        request->getKey(),
                        oldOperationMode, newOperationMode, flagFinishFactoryStart,
                        m_parameters[m_camIdInfo.cameraId[MAIN_CAM]]->getStandbyState(),
                        m_dualTransitionCount,
                        m_dualCaptureLockCount,
                        lockStandby,
                        dualOperationModeLockCount,
                        oldOperationSensor, newOperationSensor,
                        oldMasterCamId, masterCamId, oldSlaveCamId, slaveCamId);
                for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
                    CLOGD("Standby State : Slave(Id = %d, State = %d)",
                        m_camIdInfo.cameraId[i], m_parameters[m_camIdInfo.cameraId[i]]->getStandbyState());
                }

                switch (newOperationMode) {
                case DUAL_OPERATION_MODE_MASTER:
                    needCheckMasterStandby = true;
                    break;
                case DUAL_OPERATION_MODE_SLAVE:
                    needCheckSlaveStandby = true;
                    break;
                case DUAL_OPERATION_MODE_SYNC:
                    needCheckMasterStandby = true;
                    needCheckSlaveStandby = true;
                    break;
                default:
                    break;
                }

                if ((needCheckMasterStandby && (masterCamId < 0)) ||
                    (needCheckSlaveStandby && (slaveCamId < 0))) {
                    CLOGW("[R%d] OperationSensor is not proper: forcely fall back to old dualOperationMode(%d -> %d)"
                           "OperationSensor[%d -> %d] masterCamId = %d slaveCamId = %d needCheckMasterStandby = %d needCheckSlaveStandby = %d",
                                request->getKey(), newOperationMode, oldOperationMode,
                                oldOperationSensor, newOperationSensor, masterCamId, slaveCamId,
                                needCheckMasterStandby, needCheckSlaveStandby);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                        needCheckMasterStandby = false;
                        needCheckSlaveStandby = false;
                }

                if (newOperationMode == DUAL_OPERATION_MODE_SYNC) {
                    if (dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION) {
                        if (newOperationSensor == DUAL_OPERATION_SENSOR_BACK_MAIN_SUB2) {
                            m_dualTransitionCount = DUAL_SWITCH_TRANSITION_FRAME_COUNT;
                        } else {
                            m_dualTransitionCount = DUAL_TRANSITION_FRAME_COUNT;
                        }
                    } else {
                        m_dualTransitionCount = DUAL_SWITCH_TRANSITION_FRAME_COUNT;
                    }
                    m_dualTransitionOperationSensor = newOperationSensor;
                }

                if (((oldOperationMode != newOperationMode)
                    || (oldOperationSensor != newOperationSensor))
                    && (lockStandby == false)) {
                    CLOGD("[R%d] standby mode trigger start (STANDBY_OFF case), modeChange(%d)",
                            request->getKey(), isNeedModeChange);
#ifdef USE_DEFER_STANDB_ON_CONTROL
                    ret = m_setStandbyModeTrigger(flagFinishFactoryStart, newOperationMode, oldOperationMode,
                                false, &isTriggered, newOperationSensor, oldOperationSensor, isTriggerSkipped);
#else
                    ret = m_setStandbyModeTrigger(flagFinishFactoryStart, newOperationMode, oldOperationMode,
                                isNeedModeChange, &isTriggered, newOperationSensor, oldOperationSensor, isTriggerSkipped);
#endif
                    if (ret != NO_ERROR) {
                        CLOGE("Set Standby mode(false) fail! ret(%d)", ret);
                    }

                    if (isTriggerSkipped) {
                         CLOGD("StandbyModeTrigger is skipped: forcedly set the dualOperationMode(%d -> %d) Sensor(%d -> %d)",
                                newOperationMode, oldOperationMode, newOperationSensor, oldOperationSensor);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                    }
                }

                if (oldOperationMode != DUAL_OPERATION_MODE_NONE) {
                    if (flagFinishFactoryStart == false) {
                        CLOGW("[R%d] not finish dualFrameFactoryStartThread forcely set to dualOperationMode(%d -> %d)",
                                request->getKey(), newOperationMode, oldOperationMode);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                        needCheckMasterStandby = false;
                        needCheckSlaveStandby = false;
                    }

                    if (needCheckMasterStandby &&
                            m_parameters[masterCamId]->getStandbyState() != DUAL_STANDBY_STATE_OFF) {
                        CLOGW("[R%d] not ready of cameraId(%d) state(%d) forcely set to dualOperationMode(%d -> %d) Sensor(%d -> %d)",
                                request->getKey(), m_cameraId, m_parameters[masterCamId]->getStandbyState(),
                                newOperationMode, oldOperationMode, newOperationSensor, oldOperationSensor);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                    }

                    if (needCheckSlaveStandby &&
                            m_parameters[slaveCamId]->getStandbyState() != DUAL_STANDBY_STATE_OFF) {

                        CLOGW("[R%d] not ready of cameraId(%d) state(%d) forcedly set to dualOperationMode(%d -> %d) Sensor(%d -> %d)",
                                request->getKey(), slaveCamId, m_parameters[slaveCamId]->getStandbyState(),
                                newOperationMode, oldOperationMode, newOperationSensor, oldOperationSensor);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                    }

                    if (lockStandby == true) {
                        switch (newOperationMode) {
                        case DUAL_OPERATION_MODE_MASTER:
                            if (oldOperationMode == DUAL_OPERATION_MODE_SYNC) {
                                CLOGW("[R%d] please keep the previous operation mode(%d / %d) for capture. cnt(%d/%zu)",
                                        request->getKey(), newOperationMode, oldOperationMode,
                                        m_dualCaptureLockCount, captureProcessSize);
                                newOperationMode = oldOperationMode;
                                newOperationSensor = oldOperationSensor;
                            }
                            break;
                        case DUAL_OPERATION_MODE_SLAVE:
                            if (oldOperationMode == DUAL_OPERATION_MODE_SYNC) {
                                CLOGW("[R%d] please keep the previous operation mode(%d / %d) for capture, cnt(%d/%zu)",
                                        request->getKey(), newOperationMode, oldOperationMode,
                                        m_dualCaptureLockCount, captureProcessSize);
                                newOperationMode = oldOperationMode;
                                newOperationSensor = oldOperationSensor;
                            }
                            break;
                        default:
                            break;
                        }
                    }

                    if (dualOperationModeLockCount > 0) {
                        CLOGW("[R%d] lock previous operation mode(%d / %d), lockCnt(%d)",
                                request->getKey(), newOperationMode, oldOperationMode,
                                dualOperationModeLockCount);
                        newOperationMode = oldOperationMode;
                        newOperationSensor = oldOperationSensor;
                    }
                }

                m_getCameraIdFromOperationSensor(newOperationSensor, &masterCamId, &slaveCamId);
                if (newOperationMode == DUAL_OPERATION_MODE_SLAVE) {
                    masterCamId = slaveCamId;
                }
                m_configurations->setDualOperationMode(newOperationMode);
                m_configurations->setDualOperationSensor(newOperationSensor, false);
                m_configurations->setDualCamId(masterCamId, slaveCamId, false);
#ifdef USE_DEFER_STANDB_ON_CONTROL
                if (((oldOperationMode != newOperationMode)
                    || (oldOperationSensor != newOperationSensor))
                    && (lockStandby == false)
                    && isNeedModeChange
                    && (isTriggerSkipped == false)) {
                    CLOGD("[R%d] standby mode trigger start (STANDBY_ON case), modeChange(%d)",
                            request->getKey(), isNeedModeChange);
                    //STANDBY_ON case is called only if isNeedModeChange is true
                    ret = m_setStandbyModeTrigger(flagFinishFactoryStart, newOperationMode, oldOperationMode,
                                true, &isTriggered, newOperationSensor, oldOperationSensor, isTriggerSkipped);
                    if (ret != NO_ERROR) {
                        CLOGE("Set Standby mode(false) fail! ret(%d)", ret);
                    }
                }
#endif
            } else if (newOperationMode == DUAL_OPERATION_MODE_SYNC) {
                CLOGD("[R%d] stanby mode trigger start, modeChange(%d)",
                        request->getKey(), isNeedModeChange);
                ret = m_setStandbyModeTrigger(flagFinishFactoryStart, newOperationMode, oldOperationMode,
                            isNeedModeChange, &isTriggered, newOperationSensor, oldOperationSensor, isTriggerSkipped);
                if (ret != NO_ERROR) {
                    CLOGE("Set Standby mode(false) fail! ret(%d)", ret);
                } else {
                    if (isTriggered) {
                        /*
                         * This variable will be used in case of situation that
                         * "Early sensorStandby off" is finished too fast
                         * before the request to trigger it is used in m_createPreviewFrameFunc().
                         */
                        m_earlyTriggerRequestKey = request->getKey();
                    }
                }

                /* Clearing essentialRequestList */
                m_latestRequestListLock.lock();
                m_essentialRequestList.clear();
                m_latestRequestListLock.unlock();
            }
        }

        if (isNeedModeChange == true) {
            if (m_dualCaptureLockCount > 0) {
                m_dualCaptureLockCount--;
            }

            m_configurations->decreaseDualOperationModeLockCount();
            m_configurations->checkFallbackState();
        }
    }
    return ret;
}

status_t ExynosCamera::m_checkDualOperationMode_vendor(ExynosCameraRequestSP_sprt_t request,
                                                 bool isNeedModeChange, bool isReprocessing, bool flagFinishFactoryStart,
                                                 const DualTransitionInfo &transInfo,
                                                 enum DUAL_OPERATION_MODE &newOperationMode,
                                                 enum DUAL_OPERATION_SENSORS &newOperationSensor)
{
    if (m_scenario != SCENARIO_DUAL_REAR_ZOOM) {
        return NO_ERROR;
    }

    DUAL_OPERATION_MODE fallbackOperationMode = DUAL_OPERATION_MODE_NONE;
    DUAL_OPERATION_SENSORS fallbackOperationSensor = DUAL_OPERATION_SENSOR_MAX;
    DUAL_OPERATION_SENSORS fallbackOperationReprSensor = DUAL_OPERATION_SENSOR_MAX;
    bool fallback = m_configurations->getFallbackState(fallbackOperationMode,
                                                    fallbackOperationSensor,
                                                    fallbackOperationReprSensor);

    newOperationMode = (isReprocessing == true) ? transInfo.dualOpRepMode : transInfo.dualOpMode;
    newOperationSensor = (isReprocessing == true) ? transInfo.dualOpRepSensor : transInfo.dualOpSensor;

    if (isReprocessing == true) {
        if (fallback == true
            && ((newOperationMode != fallbackOperationMode)
                || (newOperationSensor != fallbackOperationReprSensor))) {
            CLOGD("[Reprocessing] fallback mode(%d -> %d), sensor(%d -> %d)",
                    newOperationMode, fallbackOperationMode,
                    newOperationSensor, fallbackOperationReprSensor);

            newOperationMode = fallbackOperationMode;
            newOperationSensor = fallbackOperationReprSensor;
        }

        /* Doesn't need to update when there's no capture request */
        if (request->getSizeOfFactory(FRAME_FACTORY_TYPE_REPROCESSING) <= 0) {
            return NO_ERROR;
        }

        DUAL_OPERATION_MODE preveiwOperationMode = m_configurations->getDualOperationMode();
        DUAL_OPERATION_SENSORS previewOperationSensor = m_configurations->getDualOperationSensor(false);

        switch (newOperationMode) {
        case DUAL_OPERATION_MODE_SYNC:
        {
            bool needFusion = false;
            if (preveiwOperationMode == DUAL_OPERATION_MODE_SYNC) {
                needFusion = m_configurations->checkFusionCaptureMode(request,
                                                                    newOperationMode,
                                                                    newOperationSensor);

                if (needFusion == false) {
                    switch (newOperationSensor) {
                    case DUAL_OPERATION_SENSOR_BACK_SUB_MAIN:
                        newOperationMode = DUAL_OPERATION_MODE_SLAVE;
                        newOperationSensor = DUAL_OPERATION_SENSOR_BACK_SUB;
                        break;
                    case DUAL_OPERATION_SENSOR_BACK_SUB2_MAIN:
                        newOperationMode = DUAL_OPERATION_MODE_SLAVE;
                        newOperationSensor = DUAL_OPERATION_SENSOR_BACK_SUB2;
                        break;
                    default:
                        newOperationMode = DUAL_OPERATION_MODE_MASTER;
                        newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;
                        break;
                    }

                    CLOGD("[Reprocessing][R%d] forcely set to single(%d, %d) only", request->getKey(),
                            newOperationMode, newOperationSensor);
                }
                break;
            }
            [[fallthrough]];
        }
        case DUAL_OPERATION_MODE_SLAVE:
            [[fallthrough]];
        case DUAL_OPERATION_MODE_MASTER:
        {
            bool forcelySet = false;
            if (previewOperationSensor != newOperationSensor) {
                if (newOperationMode == DUAL_OPERATION_MODE_SYNC) {
                    // if reprocessing mode is sync mode, sensor should be same!
                    forcelySet = true;
                } else {
                    // check if this reprocessing mode can be supported in current preview mode
                    if (isContainedOfSensor(newOperationSensor, previewOperationSensor)) {
                        forcelySet = true;
                    }
                }
            }

            if (forcelySet) {
                // set minumum sensor mode
                if (newOperationMode == DUAL_OPERATION_MODE_SYNC)
                    newOperationSensor = DUAL_OPERATION_SENSOR_BACK_MAIN;

                CLOGD("[Reprocessing][R%d] forcely set (mode: %d -> %d, sensor: %d -> %d)", request->getKey(),
                        preveiwOperationMode, newOperationMode,
                        previewOperationSensor, newOperationSensor);

                newOperationMode = preveiwOperationMode;
                newOperationSensor = previewOperationSensor;
            }
            break;
        }
        default:
            CLOGE("[R%d] invalid Operation mode (%d)", request->getKey(), newOperationMode);
            break;
        }
    } else {
        if (fallback == true) {
            if (newOperationMode != fallbackOperationMode ||
                newOperationSensor != fallbackOperationSensor) {
                CLOGV("[Preview] fallback mode(%d -> %d), sensor(%d -> %d)",
                        newOperationMode, fallbackOperationMode,
                        newOperationSensor, fallbackOperationSensor);

                newOperationMode = fallbackOperationMode;
                newOperationSensor = fallbackOperationSensor;
            }
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setStandbyModeTrigger(bool flagFinishFactoryStart,
        enum DUAL_OPERATION_MODE newDualOperationMode,
        enum DUAL_OPERATION_MODE oldDualOperationMode,
        bool tryStandbyOn,
        bool *isTriggered,
        enum DUAL_OPERATION_SENSORS newOperationSensor,
        enum DUAL_OPERATION_SENSORS oldOperationSensor,
        bool &isTriggerSkipped)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;

    bool slaveSensorStandby = false;  /* target */
    int32_t masterCamId, slaveCamId;
    int32_t oldMasterCamId, oldSlaveCamId;
    bool isModeValid = true;
    dual_standby_state_t sensorStandbyStateCur[MAX_NUM_SENSORS] = {DUAL_STANDBY_STATE_ON};
    bool sensorStandbySupported[MAX_NUM_SENSORS] = {false};
    bool sensorStandbyModeCur[MAX_NUM_SENSORS] = {false};
    bool sensorStandbyModeRequired[MAX_NUM_SENSORS] = {false};
    dual_standby_trigger_info_t sensorStandbyTrigger[MAX_NUM_SENSORS] = {};
    int masterCamIdIdx = -1;
    int slaveCamIdIdx = -1;
    bool needStandbyOFFTrigger = false;

    isTriggerSkipped = false;

    if (isTriggered == NULL) {
        CLOGE("isTriggerd is NULL");
        return INVALID_OPERATION;
    }

    *isTriggered = false;

    if (flagFinishFactoryStart == false) {
        CLOGD("Not yet finishing startFactoryThread(dualOperationMode(%d -> %d))",
                newDualOperationMode, oldDualOperationMode);
        return ret;
    }


    /* initializing the sensor standby state */
    for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
        sensorStandbySupported[i] = m_parameters[m_camIdInfo.cameraId[i]]->isSupportedFunction(SUPPORTED_HW_FUNCTION_SENSOR_STANDBY);
        sensorStandbyStateCur[i] = m_parameters[m_camIdInfo.cameraId[i]]->getStandbyState();
        sensorStandbyModeCur[i] = (sensorStandbyStateCur[i] >= DUAL_STANDBY_STATE_OFF) ? false : true;
        sensorStandbyModeRequired[i] = true; /* off */
    }

    m_getCameraIdFromOperationSensor(newOperationSensor, &masterCamId, &slaveCamId);
    m_getCameraIdFromOperationSensor(oldOperationSensor, &oldMasterCamId, &oldSlaveCamId);

    /*
     * TODO: StandBy On/Off Should be run for all possible sensors (MAX_NUM_SENSORS).
     * SensorStandby = false for the required sensors
     * SensorStandby = true for the unused sensors
     */

    /* Just for mode validation */
    switch (oldDualOperationMode) {
    case DUAL_OPERATION_MODE_MASTER:
        switch (newDualOperationMode) {
        case DUAL_OPERATION_MODE_SLAVE:
            /* No limitation */
            break;
        case DUAL_OPERATION_MODE_SYNC:
            if ((oldMasterCamId >= 0) && (masterCamId >= 0) && (masterCamId != oldMasterCamId)) {
                isModeValid = false;
            }
            break;
        case DUAL_OPERATION_MODE_MASTER:
            //TODO: If sensor is changed, need to standby the old sensor.
            if ((oldMasterCamId >= 0) && (masterCamId >= 0) && (masterCamId != oldMasterCamId)) {
                /*
                 * when sensor is changed, previous sesnor should be stream_off before
                 * stream_on the new sensor
                 */
                isModeValid = false;
            }
            break;
        default:
            isModeValid = false;
            CLOGE("invalid transition of dualOperationMode");
            break;
        }
        break;
    case DUAL_OPERATION_MODE_SLAVE:
        switch (newDualOperationMode) {
        case DUAL_OPERATION_MODE_MASTER:
            /* No limitation */
            break;
        case DUAL_OPERATION_MODE_SYNC:
            if ((oldSlaveCamId >= 0) && (slaveCamId >= 0) && (slaveCamId != oldSlaveCamId)) {
                isModeValid = false;
            }
            break;
        case DUAL_OPERATION_MODE_SLAVE:
            //TODO: If sensor is changed, need to standby the old sensor.
            if ((oldSlaveCamId >= 0) && (slaveCamId >= 0) && (slaveCamId != oldSlaveCamId)) {
                slaveSensorStandby = true; /* off */
                isModeValid = false;
            }
            break;
        default:
            isModeValid = false;
            CLOGE("invalid trasition of dualOperationMode");
            break;
        }
        break;
    case DUAL_OPERATION_MODE_SYNC:
        switch (newDualOperationMode) {
        case DUAL_OPERATION_MODE_MASTER:
            if ((oldMasterCamId >= 0) && (masterCamId >= 0) && (masterCamId != oldMasterCamId)) {
                isModeValid = false;
            }
            break;
        case DUAL_OPERATION_MODE_SLAVE:
            if (slaveCamId != oldSlaveCamId) {
                isModeValid = false;
            }
            break;
        case DUAL_OPERATION_MODE_SYNC:
            //TODO: If sensors are changed, need to standby the old unused sensors.
            if (((oldMasterCamId >= 0) && (masterCamId >= 0) && (masterCamId != oldMasterCamId))
                || ((oldSlaveCamId >= 0) && (slaveCamId >= 0) && (slaveCamId != oldSlaveCamId))) {
                isModeValid = false;
            }
            break;
        default:
            isModeValid = false;
            CLOGE("invalid trasition of dualOperationMode");
            break;
        }
        break;
    default:
        isModeValid = false;
        CLOG_ASSERT("wrong dual operationMode(%d)", oldDualOperationMode);
        break;
    }

    CLOGD("OperationMode(%d => %d) OperationSensor(%d => %d) masterCamId(%d => %d) slaveCamId(%d => %d) tryStandbyOn (%d)",
                    oldDualOperationMode, newDualOperationMode,
                    oldOperationSensor, newOperationSensor,
                    oldMasterCamId, masterCamId, oldSlaveCamId, slaveCamId, tryStandbyOn);

    if (!isModeValid) {
        CLOGE("not supported mode!!!");
        return NO_ERROR;
    }

    if (masterCamId >= 0) {
        masterCamIdIdx = getCameraIdx(masterCamId);
        if (masterCamIdIdx >= 0) {
            sensorStandbyModeRequired[masterCamIdIdx] = false;  /* on  */
        } else {
            CLOGE("masterCamId (%d) masterCamIdIdx (%d) is invalid !!!", masterCamId, masterCamIdIdx);
        }
    }

    if (slaveCamId >= 0) {
        slaveCamIdIdx = getCameraIdx(slaveCamId);
        if (slaveCamIdIdx >= 0) {
            sensorStandbyModeRequired[slaveCamIdIdx] = false;  /* on  */
        } else {
            CLOGE("slaveCamId (%d) slaveCamIdIdx (%d) is invalid !!!", slaveCamId, slaveCamIdIdx);
        }
    }

    /*
     * update the standBy states into params.
     * Assuming m_camIdInfo.cameraId[0] as master always.
     * TODO: support any camera ID as master
     */

    for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
        if (sensorStandbyModeCur[i] != sensorStandbyModeRequired[i]) {
            if (!sensorStandbySupported[i]) {
                 /* poststandby case */
                if (sensorStandbyModeRequired[i] == true) {
                    m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_ON);
                } else {
                    m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_OFF);
                }
            } else {
                /* sensorstandby case */
                if (i == MAIN_CAM) {
                    sensorStandbyTrigger[i].mode = sensorStandbyModeRequired[i] ? DUAL_STANDBY_TRIGGER_MASTER_ON : DUAL_STANDBY_TRIGGER_MASTER_OFF;
                    sensorStandbyTrigger[i].camIdIndex = i;
                    CLOGD("M (%d => %d, camId = %d, sensorStandbyTrigger = %d)", sensorStandbyModeCur[i],
                            sensorStandbyModeRequired[i], m_camIdInfo.cameraId[i], sensorStandbyTrigger[i].mode);
                } else {
                    sensorStandbyTrigger[i].mode = sensorStandbyModeRequired[i] ? DUAL_STANDBY_TRIGGER_SLAVE_ON : DUAL_STANDBY_TRIGGER_SLAVE_OFF;
                    sensorStandbyTrigger[i].camIdIndex = i;
                    /* slave shot sync thread */
                    if (sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_OFF) {
                        if (m_slaveMainThread->isRunning() == false)
                            m_slaveMainThread->run(PRIORITY_URGENT_DISPLAY);
                    }
                    CLOGD("S (%d => %d, camId = %d, sensorStandbyTrigger = %d tryStandbyOn = %d)", sensorStandbyModeCur[i],
                            sensorStandbyModeRequired[i], m_camIdInfo.cameraId[i], sensorStandbyTrigger[i].mode, tryStandbyOn);
                }
            }
        }
    }

#ifdef USE_DEFER_STANDB_ON_CONTROL
    /*
     * ordering for performance.
     * Master & slave standby off -> Master & Slave standby on
     */
    bool isAnyPendingStandByOperation = false;
    dual_standby_state_t  localStandbyState;

    for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
        localStandbyState = m_parameters[m_camIdInfo.cameraId[i]]->getStandbyState();
        if ((localStandbyState == DUAL_STANDBY_STATE_ON_READY)
            || (localStandbyState == DUAL_STANDBY_STATE_OFF_READY)) {
            isAnyPendingStandByOperation = true;
            break;
        }
    }

    if (!tryStandbyOn) {
        //check if needStandbyOFFTrigger is required.
#ifdef USE_STANDBY_Q_SKIP_MODE
        for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
            if ((sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_MASTER_OFF)
                || (sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_OFF)) {
                needStandbyOFFTrigger = true;
                break;
            }
        }

        //skip logic
        if (needStandbyOFFTrigger) {
            int standbyTriggerQDepth = m_dualStandbyTriggerQ->getSizeOfProcessQ();
            if ((standbyTriggerQDepth > 0)
                || (isAnyPendingStandByOperation == true)) {
                isTriggerSkipped = true;
                CLOGD("Skipping StandBy Control : standbyTriggerQDepth (%d)", standbyTriggerQDepth);
                return ret;
            }
        }
#endif
        /*
         * Master standby off Case:
         * CamID[0] is assumed as master.
         * TODO: need to handle the dynamic config
         */
        if (sensorStandbyTrigger[0].mode == DUAL_STANDBY_TRIGGER_MASTER_OFF) {
            m_parameters[m_camIdInfo.cameraId[MAIN_CAM]]->setStandbyState(DUAL_STANDBY_STATE_OFF_READY);
            m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[0]);
            *isTriggered = true;
        }


         /*
         * slave standby off Case:
         * Case : slave_1 sensor needs standby_on and slave_2 sensor needs standby_off.
         * The above case needs slave_1 must be set to standby_on first, then slave_2 must be set to standby_off
         */

        for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
            if (sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_OFF) {
                m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_OFF_READY);
                m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[i]);
                *isTriggered = true;
            }
        }
    } else {
        /*
         * slave standby on Case:
         */

        for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
            if (sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_ON) {
                m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_ON_READY);
                m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[i]);
                *isTriggered = true;
            }
        }


        /*
         * Master standby on Case:
         */
        if (sensorStandbyTrigger[0].mode == DUAL_STANDBY_TRIGGER_MASTER_ON) {
            m_parameters[m_camIdInfo.cameraId[MAIN_CAM]]->setStandbyState(DUAL_STANDBY_STATE_ON_READY);
            m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[0]);
            *isTriggered = true;
        }
    }
#else
    /*
     * ordering for performance.
     * Master & slave standby off -> Master & Slave standby on
     */

    /*
     * slave standby off Case:
     * Case : slave_1 sensor needs standby_on and slave_2 sensor needs standby_off.
     * The above case needs slave_1 must be set to standby_on first, then slave_2 must be set to standby_off
     * TODO: Currently, Following cases are not supported in one call
     * Case-1: SLAVE_1 : STANDBY_ON => STANDBY_OFF & SLAVE_1 : STANDBY_ON => STANDBY_OFF
     * case-2: SLAVE_1 : STANDBY_ON => STANDBY_OFF & SLAVE_2 : STANDBY_OFF => STANDBY_ON
     * case-3: SLAVE_1 : STANDBY_OFF => STANDBY_ON & SLAVE_2 : STANDBY_ON => STANDBY_OFF
     * case-4: SLAVE_1 : STANDBY_OFF & SLAVE_2 : STANDBY_OFF
     */

    for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
        CLOGD("sensorStandbyTrigger[%s].mode = %d", i == 1 ? "TELE":"S-W", sensorStandbyTrigger[i].mode);
        if (sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_OFF) {
            m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_OFF_READY);
            m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[i]);
            *isTriggered = true;
        }
    }

    /*
     * Master standby off Case:
     * CamID[0] is assumed as master.
     * TODO: need to handle the dynamic config
     */
    if (sensorStandbyTrigger[0].mode == DUAL_STANDBY_TRIGGER_MASTER_OFF) {
        m_parameters[m_camIdInfo.cameraId[MAIN_CAM]]->setStandbyState(DUAL_STANDBY_STATE_OFF_READY);
        m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[0]);
        *isTriggered = true;
    }

    /*
     * slave standby on Case:
     */

    for (int i = 1; i < m_camIdInfo.numOfSensors; i++) {
        CLOGD("sensorStandbyTrigger[%s].mode = %d", i == 1 ? "TELE":"S-W", sensorStandbyTrigger[i].mode);
        if (tryStandbyOn && sensorStandbyTrigger[i].mode == DUAL_STANDBY_TRIGGER_SLAVE_ON) {
            m_parameters[m_camIdInfo.cameraId[i]]->setStandbyState(DUAL_STANDBY_STATE_ON_READY);
            m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[i]);
            *isTriggered = true;
        }
    }


    /*
     * Master standby on Case:
     */
    if (tryStandbyOn && sensorStandbyTrigger[0].mode == DUAL_STANDBY_TRIGGER_MASTER_ON) {
        m_parameters[m_camIdInfo.cameraId[MAIN_CAM]]->setStandbyState(DUAL_STANDBY_STATE_ON_READY);
        m_dualStandbyTriggerQ->pushProcessQ(&sensorStandbyTrigger[0]);
        *isTriggered = true;
    }
#endif

    if ((m_dualStandbyTriggerQ->getSizeOfProcessQ() > 0) &&
            (m_dualStandbyThread->isRunning() == false)) {
        m_dualStandbyThread->run(PRIORITY_URGENT_DISPLAY);
    }

    return ret;
}

bool ExynosCamera::m_dualStandbyThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    bool standby;
    bool isNeedPrepare;
    dual_standby_trigger_info_t dualStandbyTrigger_info;
    ExynosCameraFrameFactory *factory;
    enum DUAL_OPERATION_MODE dualOperationMode = DUAL_OPERATION_MODE_NONE;
    frame_type_t prepareFrameType;
    int32_t camIdIndex;

    ret = m_dualStandbyTriggerQ->waitAndPopProcessQ(&dualStandbyTrigger_info);
    if (ret == TIMED_OUT) {
        CLOGW("Time-out to wait m_dualStandbyTriggerQ");
        goto THREAD_EXIT;
    } else if (ret != NO_ERROR) {
        CLOGE("Failed to waitAndPopProcessQ for m_dualStandbyTriggerQ");
        goto THREAD_EXIT;
    }

    if (m_getState() != EXYNOS_CAMERA_STATE_RUN) {
        CLOGE("state is not RUN!!(%d)", m_getState());
        goto THREAD_EXIT;
    }

    /* default stream-off */
    standby = true;
    isNeedPrepare = false;
    camIdIndex = dualStandbyTrigger_info.camIdIndex;

    CLOGI("dualStandbyTriggerQ(%d) prepare(%d) trigger(%d) camIdIndex(%d) camID(%d)",
            m_dualStandbyTriggerQ->getSizeOfProcessQ(), m_prepareFrameCount,
            dualStandbyTrigger_info.mode, camIdIndex,
            m_camIdInfo.cameraId[camIdIndex]);

    switch (dualStandbyTrigger_info.mode) {
    case DUAL_STANDBY_TRIGGER_MASTER_OFF:
        isNeedPrepare = true;
        standby = false;
        dualOperationMode = DUAL_OPERATION_MODE_MASTER;
        prepareFrameType = FRAME_TYPE_TRANSITION;
        /* fall through */
    case DUAL_STANDBY_TRIGGER_MASTER_ON:
        factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
        break;
    case DUAL_STANDBY_TRIGGER_SLAVE_OFF:
        isNeedPrepare = true;
        standby = false;
        dualOperationMode = DUAL_OPERATION_MODE_SLAVE;
        prepareFrameType = FRAME_TYPE_TRANSITION_SLAVE;
        /* fall through */
    case DUAL_STANDBY_TRIGGER_SLAVE_ON:
        factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + camIdIndex];
        break;
    default:
        CLOG_ASSERT("invalid trigger(%d)", dualStandbyTrigger_info.mode);
        break;
    }

    if (standby == false) {
        uint32_t pipeId = PIPE_3AA;

        ExynosCameraFrameFactory *factoryTemp;
        int camIndexEnd = (camIdIndex == 0) ? 1: m_camIdInfo.numOfSensors;
        int camIndexStart = (camIdIndex == 0) ? camIdIndex: 1;

        /*
         * In case of master, no need to wait for the slave sensor status.
         * In case of Slave, no need to wait for the master sensor status.
         */
        CLOGD("camIndexStart = %d camIndexEnd = %d camIdIndex = %d", camIndexStart, camIndexEnd, camIdIndex);

        for (int i = camIndexStart; i < camIndexEnd; i++) {
            /* wait for real finish */
            factoryTemp = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + i];
            enum SENSOR_STANDBY_STATE state;
            int retryCount;
            bool tryRecovery = true;

TRY_STANDBY_RECOVERY:
            retryCount = 100; /* maximum 100ms(1frame) * 5 */
            state = factoryTemp->getSensorStandbyState(pipeId);
            while (state != SENSOR_STANDBY_ON && retryCount > 0) {
                usleep(5000); /* 5ms */
                retryCount--;
                CLOGW("waiting for previous standby off(%d) retry(%d) idx (%d) camIdIndex (%d)!!", state, retryCount, i, camIdIndex);
                state = factoryTemp->getSensorStandbyState(pipeId);
            }
            if (retryCount <= 0) {
                CLOGE("previous standby_on failed: cam_idx (%d) state(%d) cur_camIdIndex (%d)!!", i, state, camIdIndex);
                if (tryRecovery) {
                    CLOGD("Trying to sensorPipeForceDone() for the Cam (%d)", m_camIdInfo.cameraId[i]);
                    tryRecovery = false;
                    factoryTemp->sensorPipeForceDone();
                    goto TRY_STANDBY_RECOVERY;
                }
                goto THREAD_EXIT;
            }
            if (m_captureSelector[m_camIdInfo.cameraId[i]])
                m_captureSelector[m_camIdInfo.cameraId[i]]->clearList();
        }
    }

    if (isNeedPrepare == true) {
        /* It is for standbyHint */
        ret = factory->sensorStandby(standby, true);
        if (ret != NO_ERROR)
            CLOGE("sensorStandby(%d, true) fail! ret(%d)", standby, ret);

        // reset display info
        m_parameters[m_camIdInfo.cameraId[camIdIndex]]->initDisplayStatRoi();

        for (int i = 0; i < m_prepareFrameCount; i++) {
            ret = m_createInternalFrameFunc(NULL, false, REQ_SYNC_NONE, prepareFrameType, m_camIdInfo.cameraId[camIdIndex]);
            if (ret != NO_ERROR) {
                CLOGE("Failed to createFrameFunc for preparing frame. prepareCount %d/%d",
                        i, m_prepareFrameCount);
            } else {
                CLOGD("[CAM_ID%d] m_createInternalFrameFunc success. prepareCount %d/%d",
                    m_camIdInfo.cameraId[camIdIndex], i, m_prepareFrameCount);
            }
        }
    }

    ret = factory->sensorStandby(standby);
    if (ret != NO_ERROR) {
        CLOGE("sensorStandby(%s) fail! ret(%d)", (standby?"On":"Off"), ret);
    } else {
        switch (dualStandbyTrigger_info.mode) {
        case DUAL_STANDBY_TRIGGER_MASTER_OFF:
            m_parameters[m_camIdInfo.cameraId[camIdIndex]]->setStandbyState(DUAL_STANDBY_STATE_OFF);
            break;
        case DUAL_STANDBY_TRIGGER_MASTER_ON:
            m_parameters[m_camIdInfo.cameraId[camIdIndex]]->setStandbyState(DUAL_STANDBY_STATE_ON);
            break;
        case DUAL_STANDBY_TRIGGER_SLAVE_OFF:
            m_parameters[m_camIdInfo.cameraId[camIdIndex]]->setStandbyState(DUAL_STANDBY_STATE_OFF);
            break;
        case DUAL_STANDBY_TRIGGER_SLAVE_ON:
            m_parameters[m_camIdInfo.cameraId[camIdIndex]]->setStandbyState(DUAL_STANDBY_STATE_ON);
            break;
        default:
            break;
        }
        CLOGI("dualStandbyTriggerQ(%d) prepare(%d) trigger(%d) standby(camId:%d, State:%d)",
                m_dualStandbyTriggerQ->getSizeOfProcessQ(), m_prepareFrameCount, dualStandbyTrigger_info.mode,
                m_camIdInfo.cameraId[camIdIndex],
                m_parameters[m_camIdInfo.cameraId[camIdIndex]]->getStandbyState());
    }

THREAD_EXIT:
    if (m_dualStandbyTriggerQ->getSizeOfProcessQ() > 0) {
        CLOGI("run again. dualStandbyTriggerQ size %d", m_dualStandbyTriggerQ->getSizeOfProcessQ());
        return true;
    } else {
        CLOGI("Complete to dualStandbyTriggerQ");
        return false;
    }

    return ret;
}

status_t ExynosCamera::m_setBokehRefocusFusionYuvBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                                int fusionPipeId, int srcNodeId,
                                                                ExynosCameraBuffer &srcBuffer)
{
    status_t ret = NO_ERROR;
    ExynosRect rect = convertingBufferDst2Rect(&srcBuffer, V4L2_PIX_FMT_NV21);
    ExynosCameraBuffer dstBuffer;
    frame_handle_components_t components;
    int managerType = BUFFER_MANAGER_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    switch (frame->getFrameType()) {
    case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
    case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
        managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
        break;
    default:
        break;
    }
#endif
    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

    buffer_manager_tag_t bufTag;
    bufTag.pipeId[0] = srcNodeId;
    bufTag.managerType = managerType;
    ret = m_bufferSupplier->getBuffer(bufTag, &dstBuffer);
    if (ret != NO_ERROR || dstBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                frame->getFrameCount(), dstBuffer.index, ret);
        return ret;
    }

    int nodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION1_REPROCESSING);
    ret = frame->setDstBufferState(fusionPipeId, ENTITY_BUFFER_STATE_REQUESTED, nodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setDstBufferState. pipeId %d pos %d", fusionPipeId, nodeIndex);
    } else {
        ret = frame->setDstBuffer(fusionPipeId, dstBuffer, nodeIndex);
        if (ret != NO_ERROR) {
            CLOGE("setDstBuffer fail, pipeId(%d), ret(%d)",
                    fusionPipeId, ret);
            return ret;
        }

        // copy dst crop information from src to this target
        copyRectSrc2DstBuffer(&srcBuffer, &dstBuffer);
    }

    ret = frame->setDstRect(fusionPipeId, rect, nodeIndex);
    if(ret != NO_ERROR) {
        CLOGE("setDstRect fail, pipeId(%d), ret(%d)", PIPE_FUSION1_REPROCESSING, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setBokehRefocusFusionDepthBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                                    int fusionPipeId, int depthNodeIndex,
                                                                    ExynosCameraBuffer *srcBuffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer dstBuffer;
    buffer_manager_tag_t bufTag;
    frame_handle_components_t components;
    int managerType = BUFFER_MANAGER_ION_TYPE;
    int depthFormat = V4L2_PIX_FMT_Z16;

    bufTag.pipeId[0] = PIPE_FUSION2_REPROCESSING;
    bufTag.managerType = managerType;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    switch (frame->getFrameType()) {
    case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
    case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
        managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
        break;
    default:
        break;
    }
#endif

    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

    ret = m_bufferSupplier->getBuffer(bufTag, &dstBuffer);
    if (ret != NO_ERROR || dstBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                frame->getFrameCount(), dstBuffer.index, ret);
        return ret;
    }

    ret = frame->setDstBufferState(fusionPipeId, ENTITY_BUFFER_STATE_REQUESTED, depthNodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("Failed to setDstBufferState. pipeId %d pos %d", fusionPipeId, depthNodeIndex);
    } else {
         ret = frame->setDstBuffer(fusionPipeId, dstBuffer, depthNodeIndex);
        if (ret != NO_ERROR) {
            CLOGE("setDstBuffer fail, pipeId(%d), ret(%d)",
                    fusionPipeId, ret);
            return ret;
        }

        // copy dst crop information from src to this target
        copyRectSrc2DstBuffer(srcBuffer, &dstBuffer);
    }

    /* ToDo: Depth size setting */
    ExynosRect rect = convertingBufferDst2Rect(srcBuffer, depthFormat);
    ret = frame->setDstRect(fusionPipeId, rect, depthNodeIndex);
    if(ret != NO_ERROR) {
        CLOGE("setDstRect fail, pipeId(%d), ret(%d)", PIPE_FUSION2_REPROCESSING, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_returnBokehRefocusFusionDstBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                                        int fusionPipeId)
{
    ExynosCameraBuffer dstBuffer;
    frame_handle_components_t components;
    m_getFrameHandleComponentsWrapper(frame, &components);

    uint32_t nodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION1_REPROCESSING);
    status_t ret = frame->getDstBuffer(fusionPipeId, &dstBuffer, nodeIndex);
    if (ret != NO_ERROR || dstBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
                frame->getFrameCount(), dstBuffer.index, fusionPipeId, nodeIndex, ret);
    } else {
        ret = m_bufferSupplier->putBuffer(dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                    frame->getFrameCount(), dstBuffer.index, ret);
            return ret;
        }
    }

    nodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION2_REPROCESSING);
    ret = frame->getDstBuffer(fusionPipeId, &dstBuffer, nodeIndex);
    if (ret != NO_ERROR || dstBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
                frame->getFrameCount(), dstBuffer.index, fusionPipeId, nodeIndex, ret);
    } else {
        ret = m_bufferSupplier->putBuffer(dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                    frame->getFrameCount(), dstBuffer.index, ret);
            return ret;
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setBokehRefocusJpegSrcBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                        int srcPipeId, frame_handle_components_t components)
{
    /* Jpeg source buffer setting */
    ExynosCameraBuffer buffer;
    int jpegPipeId = PIPE_JPEG_REPROCESSING;
    uint32_t nodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION1_REPROCESSING);
    status_t ret = frame->getDstBuffer(srcPipeId, &buffer, nodeIndex);
    if (ret != NO_ERROR || buffer.index <= -1) {
        CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
            frame->getFrameCount(), buffer.index, srcPipeId, nodeIndex, ret);
        return ret;
    }

    nodeIndex = OUTPUT_NODE_2;
    ret = frame->setSrcBuffer(jpegPipeId, buffer, nodeIndex);
    if (ret != NO_ERROR || buffer.index <= -1) {
        CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
            frame->getFrameCount(), buffer.index, jpegPipeId, nodeIndex, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_composeBokehRefocusJpegBuffer(ExynosCameraFrameSP_sptr_t frame,
                                                            int bokehJpegPipeId,
                                                            int piepIdDepth,
                                                            ExynosCameraBuffer *jpegServiceBuffer)
{
    status_t ret = NO_ERROR;
    int piepIdJpeg = PIPE_JPEG_REPROCESSING;
    int masterJpegPipeId = PIPE_JPEG1_REPROCESSING;
    int bokehJpegSize = frame->getJpegEncodeSize(bokehJpegPipeId);
    int masterJpegSize = frame->getJpegEncodeSize(masterJpegPipeId);
    ExynosCameraBuffer bokehJpegBuffer, masterJpegbuffer, depthBuffer;
    frame_handle_components_t components;
    m_getFrameHandleComponentsWrapper(frame, &components);

    int nodeIndex = components.reprocessingFactory->getNodeType(bokehJpegPipeId);
    ret = frame->getDstBuffer(piepIdJpeg, &bokehJpegBuffer, nodeIndex);
    if (ret < 0) {
        CLOGE("getDstBuffer fail, pipeId(%d), ret(%d)", piepIdJpeg, ret);
        return ret;
    }

    nodeIndex = components.reprocessingFactory->getNodeType(masterJpegPipeId);
    ret = frame->getDstBuffer(piepIdJpeg, &masterJpegbuffer, nodeIndex);
    if (ret < 0) {
        CLOGE("getDstBuffer fail, pipeId(%d), ret(%d)", piepIdJpeg, ret);
        return ret;
    }

    buffer_manager_tag_t bufTag;
    int managerType = BUFFER_MANAGER_ION_TYPE;

    bufTag.pipeId[0] = PIPE_FUSION2_REPROCESSING;
    bufTag.managerType = managerType;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    switch (frame->getFrameType()) {
    case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
    case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
        managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
        break;
    default:
        break;
    }
#endif

    ret = m_bufferSupplier->getBuffer(bufTag, &depthBuffer);
    if (ret != NO_ERROR || depthBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                frame->getFrameCount(), depthBuffer.index, ret);
        return ret;
    }

    CLOGD("[F%d T%d] bokehJpegSize = %#x byte, masterJpegSize = %#x byte, depthSize = %#x",
            frame->getFrameCount(), frame->getFrameType(),
            bokehJpegSize, masterJpegSize, depthBuffer.size[0]);
#if 0 /* only dump */
    char filePath[70];

    memset(filePath, 0, sizeof(filePath));
    snprintf(filePath, sizeof(filePath), "/data/vendor/camera/CAM%d_F%d.jpeg",
            m_cameraId, frame->getFrameCount());
    ret = dumpToFile((char *)filePath, masterJpegbuffer.addr[0], masterJpegbuffer.size[0]);

    memset(filePath, 0, sizeof(filePath));
    snprintf(filePath, sizeof(filePath), "/data/vendor/camera/CAM%d_F%d.depth",
            m_cameraId, frame->getFrameCount());
    ret = dumpToFile((char *)filePath, depthBuffer.addr[0], depthBuffer.size[0]);
#endif

    ret = m_bufferSupplier->putBuffer(masterJpegbuffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]getSrcBuffer fail, pipeId(%d), ret(%d)",
            frame->getFrameCount(), piepIdJpeg, ret);
        return ret;
    }

    ret = m_bufferSupplier->putBuffer(depthBuffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]getSrcBuffer fail, pipeId(%d), ret(%d)",
            frame->getFrameCount(), piepIdJpeg, ret);
        return ret;
    }

    (*jpegServiceBuffer) = bokehJpegBuffer;

    return NO_ERROR;
}

status_t ExynosCamera::m_setBokehRefocusBuffer(bool isRemosaic)
{
    status_t ret = NO_ERROR;
    buffer_manager_tag_t bufTag;
    buffer_manager_configuration_t bufConfig;
    const buffer_manager_tag_t initBufTag;
    const buffer_manager_configuration_t initBufConfig;
    uint32_t width = 0, height = 0;
    m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, &width, &height);

    /* Refocus Sub Jpeg buffer */
    bufTag = initBufTag;
    bufTag.pipeId[0] = PIPE_JPEG1_REPROCESSING;
    if (isRemosaic)
        bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
    else
        bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
    ret = m_bufferSupplier->createBufferManager("REFOCUS_INTERNAL_JPEG_BUF", m_ionAllocator, bufTag);
    if (ret != NO_ERROR) {
        CLOGE("Failed to create REFOCUS_INTERNAL_JPEG_BUF. ret %d", ret);
        return ret;
    }

    bufConfig = initBufConfig;
    bufConfig.planeCount = 2;
    bufConfig.size[0] = width * height * 2;
    bufConfig.reqBufCount = 1;
    bufConfig.allowedMaxBufCount = bufConfig.reqBufCount;
    bufConfig.batchSize = m_parameters[m_cameraId]->getBatchSize((enum pipeline)bufTag.pipeId[0]);
    bufConfig.type = EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE;
    bufConfig.allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;
    bufConfig.createMetaPlane = true;
    bufConfig.createDebugInfoPlane = isSupportedDebugInfoPlane((enum pipeline)bufTag.pipeId[0], &(bufConfig.planeCount));
    bufConfig.needMmap = true;
    bufConfig.reservedMemoryCount = 0;
    bufConfig.debugInfo = {(int)width, (int)height, 0};

    ret = m_allocBuffers(bufTag, bufConfig);
    if (ret != NO_ERROR) {
        CLOGE("Failed to alloc REFOCUS_INTERNAL_JPEG_BUF. ret %d", ret);
        return ret;
    }

    /* Refocus Depth buffer */
    bufTag = initBufTag;
    bufTag.pipeId[0] = PIPE_FUSION2_REPROCESSING;
    if (isRemosaic)
        bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
    else
        bufTag.managerType = BUFFER_MANAGER_ION_TYPE;

    ret = m_bufferSupplier->createBufferManager("REFOCUS_INTERNAL_DEPTH_BUF", m_ionAllocator, bufTag);
    if (ret != NO_ERROR) {
        CLOGE("Failed to create REFOCUS_INTERNAL_DEPTH_BUF. ret %d", ret);
        return ret;
    }

    bufConfig = initBufConfig;
    bufConfig.planeCount = 2;
    bufConfig.size[0] = ROUND_UP(width * 2, CAMERA_16PX_ALIGN) * height; /* Depth format */
    bufConfig.reqBufCount = 1;
    bufConfig.allowedMaxBufCount = bufConfig.reqBufCount;
    bufConfig.batchSize = m_parameters[m_cameraId]->getBatchSize((enum pipeline)bufTag.pipeId[0]);
    bufConfig.type = EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE;
    bufConfig.allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;
    bufConfig.createMetaPlane = true;
    bufConfig.createDebugInfoPlane = isSupportedDebugInfoPlane((enum pipeline)bufTag.pipeId[0], &(bufConfig.planeCount));
    bufConfig.needMmap = true;
    bufConfig.reservedMemoryCount = 0;
    bufConfig.debugInfo = {(int)width, (int)height, 0};

    ret = m_allocBuffers(bufTag, bufConfig);
    if (ret != NO_ERROR) {
        CLOGE("Failed to alloc REFOCUS_INTERNAL_DEPTH_BUF. ret %d", ret);
        return ret;
    }

    return NO_ERROR;
}
#endif

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera::m_getSyncLogId(void)
{
    return ++cameraSyncLogId;
}
#endif

bool ExynosCamera::m_monitorThreadFunc(void)
{
    CLOGV("");

    int *threadState;
    uint64_t *timeInterval;
    int *pipeCountRenew, resultCountRenew;
    int ret = NO_ERROR;
    int loopCount = 0;
    int dtpStatus = 0;
    int pipeIdFlite = m_getBayerPipeId();
    int pipeIdErrorCheck = 0;
    uint32_t checkStreamCount = 1;
    frame_type_t frameType = FRAME_TYPE_PREVIEW;
    frame_handle_components_t components;
    ExynosCameraFrameFactory *factory = NULL;

    /* 1. MONITOR_THREAD_INTERVAL (0.2s)*/
    for (loopCount = 0; loopCount < MONITOR_THREAD_INTERVAL; loopCount += (MONITOR_THREAD_INTERVAL / 20)) {
        if (m_getState() == EXYNOS_CAMERA_STATE_FLUSH) {
            CLOGI("Flush is in progress");
            return false;
        }
        usleep(MONITOR_THREAD_INTERVAL / 20);
    }

    m_framefactoryCreateThread->join();
    m_dualFramefactoryCreateThread->join();

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_configurations->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
        checkStreamCount = 2;
    }
#endif

    for (uint32_t i = 1; i <= checkStreamCount; i++) {
        switch (i) {
#ifdef USE_DUAL_CAMERA
        case 2:
            frameType = FRAME_TYPE_PREVIEW_SLAVE;
            break;
#endif
        case 1:
        default:
#ifdef USE_DUAL_CAMERA
            if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
                && m_configurations->getDualOperationMode() == DUAL_OPERATION_MODE_SLAVE) {
                frameType = FRAME_TYPE_PREVIEW_SLAVE;
            } else
#endif
            {
                frameType = FRAME_TYPE_PREVIEW;
            }
            break;
        }

        m_getFrameHandleComponentsWrapper(frameType, &components);

        factory = components.previewFactory;
        if (factory == NULL) {
            CLOGE("previewFrameFactory is NULL. Skip monitoring.");
            return false;
        }

        if (factory->isCreated() == false) {
            CLOGE("previewFrameFactory is not Created. Skip monitoring.");
            return false;
        }

        bool flag3aaIspM2M = (components.parameters->getHwConnectionMode(PIPE_3AA, PIPE_ISP) == HW_CONNECTION_MODE_M2M);
        bool flagIspMcscM2M = (components.parameters->getHwConnectionMode(PIPE_ISP, PIPE_MCSC) == HW_CONNECTION_MODE_M2M);

        /* 2. Select Error check pipe ID (of last output node) */
        if (flagIspMcscM2M == true
                && IS_OUTPUT_NODE(factory, PIPE_MCSC) == true) {
            pipeIdErrorCheck = PIPE_MCSC;
        } else if (flag3aaIspM2M == true
                && IS_OUTPUT_NODE(factory, PIPE_ISP) == true) {
            pipeIdErrorCheck = PIPE_ISP;
        } else {
            pipeIdErrorCheck = PIPE_3AA;
        }

        if (factory->checkPipeThreadRunning(pipeIdErrorCheck) == false) {
            CLOGW("pipe(%d) is not running.. Skip monitoring.", pipeIdErrorCheck);
            return false;
        }

        /* 3. Monitor sync log output */
#ifdef MONITOR_LOG_SYNC
        uint32_t pipeIdLogSync = PIPE_3AA;

        /* If it is not front camera in PIP and sensor pipe is running, do sync log */
        if (factory->checkPipeThreadRunning(pipeIdLogSync)
            && !(getCameraId() == CAMERA_ID_FRONT && m_configurations->getMode(CONFIGURATION_PIP_MODE))) {
            if (!(m_syncLogDuration % MONITOR_LOG_SYNC_INTERVAL)) {
                uint32_t syncLogId = m_getSyncLogId();
                CLOGI("@FIMC_IS_SYNC %d", syncLogId);
                factory->syncLog(pipeIdLogSync, syncLogId);
            }
            m_syncLogDuration++;
        }
#endif

        /* 4. DTP status check */
        factory->getControl(V4L2_CID_IS_G_DTPSTATUS, &dtpStatus, pipeIdFlite);

        if (dtpStatus == 1) {
            CLOGE("DTP Detected. dtpStatus(%d)", dtpStatus);
            dump();

            /* Once this error is returned by some method, or if notify() is called with ERROR_DEVICE,
             * only the close() method can be called successfully. */
            m_requestMgr->notifyDeviceError();
            ret = m_transitState(EXYNOS_CAMERA_STATE_ERROR);
            if (ret != NO_ERROR) {
                CLOGE("Failed to transitState into ERROR. ret %d", ret);
            }

#ifdef SENSOR_OVERFLOW_CHECK
            android_printAssert(NULL, LOG_TAG, "killed by itself");
#endif
            return false;
        }

        /* 5. ESD check */
        if (m_streamManager->findStream(HAL_STREAM_ID_PREVIEW) == true) {
            factory->getThreadState(&threadState, pipeIdErrorCheck);
            factory->getThreadRenew(&pipeCountRenew, pipeIdErrorCheck);
            if ((*threadState == ERROR_POLLING_DETECTED) || (*pipeCountRenew > ERROR_DQ_BLOCKED_COUNT)) {
                CLOGE("ESD Detected. threadState(%d) pipeCountRenew(%d)", *threadState, *pipeCountRenew);
                goto ERROR;
            }

            CLOGV("Thread Renew Count [%d]", *pipeCountRenew);
            factory->incThreadRenew(pipeIdErrorCheck);
        }

        /* 6. Result Callback Error check */
        if (m_getState() == EXYNOS_CAMERA_STATE_RUN) {
            int error_delay_count = 0;
#ifdef USE_LONGEXPOSURECAPTURE
            /*
                in case of long exposure capture scenario.
                sensor support long exposure 32s, Device Error Detected wait time need to increase
            */
            if (m_configurations->getMode(CONFIGURATION_SESSION_MODE)
                &&  m_configurations->getModeMultiValue(CONFIGURATION_MULTI_SESSION_MODE_VALUE, EXYNOS_SESSION_MODE_PRO)) {
                error_delay_count  = ERROR_RESULT_LONGEXPOSURECAPTURE_DALAY_COUNT;
            } else
#endif
            {
                error_delay_count  = ERROR_RESULT_DALAY_COUNT;
            }
            resultCountRenew = m_requestMgr->getResultRenew();
            if (resultCountRenew > error_delay_count) {
                CLOGE("Device Error Detected. Probably there was not result callback about %.2f seconds. resultCountRenew(%d)",
                        (float)MONITOR_THREAD_INTERVAL / 1000000 * (float)error_delay_count, resultCountRenew);
                m_requestMgr->dump();
                goto ERROR;
            }

            CLOGV("Result Renew Count [%d]", resultCountRenew);
            m_requestMgr->incResultRenew();
        } else {
            m_requestMgr->resetResultRenew();
        }

        factory->getThreadInterval(&timeInterval, pipeIdErrorCheck);
        CLOGV("Thread IntervalTime [%ju]", *timeInterval);
    }

    return true;

ERROR:
    dump();

    /* Once this error is returned by some method, or if notify() is called with ERROR_DEVICE,
     * only the close() method can be called successfully. */
    m_requestMgr->notifyDeviceError();
    ret = m_transitState(EXYNOS_CAMERA_STATE_ERROR);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState into ERROR. ret %d", ret);
    }

    return false;
}

#ifdef BUFFER_DUMP
bool ExynosCamera::m_dumpThreadFunc(void)
{
    char filePath[70];
    int imagePlaneCount = -1;
    status_t ret = NO_ERROR;
    buffer_dump_info_t bufDumpInfo;

    ret = m_dumpBufferQ->waitAndPopProcessQ(&bufDumpInfo);
    if (ret == TIMED_OUT) {
        CLOGW("Time-out to wait dumpBufferQ");
        goto THREAD_EXIT;
    } else if (ret != NO_ERROR) {
        CLOGE("Failed to waitAndPopProcessQ for dumpBufferQ");
        goto THREAD_EXIT;
    }

    memset(filePath, 0, sizeof(filePath));
    snprintf(filePath, sizeof(filePath), "/data/media/0/%s_CAM%d_F%d_%c%c%c%c_%dx%d.img",
            bufDumpInfo.name,
            m_cameraId,
            bufDumpInfo.frameCount,
            v4l2Format2Char(bufDumpInfo.format, 0),
            v4l2Format2Char(bufDumpInfo.format, 1),
            v4l2Format2Char(bufDumpInfo.format, 2),
            v4l2Format2Char(bufDumpInfo.format, 3),
            bufDumpInfo.width, bufDumpInfo.height);

    imagePlaneCount = bufDumpInfo.buffer.planeCount - 1;

    switch (imagePlaneCount) {
    case 2:
        if (bufDumpInfo.buffer.addr[1] == NULL) {
            android_printAssert(NULL, LOG_TAG, "[CAM_ID(%d)][%s]-ASSERT(%s[%d]):[F%d B%d P1]%s_Buffer do NOT have virtual address",
                    m_cameraId, m_name, __FUNCTION__, __LINE__,
                    bufDumpInfo.frameCount, bufDumpInfo.buffer.index, bufDumpInfo.name);
            goto THREAD_EXIT;
        }
    case 1:
        if (bufDumpInfo.buffer.addr[0] == NULL) {
            android_printAssert(NULL, LOG_TAG, "[CAM_ID(%d)][%s]-ASSERT(%s[%d]):[F%d B%d P0]%s_Buffer do NOT have virtual address",
                    m_cameraId, m_name, __FUNCTION__, __LINE__,
                    bufDumpInfo.frameCount, bufDumpInfo.buffer.index, bufDumpInfo.name);
            goto THREAD_EXIT;
        }
    default:
        break;
    }

    if (imagePlaneCount > 1) {
        ret = dumpToFile2plane((char *)filePath,
                               bufDumpInfo.buffer.addr[0], bufDumpInfo.buffer.addr[1],
                               bufDumpInfo.buffer.size[0], bufDumpInfo.buffer.size[1]);
    } else {
        ret = dumpToFile((char *)filePath, bufDumpInfo.buffer.addr[0], bufDumpInfo.buffer.size[0]);
    }
    if (ret == false) {
        CLOGE("[F%d B%d]Failed to dumpToFile. FilePath %s size %d/%d",
                bufDumpInfo.frameCount, bufDumpInfo.buffer.index, filePath,
                bufDumpInfo.buffer.size[0], bufDumpInfo.buffer.size[1]);
        goto THREAD_EXIT;
    }

THREAD_EXIT:
    if (bufDumpInfo.buffer.index > -1) {
        m_bufferSupplier->putBuffer(bufDumpInfo.buffer);
    }

    if (m_dumpBufferQ->getSizeOfProcessQ() > 0) {
        CLOGI("run again. dumpBufferQ size %d",
                 m_dumpBufferQ->getSizeOfProcessQ());
        return true;
    } else {
        CLOGI("Complete to dumpFrame.");
        return false;
    }
}
#endif

status_t ExynosCamera::m_transitState(ExynosCamera::exynos_camera_state_t state)
{
    int ret = NO_ERROR;

    Mutex::Autolock lock(m_stateLock);

    CLOGV("State transition. curState %d newState %d", m_state, state);

    if (state == EXYNOS_CAMERA_STATE_ERROR) {
        CLOGE("Exynos camera state is ERROR! The device close of framework must be called.");
    }

    switch (m_state) {
    case EXYNOS_CAMERA_STATE_OPEN:
        if (state != EXYNOS_CAMERA_STATE_INITIALIZE
            && state != EXYNOS_CAMERA_STATE_ERROR) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
    case EXYNOS_CAMERA_STATE_INITIALIZE:
        if (state != EXYNOS_CAMERA_STATE_CONFIGURED
            && state != EXYNOS_CAMERA_STATE_FLUSH
            && state != EXYNOS_CAMERA_STATE_ERROR) {
        /* GED Camera appliction calls flush() directly after calling initialize(). */
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
    case EXYNOS_CAMERA_STATE_CONFIGURED:
        if (state != EXYNOS_CAMERA_STATE_CONFIGURED
            && state != EXYNOS_CAMERA_STATE_START
            && state != EXYNOS_CAMERA_STATE_FLUSH
            && state != EXYNOS_CAMERA_STATE_ERROR) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
    case EXYNOS_CAMERA_STATE_START:
        if (state == EXYNOS_CAMERA_STATE_FLUSH) {
            /* When FLUSH state is requested on START state, it must wait state to transit to RUN.
            * On FLUSH state, Frame factory will be stopped.
            * On START state, Frame factory will be run.
            * They must be done sequencially
            */

            /* HACK
            * The original retryCount is 10 (1s), : int retryCount = 10;
            * Due to the sensor entry time issues, it has increased to 30 (3s).
            */
            int retryCount = 30;

            while (m_state == EXYNOS_CAMERA_STATE_START && retryCount-- > 0) {
                CLOGI("Wait to RUN state");
                m_stateLock.unlock();
                /* Sleep 100ms */
                usleep(100000);
                m_stateLock.lock();
            }

            if (retryCount < 0 && m_state == EXYNOS_CAMERA_STATE_START) {
#ifdef AVOID_ASSERT_WAIT_STATE_FLUSH
                CLOGI("Failed to wait EXYNOS_CAMERA_RUN state skip Wait. curState %d newState %d", m_state, state);
#else
                android_printAssert(NULL, LOG_TAG,
                        "ASSERT(%s):Failed to wait EXYNOS_CAMERA_RUN state. curState %d newState %d",
                        __FUNCTION__, m_state, state);
#endif
            }
        } else if (state != EXYNOS_CAMERA_STATE_RUN
            && state != EXYNOS_CAMERA_STATE_ERROR) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }

        break;
    case EXYNOS_CAMERA_STATE_RUN:
        if (state != EXYNOS_CAMERA_STATE_FLUSH
            && state != EXYNOS_CAMERA_STATE_ERROR
#ifdef SUPPORT_SENSOR_MODE_CHANGE
            && state != EXYNOS_CAMERA_STATE_SWITCHING_SENSOR
#endif
        ) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
    case EXYNOS_CAMERA_STATE_FLUSH:
        if (state != EXYNOS_CAMERA_STATE_CONFIGURED
            && state != EXYNOS_CAMERA_STATE_ERROR
        ) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
    case EXYNOS_CAMERA_STATE_ERROR:
        if (state != EXYNOS_CAMERA_STATE_ERROR) {
            /*
             * Camera HAL device ops functions that have a return value will all return -ENODEV / NULL
             * in case of a serious error.
             * This means the device cannot continue operation, and must be closed by the framework.
             */
            ret = NO_INIT;
            goto ERR_EXIT;
        }
        break;
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    case EXYNOS_CAMERA_STATE_SWITCHING_SENSOR:
        if (state != EXYNOS_CAMERA_STATE_RUN) {
            ret = INVALID_OPERATION;
            goto ERR_EXIT;
        }
        break;
#endif
    default:
        CLOGW("Invalid curState %d maxValue %d", state, EXYNOS_CAMERA_STATE_MAX);
        ret = INVALID_OPERATION;
        goto ERR_EXIT;
    }

    m_state = state;
    return NO_ERROR;

ERR_EXIT:
    CLOGE("Invalid state transition. curState %d newState %d", m_state, state);
    return ret;
}

ExynosCamera::exynos_camera_state_t ExynosCamera::m_getState(void)
{
    Mutex::Autolock lock(m_stateLock);

    return m_state;
}

status_t ExynosCamera::m_checkRestartStream(ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;
    bool restart = m_configurations->getRestartStream();

    if (restart) {
        CLOGD("Internal restart stream request[R%d]",
             request->getKey());
        ret = m_restartStreamInternal();
        if (ret != NO_ERROR) {
            CLOGE("m_restartStreamInternal failed [%d]", ret);
            return ret;
        }

        m_requestMgr->registerToServiceList(request);
        m_configurations->setRestartStream(false);
    }

    return ret;
}

#ifdef FPS_CHECK
void ExynosCamera::m_debugFpsCheck(enum pipeline pipeId)
{
    uint32_t debugMaxPipeNum = sizeof(m_debugFpsCount);
    uint32_t id = pipeId % debugMaxPipeNum;
    uint32_t debugStartCount = 1, debugEndCount = 30;

    m_debugFpsCount[id]++;

    if (m_debugFpsCount[id] == debugStartCount) {
        m_debugFpsTimer[id].start();
    }

    if (m_debugFpsCount[id] == debugStartCount + debugEndCount) {
        m_debugFpsTimer[id].stop();
        long long durationTime = m_debugFpsTimer[id].durationMsecs();
        CLOGI("FPS_CHECK(id:%d), duration %lld / 30 = %lld ms. %lld fps",
                pipeId, durationTime, durationTime / debugEndCount, 1000 / (durationTime / debugEndCount));
        m_debugFpsCount[id] = 0;
    }
}
#endif

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
bool ExynosCamera::m_reverseProcessingBayerThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    bool loop = false;
    int dstPos = -1;
    int leaderPipeId = PIPE_3AA_REPROCESSING;
    entity_buffer_state_t bufferState = ENTITY_BUFFER_STATE_INVALID;

    ExynosCameraRequestSP_sprt_t request = NULL;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    ExynosCameraBuffer dstBuffer;
    ExynosCameraBuffer serviceBuffer;
    frame_handle_components_t components;

    CLOGV("Wait reverseProcessingBayerQ");
    ret = m_reverseProcessingBayerQ->waitAndPopProcessQ(&frame);
    if (ret == TIMED_OUT) {
        CLOGV("Wait timeout to reverseProcessingBayerQ");
        loop = true;
        goto p_err;
    } else if (ret != NO_ERROR) {
        CLOGE("Failed to waitAndPopProcessQ to reverseProcessingBayerQ");
        goto p_err;
    } else if (frame == NULL) {
        CLOGE("Frame is NULL");
        goto p_err;
    }

    ExynosCameraRequestManager* requestMgr = m_getRequestManager(frame);

    request = requestMgr->getRunningRequest(frame->getFrameCount());
    if (request == NULL) {
        CLOGE("[F%d]Failed to get request.", frame->getFrameCount());
        return INVALID_OPERATION;
    }

    m_getFrameHandleComponentsWrapper(frame, &components);
    dstPos = components.reprocessingFactory->getNodeType(PIPE_3AC_REPROCESSING);

    ret = frame->getDstBuffer(leaderPipeId, &dstBuffer, dstPos);
    if (ret != NO_ERROR || dstBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getDstBuffer. pos %d. ret %d",
                frame->getFrameCount(), dstBuffer.index, dstPos, ret);
        goto p_err;
    }

    ret = frame->getDstBufferState(leaderPipeId, &bufferState, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]Failed to getDstBufferState. pos %d. ret %d",
                frame->getFrameCount(), dstBuffer.index, dstPos, ret);
        goto p_err;
    }

    if (bufferState == ENTITY_BUFFER_STATE_ERROR) {
        CLOGE("[R%d F%d B%d]Invalid RAW buffer state. bufferState %d",
                request->getKey(), frame->getFrameCount(), dstBuffer.index, bufferState);
        request->setStreamBufferStatus(HAL_STREAM_ID_RAW, CAMERA3_BUFFER_STATUS_ERROR);
        goto p_err;
    }

    /* HACK: get service buffer backuped by output node index */
    ret = frame->getDstBuffer(leaderPipeId, &serviceBuffer, OUTPUT_NODE_1);
    if (ret != NO_ERROR || serviceBuffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getDstBuffer for serviceBuffer. pos %d. ret %d",
                frame->getFrameCount(), serviceBuffer.index, OUTPUT_NODE_1, ret);
        return ret;
    }

    /* reverse the raw buffer */
    ret = m_reverseProcessingBayer(frame, &dstBuffer, &serviceBuffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]Failed m_reverseProcessingBayer %d",
                frame->getFrameCount(), dstBuffer.index, ret);
    }

    ret = m_bufferSupplier->putBuffer(dstBuffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]Failed to putBuffer for bayerBuffer. ret %d",
                frame->getFrameCount(), dstBuffer.index, ret);
    }

    ret = m_sendRawStreamResult(request, &serviceBuffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d B%d]Failed to sendRawStreamResult. ret %d",
                frame->getFrameCount(), dstBuffer.index, ret);
    }

p_err:

    return loop;
}
#endif

#ifdef SUPPORT_HW_GDC
bool ExynosCamera::m_gdcThreadFunc(void)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    status_t ret = NO_ERROR;
    int outputPortId = -1;
    int srcPipeId = -1;
    int srcNodeIndex = -1;
    int dstPipeId = -1;
    int srcFmt = -1;
    int dstFmt = -1;
    ExynosRect gdcSrcRect[2];
    ExynosRect gdcDstRect;
    ExynosCameraBuffer gdcSrcBuf;
    ExynosCameraBuffer gdcDstBuf;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    frame_handle_components_t components;
    struct camera2_stream *meta = NULL;
    camera2_node_group bcropNodeGroupInfo;
    int perframeInfoIndex = -1;

    outputPortId = m_streamManager->getOutputPortId(HAL_STREAM_ID_VIDEO);
    if (outputPortId < 0) {
        CLOGE("There is NO VIDEO stream");
        goto exit_thread_loop;
    }

    CLOGV("Wait gdcQ");
    ret = m_gdcQ->waitAndPopProcessQ(&frame);
    if (ret == TIMED_OUT) {
        CLOGW("Wait timeout to gdcQ");
        goto retry_thread_loop;
    } else if (ret != NO_ERROR) {
        CLOGE("Failed to waitAndPopProcessQ to gdcQ");
        goto retry_thread_loop;
    } else if (frame == NULL) {
        CLOGE("Frame is NULL");
        goto retry_thread_loop;
    } else {
        m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

        ExynosCameraFrameFactory *factory = components.previewFactory;

        bool flag3aaIspM2M = (components.parameters->getHwConnectionMode(PIPE_3AA, PIPE_ISP) == HW_CONNECTION_MODE_M2M);
        bool flagIspMcscM2M = (components.parameters->getHwConnectionMode(PIPE_ISP, PIPE_MCSC) == HW_CONNECTION_MODE_M2M);

        if (flagIspMcscM2M == true
            && IS_OUTPUT_NODE(factory, PIPE_MCSC) == true) {
            srcPipeId = PIPE_MCSC;
        } else if (flag3aaIspM2M == true
                && IS_OUTPUT_NODE(factory, PIPE_ISP) == true) {
            srcPipeId = PIPE_ISP;
        } else {
            srcPipeId = PIPE_3AA;
        }

        srcNodeIndex = factory->getNodeType(PIPE_MCSC0 + outputPortId);
        dstPipeId = PIPE_GDC;
        srcFmt = m_configurations->getYuvFormat(outputPortId);
        dstFmt = m_configurations->getYuvFormat(outputPortId);

        /* Set Source Rect */
        ret = frame->getDstBuffer(srcPipeId, &gdcSrcBuf, srcNodeIndex);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getDstBuffer. pipeId %d nodeIndex %d ret %d",
                    frame->getFrameCount(),
                    srcPipeId, srcNodeIndex, ret);
            goto retry_thread_loop;
        }

        meta = (struct camera2_stream *)gdcSrcBuf.addr[gdcSrcBuf.getMetaPlaneIndex()];
        if (meta == NULL) {
            CLOGE("[F%d B%d]meta is NULL. pipeId %d nodeIndex %d",
                    frame->getFrameCount(), gdcSrcBuf.index,
                    srcPipeId, srcNodeIndex);
            goto retry_thread_loop;
        }

        gdcSrcRect[0].x             = meta->output_crop_region[0];
        gdcSrcRect[0].y             = meta->output_crop_region[1];
        gdcSrcRect[0].w             = meta->output_crop_region[2];
        gdcSrcRect[0].h             = meta->output_crop_region[3];
        gdcSrcRect[0].fullW         = meta->output_crop_region[2];
        gdcSrcRect[0].fullH         = meta->output_crop_region[3];
        gdcSrcRect[0].colorFormat   = srcFmt;

        /* Set Bcrop Rect */
        perframeInfoIndex = (components.parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) ? PERFRAME_INFO_3AA : PERFRAME_INFO_FLITE;
        frame->getNodeGroupInfo(&bcropNodeGroupInfo, perframeInfoIndex);
        components.parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&gdcSrcRect[1].fullW, (uint32_t *)&gdcSrcRect[1].fullH);

        gdcSrcRect[1].x             = bcropNodeGroupInfo.leader.input.cropRegion[0];
        gdcSrcRect[1].y             = bcropNodeGroupInfo.leader.input.cropRegion[1];
        gdcSrcRect[1].w             = bcropNodeGroupInfo.leader.input.cropRegion[2];
        gdcSrcRect[1].h             = bcropNodeGroupInfo.leader.input.cropRegion[3];
        gdcSrcRect[1].colorFormat   = srcFmt;

        /* Set Destination Rect */
        gdcDstRect.x            = 0;
        gdcDstRect.y            = 0;
        gdcDstRect.w            = gdcSrcRect[0].w;
        gdcDstRect.h            = gdcSrcRect[0].h;
        gdcDstRect.fullW        = gdcSrcRect[0].fullW;
        gdcDstRect.fullH        = gdcSrcRect[0].fullH;
        gdcDstRect.colorFormat  = dstFmt;

        for (int i = 0; i < SRC_BUFFER_COUNT_MAX; i++) {
            ret = frame->setSrcRect(dstPipeId, gdcSrcRect[i], i);
            if (ret != NO_ERROR) {
                CLOGE("[F%d]Failed to setSrcRect. pipeId %d index %d ret %d",
                        frame->getFrameCount(), dstPipeId, i, ret);
                goto retry_thread_loop;
            }
        }

        ret = frame->setDstRect(dstPipeId, gdcDstRect);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to setDstRect. pipeId %d ret %d",
                    frame->getFrameCount(), dstPipeId, ret);
            goto retry_thread_loop;
        }

        ret = m_setSrcBuffer(dstPipeId, frame, &gdcSrcBuf);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to setSrcBuffer. pipeId %d ret %d",
                    frame->getFrameCount(), gdcSrcBuf.index,
                    dstPipeId, ret);
            goto retry_thread_loop;
        }

        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[dstPipeId], dstPipeId);
        factory->pushFrameToPipe(frame, dstPipeId);
    }

    return true;

retry_thread_loop:
    if (m_gdcQ->getSizeOfProcessQ() > 0) {
        return true;
    }

exit_thread_loop:
    return false;
}
#endif

int ExynosCamera::m_getMcscLeaderPipeId(frame_handle_components_t *components)
{
    uint32_t pipeId = -1;

    if (components == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):components is NULL", __FUNCTION__, __LINE__);
    }

    uint32_t cameraId = (uint32_t)components->reprocessingFactory->getCameraId();

    bool flag3aaIspM2M = (components->parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_ISP_REPROCESSING) == HW_CONNECTION_MODE_M2M);
    bool flagIspMcscM2M = (components->parameters->getHwConnectionMode(PIPE_ISP_REPROCESSING, PIPE_MCSC_REPROCESSING) == HW_CONNECTION_MODE_M2M);

    if (flagIspMcscM2M == true
        && IS_OUTPUT_NODE(components->reprocessingFactory, PIPE_MCSC_REPROCESSING) == true) {
        pipeId = PIPE_MCSC_REPROCESSING;
    } else if (flag3aaIspM2M == true
            && IS_OUTPUT_NODE(components->reprocessingFactory, PIPE_ISP_REPROCESSING) == true) {
        pipeId = PIPE_ISP_REPROCESSING;
    } else {
        pipeId = PIPE_3AA_REPROCESSING;
    }

    return pipeId;
}

int ExynosCamera::m_getCurrentCamIdx(__unused bool isReprocessing, int camType)
{
    //this function returns 0 or 1 (default) when it is failed to find the actual master/slave cam.
    int32_t CamIdx;

    if (m_camIdInfo.numOfSensors <= 1) {
        if (camType == MAIN_CAM) {
            CamIdx = 0;
        } else {
            CamIdx = 1;
        }
        return CamIdx;
    }

#ifdef USE_DUAL_CAMERA
    int32_t masterCamId, slaveCamId;
    enum DUAL_OPERATION_SENSORS dualOperationSensor;

    dualOperationSensor = m_configurations->getDualOperationSensor(isReprocessing);

    m_getCameraIdFromOperationSensor(dualOperationSensor, &masterCamId, &slaveCamId);

    if (camType == MAIN_CAM) {
        masterCamId = (masterCamId >= 0) ? masterCamId: m_camIdInfo.cameraId[MAIN_CAM];
        CamIdx = getCameraIdx(masterCamId);
        CamIdx = (CamIdx >= 0) ? CamIdx: 0;
    } else {
        if (slaveCamId < 0) {
            if (m_camIdInfo.cameraId[SUB_CAM2] >= 0) {
                if (m_parameters[m_camIdInfo.cameraId[SUB_CAM2]]->getStandbyState() >= DUAL_STANDBY_STATE_OFF) {
                    slaveCamId = m_camIdInfo.cameraId[SUB_CAM2];
                } else {
                    slaveCamId = m_camIdInfo.cameraId[SUB_CAM];
                }
            } else {
                slaveCamId = m_camIdInfo.cameraId[SUB_CAM];
            }
        }

        CamIdx = getCameraIdx(slaveCamId);
        CamIdx = (CamIdx > 0) ? CamIdx: 1;
     }
#else
    if (camType == MAIN_CAM) {
        CamIdx = 0;
    } else {
        CamIdx = 1;
    }
#endif

    return CamIdx;
}

int ExynosCamera::m_getCurrentCamIdx(__unused bool isReprocessing, int camType, frame_handle_components_t *components)
{
    //this function returns 0 or 1 (default) when it is failed to find the actual master/slave cam.
    int32_t CamIdx;

    if (m_camIdInfo.numOfSensors <= 1) {
        if (camType == MAIN_CAM) {
            CamIdx = 0;
        } else {
            CamIdx = 1;
        }
        return CamIdx;
    }

#ifdef USE_DUAL_CAMERA
    int32_t masterCamId, slaveCamId;
    enum DUAL_OPERATION_SENSORS dualOperationSensor;

    dualOperationSensor = components->configuration->getDualOperationSensor(isReprocessing);

    components->configuration->getCameraIdFromOperationSensor(dualOperationSensor, &masterCamId, &slaveCamId);

    if (camType == MAIN_CAM) {
        masterCamId = (masterCamId >= 0) ? masterCamId: m_camIdInfo.cameraId[MAIN_CAM];
        CamIdx = getCameraIdx(masterCamId);
        CamIdx = (CamIdx >= 0) ? CamIdx: 0;
    } else {
        if (slaveCamId >= 0) {
            slaveCamId = slaveCamId;
        } else {
            if (m_camIdInfo.cameraId[SUB_CAM2] >= 0) {
                if (components->parameters->getStandbyState() >= DUAL_STANDBY_STATE_OFF) {
                    slaveCamId = m_camIdInfo.cameraId[SUB_CAM2];
                } else {
                    slaveCamId = m_camIdInfo.cameraId[SUB_CAM];
                }
            } else {
                slaveCamId = m_camIdInfo.cameraId[SUB_CAM];
            }
        }

        CamIdx = getCameraIdx(slaveCamId);
        CamIdx = (CamIdx > 0) ? CamIdx: 1;
     }
#else
    if (camType == MAIN_CAM) {
        CamIdx = 0;
    } else {
        CamIdx = 1;
    }
#endif

    return CamIdx;
}

void ExynosCamera::m_getFrameHandleComponentsWrapper(ExynosCameraFrameSP_sptr_t frame, frame_handle_components_t *components, int cameraSessionId)
{
    m_getFrameHandleComponents((frame_type_t)frame->getFrameType(), components, frame->getCameraId(), cameraSessionId);
}

void ExynosCamera::m_getFrameHandleComponentsWrapper(frame_type_t frameType, frame_handle_components_t *components, int cameraId, int cameraSessionId)
{
    switch (frameType) {
#ifdef USE_DUAL_CAMERA
        case FRAME_TYPE_PREVIEW_SLAVE:
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
        case FRAME_TYPE_INTERNAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_SLAVE:
        case FRAME_TYPE_TRANSITION_SLAVE:
            int32_t slaveCamIdx;
            if (cameraId < 0) {
                slaveCamIdx = m_getCurrentCamIdx(false, SUB_CAM);
                cameraId = m_camIdInfo.cameraId[slaveCamIdx];
            }
            break;
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
        case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
            cameraId = m_cameraId;
            break;
#endif //SUPPORT_SENSOR_MODE_CHANGE
        default:
            cameraId = m_cameraId;
            break;
    }

    m_getFrameHandleComponents(frameType, components, cameraId, cameraSessionId);
}

void ExynosCamera::m_getFrameHandleComponents(frame_type_t frameType, frame_handle_components_t *components, int cameraId, int cameraSessionId)
{
    int32_t camIdx = getCameraIdx(cameraId);
    if (camIdx < 0) {
        CLOG_ASSERT("invalid cameraId!! (frameType:%d, cameraId:%d, session:%d)",
            frameType, cameraId, cameraSessionId);
    }

    ExynosCameraResourceManager::resources_t* resources = m_resourceManager->getResource(cameraSessionId);

    components->configuration = resources->configuration;
    components->streamMgr = resources->streamMgr;

    switch (frameType) {
#ifdef USE_DUAL_CAMERA
        case FRAME_TYPE_PREVIEW_SLAVE:
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
        case FRAME_TYPE_INTERNAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_SLAVE:
        case FRAME_TYPE_TRANSITION_SLAVE:
            components->previewFactory = resources->frameFactories[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW + camIdx];
            components->parameters = resources->parameters[cameraId];
            components->activityControl = resources->activityControls[cameraId];
            components->captureSelector = resources->captureProperty.frameSelector[cameraId];
            components->currentCameraId = cameraId;
            components->reprocessingFactory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING + camIdx];
            if ((frameType == FRAME_TYPE_REPROCESSING_SLAVE)
                || (frameType == FRAME_TYPE_REPROCESSING_DUAL_SLAVE)) {
                components->currentCameraId = cameraId;
            }
            break;
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
        case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
            components->previewFactory = resources->frameFactories[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
#ifdef SUPPORT_REMOSAIC_CAPTURE
            components->reprocessingFactory = resources->frameFactories[FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING];
#else
            components->reprocessingFactory = resources->frameFactories[FRAME_FACTORY_TYPE_REPROCESSING];
#endif //SUPPORT_REMOSAIC_CAPTURE
            components->parameters = resources->parameters[cameraId];
            components->activityControl = resources->activityControls[cameraId];
            components->captureSelector = resources->captureProperty.frameSelector[cameraId];
            components->currentCameraId = cameraId;
            break;
#endif //SUPPORT_SENSOR_MODE_CHANGE
        default:
            components->previewFactory = resources->frameFactories[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
            components->reprocessingFactory = resources->frameFactories[FRAME_FACTORY_TYPE_REPROCESSING];
            components->parameters = resources->parameters[cameraId];
            components->activityControl = resources->activityControls[cameraId];
            components->captureSelector = resources->captureProperty.frameSelector[cameraId];
            components->currentCameraId = cameraId;
            break;
    }
}

status_t ExynosCamera::m_updateFDAEMetadata(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *factory = NULL;
    camera2_shot_ext frameShotExt;
    fdae_info_t fdaeInfo;
    struct v4l2_ext_control extCtrl;
    struct v4l2_ext_controls extCtrls;

    ret = frame->getMetaData(&frameShotExt);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getMetaData. ret %d",
                frame->getFrameCount(), ret);
        return ret;
    }

    memset(&fdaeInfo, 0x00, sizeof(fdaeInfo));
    memset(&extCtrl, 0x00, sizeof(extCtrl));
    memset(&extCtrls, 0x00, sizeof(extCtrls));

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    fdaeInfo.frameCount = frameShotExt.shot.dm.request.frameCount;

    for (int index = 0; index < NUM_OF_DETECTED_FACES; index++) {
        if (frameShotExt.hfd.faceIds[index] > 0) {
            fdaeInfo.id[index]          = frameShotExt.hfd.faceIds[index];
            fdaeInfo.score[index]       = frameShotExt.hfd.score[index];
            fdaeInfo.rect[index][X1]    = frameShotExt.hfd.faceRectangles[index][X1];
            fdaeInfo.rect[index][Y1]    = frameShotExt.hfd.faceRectangles[index][Y1];
            fdaeInfo.rect[index][X2]    = frameShotExt.hfd.faceRectangles[index][X2];
            fdaeInfo.rect[index][Y2]    = frameShotExt.hfd.faceRectangles[index][Y2];
            fdaeInfo.isRot[index]       = frameShotExt.hfd.is_rot[index];
            fdaeInfo.rot[index]         = frameShotExt.hfd.rot[index];
            fdaeInfo.faceNum           += 1;

            CLOGV("[F%d(%d)]id %d score %d rect %d,%d %dx%d isRot %d rot %d",
                    frame->getFrameCount(),
                    fdaeInfo.frameCount,
                    fdaeInfo.id[index],
                    fdaeInfo.score[index],
                    fdaeInfo.rect[index][X1],
                    fdaeInfo.rect[index][Y1],
                    fdaeInfo.rect[index][X2],
                    fdaeInfo.rect[index][Y2],
                    fdaeInfo.isRot[index],
                    fdaeInfo.rot[index]);
        }
    }

    CLOGV("[F%d(%d)]setExtControl for FDAE. FaceNum %d",
            frame->getFrameCount(), fdaeInfo.frameCount, fdaeInfo.faceNum);

    extCtrl.id = V4L2_CID_IS_FDAE;
    extCtrl.ptr = &fdaeInfo;

    extCtrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    extCtrls.count = 1;
    extCtrls.controls = &extCtrl;

    ret = factory->setExtControl(&extCtrls, PIPE_VRA);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to setExtControl(V4L2_CID_IS_FDAE). ret %d",
                frame->getFrameCount(), ret);
    }

    return ret;
}


#ifdef USES_CAMERA_EXYNOS_VPL
status_t ExynosCamera::m_updateNFDInfo(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *factory = NULL;
    fd_info fdInfo;
    struct v4l2_ext_control extCtrl;
    struct v4l2_ext_controls extCtrls;
    enum NODE_TYPE nodeType;
    int pipeId;
    frame_handle_components_t components;

    memset(&fdInfo, 0x00, sizeof(fdInfo));
    memset(&extCtrl, 0x00, sizeof(extCtrl));
    memset(&extCtrls, 0x00, sizeof(extCtrls));

    m_getFrameHandleComponentsWrapper(frame, &components);
    factory = components.previewFactory;

    ret = frame->getNFDInfo(&fdInfo);

    CLOGV("[F%d(%d)]setExtControl for NFD. FaceNum %d",
            frame->getFrameCount(), fdInfo.frame_count, fdInfo.face_num);

    extCtrl.id = V4L2_CID_IS_S_NFD_DATA;
    extCtrl.ptr = &fdInfo;

    extCtrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    extCtrls.count = 1;
    extCtrls.controls = &extCtrl;

#ifdef USE_VRA_FD
    pipeId = PIPE_VRA;
#else
    pipeId = PIPE_3AA;
#endif
    nodeType = factory->getNodeType(pipeId);
    ret = factory->setExtControl(&extCtrls, pipeId, nodeType);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to setExtControl(V4L2_CID_IS_S_NFD_DATA). ret %d",
                frame->getFrameCount(), ret);
    }

    return ret;
}

status_t ExynosCamera::m_updateFaceDetectMeta(ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    camera2_shot_ext frameShotExt;
    fd_info fdInfo;

    memset(&fdInfo, 0x00, sizeof(fdInfo));
    memset(&frameShotExt, 0x00, sizeof(frameShotExt));

#if defined(USE_VRA_FD)
    return ret;
#endif

    ret = frame->getNFDInfo(&fdInfo);

    ret = frame->getMetaData(&frameShotExt);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getMetaData. ret %d",
                frame->getFrameCount(), ret);
        return ret;
    }

    /* TODO: Set the dm.stats.faceDetectMode as proper value */
    frameShotExt.shot.dm.stats.faceDetectMode = frameShotExt.shot.ctl.stats.faceDetectMode;

    for (int i = 0; i < CAMERA2_MAX_FACES; i++) {
        if (i >= fdInfo.face_num && frameShotExt.shot.dm.stats.faceIds[i] <= 0) {
            continue;
        }

        frameShotExt.shot.dm.stats.faceIds[i]               = fdInfo.id[i];
        frameShotExt.shot.dm.stats.faceScores[i]            = (uint8_t)fdInfo.score[i];
        frameShotExt.shot.dm.stats.faceRectangles[i][X1]    = (uint32_t)fdInfo.rect[i].offset_x;
        frameShotExt.shot.dm.stats.faceRectangles[i][Y1]    = (uint32_t)fdInfo.rect[i].offset_y;
        frameShotExt.shot.dm.stats.faceRectangles[i][X2]    = (uint32_t)(fdInfo.rect[i].offset_x + fdInfo.rect[i].width);
        frameShotExt.shot.dm.stats.faceRectangles[i][Y2]    = (uint32_t)(fdInfo.rect[i].offset_y + fdInfo.rect[i].height);
        for (int j = 0; j < 6; j++)
            frameShotExt.shot.dm.stats.faceLandmarks[i][j]  = -1;

        CLOGV("[F:%d]faceDetectMode(%d), face_num(%d), id(%d), score(%f), rotation(%f), faceRect(%f,%f,%fx%f), imgRect(%d,%d, %dx%d)(%dx%d)",
                fdInfo.frame_count, frameShotExt.shot.dm.stats.faceDetectMode,
                fdInfo.face_num, fdInfo.id[i], fdInfo.score[i], fdInfo.rotation[i],
                fdInfo.rect[i].offset_x, fdInfo.rect[i].offset_y, fdInfo.rect[i].width, fdInfo.rect[i].height,
                fdInfo.crop_x, fdInfo.crop_y, fdInfo.crop_w, fdInfo.crop_h, fdInfo.width, fdInfo.height);

        frameShotExt.hfd.faceIds[i]             = fdInfo.id[i];
        frameShotExt.hfd.score[i]               = (uint32_t)fdInfo.score[i];
        frameShotExt.hfd.faceRectangles[i][0]   = (uint32_t)fdInfo.rect[i].offset_x;
        frameShotExt.hfd.faceRectangles[i][1]   = (uint32_t)fdInfo.rect[i].offset_y;
        frameShotExt.hfd.faceRectangles[i][2]   = (uint32_t)(fdInfo.rect[i].offset_x + fdInfo.rect[i].width);
        frameShotExt.hfd.faceRectangles[i][3]   = (uint32_t)(fdInfo.rect[i].offset_y + fdInfo.rect[i].height);
        frameShotExt.hfd.is_rot[i]              = 0;
        frameShotExt.hfd.is_yaw[i]              = (uint32_t)fdInfo.yaw[i];
        frameShotExt.hfd.rot[i]                 = (uint32_t)fdInfo.rotation[i];
        frameShotExt.hfd.mirror_x[i]            = 0;
        frameShotExt.hfd.hw_rot_mirror[i]       = 0;
    }

    frame_handle_components_t components;
    m_getFrameHandleComponentsWrapper(frame, &components);
    if (components.parameters->checkFaceDetectMeta(&frameShotExt) == true) {
        frame->setMetaData(&frameShotExt);
    }

    return ret;
}
#endif //SUPPORT_NFD

void ExynosCamera::m_adjustSlave3AARegion(frame_handle_components_t *components, ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    camera2_shot_ext frameShotExt;
    ExynosRect masterHwSensorSize;
    ExynosRect masterHwBcropSize;
    ExynosRect slaveHwSensorSize;
    ExynosRect slaveHwBcropSize;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;
    uint32_t *region = NULL;
    int cameraId = frame->getCameraId();
    int sensorInfoCameraId = m_cameraId;
    if (m_camIdInfo.sensorInfoCamIdx >= 0)
        sensorInfoCameraId = m_camIdInfo.cameraId[m_camIdInfo.sensorInfoCamIdx];

    if (cameraId == sensorInfoCameraId) return;

    ret = frame->getMetaData(&frameShotExt);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getMetaData. ret %d",
                frame->getFrameCount(), ret);
        return;
    }

    m_parameters[sensorInfoCameraId]->getPreviewBayerCropSize(&masterHwSensorSize, &masterHwBcropSize);
    components->parameters->getPreviewBayerCropSize(&slaveHwSensorSize, &slaveHwBcropSize);
    float sensorRatio = getSensorRatio(&m_camIdInfo, cameraId);

    /* Scale up the 3AA region
       to adjust into the 3AA input size of slave sensor stream
     */
    scaleRatioW = (float) slaveHwBcropSize.w / (float) masterHwBcropSize.w;
    scaleRatioH = (float) slaveHwBcropSize.h / (float) masterHwBcropSize.h;

    int centerX = masterHwBcropSize.w / 2;
    int centerY = masterHwBcropSize.h / 2;

    for (int i = 0; i < 3; i++)  {
        switch (i) {
        case 0:
            region = frameShotExt.shot.ctl.aa.afRegions;
            break;
        case 1:
            region = frameShotExt.shot.ctl.aa.aeRegions;
            break;
        case 2:
            region = frameShotExt.shot.ctl.aa.awbRegions;
            break;
        default:
            continue;
        }

        if (!(region[0] | region[1] | region[2] | region[3])) continue;

        int x1 = region[0];
        int y1 = region[1];
        int x2 = region[2];
        int y2 = region[3];

        x1 -= centerX;
        y1 -= centerY;
        x2 -= centerX;
        y2 -= centerY;

        x1 *= sensorRatio;
        y1 *= sensorRatio;
        x2 *= sensorRatio;
        y2 *= sensorRatio;

        x1 += centerX;
        y1 += centerY;
        x2 += centerX;
        y2 += centerY;

        x1 *= scaleRatioW;
        y1 *= scaleRatioH;
        x2 *= scaleRatioW;
        y2 *= scaleRatioH;

        CFLOGV(frame, "[[%d]SensorInfoCam:%d,Cam%d] MasterBcrop %d,%d %dx%d(%d) SlaveBcrop %d,%d %dx%d(%d) " \
                      "[Region %d, %d, %d, %d] to [%d,%d,%d,%d] sensorRatio:%f, scalerRatio(%f,%f), Center(%d,%d)",
                i, sensorInfoCameraId, cameraId,
                masterHwBcropSize.x, masterHwBcropSize.y, masterHwBcropSize.w, masterHwBcropSize.h,
                SIZE_RATIO(masterHwBcropSize.w, masterHwBcropSize.h),
                slaveHwBcropSize.x, slaveHwBcropSize.y, slaveHwBcropSize.w, slaveHwBcropSize.h,
                SIZE_RATIO(slaveHwBcropSize.w, slaveHwBcropSize.h),
                region[0],  region[1],  region[2],  region[3],
                x1, y1, x2, y2,
                sensorRatio, scaleRatioW, scaleRatioH,
                centerX, centerY);

        /* Top-left */
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 < 0) x2 = 0;
        if (y2 < 0) y2 = 0;

        /* Bottom-right */
        if (x1 > slaveHwBcropSize.w) x1 = slaveHwBcropSize.w;
        if (y1 > slaveHwBcropSize.h) y1 = slaveHwBcropSize.h;
        if (x2 > slaveHwBcropSize.w) x2 = slaveHwBcropSize.w;
        if (y2 > slaveHwBcropSize.h) y2 = slaveHwBcropSize.h;

        region[0] = x1;
        region[1] = y1;
        region[2] = x2;
        region[3] = y2;
    }

    ret = frame->setMetaData(&frameShotExt);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to setMetaData. ret %d", frame->getFrameCount(), ret);
        return;
    }
}

#ifdef USE_DUAL_CAMERA
status_t ExynosCamera::m_handleCaptureFusionPlugin(ExynosCameraFrameSP_sptr_t frame, int leaderPipeId, int nextPipeId)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer buffer;
    frame_handle_components_t components;
    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

#if !defined(USES_COMBINE_PLUGIN)
    /*
     * All src buffers will be released in converter.
     * COMBINE reprocessing plugIn need to keep all input buffer till finishing capture.
     */
    ret = frame->getSrcBuffer(leaderPipeId, &buffer, OUTPUT_NODE_2);
    if (ret != NO_ERROR) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", leaderPipeId, ret);
    } else {
        if (buffer.index >= 0) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d T%d B%d]Failed to putBuffer for PipeId(%d). ret %d",
                        frame->getFrameCount(), frame->getFrameType(), buffer.index, leaderPipeId, ret);
            }
        }
    }
#endif
    if (frame->getPPScenario(leaderPipeId) == PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
        uint32_t nodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION0_REPROCESSING);
        ret = frame->getDstBuffer(leaderPipeId, &buffer, nodeIndex);
        if (ret != NO_ERROR || buffer.index <= -1) {
            CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
                    frame->getFrameCount(), buffer.index, leaderPipeId, nodeIndex, ret);
            return ret;
        }

        if (checkLastFrameForMultiFrameCapture(frame) == false
            || frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR) == true) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer. ret %d",
                        frame->getFrameCount(), buffer.index, ret);
            }
            frame->setSrcBufferState(nextPipeId, ENTITY_BUFFER_STATE_ERROR);
        } else {
            nodeIndex = OUTPUT_NODE_1;
            ret = frame->setSrcBufferState(nextPipeId, ENTITY_BUFFER_STATE_REQUESTED, nodeIndex);
            if (ret != NO_ERROR) {
                CLOGE("Failed to setSrcBufferState. pipeId %d", nextPipeId);
            } else {
                ret = frame->setSrcBuffer(nextPipeId, buffer, nodeIndex);
                if (ret != NO_ERROR || buffer.index <= -1) {
                    CLOGE("[F%d B%d]Failed to setSrcBuffer. pipeId %d, nodeId %d ret %d",
                            frame->getFrameCount(), buffer.index, nextPipeId, nodeIndex, ret);
                    return ret;
                }
            }

            if (components.configuration->getMode(CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE) == true) {
                ret = m_setBokehRefocusJpegSrcBuffer(frame, leaderPipeId, components);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d]setRefocusJpegBuffer failed, ret(%d)",
                            frame->getFrameCount(), ret);
                    return ret;
                }

                /* Second jpeg Dst buffer setting */
                buffer_manager_tag_t bufTag;
                bufTag.pipeId[0] = PIPE_JPEG1_REPROCESSING;
                bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
                switch (frame->getFrameType()) {
                case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
                case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
                    bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
                    break;
                default:
                    break;
                }
#endif
                ret = m_bufferSupplier->getBuffer(bufTag, &buffer);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                            frame->getFrameCount(), buffer.index, ret);
                    return ret;
                }

                ret = frame->setDstBuffer(nextPipeId, buffer,
                                            components.reprocessingFactory->getNodeType(PIPE_JPEG1_REPROCESSING));
                if (ret != NO_ERROR || buffer.index <= -1) {
                    CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
                        frame->getFrameCount(), buffer.index, nextPipeId,
                        components.reprocessingFactory->getNodeType(PIPE_JPEG1_REPROCESSING), ret);
                    return ret;
                }

                ExynosRect jpegRect;
                components.configuration->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&jpegRect.w, (uint32_t *)&jpegRect.h);
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
                if (m_scenario == SCENARIO_DUAL_REAR_ZOOM) {
                    ExynosCameraRequestSP_sprt_t request = m_requestMgr->getRunningRequest(frame->getFrameCount());
                    const camera3_stream_buffer_t *streamBuffer = request->getOutputBuffer(HAL_STREAM_ID_JPEG, false);
                    if (streamBuffer != nullptr) {
                        jpegRect.w = streamBuffer->stream->width;
                        jpegRect.h = streamBuffer->stream->height;
                        CFLOGD(frame, "jpeg streamBufferSize(%dx%d)", jpegRect.w, jpegRect.h);
                    }
                }
#endif
                camera2_shot_ext shotExt;
                frame->getMetaData(&shotExt);

                if (components.parameters->supportJpegYuvRotation() == true) {
                    ExynosCameraRequestSP_sprt_t request = m_requestMgr->getRunningRequest(frame->getFrameCount());
                    frame->setRotation(bufTag.pipeId[0], request->getRotation(HAL_STREAM_ID_JPEG));
                }

                frame->setJpegPipeInfo(bufTag.pipeId[0], jpegRect, &shotExt.shot);

                int depthNodeIndex = 0;
                if (components.configuration->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == true) {
                    depthNodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION0_REPROCESSING);
                } else {
                    depthNodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION2_REPROCESSING);
                }

                ret = frame->getDstBuffer(leaderPipeId, &buffer, depthNodeIndex);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CLOGE("[F%d B%d]Failed to getDstBuffer. pipeId %d, nodeId %d ret %d",
                            frame->getFrameCount(), buffer.index, leaderPipeId, depthNodeIndex, ret);
                } else {
                    ret = m_bufferSupplier->putBuffer(buffer);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                                frame->getFrameCount(), buffer.index, ret);
                        return ret;
                    }
                }
            }
        }
    } else {
        ret = frame->getDstBuffer(leaderPipeId, &buffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d P%d]Failed to getDstBuffer. ret %d",
                    frame->getFrameCount(), leaderPipeId, ret);
            return ret;
        }

        ret = frame->setSrcBufferState(nextPipeId, ENTITY_BUFFER_STATE_REQUESTED);
        if (ret != NO_ERROR) {
            CLOGE("Failed to setSrcBufferState. pipeId %d", nextPipeId);
        } else {
            ret = frame->setSrcBuffer(nextPipeId, buffer);
            if (ret != NO_ERROR) {
                CLOGE("setSrcBuffer() fail, pipeId(%d), ret(%d)", nextPipeId, ret);
                return ret;
            }
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_updateSizeBeforeDualFusion(ExynosCameraFrameSP_sptr_t frame, int pipeId)
{
    status_t ret = NO_ERROR;

    frame_handle_components_t components;
    m_getFrameHandleComponentsWrapper(frame, &components);
    ExynosCameraRequestSP_sprt_t request = m_requestMgr->getRunningRequest(frame->getFrameCount());
    ExynosCameraBuffer buffer;
    int pipeId_next = PIPE_FUSION;
    int outputPortId = frame->getOnePortId();
    int format = m_configurations->getYuvFormat(outputPortId);

    if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
        ///////////////////////
        // set Src(WIDE) Rect
        /*
        * use pipeId_next, not pipeId.
        * because, pipeId is changed in somewhere.
        */
        buffer.index = -2;
        ret = frame->getSrcBuffer(pipeId_next, &buffer);
        if(buffer.index < 0) {
            CLOGE("Invalid buffer index(%d), framecount(%d), pipeId(%d)",
                buffer.index, frame->getFrameCount(), pipeId_next);
        } else {
            ExynosRect rect = convertingBufferDst2Rect(&buffer, format);

            CLOGV("wide src.x(%d), src.y(%d), src.w(%d), src.h(%d), src.fullW(%d), src.fullH(%d), src.colorFormat(%d)",
                        rect.x, rect.y, rect.w, rect.h, rect.fullW, rect.fullH, rect.colorFormat);

            ret = frame->setSrcRect(pipeId_next, rect, OUTPUT_NODE_1);
            if(ret != NO_ERROR) {
                CLOGE("setSrcRect fail, pipeId(%d), ret(%d)", pipeId_next, ret);
            }
        }

        ///////////////////////
        // set Src(TELE) Rect
        CLOGV("frame->getFrameType() : %d", frame->getFrameType());

        switch (frame->getFrameType()) {
        case FRAME_TYPE_PREVIEW_DUAL_MASTER:
        case FRAME_TYPE_PREVIEW_DUAL_SLAVE:
        case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
            buffer.index = -2;
            ret = frame->getDstBuffer(pipeId, &buffer);
            if (buffer.index < 0) {
                CLOGE("Invalid buffer index(%d), framecount(%d), pipeId(%d)",
                    buffer.index, frame->getFrameCount(), pipeId);
            } else {
                ExynosRect rect = convertingBufferDst2Rect(&buffer, format);

                CLOGV("tele src.x(%d), src.y(%d), src.w(%d), src.h(%d), src.fullW(%d), src.fullH(%d), src.colorFormat(%d)",
                            rect.x, rect.y, rect.w, rect.h, rect.fullW, rect.fullH, rect.colorFormat);

                ret = frame->setSrcRect(pipeId_next, rect, OUTPUT_NODE_2);
                if(ret != NO_ERROR) {
                    CLOGE("setSrcRect fail, pipeId(%d), ret(%d)", pipeId_next, ret);
                }
            }
            break;
        default:
            break;
        }
    }

    ///////////////////////
    // set Dst Rect
    ExynosRect dstRect;
    int width, height;

    components.parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&width, (uint32_t *)&height, outputPortId);

    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.w = width;
    dstRect.h = height;
    dstRect.fullW = width;
    dstRect.fullH = height;
    if (request->hasStream(HAL_STREAM_ID_PREVIEW)) {
        dstRect.colorFormat = format;
    } else if (request->hasStream(HAL_STREAM_ID_CALLBACK)) {
        dstRect.colorFormat = V4L2_PIX_FMT_NV21;
    } else {
        dstRect.colorFormat = format;
    }

    CLOGV("dst.x(%d), dst.y(%d), dst.w(%d), dst.h(%d), dst.fullW(%d), dst.fullH(%d), dst.colorFormat(%d)",
            dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH, dstRect.colorFormat);

    ret = frame->setDstRect(pipeId_next, dstRect);
    if (ret != NO_ERROR) {
        CLOGE("setDstRect(Pipe:%d) failed, Fcount(%d), ret(%d)",
                pipeId_next, frame->getFrameCount(), ret);
    }

    return ret;
}
#endif

#ifdef SUPPORT_ME
status_t ExynosCamera::m_handleMeBuffer(ExynosCameraFrameSP_sptr_t frame, int mePos, const int meLeaderPipeId)
{
    int leaderPipeId = -1;
    ExynosCameraFrameEntity* entity = NULL;
    bool bError = false;
    ExynosCameraBuffer buffer;
    entity_buffer_state_t bufferState = ENTITY_BUFFER_STATE_INVALID;
    int dstPos = mePos;
    status_t ret = NO_ERROR;

    if (frame->getRequest(PIPE_ME) == false) {
        frame->setRequest(PIPE_GMV, false);
        return NO_ERROR;
    }

    /* 1. verify pipeId including ME */
    if (meLeaderPipeId < 0) {
        entity = frame->getFrameDoneFirstEntity();
        if (entity == NULL) {
            CLOGE("current entity is NULL, HAL-frameCount(%d)",
                    frame->getFrameCount());
            /* TODO: doing exception handling */
            return INVALID_OPERATION;
        }
        leaderPipeId = entity->getPipeId();
    } else {
        leaderPipeId = meLeaderPipeId;
    }

    if (m_parameters[m_cameraId]->getLeaderPipeOfMe() != leaderPipeId) {
        return NO_ERROR;
    }

    /* 2. handling ME buffer*/
    ret = frame->getDstBuffer(leaderPipeId, &buffer, dstPos);
    if (ret != NO_ERROR || buffer.index < 0) {
        CLOGE("[F%d B%d]Failed to getDstBuffer for PIPE_ME. ret %d",
                frame->getFrameCount(), buffer.index, ret);
        bError = true;
    }

    ret = frame->getDstBufferState(leaderPipeId, &bufferState, dstPos);
    if (ret != NO_ERROR) {
        CLOGE("[F%d P%d]Failed to getDstBufferState for PIPE_ME",
                frame->getFrameCount(), leaderPipeId);
        bError = true;
    }

    if (buffer.index >= 0 && bufferState == ENTITY_BUFFER_STATE_COMPLETE) {
        /* In this case only, the me tag can be appendedd to the frame */
        if (frame->getRawStateInSelector() == FRAME_STATE_IN_SELECTOR_PUSHED) {
            ret = frame->addSelectorTag(m_captureSelector[m_cameraId]->getId(), leaderPipeId, dstPos, false);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to addSelectorTag. pipeId %d, dstPos %d, ret %d",
                        frame->getFrameCount(), buffer.index, leaderPipeId, dstPos, ret);
                bError = true;
            }
        } else if (frame->getRequest(PIPE_GMV) == true) {
            ret = m_setupEntity(PIPE_GMV, frame, &buffer, NULL);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to setSrcBuffer to PIPE_GMV. ret %d",
                        frame->getFrameCount(), buffer.index, ret);
                bError = true;
            }
        } else if (frame->getRequest(PIPE_VDIS) == true) {
            /* Don't release me buffer in order to support  use case for ME */
        } else {
            /* Release me buffer */
            bError = true;
        }

    } else {
        CLOGE("[F%d B%d]Invalid buffer. pipeId %d",
                frame->getFrameCount(), buffer.index, leaderPipeId);
        bError = true;
    }

    /* 3. error case */
    if (bError == true) {
        ret = frame->setDstBufferState(leaderPipeId, ENTITY_BUFFER_STATE_ERROR, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("[F%d P%d]Failed to setDstBufferState with ERROR for PIPE_ME",
                    frame->getFrameCount(), leaderPipeId);
        }

        if (buffer.index >= 0) {
            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer for ME. ret %d",
                        frame->getFrameCount(), buffer.index, ret);
            }
        }
    }

    return ret;
}
#endif

bool ExynosCamera::previewStreamFunc(ExynosCameraFrameSP_sptr_t newFrame,int pipeId)
{
    return m_previewStreamFunc(newFrame, pipeId);
}

ExynosCameraStreamThread::ExynosCameraStreamThread(ExynosCamera* cameraObj, const char* name, int pipeId)
    : ExynosCameraThread<ExynosCameraStreamThread>(this, &ExynosCameraStreamThread::m_streamThreadFunc, name)
{
    m_mainCamera = cameraObj;
    m_frameDoneQ = NULL;
    m_pipeId = pipeId;
}

ExynosCameraStreamThread::~ExynosCameraStreamThread()
{
}

frame_queue_t* ExynosCameraStreamThread::getFrameDoneQ()
{
    return m_frameDoneQ;
}

void ExynosCameraStreamThread::setFrameDoneQ(frame_queue_t* frameDoneQ)
{
    m_frameDoneQ = frameDoneQ;
}

bool ExynosCameraStreamThread::m_streamThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    if(m_frameDoneQ == NULL) {
        ALOGE("frameDoneQ is NULL");
        return false;
    }

    ret = m_frameDoneQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            /* CLOGW("wait timeout"); */
        } else {
            ALOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_mainCamera->previewStreamFunc(newFrame, m_pipeId);
}

bool ExynosCamera::m_checkPureBayerReprocessingFrom(ExynosCameraFrameSP_sptr_t frame) {
    ExynosCameraFrameEntity* entity = NULL;

    entity = frame->getFirstEntity();
    while(entity != NULL) {
        if (entity->getPipeId() == PIPE_3AA_REPROCESSING) {
            return true;
        }
        entity = frame->getNextEntity();
    }

    return false;
}

#ifdef USE_SLSI_PLUGIN
status_t ExynosCamera::m_handleCaptureFramePlugin(ExynosCameraFrameSP_sptr_t frame,
                                                            int leaderPipeId,
                                                            int &pipeId_next,
                                                            int &nodePipeId)
{
    status_t ret = 0;
    ExynosCameraBuffer srcBuffer, dstBuffer;
    frame_handle_components_t components;
    int32_t scenario = 0;

    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));

    switch(leaderPipeId) {
    case PIPE_PLUGIN_PRE1_REPROCESSING:
    case PIPE_PLUGIN_PRE2_REPROCESSING:
        break;
    case PIPE_3AA_REPROCESSING:
    case PIPE_ISP_REPROCESSING:
#ifdef USE_DUAL_CAMERA
    {
        int setupPluginId = -1;
#ifdef USES_COMBINE_PLUGIN
        setupPluginId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        setupPluginId = PIPE_FUSION_REPROCESSING;
#endif
        if (components.configuration->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW
            && frame->getRequest(setupPluginId) == true
            && frame->getRequest(PIPE_SYNC_REPROCESSING) == true) {
            nodePipeId = PIPE_MCSC0_REPROCESSING + components.configuration->getModeValue(CONFIGURATION_YUV_STALL_PORT);
            m_setupCapturePlugIn(frame, leaderPipeId, nodePipeId, setupPluginId);
            pipeId_next = PIPE_SYNC_REPROCESSING;
            break;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    case PIPE_CLAHE_REPROCESSING:
        if (frame->getRequest(PIPE_CLAHEC_REPROCESSING) == true) {
            if (frame->getRequest(PIPE_PLUGIN_POST1_REPROCESSING) == true) {
                pipeId_next = PIPE_PLUGIN_POST1_REPROCESSING;
                nodePipeId = PIPE_CLAHEC_REPROCESSING;
                m_setupCapturePlugIn(frame, leaderPipeId, nodePipeId, pipeId_next);
                break;
            }
        }
#endif

    case PIPE_VRA_REPROCESSING:
#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    case PIPE_SW_MCSC_REPEOCESSING:
#endif
        if (frame->getRequest(PIPE_PLUGIN_POST1_REPROCESSING) == true) {
            pipeId_next = PIPE_PLUGIN_POST1_REPROCESSING;
            nodePipeId = PIPE_MCSC0_REPROCESSING + components.configuration->getModeValue(CONFIGURATION_YUV_STALL_PORT);
            m_setupCapturePlugIn(frame, leaderPipeId, nodePipeId, pipeId_next);
            break;
        } else {
            nodePipeId = PIPE_MCSC0_REPROCESSING + components.configuration->getModeValue(CONFIGURATION_YUV_STALL_PORT);
        }
    case PIPE_PLUGIN_POST1_REPROCESSING:
        ////////////////////////////////////////////////
        // release src buffer
        if (leaderPipeId == PIPE_PLUGIN_POST1_REPROCESSING) {
            scenario = frame->getPPScenario(leaderPipeId);

            switch (scenario) {
#ifdef USES_HIFI_LLS
            case PLUGIN_SCENARIO_HIFILLS_REPROCESSING:
                // no free.
                break;
#endif
#ifdef USES_COMBINE_PLUGIN
            case PLUGIN_SCENARIO_COMBINE_REPROCESSING:
            case PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING:
#if 0
                /*
                 * All src buffers will be released in converter.
                 * COMBINE reprocessing plugIn need to keep all input buffer till finishing capture.
                 */
                ret = m_putSrcBuffer(frame, leaderPipeId);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d]m_putSrcBuffer(leaderPipeId(%d)) fail", frame->getFrameCount(), leaderPipeId);
                }
#endif
                break;
#endif
            default:
                CLOGW("[F%d]Invalid PP scenario(%d)",
                    frame->getFrameCount(), scenario);
                break;
            }
        }

        ////////////////////////////////////////////////
        // about dst buffer
        if (frame->getRequest(PIPE_PLUGIN_POST2_REPROCESSING) == true) {
            pipeId_next = PIPE_PLUGIN_POST2_REPROCESSING;
            m_setupCapturePlugIn(frame, leaderPipeId, nodePipeId, pipeId_next);
        } else {
            pipeId_next = PIPE_JPEG_REPROCESSING;
#ifdef USE_DUAL_CAMERA
            if (frame->getPPScenario(leaderPipeId) == components.configuration->getFusionCapturePluginScenario()) {
                ret = m_handleCaptureFusionPlugin(frame, leaderPipeId, pipeId_next);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d P%d]Failed to m_handleCaptureFusionPlugin. pipeId %d ret %d",
                            frame->getFrameCount(), leaderPipeId, ret);
                    return ret;
                }
            } else
#endif
            {
                ////////////////////////////////////////////////
                // release dst buffer
                int nodeIndex = 0;
#ifdef USE_SLSI_PLUGIN
                if (leaderPipeId == PIPE_PLUGIN_POST1_REPROCESSING) {
                    nodeIndex = 0;
                } else
#endif
                {
                    nodeIndex = components.reprocessingFactory->getNodeType(nodePipeId);
                }

                ret = frame->getDstBuffer(leaderPipeId, &dstBuffer, nodeIndex);

                if (ret != NO_ERROR) {
                    CLOGE("[F%d]Failed to getDstBuffer. pipeId %d ret %d",
                            frame->getFrameCount(), leaderPipeId, ret);
                    return ret;
                }

                if (checkLastFrameForMultiFrameCapture(frame) == false) {
                    ret = frame->setDstBufferState(leaderPipeId, ENTITY_BUFFER_STATE_REQUESTED, nodeIndex);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d]setDstBufferState(leaderPipeId : %d, ENTITY_BUFFER_STATE_REQUESTED, nodeIndex : %d) fail",
                            frame->getFrameCount(), leaderPipeId, nodeIndex);
                        return ret;
                    }

                    ExynosCameraBuffer nullBuf;
                    frame->setDstBuffer(leaderPipeId, nullBuf, nodeIndex);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d]setDstBuffer(leaderPipeId : %d, nullBuf, nodeIndex : %d) fail",
                            frame->getFrameCount(), leaderPipeId, nodeIndex);
                        return ret;
                    }

                    frame->setSrcBufferState(pipeId_next, ENTITY_BUFFER_STATE_ERROR);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d]setDstBufferState(pipeId_next : %d, ENTITY_BUFFER_STATE_ERROR, nodeIndex : %d) fail",
                            frame->getFrameCount(), pipeId_next, ENTITY_BUFFER_STATE_ERROR, nodeIndex);
                        return ret;
                    }
                } else {
#ifdef USE_GSC_FOR_CAPTURE_YUV_RESCALE
                    m_resizeToScaledYuv(frame, leaderPipeId);
                    ret = frame->getDstBuffer(PIPE_GSC_REPROCESSING3, &dstBuffer);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d B%d]Failed to run GSC. pipeId %d ret %d",
                            frame->getFrameCount(), dstBuffer.index, PIPE_GSC_REPROCESSING3, ret);
                        return ret;
                    }
#endif

                    ////////////////////////////////////////////////
                    // forward dst buffer
                    ret = frame->setSrcBuffer(pipeId_next, dstBuffer);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d B%d]Failed to setSrcBuffer. pipeId %d ret %d",
                            frame->getFrameCount(), dstBuffer.index, leaderPipeId, ret);
                        return ret;
                    }
                }
            }
        }
        break;

    case PIPE_PLUGIN_POST2_REPROCESSING:
        ret = frame->getDstBuffer(leaderPipeId, &dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getDstBuffer. pipeId %d ret %d",
                    frame->getFrameCount(), nodePipeId, ret);
            return ret;
        }

        pipeId_next = PIPE_JPEG_REPROCESSING;

        ret = frame->setSrcBuffer(pipeId_next, dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("setSrcBuffer() fail, pipeId(%d), ret(%d)", pipeId_next, ret);
            return ret;
        }
        break;

#ifdef USE_DUAL_CAMERA
    case PIPE_SYNC_REPROCESSING:
#ifdef USES_COMBINE_PLUGIN
        pipeId_next = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        pipeId_next = PIPE_FUSION_REPROCESSING;
#endif
        ret = frame->getDstBuffer(leaderPipeId, &dstBuffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getDstBuffer. pipeId %d ret %d",
                frame->getFrameCount(), nodePipeId, ret);
            return ret;
        }

        if (dstBuffer.index < 0) {
            CLOGE("[F%d]Invalid buffer index(%d), pipeId(%d)",
                    dstBuffer.index, frame->getFrameCount(), leaderPipeId);
            return ret;
        }

        if (frame->getFrameState() == FRAME_STATE_SKIPPED ) {
            ret = m_bufferSupplier->putBuffer(dstBuffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d T%d B%d]Failed to putBuffer for PIPE_SYNC_REPROCESSING. ret %d",
                    frame->getFrameCount(), frame->getFrameType(), dstBuffer.index, ret);
            }
        } else {
            int pos = OUTPUT_NODE_2;
#ifdef USES_COMBINE_PLUGIN
            // single frame should go to combine plugIn
            if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SLAVE) {
                pos = OUTPUT_NODE_1;
            }
#endif
            ExynosRect rect = convertingBufferDst2Rect(&dstBuffer, V4L2_PIX_FMT_NV21);
            frame->setSrcRect(pipeId_next, rect, pos);
            ret = frame->setSrcBuffer(pipeId_next, dstBuffer, pos);
            if (ret != NO_ERROR) {
                CLOGE("setSrcBuffer fail, pipeId(%d), ret(%d)",
                            pipeId_next, ret);
            }

            if (m_scenario == SCENARIO_DUAL_REAR_ZOOM &&
                    frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
                int32_t displayCameraId = m_configurations->getDualDisplayCameraId();
                // update dst rect as actual master camera's rect
                CFLOGD(frame, "choose output2 node's size due to (displayCamera:%d)", displayCameraId);
                if (frame->getCameraId(OUTPUT_NODE_2) == displayCameraId) {
                    // set the new displayCameraId
                    frame->setDisplayCameraId(displayCameraId);
                    int pos = components.reprocessingFactory->getNodeType(PIPE_FUSION0_REPROCESSING);
                    ret = frame->setDstRect(pipeId_next, rect, pos);
                    if (ret != NO_ERROR) {
                        CFLOGE(frame, "can't get find dst rect");
                    }
                }
            }
        }
        break;

    case PIPE_FUSION_REPROCESSING:
        pipeId_next = PIPE_JPEG_REPROCESSING;
        ret = m_handleCaptureFusionPlugin(frame, leaderPipeId, pipeId_next);
        if (ret != NO_ERROR) {
            CLOGE("[F%d P%d]Failed to m_handleCaptureFusionPlugin. pipeId %d ret %d",
                    frame->getFrameCount(), leaderPipeId, ret);
            return ret;
        }
        break;
#endif
    }

    return ret;
}

status_t ExynosCamera::m_checkDynamicBayerMode(void)
{
    status_t ret = NO_ERROR;
    bool useDynamicBayer = m_configurations->getMode(CONFIGURATION_DYNAMIC_BAYER_MODE);

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true
        && m_streamManager->findStream(HAL_STREAM_ID_VIDEO) == false
        && (m_streamManager->findStream(HAL_STREAM_ID_JPEG) == true
            || m_streamManager->findStream(HAL_STREAM_ID_CALLBACK_STALL) == true)) {
        useDynamicBayer = false;
    }
#endif

    m_checkVendorDynamicBayerMode(useDynamicBayer);

#if defined(DEBUG_RAWDUMP) || defined(DEBUG_DUMP_IMAGE_NEED_ALWAYS_BAYER)
    if (m_configurations->checkBayerDumpEnable() == true) {
        useDynamicBayer = false;
    }
#endif

    m_configurations->setMode(CONFIGURATION_DYNAMIC_BAYER_MODE, useDynamicBayer);
    CLOGD("useDynamicBayer(%d)", useDynamicBayer);

    return ret;
}

status_t ExynosCamera::m_checkCaptureMode(__unused ExynosCameraRequestSP_sprt_t request, frame_handle_components_t& components)
{
    status_t ret = NO_ERROR;

    components.parameters->checkPostProcessingCapture(request);

    if (components.parameters->getBayerFrameLock() == true) {
        m_checkLockFrameHoldCount(components);
    }

    return ret;
}

status_t ExynosCamera::m_prepareCaptureMode(__unused ExynosCameraRequestSP_sprt_t request,
                                                    __unused frame_type_t frameType,
                                                    frame_handle_components_t& components)
{
    status_t ret = NO_ERROR;
    int prePluginCnt = 0, postPluginCnt = 0;
    bool requestFusionPlugin = false;
#ifdef USE_DUAL_CAMERA
    components.reprocessingFactory->setRequest(PIPE_SYNC_REPROCESSING, false);
    components.reprocessingFactory->setRequest(PIPE_FUSION_REPROCESSING, false);
#endif
    components.reprocessingFactory->setRequest(PIPE_PLUGIN_PRE1_REPROCESSING, false);
    components.reprocessingFactory->setRequest(PIPE_PLUGIN_PRE2_REPROCESSING, false);
    components.reprocessingFactory->setRequest(PIPE_PLUGIN_POST1_REPROCESSING, false);
    components.reprocessingFactory->setRequest(PIPE_PLUGIN_POST2_REPROCESSING, false);

    /*
       1. update factorySetting.
    */
    ////////////////////////////////////////////////
    // pre
    bool hifiMode = false, nightShotBayer = false, superNightShotBayer = false, hdrBayer = false;

#ifdef USES_HIFI
    hifiMode = m_configurations->getMode(CONFIGURATION_HIFI_MODE);
#endif
#ifdef USES_COMBINE_PLUGIN
    nightShotBayer = m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE);

    superNightShotBayer = m_configurations->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE);
#ifdef USE_DUAL_CAMERA
    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW
        && ExynosCameraFrame::convertDualModeByFrameType(frameType) == DUAL_OPERATION_MODE_SYNC
        && frameType == FRAME_TYPE_REPROCESSING_DUAL_SLAVE) {
        superNightShotBayer = false;
    }
#endif

    hdrBayer = m_configurations->getMode(CONFIGURATION_HDR_BAYER_MODE);
#endif

    if (hifiMode || nightShotBayer || superNightShotBayer || hdrBayer) {
        prePluginCnt++;
    }

    ////////////////////////////////////////////////
    // post
    bool hifiLLsMode = false, nightShotYuv = false, hdrYuv = false, flashMultiFrameDenoiseYuv = false, beautyFaceYuv = false, dualFusion = false,
        superResolution = false, oisDenoiseYuv = false, sportsCaptureYuv = false, singleCapture = false;

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW
        && ExynosCameraFrame::convertDualModeByFrameType(frameType) == DUAL_OPERATION_MODE_SYNC) {
        switch (frameType) {
        case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
#ifdef USES_COMBINE_PLUGIN
            dualFusion = true;
#else
            components.reprocessingFactory->setRequest(PIPE_FUSION_REPROCESSING, true);
#endif
            [[fallthrough]];
        case FRAME_TYPE_REPROCESSING_DUAL_SLAVE:
            components.reprocessingFactory->setRequest(PIPE_SYNC_REPROCESSING, true);
            break;
        default:
            break;
        }
    }
#endif

#ifdef USES_HIFI_LLS
    hifiLLsMode = m_configurations->getMode(CONFIGURATION_HIFI_LLS_MODE);
#endif

#ifdef USES_COMBINE_PLUGIN
    nightShotYuv = m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE);
    hdrYuv = m_configurations->getMode(CONFIGURATION_HDR_YUV_MODE);
    flashMultiFrameDenoiseYuv = m_configurations->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE);
    beautyFaceYuv = m_configurations->getMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE);
    superResolution = m_configurations->getMode(CONFIGURATION_SUPER_RESOLUTION_MODE);
    oisDenoiseYuv = m_configurations->getMode(CONFIGURATION_OIS_DENOISE_YUV_MODE);
    sportsCaptureYuv = m_configurations->getMode(CONFIGURATION_SPORTS_YUV_MODE);
    singleCapture = m_configurations->getMode(CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE);
#endif

    if (hifiLLsMode || nightShotYuv || hdrYuv || flashMultiFrameDenoiseYuv || beautyFaceYuv
        || dualFusion || superResolution || oisDenoiseYuv || sportsCaptureYuv || singleCapture) {
        postPluginCnt++;
    }

    if (prePluginCnt > 2 || postPluginCnt > 2 ) {
        CLOGD("invalid active plugin count pre(%d) post(%d)", prePluginCnt, postPluginCnt);
        return INVALID_OPERATION;
    }

    /*
        3. update active pipe plugin.
    */
    for(int i = 0 ; i < prePluginCnt; i++) {
        components.reprocessingFactory->setRequest(PIPE_PLUGIN_PRE1_REPROCESSING + i, true);
    }

    for(int i = 0 ; i < postPluginCnt; i++) {
        components.reprocessingFactory->setRequest(PIPE_PLUGIN_POST1_REPROCESSING + i, true);
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_prepareCapturePlugin(__unused ExynosCameraFrameFactory *targetfactory,
                                                        __unused frame_type_t frameType,
                                                        __unused list<int> &preSceanrio,
                                                        __unused map<int, int> *scenarioList)
{
    status_t ret = NO_ERROR;
    list<int>::iterator iter;
    map<int, int>::iterator mapIter;
    list<int> postSceanrio;
    list<int> prePipeId;
    list<int> postPipeId;

#ifdef USE_DUAL_CAMERA
    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW
        && ExynosCameraFrame::convertDualModeByFrameType(frameType) == DUAL_OPERATION_MODE_SYNC) {
        if (frameType == FRAME_TYPE_REPROCESSING_DUAL_MASTER) {
#ifndef USES_COMBINE_PLUGIN
            postPipeId.push_back(PIPE_FUSION_REPROCESSING);
#endif
            postSceanrio.push_back(m_configurations->getFusionCapturePluginScenario());
        }
    }
#endif

    for(int i = PIPE_PLUGIN_BASE_REPROCESSING ; i <= PIPE_PLUGIN_MAX_REPROCESSING; i++) {
        if (i < PIPE_PLUGIN_POST1_REPROCESSING) {
            prePipeId.push_back(i);
        } else {
            postPipeId.push_back(i);
        }
    }

    /* 1. get the scenario list */
    /* - pre sceanrio */
#ifdef USES_HIFI
    if (m_configurations->getMode(CONFIGURATION_HIFI_MODE)) {
        preSceanrio.push_back(PLUGIN_SCENARIO_HIFI_REPROCESSING);
    }
#endif
#ifdef USES_COMBINE_PLUGIN
    if (m_configurations->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == true) {
#ifdef USE_DUAL_CAMERA
        if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
            if (ExynosCameraFrame::convertDualModeByFrameType(frameType) == DUAL_OPERATION_MODE_SYNC
                && frameType == FRAME_TYPE_REPROCESSING_DUAL_SLAVE) {
                CLOGV("[T%d] skip to Pre plugin", frameType);
            } else {
                preSceanrio.push_back(PLUGIN_SCENARIO_COMBINE_REPROCESSING);
            }
        } else
#endif
        {
            preSceanrio.push_back(PLUGIN_SCENARIO_COMBINE_REPROCESSING);
        }
    } else if (m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_BAYER_MODE)
            || m_configurations->getMode(CONFIGURATION_HDR_BAYER_MODE)) {
        preSceanrio.push_back(PLUGIN_SCENARIO_COMBINE_REPROCESSING);
    } else
#endif
    {
        // nop;
    }

    /* - post sceanrio */
#ifdef USES_HIFI_LLS
    if (m_configurations->getMode(CONFIGURATION_HIFI_LLS_MODE)) {
        postSceanrio.push_back(PLUGIN_SCENARIO_HIFILLS_REPROCESSING);
    } else
#endif
#ifdef USES_COMBINE_PLUGIN
    if ((postSceanrio.empty() == true)
        && (m_configurations->getMode(CONFIGURATION_NIGHT_SHOT_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_HDR_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_FLASH_MULTI_FRAME_DENOISE_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_BEAUTY_FACE_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_SUPER_RESOLUTION_MODE)
            || m_configurations->getMode(CONFIGURATION_OIS_DENOISE_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_SPORTS_YUV_MODE)
            || m_configurations->getMode(CONFIGURATION_COMBINE_SINGLE_CAPTURE_MODE))) {
        postSceanrio.push_back(PLUGIN_SCENARIO_COMBINE_REPROCESSING);
    } else
#endif
    {
        // nop;
    }

    /* TODO : 2. reordering plugin list and pipe */
    for (iter = preSceanrio.begin() ; iter != preSceanrio.end() ; iter++) {
        int key = prePipeId.front();
        int value = *iter;
        prePipeId.pop_front();
        scenarioList->insert(pair<int,int>(key, value));
    }

    for (iter = postSceanrio.begin() ; iter != postSceanrio.end() ; iter++) {
        int key = postPipeId.front();
        int value = *iter;
        postPipeId.pop_front();
        scenarioList->insert(pair<int, int>(key, value));
    }

    /* 3. prepare plugin */
    for (mapIter = scenarioList->begin() ; mapIter != scenarioList->end() ; mapIter++) {
        targetfactory->connectPPScenario(mapIter->first, mapIter->second);
        targetfactory->startPPScenario(mapIter->first, mapIter->second);
    }

    return ret;
}

status_t ExynosCamera::m_setupCapturePlugIn(ExynosCameraFrameSP_sptr_t frame,
                                           int pipeId, int subPipeId, int pipeId_next,
                                           ExynosCameraBuffer *srcBuffer,
                                           ExynosCameraBuffer *dstBuffer)
{
    status_t ret = NO_ERROR;
    int scenario = 0;
    struct camera2_stream *shot_stream = NULL;
    struct camera2_shot_ext *shot_ext = NULL;
    ExynosCameraBuffer newSrcBuffer;
    ExynosCameraBuffer newDstBuffer;
    ExynosRect srcRect;
    ExynosRect dstRect;
    bool getFaceDetectMeta = false;
    frame_handle_components_t components;
    int sizeW = 0, sizeH = 0;
    int dstNodeIndex = 0;

    m_getFrameHandleComponentsWrapper(frame, &components, m_getCameraSessionId(frame));
    scenario = frame->getPPScenario(pipeId_next);

    CLOGD("[F%d T%d P%d] subPipeId %d pipeId_next %d scenario(%d)",
            frame->getFrameCount(), frame->getFrameType(), pipeId,
            subPipeId, pipeId_next, scenario);
    CLOGV("STREAM_TYPE_YUVCB_STALL(%d)", frame->getStreamRequested(STREAM_TYPE_YUVCB_STALL));

    ////////////////////////////////////////////////
    // about src buffer
    if (srcBuffer) {
        ////////////////////////////////////////////////
        // if src buffer valid, just use it.
        newSrcBuffer = *srcBuffer;
    } else {
        ret = frame->getDstBuffer(pipeId, &newSrcBuffer, components.reprocessingFactory->getNodeType(subPipeId));
        if (ret != NO_ERROR || newSrcBuffer.index < 0) {
            CLOGE("Failed to get DST newSrcBuffer. pipeId %d subPipeId %d bufferIndex %d frameCount %d ret %d",
                pipeId, subPipeId, newSrcBuffer.index, frame->getFrameCount(), ret);
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // about dst buffer
    if (dstBuffer) {
        ////////////////////////////////////////////////
        // if dst buffer valid, just use it.
        newDstBuffer = *dstBuffer;
    } else {
        switch (scenario) {
#ifdef USES_HIFI_LLS
        case PLUGIN_SCENARIO_HIFILLS_REPROCESSING:
        {
            ////////////////////////////////////////////////
            //just set src & dst samely..
            newDstBuffer = newSrcBuffer;
            break;
        }
#endif
#ifdef USES_COMBINE_PLUGIN
#ifdef USE_DUAL_CAMERA
        case PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING:
            break;
#endif
        case PLUGIN_SCENARIO_COMBINE_REPROCESSING:
#endif
        default:
        {
            /*
             * ToDo: Buffer handling of Combine plugin should consider situations
             * in which Pre plugin and Post plugin are used simultaneously.
             */

            ////////////////////////////////////////////////
            // buffer setting for dst
            if (frame->getFrameIndex() == 0) {
                ////////////////////////////////////////////////
                // get new buffer for dst
                buffer_manager_tag_t bufTag;

                bufTag.pipeId[0] = pipeId_next;
                bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
                switch (frame->getFrameType()) {
                case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
                case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
                    bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
                    break;
                default:
                    break;
                }
#endif
                ret = m_bufferSupplier->getBuffer(bufTag, &newDstBuffer);
                if (ret != NO_ERROR || newDstBuffer.index < 0) {
                    CLOGE("[F%d B%d] getBuffer(bufTag.pipeId[0] : %d) fail",
                        frame->getFrameCount(), newDstBuffer.index, bufTag.pipeId[0]);

                    // hack
                    // just use src buffer, for running capture.
                    newDstBuffer = newSrcBuffer;
                }

                if (pipeId_next == PIPE_PLUGIN_PRE1_REPROCESSING) {
                    m_prePlugInDstBuffer = newDstBuffer;
                } else if (pipeId_next == PIPE_PLUGIN_POST1_REPROCESSING) {
                    m_postPlugInDstBuffer = newDstBuffer;
                } else {
                    CLOGW("[F%d B%d] invalid pipeId_next : %d", frame->getFrameCount(), newDstBuffer.index, pipeId_next);
                }
            } else {
                ////////////////////////////////////////////////
                // reuse the buffer
                newDstBuffer = m_postPlugInDstBuffer;

                if (pipeId_next == PIPE_PLUGIN_PRE1_REPROCESSING) {
                    newDstBuffer = m_prePlugInDstBuffer;
                } else if (pipeId_next == PIPE_PLUGIN_POST1_REPROCESSING) {
                    newDstBuffer = m_postPlugInDstBuffer;
                } else {
                    CLOGW("[F%d B%d] invalid pipeId_next : %d", frame->getFrameCount(), newDstBuffer.index, pipeId_next);
                }
            }

            ////////////////////////////////////////////////
            break;
        }
        }
    }

    shot_stream = (struct camera2_stream *)(newSrcBuffer.addr[newSrcBuffer.getMetaPlaneIndex()]);
    if (shot_stream != NULL) {
        CLOGV("(%d %d %d %d)",
                shot_stream->fcount,
                shot_stream->rcount,
                shot_stream->findex,
                shot_stream->fvalid);
        CLOGV("(%d %d %d %d)(%d %d %d %d)",
                shot_stream->input_crop_region[0],
                shot_stream->input_crop_region[1],
                shot_stream->input_crop_region[2],
                shot_stream->input_crop_region[3],
                shot_stream->output_crop_region[0],
                shot_stream->output_crop_region[1],
                shot_stream->output_crop_region[2],
                shot_stream->output_crop_region[3]);
    } else {
        CLOGE("shot_stream is NULL");
        return INVALID_OPERATION;
    }

    switch (scenario) {
#ifdef USES_HIFI_LLS
        case PLUGIN_SCENARIO_HIFILLS_REPROCESSING:
        {
            int pictureW = 0, pictureH = 0;
            srcRect = convertingBufferDst2Rect(&newSrcBuffer, V4L2_PIX_FMT_NV21);

            if (frame->getMode(FRAME_MODE_VENDOR_YUV_STALL)) {
                int vendorStallPipeId = (frame->getModeValue(FRAME_MODE_VENDOR_YUV_STALL_PORT) % ExynosCameraParameters::YUV_MAX) + PIPE_MCSC0_REPROCESSING;
                int dstPos = components.reprocessingFactory->getNodeType(vendorStallPipeId);

                components.configuration->getSize(CONFIGURATION_MAX_YUV_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
                ret = frame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED, dstPos);
                if (ret != NO_ERROR) {
                    CLOGE("Failed to setDstBufferState. pipeId %d pos %d", pipeId, dstPos);
                } else {
                    ret = frame->setDstBuffer(pipeId, newSrcBuffer, dstPos);
                    if (ret != NO_ERROR || newSrcBuffer.index < 0) {
                        CLOGE("Failed to get DST newSrcBuffer. pipeId %d subPipeId %d bufferIndex %d frameCount %d ret %d dstPipeId(%d)",
                                pipeId, subPipeId, newSrcBuffer.index, frame->getFrameCount(), ret, vendorStallPipeId);
                        return ret;
                    }
                    frame->setRequest(vendorStallPipeId, true);
                }
            } else {
                components.configuration->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureW, (uint32_t *)&pictureH);
            }

            dstRect.x       = 0;
            dstRect.y       = 0;
            dstRect.w       = pictureW;
            dstRect.h       = pictureH;
            dstRect.fullW   = pictureW;
            dstRect.fullH   = pictureH;
            dstRect.colorFormat = V4L2_PIX_FMT_NV21;
            CLOGD("Src (%d %d %d %d full %d %d) Dst(%d %d %d %d full %d %d)",
                    srcRect.x,
                    srcRect.y,
                    srcRect.w,
                    srcRect.h,
                    srcRect.fullW,
                    srcRect.fullH,
                    dstRect.x,
                    dstRect.y,
                    dstRect.w,
                    dstRect.h,
                    dstRect.fullW,
                    dstRect.fullH
                    );
        }
        break;
#endif
#ifdef USE_SLSI_PLUGIN
        case PLUGIN_SCENARIO_COMBINE_REPROCESSING:
        {
            getFaceDetectMeta = true;
            if (pipeId_next == PIPE_PLUGIN_PRE1_REPROCESSING ||
                pipeId_next == PIPE_PLUGIN_PRE2_REPROCESSING) {
                ////////////////////////////////////////////////
                // Bayer
#ifdef USE_3AA_CROP_AFTER_BDS
                if (components.parameters->getUsePureBayerReprocessing() == true) {
                    components.parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&srcRect.w, (uint32_t *)&srcRect.h);
                    dstRect = srcRect;
                }
                else
#endif
                {
                    components.parameters->getPictureBayerCropSize(&srcRect, &dstRect);
                }

                srcRect.fullW = srcRect.w;
                srcRect.fullH = srcRect.h;

                dstRect.fullW = dstRect.w;
                dstRect.fullH = dstRect.h;

                srcRect.colorFormat = components.parameters->getBayerFormat(m_getBayerPipeId());
                dstRect.colorFormat = srcRect.colorFormat;
            } else {
                ////////////////////////////////////////////////
                // YUV
                int colorFormat = (pipeId == PIPE_PLUGIN_PRE1_REPROCESSING || pipeId == PIPE_PLUGIN_PRE2_REPROCESSING) ?
                                    components.parameters->getBayerFormat(m_getBayerPipeId()) : V4L2_PIX_FMT_NV21;

                srcRect = convertingBufferDst2Rect(&newSrcBuffer, colorFormat);
                dstRect = srcRect;

                srcRect.fullW = ALIGN_UP(srcRect.fullW, CAMERA_16PX_ALIGN);
            }

            ////////////////////////////////////////////////
            break;
        }
#endif
#ifdef USE_DUAL_CAMERA
        case PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING:
        {
            getFaceDetectMeta = true;
            srcRect = convertingBufferDst2Rect(&newSrcBuffer, V4L2_PIX_FMT_NV21);
            dstRect = srcRect;
            dstNodeIndex = components.reprocessingFactory->getNodeType(PIPE_FUSION0_REPROCESSING);

            if (components.configuration->getMode(CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE) == true
                && frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR) == true) {
                ret = m_setBokehRefocusFusionDepthBuffer(frame, pipeId_next, dstNodeIndex, &newSrcBuffer);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d]setBokehRefocusFusionDepthBuffer failed, ret(%d)",
                            frame->getFrameCount(), ret);
                }
            } else {
                buffer_manager_tag_t bufTag;
                bufTag.pipeId[0] = subPipeId;
                bufTag.managerType = BUFFER_MANAGER_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
                switch (frame->getFrameType()) {
                case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
                case FRAME_TYPE_REPROCESSING_INTERNAL_SENSOR_TRANSITION:
                    bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
                    break;
                default:
                    break;
                }
#endif
                ret = m_bufferSupplier->getBuffer(bufTag, &newDstBuffer);
                if (ret != NO_ERROR || newDstBuffer.index < 0) {
                    CLOGE("[F%d B%d]Failed to getBuffer. ret %d",
                            frame->getFrameCount(), newDstBuffer.index, ret);
                    return ret;
                } else {
                    // copy dst crop information from src to this target
                    copyRectSrc2DstBuffer(&newSrcBuffer, &newDstBuffer);
                }

                if ((m_scenario == SCENARIO_DUAL_REAR_PORTRAIT) &&
                        checkLastFrameForMultiFrameCapture(frame) == true) {
                    ret = m_setBokehRefocusFusionYuvBuffer(frame, pipeId_next, subPipeId, newSrcBuffer);
                    if (ret != NO_ERROR) {
                        CLOGE("[F%d]setRefocusFusionBuffer failed, ret(%d)",
                                frame->getFrameCount(), ret);
                    }

                    if (components.configuration->getMode(CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE) == true
                        && components.configuration->getMode(CONFIGURATION_SUPER_NIGHT_SHOT_BAYER_MODE) == false) {
                        int depthNodType = components.reprocessingFactory->getNodeType(PIPE_FUSION2_REPROCESSING);
                        ret = m_setBokehRefocusFusionDepthBuffer(frame, pipeId_next, depthNodType, &newSrcBuffer);
                        if (ret != NO_ERROR) {
                            CLOGE("[F%d]setBokehRefocusFusionDepthBuffer failed, ret(%d)",
                                    frame->getFrameCount(), ret);
                        }
                    }
                }
            }
            break;
        }
#endif
        default:
        {
            int colorFormat = (pipeId == PIPE_PLUGIN_PRE1_REPROCESSING || pipeId == PIPE_PLUGIN_PRE2_REPROCESSING) ?
                                components.parameters->getBayerFormat(m_getBayerPipeId()) : V4L2_PIX_FMT_NV21;

            srcRect = convertingBufferDst2Rect(&newSrcBuffer, colorFormat);
            dstRect = convertingBufferDst2Rect(&newDstBuffer, colorFormat);

            break;
        }
    }

    shot_ext = (struct camera2_shot_ext *)(newSrcBuffer.addr[newSrcBuffer.getMetaPlaneIndex()]);

    ret = frame->getMetaData(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to getMetaData. ret %d", frame->getFrameCount(), ret);
    }

#ifdef CAPTURE_FD_SYNC_WITH_PREVIEW
    if (shot_ext->shot.ctl.stats.faceDetectMode > FACEDETECT_MODE_OFF
            && components.parameters->getHwConnectionMode(PIPE_MCSC, PIPE_VRA) == HW_CONNECTION_MODE_M2M) {
        /* When VRA works as M2M mode, FD metadata will be updated with the latest one in Parameters */
        getFaceDetectMeta = true;
    }
#endif

    if (getFaceDetectMeta == true) {
        components.parameters->getFaceDetectMeta(shot_ext);

        ret = frame->storeDynamicMeta(shot_ext);
        if (ret != NO_ERROR) {
            CLOGE("[F%d(%d) B%d]Failed to storeUserDynamicMeta. ret %d",
                    frame->getFrameCount(),
                    shot_ext->shot.dm.request.frameCount,
                    newSrcBuffer.index, ret);
            return ret;
        }
    }

    CLOGD("src size[%d, %d, %d, %d, %d, %d], dst size[%d, %d, %d, %d, %d, %d]",
        srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH,
        dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);

    ret = frame->setSrcRect(pipeId_next, srcRect);
    if (ret != NO_ERROR) {
        CLOGE("setSrcRect(Pipe:%d) failed, Fcount(%d), ret(%d)",
            pipeId_next, frame->getFrameCount(), ret);
        return ret;
    }

    ret = frame->setDstRect(pipeId_next, dstRect, dstNodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("setDstRect(Pipe:%d) failed, Fcount(%d), ret(%d)",
                pipeId_next, frame->getFrameCount(), ret);
        return ret;
    }

    /* The last Frame on HIFI and NV21 scenario can have STREAM_TYP_YUVCB_STALL only requested. */
    if (   (frame->getStreamRequested(STREAM_TYPE_YUVCB_STALL) == true)
        && (
#ifdef USES_HIFI_LLS
            (scenario == PLUGIN_SCENARIO_HIFILLS_REPROCESSING) ||
#endif
#ifdef USES_COMBINE_PLUGIN
            (scenario == PLUGIN_SCENARIO_COMBINE_REPROCESSING) ||
#endif
            (0)
           )
        && (frame->getMode(FRAME_MODE_VENDOR_YUV_STALL) == false)
        ) {
        entity_buffer_state_t entityBufferState;

        ret = frame->getSrcBufferState(pipeId_next, &entityBufferState);
        if (ret < 0) {
            CLOGE("getSrcBufferState fail, pipeId(%d), ret(%d)", pipeId_next, ret);
            return ret;
        }

        if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
            ret = m_setSrcBuffer(pipeId_next, frame, &newSrcBuffer);
            if (ret < 0) {
                CLOGE("m_setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId_next, ret);
                return ret;
            }
        }

        ret = frame->setEntityState(pipeId_next, ENTITY_STATE_PROCESSING);
        if (ret < 0) {
            CLOGE("setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)",
                 pipeId_next, ret);
            return ret;
        }

        if (frame->getRequest(PIPE_PLUGIN_POST2_REPROCESSING) == true) {
            ExynosCameraBuffer tempBuffer;
            ret = frame->getDstBuffer(pipeId_next, &tempBuffer, components.reprocessingFactory->getNodeType(pipeId_next));
            memcpy(tempBuffer.addr[tempBuffer.getMetaPlaneIndex()],
                    newSrcBuffer.addr[newSrcBuffer.getMetaPlaneIndex()], sizeof(struct camera2_shot_ext));
        }
    } else {
        ret = m_setupEntity(pipeId_next, frame, &newSrcBuffer, &newDstBuffer, dstNodeIndex);
        if (ret != NO_ERROR) {
            CLOGE("setupEntity failed, pipeId(%d), ret(%d)", pipeId_next, ret);
            return ret;
        }
    }

    return ret;
}
#endif

#ifdef SUPPORT_SENSOR_MODE_CHANGE
void ExynosCamera::m_startSensorModeTransition(void)
{
    ExynosCameraFrameFactory* factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    if (m_parameters[m_cameraId]->isSensorModeTransition() == false) {
        CLOGD("[REMOSAIC] make frame with FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_START");
        ExynosCameraFrameSP_sptr_t sensorTrangitStartFrame = factory->createNewFrame(0);
        sensorTrangitStartFrame->setFrameInfo(NULL, NULL, 0, FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_START);
        m_shotDoneQ->pushProcessQ(&sensorTrangitStartFrame);
    }
}

void ExynosCamera::m_stopSensorModeTransition(void)
{
    ExynosCameraFrameFactory* factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    if (m_parameters[m_cameraId]->isSensorModeTransition() == true) {
        CLOGD("[REMOSAIC] make frame with FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_STOP");
        ExynosCameraFrameSP_sptr_t sensorTrangitStartFrame = factory->createNewFrame(0);
        sensorTrangitStartFrame->setFrameInfo(NULL, NULL, 0, FRAME_TYPE_INTERNAL_SENSOR_TRANSITION_STOP);
        m_shotDoneQ->pushProcessQ(&sensorTrangitStartFrame);
    }
}

status_t ExynosCamera::m_switchSensorMode(ExynosCameraRequestSP_sprt_t request, bool toggle, bool streamOff)
{
    CLOGD("switchSensorMode start(%d) streamOff(%d)", toggle, streamOff);

    status_t ret = NO_ERROR;

    //FrameFactoryList previewFactoryAddrList;
    FrameFactoryListIterator factorylistIter;
    ExynosCameraFrameFactory *factory = NULL;
    ExynosCameraParameters *parameters;

    if(m_getState() != EXYNOS_CAMERA_STATE_RUN) {
        CLOGW("Cannot switch sensor current state(%d)", m_getState());
        return INVALID_OPERATION;
    }

    m_transitState(EXYNOS_CAMERA_STATE_SWITCHING_SENSOR);

    //request->getFactoryAddrList(FRAME_FACTORY_TYPE_CAPTURE_PREVIEW, &previewFactoryAddrList);
    //request->getFrameFactoryList(&previewFactoryAddrList);

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    if (factory == NULL) {
        CLOGE("factory is NULL!!");
        return INVALID_OPERATION;
    }

    if (toggle == true || (toggle == false && streamOff == true)) {
        CLOGD("stopSensorPipe (%p)", factory);
        ret = factory->stopSensorPipe();
        if (ret != NO_ERROR) {
            CLOGE("Failed to do stopSensorPipe()");
            return INVALID_OPERATION;
        }
    }

    m_parameters[m_cameraId]->sensorModeTransition(toggle);

    if (toggle == true) {
        m_configurations->setMode(CONFIGURATION_REMOSAIC_CAPTURE_ON, true);
        m_configurations->checkExtSensorMode();
        m_configurations->setMode(CONFIGURATION_REMOSAIC_CAPTURE_ON, false);
    } else {
        m_configurations->checkExtSensorMode();
    }

    if (toggle == false && streamOff == true) {
        CLOGI("stream off : sensor transition mode");
        m_transitState(EXYNOS_CAMERA_STATE_RUN);
        return NO_ERROR;
    }

    ret = factory->initSensorPipe();
    if (ret != NO_ERROR) {
        CLOGE("Failed to do initSensorPipe()");
        return INVALID_OPERATION;
    }

    //uint32_t prepare = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA];
    //prepare = prepare + parameters->getSensorControlDelay() + 1;


    m_transitState(EXYNOS_CAMERA_STATE_RUN);

    uint32_t prepare = 0;
    if (toggle) {
        /* normal -> remosaic mode */
        prepare = m_parameters[m_cameraId]->getSensorModeTransitionFrameCount();
    } else {
        /* remosaic -> normal mode */
        prepare = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA] + m_parameters[m_cameraId]->getSensorControlDelay() + 1;
    }

    CLOGD("prepare %d", prepare);

#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
    // join to allocate remosaic buffers
    m_setRemosaicBufferThread->join();
#endif

    CLOGD("stopAndWaitSensorPipeThread(%p) for previous processing", factory);
    factory->stopAndWaitSensorPipeThread();
    if (ret != NO_ERROR) {
        CLOGE("Failed to do stopAndWaitSensorPipeThread)");
        return INVALID_OPERATION;
    }

    for (uint32_t i = 0; i < prepare; i++) {
        ret = m_createSensorTransitionFrameFunc(toggle);
        if (ret != NO_ERROR) {
            CLOGE("Failed to createFrameFunc for preparing frame. prepareCount %d/%d",
                     i, prepare);
        }
    }

    CLOGD("startSensorPipe (%p)", factory);
    factory->startSensorPipe();
    if (ret != NO_ERROR) {
        CLOGE("Failed to do startSensorPipe()");
        return INVALID_OPERATION;
    }

    CLOGD("startSensorPipeThread (%p)", factory);
    factory->startSensorPipeThread();
    if (ret != NO_ERROR) {
        CLOGE("Failed to do startSensorPipeThread()");
        return INVALID_OPERATION;
    }

    CLOGD("switchSensorMode end(%d)", toggle);

    return ret;
}
#endif //SUPPORT_SENSOR_MODE_CHANGE

#ifdef WAIT_STANDBY_ON_EXCEPT_CURRENT_CAMERA
status_t ExynosCamera::m_waitDualStandbyOnForRemosaicCapture(int currentCameraId)
{
    status_t ret = NO_ERROR;

    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        && m_configurations->getMode(CONFIGURATION_ALWAYS_DUAL_FORCE_SWITCHING_MODE) == true) {
        int cameraId[MAX_NUM_SENSORS];

        for (int i = MAIN_CAM; i < MAX_NUM_SENSORS; i++) {
            int dualTransitionCount = DUAL_TRANSITION_FRAME_COUNT * 30;
            cameraId[i] = m_camIdInfo.cameraId[m_getCurrentCamIdx(false, i)];

            if (cameraId[i] > 0 && cameraId[i] != currentCameraId) {
                while (m_parameters[cameraId[i]]->getStandbyState() != DUAL_STANDBY_STATE_ON) {
                    if (dualTransitionCount > 0) {
                        if (dualTransitionCount % DUAL_TRANSITION_FRAME_COUNT == 0) {
                            CLOGD("wait for dual sub camera stream off. cameraId:%d, stanbyState(%d), dualTransitionCount(%d)",
                                    cameraId[i], m_parameters[cameraId[i]]->getStandbyState(), dualTransitionCount);
                        }
                        dualTransitionCount--;
                        usleep(3000);
                    } else {
                        android_printAssert(NULL, LOG_TAG,
                                "ASSERT(%s): sub camera stream off fail. transitionCount(%d), (cameraId %d / stanbyState %d)",
                                __FUNCTION__,
                                cameraId[i], m_parameters[cameraId[i]]->getStandbyState(), dualTransitionCount);
                    }
                }
            }
        }
    }

    return NO_ERROR;
}
#endif

void ExynosCamera::m_updateMetaDataCaptureIntent(struct camera2_shot_ext *shot_ext, uint32_t frameType)
{
    int captureIntent = -1;

    bool isRemosaicCaptureMode = false;

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    if (frameType == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
        || frameType == FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION
        )
    {
        isRemosaicCaptureMode = true;
    }
#endif

    if (isRemosaicCaptureMode == true) {
        captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_REMOSAIC_SINGLE;

        shot_ext->shot.ctl.aa.captureIntent = (enum aa_capture_intent)captureIntent;
        shot_ext->shot.ctl.aa.vendor_captureCount = 1;

        CLOGD("[REMOSAIC] captureIntent(%d), vendor_captureCount(%d)",
                shot_ext->shot.ctl.aa.captureIntent, shot_ext->shot.ctl.aa.vendor_captureCount);
    } else {
        CLOGD("[REMOSAIC] do not update captureIntent");
    }
    return;
}

void ExynosCamera::m_createCaptureStreamQ(void)
{
    /* Create queue for capture path */
    if ( m_yuvCaptureDoneQ == NULL) {
        m_yuvCaptureDoneQ = new frame_queue_t;
        m_yuvCaptureDoneQ->setWaitTime(2000000000);
    } else {
        CLOGD("m_yuvCaptureDoneQ is exist");
    }

    if (m_reprocessingDoneQ == NULL) {
        m_reprocessingDoneQ = new frame_queue_t;
        m_reprocessingDoneQ->setWaitTime(2000000000);
    } else {
        CLOGD("m_reprocessingDoneQ is exist");
    }

    if (m_bayerStreamQ == NULL) {
        m_bayerStreamQ = new frame_queue_t;
        m_bayerStreamQ->setWaitTime(2000000000);
    } else {
        CLOGD("m_bayerStreamQ is exist");
    }

#ifdef USE_DUAL_CAMERA
    if (m_scenario == SCENARIO_DUAL_REAR_ZOOM
        || m_scenario == SCENARIO_DUAL_REAR_PORTRAIT
        || m_scenario == SCENARIO_DUAL_FRONT_PORTRAIT) {
        if (m_selectDualSlaveBayerQ == NULL) {
            m_selectDualSlaveBayerQ = new frame_queue_t();
            m_selectDualSlaveBayerQ->setWaitTime(500000000); /* 500ms */

#ifdef USE_DUAL_BAYER_SYNC
            m_syncBayerFrameDoneQ = new frame_queue_t();
            m_syncBayerFrameDoneQ->setWaitTime(30000000); /* 30ms */
#endif
        } else {
            CLOGD("m_selectDualSlaveBayerQ is exist");
        }

        if (m_createReprocessingFrameQ == NULL) {
            m_createReprocessingFrameQ = new create_frame_info_queue_t(m_createReprocessingFrameThread);
        } else {
            CLOGD("m_createReprocessingFrameQ is exist");
        }

        if (m_prepareBokehAnchorCaptureQ == NULL) {
            m_prepareBokehAnchorCaptureQ = new frame_queue_t(m_prepareBokehAnchorCaptureThread);
        }

        if (m_prepareBokehAnchorMasterCaptureDoneQ == NULL) {
            m_prepareBokehAnchorMasterCaptureDoneQ = new frame_queue_t();
            m_prepareBokehAnchorMasterCaptureDoneQ->setWaitTime(500000000); /* 500ms */
        }

        if (m_prepareBokehAnchorSlaveCaptureDoneQ == NULL) {
            m_prepareBokehAnchorSlaveCaptureDoneQ = new frame_queue_t();
            m_prepareBokehAnchorSlaveCaptureDoneQ->setWaitTime(500000000); /* 500ms */
        }
    } else {
        m_createReprocessingFrameQ = NULL;
        m_selectDualSlaveBayerQ = NULL;
#ifdef USE_DUAL_BAYER_SYNC
        m_syncBayerFrameDoneQ = NULL;
#endif
    }
#endif

    if (m_captureQ == NULL) {
        m_captureQ = new frame_queue_t(m_captureThread);
    } else {
        CLOGD("m_captureQ is exist");
    }

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    if (m_reverseProcessingBayerQ == NULL) {
        m_reverseProcessingBayerQ = new frame_queue_t(m_reverseProcessingBayerThread);
    } else {
        CLOGD("m_reverseProcessingBayerQ is exist");
    }
#endif

}

void ExynosCamera::m_releaseCaptureStreamQ(void)
{
    CLOGI("-IN-");

    if (isOfflineCaptureRunning()) {
        CLOGD("[OFFLINE] offline capture is running");
        return;
    }

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    if (m_reverseProcessingBayerQ != NULL) {
        m_reverseProcessingBayerQ->release();
    }
#endif

    if (m_yuvCaptureDoneQ != NULL) {
        m_yuvCaptureDoneQ->release();
    }

    if (m_reprocessingDoneQ != NULL) {
        m_reprocessingDoneQ->release();
    }

    if (m_captureQ != NULL) {
        m_captureQ->release();
    }

    if (m_bayerStreamQ != NULL) {
        m_bayerStreamQ->release();
    }

    if (m_selectBayerQ != NULL) {
        m_selectBayerQ->release();
    }

#ifdef USE_DUAL_CAMERA
    if (m_selectDualSlaveBayerQ != NULL) {
        m_selectDualSlaveBayerQ->release();
    }

#ifdef USE_DUAL_BAYER_SYNC
    if (m_syncBayerFrameDoneQ != NULL) {
        m_syncBayerFrameDoneQ->release();
    }
#endif

    if (m_prepareBokehAnchorCaptureQ != NULL) {
        m_prepareBokehAnchorCaptureQ->release();
    }

    if (m_prepareBokehAnchorMasterCaptureDoneQ != NULL) {
        m_prepareBokehAnchorMasterCaptureDoneQ->release();
    }

    if (m_prepareBokehAnchorSlaveCaptureDoneQ != NULL) {
        m_prepareBokehAnchorSlaveCaptureDoneQ->release();
    }
#endif
}

void ExynosCamera::m_destroyCaptureStreamQ(void)
{
    CLOGI("-IN-");

    if (isOfflineCaptureRunning()) {
        CLOGD("[OFFLINE] offline capture is running");
        return;
    }

#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    if (m_reverseProcessingBayerQ != NULL) {
        delete m_reverseProcessingBayerQ;
        m_reverseProcessingBayerQ = NULL;
    }
#endif

    if (m_yuvCaptureDoneQ != NULL) {
        delete m_yuvCaptureDoneQ;
        m_yuvCaptureDoneQ = NULL;
    }

    if (m_reprocessingDoneQ != NULL) {
        delete m_reprocessingDoneQ;
        m_reprocessingDoneQ = NULL;
    }

    if (m_captureQ != NULL) {
        delete m_captureQ;
        m_captureQ = NULL;
    }

    if (m_bayerStreamQ != NULL) {
        delete m_bayerStreamQ;
        m_bayerStreamQ = NULL;
    }

    if (m_selectBayerQ != NULL) {
        delete m_selectBayerQ;
        m_selectBayerQ = NULL;
    }

#ifdef USE_DUAL_CAMERA
    if (m_selectDualSlaveBayerQ != NULL) {
        delete m_selectDualSlaveBayerQ;
        m_selectDualSlaveBayerQ = NULL;
    }

#ifdef USE_DUAL_BAYER_SYNC
    if (m_syncBayerFrameDoneQ != NULL) {
        delete m_syncBayerFrameDoneQ;
        m_syncBayerFrameDoneQ = NULL;
    }
#endif

    if (m_createReprocessingFrameQ != NULL) {
        delete m_createReprocessingFrameQ;
        m_createReprocessingFrameQ = NULL;
    }

    if (m_prepareBokehAnchorCaptureQ != NULL) {
        delete m_prepareBokehAnchorCaptureQ;
        m_prepareBokehAnchorCaptureQ = NULL;
    }

    if (m_prepareBokehAnchorMasterCaptureDoneQ != NULL) {
        delete m_prepareBokehAnchorMasterCaptureDoneQ;
        m_prepareBokehAnchorMasterCaptureDoneQ = NULL;
    }

    if (m_prepareBokehAnchorSlaveCaptureDoneQ != NULL) {
        delete m_prepareBokehAnchorSlaveCaptureDoneQ;
        m_prepareBokehAnchorSlaveCaptureDoneQ = NULL;
    }
#endif

    if (m_resizeYuvDoneQ != NULL) {
        delete m_resizeYuvDoneQ;
        m_resizeYuvDoneQ = NULL;
    }

}

void ExynosCamera::m_captureThreadStopAndInputQ(void)
{
    CLOGI("-IN-");

    if (isOfflineCaptureRunning()) {
        return;
    }

    stopThreadAndInputQ(m_bayerStreamThread, 1, m_bayerStreamQ);
    stopThreadAndInputQ(m_captureThread, 2, m_captureQ, m_reprocessingDoneQ);
    stopThreadAndInputQ(m_thumbnailCbThread, 2, m_thumbnailCbQ, m_thumbnailPostCbQ);
    stopThreadAndInputQ(m_captureStreamThread, 1, m_yuvCaptureDoneQ);
#if defined(USE_RAW_REVERSE_PROCESSING) && defined(USE_SW_RAW_REVERSE_PROCESSING)
    stopThreadAndInputQ(m_reverseProcessingBayerThread, 1, m_reverseProcessingBayerQ);
#endif
#ifdef USE_DUAL_CAMERA
    stopThreadAndInputQ(m_prepareBokehAnchorCaptureThread,
                        3, m_prepareBokehAnchorCaptureQ,
                        m_prepareBokehAnchorMasterCaptureDoneQ, m_prepareBokehAnchorSlaveCaptureDoneQ);
#endif

    return;
}

status_t ExynosCamera::m_stopPipeline(void)
{
    CLOGI("-IN-");

    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *frameFactory = NULL;

    for (int i = FRAME_FACTORY_TYPE_MAX - 1; i >= 0; i--) {
        if (m_frameFactory[i] != NULL) {
            frameFactory = m_frameFactory[i];

            if(isOfflineCaptureRunning()) {
                if(frameFactory->isForReprocessing()) {
                    CLOGD("[OFFLINE] skip deinit reprocessing factory");
                    continue;
                }
            }

            for (int k = i - 1; k >= 0; k--) {
               if (frameFactory == m_frameFactory[k]) {
                   CLOGD("m_frameFactory index(%d) and index(%d) are same instance,"
                        " set index(%d) = NULL", i, k, k);
                   m_frameFactory[k] = NULL;
               }
            }

            TIME_LOGGER_UPDATE(m_cameraId, m_frameFactory[i]->getFactoryType(), m_frameFactory[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_STREAM_STOP_START, 0);
            ret = m_stopFrameFactory(m_frameFactory[i]);
            TIME_LOGGER_UPDATE(m_cameraId, m_frameFactory[i]->getFactoryType(), m_frameFactory[i]->getCameraId(), CUMULATIVE_CNT, FACTORY_STREAM_STOP_END, 0);
            if (ret < 0)
                CLOGE("m_frameFactory[%d] stopPipes fail", i);

            CLOGD("m_frameFactory[%d] stopPipes", i);
        }
    }

    return ret;
}

void ExynosCamera::m_setFrameDoneQtoThread(void)
{
    CLOGI("-IN-");

    /* m_previewStreamXXXThread is for seperated frameDone each own handler */
    m_previewStreamBayerThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_FLITE]);

    m_previewStream3AAThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_3AA]);

    m_previewStreamISPThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_ISP]);

    m_previewStreamMCSCThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_MCSC]);

#ifdef SUPPORT_HW_GDC
    m_previewStreamGDCThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_GDC]);
#endif

#ifdef USE_CLAHE_PREVIEW
    m_previewStreamCLAHEThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_CLAHE]);
#endif

#ifdef USE_SLSI_PLUGIN
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        m_previewStreamPlugInThreadMap[i]->setFrameDoneQ(m_pipeFrameDoneQ[i]);
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    m_previewStreamSWMCSCThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_SW_MCSC]);
#endif

#ifdef USES_SW_VDIS
    m_previewStreamVDISThread->setFrameDoneQ(m_pipeFrameDoneQ[PIPE_VDIS]);
#endif
}

int ExynosCamera::m_getCameraSessionId(ExynosCameraFrameSP_sptr_t frame)
{
    int cameraSessionId = 0;
#ifdef USES_OFFLINE_CAPTURE
    cameraSessionId = m_offlineCapture->getCameraSessionId(frame);
#endif
    return cameraSessionId;
}

int ExynosCamera::m_getCameraSessionId(ExynosCameraRequestSP_sprt_t request)
{
    int cameraSessionId = 0;
#ifdef USES_OFFLINE_CAPTURE
    cameraSessionId = m_offlineCapture->getCameraSessionId(request);
#endif
    return cameraSessionId;
}

ExynosCameraRequestManager* ExynosCamera::m_getRequestManager(ExynosCameraFrameSP_sptr_t frame)
{
    return m_resourceManager->getRequestManager(m_getCameraSessionId(frame));
}

ExynosCameraRequestManager* ExynosCamera::m_getRequestManager(ExynosCameraRequestSP_sprt_t request)
{
    return m_resourceManager->getRequestManager(m_getCameraSessionId(request));
}

#ifdef USES_OFFLINE_CAPTURE
status_t ExynosCamera::m_rearrangeResources(int cameraSessionId)
{
    status_t ret = NO_ERROR;

    int resourceRefCount = m_resourceManager->getRefCount();
    CLOGD("[OFFLINE] cameraSessionId(%d), resourceRefCount(%d)", cameraSessionId, resourceRefCount);

    if (cameraSessionId != resourceRefCount) {

        if (isOfflineCaptureRunning() != true) {
            m_resourceManager->deInitBufferSupplier(&m_bufferSupplier, &m_ionAllocator);
        }

        m_resourceManager->cloneResources(cameraSessionId);

        m_bufferSupplier = new ExynosCameraBufferSupplier(m_cameraId);

        ret = m_createIonAllocator(&m_ionAllocator);
        if (ret != NO_ERROR) {
            CLOGE("Failed to create ionAllocator. ret %d", ret);
        } else {
            CLOGD("IonAllocator is created");
        }

        m_resourceManager->initBufferSupplier(m_bufferSupplier, m_ionAllocator);

        for (int i = 0; i < m_camIdInfo.numOfSensors; i++) {
            if (m_captureSelector[m_camIdInfo.cameraId[i]] != NULL) {
                m_captureSelector[m_camIdInfo.cameraId[i]]->setBufferSupplier(m_bufferSupplier);
            }
        }

        if (m_captureZslSelector != NULL) {
            m_captureZslSelector->setBufferSupplier(m_bufferSupplier);
        }
    }

    if (m_bufferSupplier == nullptr) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):bufferSupplier is NULL!!", __FUNCTION__, __LINE__);
    }

    return ret;
}
#endif
}; /* namespace android */
