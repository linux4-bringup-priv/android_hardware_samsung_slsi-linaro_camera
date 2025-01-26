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
#define LOG_TAG "ExynosCameraFrameFactoryPreview"
#include <log/log.h>

#include "ExynosCameraFrameFactoryPreview.h"

namespace android {

ExynosCameraFrameFactoryPreview::~ExynosCameraFrameFactoryPreview()
{
    status_t ret = NO_ERROR;

    if (m_shot_ext != NULL) {
        delete m_shot_ext;
        m_shot_ext = NULL;
    }

    ret = destroy();
    if (ret != NO_ERROR)
        CLOGE("destroy fail");
}

status_t ExynosCameraFrameFactoryPreview::create()
{
    Mutex::Autolock lock(ExynosCameraStreamMutex::getInstance()->getStreamMutex());
    CLOGI("");

    status_t ret = NO_ERROR;
    uint32_t pipeId = PIPE_3AA;

    ret = ExynosCameraFrameFactoryBase::create();
    if (ret != NO_ERROR) {
        CLOGE("Pipe create fail, ret(%d)", ret);
        return ret;
    }

#ifdef USE_VRA_FD
    if (m_flag3aaVraOTF != HW_CONNECTION_MODE_NONE)
        pipeId = PIPE_VRA;
#endif

    /* EOS */
    ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", pipeId, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* s_ctrl HAL version for selecting dvfs table */
    ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_HAL_VERSION, IS_HAL_VER_3_2);
    if (ret < 0)
        CLOGW("V4L2_CID_IS_HAL_VERSION is fail");

    ret = m_transitState(FRAME_FACTORY_STATE_CREATE);

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::postcreate(void)
{
    CLOGI("");
    status_t ret = NO_ERROR;

    ret = ExynosCameraFrameFactoryBase::postcreate();
    if (ret != NO_ERROR) {
        CLOGE("Pipe create fail, ret(%d)", ret);
        return ret;
    }

    /* EOS */
    ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", PIPE_3AA, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* s_ctrl HAL version for selecting dvfs table */
    ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_HAL_VERSION, IS_HAL_VER_3_2);
    if (ret < 0)
        CLOGW("WARN(%s): V4L2_CID_IS_HAL_VERSION is fail", __FUNCTION__);

    ret = m_transitState(FRAME_FACTORY_STATE_CREATE);

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::fastenAeStable(int32_t numFrames, ExynosCameraBuffer *buffers)
{
    CLOGI(" Start");

    status_t ret = NO_ERROR;
    status_t totalRet = NO_ERROR;

    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraFrameEntity *newEntity = NULL;
    frame_queue_t instantQ;

    int hwSensorW = 0, hwSensorH = 0;
    int bcropX = 0, bcropY = 0, bcropW = 0, bcropH = 0;
    int hwPreviewW = 0, hwPreviewH = 0;
    int index = 0;
    uint32_t minFrameRate, maxFrameRate, sensorFrameRate = 0;
    struct v4l2_streamparm streamParam;

    int pipeId = PIPE_3AA;

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    }

    if (numFrames == 0) {
        CLOGW("umFrames is %d, we skip fastenAeStable", numFrames);
        return NO_ERROR;
    }

    if (m_configurations->getCameraId() == CAMERA_ID_FRONT) {
        sensorFrameRate = FASTEN_AE_FPS_FRONT;
    } else {
        sensorFrameRate = FASTEN_AE_FPS;
    }

    /* 1. Initialize pipes */
    m_parameters->getFastenAeStableSensorSize(&hwSensorW, &hwSensorH, index);
    m_parameters->getFastenAeStableBcropSize(&bcropW, &bcropH, index);
    m_parameters->getFastenAeStableBdsSize(&hwPreviewW, &hwPreviewH, index);

    bcropX = ALIGN_UP(((hwSensorW - bcropW) >> 1), 2);
    bcropY = ALIGN_UP(((hwSensorH - bcropH) >> 1), 2);

    /* x - y */
    if (bcropX < 0) {
        CLOGE("invalid bcropX.(%d)", bcropX);
        bcropX = 0;
    }
    if (bcropY < 0) {
        CLOGE("invalid bcropY.(%d)", bcropY);
        bcropY = 0;
    }

    if (bcropW < hwPreviewW || bcropH < hwPreviewH) {
        CLOGD("bayerCropSize %dx%d is smaller than BDSSize %dx%d. Force bayerCropSize",
            bcropW, bcropH, hwPreviewW, hwPreviewH);

        hwPreviewW = bcropW;
        hwPreviewH = bcropH;
    }

    CLOGI("numFrames = %d hwSensorW = %d hwSensorH = %d sensorFrameRate = %d",
        numFrames, hwSensorW, hwSensorH, sensorFrameRate);

    /*
     * we must set flite'setupPipe on 3aa_pipe.
     * then, setControl/getControl about BNS size
     */
    ret = m_initPipesFastenAeStable(numFrames, hwSensorW, hwSensorH, sensorFrameRate);
    if (ret != NO_ERROR) {
        CLOGE("m_initPipesFastenAeStable() fail, ret(%d)", ret);
        return ret;
    }

    for (int i = 0; i < numFrames; i++) {
        /* 2. Generate instant frames */
        newFrame = m_frameMgr->createFrame(m_configurations, i);

        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            ret = INVALID_OPERATION;
            goto cleanup;
        }

        ret = m_initFrameMetadata(newFrame);
        if (ret != NO_ERROR)
            CLOGE("frame(%d) metadata initialize fail", i);

        newEntity = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        newFrame->addSiblingEntity(NULL, newEntity);
        newFrame->setNumRequestPipe(1);

        newEntity->setSrcBuf(buffers[i]);

        /* 3. Set metadata for instant on */
        camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buffers[i].addr[buffers[i].getMetaPlaneIndex()]);

        if (shot_ext != NULL) {
            int aeRegionX = (hwSensorW) / 2;
            int aeRegionY = (hwSensorH) / 2;

            newFrame->getMetaData(shot_ext);
            m_parameters->duplicateCtrlMetadata((void *)shot_ext);
            m_activityControl->activityBeforeExecFunc(PIPE_3AA, (void *)&buffers[i]);

            setMetaCtlAeTargetFpsRange(shot_ext, sensorFrameRate, sensorFrameRate);
            setMetaCtlSensorFrameDuration(shot_ext, (uint64_t)((1000 * 1000 * 1000) / (uint64_t)sensorFrameRate));

            /* set afMode into INFINITY */
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_CANCEL;
            shot_ext->shot.ctl.aa.vendor_afmode_option &= (0 << AA_AFMODE_OPTION_BIT_MACRO);

            setMetaCtlAeRegion(shot_ext, aeRegionX, aeRegionY, aeRegionX, aeRegionY, 0);

            /* Set video mode off for fastAE */
            setMetaVideoMode(shot_ext, AA_VIDEOMODE_OFF);

            /* Set 3AS size */
            enum NODE_TYPE nodeType = getNodeType(PIPE_3AA);
            int nodeNum = m_deviceInfo[PIPE_3AA].nodeNum[nodeType];
            if (nodeNum <= 0) {
                CLOGE(" invalid nodeNum(%d). so fail", nodeNum);
                ret = INVALID_OPERATION;
                goto cleanup;
            }

            setMetaNodeLeaderVideoID(shot_ext, nodeNum - FIMC_IS_VIDEO_BAS_NUM);
            setMetaNodeLeaderRequest(shot_ext, false);
            setMetaNodeLeaderInputSize(shot_ext, bcropX, bcropY, bcropW, bcropH);

            /* Set 3AP size */
            nodeType = getNodeType(PIPE_3AP);
            nodeNum = m_deviceInfo[PIPE_3AA].nodeNum[nodeType];
            if (nodeNum <= 0) {
                CLOGE(" invalid nodeNum(%d). so fail", nodeNum);
                ret = INVALID_OPERATION;
                goto cleanup;
            }

            int perframePosition = 0;
            setMetaNodeCaptureVideoID(shot_ext, perframePosition, nodeNum - FIMC_IS_VIDEO_BAS_NUM);
            setMetaNodeCaptureRequest(shot_ext, perframePosition, false);
            setMetaNodeCaptureOutputSize(shot_ext, perframePosition, 0, 0, hwPreviewW, hwPreviewH);

            /* Set ISPC/ISPP size (optional) */
            if (m_flag3aaIspOTF != HW_CONNECTION_MODE_M2M) {
                if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
                    /* Case of ISP-MCSC M2M */
                    nodeType = getNodeType(PIPE_ISPC);
                } else {
                    /* Case of ISP-MCSC OTF */
                    nodeType = getNodeType(PIPE_MCSC0);
                }

                nodeNum = m_deviceInfo[PIPE_3AA].nodeNum[nodeType];
                if (nodeNum <= 0) {
                    CLOGE(" invalid nodeNum(%d). so fail", nodeNum);
                    ret = INVALID_OPERATION;
                    goto cleanup;
                }

                perframePosition = 1; /* 3AP:0, ISPC/ISPP:1 */
                setMetaNodeCaptureVideoID(shot_ext, perframePosition, nodeNum - FIMC_IS_VIDEO_BAS_NUM);
                setMetaNodeCaptureRequest(shot_ext, perframePosition, false);
                setMetaNodeCaptureOutputSize(shot_ext, perframePosition, 0, 0, hwPreviewW, hwPreviewH);
            }
        }

        /* 4. Push instance frames to pipe */
        ret = pushFrameToPipe(newFrame, PIPE_3AA);
        if (ret != NO_ERROR) {
            CLOGE(" pushFrameToPipeFail, ret(%d)", ret);
            goto cleanup;
        }
        CLOGD("Instant shot - FD(%d, %d)", buffers[i].fd[0], buffers[i].fd[1]);

        instantQ.pushProcessQ(&newFrame);
    }

    /* 5. Pipe instant on */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_FLITE]->instantOn(0);
        if (ret != NO_ERROR) {
            CLOGE(" FLITE On fail, ret(%d)", ret);
            goto cleanup;
        }
    }

#ifdef USE_VRA_FD
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) || (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = m_pipes[PIPE_VRA]->start();
        if (ret != NO_ERROR) {
            CLOGE("VRA start fail, ret(%d)", ret);
            goto cleanup;
        }
    }
#endif

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_MCSC]->start();
        if (ret != NO_ERROR) {
            CLOGE("MCSC start fail, ret(%d)", ret);
            goto cleanup;
        }
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_ISP)]->start();
        if (ret < 0) {
            CLOGE("ISP start fail, ret(%d)", ret);
            goto cleanup;
        }
    }

    ret = m_pipes[PIPE_3AA]->instantOn(numFrames);
    if (ret != NO_ERROR) {
        CLOGE("3AA instantOn fail, ret(%d)", ret);
        goto cleanup;
    }

    CLOGI(" Start S_STREAM");

    /* 6. SetControl to sensor instant on */
    ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_S_STREAM, (1 | numFrames << SENSOR_INSTANT_SHIFT));
    if (ret != NO_ERROR) {
        CLOGE("instantOn fail, ret(%d)", ret);
        goto cleanup;
    }

    CLOGI(" End S_STREAM");

cleanup:
    totalRet |= ret;

    /* 7. Pipe instant off */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_FLITE]->instantOff();
        if (ret != NO_ERROR) {
            CLOGE(" FLITE Off fail, ret(%d)", ret);
        }
    }

    ret = m_pipes[PIPE_3AA]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret != NO_ERROR) {
        CLOGE("3AA force done fail, ret(%d)", ret);
    }

    ret = m_pipes[PIPE_3AA]->instantOff();
    if (ret != NO_ERROR) {
        CLOGE("3AA instantOff fail, ret(%d)", ret);
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_ISP)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("ISP stop fail, ret(%d)", ret);
        }
    }

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_MCSC]->stop();
        if (ret != NO_ERROR) {
            CLOGE("MCSC stop fail, ret(%d)", ret);
        }
    }

#ifdef USE_VRA_FD
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) || (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = m_pipes[PIPE_VRA]->stop();
        if (ret != NO_ERROR) {
            CLOGE("VRA stop fail, ret(%d)", ret);
        }
    }
#endif

    /* 8. Rollback framerate after fastenfeenable done */
    /* setParam for Frame rate : must after setInput on Flite */
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    m_configurations->getPreviewFpsRange(&minFrameRate, &maxFrameRate);
    sensorFrameRate = maxFrameRate;

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = sensorFrameRate;
    CLOGI("set framerate (denominator=%d)", sensorFrameRate);

    ret = setParam(&streamParam, pipeId);
    if (ret != NO_ERROR) {
        CLOGE("setParam(%d) fail, ret(%d)", pipeId, ret);
        return INVALID_OPERATION;
    }

    /* 9. Clean up all frames */
    for (int i = 0; i < numFrames; i++) {
        newFrame = NULL;
        if (instantQ.getSizeOfProcessQ() == 0)
            break;

        ret = instantQ.popProcessQ(&newFrame);
        if (ret != NO_ERROR) {
            CLOGE("pop instantQ fail, ret(%d)", ret);
            continue;
        }

        if (newFrame == NULL) {
            CLOGE("newFrame is NULL,");
            continue;
        }

        newFrame = NULL;
    }

    CLOGI("Done");

    ret |= totalRet;
    return ret;
}

status_t ExynosCameraFrameFactoryPreview::initPipes(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    int hwSensorW = 0, hwSensorH = 0;
    uint32_t minFrameRate = 0, maxFrameRate = 0, sensorFrameRate = 0;

    m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    m_configurations->getPreviewFpsRange(&minFrameRate, &maxFrameRate);
    sensorFrameRate = maxFrameRate;

#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE
    if (m_configurations->getMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE)) {
        int vendorFps = 0;
        vendorFps = m_configurations->getModeValue(CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE);
        sensorFrameRate = vendorFps;
        CLOGD("Override vendor :FPS(%d)", vendorFps);
    }
#endif

    /* default settting */
    m_sensorStandby = true;
    m_needSensorStreamOn = true;

    /* setDeviceInfo does changing path */
    ret = m_setupConfig();
    if (ret != NO_ERROR) {
        CLOGE("m_setupConfig() fail");
        return ret;
    }

    /*
     * we must set flite'setupPipe on 3aa_pipe.
     * then, setControl/getControl about BNS size
     */
    ret = m_initPipes(sensorFrameRate);
    if (ret != NO_ERROR) {
        CLOGE("m_initPipes() fail");
        return ret;
    }

    ret = m_initFlitePipe(hwSensorW, hwSensorH, sensorFrameRate);
    if (ret != NO_ERROR) {
        CLOGE("m_initFlitePipe() fail");
        return ret;
    }

    m_frameCount = 0;

    ret = m_transitState(FRAME_FACTORY_STATE_INIT);

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::mapBuffers(void)
{
    status_t ret = NO_ERROR;

    ret = m_pipes[PIPE_3AA]->setMapBuffer();
    if (ret != NO_ERROR) {
        CLOGE("3AA mapBuffer fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_ISP]->setMapBuffer();
        if (ret != NO_ERROR) {
            CLOGE("ISP mapBuffer fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_MCSC]->setMapBuffer();
        if (ret != NO_ERROR) {
            CLOGE("MCSC mapBuffer fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

#ifdef USE_VRA_FD
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) || (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = m_pipes[PIPE_VRA]->setMapBuffer();
        if (ret != NO_ERROR) {
            CLOGE("MCSC mapBuffer fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_CLAHE]->setMapBuffer();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE mapBuffer fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

    CLOGI("Map buffer Success!");

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::preparePipes(void)
{
    /* NOTE: Prepare for 3AA is moved after ISP stream on */
    /* we must not qbuf before stream on, when sensor group. */
    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::startInitialThreads(void)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;

    CLOGI("start pre-ordered initial pipe thread");

    if (m_sensorStandby == false) {
        if (m_flagPipeXNeedToRun(PIPE_FLITE) == true) {
            ret = startThread(PIPE_FLITE);
            if (ret != NO_ERROR)
                return ret;
        }

        ret = startThread(PIPE_3AA);
        if (ret != NO_ERROR)
            return ret;
    } else {
        CLOGI("skip the starting Sensor Thread");
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = startThread(PIPE_ISP);
        if (ret != NO_ERROR)
            return ret;
    }

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = startThread(PIPE_MCSC);
        if (ret != NO_ERROR)
            return ret;
    }

#ifdef USE_VRA_FD
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) || (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = startThread(PIPE_VRA);
        if (ret != NO_ERROR)
            return ret;
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE) == true) {
            ret = startThread(PIPE_CLAHE);
            if (ret != NO_ERROR)
                return ret;
        }
    }
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::setStopFlag(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[PIPE_FLITE]->setStopFlag();
    }

    if (m_pipes[PIPE_3AA]->flagStart() == true)
        ret = m_pipes[PIPE_3AA]->setStopFlag();

    if (m_pipes[PIPE_ISP]->flagStart() == true)
        ret = m_pipes[PIPE_ISP]->setStopFlag();

    if (m_pipes[PIPE_MCSC]->flagStart() == true)
        ret = m_pipes[PIPE_MCSC]->setStopFlag();

#ifdef USE_VRA_FD
    if (m_pipes[PIPE_VRA] != NULL && m_pipes[PIPE_VRA]->flagStart() == true)
        ret = m_pipes[PIPE_VRA]->setStopFlag();
#endif

#ifdef USE_CLAHE_PREVIEW
    if (m_pipes[PIPE_CLAHE]->flagStart() == true)
        ret = m_pipes[PIPE_CLAHE]->setStopFlag();
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    if (m_pipes[PIPE_SW_MCSC] != NULL && m_pipes[PIPE_SW_MCSC]->flagStart() == true)
        ret = m_pipes[PIPE_SW_MCSC]->setStopFlag();
#endif

    return NO_ERROR;
}

/*
 * It is called when something seriously wrong with the current sensor state.
 * It's intended to reset the sensor state
 */
status_t ExynosCameraFrameFactoryPreview::sensorPipeForceDone(void)
{
    status_t ret = NO_ERROR;

    int pipeId = -1;

    if (m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    ret = m_pipes[pipeId]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret != NO_ERROR) {
       CLOGE("Pipe(%d) force done fail, ret(%d)", pipeId, ret);
    }

   return ret;
}

status_t ExynosCameraFrameFactoryPreview::sensorStandby(bool flagStandby, bool standByHintOnly)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;

    int pipeId = -1;

    if (m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    //sensor PIPE
    if (m_pipes[pipeId] != NULL)
        m_pipes[INDEX(pipeId)]->setStandbyHint(flagStandby);

    if (standByHintOnly) {
        if (m_pipes[pipeId] != NULL)
            CLOGI("Sensor Standby : setStandbyHint (%s)", (flagStandby ? "On" : "Off"));
        return ret;
    }

    if (m_sensorStandby == flagStandby &&
            m_pipes[pipeId]->flagStart() == true) {
        CLOGI("already sensor standby(%d)", flagStandby);
        return ret;
    }

    m_sensorStandby = flagStandby;

    ret = m_pipes[pipeId]->sensorStandby(flagStandby);
    if (ret != NO_ERROR) {
        CLOGE("Sensor standby(%s) fail! ret(%d)",
                (flagStandby?"On":"Off"), ret);
    }

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            m_pipes[INDEX(i)]->setStandbyHint(flagStandby);
        }
    }

    CLOGI("Sensor Standby(%s) End", (flagStandby ? "On" : "Off"));

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::m_setDeviceInfo(void)
{
    CLOGI("");

    int pipeId = -1;
    int node3aa = -1, node3ac = -1, node3ap = -1, node3af = -1;
    int nodeIsp = -1, nodeIspc = -1, nodeIspp = -1;
    int nodeMcsc = -1, nodeMcscp0 = -1, nodeMcscp1 = -1, nodeMcscp2 = -1, nodeMcscpDs = -1;
    int nodeMcscp3 = -1, nodeMcscp4 = -1;
    int nodePaf = -1;
    int nodeVra = -1, nodeMe = FIMC_IS_VIDEO_ME0C_NUM;
    int nodeClh = -1, nodeClhc = -1;
    int previousPipeId = -1;
    int mcscSrcPipeId = -1;
    int pipeId3aaSrc = -1;
    int vraSrcPipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    bool flagStreamLeader = true;

    m_initDeviceInfo(PIPE_FLITE);
    m_initDeviceInfo(PIPE_3AA);
    m_initDeviceInfo(PIPE_ISP);
    m_initDeviceInfo(PIPE_MCSC);
#ifdef USE_VRA_FD
    m_initDeviceInfo(PIPE_VRA);
#endif
#ifdef USE_CLAHE_PREVIEW
    m_initDeviceInfo(PIPE_CLAHE);
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    m_initDeviceInfo(PIPE_SW_MCSC);
#endif

#ifdef SUPPORT_VIRTUALFD_PREVIEW
    int virtualNodeMCSC[MCSC_PORT_MAX] = {-1};
    int virtualNodeDS = -1;
#endif

#ifdef USE_DUAL_CAMERA
    if (m_camIdInfo->cameraId[SUB_CAM] == getCameraId()
        || m_camIdInfo->cameraId[SUB_CAM2] == getCameraId()) {
        node3aa = FIMC_IS_VIDEO_31S_NUM;
        node3ac = FIMC_IS_VIDEO_31C_NUM;
        node3ap = FIMC_IS_VIDEO_31P_NUM;
        node3af = FIMC_IS_VIDEO_31F_NUM;
#ifdef USE_PAF
        nodePaf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
    } else
#endif
    if (m_configurations->getMode(CONFIGURATION_PIP_SUB_CAM_MODE) == true) {
        node3aa = FIMC_IS_VIDEO_31S_NUM;
        node3ac = FIMC_IS_VIDEO_31C_NUM;
        node3ap = FIMC_IS_VIDEO_31P_NUM;
        node3af = FIMC_IS_VIDEO_31F_NUM;
#ifdef USE_PAF
        nodePaf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
    } else {
#if defined(FRONT_CAMERA_3AA_NUM)
        if (isFrontCamera(getCameraId()) == true) {
        if (FRONT_CAMERA_3AA_NUM == FIMC_IS_VIDEO_30S_NUM) {
            node3aa = FIMC_IS_VIDEO_30S_NUM;
            node3ac = FIMC_IS_VIDEO_30C_NUM;
            node3ap = FIMC_IS_VIDEO_30P_NUM;
            node3af = FIMC_IS_VIDEO_30F_NUM;
#ifdef USE_PAF
            nodePaf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif
        } else {
            node3aa = FIMC_IS_VIDEO_31S_NUM;
            node3ac = FIMC_IS_VIDEO_31C_NUM;
            node3ap = FIMC_IS_VIDEO_31P_NUM;
            node3af = FIMC_IS_VIDEO_31F_NUM;
#ifdef USE_PAF
            nodePaf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
        }
    } else
#endif
        {
            /* default single path */
            node3aa = FIMC_IS_VIDEO_30S_NUM;
            node3ac = FIMC_IS_VIDEO_30C_NUM;
            node3ap = FIMC_IS_VIDEO_30P_NUM;
            node3af = FIMC_IS_VIDEO_30F_NUM;
#ifdef USE_PAF
            nodePaf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif
        }
    }

#ifdef USE_3AG_CAPTURE
    if (node3ac == FIMC_IS_VIDEO_30C_NUM ||
        node3ac == FIMC_IS_VIDEO_31C_NUM)
        node3ac = CONVERT_3AC_TO_3AG(node3ac);
#endif

    if (m_parameters->isUseVideoHQISP() == false) {
        nodeIsp = FIMC_IS_VIDEO_I0S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I0C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I0P_NUM;
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
    } else {
        nodeIsp = FIMC_IS_VIDEO_I1S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I1C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I1P_NUM;
        nodeMcsc = FIMC_IS_VIDEO_M1S_NUM;
    }

#ifdef SUPPORT_ME
    nodeMe = FIMC_IS_VIDEO_ME0C_NUM;
#else
    nodeMe = -1;
#endif

    nodeMcscp0 = FIMC_IS_VIDEO_M0P_NUM;
    nodeMcscp1 = FIMC_IS_VIDEO_M1P_NUM;
    nodeMcscp2 = FIMC_IS_VIDEO_M2P_NUM;
    if (m_parameters->isUseVideoHQISP() == true) {
        nodeMcscp3 = FIMC_IS_VIDEO_M3P_NUM;
        nodeMcscp4 = FIMC_IS_VIDEO_M4P_NUM;
    }
    nodeVra = FIMC_IS_VIDEO_VRA_NUM;

#ifdef SUPPORT_VIRTUALFD_PREVIEW
    virtualNodeMCSC[MCSC_PORT_0] = nodeMcscp0;
    virtualNodeMCSC[MCSC_PORT_1] = nodeMcscp1;
    virtualNodeMCSC[MCSC_PORT_2] = nodeMcscp2;
    virtualNodeMCSC[MCSC_PORT_3] = nodeMcscp3;
    virtualNodeMCSC[MCSC_PORT_4] = nodeMcscp4;
    virtualNodeDS = FIMC_IS_VIDEO_M5P_NUM;
#endif

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        nodeMcscpDs = FIMC_IS_VIDEO_M5P_NUM;
        break;
    case 3:
        nodeMcscpDs = FIMC_IS_VIDEO_M3P_NUM;
        break;
    case 1:
        nodeMcscpDs = PREVIEW_GSC_NODE_NUM;
        break;
    default:
        CLOGE("invalid output port(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

#ifdef USE_CLAHE_PREVIEW
    nodeClh = FIMC_IS_VIDEO_CLH0S_NUM;
    nodeClhc = FIMC_IS_VIDEO_CLH0C_NUM;
#endif

    /*
     * FLITE
     */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    /* FLITE */
    nodeType = getNodeType(PIPE_FLITE);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_FLITE;
    m_deviceInfo[pipeId].nodeNum[nodeType] = getFliteNodenum(m_cameraId);
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "FLITE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[nodeType], false, flagStreamLeader, m_flagReprocessing);

    /* Other nodes is not stream leader */
    flagStreamLeader = false;

    /* VC0 for bayer */
    nodeType = getNodeType(PIPE_VC0);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VC0;
    m_deviceInfo[pipeId].nodeNum[nodeType] = getFliteCaptureNodenum(m_cameraId, m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_FLITE)]);
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "BAYER", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_FLITE)], false, flagStreamLeader, m_flagReprocessing);

#ifdef SUPPORT_DEPTH_MAP
    /* VC1 for depth */
    if (m_parameters->isDepthMapSupported()) {
        nodeType = getNodeType(PIPE_VC1);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VC1;
        m_deviceInfo[pipeId].nodeNum[nodeType] = getDepthVcNodeNum(m_cameraId);
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "DEPTH", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_FLITE)], false, flagStreamLeader, m_flagReprocessing);
    }
#endif // SUPPORT_DEPTH_MAP

#ifdef SUPPORT_PD_IMAGE
    /* VC1 for depth */
    if (m_parameters->isPDImageSupported()) {
        nodeType = getNodeType(PIPE_VC1);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VC1;
        m_deviceInfo[pipeId].nodeNum[nodeType] = getDepthVcNodeNum(m_cameraId); //same as depth
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "DEPTH", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_FLITE)], false, flagStreamLeader, m_flagReprocessing);
    }
#endif // SUPPORT_PD_IMAGE

    if (m_parameters->isSensorGyroSupported() == true) {
        /* VC3 for sensor gyro */
        nodeType = getNodeType(PIPE_VC3);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_VC3;
        m_deviceInfo[pipeId].nodeNum[nodeType] = getSensorGyroNodeNum(m_cameraId);
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "SENSOR_GYRO", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_FLITE)], false, flagStreamLeader, m_flagReprocessing);
    }

    /*
     * 3AA
     */
    previousPipeId = pipeId;
    pipeId = PIPE_3AA;
    pipeId3aaSrc = PIPE_VC0;

#ifdef USE_PAF_FOR_PREVIEW
    /* PAF : PAF & 3AA is always connected in OTF*/
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        flagStreamLeader = true;
    }

    nodeType = getNodeType(PIPE_PAF);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_PAF;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodePaf;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "PAF", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_VC0)], m_flagFlite3aaOTF, flagStreamLeader, m_flagReprocessing);
    flagStreamLeader = false;
    pipeId3aaSrc = PIPE_PAF;
    previousPipeId = pipeId;
#endif

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_3AA;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3aa;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(pipeId3aaSrc)], m_flagFlite3aaOTF, flagStreamLeader, m_flagReprocessing);

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ac;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], true, flagStreamLeader, m_flagReprocessing);

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ap;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], true, flagStreamLeader, m_flagReprocessing);


#ifdef SUPPORT_3AF
    nodeType = getNodeType(PIPE_3AF);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AF;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3af;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_FD", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], true, flagStreamLeader, m_flagReprocessing);
#endif

#ifdef USE_VRA_FD
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) {
        /*
         * VRA
         */
        previousPipeId = pipeId;
        vraSrcPipeId = PIPE_3AF;

        nodeType = getNodeType(PIPE_VRA);
        m_deviceInfo[PIPE_VRA].pipeId[nodeType]  = PIPE_VRA;
        m_deviceInfo[PIPE_VRA].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[PIPE_VRA].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[PIPE_VRA].nodeName[nodeType], "VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        /* Workaround for Driver limitation : Due to driver limitation, VRA should be in different stream to connect the VRA to 3AA */
        m_sensorIds[PIPE_VRA][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flag3aaVraOTF, true, true);
    }
#endif

    /*
     * ISP
     */
    previousPipeId = pipeId;

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP;
    }

    /* ISPS */
    nodeType = getNodeType(PIPE_ISP);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_ISP;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIsp;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_3AP)], m_flag3aaIspOTF, flagStreamLeader, m_flagReprocessing);

    /*
     * MCSC
     */
    previousPipeId = pipeId;
    mcscSrcPipeId = PIPE_ISP;
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPC;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspc;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "ISP_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP)], true, flagStreamLeader, m_flagReprocessing);

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPP;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspp;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "ISP_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP)], true, flagStreamLeader, m_flagReprocessing);

        pipeId = PIPE_MCSC;
        mcscSrcPipeId = PIPE_ISPC;
    }

    /* MCSC */
    nodeType = getNodeType(PIPE_MCSC);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcsc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(mcscSrcPipeId)], m_flagIspMcscOTF, flagStreamLeader, m_flagReprocessing);

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    m_deviceInfo[pipeId].virtualNodeNum[nodeType] = virtualNodeMCSC[MCSC_PORT_0];
#else
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscp0;
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC1 */
    nodeType = getNodeType(PIPE_MCSC1);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    m_deviceInfo[pipeId].virtualNodeNum[nodeType] = virtualNodeMCSC[MCSC_PORT_1];
#else
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscp1;
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_PREVIEW_CALLBACK", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC2 */
    nodeType = getNodeType(PIPE_MCSC2);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    m_deviceInfo[pipeId].virtualNodeNum[nodeType] = virtualNodeMCSC[MCSC_PORT_2];
#else
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscp2;
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_RECORDING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 3:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC1)] = nodeMcscp1;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC2)] = nodeMcscp2;
        /* Not break; */
    case 1:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC0)] = nodeMcscp0;
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if (m_parameters->isUseVideoHQISP() == true) {
        /* MCSC3 */
        nodeType = getNodeType(PIPE_MCSC_JPEG);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_JPEG;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscp3;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_CAPTURE_JPEG", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);

        /* MCSC4 */
        nodeType = getNodeType(PIPE_MCSC_THUMB);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_THUMB;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscp4;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_CAPTURE_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);
    }

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
    pipeId = PIPE_SW_MCSC;

    nodeType = getNodeType(PIPE_SW_MCSC);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_SW_MCSC;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "SW_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC0);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC1);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC2);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
#endif

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        /* MCSC5 */
        nodeType = getNodeType(PIPE_MCSC5);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC5;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscpDs;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_DS", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC)], true, flagStreamLeader, m_flagReprocessing);

        /*
         * VRA
         */
        previousPipeId = pipeId;
        vraSrcPipeId = PIPE_MCSC5;
        pipeId = PIPE_VRA;

        nodeType = getNodeType(PIPE_VRA);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VRA;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flagMcscVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    /*
     * CLAHE
     */
    previousPipeId = PIPE_ISP;
    pipeId = PIPE_CLAHE;

    nodeType = getNodeType(PIPE_CLAHE);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_CLAHE;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeClh;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "CLAHE_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_MCSC0)], m_flagMcscClaheOTF, flagStreamLeader, m_flagReprocessing);

    nodeType = getNodeType(PIPE_CLAHEC);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_CLAHEC;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeClhc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "CLAHE_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_CLAHE)], true, flagStreamLeader, m_flagReprocessing);
#endif

#ifdef SUPPORT_ME
    if (nodeMe > 0) {
        pipeId = m_parameters->getLeaderPipeOfMe();

        /* ME */
        nodeType = getNodeType(PIPE_ME);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ME;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMe;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "ME_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(pipeId)], true, flagStreamLeader, m_flagReprocessing);
    }
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_initPipes(uint32_t frameRate)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int pipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    enum NODE_TYPE leaderNodeType = OUTPUT_NODE;

    int maxSensorW = 0, maxSensorH = 0, hwSensorW = 0, hwSensorH = 0;
    int yuvWidth[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvHeight[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvFormat[ExynosCameraParameters::YUV_MAX] = {0};
    camera_pixel_size yuvPixelSize[ExynosCameraParameters::YUV_MAX] = {CAMERA_PIXEL_SIZE_8BIT};
    int hwPictureW = 0, hwPictureH = 0;
    int maxThumbnailW = 0, maxThumbnailH = 0;
    int dsWidth = MAX_VRA_INPUT_WIDTH;
    int dsHeight = MAX_VRA_INPUT_HEIGHT;
    int dsFormat = m_parameters->getHwVraInputFormat();
    int yuvBufferCnt[ExynosCameraParameters::YUV_MAX] = {0};
    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);
    int hwVdisformat = m_parameters->getHWVdisFormat();
#ifdef SUPPORT_ME
    int hwMeFormat = m_parameters->getMeFormat();
#endif
    int pictureFormat = m_parameters->getHwPictureFormat();
    camera_pixel_size picturePixelSize = m_parameters->getHwPicturePixelSize();
    int perFramePos = 0;
    int yuvIndex = -1;
    struct ExynosConfigInfo *config = m_configurations->getConfig();
    ExynosRect bnsSize;
    ExynosRect bcropSize;
    ExynosRect bdsSize;
    ExynosRect tempRect;
    int ispSrcW, ispSrcH;
#ifdef USE_DUAL_CAMERA
    int cameraScenario = m_configurations->getScenario();
    enum DUAL_PREVIEW_MODE dualPreviewMode = m_configurations->getDualPreviewMode();
#endif

    int supportedSensorExMode = m_parameters->getSupportedSensorExMode();
    int extendSensorMode = m_configurations->getModeValue(CONFIGURATION_EXTEND_SENSOR_MODE);

    /* default settting */
    m_sensorStandby = true;
    m_needSensorStreamOn = true;

    m_parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&maxSensorW, (uint32_t *)&maxSensorH);
    m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    m_parameters->getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);
    m_parameters->getSize(HW_INFO_MAX_THUMBNAIL_SIZE, (uint32_t *)&maxThumbnailW, (uint32_t *)&maxThumbnailH);

    m_parameters->getPreviewBayerCropSize(&bnsSize, &bcropSize, false);
    m_parameters->getPreviewBdsSize(&bdsSize, false);

    CLOGI("MaxSensorSize %dx%d sensorSize %dx%d bayerFormat %x",
             maxSensorW, maxSensorH, hwSensorW, hwSensorH, bayerFormat);
    CLOGI("BnsSize %dx%d BcropSize %dx%d BdsSize %dx%d",
            bnsSize.w, bnsSize.h, bcropSize.w, bcropSize.h, bdsSize.w, bdsSize.h);
    CLOGI("DS Size %dx%d Format %x Buffer count %d",
            dsWidth, dsHeight, dsFormat, config->current->bufInfo.num_vra_buffers);

    CLOGI("HwPictureSize(%dx%d)", hwPictureW, hwPictureH);
    CLOGI("MaxThumbnailSize(%dx%d)", maxThumbnailW, maxThumbnailH);

    for (int i = ExynosCameraParameters::YUV_0; i < ExynosCameraParameters::YUV_MAX; i++) {
#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
        int onePortId = m_configurations->getOnePortId();
        m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[i], (uint32_t *)&yuvHeight[i], onePortId);
        yuvFormat[i] =  m_configurations->getYuvFormat(onePortId);
        yuvPixelSize[i] = m_configurations->getYuvPixelSize(onePortId);
        yuvBufferCnt[i] = m_configurations->getYuvBufferCount(onePortId);
        CLOGV("onePortId: %d, yuvWidth[%d]: %d, yuvHeight[%d]: %d, yuvFormat[%d]: %d, yuvPixelSize[%d]: %d, yuvBufferCnt[%d]:%d",
                onePortId, i, yuvWidth[i], i, yuvHeight[i], i, yuvFormat[i], i, yuvPixelSize[i], i, yuvBufferCnt[i]);
#else
        m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[i], (uint32_t *)&yuvHeight[i], i);
        yuvFormat[i] = m_configurations->getYuvFormat(i);
        yuvPixelSize[i] = m_configurations->getYuvPixelSize(i);
        yuvBufferCnt[i] = m_configurations->getYuvBufferCount(i);
#endif

#ifdef USE_DUAL_CAMERA
#if 0
        // TODO: need to handle fusionSize
        if (m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION
            && m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
            if (m_parameters->isAlternativePreviewPortId(i) == true) {
                int previewPortId = m_parameters->getPreviewPortId();

                m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[i], (uint32_t *)&yuvHeight[i], previewPortId);
            } else if (m_parameters->isPreviewPortId(i) == true || m_parameters->isPreviewCbPortId(i) == true) {
                ExynosRect fusionSrcRect;
                ExynosRect fusionDstRect;

                m_parameters->getFusionSize(yuvWidth[i], yuvHeight[i],
                        &fusionSrcRect, &fusionDstRect,
                        DUAL_SOLUTION_MARGIN_VALUE_30);

                yuvWidth[i] = fusionSrcRect.w;
                yuvHeight[i] = fusionSrcRect.h;
            }
        }
#endif
#endif

        CLOGI("YUV[%d] Size %dx%d Format %x PixelSizeNum %d Buffer count %d",
                i, yuvWidth[i], yuvHeight[i], yuvFormat[i], yuvPixelSize[i], yuvBufferCnt[i]);
    }

    /*
     * FLITE
     */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    /* setParam for Frame rate : must after setInput on Flite */
    struct v4l2_streamparm streamParam;
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = frameRate;
    if (supportedSensorExMode & (1 << extendSensorMode)) {
        bool flagExtendedMode = true;
        if (extendSensorMode == EXTEND_SENSOR_MODE_SW_REMOSAIC) {
            int count = 0;
            int supportedRemosaicMode = m_parameters->getSupportedRemosaicModes(count);
            if (count < 2) {
                flagExtendedMode = false;
            }
        }

        if (flagExtendedMode) {
            streamParam.parm.capture.extendedmode = extendSensorMode;
            CLOGI("Set (extendedmode=%d), supportedSensorExMode(0x%x)", extendSensorMode, supportedSensorExMode);
        }
    }
    CLOGI("Set framerate (denominator=%d) (extendedmode=%d)", frameRate, streamParam.parm.capture.extendedmode);

    ret = setParam(&streamParam, pipeId);
    if (ret != NO_ERROR) {
        CLOGE("FLITE setParam(frameRate(%d), pipeId(%d)) fail", frameRate, pipeId);
        return INVALID_OPERATION;
    }

    ret = m_setSensorSize(pipeId, hwSensorW, hwSensorH);
    if (ret != NO_ERROR) {
        CLOGE("m_setSensorSize(pipeId(%d), hwSensorW(%d), hwSensorH(%d)) fail", pipeId, hwSensorW, hwSensorH);
        return ret;
    }

    /* FLITE */
    nodeType = getNodeType(PIPE_FLITE);
    bayerFormat = m_parameters->getBayerFormat(PIPE_FLITE);

    /* set v4l2 buffer size */
    tempRect.fullW = 32;
    tempRect.fullH = 64;
    tempRect.colorFormat = bayerFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

    /* Set output node default info */
    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_FLITE);

    /* BAYER */
    nodeType = getNodeType(PIPE_VC0);
    perFramePos = PERFRAME_BACK_VC0_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_VC0);

    /* set v4l2 buffer size */
    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    /* set v4l2 video node bytes per plane */
    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->isDepthMapSupported()) {
        /* Depth Map Configuration */
        int depthMapW = 0, depthMapH = 0;
        int depthMapFormat = DEPTH_MAP_FORMAT;

        ret = m_parameters->getDepthMapSize(&depthMapW, &depthMapH);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getDepthMapSize");
            return ret;
        }

        CLOGI("DepthMapSize %dx%d", depthMapW, depthMapH);

        tempRect.fullW = depthMapW;
        tempRect.fullH = depthMapH;
        tempRect.colorFormat = depthMapFormat;

        nodeType = getNodeType(PIPE_VC1);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = NUM_DEPTHMAP_BUFFERS;

        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

#ifdef SUPPORT_PD_IMAGE
    if (m_parameters->isPDImageSupported()) {
        /* Depth Map Configuration */
        int pdImageW = 0, pdImageH = 0;
        int bayerFormat = PD_IMAGE_FORMAT;

        ret = m_parameters->getPDImageSize(pdImageW, pdImageH);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getPDImageSize");
            return ret;
        }

        CLOGI("getPDImageSize %dx%d", pdImageW, pdImageH);

        tempRect.fullW = pdImageW;
        tempRect.fullH = pdImageH;
        tempRect.colorFormat = bayerFormat;

        nodeType = getNodeType(PIPE_VC1);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = NUM_PD_IMAGE_BUFFERS;

        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

    ////////////////////////////////////////////////
    // SENSOR GYRO
    if (m_parameters->isSensorGyroSupported() == true) {
        ret = m_parameters->getSize(HW_INFO_HW_SENSOR_GYRO_SIZE, (uint32_t *)&tempRect.fullW, (uint32_t *)&tempRect.fullH);
        if (ret != NO_ERROR) {
            CLOGE("getSize(HW_INFO_HW_SENSOR_GYRO_SIZE) fail");
            return ret;
        }

        tempRect.colorFormat = m_parameters->getSensorGyroFormat();

        CLOGI("Sensor Gyro size %dx%d", tempRect.fullW, tempRect.fullH);

        nodeType = getNodeType(PIPE_VC3);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }

    ////////////////////////////////////////////////

    /* setup pipe info to FLITE pipe */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("FLITE setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /*
     * 3AA
     */
    pipeId = PIPE_3AA;

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA);
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_OTF) {
        /* set v4l2 buffer size */
        tempRect.fullW = 32;
        tempRect.fullH = 64;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
    } else if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

#if 0
        switch (bayerFormat) {
        case V4L2_PIX_FMT_SBGGR16:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR12:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR10:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
            break;
        default:
            CLOGW("Invalid bayer format(%d)", bayerFormat);
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        }
#endif
        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
    }

    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

    /* Set output node default info */
    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_3AA);

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC);
    perFramePos = PERFRAME_BACK_3AC_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AC);

    /* set v4l2 buffer size */
    tempRect.fullW = bcropSize.w;
    tempRect.fullH = bcropSize.h;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

#if 0
    /* set v4l2 video node bytes per plane */
    switch (bayerFormat) {
    case V4L2_PIX_FMT_SBGGR16:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR12:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR10:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
        break;
    default:
        CLOGW("Invalid bayer format(%d)", bayerFormat);
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    }
#endif
    /* set v4l2 video node buffer count */
    switch(m_parameters->getReprocessingBayerMode()) {
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
        case REPROCESSING_BAYER_MODE_NONE:
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
            break;
        case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
            break;
        default:
            CLOGE("Invalid reprocessing mode(%d)", m_parameters->getReprocessingBayerMode());
    }

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP);
    perFramePos = PERFRAME_BACK_3AP_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AP);

    /* set v4l2 buffer size */
    tempRect.fullW = bdsSize.w;
    tempRect.fullH = bdsSize.h;
    tempRect.colorFormat = bayerFormat;

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    m_parameters->setSize(HW_INFO_SENSOR_MODE_CHANGE_BDS_SIZE, bdsSize.w, bdsSize.h);
#endif

    /* set v4l2 video node bytes per plane */
    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
    pipeInfo[nodeType].pixelCompInfo = m_parameters->getPixelCompInfo(PIPE_3AP);

#if 0
    switch (bayerFormat) {
    case V4L2_PIX_FMT_SBGGR16:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR12:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR10:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
        break;
    default:
        CLOGW("Invalid bayer format(%d)", bayerFormat);
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    }
#endif

    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat, m_parameters->isUseBayerCompression());

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
    CLOGV("Pipe(%d) nodeType (%d) : pixelSize (%d) pixelCompInfo(%d)", PIPE_3AP, nodeType, pipeInfo[nodeType].pixelSize,
        pipeInfo[nodeType].pixelCompInfo);

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_3AF
    /* 3AF */
    nodeType = getNodeType(PIPE_3AF);
    perFramePos = 0; //it is dummy info
    // TODO: need to remove perFramePos from the SET_CAPTURE_DEVICE_BASIC_INFO

    /* set v4l2 buffer size */
    tempRect.fullW = MAX_VRA_INPUT_WIDTH;
    tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
    tempRect.colorFormat = m_parameters->getHW3AFdFormat();

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

#ifdef SUPPORT_ME
    if(m_parameters->getLeaderPipeOfMe() == pipeId) {
        /* ME */
        nodeType = getNodeType(PIPE_ME);
        perFramePos = PERFRAME_BACK_ME_POS;

        /* set v4l2 buffer size */
        m_parameters->getMeSize(&tempRect.fullW, &tempRect.fullH);
        tempRect.colorFormat = hwMeFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = CAMERA_ME_MAX_BUFFER;

        /* set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

    /* setup pipe info to 3AA pipe */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("3AA setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    if (!m_useBDSOff) {
        ispSrcW = bdsSize.w;
        ispSrcH = bdsSize.h;
    } else {
        ispSrcW = bcropSize.w;
        ispSrcH = bcropSize.h;
    }

    /*
     * ISP
     */

    /* ISPS */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP;
        nodeType = getNodeType(PIPE_ISP);
        bayerFormat = m_parameters->getBayerFormat(PIPE_ISP);

        /* set v4l2 buffer size */
        tempRect.fullW = ispSrcW;
        tempRect.fullH = ispSrcH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
        pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat, m_parameters->isUseBayerCompression());
        pipeInfo[nodeType].pixelCompInfo = m_parameters->getPixelCompInfo(PIPE_ISP);

#if 0
        switch (bayerFormat) {
        case V4L2_PIX_FMT_SBGGR16:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR12:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR10:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
            break;
        default:
            CLOGW("Invalid bayer format(%d)", bayerFormat);
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        }
#endif
        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
        CLOGD("Pipe(%d) nodeType (%d) : pixelSize (%d) pixelCompInfo(%d)", PIPE_ISP, nodeType, pipeInfo[nodeType].pixelSize,
                pipeInfo[nodeType].pixelCompInfo);


        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_ISP);
    }

#ifdef SUPPORT_ME
    if(m_parameters->getLeaderPipeOfMe() == pipeId) {
        /* ME */
        nodeType = getNodeType(PIPE_ME);
        perFramePos = PERFRAME_BACK_ME_POS;

        /* set v4l2 buffer size */
        m_parameters->getMeSize(&tempRect.fullW, &tempRect.fullH);
        tempRect.colorFormat = hwMeFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = CAMERA_ME_MAX_BUFFER;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

    /* setup pipe info to ISP pipe */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC);
        perFramePos = PERFRAME_BACK_ISPC_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = ispSrcW;
        tempRect.fullH = ispSrcH;
        tempRect.colorFormat = hwVdisformat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 2, CAMERA_16PX_ALIGN);

        pipeInfo[nodeType].pixelSize = CAMERA_PIXEL_SIZE_PACKED_10BIT;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP);
        perFramePos = PERFRAME_BACK_ISPP_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = ispSrcW;
        tempRect.fullH = ispSrcH;
        tempRect.colorFormat = hwVdisformat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 2, CAMERA_TPU_CHUNK_ALIGN_W);

        pipeInfo[nodeType].pixelSize = CAMERA_PIXEL_SIZE_PACKED_10BIT;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("ISP setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /*
     * MCSC
     */

    /* MCSC */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_MCSC;
        nodeType = getNodeType(PIPE_MCSC);

        /* set v4l2 buffer size */
        tempRect.fullW = ispSrcW;
        tempRect.fullH = ispSrcH;
        tempRect.colorFormat = hwVdisformat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 2, CAMERA_16PX_ALIGN);

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_MCSC);
    }

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
    case 3:
        /* MCSC2 */
        nodeType = getNodeType(PIPE_MCSC2);
        perFramePos = PERFRAME_BACK_MCSC2_POS;
        yuvIndex = ExynosCameraParameters::YUV_2;
        m_parameters->setYuvOutPortId(PIPE_MCSC2, yuvIndex);

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

#ifdef USE_BUFFER_WITH_STRIDE
#ifdef USE_DUAL_CAMERA
        if (cameraScenario == SCENARIO_DUAL_REAR_ZOOM
                && dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION
                && (m_parameters->isPreviewPortId(yuvIndex) || m_parameters->isPreviewCbPortId(yuvIndex))) {
            pipeInfo[nodeType].bytesPerPlane[0] = 0;
        } else
#endif
        {
            pipeInfo[nodeType].bytesPerPlane[0] = yuvWidth[yuvIndex];
        }
#endif

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = yuvBufferCnt[yuvIndex];

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* MCSC1 */
        nodeType = getNodeType(PIPE_MCSC1);
        perFramePos = PERFRAME_BACK_MCSC1_POS;
        yuvIndex = ExynosCameraParameters::YUV_1;
        m_parameters->setYuvOutPortId(PIPE_MCSC1, yuvIndex);

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node bytes per plane */
#ifdef USE_BUFFER_WITH_STRIDE
        /* to use stride for preview buffer, set the bytesPerPlane */
#ifdef USE_DUAL_CAMERA
        if (cameraScenario == SCENARIO_DUAL_REAR_ZOOM
                && dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION
                && (m_parameters->isPreviewPortId(yuvIndex) || m_parameters->isPreviewCbPortId(yuvIndex))) {
            pipeInfo[nodeType].bytesPerPlane[0] = 0;
        } else
#endif
        {
            pipeInfo[nodeType].bytesPerPlane[0] = yuvWidth[yuvIndex];
        }
#endif

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = yuvBufferCnt[yuvIndex];

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
        /* Not break; */

    case 1:
        /* MCSC0 */
        nodeType = getNodeType(PIPE_MCSC0);
        perFramePos = PERFRAME_BACK_SCP_POS;
        yuvIndex = ExynosCameraParameters::YUV_0;
        m_parameters->setYuvOutPortId(PIPE_MCSC0, yuvIndex);

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node bytes per plane */
#ifdef USE_BUFFER_WITH_STRIDE
        /* to use stride for preview buffer, set the bytesPerPlane */
#ifdef USE_DUAL_CAMERA
        if (cameraScenario == SCENARIO_DUAL_REAR_ZOOM
                && dualPreviewMode == DUAL_PREVIEW_MODE_SW_FUSION
                && (m_parameters->isPreviewPortId(yuvIndex) || m_parameters->isPreviewCbPortId(yuvIndex))) {
            pipeInfo[nodeType].bytesPerPlane[0] = 0;
        } else
#endif
        {
            pipeInfo[nodeType].bytesPerPlane[0] = yuvWidth[yuvIndex];
        }
#endif

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = yuvBufferCnt[yuvIndex];

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if (m_parameters->isUseVideoHQISP() == true) {
        /* Jpeg Thumbnail */
        nodeType = getNodeType(PIPE_MCSC_THUMB);
        perFramePos = PERFRAME_BACK_MCSC_THUMB_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* Jpeg Main */
        nodeType = getNodeType(PIPE_MCSC_JPEG);
        perFramePos = PERFRAME_BACK_MCSC_JPEG_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }

#ifdef SUPPORT_VIRTUALFD_PREVIEW
        ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC0), getNodeType(PIPE_MCSC1));
        ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC0), getNodeType(PIPE_MCSC2));

        frame_queue_t *frameQ = NULL;
        ExynosCameraNode **nodes = NULL;
        sp<ReorderMCSCHandlerPreview> handler = new ReorderMCSCHandlerPreview(getCameraId(), pipeId, m_configurations, m_parameters, PIPE_HANDLER::SCENARIO_REORDER);

        handler->setName("MCSC Reorder Handler Preview");
        handler->addUsage(PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ);
        handler->addUsage(PIPE_HANDLER::USAGE_PRE_QBUF);
        handler->addUsage(PIPE_HANDLER::USAGE_POST_DQBUF);

        m_pipes[INDEX(pipeId)]->getInputFrameQ(&frameQ);
        handler->setInputFrameQ(frameQ);
        m_pipes[INDEX(pipeId)]->getOutputFrameQ(&frameQ);
        handler->setOutputFrameQ(frameQ);
        m_pipes[INDEX(pipeId)]->getFrameDoneQ(&frameQ);
        handler->setFrameDoneQ(frameQ);
        handler->setPipeInfo(m_deviceInfo[INDEX(pipeId)].pipeId, m_deviceInfo[INDEX(pipeId)].virtualNodeNum);
        handler->setPerframeIndex(pipeInfo[OUTPUT_NODE].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex);
        handler->setBufferSupplier(m_bufferSupplier);

        m_pipes[INDEX(pipeId)]->getNodes(&nodes);
        handler->setNodes(&nodes);

        m_pipes[INDEX(pipeId)]->setHandler(handler);
#endif

#ifdef USE_MCSC_DS
    /* MCSC5 */
    nodeType = getNodeType(PIPE_MCSC5);
    perFramePos = PERFRAME_BACK_MCSC5_POS;

    /* set v4l2 buffer size */
    tempRect.fullW = dsWidth;
    tempRect.fullH = dsHeight;
    tempRect.colorFormat = dsFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("MCSC setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /* VRA */
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_VRA;
        nodeType = getNodeType(PIPE_VRA);

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = dsFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);
    }
#endif

    ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
    if (ret != NO_ERROR) {
        CLOGE("MCSC setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* clear pipeInfo for next setupPipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

#ifdef USE_VRA_FD
    /* VRA */
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
        (m_flagMcscVraOTF != HW_CONNECTION_MODE_M2M)) {
        pipeId = PIPE_VRA;
        nodeType = getNodeType(PIPE_VRA);

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = dsFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("VRA setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    /*
     * CLAHE
     */

    /* get recording port Id or set default as 0 */
    yuvIndex = m_parameters->getRecordingPortId();
    if (yuvIndex < 0) {
        yuvIndex = 0;
    }

    /* CLAHE */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_CLAHE;
        nodeType = getNodeType(PIPE_CLAHE);

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = yuvBufferCnt[yuvIndex];

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_CLAHE);
    }

    /* setup pipe info to CLAHE pipe */
    /* CLAHEC */
    nodeType = getNodeType(PIPE_CLAHEC);

    /* set v4l2 buffer size */
    tempRect.fullW = yuvWidth[yuvIndex];
    tempRect.fullH = yuvHeight[yuvIndex];
    tempRect.colorFormat = yuvFormat[yuvIndex];

    /* set v4l2 format pixel size */
    pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = yuvBufferCnt[yuvIndex];

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    int recordingPipeId = (yuvIndex < 0) ? PIPE_MCSC0 : PIPE_MCSC0 + yuvIndex;
    m_sensorIds[pipeId][getNodeType(PIPE_CLAHE)] = m_getSensorId(
                                                    m_deviceInfo[PIPE_ISP].nodeNum[getNodeType(recordingPipeId)],
                                                    m_flagMcscClaheOTF, false/*flagStreamLeader*/, m_flagReprocessing);

    ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
    if (ret != NO_ERROR) {
        CLOGE("CLAHE setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* clear pipeInfo for next setupPipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

#endif

#if defined(USE_SLSI_PLUGIN)
    for (int i = PIPE_PLUGIN_BASE; i <= PIPE_PLUGIN_MAX; i++) {
        if (m_pipes[i] == NULL) continue;

        Map_t map;
        ret = m_pipes[i]->setupPipe(&map);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_PLUGIN%d setupPipe fail, ret(%d)", i, ret);
            return INVALID_OPERATION;
        }
    }
#endif

#if defined(USE_DUAL_CAMERA) && defined(USE_SLSI_PLUGIN)
    if (m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true &&
        m_configurations->getDualPreviewMode() == DUAL_PREVIEW_MODE_SW_FUSION) {
        if (m_pipes[PIPE_FUSION] != NULL) {
            Map_t map;

            /* Fusion */
            ret = m_pipes[PIPE_FUSION]->setupPipe(&map);
            if(ret != NO_ERROR) {
                CLOGE("Fusion setupPipe fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

#ifdef USES_SW_VDIS
    if (m_configurations->getModeValue(CONFIGURATION_VIDEO_STABILIZATION_ENABLE) > 0) {
        Map_t map;

        /* vdis */
        if (m_pipes[INDEX(PIPE_VDIS)] != NULL) {
            ret = m_pipes[INDEX(PIPE_VDIS)]->setupPipe(&map);
            if (ret != NO_ERROR) {
                CLOGE("vdis setupPipe fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
     /* nfd */
     if (m_pipes[INDEX(PIPE_NFD)] != NULL) {
         Map_t map;
         ret = m_pipes[INDEX(PIPE_NFD)]->setupPipe(&map);
         if (ret != NO_ERROR) {
             CLOGE("nfd setupPipe fail, ret(%d)", ret);
             return INVALID_OPERATION;
         }
     }
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_initPipesFastenAeStable(int32_t numFrames,
                                                                    int hwSensorW,
                                                                    int hwSensorH,
                                                                    uint32_t frameRate)
{
    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int pipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    enum NODE_TYPE leaderNodeType = OUTPUT_NODE;

    ExynosRect tempRect;
    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);
    int hwVdisformat = m_parameters->getHWVdisFormat();
    int hwPreviewFormat = (m_parameters->getHwPreviewFormat() == 0) ? V4L2_PIX_FMT_NV21M : m_parameters->getHwPreviewFormat();
    int vraWidth = MAX_VRA_INPUT_WIDTH, vraHeight = MAX_VRA_INPUT_HEIGHT;
    int vraFormat = m_parameters->getHwVraInputFormat();
    int perFramePos = 0;

    /*
     * FLITE
     */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }

    /* setParam for Frame rate : must after setInput on Flite */
    struct v4l2_streamparm streamParam;
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = frameRate;
    CLOGI("Set framerate (denominator=%d)", frameRate);

    ret = setParam(&streamParam, pipeId);
    if (ret != NO_ERROR) {
        CLOGE("FLITE setParam(frameRate(%d), pipeId(%d)) fail", frameRate, pipeId);
        return INVALID_OPERATION;
    }

    ret = m_setSensorSize(pipeId, hwSensorW, hwSensorH);
    if (ret != NO_ERROR) {
        CLOGE("m_setSensorSize(pipeId(%d), hwSensorW(%d), hwSensorH(%d)) fail", pipeId, hwSensorW, hwSensorH);
        return ret;
    }

    /* FLITE */
    nodeType = getNodeType(PIPE_FLITE);

    /* set v4l2 buffer size */
    tempRect.fullW = 32;
    tempRect.fullH = 64;
    tempRect.colorFormat = bayerFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = numFrames;

    /* Set output node default info */
    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_FLITE);

    /* BAYER */
    nodeType = getNodeType(PIPE_VC0);
    perFramePos = PERFRAME_BACK_VC0_POS;

    /* set v4l2 buffer size */
    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    /* packed bayer bytesPerPlane */
    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = numFrames;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_DEPTH_MAP
  if (m_parameters->isDepthMapSupported()) {
        /* Depth Map Configuration */
        int depthMapW = 0, depthMapH = 0;
        int depthMapFormat = DEPTH_MAP_FORMAT;

        ret = m_parameters->getDepthMapSize(&depthMapW, &depthMapH);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getDepthMapSize");
            return ret;
        }

        CLOGI("DepthMapSize %dx%d", depthMapW, depthMapH);

        tempRect.fullW = depthMapW;
        tempRect.fullH = depthMapH;
        tempRect.colorFormat = depthMapFormat;

        nodeType = getNodeType(PIPE_VC1);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = NUM_DEPTHMAP_BUFFERS;

        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

#ifdef SUPPORT_PD_IMAGE
        if (m_parameters->isPDImageSupported()) {
            /* Depth Map Configuration */
            int pdImageW = 0, pdImageH = 0;
            int bayerFormat = PD_IMAGE_FORMAT;

            ret = m_parameters->getPDImageSize(pdImageW, pdImageH);
            if (ret != NO_ERROR) {
                CLOGE("Failed to getPDImageSize");
                return ret;
            }

            CLOGI("getPDImageSize %dx%d", pdImageW, pdImageH);

            tempRect.fullW = pdImageW;
            tempRect.fullH = pdImageH;
            tempRect.colorFormat = bayerFormat;

            nodeType = getNodeType(PIPE_VC1);
            pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
            pipeInfo[nodeType].bufInfo.count = NUM_PD_IMAGE_BUFFERS;

            SET_CAPTURE_DEVICE_BASIC_INFO();
        }
#endif

    ////////////////////////////////////////////////
    // SENSOR GYRO
    if (m_parameters->isSensorGyroSupported() == true) {
        ret = m_parameters->getSize(HW_INFO_HW_SENSOR_GYRO_SIZE, (uint32_t *)&tempRect.fullW, (uint32_t *)&tempRect.fullH);
        if (ret != NO_ERROR) {
            CLOGE("getSize(HW_INFO_HW_SENSOR_GYRO_SIZE) fail");
            return ret;
        }

        tempRect.colorFormat = m_parameters->getSensorGyroFormat();

        CLOGI("Sensor Gyro size %dx%d", tempRect.fullW, tempRect.fullH);

        nodeType = getNodeType(PIPE_VC3);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = numFrames;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }

    ////////////////////////////////////////////////

    /* setup pipe info to FLITE pipe */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("FLITE setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    pipeId = PIPE_3AA;

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA);
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);

    /* set v4l2 buffer size */
    tempRect.fullW = 32;
    tempRect.fullH = 64;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_3AA);

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC);
    perFramePos = PERFRAME_BACK_3AC_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AC);

    /* set v4l2 buffer size */
    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP);
    perFramePos = PERFRAME_BACK_3AP_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AP);

    tempRect.colorFormat = bayerFormat;
    pipeInfo[nodeType].bufInfo.count = numFrames;
    pipeInfo[nodeType].pixelCompInfo = m_parameters->getPixelCompInfo(PIPE_3AP);

    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_3AF
    /* 3AF */
    nodeType = getNodeType(PIPE_3AF);
    perFramePos = 0; //it is dummy info
    // TODO: need to remove perFramePos from the SET_CAPTURE_DEVICE_BASIC_INFO

    /* set v4l2 buffer size : setup the 3AP size*/
    tempRect.colorFormat = m_parameters->getHW3AFdFormat();

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = numFrames;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* setup pipe info to 3AA pipe */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("3AA setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }
#endif

    /* ISPS */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP;
        nodeType = getNodeType(PIPE_ISP);
        bayerFormat = m_parameters->getBayerFormat(PIPE_ISP);

        tempRect.colorFormat = bayerFormat;
        pipeInfo[nodeType].bufInfo.count = numFrames;
        pipeInfo[nodeType].pixelCompInfo = m_parameters->getPixelCompInfo(PIPE_ISP);

        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_ISP);
    }

    /* setup pipe info to ISP pipe */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC);
        perFramePos = PERFRAME_BACK_ISPC_POS;

        tempRect.colorFormat = hwVdisformat;

        pipeInfo[nodeType].bufInfo.count = numFrames;

        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP);
        perFramePos = PERFRAME_BACK_ISPP_POS;

        pipeInfo[nodeType].bufInfo.count = numFrames;

        SET_CAPTURE_DEVICE_BASIC_INFO();

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("ISP setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /* MCSC */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_MCSC;
        nodeType = getNodeType(PIPE_MCSC);

        pipeInfo[nodeType].bufInfo.count = numFrames;

        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_MCSC);
    }

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0);
    perFramePos = PERFRAME_BACK_SCP_POS;

    tempRect.colorFormat = hwPreviewFormat;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* MCSC1 */
    nodeType = getNodeType(PIPE_MCSC1);
    perFramePos = PERFRAME_BACK_MCSC1_POS;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* MCSC2 */
    nodeType = getNodeType(PIPE_MCSC2);
    perFramePos = PERFRAME_BACK_MCSC2_POS;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef USE_MCSC_DS
    /* MCSC5 */
    nodeType = getNodeType(PIPE_MCSC5);
    perFramePos = PERFRAME_BACK_MCSC5_POS;

    tempRect.fullW = vraWidth;
    tempRect.fullH = vraHeight;
    tempRect.colorFormat = vraFormat;

    pipeInfo[nodeType].bufInfo.count = numFrames;

    SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

    ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
    if (ret != NO_ERROR) {
        CLOGE("MCSC setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* clear pipeInfo for next setupPipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

#ifdef USE_VRA_FD
    /* VRA */
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_VRA;
        nodeType = getNodeType(PIPE_VRA);

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = m_parameters->getHwVraInputFormat();

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = numFrames;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("MCSC setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_VRA_FD
    /* VRA */
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
        (m_flagMcscVraOTF != HW_CONNECTION_MODE_M2M)) {
        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;

        pipeId = PIPE_VRA;
        nodeType = getNodeType(PIPE_VRA);

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = m_parameters->getHwVraInputFormat();

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = numFrames;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("VRA setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

    /* set BNS ratio */
    pipeId = PIPE_FLITE;
    if (m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) != HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_3AA;
    }

    int bnsScaleRatio = 1000;
    ret = m_pipes[pipeId]->setControl(V4L2_CID_IS_S_BNS, bnsScaleRatio);
    if (ret != NO_ERROR) {
        CLOGE("Set BNS(%.1f) fail, ret(%d)", (float)(bnsScaleRatio / 1000), ret);
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_fillNodeGroupInfo(ExynosCameraFrameSP_sptr_t frame)
{
    camera2_node_group node_group_info_flite;
    camera2_node_group node_group_info_3aa;
    camera2_node_group node_group_info_isp;
    camera2_node_group node_group_info_mcsc;
    camera2_node_group node_group_info_vra;
    camera2_node_group node_group_info_clahe;
    camera2_node_group *node_group_info_temp = NULL;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    camera2_virtual_node_group virtual_node_group_info;
    camera2_virtual_node_group *virtual_node_group_temp = NULL;
#endif

    status_t ret = NO_ERROR;
    int pipeId = -1;
    int nodePipeId = -1;
    uint32_t perframePosition = 0;

    int yuvFormat[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvIndex = -1;

    for (int i = ExynosCameraParameters::YUV_0; i < ExynosCameraParameters::YUV_MAX; i++) {
        yuvFormat[i] = m_configurations->getYuvFormat(i);
    }

    memset(&node_group_info_flite, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_isp, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_mcsc, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_vra, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_clahe, 0x0, sizeof(camera2_node_group));
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    memset(&virtual_node_group_info, 0x0, sizeof(camera2_virtual_node_group));
#endif

    if (m_flagFlite3aaOTF != HW_CONNECTION_MODE_M2M) {
        /* 3AA */
        pipeId = PIPE_3AA;
        perframePosition = 0;
        node_group_info_temp = &node_group_info_flite;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);

        nodePipeId = PIPE_VC0;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

#ifdef SUPPORT_DEPTH_MAP
        /* VC1 for depth */
        if (m_configurations->getMode(CONFIGURATION_DEPTH_MAP_MODE) == true) {
            nodePipeId = PIPE_VC1;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, DEPTH_MAP_FORMAT);
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif // SUPPORT_DEPTH_MAP

#ifdef SUPPORT_PD_IMAGE
        /* VC1 for PD_IMAGE */
        if (m_parameters->isPDImageSupported()) {
            nodePipeId = PIPE_VC1;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, PD_IMAGE_FORMAT);
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif // SUPPORT_PD_IMAGE

        ////////////////////////////////////////////////
        // SENSOR GYRO
        if (m_configurations->getMode(CONFIGURATION_SENSOR_GYRO_MODE) == true) {
            nodePipeId = PIPE_VC3;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getSensorGyroFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        ////////////////////////////////////////////////

        nodePipeId = PIPE_3AC;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_3AP;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    } else {
        /* FLITE */
        pipeId = PIPE_FLITE;
        perframePosition = 0;

        node_group_info_temp = &node_group_info_flite;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);

        nodePipeId = PIPE_VC0;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;


#ifdef SUPPORT_DEPTH_MAP
        /* VC1 for depth */
        if (m_configurations->getMode(CONFIGURATION_DEPTH_MAP_MODE) == true) {
            nodePipeId = PIPE_VC1;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif // SUPPORT_DEPTH_MAP

#ifdef SUPPORT_PD_IMAGE
        /* VC1 for PD_IMAGE */
        if (m_parameters->isPDImageSupported()) {
            nodePipeId = PIPE_VC1;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, PD_IMAGE_FORMAT);
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif // SUPPORT_PD_IMAGE

        ////////////////////////////////////////////////
        // SENSOR GYRO
        if (m_configurations->getMode(CONFIGURATION_SENSOR_GYRO_MODE) == true) {
            nodePipeId = PIPE_VC3;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getSensorGyroFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        ////////////////////////////////////////////////

        /* 3AA */
        pipeId = PIPE_3AA;
        perframePosition = 0;

        node_group_info_temp = &node_group_info_3aa;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);

        nodePipeId = PIPE_3AC;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_3AP;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }

#ifdef SUPPORT_3AF
    nodePipeId = PIPE_3AF;
    ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                pipeId, nodePipeId, perframePosition, m_parameters->getHW3AFdFormat());
    if (ret != NO_ERROR) {
        CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
        return ret;
    }
    perframePosition++;
#endif

#ifdef SUPPORT_ME
    if (m_parameters->getLeaderPipeOfMe() == pipeId) {
        nodePipeId = PIPE_ME;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                pipeId, nodePipeId, perframePosition, m_parameters->getMeFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#endif

    /* ISP */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP;
        perframePosition = 0;
        node_group_info_temp = &node_group_info_isp;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);
    }

    /* MCSC */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        nodePipeId = PIPE_ISPC;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHWVdisFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_ISPP;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHWVdisFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        pipeId = PIPE_MCSC;
        perframePosition = 0;
        node_group_info_temp = &node_group_info_mcsc;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getHWVdisFormat();
    }

    nodePipeId = PIPE_MCSC0;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                pipeId, nodePipeId, perframePosition,
                                yuvFormat[ExynosCameraParameters::YUV_0]);
#else
    ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
            pipeId, nodePipeId, perframePosition, yuvFormat[ExynosCameraParameters::YUV_0]);
#endif
    if (ret != NO_ERROR) {
        CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
        return ret;
    }
    perframePosition++;

    if(m_parameters->getNumOfMcscOutputPorts() > 1){
        nodePipeId = PIPE_MCSC1;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                pipeId, nodePipeId, perframePosition,
                yuvFormat[ExynosCameraParameters::YUV_1]);
#else
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                pipeId, nodePipeId, perframePosition, yuvFormat[ExynosCameraParameters::YUV_1]);
#endif
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_MCSC2;
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                pipeId, nodePipeId, perframePosition,
                yuvFormat[ExynosCameraParameters::YUV_2]);
#else
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                pipeId, nodePipeId, perframePosition, yuvFormat[ExynosCameraParameters::YUV_2]);
#endif
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#ifdef SUPPORT_VIRTUALFD_PREVIEW
    else {
        nodePipeId = PIPE_MCSC1;
        ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                pipeId, nodePipeId, perframePosition,
                yuvFormat[ExynosCameraParameters::YUV_1]);
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_MCSC2;
        ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                pipeId, nodePipeId, perframePosition,
                yuvFormat[ExynosCameraParameters::YUV_2]);
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#endif

    if (m_parameters->isUseVideoHQISP() == true) {
        nodePipeId = PIPE_MCSC_JPEG;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        nodePipeId = PIPE_MCSC_THUMB;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }


#ifdef USE_MCSC_DS
    /* VRA */
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        nodePipeId = PIPE_MCSC5;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHwVraInputFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    /* CLAHE */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = INDEX(PIPE_CLAHE);
        perframePosition = 0;
        node_group_info_temp = &node_group_info_clahe;
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getHwPictureFormat();

        nodePipeId = PIPE_CLAHEC;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#endif

#ifdef SUPPORT_ME
    if (m_parameters->getLeaderPipeOfMe() == pipeId) {
        nodePipeId = PIPE_ME;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                pipeId, nodePipeId, perframePosition, m_parameters->getMeFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
    }
#endif

#ifdef USE_VRA_FD
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) || (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
        /* 3AA to VRA is a sideway path*/
        node_group_info_vra.leader.request = 1;
        node_group_info_vra.leader.pixelformat = m_parameters->getHwVraInputFormat();
    }
#endif

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_FLITE,
                frame,
                &node_group_info_flite);
        frame->storeNodeGroupInfo(&node_group_info_flite, PERFRAME_INFO_FLITE);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_FLITE);
#endif

        updateNodeGroupInfo(
                PIPE_3AA,
                frame,
                &node_group_info_3aa);
        frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_3AA);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_3AA);
#endif
    } else {
        updateNodeGroupInfo(
                PIPE_3AA,
                frame,
                &node_group_info_flite);
        frame->storeNodeGroupInfo(&node_group_info_flite, PERFRAME_INFO_FLITE);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_FLITE);
#endif
    }

    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_ISP,
                frame,
                &node_group_info_isp);
        frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_ISP);
#endif
    }

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_MCSC,
                frame,
                &node_group_info_mcsc);
        frame->storeNodeGroupInfo(&node_group_info_mcsc, PERFRAME_INFO_MCSC);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_MCSC);
#endif
    }

#if defined (USE_VRA_FD)
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) || (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
        updateNodeGroupInfo(
                PIPE_VRA,
                frame,
                &node_group_info_vra);
        frame->storeNodeGroupInfo(&node_group_info_vra, PERFRAME_INFO_VRA);
#ifdef SUPPORT_VIRTUALFD_PREVIEW
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_VRA);
#endif
    }
#endif

#ifdef USE_CLAHE_PREVIEW
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_CLAHE,
                frame,
                &node_group_info_clahe);
        frame->storeNodeGroupInfo(&node_group_info_clahe, PERFRAME_INFO_CLAHE);
    }
#endif

#if defined(USE_SW_MCSC) && (USE_SW_MCSC == true)
            nodePipeId = PIPE_SW_MCSC;
            if (m_request[nodePipeId]) {
                ExynosRect inputSize, outputSize, ratioCropSize;
                camera2_node_group node_group_info_sw_mcsc;
                memset(&node_group_info_sw_mcsc, 0x0, sizeof(camera2_node_group));

                perframePosition = 0;
                int count = 3;
                int swMCSCyuvFormat[FIMC_IS_VIDEO_M5P_NUM] = {0};
                int yuvWidth[ExynosCameraParameters::YUV_MAX] = {0};
                int yuvHeight[ExynosCameraParameters::YUV_MAX] = {0};
                int onePortId = m_configurations->getOnePortId();
                int secondPortId = m_configurations->getSecondPortId();

                for (int i = 0; i < count; i++ ) {
                    swMCSCyuvFormat[i] = yuvFormat[i];
                    m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[i], (uint32_t *)&yuvHeight[i], i);
                }

                node_group_info_sw_mcsc.leader.pixelformat = yuvFormat[(onePortId % ExynosCameraParameters::YUV_MAX) + ExynosCameraParameters::YUV_0];

                node_group_info_sw_mcsc.capture[secondPortId].request = m_request[INDEX(nodePipeId)];
                CLOGV("node_group_info_sw_mcsc.capture[%d].request = %d", secondPortId, node_group_info_sw_mcsc.capture[secondPortId].request);
                node_group_info_sw_mcsc.capture[secondPortId].vid = (FIMC_IS_VIDEO_M0P_NUM + secondPortId) - FIMC_IS_VIDEO_BAS_NUM;
                node_group_info_sw_mcsc.capture[secondPortId].pixelformat = swMCSCyuvFormat[secondPortId];

                inputSize.x = 0;
                inputSize.y = 0;
                inputSize.w = yuvWidth[onePortId];
                inputSize.h = yuvHeight[onePortId];
                outputSize.x = 0;
                outputSize.y = 0;
                outputSize.w = yuvWidth[secondPortId];
                outputSize.h = yuvHeight[secondPortId];

                ret = getCropRectAlign(inputSize.w, inputSize.h, outputSize.w, outputSize.h,
                                        &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
                                        CAMERA_MCSC_ALIGN, 2, 1.0f);
                if (ret != NO_ERROR) {
                    CLOGE2("getCropRectAlign failed. MCSC in_crop %dx%d, MCSC%d out_size %dx%d",
                            inputSize.w, inputSize.h, secondPortId, outputSize.w, outputSize.h);
                    ratioCropSize.x = 0;
                    ratioCropSize.y = 0;
                    ratioCropSize.w = inputSize.w;
                    ratioCropSize.h = inputSize.h;
                }

                node_group_info_sw_mcsc.capture[secondPortId].input.cropRegion[0] = ratioCropSize.x;
                node_group_info_sw_mcsc.capture[secondPortId].input.cropRegion[1] = ratioCropSize.y;
                node_group_info_sw_mcsc.capture[secondPortId].input.cropRegion[2] = ratioCropSize.w;
                node_group_info_sw_mcsc.capture[secondPortId].input.cropRegion[3] = ratioCropSize.h;
                node_group_info_sw_mcsc.capture[secondPortId].output.cropRegion[0] = outputSize.x;
                node_group_info_sw_mcsc.capture[secondPortId].output.cropRegion[1] = outputSize.y;
                node_group_info_sw_mcsc.capture[secondPortId].output.cropRegion[2] = outputSize.w;
                node_group_info_sw_mcsc.capture[secondPortId].output.cropRegion[3] = outputSize.h;
                CLOGV("PIPE_SW_MCSC intput cropRegion (%d,%d,%d,%d) output cropRegion (%d,%d,%d,%d)",
                        ratioCropSize.x, ratioCropSize.y, ratioCropSize.w, ratioCropSize.h,
                        outputSize.x, outputSize.y, outputSize.w, outputSize.h);

                updateNodeGroupInfo(
                        PIPE_SW_MCSC,
                        frame,
                        &node_group_info_sw_mcsc);
                frame->storeNodeGroupInfo(&node_group_info_sw_mcsc, PERFRAME_INFO_SWMCSC);
            }
#endif

    return NO_ERROR;
}

bool ExynosCameraFrameFactoryPreview::m_flagPipeXNeedToRun(int pipeId)
{
    bool ret = false;

    switch (pipeId) {
    case PIPE_FLITE:
        if ((m_request[PIPE_VC0] == true || m_request[PIPE_VC3] == true) &&
            (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M)) {
            ret = true;
        }
        break;
    default:
        CLOGE("Invalid pipeId(%d). so fail", pipeId);
        break;
    }

    return ret;
}

void ExynosCameraFrameFactoryPreview::m_init(void)
{
    m_flagReprocessing = false;

    m_shot_ext = new struct camera2_shot_ext;
}

#ifdef SUPPORT_SENSOR_MODE_CHANGE
#define STOP_PIPE_THREAD(pipe) \
    if (m_pipes[pipe]->isThreadRunning() == true) { \
        if (stopThread(pipe) != NO_ERROR) { \
            return INVALID_OPERATION; \
        } \
    }

#define STOP_PIPE(pipe) \
    if (m_pipes[pipe]->stop() != NO_ERROR) { \
        CLOGE("Failed to stop (%d) pipe", pipe); \
        return INVALID_OPERATION; \
    }

status_t ExynosCameraFrameFactoryPreview::initSensorPipe(void) {
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int pipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    enum NODE_TYPE leaderNodeType = OUTPUT_NODE;

    int hwSensorW = 0, hwSensorH = 0;

#ifdef SUPPORT_ME
    int hwMeFormat = m_parameters->getMeFormat();
#endif
    int perFramePos = 0;
    struct ExynosConfigInfo *config = m_configurations->getConfig();

    if (config == NULL) {
        CLOGE("config is NULL!!");
        return INVALID_OPERATION;
    }

    ExynosRect bnsSize;
    ExynosRect bcropSize;
    ExynosRect bdsSize;
    ExynosRect tempRect;
    int ispSrcW, ispSrcH;

    int supportedSensorExMode = m_parameters->getSupportedSensorExMode();
    int extendSensorMode = m_configurations->getModeValue(CONFIGURATION_EXTEND_SENSOR_MODE);

    uint32_t sensorPipeId = (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) ? PIPE_FLITE : PIPE_3AA;

    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);

    uint32_t minFrameRate = 0, maxFrameRate = 0, sensorFrameRate = 0;
    if (m_parameters->isSensorModeTransition() == true &&
        m_parameters->isStillShotSensorFpsSupported() == true) {
        sensorFrameRate = m_parameters->getStillShotSensorFps();
    } else {
        m_configurations->getPreviewFpsRange(&minFrameRate, &maxFrameRate);
        sensorFrameRate = maxFrameRate;

#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE
        if (m_configurations->getMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE)) {
            int vendorFps = 0;
            vendorFps = m_configurations->getModeValue(CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE);
            sensorFrameRate = vendorFps;
            CLOGD("Override vendor :FPS(%d)", vendorFps);
        }
#endif
    }

    m_parameters->getPreviewBayerCropSize(&bnsSize, &bcropSize, false);
    //m_parameters->getPreviewBdsSize(&bdsSize, false);
#ifdef SUPPORT_SENSOR_MODE_CHANGE
    m_parameters->getSize(HW_INFO_SENSOR_MODE_CHANGE_BDS_SIZE, (uint32_t *)&bdsSize.w, (uint32_t *)&bdsSize.h);
#endif

    /*
     * FLITE
     */

    /* setParam for Frame rate : must after setInput on Flite */
    struct v4l2_streamparm streamParam;
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = sensorFrameRate;

    if (supportedSensorExMode & (1 << extendSensorMode)) {
        bool flagExtendedMode = true;
        if (extendSensorMode == EXTEND_SENSOR_MODE_SW_REMOSAIC) {
            int count = 0;
            int supportedRemosaicMode = m_parameters->getSupportedRemosaicModes(count);
            if (count < 2) {
                flagExtendedMode = false;
            }
        }

        if (flagExtendedMode) {
            streamParam.parm.capture.extendedmode = extendSensorMode;
            CLOGI("Set (extendedmode=%d), supportedSensorExMode(0x%x)", extendSensorMode, supportedSensorExMode);
        }
    }
    CLOGI("Set framerate (denominator=%d)", sensorFrameRate);

    ret = setParam(&streamParam, sensorPipeId);
    if (ret != NO_ERROR) {
        CLOGE("FLITE setParam(frameRate(%d), pipeId(%d)) fail", sensorFrameRate, sensorPipeId);
        return INVALID_OPERATION;
    }

    m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    ret = m_setSensorSize(sensorPipeId, hwSensorW, hwSensorH);
    if (ret != NO_ERROR) {
        CLOGE("m_setSensorSize(pipeId(%d), hwSensorW(%d), hwSensorH(%d)) fail", sensorPipeId, hwSensorW, hwSensorH);
        return ret;
    }

    /* FLITE */
    pipeId = sensorPipeId;

    nodeType = getNodeType(PIPE_FLITE);

    /* set v4l2 buffer size */
    tempRect.fullW = 32;
    tempRect.fullH = 64;
    tempRect.colorFormat = bayerFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

    /* Set output node default info */
    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_FLITE);

    /* BAYER */
    nodeType = getNodeType(PIPE_VC0);
    perFramePos = PERFRAME_BACK_VC0_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_VC0);

    /* set v4l2 buffer size */
    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    CLOGE("[SENSOR TRANSITION] hwSensor(%d x %d) format(%x)", tempRect.fullW, tempRect.fullH, bayerFormat);
    /* set v4l2 video node bytes per plane */
    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->isDepthMapSupported()) {
        /* Depth Map Configuration */
        int depthMapW = 0, depthMapH = 0;
        int depthMapFormat = DEPTH_MAP_FORMAT;

        ret = m_parameters->getDepthMapSize(&depthMapW, &depthMapH);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getDepthMapSize");
            return ret;
        }

        CLOGI("DepthMapSize %dx%d", depthMapW, depthMapH);

        tempRect.fullW = depthMapW;
        tempRect.fullH = depthMapH;
        tempRect.colorFormat = depthMapFormat;

        nodeType = getNodeType(PIPE_VC1);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = NUM_DEPTHMAP_BUFFERS;

        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

#ifdef SUPPORT_PD_IMAGE
    if (m_parameters->isPDImageSupported()) {
        /* Depth Map Configuration */
        int pdImageW = 0, pdImageH = 0;
        int bayerFormat = PD_IMAGE_FORMAT;

        ret = m_parameters->getPDImageSize(pdImageW, pdImageH);
        if (ret != NO_ERROR) {
            CLOGE("Failed to getPDImageSize");
            return ret;
        }

        CLOGI("getPDImageSize %dx%d", pdImageW, pdImageH);

        tempRect.fullW = pdImageW;
        tempRect.fullH = pdImageH;
        tempRect.colorFormat = bayerFormat;

        nodeType = getNodeType(PIPE_VC1);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = NUM_PD_IMAGE_BUFFERS;

        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

    ////////////////////////////////////////////////
    // SENSOR GYRO
    if (m_parameters->isSensorGyroSupported() == true) {
        ret = m_parameters->getSize(HW_INFO_HW_SENSOR_GYRO_SIZE, (uint32_t *)&tempRect.fullW, (uint32_t *)&tempRect.fullH);
        if (ret != NO_ERROR) {
            CLOGE("getSize(HW_INFO_HW_SENSOR_GYRO_SIZE) fail");
            return ret;
        }

        tempRect.colorFormat = m_parameters->getSensorGyroFormat();

        CLOGI("Sensor Gyro size %dx%d", tempRect.fullW, tempRect.fullH);

        nodeType = getNodeType(PIPE_VC3);
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, tempRect.colorFormat);
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }

    ////////////////////////////////////////////////

    /* setup pipe info to FLITE pipe */
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[sensorPipeId]->setupPipe(pipeInfo, m_sensorIds[sensorPipeId]);
        if (ret != NO_ERROR) {
            CLOGE("FLITE setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /*
     * 3AA
     */
    pipeId = PIPE_3AA;

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA);
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AA);

    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_OTF) {
        /* set v4l2 buffer size */
        tempRect.fullW = 32;
        tempRect.fullH = 64;
        tempRect.colorFormat = bayerFormat;
        CLOGE("[SENSOR TRANSITION] hwSensor(%d x %d) format(%x)", tempRect.fullW, tempRect.fullH, bayerFormat);
        CLOGE("[SENSOR TRANSITION] bufInfo.count(%d)", config->current->bufInfo.num_3aa_buffers);

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
    } else if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

#if 0
        switch (bayerFormat) {
        case V4L2_PIX_FMT_SBGGR16:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR12:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
            break;
        case V4L2_PIX_FMT_SBGGR10:
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
            break;
        default:
            CLOGW("Invalid bayer format(%d)", bayerFormat);
            pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
            break;
        }
#endif
        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
    }

    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

    /* Set output node default info */
    SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_3AA);

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC);
    perFramePos = PERFRAME_BACK_3AC_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AC);

    /* set v4l2 buffer size */
    tempRect.fullW = bcropSize.w;
    tempRect.fullH = bcropSize.h;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

#if 0
    /* set v4l2 video node bytes per plane */
    switch (bayerFormat) {
    case V4L2_PIX_FMT_SBGGR16:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR12:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR10:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
        break;
    default:
        CLOGW("Invalid bayer format(%d)", bayerFormat);
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    }
#endif

    /* set v4l2 video node buffer count */
    switch(m_parameters->getReprocessingBayerMode()) {
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
        case REPROCESSING_BAYER_MODE_NONE:
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
            break;
        case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
            break;
        default:
            CLOGE("Invalid reprocessing mode(%d)", m_parameters->getReprocessingBayerMode());
    }

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP);
    perFramePos = PERFRAME_BACK_3AP_POS;
    bayerFormat = m_parameters->getBayerFormat(PIPE_3AP);

    /* set v4l2 buffer size */
    tempRect.fullW = bdsSize.w;
    tempRect.fullH = bdsSize.h;
    tempRect.colorFormat = bayerFormat;
    CLOGE("[SENSOR TRANSITION] hwSensor(%d x %d) format(%x)", tempRect.fullW, tempRect.fullH, bayerFormat);

    /* set v4l2 video node bytes per plane */
    pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
#if 0
    switch (bayerFormat) {
    case V4L2_PIX_FMT_SBGGR16:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR12:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR10:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
        break;
    default:
        CLOGW("Invalid bayer format(%d)", bayerFormat);
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    }
#endif

    pipeInfo[nodeType].pixelCompInfo = m_parameters->getPixelCompInfo(PIPE_3AP);
    pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat, m_parameters->isUseBayerCompression());

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
    CLOGE("[SENSOR TRANSITION] bufInfo.count(%d)", pipeInfo[nodeType].bufInfo.count);

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_3AF
    /* 3AF */
    nodeType = getNodeType(PIPE_3AF);
    perFramePos = 0; //it is dummy info
    // TODO: need to remove perFramePos from the SET_CAPTURE_DEVICE_BASIC_INFO

    /* set v4l2 buffer size */
    tempRect.fullW = MAX_VRA_INPUT_WIDTH;
    tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
    tempRect.colorFormat = m_parameters->getHW3AFdFormat();

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

#ifdef SUPPORT_ME
    if(m_parameters->getLeaderPipeOfMe() == pipeId) {
        /* ME */
        nodeType = getNodeType(PIPE_ME);
        perFramePos = PERFRAME_BACK_ME_POS;

        /* set v4l2 buffer size */
        m_parameters->getMeSize(&tempRect.fullW, &tempRect.fullH);
        tempRect.colorFormat = hwMeFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = CAMERA_ME_MAX_BUFFER;

        /* set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
    }
#endif

    /* setup pipe info to 3AA pipe */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("3AA setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_initFlitePipe(hwSensorW, hwSensorH, sensorFrameRate);
    if (ret != NO_ERROR) {
        CLOGE("m_initFlitePipe() fail");
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::startSensorPipe(void) {
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;

    ret = m_pipes[PIPE_3AA]->start();
    if (ret != NO_ERROR) {
        CLOGE("3AA start fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    int pipeId = PIPE_3AA;
    if (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_FLITE;
        ret = m_pipes[PIPE_FLITE]->start();
        if (ret != NO_ERROR) {
            CLOGE("FLITE start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_needSensorStreamOn == true) {
        ret = m_pipes[pipeId]->prepare();
        if (ret != NO_ERROR) {
            CLOGE("pipeId %d prepare fail, ret(%d)", pipeId, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        CLOGD("sensorStream(true)");
        ret = m_pipes[pipeId]->sensorStream(true);
        if (ret != NO_ERROR) {
            CLOGE("pipeId %d sensorStream on fail, ret(%d)", pipeId, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* change state to streamOn(standby off) */
        m_sensorStandby = false;
    } else {
        CLOGI("skip the Sensor Stream");
    }

    ret = m_transitState(FRAME_FACTORY_STATE_RUN);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState. ret %d", ret);
        return ret;
    }

    ret = m_setQOS();

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::stopSensorPipe(void) {
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;

    uint32_t sensorPipeId = (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) ? PIPE_FLITE : PIPE_3AA;

    CLOGD("stopPipeThread pipe_id(%s)", (sensorPipeId == PIPE_FLITE)?"PIPE_FLITE":"PIPE_3AA");

    /* Flush sensor pipe */
    m_pipes[sensorPipeId]->flush();

    /* Stop sensor pipe thread */
    STOP_PIPE_THREAD(sensorPipeId);
    if (sensorPipeId != PIPE_3AA)
        STOP_PIPE_THREAD(PIPE_3AA);

    /* sensorStream(false) */
    CLOGD("sensorStream(false)");
    ret = m_pipes[sensorPipeId]->sensorStream(false);
    if (ret != NO_ERROR) {
        CLOGE("Failed to sensorStream OFF. ret %d", ret);
        return INVALID_OPERATION;
    }

    /* stop sensor pipe */
    if (sensorPipeId == PIPE_FLITE) {
        /* stop FLITE pipe */
        CLOGD("stopPipe(PIPE_FLITE)");
        STOP_PIPE(PIPE_FLITE);
    }

    /* 3AA force done */
    CLOGD("forceDone(PIPE_3AA)");
    ret = m_pipes[PIPE_3AA]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_3AA force done fail, ret(%d)", ret);
        /* TODO: exception handling */
    }

    /* stop 3AA pipe */
    CLOGD("stopPipe(PIPE_3AA)");
    STOP_PIPE(PIPE_3AA);

    CLOGD("m_stopSensorPipe end");
    return ret;
}

status_t ExynosCameraFrameFactoryPreview::startSensorPipeThread(void) {
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;

    CLOGI("start pre-ordered initial pipe thread");

    if (m_sensorStandby == false) {
        if (m_flagPipeXNeedToRun(PIPE_FLITE) == true) {
            ret = startThread(PIPE_FLITE);
            if (ret != NO_ERROR)
                return ret;
        }

        ret = startThread(PIPE_3AA);
        if (ret != NO_ERROR)
            return ret;
    } else {
        CLOGI("skip the starting Sensor Thread");
    }

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::stopAndWaitSensorPipeThread(void) {
    Mutex::Autolock lock(m_sensorStandbyLock);

    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    uint32_t sensorPipeId = (m_flagFlite3aaOTF == HW_CONNECTION_MODE_M2M) ? PIPE_FLITE : PIPE_3AA;

    CLOGD("stopAndWaitPipeThread pipe_id(%s)", (sensorPipeId == PIPE_FLITE)?"PIPE_FLITE":"PIPE_3AA");

    /* stop sensor pipe */
    if (sensorPipeId == PIPE_FLITE) {
        ret = stopThreadAndWait(PIPE_FLITE);
        if (ret != NO_ERROR) {
            CLOGE("PIPE_FLITE stopThreadAndWait fail, ret(%d)", ret);
            funcRet |= ret;
        }
    }

    ret = stopThreadAndWait(PIPE_3AA);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_3AA stopThreadAndWait fail, ret(%d)", ret);
        funcRet |= ret;
    }

    CLOGD("m_stopAndWaitSensorPipeThread end");

    return funcRet;
}

status_t ExynosCameraFrameFactoryPreview::m_setQOS(void) {
    status_t ret = NO_ERROR;

    int32_t bigMinLock, littleMinLock;
    bool lockFlag = m_configurations->getMinLockFreq(bigMinLock, littleMinLock);
    if (lockFlag) {
        CLOGI("CPU lock(%d) big(0x%x) little(0x%x)", lockFlag, bigMinLock, littleMinLock);
        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER1, bigMinLock);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER1 setControl fail, ret(%d)", ret);
        }

        ret = m_pipes[PIPE_3AA]->setControl(V4L2_CID_IS_DVFS_CLUSTER0, littleMinLock);
        if (ret != NO_ERROR) {
            CLOGE("V4L2_CID_IS_DVFS_CLUSTER0 setControl fail, ret(%d)", ret);
        }
    }

    return ret;
}

#endif //SUPPORT_SENSOR_MODE_CHANGE


}; /* namespace android */
