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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraMCPipe"
#define INTERNAL_FRAME_LOG_DURATION (33 * 5) /* about 5s */

#include "ExynosCameraMCPipe.h"

#ifdef USE_DEBUG_PROPERTY
#include "ExynosCameraProperty.h"
#endif

namespace android {

#ifdef USE_MCPIPE_SERIALIZATION_MODE
Mutex ExynosCameraMCPipe::g_serializationLock[ExynosCameraParameters::CRITICAL_SECTION_TYPE_END];
#endif

ExynosCameraMCPipe::~ExynosCameraMCPipe()
{
    this->destroy();
}

status_t ExynosCameraMCPipe::create(int32_t *sensorIds)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    ret = m_createSensorNode(sensorIds);
    if (ret < 0) {
        CLOGE("m_createSensorNode() fail");
        return ret;
    }

    ret = m_preCreate();
    if (ret != NO_ERROR) {
        CLOGE("m_preCreate() fail, ret(%d)", ret);
        return ret;
    }

    ret = m_postCreate();
    if (ret != NO_ERROR) {
        CLOGE("m_postCreate() fail, ret(%d)", ret);
        return ret;
    }


#ifdef SUPPORT_POST_SCALER_ZOOM
    /* TODO: FIXME: set proper gsc number */
    m_gscWrapper = new ExynosCameraGSCWrapper(PREVIEW_GSC_NODE_NUM);
    if (m_gscWrapper == nullptr) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Fail to new gscWrapper instance. so, assert!!!!",
                __FUNCTION__, __LINE__);
    }

    if (m_gscWrapper->create() != NO_ERROR) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Fail to create gscWrapper so, assert!!!!",
                __FUNCTION__, __LINE__);
    }
#endif

    return ret;
}

status_t ExynosCameraMCPipe::precreate(__unused int32_t *sensorIds)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    ret = m_createSensorNode(sensorIds);
    if (ret < 0) {
        CLOGE("m_createSensorNode() fail");
        return ret;
    }

    ret = m_preCreate();
    if (ret != NO_ERROR) {
        CLOGE("m_preCreate() fail, ret(%d)", ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraMCPipe::postcreate(int32_t *sensorIds)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    ret = m_postCreate(sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("m_postCreate() fail, ret(%d)", ret);
        return ret;
    }

    CLOGV("postcreate() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::destroy(void)
{
    CLOGI("");
    status_t ret = NO_ERROR;

    for (int i = (MAX_NODE - 1); i >= OUTPUT_NODE; i--) {
        if (m_node[i] != NULL) {
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            if (m_sharedNode[i] == true) {
                CLOGD("node(%s)->destroy skipped, shared node ret(%d)", m_deviceInfo->nodeName[i], ret);
                continue;
            }
#endif

            if (i == m_sensorNodeIndex) {
                m_destroyNode(m_cameraId, m_node[i]);
                m_node[i] = NULL;
                continue;
            }

#ifdef SUPPORT_DEPTH_MAP
            if (i == m_depthVcNodeIndex) {
                m_destroyVcNode(m_cameraId, m_node[i]);
                m_node[i] = NULL;
                continue;
            }
#endif // SUPPORT_DEPTH_MAP

            if (OUTPUT_NODE < i
                && m_node[OUTPUT_NODE] != NULL
                && m_deviceInfo->nodeNum[OUTPUT_NODE] == m_deviceInfo->nodeNum[i]) {
                /* In this case(3AA of 54xx), 3AA, 3AP node is same.
                 * So, should close one node. Skip 3AP node.
                 */
            } else {
                ret = m_node[i]->close();
                if (ret != NO_ERROR) {
                    CLOGE("Main node(%s) close fail, ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }
            }

            SAFE_DELETE(m_node[i]);
            CLOGD("Main node(%s, sensorIds:%d) closed",
                     m_deviceInfo->nodeName[i], m_sensorIds[i]);
        }
    }

    CLOGD("Node destroyed");

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        SAFE_DELETE(m_inputFrameQ);
    }

    if (m_requestFrameQ != NULL) {
        m_requestFrameQ->release();
        SAFE_DELETE(m_requestFrameQ);
    }

#ifdef SUPPORT_POST_SCALER_ZOOM
    if (m_gscWrapper != nullptr) {
        m_gscWrapper->destroy();
        m_gscWrapper = nullptr;
    }
#endif

#ifdef DEBUG_DUMP_IMAGE
    if (m_dumpImageQ != nullptr) {
        m_dumpImageQ->release();
        SAFE_DELETE(m_dumpImageQ);
    }
#endif

    CLOGI("destroy() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    uint32_t pipeId = 0;
    int result = 0;
    ExynosCameraNode *setFileSettingNode = NULL;

    /* TODO: check node state */

    /* set new sensorId to m_sensorIds */
    if (sensorIds != NULL) {
        CLOGD("set new sensorIds");
        for (int i = OUTPUT_NODE; i < MAX_NODE; i++)
            m_sensorIds[i] = sensorIds[i];
    }

    ret = m_setInput(m_node, m_deviceInfo->nodeNum, m_sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("m_setInput(Main) fail, ret(%d)", ret);
        return ret;
    }

    if (pipeInfos != NULL) {
        ret = m_setPipeInfo(pipeInfos);
        if (ret != NO_ERROR) {
            CLOGE("m_setPipeInfo() fail, ret(%d)", ret);
            return ret;
        }
    } else {
        CLOGE("pipeInfos is NULL");
        return BAD_VALUE;
    }

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        result = getPipeId((enum NODE_TYPE)i);
        if (0 <= result) {
            if (m_node[i] != NULL)
                setFileSettingNode = m_node[i];
            else
                continue;

            pipeId = (uint32_t)result;

            ret = m_setSetfile(setFileSettingNode, pipeId);
            if (ret != NO_ERROR) {
                CLOGE("m_setSetfile() fail, ret(%d)", ret);
                return ret;
            }
        }

        for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
            m_runningFrameList[i][j] = NULL;
        }
        m_numOfRunningFrame[i] = 0;
    }

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    if (getPipeId() == PIPE_3AA || getPipeId() == PIPE_FLITE) {
        m_prepareBufferCount += m_parameters->getSensorControlDelay();
    }

    CLOGI("setupPipe() is succeed, Pipe(%d), prepare(%d)", getPipeId(), m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::prepare(void)
{
    /* need modify */
    CLOGD("");
    status_t ret = NO_ERROR;
    int32_t prepareCount = m_prepareBufferCount;

    if (prepareCount > m_inputFrameQ->getSizeOfProcessQ()) {
        CLOGE("m_putBufferThreadFunc no Frame(count = %d), running(%d)",
            m_inputFrameQ->getSizeOfProcessQ(),
            m_putBufferThread->isRunning());
        prepareCount = m_inputFrameQ->getSizeOfProcessQ();
    }

    /*
     * prepare on only capture node
     * output node doesn't need prepare
     */
    for (int32_t i = 0; i < prepareCount; i++)
        m_putBufferThreadFunc();

    CLOGI("prepare() is succeed, Pipe(%d), prepare(%d/%d)",
            getPipeId(), prepareCount, m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::start(void)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    /* TODO: check state ready for start */
    ret = m_startNode();
    if (ret != NO_ERROR) {
        CLOGE("m_startNode() fail, ret(%d)", ret);
        return ret;
    }

    m_threadState = 0;
    m_threadRenew = 0;

    m_lastFrameCount = 0;
    m_lastMetaFrameCount = 0;
    m_flagStartPipe = true;
    m_flagTryStop = false;
#ifdef DEBUG_DUMP_IMAGE
    m_dumpBufferCount = 0;
#ifdef SEQUENTIAL_RAW_DUMP
    m_sequentialDumpCount = SEQUENTIAL_RAW_DUMP_COUNT;
#endif
#endif

    CLOGI("start() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::stop(void)
{
    CLOGV("");
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    ret = m_stopNode();
    if (ret != NO_ERROR) {
        CLOGE("m_stopNode() fail, ret(%d)", ret);
        /* If stop() is error, driver will recover */
        funcRet |= ret;
    }

    m_putBufferThread->requestExitAndWait();
    m_getBufferThread->requestExitAndWait();

    CLOGD("Thread exited");

    m_inputFrameQ->release();
    m_requestFrameQ->release();

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_unlockSerializeOperation((enum pipeline)getPipeId());
#endif

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
            m_runningFrameList[i][j] = NULL;
        }
        m_numOfRunningFrame[i] = 0;
        m_skipBuffer[i].index = -2;
        m_skipPutBuffer[i] = false;

        if (m_node[i] != NULL)
            m_node[i]->removeItemBufferQ();

    }

    m_sensorStandbyLock.lock();
    m_flagSensorStandby = SENSOR_STANDBY_OFF;
    m_sensorStandbyLock.unlock();
    m_flagStartPipe = false;
    m_flagTryStop = false;

#ifdef DEBUG_DUMP_IMAGE
    m_dumpBufferThread->requestExitAndWait();
    m_dumpImageQ->release();
    m_RawFrameHoldList.release();
#endif

    CLOGI("stop() is succeed, Pipe(%d)", getPipeId());

    return funcRet;
}

bool ExynosCameraMCPipe::flagStart(void)
{
    return m_flagStartPipe;
}

status_t ExynosCameraMCPipe::startThread(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    if (m_outputFrameQ == NULL) {
        CLOGE("outputFrameQ is NULL, cannot start");
        return INVALID_OPERATION;
    }

    /* init the internal frame log count */
    m_putInternalFrameLogCnt = 0;
    m_getInternalFrameLogCnt = 0;

    if (m_flagSensorStandby != SENSOR_STANDBY_ON) {
        m_putBufferThread->run(PRIORITY_URGENT_DISPLAY);
        m_getBufferThread->run(PRIORITY_URGENT_DISPLAY);
    }

    CLOGI("startThread is succeed, Pipe(%d), standby(%d)", getPipeId(), m_flagSensorStandby);

    return ret;
}

status_t ExynosCameraMCPipe::stopThread(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    m_putBufferThread->requestExit();
    m_getBufferThread->requestExit();

    m_inputFrameQ->sendCmd(WAKE_UP);
    m_requestFrameQ->sendCmd(WAKE_UP);

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_unlockSerializeOperation((enum pipeline)getPipeId());
#endif

    m_dumpRunningFrameList();

#ifdef DEBUG_DUMP_IMAGE
    m_dumpBufferThread->requestExit();
    m_dumpImageQ->sendCmd(WAKE_UP);
#endif

    CLOGI("stopThread is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::stopThreadAndWait(int sleep, int times)
{
    CLOGD(" IN");
    status_t status = NO_ERROR;
    int i = 0;

    for (i = 0; i < times ; i++) {
        if (m_putBufferThread->isRunning() == false && m_getBufferThread->isRunning() == false) {
            break;
        }
        usleep(sleep * 1000);
    }

    if (i >= times) {
        status = TIMED_OUT;
        CLOGE(" stopThreadAndWait failed, waitTime(%d)ms", sleep*times);
    }

    CLOGV(" OUT");
    return status;
}

bool ExynosCameraMCPipe::flagStartThread(void)
{
    return m_putBufferThread->isRunning();
}

status_t ExynosCameraMCPipe::sensorStream(bool on)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    CLOGD("");
    status_t ret = NO_ERROR;

    int value = on ? IS_ENABLE_STREAM : IS_DISABLE_STREAM;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setControl(V4L2_CID_IS_S_STREAM, value);
            if (ret != NO_ERROR) {
                CLOGE("sensorStream failed, %s node, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
            }

            return ret;
        }
    }

    /* Set flag for prevent to duplication setting */
    if (on == false) {
        m_flagSensorStandby = SENSOR_STANDBY_ON;
    }

    CLOGE("All Nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::sensorStandby(bool on)
{
    m_sensorStandbyLock.lock();

    status_t ret = NO_ERROR;

    if (on == false) {
        if (m_flagSensorStandby == SENSOR_STANDBY_ON
            && m_flagStartPipe == true) {
            /* stop Thread */
            ret = stopThread();
            if (ret != NO_ERROR) {
                CLOGE("Stop Thread fail! ret(%d)", ret);
            }

            CLOGI("Wait for putBufferThread Exit!!(%d)", m_inputFrameQ->getSizeOfProcessQ());
            m_putBufferThread->requestExitAndWait();
            CLOGI("Wait Done for putBufferThread Exit!!");
            CLOGI("Wait for getBufferThread Exit!!(%d)", m_requestFrameQ->getSizeOfProcessQ());

            /* unlock for a while until getBufferThread exit */
            m_sensorStandbyLock.unlock();
            m_getBufferThread->requestExitAndWait();
            CLOGI("Wait Done for getBufferThread Exit!!");
            m_sensorStandbyLock.lock();

            m_skipBufferHint = false;

            /* just check if the standby state was changed in short time */
            if (m_flagSensorStandby != SENSOR_STANDBY_ON) {
                CLOGW("Invalid State!!(%d)", m_flagSensorStandby);
                goto UNLOCK;
            }

            ret = prepare();
            if (ret != NO_ERROR) {
                CLOGE("Prepare fail! ret(%d)", ret);
            }

            ret = m_sensorStandby(on);
            if (ret != NO_ERROR) {
                CLOGE("Sensor standby(%s) fail! ret(%d)",
                        (on?"On":"Off"), ret);
            }
        }
        m_skipBufferHint = false;

        m_flagSensorStandby = SENSOR_STANDBY_OFF;

        if (m_flagStartPipe == true) {
            ret = startThread();
            if (ret != NO_ERROR) {
                CLOGE("Stop Thread fail! ret(%d)", ret);
            }
        }
    }

    if (on == true
        && m_flagSensorStandby == SENSOR_STANDBY_OFF) {
        m_skipBufferHint = true;
        if (m_flagStartPipe == false) {
            m_flagSensorStandby = SENSOR_STANDBY_ON;
        } else {
            m_flagSensorStandby = SENSOR_STANDBY_ON_READY;
        }

        m_inputFrameQ->sendCmd(WAKE_UP);
        m_requestFrameQ->sendCmd(WAKE_UP);
    }

UNLOCK:
    CLOGD("%s(on:%d, standby:%d, start:%d)",
            (on ? "On" : "Off"), on, m_flagSensorStandby, m_flagStartPipe);

    m_sensorStandbyLock.unlock();

    return ret;
}

enum SENSOR_STANDBY_STATE ExynosCameraMCPipe::getSensorStandbyState(void)
{
    Mutex::Autolock lock(m_sensorStandbyLock);

    return m_flagSensorStandby;
}

status_t ExynosCameraMCPipe::forceDone(unsigned int cid, int value)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    if (m_node[OUTPUT_NODE] == NULL) {
        CLOGE("m_node[OUTPUT_NODE] is NULL");
        return INVALID_OPERATION;
    }

    ret = m_forceDone(m_node[OUTPUT_NODE], cid, value);
    if (ret != NO_ERROR) {
        CLOGE("m_forceDone() is failed, ret");
        return ret;
    }

    CLOGI("forceDone() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::setControl(int cid, int value)
{
    CLOGV("cdi %d value %d", cid, value);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setControl(cid, value);
            if (ret != NO_ERROR) {
                CLOGE("m_node(%s)->setControl failed, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGV("setControl() is succeed, Pipe(%d)", getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("All nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::setControl(int cid, int value, enum NODE_TYPE nodeType)
{
    CLOGV("cid %d value %d nodeType %d", cid, value, nodeType);
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("m_node[%d] == NULL. so, fail", nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->setControl(cid, value);
    if (ret != NO_ERROR) {
        CLOGE("m_node(%s)->setControl failed, ret(%d)",
                 m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGV("setControl() is succeed, Pipe(%d)", getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::getControl(int cid, int *value)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->getControl(cid, value);
            if (ret != NO_ERROR) {
                CLOGE("m_node(%s)->getControl failed, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGV("getControl() is succeed, Pipe(%d)", getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("All nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::getControl(int cid, int *value, enum NODE_TYPE nodeType)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("m_node[%d] == NULL. so, fail", nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->getControl(cid, value);
    if (ret != NO_ERROR) {
        CLOGE("m_node(%s)->getControl failed, ret(%d)",
                 m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGV("getControl() is succeed, Pipe(%d)", getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::setExtControl(struct v4l2_ext_controls *ctrl)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setExtControl(ctrl);
            if (ret != NO_ERROR) {
                CLOGE("m_node(%s)->setControl failed, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGV("setControl() is succeed, Pipe(%d)",
                     getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("All nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::setExtControl(struct v4l2_ext_controls *ctrl, enum NODE_TYPE nodeType)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("m_node[%d] == NULL. so, fail",
                 nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->setExtControl(ctrl);
    if (ret != NO_ERROR) {
        CLOGE("m_node(%s)->setControl failed, ret(%d)",
                 m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGV("setControl() is succeed, Pipe(%d)",
             getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::setParam(struct v4l2_streamparm streamParam)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setParam(&streamParam);
            if (ret != NO_ERROR) {
                CLOGE("m_node(%s)->setParam failed, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGV("setParam() is succeed, Pipe(%d)", getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("All nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::pushFrame(ExynosCameraFrameSP_dptr_t newFrame)
{
    Mutex::Autolock lock(m_pipeframeLock);
    if (newFrame == NULL) {
        CLOGE("newFrame is NULL");
        return BAD_VALUE;
    }

    if ((m_skipBufferHint == true) && (m_standbyHintFlag == true)) {
        CLOGE("[F%d]:  m_flagSensorStandby = %d : FrameQ_size (%d, %d) skipping the frame",
                        newFrame->getFrameCount(), m_flagSensorStandby,
                        m_inputFrameQ->getSizeOfProcessQ(),
                        m_requestFrameQ->getSizeOfProcessQ());

        /* Error handling for SrcBuffer and Frame */
        return m_frameErrorHandler(newFrame);
    }

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    sp<PipeHandler> handler = NULL;
    status_t ret = getHandler(PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ, handler);
    if (ret == NO_ERROR) {
        PIPE_HANDLER::QUERY status = PIPE_HANDLER::QUERY_BASE;
        handler->process(PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ, newFrame, status);
    }
#endif

    m_inputFrameQ->pushProcessQ(&newFrame);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::instantOn(int32_t numFrames)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    uint32_t nodeCount = 0;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraBuffer newBuffer;

    if (m_inputFrameQ->getSizeOfProcessQ() != numFrames) {
        CLOGE("instantOn need %d Frames, but %d Frames are queued",
                 numFrames, m_inputFrameQ->getSizeOfProcessQ());
        return BAD_VALUE;
    }

    for (int i = (MAX_CAPTURE_NODE - 1); i >= OUTPUT_NODE; i--) {
        if (m_node[i] != NULL) {
#ifdef SUPPORT_DEPTH_MAP
            if (i == m_depthVcNodeIndex) {
                continue;
            }
#endif // SUPPORT_DEPTH_MAP

            ret = m_node[i]->start();
            if (ret != NO_ERROR) {
                CLOGE("node(%s) instantOn fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            nodeCount++;
        }
    }

    if (nodeCount == 0) {
        CLOGE("All nodes is NULL");
        return INVALID_OPERATION;
    }

    for (int i = 0; i < numFrames; i++) {
        ret = m_inputFrameQ->popProcessQ(&newFrame);
        if (ret != NO_ERROR) {
            CLOGE("Wait and pop fail, ret(%d)", ret);
            return ret;
        }

        if (newFrame == NULL) {
            CLOGE("newFrame is NULL");
            return INVALID_OPERATION;
        }

        ret = newFrame->getSrcBuffer(getPipeId(), &newBuffer);
        if (ret != NO_ERROR) {
            CLOGE("Frame get buffer fail, ret(%d)", ret);
            return ret;
        }

        if (m_node[OUTPUT_NODE] != NULL) {
            CLOGD("Put instantOn Buffer (index %d)", newBuffer.index);

            ret = m_node[OUTPUT_NODE]->putBuffer(&newBuffer);
            if (ret != NO_ERROR) {
                CLOGE("putBuffer() fail, ret(%d)", ret);
                return ret;
                /* TODO: doing exception handling */
            }
        } else {
            CLOGE("m_node[OUTPUT_NODE] is NULL");
            return INVALID_OPERATION;
        }
    }

    CLOGI("instantOn() is succeed, Pipe(%d), Frames(%d)",
             getPipeId(), numFrames);

    return ret;
}

status_t ExynosCameraMCPipe::instantOff(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_node[i] != NULL) {
#ifdef SUPPORT_DEPTH_MAP
            if (i == m_depthVcNodeIndex) {
                continue;
            }
#endif // SUPPORT_DEPTH_MAP

            ret = m_node[i]->stop();
            if (ret != NO_ERROR) {
                CLOGE("node(%s) stop fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }

            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("node(%s) clrBuffers fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    CLOGI("instantOff() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition)
{
    CLOGV("");
    status_t ret = NO_ERROR;
    int planeCount = 0;

    if (pipePosition == DST_PIPE) {
        if (m_node[OUTPUT_NODE] == NULL) {
            CLOGE("m_node[OUTPUT_NODE] is NULL");
            return INVALID_OPERATION;
        }

        ret = m_node[OUTPUT_NODE]->getSize(fullW, fullH);
        if (ret != NO_ERROR) {
            CLOGE("node(%s) getSize fail, ret(%d)",
                     m_deviceInfo->nodeName[OUTPUT_NODE], ret);
            return ret;
        }

        ret = m_node[OUTPUT_NODE]->getColorFormat(colorFormat, &planeCount);
        if (ret != NO_ERROR) {
            CLOGE("node(%s) getColorFormat fail, ret(%d)",
                     m_deviceInfo->nodeName[OUTPUT_NODE], ret);
            return ret;
        }
    } else if (pipePosition == SRC_PIPE) {
        for (int i = (MAX_CAPTURE_NODE - 1); i > OUTPUT_NODE; i--) {
            if (m_node[i] != NULL) {
                ret = m_node[i]->getSize(fullW, fullH);
                if (ret != NO_ERROR) {
                    CLOGE("node(%s) getSize fail, ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->getColorFormat(colorFormat, &planeCount);
                if (ret != NO_ERROR) {
                    CLOGE("node(%s) getColorFormat fail, ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGV("getPipeInfo() is succeed, Pipe(%d)", getPipeId((enum NODE_TYPE)i));
                return ret;
            }
        }
        CLOGE("all capture m_node is NULL");
        return INVALID_OPERATION;
    } else {
        CLOGE("Pipe position is Invalid, position(%d)", pipePosition);
        return BAD_VALUE;
    }

    CLOGV("getPipeInfo() is succeed, Pipe(%d)", getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::setPipeId(uint32_t id)
{
    return this->setPipeId(OUTPUT_NODE, id);
}

uint32_t ExynosCameraMCPipe::getPipeId(void)
{
    return (uint32_t)this->getPipeId(OUTPUT_NODE);
}

status_t ExynosCameraMCPipe::setPipeId(enum NODE_TYPE nodeType, uint32_t id)
{
    if (nodeType < OUTPUT_NODE || MAX_NODE <= nodeType) {
        CLOGE("Invalid nodeType(%d). so, fail", nodeType);
        return BAD_VALUE;
    }

    CLOGD("nodeType(%d), id(%d)", nodeType, id);

    m_pipeIdArr[nodeType] = id;

    if (nodeType == OUTPUT_NODE)
        m_pipeId = id;

    return NO_ERROR;
}

int ExynosCameraMCPipe::getPipeId(enum NODE_TYPE nodeType)
{
    if (nodeType < OUTPUT_NODE || MAX_NODE <= nodeType) {
        CLOGE("Invalid nodeType(%d). so, fail", nodeType);
        return -1;
    }

    return m_pipeIdArr[nodeType];
}


status_t ExynosCameraMCPipe::setPipeName(const char *pipeName)
{
    CLOGD("");

    strncpy(m_name,  pipeName, (EXYNOS_CAMERA_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

char *ExynosCameraMCPipe::getPipeName(void)
{
    return m_name;
}

status_t ExynosCameraMCPipe::setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier)
{
    CLOGD("");

    m_bufferSupplier = bufferSupplier;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setBufferManager(ExynosCameraBufferManager **bufferManager)
{
    CLOGD("");

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++)
        m_bufferManager[i] = bufferManager[i];

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::clearInputFrameQ(void)
{
    CLOGD("");

    if (m_inputFrameQ != NULL)
        m_inputFrameQ->release();

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getInputFrameQ(frame_queue_t **inputFrameQ)
{
    *inputFrameQ = m_inputFrameQ;

    if (*inputFrameQ == NULL)
        CLOGE("inputFrameQ is NULL");

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setOutputFrameQ(frame_queue_t *outputFrameQ)
{
    CLOGV("");

    m_outputFrameQ = outputFrameQ;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getOutputFrameQ(frame_queue_t **outputFrameQ)
{
    *outputFrameQ = m_outputFrameQ;

    if (*outputFrameQ == NULL)
        CLOGE("outputFrameQ is NULL");

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setFrameDoneQ(frame_queue_t *frameDoneQ)
{
    CLOGV("");

    m_frameDoneQ = frameDoneQ;
    m_flagFrameDoneQ = true;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getFrameDoneQ(frame_queue_t **frameDoneQ)
{
    *frameDoneQ = m_frameDoneQ;

    if (*frameDoneQ == NULL)
        CLOGE("frameDoneQ is NULL");

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setNodeInfos(camera_node_objects_t *nodeObjects, bool flagReset)
{
    CLOGD("setNodeInfos flagReset(%s)",
             (flagReset) ? "True" : "False");

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        m_node[i] = nodeObjects->node[i];

        if (flagReset == true) {
            if (m_node[i] != NULL)
                m_node[i]->resetInput();
        }
    }

    if (flagReset == true) {
        m_frameDoneQ = NULL;
        m_flagFrameDoneQ = false;
        m_outputFrameQ = NULL;
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getNodeInfos(camera_node_objects_t *nodeObjects)
{
    CLOGD("getNodeInfos");

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        nodeObjects->node[i] = m_node[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setMapBuffer(__unused ExynosCameraBuffer *srcBuf, __unused ExynosCameraBuffer *dstBuf)
{
    status_t ret = NO_ERROR;

    /* Output Node */
    for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
        if (m_node[i] != NULL &&
                m_bufferManager[i] != NULL)
            ret |= m_setMapBuffer(i);
    }

    /* Capture Node */
    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        if (m_deviceInfo->connectionMode[i] == HW_CONNECTION_MODE_M2M_BUFFER_HIDING
            && m_node[i] != NULL
            && m_bufferManager[i] != NULL)
            ret |= m_setMapBuffer(i);
    }

    return ret;
}

status_t ExynosCameraMCPipe::setBoosting(bool isBoosting)
{
    CLOGD("");

    m_isBoosting = isBoosting;

    return NO_ERROR;
}

bool ExynosCameraMCPipe::isThreadRunning(void)
{
    if (m_putBufferThread->isRunning() || m_getBufferThread->isRunning())
        return true;

    return false;
}

status_t ExynosCameraMCPipe::getThreadState(int **threadState)
{
    *threadState = &m_threadState;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getThreadInterval(uint64_t **timeInterval)
{
    *timeInterval = &m_timeInterval;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getThreadRenew(int **timeRenew)
{
    *timeRenew = &m_threadRenew;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::incThreadRenew(void)
{
    m_threadRenew ++;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::resetThreadRenew(void)
{
    m_threadRenew = 0;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setStopFlag(void)
{
    CLOGD("");

    m_flagTryStop = true;

    return NO_ERROR;
}

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
status_t ExynosCameraMCPipe::setHandler(sp<PipeHandler> handler)
{
    status_t ret = NO_ERROR;
    m_handler[handler->getScenario()] = handler;
    return ret;
}

status_t ExynosCameraMCPipe::getHandler(PIPE_HANDLER::USAGE usage, sp<PipeHandler> &handler)
{
    status_t ret = INVALID_OPERATION;
    bool funcRet = false;
    for(int i = 0 ; i < PIPE_HANDLER::SCENARIO_MAX; i++) {
        if (m_handler[i] != NULL) {
            funcRet = m_handler[i]->isUsage(usage);
            if (funcRet == true) {
                ret = NO_ERROR;
                handler = m_handler[i];
            }
        }
    }
    return ret;
}

status_t ExynosCameraMCPipe::getHandler(PIPE_HANDLER::SCENARIO scenario, sp<PipeHandler> &handler)
{
    status_t ret = INVALID_OPERATION;
    if (m_handler[scenario] != NULL) {
        handler = m_handler[scenario];
        ret = NO_ERROR;
    }
    return ret;
}

status_t ExynosCameraMCPipe::getNodes(ExynosCameraNode **node[MAX_NODE])
{
    status_t ret = NO_ERROR;

    *node = m_node;

    return ret;
}

status_t ExynosCameraMCPipe::setShreadNode(int32_t src, int32_t dst)
{
    status_t ret = NO_ERROR;
    if (m_node[src] == NULL) {
        CLOGE("m_node src(%d) failed, dst(%d)", src, dst);
        ret = INVALID_OPERATION;
    } else {
        int srcFd = -1, dstFd = -1;
        m_node[dst] = m_node[src];
        m_sharedNode[dst] = true;

        m_node[src]->getFd(&srcFd);
        m_node[dst]->getFd(&dstFd);
        CLOGD("m_node[%d]=fd(%d) and m_node[%d]=fd(%d) is shared", src, srcFd, dst, dstFd);
    }
    return ret;
}
#endif

int ExynosCameraMCPipe::getRunningFrameCount(void)
{
    int runningFrameCount = 0;

    for (uint32_t i = 0; i < m_numBuffers[OUTPUT_NODE]; i++) {
        if (m_runningFrameList[OUTPUT_NODE][i] != NULL) {
            runningFrameCount++;
        }
    }

    return runningFrameCount;
}

#ifdef USE_MCPIPE_SERIALIZATION_MODE
void ExynosCameraMCPipe::needSerialization(bool enable)
{
    CLOGI("%s serialized operation %s",
             m_name,
            (enable == true)? "enabled" : "disabled");

    m_serializeOperation = enable;
}
#endif

void ExynosCameraMCPipe::dump(void)
{
    m_dumpRunningFrameList();

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            m_node[i]->dump();
        }

        if (m_bufferManager[i] != NULL) {
            m_bufferManager[i]->dump();
        }
    }

    return;
}

status_t ExynosCameraMCPipe::dumpFimcIsInfo(bool bugOn)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    ret = m_node[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_DUMP, bugOn);
    if (ret != NO_ERROR)
        CLOGE("m_node[OUTPUT_NODE]->setControl failed");

    return ret;
}

//#ifdef MONITOR_LOG_SYNC
status_t ExynosCameraMCPipe::syncLog(uint32_t syncId)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    ret = m_node[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_SYNC_LOG, syncId);
    if (ret != NO_ERROR)
        CLOGE("m_node[OUTPUT_NODE]->setControl failed");

    return ret;
}
//#endif

status_t ExynosCameraMCPipe::m_preCreate(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    ExynosCameraNode *jpegNode = NULL;

    /* Create & open output/capture nodes */
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {

        // sensor's open happen in m_createSensorNode, for performance
        if (i == m_sensorNodeIndex) {
            continue;
        }

#ifdef SUPPORT_DEPTH_MAP
        if (i == m_depthVcNodeIndex) {
            continue;
        }
#endif // SUPPORT_DEPTH_MAP

        void *module = NULL;
        enum EXYNOS_CAMERA_NODE_LOCATION location = NODE_LOCATION_DST;
        int pipeId = getPipeId((enum NODE_TYPE)i);
        int vId = m_deviceInfo->nodeNum[i];
        if (m_flagValidInt(vId) == true) {
            m_nodeIdToPipeIdMap[i] = pipeId;
            m_videoIdToPipeIdMap[vId - FIMC_IS_VIDEO_BAS_NUM] = pipeId;
            CLOGD("[%d][%s] vId:%d, pipeId:%d", i, m_deviceInfo->nodeName[i], vId, pipeId);

            switch (vId) {
            case FIMC_IS_VIDEO_HWFC_JPEG_NUM:
            case FIMC_IS_VIDEO_HWFC_THUMB_NUM:
                if (m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_SRC_REPROCESSING
                    || m_deviceInfo->pipeId[i] == PIPE_HWFC_THUMB_SRC_REPROCESSING) {
                    location = NODE_LOCATION_SRC;
                }

                /* JpegHAL Node case */
                m_node[i] = (ExynosCameraNode*)new ExynosCameraNodeJpegHAL();

                if (jpegNode == NULL) {
                    jpegNode = m_node[i];
                } else {
                    ret = jpegNode->getInternalModule(&module);
                    if (ret != NO_ERROR) {
                        CLOGE("jpegNode->getInternalModule failed");
                        return ret;
                    }
                }

                ret = m_node[i]->create(m_deviceInfo->nodeName[i], m_cameraId, location, module);
                if (ret != NO_ERROR) {
                    CLOGE("Create node fail(Node:%s), ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->open(vId, (int)m_parameters->isUseThumbnailHWFC());
                if (ret != NO_ERROR) {
                    CLOGE("Open node fail(Node:%s), ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGD("JpegHAL Node(%d) opened", vId);

                break;
            default:
                /* Normal case */
                m_node[i] = new ExynosCameraNode();

                ret = m_node[i]->create(m_deviceInfo->nodeName[i], m_cameraId);
                if (ret != NO_ERROR) {
                    CLOGE("Create node fail(Node:%s), ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->open(vId);
                if (ret != NO_ERROR) {
                    CLOGE("Open node fail(Node:%s), ret(%d)",
                             m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGV("Node(%d) opened", vId);

                break;
            }
        }
    }

    m_putBufferThreadName.clear();
    m_getBufferThreadName.clear();
    m_putBufferThreadName.appendFormat("putBuf%d-%d", getPipeId(), m_cameraId);
    m_getBufferThreadName.appendFormat("getBuf%d-%d", getPipeId(), m_cameraId);

    m_putBufferThread = new MCPipeThread(this,
            &ExynosCameraMCPipe::m_putBufferThreadFunc, m_putBufferThreadName.c_str(), PRIORITY_URGENT_DISPLAY);
    m_getBufferThread = new MCPipeThread(this,
            &ExynosCameraMCPipe::m_getBufferThreadFunc, m_getBufferThreadName.c_str(), PRIORITY_URGENT_DISPLAY);
#ifdef DEBUG_DUMP_IMAGE
    m_dumpBufferThread = new MCPipeThread(this,
            &ExynosCameraMCPipe::m_dumpBufferThreadFunc, "dumpBufferThread", PRIORITY_URGENT_DISPLAY);
#endif

    if (m_reprocessing == true) {
        m_inputFrameQ = new frame_queue_t(m_putBufferThread);
        m_requestFrameQ = new frame_queue_t(m_getBufferThread);
    } else {
        m_inputFrameQ = new frame_queue_t;
        m_requestFrameQ = new frame_queue_t;
    }

#ifdef DEBUG_DUMP_IMAGE
    m_dumpImageQ = new ExynosCameraList<ExynosCameraImageDumpInfo_t>();
    m_dumpImageQ->setWaitTime(5500000000);      /* 5.5 sec */
    m_RawFrameHoldList.setWaitTime(5500000000);
#endif

    /* Set wait time 0.55 sec. Because, it support 2fps */
    m_inputFrameQ->setWaitTime(550000000);      /* .55 sec */
    m_requestFrameQ->setWaitTime(550000000);    /* .55 sec */

    CLOGI("m_preCreate() is succeed, Pipe(%d), prepare(%d)",
             getPipeId(), m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::m_postCreate(int32_t *sensorIds)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (sensorIds != NULL) {
            CLOGD("Set new sensorIds[%d] : %d", i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }
    }

    ret = m_setInput(m_node, m_deviceInfo->nodeNum, m_sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("m_setInput(Main) fail, ret(%d)", ret);
        return ret;
    }

    CLOGI("m_postCreate() is succeed, Pipe(%d)", getPipeId());

    return ret;
}

bool ExynosCameraMCPipe::m_putBufferThreadFunc(void)
{
    status_t ret = NO_ERROR;

#ifdef TEST_WATCHDOG_THREAD
    testErrorDetect++;
    if (testErrorDetect == 100)
        m_threadState = ERROR_POLLING_DETECTED;
#endif

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_putBuffer();
    if (ret != NO_ERROR) {
        if ((!m_standbyHintFlag) || (ret != TIMED_OUT)) {
            CLOGW("m_putbuffer fail, ret(%d)", ret);
        }
    }

    return m_checkThreadLoop(m_inputFrameQ);
}

bool ExynosCameraMCPipe::m_getBufferThreadFunc(void)
{
    status_t ret = NO_ERROR;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_getBuffer();
    if (ret != NO_ERROR && m_putInternalFrameLogCnt == 0
#ifdef USE_DUAL_CAMERA
        && ret != BAD_TYPE
#endif
    ) {
        if ((!m_standbyHintFlag) || (ret != TIMED_OUT)) {
            CLOGW("m_getBuffer fail, ret(%d)", ret);
        }
    }

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    /*
     * update renew count
     * The m_threadRenew is the count checked the getBuffer(DQbuffer) block.
     * Therefore, the time out of the getBuffer thread is unconnected with the getBuffer(DQbuffer) block.
     */
    if (ret >= 0 || ret == TIMED_OUT
#ifdef USE_DUAL_CAMERA
        || ret == BAD_TYPE
#endif
        ) {
        m_threadRenew = 0;
    }

    {
        Mutex::Autolock lock(m_sensorStandbyLock);

        if (m_flagSensorStandby == SENSOR_STANDBY_ON_READY
                && m_flagStartPipe == true
                && m_inputFrameQ->getSizeOfProcessQ() == 0
                && m_requestFrameQ->getSizeOfProcessQ() == 0) {
            //wait until the current m_putBuffer function is done.
            Mutex::Autolock lock(m_putBufferLock);
            ret = m_sensorStandby(true);
            if (ret != NO_ERROR) {
                CLOGE("Sensor standby(%s) fail! ret(%d)",
                        (m_flagSensorStandby?"On":"Off"), ret);
            }

            m_flagSensorStandby = SENSOR_STANDBY_ON;
        }
    }

    return m_checkThreadLoop(m_requestFrameQ);
}

status_t ExynosCameraMCPipe::m_putBuffer(void)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraBuffer buffer[OTF_NODE_BASE];
    int pipeId = 0;
    int bufferIndex[OTF_NODE_BASE];
    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++)
        bufferIndex[i] = -2;
    uint32_t captureNodeCount = 0;
    ExynosCameraDurationTimer blockingTimer[4];

    /* 1. Pop from input frame queue */
    blockingTimer[0].start();
    blockingTimer[1].start();
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret == TIMED_OUT) {
        if (!m_standbyHintFlag) {
            CLOGW("inputFrameQ wait timeout");
        }
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("inputFrameQ wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }
    blockingTimer[1].stop();

    if (newFrame == NULL) {
        CLOGE("New frame is NULL");
        return BAD_VALUE;
    }

    if ((m_skipBufferHint == true) && (m_standbyHintFlag == true)) {
        CLOGE("[F%d]:  m_flagSensorStandby = %d : FrameQ_size (%d, %d) skipping the frame",
                        newFrame->getFrameCount(), m_flagSensorStandby,
                        m_inputFrameQ->getSizeOfProcessQ(),
                        m_requestFrameQ->getSizeOfProcessQ());
        return m_frameErrorHandler(newFrame);
    }

    Mutex::Autolock lock(m_putBufferLock);

    if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
        || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

        ) {
        if ((m_putInternalFrameLogCnt++ % INTERNAL_FRAME_LOG_DURATION) == 0) {
            CLOGI("Internal Frame(%d), frameCount(%d), (%d)",
                    newFrame->getFrameType(), newFrame->getFrameCount(),
                    m_putInternalFrameLogCnt);
        }
    } else {
        m_putInternalFrameLogCnt = 0;
    }

    if (newFrame->getFrameState() == FRAME_STATE_SKIPPED
        || newFrame->getFrameState() == FRAME_STATE_INVALID) {
        if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
            && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
            && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

            ) {
            CLOGE("New frame is INVALID, frameCount(%d)",
                     newFrame->getFrameCount());
        }
        return m_frameErrorHandler(newFrame);
    }

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    {
        sp<PipeHandler> handler = NULL;
        status_t ret = getHandler(PIPE_HANDLER::USAGE_PRE_QBUF, handler);
        if (ret == NO_ERROR) {
            PIPE_HANDLER::QUERY status = PIPE_HANDLER::QUERY_BASE;
            handler->process(PIPE_HANDLER::USAGE_PRE_QBUF, newFrame, status);
        }
    }
#endif

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_lockSerializeOperation((enum pipeline)getPipeId());
#endif

    blockingTimer[2].start();
    for (int i = (MAX_CAPTURE_NODE - 1); i >= CAPTURE_NODE; i--) {
        if (m_node[i] == NULL)
            continue;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("getPipeId(%d) fail", i);
            return BAD_VALUE;
        }

    /* 2. Get capture node buffer(DstBuffer) from buffer manager */
        if (m_node[i] != NULL
            && newFrame->getRequest(pipeId) == true
            && m_skipPutBuffer[i] == false) {
            if (m_bufferSupplier == NULL) {
                CLOGE("[F%d]BufferSupplier is NULL, i(%d), piepId(%d)",
                        newFrame->getFrameCount(), i, pipeId);
                continue;
            }

            ret = newFrame->getDstBuffer(getPipeId(), &buffer[i], i);
            if (ret != NO_ERROR) {
                CLOGE("getDstBuffer fail. pipeId(%d), frameCount(%d), ret(%d)",
                         pipeId, newFrame->getFrameCount(), ret);
                continue;
            }

            if (buffer[i].index < 0) {
                buffer_manager_tag_t bufTag;
                bufTag.pipeId[0] = pipeId;
                bufTag.managerType = m_deviceInfo->bufferManagerType[i];
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
                if (bufTag.managerType == BUFFER_MANAGER_ION_TYPE) {
                    switch (newFrame->getFrameType()) {
                    case FRAME_TYPE_INTERNAL_SENSOR_TRANSITION:
                    case FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION:
                        bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ION_TYPE;
                        break;
                    default:
                        break;
                    }
                }
#endif
                ret = m_bufferSupplier->getBuffer(bufTag, &(buffer[i]));
                if (ret != NO_ERROR) {
                    CLOGE("[%s][F%d]Failed to getBuffer from BufferSupplier. ret %d",
                            m_deviceInfo->nodeName[i], newFrame->getFrameCount(), ret);
                    newFrame->dump();
                    newFrame->setRequest(pipeId, false);
                    if (newFrame->getInternalBufTagPipeId() >= 0
                        && bufTag.managerType == BUFFER_MANAGER_ION_TYPE
                        && newFrame->getInternalBufTagPipeId() == bufTag.pipeId[0]) {
                        newFrame->storeInternalBufTagPipeId(-1);
                    }
                    continue;
                }

                bufferIndex[i] = buffer[i].index;
            } else {
                CLOGV("Skip to get buffer from bufferMgr.\
                        pipeId(%d), frameCount(%d) bufferIndex %d)",
                        pipeId, newFrame->getFrameCount(), buffer[i].index);
                bufferIndex[i] = buffer[i].index;
            }

            if (bufferIndex[i] < 0
                || m_runningFrameList[i][(bufferIndex[i])] != NULL) {
                CLOGE("%d's New buffer is invalid, we already get buffer, index(%d), frameCount(%d)",
                        i, bufferIndex[i], newFrame->getFrameCount());
                newFrame->setRequest(pipeId, false);
                /* dump(); */
                continue;
            }

    /* 3. Put capture buffer(DstBuffer) to node */
            if (bufferIndex[i] >= 0
                && newFrame->getRequest(pipeId) == true) {
                switch(m_deviceInfo->pipeId[i]) {
                case PIPE_HWFC_JPEG_SRC_REPROCESSING:
                case PIPE_HWFC_JPEG_DST_REPROCESSING:
                case PIPE_HWFC_THUMB_SRC_REPROCESSING:
                    ret = m_setJpegInfo(i, newFrame);
                    if (ret != NO_ERROR) {
                        CLOGE("Failed to setJpegInfo, pipeId %s buffer.index %d",
                                (m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_SRC_REPROCESSING)?
                                "PIPE_HWFC_JPEG_SRC_REPROCESSING":"PIPE_HWFC_THUMB_SRC_REPROCESSING",
                                buffer[i].index);
                        continue;
                    }
                    break;
                default:
                    break;
                }

                CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(),
                        "CAPTURE[P%d/%s] buffer:%d, %s",
                        getPipeId((enum NODE_TYPE)i),
                        m_deviceInfo->nodeName[i],
                        buffer[i].index, buffer[i].bufMgrNm);

                ret = m_node[i]->putBuffer(&(buffer[i]));
                if (ret != NO_ERROR) {
                    CLOGE("node(%s)->putBuffer() fail, frameCount(%d), ret(%d)",
                             m_deviceInfo->nodeName[i], newFrame->getFrameCount(), ret);

                    /* TODO: doing exception handling */
                    if (m_bufferSupplier != NULL) {
                        ret = m_bufferSupplier->putBuffer(buffer[i]);
                        if (ret != NO_ERROR) {
                            CLOGE("[%s][F%d B%d]Failed to putBuffer. ret %d",
                                    m_deviceInfo->nodeName[i],
                                    newFrame->getFrameCount(),
                                    buffer[i].index,
                                    ret);
                        }
                    }

                    newFrame->setRequest(pipeId, false);
                } else {
                    m_skipPutBuffer[i] = true;
                    m_skipBuffer[i] = buffer[i];
                }
            }
        } else if (m_skipPutBuffer[i] == true) {
            CLOGD("%s:Skip putBuffer. framecount %d bufferIndex %d",
                    m_deviceInfo->nodeName[i], newFrame->getFrameCount(), m_skipBuffer[i].index);
        }

        if (m_skipPutBuffer[i] == true)
            captureNodeCount++;
    }
    blockingTimer[2].stop();

    if (captureNodeCount == 0) {
        if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
            && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SLAVE
            && newFrame->getFrameType() != FRAME_TYPE_TRANSITION
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
            && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

            ) {
            CLOGW("Capture node putbuffer is Zero, frameCount(%d)",
                    newFrame->getFrameCount());
            /* Comment out: 3AA and ISP must running, because it save stat and refered next frame.
             *              So, put SRC buffer to output node when DST buffers all empty(zero).
             */
            /* goto CLEAN_FRAME; */
        }
    }

    TIME_LOGGER_UPDATE(m_cameraId, newFrame->getRequestKey(), getPipeId(), INTERVAL, QBUF, newFrame->getFrameCount());

    /* 4. Get output node(SrcBuffer) buffer from frame */
    blockingTimer[3].start();
    for (int i = (MAX_OUTPUT_NODE - 1); i >= OUTPUT_NODE; i--) {
        if (m_node[i] == NULL)
            continue;

        ret = newFrame->getSrcBuffer(getPipeId(), &(buffer[i]), i);
        if (ret != NO_ERROR || buffer[i].index < 0) {
            CLOGE("Frame get src(%d) buffer(index %d) fail, frameCount(%d), ret(%d)",
                     i, buffer[i].index, newFrame->getFrameCount(), ret);
            /* TODO: doing exception handling */
            return m_frameErrorHandler(newFrame);
        }

        if (i == OUTPUT_NODE) {
            /* Output Node(Can be group video node) */
            if (m_runningFrameList[i][(buffer[i].index)] != NULL) {
                if ( (m_parameters->isReprocessing() == true)
                        && (m_parameters->getUsePureBayerReprocessing() == false)  /* if Dirty bayer reprocessing */
                        && (newFrame->getFrameType() == FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
                            || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
                            || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

                            )
                        && (getPipeId() == PIPE_ISP) ) {                /* if internal frame at ISP pipe */
                    /* In dirty bayer mode, Internal frame would not have valid
                       output buffer on ISP pipe. So suppress error message */
                    CLOGI("Internal frame will raises an error here, but it's normal operation, index(%d), frameCount(%d)",
                            buffer[i].index, newFrame->getFrameCount());
                } else {
                    CLOGE("New buffer is invalid, we already get buffer, index(%d), frameCount(%d)",
                            buffer[i].index, newFrame->getFrameCount());
                }
                /* dump(); */
                return m_frameErrorHandler(newFrame);
            }

            /* 5. Update control metadata for request, Zoom, ... */
            ret = m_updateMetadataFromFrame(newFrame, &(buffer[i]), m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex);
            if (ret != NO_ERROR) {
                CLOGW("Update metadata fail, frameCount(%d), ret(%d)",
                        newFrame->getFrameCount(), ret);
            }
        }

        CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(),
                "OUTPUT[P%d/%s] buffer:%d, %s",
                getPipeId((enum NODE_TYPE)i),
                m_deviceInfo->nodeName[i],
                buffer[i].index, buffer[i].bufMgrNm);

        /* 6. Put output buffer(SrcBuffer) to node */
        ret = m_node[i]->putBuffer(&(buffer[i]));
        if (ret != NO_ERROR) {
            CFLOGE(newFrame, "node(%s/%d)->putBuffer(buf:%s, idx:%d) fail, ret(%d)",
                    m_deviceInfo->nodeName[i], getPipeId((enum NODE_TYPE)i), buffer[i].bufMgrNm, buffer[i].index, ret);
            /* TODO: doing exception handling */
            return m_frameErrorHandler(newFrame);
        }

        ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING, i);
        if (ret != NO_ERROR) {
            CLOGE("setSrcBuffer(%d) state fail, frameCount(%d), ret(%d)",
                    i, newFrame->getFrameCount(), ret);
        }

        m_runningFrameList[i][(buffer[i].index)] = newFrame;
        m_numOfRunningFrame[i]++;
    }

    /* 7. Link capture node buffer(DstBuffer) to frame */
    for (int i = (MAX_CAPTURE_NODE - 1); i >= CAPTURE_NODE; i--) {
        if (m_node[i] == NULL)
            continue;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("getPipeId(%d) fail", i);
            return BAD_VALUE;
        }


        if (m_node[i] != NULL
                && newFrame->getRequest(pipeId) == true) {
            /* HACK: Should change ExynosCamera, Frame */
            ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
            if (ret != NO_ERROR) {
                CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                        pipeId, newFrame->getFrameCount(), ret);
            }

            if (m_skipPutBuffer[i] == true) {
                buffer[i] = m_skipBuffer[i];
                bufferIndex[i] = buffer[i].index;
            }

            ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i, INDEX(pipeId));
            if (ret != NO_ERROR) {
                CLOGE("Frame set dst buffer fail, frameCount(%d), ret(%d)",
                        newFrame->getFrameCount(), ret);
                /* TODO: doing exception handling */
                if (m_bufferManager[i] != NULL)
                    ret = m_bufferManager[i]->putBuffer(buffer[i].index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

                if (ret != NO_ERROR) {
                    CLOGE("Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                            i, newFrame->getFrameCount(), ret);
                }

                newFrame->setRequest(pipeId, false);
            }

            ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING, i);
            if (ret != NO_ERROR) {
                CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                        getPipeId(), newFrame->getFrameCount(), ret);
            }

            m_runningFrameList[i][(bufferIndex[i])] = newFrame;
            m_numOfRunningFrame[i]++;
            m_skipPutBuffer[i] = false;
            m_skipBuffer[i].index = -2;
        }
    }
    blockingTimer[3].stop();
    blockingTimer[0].stop();

    /* 8. Push frame to getBufferThread */
    m_requestFrameQ->pushProcessQ(&newFrame);

    if ((int) blockingTimer[0].durationMsecs() > 1000) { /* Over 1 sec */
        CLOGW("[F%d]putBuffer is delayed!! total %d waitInputFrameQ %d captureQ %d OutputQ %d",
                newFrame->getFrameCount(),
                (int) blockingTimer[0].durationMsecs(),
                (int) blockingTimer[1].durationMsecs(),
                (int) blockingTimer[2].durationMsecs(),
                (int) blockingTimer[3].durationMsecs());
    }

    CLOGV("OUT-");
    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_frameErrorHandler(ExynosCameraFrameSP_sptr_t newFrame)
{
    status_t ret = NO_ERROR;

    if (newFrame == NULL) {
        CLOGE("New frame is NULL");
        return BAD_VALUE;
    }

    if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
        && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

        ) {
        CLOGD("clean frame, frameCount(%d)",
                newFrame->getFrameCount());
    }

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_unlockSerializeOperation((enum pipeline)getPipeId());
#endif

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    {
        sp<PipeHandler> handler = NULL;
        status_t ret = getHandler(PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ, handler);
        if (ret == NO_ERROR) {
            PIPE_HANDLER::QUERY status = PIPE_HANDLER::QUERY_BASE;
            handler->process(PIPE_HANDLER::USAGE_POST_QBUF_ERR, newFrame, status);
        }
    }
#endif

    for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
        if (m_node[i] == NULL)
            continue;

        ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
        if (ret != NO_ERROR) {
            CLOGE("%d's setSrcBuffer state fail, frameCount(%d), ret(%d)",
                    i, newFrame->getFrameCount(), ret);
        }
    }

    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
    if (ret != NO_ERROR) {
        CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                 getPipeId(), newFrame->getFrameCount(), ret);
    }

    for (int i = (MAX_CAPTURE_NODE - 1); i >= CAPTURE_NODE; i--) {
        if (m_node[i] != NULL
            && newFrame->getRequest(getPipeId((enum NODE_TYPE)i)) == true)
            newFrame->setRequest(getPipeId((enum NODE_TYPE)i), false);
    }

    if (newFrame->getFrameState() != FRAME_STATE_SKIPPED
        && newFrame->getFrameState() != FRAME_STATE_INVALID) {
        newFrame->setFrameState(FRAME_STATE_SKIPPED);
        if (ret != NO_ERROR) {
            CLOGE("setFrameState fail, frameCount(%d), ret(%d)",
                     newFrame->getFrameCount(), ret);
        }
    }

    ret = m_completeFrame(newFrame, false);
    if (ret != NO_ERROR) {
        CLOGE("Complete frame fail, frameCount(%d), ret(%d)", newFrame->getFrameCount(), ret);
        /* TODO: doing exception handling */
    }

    if (m_frameDoneQ != NULL && m_flagFrameDoneQ == true)
        m_frameDoneQ->pushProcessQ(&newFrame);

    m_outputFrameQ->pushProcessQ(&newFrame);

    if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
        && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
        && newFrame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

        ) {
        return INVALID_OPERATION;
    } else {
        return ret;
    }
}

status_t ExynosCameraMCPipe::m_getBuffer(void)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;
    status_t nodeDqRet[OTF_NODE_BASE];
    status_t checkRet = NO_ERROR;

    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraBuffer buffer[OTF_NODE_BASE];
    int pipeId = 0;
    int v4l2Colorformat = 0;
    int planeCount[OTF_NODE_BASE] = {0};
    int bufferIndex[OTF_NODE_BASE];
    uint32_t captureNodeFrameCount[OTF_NODE_BASE];
    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++) {
        bufferIndex[i] = -2;
        captureNodeFrameCount[i] = -1;
        nodeDqRet[i] = NO_ERROR;
    }
    uint32_t captureNodeCount = 0;
    uint32_t checkPollingCount = 0;
    bool skipOutputQ = false;

    /* 1. Pop from request frame queue */
    ret = m_requestFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret == TIMED_OUT) {
        if (!m_standbyHintFlag) {
            if (m_putInternalFrameLogCnt == 0) {
                CLOGW("requestFrameQ wait timeout");
            }
        }
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("requestFrameQ wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("New frame is NULL");
        return BAD_VALUE;
    }

    /* 2. Get output buffer(SrcBuffer) from node */
    for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
        if (m_node[i] == NULL)
            continue;

        ret = m_node[i]->getBuffer(&(buffer[i]), &(bufferIndex[i]));
        nodeDqRet[i] = ret;
        if (ret != NO_ERROR) {
            CLOGE("node(%s)->getBuffer() fail, index(%d), frameCount(%d), ret(%d)",
                     m_deviceInfo->nodeName[i],
                    bufferIndex[i], newFrame->getFrameCount(), ret);

            if (buffer[i].index < 0 && bufferIndex[i] >= 0) {
                CLOGW("mismatching bufferIndex(%d) vs buffer[%d].index(%d)",
                        bufferIndex[i], i, buffer[i].index);
                bufferIndex[i] = buffer[i].index;
            }

            /* TODO: doing exception handling */
            /* Comment out : dqblock case was disappeared */
            /*
            for (int i = (MAX_NODE - 1); i > OUTPUT_NODE; i--) {
                if (newFrame->getRequest(getPipeId() + i) == true)
                    m_skipPutBuffer[i] = true;
            }

            ret = m_completeFrame(newFrame, false);
            if (ret != NO_ERROR) {
                CLOGE("Complete frame fail, ret(%d)", ret);
            }

            goto CLEAN;
            */

            if (bufferIndex[i] >= 0) {
                newFrame = m_runningFrameList[i][bufferIndex[i]];
            } else {
                ret = newFrame->getSrcBuffer(getPipeId(), &buffer[i], i);
                if (ret != NO_ERROR) {
                    CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                             i, newFrame->getFrameCount(), ret);
                } else {
                    buffer[i].index = -2;
                    ret = newFrame->setSrcBuffer(getPipeId(), buffer[i], i);
                    if (ret != NO_ERROR) {
                        CLOGE("%d's setSrcBuffer fail, fCount(%d), pipe(%d), ret(%d)",
                                i, newFrame->getFrameCount(), getPipeId(), ret);
                    }
                }
            }

            if (newFrame == NULL) {
                CLOGE("%d's Invalid DQ buffer index(%d)", i, bufferIndex[i]);
                ret = BAD_VALUE;
                goto EXIT;
            }

            ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
            if (ret != NO_ERROR) {
                CLOGE("%d's setSrcBuffer state fail, frameCount(%d), ret(%d)",
                         i, newFrame->getFrameCount(), ret);
            }
        } else {
            if (bufferIndex[i] < 0) {
                CLOGE("%d's Invalid DQ buffer index(%d)", i, bufferIndex[i]);
                ret = BAD_VALUE;
                goto EXIT;
            }

            /*
                prevent the frame null pointer exception, get the frame from m_runningFrameList
                1. in case of NDONE,  dq order was reversed.
                2. always use the m_runningFrameList instead of m_requestFrameQ
            */
            newFrame = m_runningFrameList[i][bufferIndex[i]];
            ret = newFrame->getSrcBuffer(getPipeId(), &buffer[i], i);
            if (ret != NO_ERROR) {
                CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                         i, newFrame->getFrameCount(), ret);
            } else {
                if (bufferIndex[i] != buffer[i].index) {
                    ret = newFrame->getSrcBuffer(getPipeId(), &buffer[i], i);
                    if (ret != NO_ERROR) {
                        CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                                 i, newFrame->getFrameCount(), ret);
                    }
                }

                ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE, i);
                if (ret != NO_ERROR) {
                    CLOGE("%d's setSrcBuffer state fail, frameCount(%d), ret(%d)",
                             i, newFrame->getFrameCount(), ret);
                }

                if (i == OUTPUT_NODE && m_reprocessing == false)
                    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&buffer[OUTPUT_NODE]);
            }
        }

        TIME_LOGGER_UPDATE(m_cameraId, newFrame->getRequestKey(), getPipeId(), INTERVAL, DQBUF, newFrame->getFrameCount());

        if (bufferIndex[i] >= 0) {
    /* 3. Update frame from dynamic metadata of output node buffer(SrcBuffer) for request, ... */
            if (i == OUTPUT_NODE) {
                ret = m_updateMetadataToFrame(buffer[i].addr[buffer[i].getMetaPlaneIndex()],
                                              buffer[i].index, newFrame, (enum NODE_TYPE)i);
                if (ret != NO_ERROR) {
                    CLOGE("%d's Update metadata fail, frameCount(%d), ret(%d)",
                            i, newFrame->getFrameCount(), ret);
                }
            }

            Mutex::Autolock lock(m_runningFrameDumpLock);
            m_runningFrameList[i][bufferIndex[i]] = NULL;
            m_numOfRunningFrame[i]--;
        }

        CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(),
                "OUTPUT[P%d/%s] buffer:%d, %s",
                getPipeId((enum NODE_TYPE)i),
                m_deviceInfo->nodeName[i],
                buffer[i].index, buffer[i].bufMgrNm);
    }

    if (m_parameters->isUseEarlyFrameReturn() == true
        && m_reprocessing == false
        && m_frameDoneQ != NULL\
        && m_flagFrameDoneQ == true) {
        m_frameDoneQ->pushProcessQ(&newFrame);
    }

    /* 4. Get capture buffer(DstBuffer) from node */
    for (int i = (MAX_CAPTURE_NODE - 1); i >= CAPTURE_NODE; i--) {
        ret = NO_ERROR;

        if (m_node[i] == NULL)
            continue;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("getPipeId(%d) fail", i);
            ret = BAD_VALUE;
            goto EXIT;
        }

        if (m_node[i] != NULL
            && newFrame->getRequest(pipeId) == true) {
#ifndef SKIP_SCHECK_POLLING
            if ((m_parameters->isPreviewPortId(m_parameters->getYuvOutPortId((enum pipeline)INDEX(pipeId))) == true)
                && (checkPollingCount == 0)) {
                ret = m_checkPolling(m_node[i]);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d]m_checkPolling fail. ret %d",
                            newFrame->getFrameCount(), ret);
                    /* TODO: doing exception handling */
                    // HACK: for panorama shot
                    //return false;
                }
                checkPollingCount++;
            }
#endif
            ret = m_node[i]->getBuffer(&(buffer[i]), &(bufferIndex[i]));
            nodeDqRet[i] = ret;

#ifdef USE_MCPIPE_SERIALIZATION_MODE
            m_unlockSerializeOperation((enum pipeline)pipeId);
#endif

            CLOG_PERFRAME(PATH, m_cameraId, m_name, newFrame.get(), nullptr, newFrame->getRequestKey(),
                    "CAPTURE[P%d/%s] buffer:%d, %s",
                    getPipeId((enum NODE_TYPE)i),
                    m_deviceInfo->nodeName[i],
                    buffer[i].index, buffer[i].bufMgrNm);

            captureNodeFrameCount[i] = (bufferIndex[i] >= 0) ?
                    (m_runningFrameList[i][bufferIndex[i]]->getFrameCount()) : (newFrame->getFrameCount());
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->getBuffer() fail, index(%d), frameCount(%d), ret(%d)",
                         m_deviceInfo->nodeName[i],
                        bufferIndex[i], newFrame->getFrameCount(), ret);
                /* TODO: doing exception handling */

                if (bufferIndex[i] >= 0) {
                    if (newFrame->getFrameCount() != captureNodeFrameCount[i]) {
                        CLOGW("output and capture node frame count do not match. (expect: %d != current: %d), pipeID(%d)",
                                newFrame->getFrameCount(), captureNodeFrameCount[i], pipeId);
                    } else {
                        newFrame = m_runningFrameList[i][bufferIndex[i]];
                    }
                } else {
                    ret = newFrame->getDstBuffer(getPipeId(), &buffer[i], i);
                    if (ret != NO_ERROR) {
                        CLOGE("Frame get buffer fail, frameCount(%d), ret(%d)",
                                 newFrame->getFrameCount(), ret);
                    } else {
                        bufferIndex[i] = buffer[i].index = -2;
                        ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i);
                        if (ret != NO_ERROR) {
                            CLOGE("%d's setDstBuffer fail, fCount(%d), pipe(%d), ret(%d)",
                                    i, newFrame->getFrameCount(), getPipeId(), ret);
                        }
                    }
                    newFrame->setRequest(pipeId, false);
                }

                if (newFrame == NULL) {
                    CLOGE("Invalid DQ buffer index(%d)", bufferIndex[i]);
                    ret = BAD_VALUE;
                    goto EXIT;
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
                if (ret != NO_ERROR) {
                    CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                             getPipeId(), newFrame->getFrameCount(), ret);
                }
            }

            if (bufferIndex[i] >= 0) {
                Mutex::Autolock lock(m_runningFrameDumpLock);
                m_runningFrameList[i][bufferIndex[i]] = NULL;
                m_numOfRunningFrame[i]--;
            }

    /* 5. Link capture node buffer(DstBuffer) to frame */
            if (bufferIndex[i] >= 0 && nodeDqRet[i] == NO_ERROR) {
                if (bufferIndex[i] != buffer[i].index) {
                    CLOGW("[F%d -> F%d] bufferIndex[i](%d) != buffer[i].index(%d)",
                             newFrame->getFrameCount(), m_runningFrameList[i][bufferIndex[i]]->getFrameCount(),
                             bufferIndex[i], buffer[i].index);
                    newFrame = m_runningFrameList[i][bufferIndex[i]];
                }

                /* HACK: Should change ExynosCamera, Frame */
                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
                if (ret != NO_ERROR) {
                    CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                             pipeId, newFrame->getFrameCount(), ret);
                }

                ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i, INDEX(pipeId));
                if (ret != NO_ERROR) {
                    CLOGE("Frame set dst buffer fail, frameCount(%d), ret(%d)",
                             newFrame->getFrameCount(), ret);
                    /* TODO: doing exception handling */
                    if (m_bufferManager[i] != NULL)
                        ret = m_bufferManager[i]->putBuffer(buffer[i].index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

                    if (ret != NO_ERROR) {
                        CLOGE("Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                                 i, newFrame->getFrameCount(), ret);
                    }

                    newFrame->setRequest(pipeId, false);
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE, i);
                if (ret != NO_ERROR) {
                    CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                             getPipeId(), newFrame->getFrameCount(), ret);
                }

                captureNodeCount++;

    /* 6. Update metadata of capture node buffer(DstBuffer) from output node buffer(SrcBuffer) */
                if (m_node[OUTPUT_NODE] != NULL
                    && m_deviceInfo->nodeNum[i] != FIMC_IS_VIDEO_HWFC_JPEG_NUM
                    && m_deviceInfo->nodeNum[i] != FIMC_IS_VIDEO_HWFC_THUMB_NUM) {
                    m_node[OUTPUT_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[OUTPUT_NODE]);
                    m_node[i]->getColorFormat(&v4l2Colorformat, &planeCount[i]);

                    camera2_shot_ext *shot_ext_src = (camera2_shot_ext *)buffer[OUTPUT_NODE].addr[buffer[OUTPUT_NODE].getMetaPlaneIndex()];
                    camera2_shot_ext *shot_ext_dst = (camera2_shot_ext *)buffer[i].addr[buffer[i].getMetaPlaneIndex()];
                    if (shot_ext_src != NULL && shot_ext_dst != NULL) {
                        memcpy(&shot_ext_dst->shot.ctl, &shot_ext_src->shot.ctl, sizeof(struct camera2_ctl) - sizeof(struct camera2_entry_ctl));
                        memcpy(&shot_ext_dst->shot.uctl, &shot_ext_src->shot.uctl, sizeof(struct camera2_uctl));
                        memcpy(&shot_ext_dst->shot.udm, &shot_ext_src->shot.udm, sizeof(struct camera2_udm));
                        memcpy(&shot_ext_dst->shot.dm, &shot_ext_src->shot.dm, sizeof(struct camera2_dm));

                        shot_ext_dst->setfile = shot_ext_src->setfile;
#if 0
                        shot_ext_dst->tnr_mode = shot_ext_src->tnr_mode;
#endif
                        shot_ext_dst->fd_bypass = shot_ext_src->fd_bypass;
                        shot_ext_dst->shot.dm.request.frameCount = shot_ext_src->shot.dm.request.frameCount;
                        shot_ext_dst->shot.magicNumber= shot_ext_src->shot.magicNumber;
                        shot_ext_dst->shot.uctl.isModeUd.wdr_mode = shot_ext_src->shot.uctl.isModeUd.wdr_mode;
                    } else {
                        CLOGE("metadata address fail, frameCount(%d) shot_ext src(%p) dst(%p) ",
                                 newFrame->getFrameCount(), shot_ext_src, shot_ext_dst);
                    }
                    /* Comment out : It was not useful for metadate fully update, I want know reasons for the existence. */
                    /* memcpy(buffer[i].addr[(planeCount[i] - 1)], buffer[OUTPUT_NODE].addr[(planeCount[OUTPUT_NODE] - 1)], sizeof(struct camera2_shot_ext)); */
                }
            }
        }
    }

    /*
     * skip condition :
     * 1 : all capture nodes are not valid.
     * 2 : one of capture nodes is not valid.
     */
    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++) {
        checkRet |= nodeDqRet[i];
    }

    if (captureNodeCount == 0 || checkRet != NO_ERROR) {
        if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL
#ifdef USE_DUAL_CAMERA
            || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SLAVE
            || newFrame->getFrameType() == FRAME_TYPE_TRANSITION
            || newFrame->getFrameType() == FRAME_TYPE_TRANSITION_SLAVE
#endif
#ifdef SUPPORT_SENSOR_MODE_CHANGE
            || newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION
#endif

            ) {
            if ((m_getInternalFrameLogCnt++ % INTERNAL_FRAME_LOG_DURATION) == 0) {
                CLOGI("InternalFrame(%d) frameCount(%d)\
                        : captureNodeCount == %d || checkRet(%d) != NO_ERROR.\
                        so, setFrameState(FRAME_STATE_SKIPPED), (%d)",
                        newFrame->getFrameType(), newFrame->getFrameCount(), captureNodeCount, checkRet, m_getInternalFrameLogCnt);
            }
        } else {
            CLOGE("frameCount(%d)\
                    : captureNodeCount == %d || checkRet(%d) != NO_ERROR.\
                    so, setFrameState(FRAME_STATE_SKIPPED)",
                 newFrame->getFrameCount(), captureNodeCount, checkRet);
        }

        /* set err on frame */
        newFrame->setFrameState(FRAME_STATE_SKIPPED);

        /* set err on src */
        for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
            if (m_node[i] == NULL)
                continue;

            ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
            if (ret != NO_ERROR) {
                CLOGE("%d's setSrcBufferState(%d, ENTITY_BUFFER_STATE_ERROR) fail, frameCount(%d), ret(%d)",
                        i, getPipeId(), newFrame->getFrameCount(), ret);
            }
        }

        /* set err on dst */
        for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
            int dstPipeId = getPipeId((enum NODE_TYPE)i);

            if (dstPipeId < 0)
                continue;

            if (newFrame->getRequest(dstPipeId) == true) {
                if (newFrame->getFrameCount() != captureNodeFrameCount[i]) {
                    if (captureNodeFrameCount[i] < 0) {
                        CLOGE("invalid captureNodeFrameCount, Pipe Id(%d), frameCount(%d)",
                            dstPipeId, newFrame->getFrameCount());

                        ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
                        if (ret != NO_ERROR) {
                            CLOGE("setDstBufferState(Pipe ID(%d), ENTITY_BUFFER_STATE_ERROR, %d) fail,"
                                    " frameCount(%d), ret(%d)",
                                    getPipeId(), i, newFrame->getFrameCount(), ret);
                        }
                        continue;
                    }

                    CLOGW("reset DstBuffer. frame for dst buffer(F%d) is different from frame of src buffer(F%d),"
                            " pipeId(%d), node(%d), bufferIndex(%d)",
                            captureNodeFrameCount[i], newFrame->getFrameCount(),
                            getPipeId(), i, bufferIndex[i]);

                    /* HACK: Should change ExynosCamera, Frame */
                    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
                    if (ret != NO_ERROR) {
                        CLOGE("setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                                getPipeId(), newFrame->getFrameCount(), ret);
                    }

                    ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i, INDEX(dstPipeId));
                    if (ret != NO_ERROR) {
                        CLOGE("Frame set dst buffer fail, frameCount(%d), ret(%d)",
                                newFrame->getFrameCount(), ret);
                        /* TODO: doing exception handling */

                        if (m_bufferManager[i] != NULL) {
                            ret = m_bufferManager[i]->putBuffer(buffer[i].index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                            if (ret != NO_ERROR) {
                                CLOGE("Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                                        i, newFrame->getFrameCount(), ret);
                            }
                        }

                        newFrame->setRequest(dstPipeId, false);
                    }
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
                if (ret != NO_ERROR) {
                    CLOGE("setDstBufferState(Pipe ID(%d),\
                            ENTITY_BUFFER_STATE_ERROR, %d) fail, \
                            frameCount(%d), ret(%d)",
                             getPipeId(), i, newFrame->getFrameCount(), ret);
                }
            }
        }
    } else {
        m_getInternalFrameLogCnt = 0;
#ifdef DEBUG_DUMP_IMAGE
        m_dumpBufferUtils(newFrame);
#endif
    }

    /* for debuging */
    m_lastFrameCount = newFrame->getFrameCount();
    m_lastMetaFrameCount = newFrame->getMetaFrameCount();

    /* 7. Complete frame */
    ret = m_completeFrame(newFrame);
    if (ret != NO_ERROR) {
        CLOGE("Complete frame fail, frameCount(%d), ret(%d)",
                 newFrame->getFrameCount(), ret);
        /* TODO: doing exception handling */
    }

    /* 8. Push frame to out of Pipe */
/* TODO: doing exception handling 
CLEAN:
*/

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
    {
        sp<PipeHandler> handler = NULL;
        status_t ret = getHandler(PIPE_HANDLER::USAGE_POST_DQBUF, handler);
        if (ret == NO_ERROR) {
            PIPE_HANDLER::QUERY status = PIPE_HANDLER::QUERY_BASE;
            handler->process(PIPE_HANDLER::USAGE_POST_DQBUF, newFrame, status);
            if (status == PIPE_HANDLER::QUERY_SKIP_OUTPUT)
                skipOutputQ = true;
        }
    }
#endif

    if (skipOutputQ == false) {
        m_outputFrameQ->pushProcessQ(&newFrame);
        if ((m_parameters->isUseEarlyFrameReturn() == false
            || m_reprocessing == true)
            && m_frameDoneQ != NULL
            && m_flagFrameDoneQ == true)
            m_frameDoneQ->pushProcessQ(&newFrame);
    }

    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++)
        ret |= nodeDqRet[i];

    CLOGV("OUT-");

EXIT:
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_unlockSerializeOperation((enum pipeline)getPipeId());
#endif

    return ret;
}

status_t ExynosCameraMCPipe::m_checkShotDone(struct camera2_shot_ext *shot_ext)
{
    CLOGD("");

    if (shot_ext == NULL) {
        CLOGE("shot_ext is NULL");
        return BAD_VALUE;
    }

    if (shot_ext->node_group.leader.request != 1) {
        CLOGW("3a1 NOT DONE, frameCount(%d)",
                getMetaDmRequestFrameCount(shot_ext));
        /* TODO: doing exception handling */
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCameraMCPipe::m_updateMetadataFromFrame(ExynosCameraFrameSP_sptr_t frame, ExynosCameraBuffer *buffer, int perframeInfoIndex)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    if (perframeInfoIndex < 0) {
        CLOGE("Invalid perframeInfoIndex(%d), fail!", perframeInfoIndex);
        return BAD_VALUE;
    }

    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buffer->addr[buffer->getMetaPlaneIndex()]);

    if (shot_ext != NULL) {
        int perframePosition = 0;
        ExynosRect sensorSize;
        ExynosRect bnsSize;
        ExynosRect previewBayerCropSize;
        ExynosRect pictureBayerCropSize;
        ExynosRect bdsSize;
        camera2_node_group node_group_info;
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
        camera2_virtual_node_group virtualnode_group_info;
#endif
        char captureNodeName[MAX_CAPTURE_NODE][EXYNOS_CAMERA_NAME_STR_SIZE];
        for (int i = 0; i < MAX_CAPTURE_NODE; i++)
            memset(captureNodeName[i], 0, EXYNOS_CAMERA_NAME_STR_SIZE);

        frame->getMetaData(shot_ext);

        if (m_reprocessing == false)
            m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)buffer);

        CLOGV("frameCount(%d), rCount(%d)",
                frame->getFrameCount(), getMetaDmRequestFrameCount(shot_ext));

        frame->getNodeGroupInfo(&node_group_info, perframeInfoIndex);
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
        frame->getVirtualNodeInfo(&virtualnode_group_info, perframeInfoIndex);
#endif
#ifdef USE_AF_FOV_COMPENSATION
        if(perframeInfoIndex == PERFRAME_INFO_ISP){
            m_parameters->CheckAfFovc(frame, perframeInfoIndex);
            frame->getNodeGroupInfo(&node_group_info, perframeInfoIndex);
        }
#endif
        /* Update node's size & request */
        memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));

        if (node_group_info.leader.request == 1) {
            if (m_checkNodeGroupInfo(frame, m_deviceInfo->nodeName[OUTPUT_NODE], &m_curNodeGroupInfo[perframeInfoIndex].leader, &node_group_info.leader) != NO_ERROR)
                CLOGW(" m_checkNodeGroupInfo(leader) fail");

            setMetaNodeLeaderInputSize(shot_ext,
                    node_group_info.leader.input.cropRegion[0],
                    node_group_info.leader.input.cropRegion[1],
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3]);

            setMetaNodeLeaderOutputSize(shot_ext,
                    node_group_info.leader.output.cropRegion[0],
                    node_group_info.leader.output.cropRegion[1],
                    node_group_info.leader.output.cropRegion[2],
                    node_group_info.leader.output.cropRegion[3]);

            setMetaNodeLeaderRequest(shot_ext, node_group_info.leader.request);
            setMetaNodeLeaderVideoID(shot_ext,
                    m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perFrameVideoID);
            setMetaNodeLeaderPixFormat(shot_ext, node_group_info.leader.pixelformat);
#ifdef META_USE_NODE_PIXELSIZE
            setMetaNodeLeaderPixelSize(shot_ext, node_group_info.leader.pixelsize);
#endif
        }

        if (CAPTURE_NODE_MAX < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):PipeId(%d) has Invalid perframeSupportNodeNum:CAPTURE_NODE_MAX(%d) < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum(%d), assert!!!!",
                     __FUNCTION__, __LINE__, getPipeId(), CAPTURE_NODE_MAX, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum);
        }

        /* Update capture node request from Frame */
        for (int i = CAPTURE_NODE; i < MAX_NODE; i++) {
            if (m_node[i] != NULL) {
                uint32_t videoId = 0;

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
                if (m_deviceInfo->virtualNodeNum[i] >= 0) {
                    videoId = m_deviceInfo->virtualNodeNum[i] - FIMC_IS_VIDEO_BAS_NUM;
                } else
#endif
                {
                    videoId = m_deviceInfo->nodeNum[i] - FIMC_IS_VIDEO_BAS_NUM;
                }

                for (perframePosition = 0; perframePosition < CAPTURE_NODE_MAX; perframePosition++) {
                    uint32_t perframeVid = -1;
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
                    if (m_deviceInfo->virtualNodeNum[i] >= 0) {
                        perframeVid = virtualnode_group_info.virtualVid[perframePosition];
                    } else
#endif
                    {
                        perframeVid = node_group_info.capture[perframePosition].vid;
                    }

                    if (perframeVid == videoId) {
                        node_group_info.capture[perframePosition].request = frame->getRequest(getPipeId((enum NODE_TYPE)i));
                        strncpy(captureNodeName[perframePosition], m_deviceInfo->nodeName[i], EXYNOS_CAMERA_NAME_STR_SIZE - 1);
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i ++) {
            /*
             * To set 3AP BDS size on full OTF,
             * We need to set perframeSize.
             * set size when request is 0. so, no side effect.
             */
            /* if (node_group_info.capture[i].request == 1) { */
            if ((int)node_group_info.capture[i].vid + FIMC_IS_VIDEO_BAS_NUM < 0) {
                CLOGW("invalid video ID(%d)", node_group_info.capture[i].vid);
                continue;
            }

            if (m_checkNodeGroupInfo(frame, captureNodeName[i], &m_curNodeGroupInfo[perframeInfoIndex].capture[i], &node_group_info.capture[i]) != NO_ERROR)
                CLOGW(" m_checkNodeGroupInfo(%d) fail", i);

            setMetaNodeCaptureInputSize(shot_ext, i,
                    node_group_info.capture[i].input.cropRegion[0],
                    node_group_info.capture[i].input.cropRegion[1],
                    node_group_info.capture[i].input.cropRegion[2],
                    node_group_info.capture[i].input.cropRegion[3]);

            setMetaNodeCaptureOutputSize(shot_ext, i,
                    node_group_info.capture[i].output.cropRegion[0],
                    node_group_info.capture[i].output.cropRegion[1],
                    node_group_info.capture[i].output.cropRegion[2],
                    node_group_info.capture[i].output.cropRegion[3]);

            setMetaNodeCaptureRequest(shot_ext, i, node_group_info.capture[i].request);
            setMetaNodeCaptureVideoID(shot_ext, i, node_group_info.capture[i].vid);
            setMetaNodeCapturePixFormat(shot_ext, i, node_group_info.capture[i].pixelformat);
#ifdef META_USE_NODE_PIXELSIZE
            setMetaNodeCapturePixelSize(shot_ext, i, node_group_info.capture[i].pixelsize);
#endif
        }

        // store perframe size
        m_setSizeInfo(frame, shot_ext);

        /*
           CLOGI("frameCount(%d)", shot_ext->shot.dm.request.frameCount);
           frame->dumpNodeGroupInfo(m_deviceInfo->nodeName[OUTPUT_NODE]);
           m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo[OUTPUT_NODE]);

           for (int i = (OUTPUT_NODE + 1); i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i++)
           m_dumpPerframeNodeGroupInfo("m_perframeCaptureNodeGroupInfo", m_perframeMainNodeGroupInfo[i]);
         */

        /* dump info on shot_ext, just before qbuf */
        /* m_dumpPerframeShotInfo(m_deviceInfo->nodeName[OUTPUT_NODE], frame->getFrameCount(), shot_ext); */

        CLOG_PERFRAME(META, m_cameraId, m_name, frame.get(), &shot_ext->shot, frame->getRequestKey(), "");
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_updateMetadataToFrame(void *metadata, int index, ExynosCameraFrameSP_sptr_t frame, enum NODE_TYPE nodeLocation)
{
    CLOGV("");
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t curFrame = NULL;
    camera2_shot_ext *shot_ext;
    shot_ext = (struct camera2_shot_ext *)metadata;

    if (shot_ext == NULL) {
        CLOGE("Meta buffer is null");
        return BAD_VALUE;
    }
    if (index < 0) {
        CLOGE("Invalid index(%d)", index);
        return BAD_VALUE;
    }
    if (frame == NULL) {
        CLOGE("frame is Null");
        return BAD_VALUE;
    }
    if (nodeLocation < OUTPUT_NODE) {
        CLOGE("Invalid node location(%d)", nodeLocation);
        return BAD_VALUE;
    }

    if (m_metadataTypeShot == false) {
        CLOGV("Stream type do not need update metadata");
        return NO_ERROR;
    }

    // this will make in Driver.
    shot_ext->user.ddk_version.header1 = DDK_VER_MAGIC_NUMBER_0;
    shot_ext->user.ddk_version.header2 = DDK_VER_MAGIC_NUMBER_1;

    /*
    ret = m_getFrameByIndex(&curFrame, index, nodeLocation);
    if (ret != NO_ERROR) {
        CLOGE("m_getFrameByIndex() fail, node(%s), index(%d), ret(%d)",
                 m_deviceInfo->nodeName[nodeLocation], index, ret);
        return ret;
    }
    */

    ret = frame->storeShotExtMeta(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("storeShotExtMeta() fail, ret(%d)", ret);
        return ret;
    }

#ifdef USE_DEBUG_PROPERTY
    int64_t testOvfTimestamp = 0;
    ExynosCameraProperty property;
    ret = property.get(ExynosCameraProperty::DEBUG_TEST_OVF_TIMESTAMP, LOG_TAG, testOvfTimestamp);
    if (ret != NO_ERROR) {
        CLOGE("m_property.get() fail, ret(%d)", ret);
    }

    /* timestamp update by user's control value */
    if (getPipeId() == PIPE_3AA && testOvfTimestamp > 0LL) {
        nsecs_t curTimestamp = getMetaDmSensorTimeStamp(shot_ext);
        setMetaDmSensorTimeStamp(shot_ext, curTimestamp + testOvfTimestamp);

        curTimestamp = getMetaUdmSensorTimeStampBoot(shot_ext);
        setMetaUdmSensorTimeStampBoot(shot_ext, curTimestamp + testOvfTimestamp);
    }
#endif

    ret = frame->storeDynamicMeta(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("storeDynamicMeta() fail, ret(%d)", ret);
        return ret;
    }

    ret = frame->storeUserDynamicMeta(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("storeUserDynamicMeta() fail, ret(%d)", ret);
        return ret;
    }

    if (shot_ext->shot.dm.request.frameCount != 0)
        ret = frame->setMetaDataEnable(true);

    CLOG_PERFRAME(META, m_cameraId, m_name, frame.get(), &shot_ext->shot, frame->getRequestKey(), "");

    return ret;
}

status_t ExynosCameraMCPipe::m_getFrameByIndex(ExynosCameraFrameSP_dptr_t frame, int index, enum NODE_TYPE nodeLocation)
{
    CLOGV("");

    if (nodeLocation < OUTPUT_NODE) {
        CLOGE("Invalid node location(%d)", nodeLocation);
        return BAD_VALUE;
    }
    if (index < 0) {
        CLOGE("Invalid index(%d)", index);
        return BAD_VALUE;
    }

    frame = m_runningFrameList[nodeLocation][index];
    if (frame == NULL) {
        CLOGE("Unknown buffer, index %d frame is NULL", index);
        dump();
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_completeFrame(
        ExynosCameraFrameSP_sptr_t frame,
        bool isValid)
{
    CLOGV("");
    status_t ret = NO_ERROR;

    if (frame == NULL) {
        CLOGE("Frame is NULL");
        dump();
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
        if (isValid == false) {
            CFLOGD(frame, "NOT DONE");
        }
    }

    ret = frame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret != NO_ERROR) {
        CLOGE("Set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    CLOGV("Entity pipeId(%d), frameCount(%d)",
             getPipeId(), frame->getFrameCount());

    return ret;
}

status_t ExynosCameraMCPipe::m_setInput(ExynosCameraNode *nodes[], int32_t *nodeNums, int32_t *sensorIds)
{
    status_t ret = NO_ERROR;
    int currentSensorId[MAX_NODE] = {0};

    if (nodes == NULL || nodeNums == NULL || sensorIds == NULL) {
        CLOGE(" nodes == %p || nodeNum == %p || sensorId == %p",
                 nodes, nodeNums, sensorIds);
        return INVALID_OPERATION;
    }

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        // sensor's setInput happen in m_createSensorNode, for performance
        if (i == m_sensorNodeIndex) {
            continue;
        }

#ifdef SUPPORT_DEPTH_MAP
        if (i == m_depthVcNodeIndex) {
            continue;
        }
#endif // SUPPORT_DEPTH_MAP

        if (m_flagValidInt(nodeNums[i]) == false)
            continue;

        if (m_flagValidInt(sensorIds[i]) == false)
            continue;

        if (nodes[i] == NULL)
            continue;

        currentSensorId[i] = nodes[i]->getInput();

#ifndef USE_CLAHE_PREVIEW
        if (m_flagValidInt(currentSensorId[i]) == false ||
            currentSensorId[i] != sensorIds[i])
#endif
        {

#ifdef INPUT_STREAM_MASK
            CLOGD(" setInput(sensorIds : %d) [src nodeNum : %d][nodeNums : %d][otf : %d][leader : %d][reprocessing : %d][unique sensorId : %d]",
                 sensorIds[i],
                ((sensorIds[i] & INPUT_VINDEX_MASK)   >> INPUT_VINDEX_SHIFT) + FIMC_IS_VIDEO_BAS_NUM,
                nodeNums[i],
                ((sensorIds[i] & INPUT_MEMORY_MASK)   >> INPUT_MEMORY_SHIFT),
                ((sensorIds[i] & INPUT_LEADER_MASK)   >> INPUT_LEADER_SHIFT),
                ((sensorIds[i] & INPUT_STREAM_MASK)   >> INPUT_STREAM_SHIFT),
                ((sensorIds[i] & INPUT_POSITION_MASK) >> INPUT_POSITION_SHIFT));
#else
            CLOGD(" setInput(sensorIds : %d)",
                 sensorIds[i]);
#endif
            ret = nodes[i]->setInput(sensorIds[i]);
            if (ret < 0) {
                CLOGE(" nodeNums[%d] : %d, setInput(sensorIds : %d fail, ret(%d)",
                     i, nodeNums[i], sensorIds[i],
                    ret);

                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    uint32_t planeCount = 2;
    enum YUV_RANGE yuvRange = YUV_FULL_RANGE;
    int pipeId = -1;

    if (pipeInfos == NULL) {
        CLOGE("pipeInfos is NULL");
        return BAD_VALUE;
    }

    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        if (m_node[i] != NULL) {

#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            if (m_sharedNode[i] == true) {
                CLOGD("node(%s)->setNodeInfo skipped, shared node ret(%d)", m_deviceInfo->nodeName[i], ret);
                continue;
            }
#endif

            m_numBuffers[i] = pipeInfos[i].bufInfo.count;
            m_perframeMainNodeGroupInfo[i] = pipeInfos[i].perFrameNodeGroupInfo;

            /*
             * If node size is 0, node info is skipped.
             * But the buffer number and per frame node group info of the node must be set.
             */
            if ((0 >= pipeInfos[i].rectInfo.fullW)
                || (0 >= pipeInfos[i].rectInfo.fullH)) {
                CLOGD("skip %s setNodeInfo, setFormat(%d, %d)",
                        m_node[i]->getName(),
                        pipeInfos[i].rectInfo.fullW, pipeInfos[i].rectInfo.fullH);
                continue;
            }

            /* check about OUTPUT_NODE */
            if (i == OUTPUT_NODE
                && pipeInfos[i].bufInfo.type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                CLOGE("pipeInfos[%d].bufInfo.type is not Valid(type:%d)",
                         i, pipeInfos[i].bufInfo.type);
                return BAD_VALUE;
            }

            uint32_t bytePerPlane = 0;
            int colorFormat = pipeInfos[i].rectInfo.colorFormat;

            if (m_deviceInfo->nodeNum[i] <= FIMC_IS_VIDEO_I0S_NUM
                || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_I1S_NUM
                || m_deviceInfo->nodeNum[i] >= FIMC_IS_VIDEO_SS0VC0_NUM) {
                planeCount = 1;
            } else {
                getV4l2FormatInfo(colorFormat, &bytePerPlane, &planeCount);
            }

            /* Add medadata plane count */
            planeCount++;

            pipeId = getPipeId((enum NODE_TYPE)i);
            if (pipeId < 0) {
                CLOGE("getPipeId(%d) fail", i);
                ret = BAD_VALUE;
            }

            if (m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_SCP_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M0P_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M1P_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M2P_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M3P_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M4P_NUM
               || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M5P_NUM) {
                int setfile = 0;
                int previewYuvRange = 0;

                /* MC scaler can set different format with preview */
                /*
                   int colorFormat = m_parameters->getHwPreviewFormat();

                   if (colorFormat != pipeInfos[i].rectInfo.colorFormat) {
                   CLOGE("SCP colorformat is not Valid(%d)",
                    pipeInfos[i].rectInfo.colorFormat);
                   return BAD_VALUE;
                   }
                */

                m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &previewYuvRange);

                yuvRange = YUV_FULL_RANGE;
                if ((m_parameters->isPreviewPortId(m_parameters->getYuvOutPortId((enum pipeline)INDEX(pipeId))) == true)
                    || (m_parameters->isPreviewCbPortId(m_parameters->getYuvOutPortId((enum pipeline)INDEX(pipeId))) == true)
                    || (m_parameters->isRecordingPortId(m_parameters->getYuvOutPortId((enum pipeline)INDEX(pipeId))) == true)) {
                    yuvRange = (enum YUV_RANGE)previewYuvRange;

                    CLOGD("m_node[%d]: pipeId(%d), outputPortId(%d), YUV_RANGE(%d)", i, pipeId,
                            m_parameters->getYuvOutPortId((enum pipeline)INDEX(pipeId)), yuvRange);
                }
            }

            ret = m_setNodeInfo(m_node[i], &pipeInfos[i], planeCount, yuvRange);
            if (ret != NO_ERROR) {
                CLOGE("m_setNodeInfo(W:%d, H:%d, buffer count:%d) fail(Node:%s), ret(%d)",
                        pipeInfos[i].rectInfo.fullW, pipeInfos[i].rectInfo.fullH,
                        pipeInfos[i].bufInfo.count, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setNodeInfo(ExynosCameraNode *node, camera_pipe_info_t *pipeInfos,
                                         uint32_t planeCount, enum YUV_RANGE yuvRange,
                                         __unused bool flagBayer)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    bool flagSetRequest = false;
    unsigned int requestBufCount = 0;
    int currentW = 0;
    int currentH = 0;
    int currentV4l2Colorformat = 0;
    int currentPlanesCount = 0;
    enum YUV_RANGE currentYuvRange = YUV_FULL_RANGE;
    int  currentBufferCount = 0;
    enum v4l2_buf_type currentBufType;
    enum v4l2_memory currentMemType;

    if (node == NULL) {
        CLOGE("node is NULL");
        return BAD_VALUE;
    }

    if (pipeInfos == NULL) {
        CLOGE("pipeInfos is NULL");
        return BAD_VALUE;
    }

    requestBufCount = node->reqBuffersCount();

    /* If it already set */
    if (0 < requestBufCount) {
        node->getSize(&currentW, &currentH);
        node->getColorFormat(&currentV4l2Colorformat, &currentPlanesCount, &currentYuvRange);
        node->getBufferType(&currentBufferCount, &currentBufType, &currentMemType);

        if (/* setSize */
            currentW               != pipeInfos->rectInfo.fullW ||
            currentH               != pipeInfos->rectInfo.fullH ||
            /* setColorFormat */
            currentV4l2Colorformat != pipeInfos->rectInfo.colorFormat ||
            currentPlanesCount     != (int)planeCount ||
            currentYuvRange        != yuvRange ||
            /* setBufferType */
            currentBufferCount     != (int)pipeInfos->bufInfo.count ||
            currentBufType         != (enum v4l2_buf_type)pipeInfos->bufInfo.type ||
            currentMemType         != (enum v4l2_memory)pipeInfos->bufInfo.memory) {

            flagSetRequest = true;

            CLOGW("Node is already requested. call clrBuffers()");

            CLOGW("W(%d -> %d), H(%d -> %d)",
                currentW, pipeInfos->rectInfo.fullW,
                currentH, pipeInfos->rectInfo.fullH);

            CLOGW("colorFormat(%d -> %d), planeCount(%d -> %d), yuvRange(%d -> %d)",
                currentV4l2Colorformat, pipeInfos->rectInfo.colorFormat,
                currentPlanesCount,     planeCount,
                currentYuvRange,        yuvRange);

            CLOGW("bufferCount(%d -> %d), bufType(%d -> %d), memType(%d -> %d)",
                currentBufferCount, pipeInfos->bufInfo.count,
                currentBufType,     pipeInfos->bufInfo.type,
                currentMemType,     pipeInfos->bufInfo.memory);

            ret = node->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE(" node->clrBuffers() fail");
                return ret;
            }
        }
    } else {
        flagSetRequest = true;
    }

    if (flagSetRequest == true) {
        CLOGD("set pipeInfos on %s, setFormat(%d, %d) and reqBuffers(%d), yuvRange(%d) pixelSize(%d) pixelCompInfo(%d)",
            node->getName(), pipeInfos->rectInfo.fullW,
            pipeInfos->rectInfo.fullH, pipeInfos->bufInfo.count, yuvRange,
            pipeInfos->pixelSize, pipeInfos->pixelCompInfo);

        bool flagValidSetFormatInfo = true;

        if (pipeInfos->rectInfo.fullW == 0 || pipeInfos->rectInfo.fullH == 0) {
            CLOGW("Invalid size(%d x %d), skip setSize()",
                pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH);

            flagValidSetFormatInfo = false;
        }
        node->setSize(pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH);

        if (pipeInfos->rectInfo.colorFormat == 0 || planeCount == 0) {
            CLOGW("invalid colorFormat(%d), planeCount(%d), skip setColorFormat()",
                pipeInfos->rectInfo.colorFormat, planeCount);

            flagValidSetFormatInfo = false;
        }

        node->setColorFormat(pipeInfos->rectInfo.colorFormat, planeCount, yuvRange, pipeInfos->pixelSize, pipeInfos->pixelCompInfo);

        if ((int)pipeInfos->bufInfo.type == 0 || pipeInfos->bufInfo.memory == 0) {
            CLOGW("Invalid bufInfo.type(%d), bufInfo.memory(%d), skip setBufferType()",
                (int)pipeInfos->bufInfo.type, (int)pipeInfos->bufInfo.memory);

            flagValidSetFormatInfo = false;
        }
        node->setBufferType(pipeInfos->bufInfo.count,
                            (enum v4l2_buf_type)pipeInfos->bufInfo.type,
                            (enum v4l2_memory)pipeInfos->bufInfo.memory);

        if (flagValidSetFormatInfo == true) {
            {
                ret = node->setFormat(pipeInfos->bytesPerPlane);
                if (ret != NO_ERROR) {
                    CLOGE("node->setFormat() fail");
                    return ret;
                }
            }
        }

        node->getBufferType(&currentBufferCount, &currentBufType, &currentMemType);

    } else {
        CLOGD("Skip set pipeInfos setFormat(%d, %d) and reqBuffers(%d).",
            pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH, pipeInfos->bufInfo.count);
    }

    if (currentBufferCount <= 0) {
        CLOGW("Invalid currentBufferCount(%d), skip reqBuffers()",
                 currentBufferCount);
    } else {
        ret = node->reqBuffers();
        if (ret != NO_ERROR) {
            CLOGE("node->reqBuffers() fail");
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_getPerframePosition(int *perframePosition, uint32_t pipeId)
{
    status_t ret = NO_ERROR;

    if (perframePosition == NULL) {
        CLOGE("perframePosition is NULL");
        return BAD_VALUE;
    }

    switch(pipeId) {
    case PIPE_VC0:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_VC0_POS : PERFRAME_FRONT_VC0_POS;
        break;
    case PIPE_3AC:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
        break;
    case PIPE_3AP:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
        break;
    case PIPE_ISPC:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_ISPC_POS : PERFRAME_FRONT_ISPC_POS;
        break;
    case PIPE_ISPP:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
        break;
    case PIPE_SCC:
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_SCC_POS : PERFRAME_FRONT_SCC_POS;
        break;
    case PIPE_SCP: /* Same as case of PIPE_MCSC0 */
        *perframePosition = (isBackCamera(m_cameraId) == true) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
        break;
    case PIPE_MCSC1:
        *perframePosition = PERFRAME_BACK_MCSC1_POS;
        break;
    case PIPE_MCSC2:
        *perframePosition = PERFRAME_BACK_MCSC2_POS;
        break;
    case PIPE_3AC_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_3AC_POS;
        break;
    case PIPE_3AP_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_3AP_POS;
        break;
    case PIPE_ISPC_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_ISPC_POS;
        break;
    case PIPE_ISPP_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_ISPP_POS;
        break;
    case PIPE_MCSC0_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC0_POS;
        break;
    case PIPE_MCSC1_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC1_POS;
        break;
    case PIPE_MCSC2_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC2_POS;
        break;
    case PIPE_MCSC3_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC3_POS;
        break;
    case PIPE_MCSC4_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC4_POS;
        break;
    default:
        CLOGV("Invalid pipeID(%d)", pipeId);
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setSetfile(ExynosCameraNode *node, uint32_t pipeId)
{
    CLOGV("");
    status_t ret = NO_ERROR;
    int yuvRange = 0;
    int setfile = 0;

    m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &yuvRange);

    if (m_parameters->getSetFileCtlMode() == true) {
        if (m_parameters->getSetFileCtl3AA() == true
            && (pipeId == PIPE_3AA || pipeId == PIPE_3AA_REPROCESSING)) {
            ret = node->setControl(V4L2_CID_IS_SET_SETFILE, setfile);
            if (ret != NO_ERROR) {
                CLOGE("setControl(%d) fail(ret = %d)", setfile, ret);
                return ret;
            }
        } else if (m_parameters->getSetFileCtlISP() == true
                && (pipeId == PIPE_ISP || pipeId == PIPE_ISP_REPROCESSING)) {
            ret = node->setControl(V4L2_CID_IS_SET_SETFILE, setfile);
            if (ret != NO_ERROR) {
                CLOGE("setControl(%d) fail(ret = %d)", setfile, ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_forceDone(ExynosCameraNode *node, unsigned int cid, int value)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    if (node == NULL) {
        CLOGE("node is NULL");
        return BAD_VALUE;
    }

    if (cid != V4L2_CID_IS_FORCE_DONE) {
        CLOGW("cid != V4L2_CID_IS_FORCE_DONE");
    }

    /* "value" is not meaningful */
    ret = node->setControl(V4L2_CID_IS_FORCE_DONE, value);
    if (ret != NO_ERROR) {
        CLOGE("node V4L2_CID_IS_FORCE_DONE failed");
        node->dump();
        return ret;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_sensorStandby(bool on)
{
    status_t ret = NO_ERROR;

    int value = (!on | (1 << SENSOR_USE_STANDBY_SHIFT));

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setControl(V4L2_CID_IS_S_STREAM, value);
            if (ret != NO_ERROR) {
                CLOGE("sensorStandby failed, %s node, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
            } else {
                CLOGI("sensorStandby(%s) success!", (on?"On":"Off"));
            }

            return ret;
        }
    }

    CLOGE("All Nodes is NULL");

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::m_setMapBuffer(int nodeIndex)
{
    status_t ret = NO_ERROR;

    int bufferIndex[VIDEO_MAX_FRAME];
    for (int i = 0; i < VIDEO_MAX_FRAME; i++)
        bufferIndex[i] = -2;
    ExynosCameraBuffer buffer;

    if (m_bufferManager[nodeIndex]->getAllocatedBufferCount() > 0) {
        int index = 0;
        while (m_bufferManager[nodeIndex]->getNumOfAvailableBuffer() > 0) {
            ret |= m_bufferManager[nodeIndex]->getBuffer(&(bufferIndex[index]), EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &buffer);
            if (ret != NO_ERROR) {
                CLOGE("Buffer manager getBuffer fail, manager(%d), ret(%d)",
                         nodeIndex, ret);
            }

            ret |= m_node[nodeIndex]->prepareBuffer(&buffer);
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->putBuffer() fail, ret(%d)",
                         m_deviceInfo->nodeName[nodeIndex], ret);

            }
            index++;
        }

        while (index > 0) {
            index--;
            /* TODO: doing exception handling */
            ret |= m_bufferManager[nodeIndex]->putBuffer(bufferIndex[index], EXYNOS_CAMERA_BUFFER_POSITION_NONE);
            if (ret != NO_ERROR) {
                CLOGE("Buffer manager putBuffer fail, manager(%d), ret(%d)",
                         nodeIndex, ret);
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setMapBuffer(ExynosCameraNode *node, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;

    if (buffer == NULL) {
        CLOGE("Buffer is NULL");
        return BAD_VALUE;
    }
    if (node == NULL) {
        CLOGE("Node is NULL");
        return BAD_VALUE;
    }

    /* Require code sync release-git to Repo */
#if 0
    ret = node->mapBuffer(buffer);
    if (ret != NO_ERROR)
        CLOGE("mapBuffer() fail, ret(%d)", ret);
#endif

    return ret;
}

status_t ExynosCameraMCPipe::m_setJpegInfo(int nodeType, ExynosCameraFrameSP_sptr_t frame)
{
    status_t ret = NO_ERROR;
    int pipeId = MAX_PIPE_NUM;
    ExynosRect pictureRect;
    ExynosRect thumbnailRect;
    int jpegQuality = -1;
    int thumbnailQuality = -1;
    exif_attribute_t exifInfo;
    debug_attribute_t *debugInfo;
    camera2_shot_ext shot_ext;

    /* 1. Check the invalid parameters */
    if (nodeType < 0) {
        CLOGE("Invalid nodeType %d", nodeType);
        return BAD_VALUE;
    }

    if (frame == NULL) {
        CLOGE("frame is NULL!");
        return BAD_VALUE;
    }

    /* 2. Get control informations from parameter & metadata */
    ret = frame->getMetaData(&shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getMetadata, Fcount(%d), ret %d",
                frame->getFrameCount(), ret);
    }

#ifdef SUPPORT_MULTI_STREAM_CAPTURE
    if (m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
        int width = 0, height = 0;

        m_configurations->getSize(CONFIGURATION_CAPTURE_STREAM_BUFFER_SIZE, (uint32_t *)&width, (uint32_t *)&height);
        m_parameters->getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureRect.w, (uint32_t *)&pictureRect.h);
        if ((width != 0 && width != pictureRect.w) || (height != 0 && height != pictureRect.h)) {
            pictureRect.w = width;
            pictureRect.h = height;
        }
    } else
#endif
    {
        m_configurations->getSize(CONFIGURATION_PICTURE_SIZE, (uint32_t *)&pictureRect.w, (uint32_t *)&pictureRect.h);
        // TODO: Not hw picture size? (cause of lots of sw lib)
        // m_parameters->getSize(HW_INFO_HW_PICTURE_SIZE, (uint32_t *)&pictureRect.w, (uint32_t *)&pictureRect.h);
    }

    jpegQuality = (int)shot_ext.shot.ctl.jpeg.quality;
    thumbnailRect.w = shot_ext.shot.ctl.jpeg.thumbnailSize[0];
    thumbnailRect.h = shot_ext.shot.ctl.jpeg.thumbnailSize[1];
    thumbnailQuality = shot_ext.shot.ctl.jpeg.thumbnailQuality;
    debugInfo = m_parameters->getDebugAttribute();

    /* 3. Set JPEG node perframe control information for each node */
    pipeId = m_deviceInfo->pipeId[nodeType];
    CLOGD("pipeId-%d thumbnailRect[%d %d] thumbnailQuality %d", pipeId, thumbnailRect.w, thumbnailRect.h, thumbnailQuality);

    switch (pipeId) {
    case PIPE_HWFC_JPEG_DST_REPROCESSING:
        /* Create EXIF info */
        ret = m_parameters->getFixedExifInfo(&exifInfo);
        if (ret != NO_ERROR) {
            CLOGE("Failed to get Fixed Exif Info, ret %d", ret);
        }

        if (thumbnailRect.w > 0 && thumbnailRect.h > 0) {
            exifInfo.enableThumb = true;
        } else {
            exifInfo.enableThumb = false;
        }

        m_parameters->setExifChangedAttribute(&exifInfo, &pictureRect, &thumbnailRect, &shot_ext);

        /* JPEG HAL setExifInfo */
        ret = m_node[nodeType]->setExifInfo(&exifInfo);
        if (ret != NO_ERROR)
            CLOGE("Failed to set EXIF info into %s, ret %d",
                    m_deviceInfo->nodeName[nodeType], ret);

        /* JPEG HAL setDebugInfo */
        debugInfo = m_parameters->getDebugAttribute();
        ret = m_node[nodeType]->setDebugInfo(debugInfo);
        if (ret != NO_ERROR)
            CLOGE("Failed to set DEBUG Info into %s, ret %d",
                    m_deviceInfo->nodeName[nodeType], ret);
        /* continue to setSize & setQuality */
    case PIPE_HWFC_JPEG_SRC_REPROCESSING:
        /* JPEG HAL setSize */
        ret = m_node[nodeType]->setSize(pictureRect.w, pictureRect.h);
        if (ret != NO_ERROR) {
            CLOGE("Failed to set size %dx%d into %s, ret %d",
                    pictureRect.w, pictureRect.h,
                    m_deviceInfo->nodeName[nodeType], ret);
        }

        {
            /* JPEG HAL setQuality */
            CLOGV("m_node[nodeType]->setQuality(int)");
            ret = m_node[nodeType]->setQuality(jpegQuality);
            if (ret != NO_ERROR)
                CLOGE("Failed to set jpeg quality %d into %s, ret %d",
                        jpegQuality,
                        m_deviceInfo->nodeName[nodeType], ret);
        }
        break;
    case PIPE_HWFC_THUMB_SRC_REPROCESSING:
        /* JPEG HAL setSize */
        if (thumbnailRect.w > 0 && thumbnailRect.h > 0) {
            ret = m_node[nodeType]->setSize(thumbnailRect.w, thumbnailRect.h);
            if (ret != NO_ERROR)
                CLOGE("Failed to set thumbnail size %dx%d into %s, ret %d",
                        thumbnailRect.w, thumbnailRect.h,
                        m_deviceInfo->nodeName[nodeType], ret);
        }

        /* JPEG HAL setQuality */
        m_node[nodeType]->setQuality(thumbnailQuality);
        if (ret != NO_ERROR)
            CLOGE("Failed to setQuality %d into %s, ret %d",
                    thumbnailQuality,
                    m_deviceInfo->nodeName[nodeType], ret);
        break;
    default:
        CLOGE("Invalid pipeId %d", pipeId);
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_startNode(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    for (int i = (MAX_CAPTURE_NODE - 1); i >= OUTPUT_NODE; i--) {
        /* only M2M mode need stream on/off */
        /* TODO : flite has different sensorId bit */
        if (m_node[i] != NULL) {
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            if (m_sharedNode[i] == true) {
                CLOGD("node(%s)->start skipped, shared node ret(%d)", m_deviceInfo->nodeName[i], ret);
                continue;
            }
#endif

            ret = m_node[i]->start();
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->start fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_stopNode(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_CAPTURE_NODE; i++) {
        /* only M2M mode need stream on/off */
        /* TODO : flite has different sensorId bit */
        if (m_node[i] != NULL) {
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            if (m_sharedNode[i] == true) {
                CLOGD("node(%s)->stop skipped, shared node ret(%d)", m_deviceInfo->nodeName[i], ret);
                continue;
            }
#endif

            ret = m_node[i]->stop();
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->stop fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                /* If stop() is error, driver will recover */
                funcRet |= ret;
            }

            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->clrBuffers fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                /* If clrBuffers() is error, driver will recover */
                funcRet |= ret;
            }

            m_node[i]->removeItemBufferQ();
        }
    }

    return funcRet;
}

status_t ExynosCameraMCPipe::m_clearNode(void)
{
    CLOGD("");
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
            if (m_sharedNode[i] == true) {
                CLOGD("node(%s)->clrBuffers skipped, shared node ret(%d)", m_deviceInfo->nodeName[i], ret);
                continue;
            }
#endif

            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("node(%s)->clrBuffers fail, ret(%d)",
                         m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, char *name, camera2_node *oldNode, camera2_node *newNode)
{
    if (oldNode == NULL || newNode == NULL) {
        CLOGE(" oldNode(%p) == NULL || newNode(%p) == NULL", oldNode, newNode);
        return INVALID_OPERATION;
    }

    for (int i = 0; i < 4; i++) {
        if (m_configurations->getModeValue(CONFIGURATION_HIGHSPEED_MODE) <= CONFIG_MODE::HIGHSPEED_60
            && (oldNode->input.cropRegion[i] != newNode->input.cropRegion[i] ||
            oldNode->output.cropRegion[i] != newNode->output.cropRegion[i] ||
            oldNode->request != newNode->request)) {
            CLOGD("[REQ:%d,F:%d,DRV:%d,T:%d] (%s):vid(%d), request(%d -> %d), oldCropSize(%d, %d, %d, %d / %d, %d, %d, %d) -> newCropSize(%d, %d, %d, %d / %d, %d, %d, %d)",
                frame->getRequestKey(),
                frame->getFrameCount(),
                frame->getMetaFrameCount(),
                frame->getFrameType(),
                name,
                newNode->vid,
                oldNode->request, newNode->request,
                oldNode->input. cropRegion[0], oldNode->input. cropRegion[1], oldNode->input. cropRegion[2], oldNode->input. cropRegion[3],
                oldNode->output.cropRegion[0], oldNode->output.cropRegion[1], oldNode->output.cropRegion[2], oldNode->output.cropRegion[3],
                newNode->input. cropRegion[0], newNode->input. cropRegion[1], newNode->input. cropRegion[2], newNode->input. cropRegion[3],
                newNode->output.cropRegion[0], newNode->output.cropRegion[1], newNode->output.cropRegion[2], newNode->output.cropRegion[3]);

            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        oldNode->request = newNode->request;
        oldNode->input. cropRegion[i] = newNode->input. cropRegion[i];
        oldNode->output.cropRegion[i] = newNode->output.cropRegion[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, char *name, int index, camera2_node *oldNode, camera2_node *newNode)
{
    if (oldNode == NULL || newNode == NULL) {
        CLOGE(" oldNode(%p) == NULL || newNode(%p) == NULL", oldNode, newNode);
        return INVALID_OPERATION;
    }

    for (int i = 0; i < 4; i++) {
        if (oldNode->input.cropRegion[i] != newNode->input.cropRegion[i] ||
            oldNode->output.cropRegion[i] != newNode->output.cropRegion[i] ||
            oldNode->request != newNode->request) {

            CLOGD("[REQ:%d,F:%d,DRV:%d,T:%d] name %s : index %d: PerFrame oldCropSize (%d, %d, %d, %d / %d, %d, %d, %d) -> newCropSize (%d, %d, %d, %d / %d, %d, %d, %d)",
                frame->getRequestKey(),
                frame->getFrameCount(),
                frame->getMetaFrameCount(),
                frame->getFrameType(),
                name,
                index,
                oldNode->input. cropRegion[0], oldNode->input. cropRegion[1], oldNode->input. cropRegion[2], oldNode->input. cropRegion[3],
                oldNode->output.cropRegion[0], oldNode->output.cropRegion[1], oldNode->output.cropRegion[2], oldNode->output.cropRegion[3],
                newNode->input. cropRegion[0], newNode->input. cropRegion[1], newNode->input. cropRegion[2], newNode->input. cropRegion[3],
                newNode->output.cropRegion[0], newNode->output.cropRegion[1], newNode->output.cropRegion[2], newNode->output.cropRegion[3]);

            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        oldNode->request = newNode->request;
        oldNode->input. cropRegion[i] = newNode->input. cropRegion[i];
        oldNode->output.cropRegion[i] = newNode->output.cropRegion[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(ExynosCameraFrameSP_dptr_t frame, int index, camera2_node *oldNode, camera2_node *newNode)
{
    return m_checkNodeGroupInfo(frame, m_deviceInfo->nodeName[index], oldNode, newNode);
}

void ExynosCameraMCPipe::m_dumpRunningFrameList(void)
{
    Mutex::Autolock lock(m_runningFrameDumpLock);

    CLOGI("last fcount(HAL:%d, DRV:%d)", m_lastFrameCount, m_lastMetaFrameCount);
    CLOGI("Thread running state(put:%d, get:%d)",
            m_putBufferThread->isRunning(), m_getBufferThread->isRunning());
    CLOGI("FrameQ size(input:%d, request:%d)",
            m_inputFrameQ->getSizeOfProcessQ(), m_requestFrameQ->getSizeOfProcessQ());
    CLOGI("*********runningFrameList dump***********");

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            int32_t runningFrameCnt = 0;
            CLOGV("[%s] m_numBuffers[%d] : %d", m_deviceInfo->nodeName[i], i, m_numBuffers[i]);
            for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
                if (m_runningFrameList[i][j] == NULL) {
                    CLOGV("[%s] runningFrameList[%d][%d] is NULL", m_deviceInfo->nodeName[i], i, j);
                } else {
                    CLOGD("[%s] runningFrameList[%d][%d]: fcount = %d",
                            m_deviceInfo->nodeName[i], i, j, m_runningFrameList[i][j]->getFrameCount());
                    runningFrameCnt++;
                }
            }
            CLOGD("[%s] runningFrameCnt : %d", m_deviceInfo->nodeName[i], runningFrameCnt);
        }
    }

    return;
}

void ExynosCameraMCPipe::m_dumpPerframeNodeGroupInfo(const char *name, camera_pipe_perframe_node_group_info_t nodeInfo)
{
    if (name != NULL)
        CLOGI("(%s) ++++++++++++++++++++", name);

    CLOGI("\t\t perframeSupportNodeNum : %d", nodeInfo.perframeSupportNodeNum);
    CLOGI("\t\t perFrameLeaderInfo.perframeInfoIndex : %d", nodeInfo.perFrameLeaderInfo.perframeInfoIndex);
    CLOGI("\t\t perFrameLeaderInfo.perFrameVideoID : %d", nodeInfo.perFrameLeaderInfo.perFrameVideoID);

    for (int i = 0; i < CAPTURE_NODE_MAX; i++)
        CLOGI("\t\t perFrameCaptureInfo[%d].perFrameVideoID : %d", i, nodeInfo.perFrameCaptureInfo[i].perFrameVideoID);

    if (name != NULL)
        CLOGI("(%s) ------------------------------", name);

    return;
}

void ExynosCameraMCPipe::m_dumpPerframeShotInfo(const char *name, int frameCount, camera2_shot_ext *shot_ext)
{
    if (name != NULL)
        CLOGI("(%s) frameCount(%d) ++++++++++++++++++++", name, frameCount);

    if (shot_ext != NULL) {
        for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
            CLOGI("\t\t index(%d), vid(%d) request(%d) input (%d, %d, %d, %d) output (%d, %d, %d, %d)",
                i,
                shot_ext->node_group.capture[i].vid,
                shot_ext->node_group.capture[i].request,
                shot_ext->node_group.capture[i].input.cropRegion[0],
                shot_ext->node_group.capture[i].input.cropRegion[1],
                shot_ext->node_group.capture[i].input.cropRegion[2],
                shot_ext->node_group.capture[i].input.cropRegion[3],
                shot_ext->node_group.capture[i].output.cropRegion[0],
                shot_ext->node_group.capture[i].output.cropRegion[1],
                shot_ext->node_group.capture[i].output.cropRegion[2],
                shot_ext->node_group.capture[i].output.cropRegion[3]);
        }
    } else {
        CLOGI("\t\t shot_ext == NULL");
    }

    if (name != NULL)
        CLOGI("(%s) ------------------------------", name);
}

void ExynosCameraMCPipe::m_configDvfs(void)
{
    CLOGD("");

    bool newDvfs = m_configurations->getMode(CONFIGURATION_DVFS_LOCK_MODE);

    if (newDvfs != m_dvfsLocked) {
        setControl(V4L2_CID_IS_DVFS_LOCK, 533000);
        m_dvfsLocked = newDvfs;
    }
}

bool ExynosCameraMCPipe::m_flagValidInt(int num)
{
    bool ret = false;

    if (num < 0)
        ret = false;
    else
        ret = true;

    return ret;
}

bool ExynosCameraMCPipe::m_checkThreadLoop(frame_queue_t *frameQ)
{
    Mutex::Autolock lock(m_pipeframeLock);
    bool loop = false;

    if (m_reprocessing == false)
        loop = true;

    if (m_oneShotMode == false)
        loop = true;

    if (frameQ->getSizeOfProcessQ() > 0)
        loop = true;

    if (m_flagSensorStandby == SENSOR_STANDBY_ON
        && frameQ->getSizeOfProcessQ() <= 0) {
        loop = false;
    }

    if (m_flagTryStop == true)
        loop = false;

    return loop;
}

status_t ExynosCameraMCPipe::m_checkPolling(ExynosCameraNode *node)
{
    int ret = 0;

    ret = node->polling();
    if (ret < 0) {
        CLOGE("polling fail, ret(%d)", ret);
        /* TODO: doing exception handling */

        m_threadState = ERROR_POLLING_DETECTED;
        return ERROR_POLLING_DETECTED;
    }

    return NO_ERROR;
}

void ExynosCameraMCPipe::m_init(camera_device_info_t *deviceInfo)
{
    if (deviceInfo != NULL)
        m_deviceInfo = deviceInfo;
    else
        m_deviceInfo = NULL;

/*
 * For replace old pipe, MCPipe have all variables.
 * but, if exist old pipe, same variable not declared by MCPipe.
 */
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        m_numOfRunningFrame[i] = 0;
        m_skipPutBuffer[i] = false;
        m_numBuffers[i] = 0;
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
        m_sharedNode[i] = false;
#endif
        for (int j = 0; j < MAX_BUFFERS; j++)
            m_runningFrameList[i][j] = NULL;
        memset(&m_perframeMainNodeGroupInfo[i], 0x00, sizeof(camera_pipe_perframe_node_group_info_t));

        if (m_deviceInfo != NULL)
            m_pipeIdArr[i] = m_deviceInfo->pipeId[i];
        else
            m_pipeIdArr[i] = 0;
    }

    m_requestFrameQ = NULL;

    m_flagSensorStandby = SENSOR_STANDBY_OFF;
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_serializeOperation = false;
#endif
#ifdef TEST_WATCHDOG_THREAD
    int testErrorDetect = 0;
#endif

    m_sensorNodeIndex = -1;
#ifdef SUPPORT_DEPTH_MAP
    m_depthVcNodeIndex = -1;
#endif // SUPPORT_DEPTH_MAP
#ifdef DEBUG_DUMP_IMAGE
    m_dumpBufferCount = 0;
    m_dumpImageQ = 0;
    m_sequentialDumpCount = 0;
#endif
    m_putInternalFrameLogCnt = 0;
    m_getInternalFrameLogCnt = 0;
    m_skipBufferHint = false;

    m_lastFrameCount = 0;
    m_lastMetaFrameCount = 0;

}

status_t ExynosCameraMCPipe::m_createSensorNode(int32_t *sensorIds)
{
    status_t ret = NO_ERROR;

    if (m_reprocessing == true)
        return NO_ERROR;

    int fliteNodeNum = getFliteNodenum(m_cameraId);

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_deviceInfo->nodeNum[i] == fliteNodeNum) {
            m_sensorNodeIndex = i;
            break;
        }
    }

    if (m_sensorNodeIndex < 0) {
        CLOGV("m_sensorNodeIndex(%d)", m_sensorNodeIndex);
        return NO_ERROR;
    }

    if (m_node[m_sensorNodeIndex] != NULL) {
        CLOGE("m_node[%d](%p) != NULL. so, fail", m_sensorNodeIndex, m_node[m_sensorNodeIndex]);
        return INVALID_OPERATION;
    }

    if (sensorIds == NULL) {
        CLOGE("sensorIds == NULL. so, fail");
        return INVALID_OPERATION;
    }

    if (sensorIds[m_sensorNodeIndex] <= 0) {
        CLOGE("sensorIds[%d](%d) <= 0. so, fail", m_sensorNodeIndex, sensorIds[m_sensorNodeIndex]);
        return INVALID_OPERATION;
    }

    m_node[m_sensorNodeIndex] = m_createNode(m_cameraId, m_deviceInfo->nodeNum[m_sensorNodeIndex]);
    if (m_node[m_sensorNodeIndex] == NULL) {
        CLOGE("sensor open failed sensorNodeIndex(%d) sensorIds[%d]=%d m_node[%d]=NULL",
            m_sensorNodeIndex, m_sensorNodeIndex, sensorIds[m_sensorNodeIndex], m_sensorNodeIndex);
        return INVALID_OPERATION;
    }

    // setInput here for performance.
#ifdef INPUT_STREAM_MASK
    CLOGD(" i(%2d) \t setInput(sensorIds : %d) [src nodeNum : %d][nodeNums : %d][otf : %d][leader : %d][reprocessing : %d][unique sensorId : %d]",
        m_sensorNodeIndex,
         sensorIds[m_sensorNodeIndex],
        ((sensorIds[m_sensorNodeIndex] & INPUT_VINDEX_MASK)   >> INPUT_VINDEX_SHIFT) + FIMC_IS_VIDEO_BAS_NUM,
        m_deviceInfo->nodeNum[m_sensorNodeIndex],
        ((sensorIds[m_sensorNodeIndex] & INPUT_MEMORY_MASK)   >> INPUT_MEMORY_SHIFT),
        ((sensorIds[m_sensorNodeIndex] & INPUT_LEADER_MASK)   >> INPUT_LEADER_SHIFT),
        ((sensorIds[m_sensorNodeIndex] & INPUT_STREAM_MASK)   >> INPUT_STREAM_SHIFT),
        ((sensorIds[m_sensorNodeIndex] & INPUT_POSITION_MASK) >> INPUT_POSITION_SHIFT));
#else
    CLOGD("setInput(sensorIds : %d)", sensorIds[m_sensorNodeIndex]);
#endif

    ret = m_node[m_sensorNodeIndex]->setInput(sensorIds[m_sensorNodeIndex]);

    if (ret < 0) {
        CLOGE("nodeNums[%d] : %p, setInput(sensorIds : %d fail, ret(%d)",
            m_sensorNodeIndex, m_node[m_sensorNodeIndex], m_sensorIds[m_sensorNodeIndex], ret);
        m_destroyNode(m_cameraId, m_node[m_sensorNodeIndex]);
        m_node[m_sensorNodeIndex] = NULL;
        return ret;
    }

    int pipeId = getPipeId((enum NODE_TYPE)m_sensorNodeIndex);
    int vId = m_deviceInfo->nodeNum[m_sensorNodeIndex] - FIMC_IS_VIDEO_BAS_NUM;
    m_sensorIds[m_sensorNodeIndex] = sensorIds[m_sensorNodeIndex];
    m_nodeIdToPipeIdMap[m_sensorNodeIndex] = pipeId;
    m_videoIdToPipeIdMap[vId] = pipeId;

#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->isDepthMapSupported()) {
        int depthMapNodeNum = getDepthVcNodeNum(m_cameraId);

        for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
            if (m_deviceInfo->nodeNum[i] == depthMapNodeNum) {
                m_depthVcNodeIndex = i;
                break;
            }
        }

        if (m_depthVcNodeIndex < 0) {
            CLOGV("m_depthVcNodeIndex(%d)", m_depthVcNodeIndex);
            return NO_ERROR;
        }

        if (m_node[m_depthVcNodeIndex] != NULL) {
            CLOGE("m_node[%d](%p) != NULL. so, fail", m_depthVcNodeIndex, m_node[m_depthVcNodeIndex]);
            return INVALID_OPERATION;
        }

        m_node[m_depthVcNodeIndex] = m_createVcNode(m_cameraId, m_deviceInfo->nodeNum[m_depthVcNodeIndex]);
        pipeId = getPipeId((enum NODE_TYPE)m_depthVcNodeIndex);
        vId = m_deviceInfo->nodeNum[m_depthVcNodeIndex] - FIMC_IS_VIDEO_BAS_NUM;
        m_nodeIdToPipeIdMap[m_depthVcNodeIndex] = pipeId;
        m_videoIdToPipeIdMap[vId] = pipeId;
    }
#endif // SUPPORT_DEPTH_MAP

    return ret;
}

status_t ExynosCameraMCPipe::setDeviceInfo(camera_device_info_t *deviceInfo)
{
    if (deviceInfo != NULL) {
        m_deviceInfo = deviceInfo;
    } else {
        m_deviceInfo = NULL;
    }

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_deviceInfo != NULL)
            m_pipeIdArr[i] = m_deviceInfo->pipeId[i];
        else
            m_pipeIdArr[i] = 0;
    }

    return NO_ERROR;
}

#ifdef USE_MCPIPE_SERIALIZATION_MODE
void ExynosCameraMCPipe::m_lockSerializeOperation(enum pipeline pipeId)
{
    if (m_serializeOperation == false) {
        return;
    }

    for (int i = ExynosCameraParameters::CRITICAL_SECTION_TYPE_START;
            i < ExynosCameraParameters::CRITICAL_SECTION_TYPE_END; i++) {
        if (m_parameters->isCriticalSection(pipeId,
                                            (enum ExynosCameraParameters::critical_section_type)i) == true) {
            CLOGD("Critical Section %d WAIT", i);
            ExynosCameraMCPipe::g_serializationLock[i].lock();
            CLOGD("Critical Section %d START", i);
        }
    }
}

void ExynosCameraMCPipe::m_unlockSerializeOperation(enum pipeline pipeId)
{
    if (m_serializeOperation == false) {
        return;
    }

    for (int i = ExynosCameraParameters::CRITICAL_SECTION_TYPE_END - 1;
            i >= ExynosCameraParameters::CRITICAL_SECTION_TYPE_START; i--) {
        if (m_parameters->isCriticalSection(pipeId,
                                            (enum ExynosCameraParameters::critical_section_type)i) == true) {
            ExynosCameraMCPipe::g_serializationLock[i].unlock();
            CLOGD("Critical Section %d END", i);
        }
    }
}
#endif


#ifdef SUPPORT_SENSOR_MODE_CHANGE
status_t ExynosCameraMCPipe::flush()
{
    status_t ret = NO_ERROR;
    CLOGD("start");

    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    if (m_inputFrameQ->getSizeOfProcessQ() > 0) {
        CLOGD("putBufferthread : requestExitAndWait");
        m_putBufferThread->requestExitAndWait();
    } else {
        CLOGD("putBufferthread : requestExit");
        m_putBufferThread->requestExit();

        Mutex::Autolock lock(m_putBufferLock);
    }

    CLOGD("putBufferThread exited");

    if (m_requestFrameQ->getSizeOfProcessQ() > 0) {
        m_getBufferThread->requestExitAndWait();
    } else {
        m_getBufferThread->requestExit();
    }

    CLOGD("m_getBufferThread exited");

    while(m_inputFrameQ->getSizeOfProcessQ() > 0) {
        m_inputFrameQ->waitAndPopProcessQ(&newFrame);
        CLOGD("[F%d] push Q to requestFrameQ", newFrame->getFrameCount());

        m_requestFrameQ->pushProcessQ(&newFrame);
    }
    m_inputFrameQ->release();

    bool skipCheckingRunningFrame = false;
    int runningFcnt = 0;

    while(m_requestFrameQ->getSizeOfProcessQ() > 0) {
        m_requestFrameQ->waitAndPopProcessQ(&newFrame);

        if (skipCheckingRunningFrame == false
                && (runningFcnt = m_getRunningFrameMinCount()) < newFrame->getFrameCount()) {
            CLOGD("runningFcnt(%d), newFrame(%d)", runningFcnt, newFrame->getFrameCount());
            m_getBufferThread->requestExitAndWait();
        }

        skipCheckingRunningFrame = true;

        newFrame->setFrameState(FRAME_STATE_SKIPPED);

        for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
            newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
        }

        ret = m_completeFrame(newFrame, false);
        if (ret != NO_ERROR) {
            CLOGE("Complete frame fail, frameCount(%d), ret(%d)", newFrame->getFrameCount(), ret);
            /* TODO: doing exception handling */
        }

        CLOGD("frameCount(%d)", newFrame->getFrameCount());

        if (m_frameDoneQ != NULL && m_flagFrameDoneQ == true)
            m_frameDoneQ->pushProcessQ(&newFrame);

        m_outputFrameQ->pushProcessQ(&newFrame);
    }

    m_requestFrameQ->release();
    CLOGD("inputFrameQ(%d), requestFrameQ(%d)",
            m_inputFrameQ->getSizeOfProcessQ(), m_requestFrameQ->getSizeOfProcessQ());

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        m_numOfRunningFrame[i] = 0;
        m_skipPutBuffer[i] = false;
        m_numBuffers[i] = 0;
#if defined(SUPPORT_VIRTUALFD_REPROCESSING) || defined(SUPPORT_VIRTUALFD_PREVIEW)
        m_sharedNode[i] = false;
#endif
        for (int j = 0; j < MAX_BUFFERS; j++)
            m_runningFrameList[i][j] = NULL;
    }

    CLOGD("end");
    return ret;
}

int ExynosCameraMCPipe::m_getRunningFrameMinCount(void)
{
    int theSmallestFrameCount = -1;
    int currentRunningFrameCount = -1;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            CLOGV("[%s] m_numBuffers[%d] : %d", m_deviceInfo->nodeName[i], i, m_numBuffers[i]);
            for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
                if (m_runningFrameList[i][j] == NULL) {
                    CLOGV("[%s] runningFrameList[%d][%d] is NULL", m_deviceInfo->nodeName[i], i, j);
                } else {
                    currentRunningFrameCount = m_runningFrameList[i][j]->getFrameCount();
                    CLOGD("[%s] runningFrameList[%d][%d]: fcount = %d",
                            m_deviceInfo->nodeName[i], i, j, currentRunningFrameCount);

                    if (theSmallestFrameCount == -1) {
                        theSmallestFrameCount = currentRunningFrameCount;
                    }

                    if(theSmallestFrameCount > currentRunningFrameCount) {
                        theSmallestFrameCount = currentRunningFrameCount;
                    }
                }
            }
        }
    }

    CLOGD("theSmallestFrameCount(%d)", theSmallestFrameCount);
    return theSmallestFrameCount;
}
#endif //SUPPORT_SENSOR_MODE_CHANGE
 
void ExynosCameraMCPipe::m_setSizeInfo(ExynosCameraFrameSP_sptr_t frame, camera2_shot_ext *shot_ext)
{
    if (shot_ext == nullptr) return;

    // TODO: only updating size info in case of preview instance
    //       For now, there's no requirement to get reprocessing size info
    if (m_isReprocessing()) return;

    camera2_node_group *node = &shot_ext->node_group;
    int vId = node->leader.vid;
    int pipeId =  m_videoIdToPipeIdMap[vId];
    frame_size_info_t info;

    switch (pipeId) {
    case PIPE_3AA:
        info.pipeId = pipeId;
        info.rect.x = node->leader.input.cropRegion[0];
        info.rect.y = node->leader.input.cropRegion[1];
        info.rect.w = node->leader.input.cropRegion[2];
        info.rect.h = node->leader.input.cropRegion[3];
        info.rect.fullW = info.rect.w + info.rect.x * 2;
        info.rect.fullH = info.rect.h + info.rect.y * 2;
        frame->setSizeInfo(FRAME_SIZE_BCROP_IN, info);

        info.rect.x = node->leader.output.cropRegion[0];
        info.rect.y = node->leader.output.cropRegion[1];
        info.rect.w = node->leader.output.cropRegion[2];
        info.rect.h = node->leader.output.cropRegion[3];
        info.rect.fullW = info.rect.w + info.rect.x * 2;
        info.rect.fullH = info.rect.h + info.rect.y * 2;
        frame->setSizeInfo(FRAME_SIZE_BCROP_OUT, info);

        // default setting
        info.pipeId = pipeId;
        info.rect.x = 0;
        info.rect.y = 0;
        info.rect.w = node->leader.output.cropRegion[2];
        info.rect.h = node->leader.output.cropRegion[3];
        info.rect.fullW = info.rect.w;
        info.rect.fullH = info.rect.h;
        frame->setSizeInfo(FRAME_SIZE_ISP_CROP_IN, info);

        // default setting
        info.pipeId = pipeId;
        info.rect.x = 0;
        info.rect.y = 0;
        info.rect.w = node->leader.output.cropRegion[2];
        info.rect.h = node->leader.output.cropRegion[3];
        info.rect.fullW = info.rect.w;
        info.rect.fullH = info.rect.h;
        frame->setSizeInfo(FRAME_SIZE_MCS_CROP_IN, info);
        break;
    case PIPE_ISP:
        info.pipeId = pipeId;
        info.rect.x = node->leader.input.cropRegion[0];
        info.rect.y = node->leader.input.cropRegion[1];
        info.rect.w = node->leader.input.cropRegion[2];
        info.rect.h = node->leader.input.cropRegion[3];
        info.rect.fullW = info.rect.w + info.rect.x * 2;
        info.rect.fullH = info.rect.h + info.rect.y * 2;
        frame->setSizeInfo(FRAME_SIZE_ISP_CROP_IN, info);

        // default setting
        info.pipeId = pipeId;
        info.rect.x = 0;
        info.rect.y = 0;
        info.rect.w = node->leader.output.cropRegion[2];
        info.rect.h = node->leader.output.cropRegion[3];
        info.rect.fullW = info.rect.w;
        info.rect.fullH = info.rect.h;
        frame->setSizeInfo(FRAME_SIZE_MCS_CROP_IN, info);
        break;
    case PIPE_MCSC:
        info.pipeId = pipeId;
        info.rect.x = node->leader.input.cropRegion[0];
        info.rect.y = node->leader.input.cropRegion[1];
        info.rect.w = node->leader.input.cropRegion[2];
        info.rect.h = node->leader.input.cropRegion[3];
        info.rect.fullW = info.rect.w + info.rect.x * 2;
        info.rect.fullH = info.rect.h + info.rect.y * 2;
        frame->setSizeInfo(FRAME_SIZE_MCS_CROP_IN, info);
        break;
    default:
        break;
    }

    for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
        vId = node->capture[i].vid;

        if (!vId) continue;
        if (!node->capture[i].request) continue;

        info.pipeId = -1;
        pipeId =  m_videoIdToPipeIdMap[vId];

        switch (pipeId) {
        case PIPE_3AP:
            info.pipeId = pipeId;
            info.rect.x = node->capture[i].output.cropRegion[0];
            info.rect.y = node->capture[i].output.cropRegion[1];
            info.rect.w = node->capture[i].output.cropRegion[2];
            info.rect.h = node->capture[i].output.cropRegion[3];
            info.rect.fullW = info.rect.w;
            info.rect.fullH = info.rect.h;
            frame->setSizeInfo(FRAME_SIZE_BDS, info);
            break;
        case PIPE_MCSC0 ... PIPE_MCSC4:
        {
            // HACK: set the size at first port
            //       skip if mcp info was set
            if (frame->getSizeInfo(FRAME_SIZE_MCP_CROP, info)) continue;

            frame_size_info_t leaderInfo;
            if (!frame->getSizeInfo(FRAME_SIZE_MCS_CROP_IN, leaderInfo)) continue;

            info.pipeId = pipeId;
            info.rect.x = node->capture[i].input.cropRegion[0];
            info.rect.y = node->capture[i].input.cropRegion[1];
            info.rect.w = node->capture[i].input.cropRegion[2];
            info.rect.h = node->capture[i].input.cropRegion[3];
            info.rect.fullW = leaderInfo.rect.w;
            info.rect.fullH = leaderInfo.rect.h;
            frame->setSizeInfo(FRAME_SIZE_MCP_CROP, info);

            info.rect.x = node->capture[i].output.cropRegion[0];
            info.rect.y = node->capture[i].output.cropRegion[1];
            info.rect.w = node->capture[i].output.cropRegion[2];
            info.rect.h = node->capture[i].output.cropRegion[3];
            info.rect.fullW = info.rect.w;
            info.rect.fullH = info.rect.h;
            frame->setSizeInfo(FRAME_SIZE_MCP_OUT, info);
            break;
        }
        default:
            break;
        }
    }
}

}; /* namespace android */
