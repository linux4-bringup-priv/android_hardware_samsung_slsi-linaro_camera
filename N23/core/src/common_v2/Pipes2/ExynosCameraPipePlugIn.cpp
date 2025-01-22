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
#define LOG_TAG "ExynosCameraPipePlugIn"
#include <log/log.h>

#include "ExynosCameraPipePlugIn.h"
#ifdef USES_DUAL_CAMERA_SOLUTION_FAKE
#include "ExynosCameraPlugInConverterFakeFusion.h"
#endif

namespace android {

ExynosCameraPipePlugIn::~ExynosCameraPipePlugIn()
{
    this->destroy();
}

status_t ExynosCameraPipePlugIn::create(__unused int32_t *sensorIds)
{
    status_t ret = NO_ERROR;

    ret = ExynosCameraSWPipe::create(sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("create was failed, ret(%d)", ret);
        return ret;
    }

    if (!m_supportMultiLibrary) {
        ret = m_create(m_scenario, sensorIds);
    }

    /* for debuging */
    m_runningFrameCount = 0;
    m_runningMetaFrameCount = 0;

    m_transitState(PIPE_STATE_CREATE);

    return ret;
}

status_t ExynosCameraPipePlugIn::destroy(void)
{
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    if (m_state < PIPE_STATE_CREATE) {
        CLOGE("already destroyed(%d)", m_state);
        return ret;
    }

    ExynosCameraPlugInSP_sptr_t          plugIn;
    ExynosCameraPlugInConverterSP_sptr_t plugInConverter;

    m_plugIn = NULL;
    m_plugInConverter = NULL;

    for (auto &item : m_plugInMap) {
        int scenario = item.first;
        m_joinPlugInThread(scenario);

        PlugInSP_sptr_t obj = item.second;
        if (obj == nullptr) {
            CLOG_ASSERT("this obj is null(0x%x)", scenario);
        }

        plugIn = obj->m_plugIn;
        plugInConverter = obj->m_plugInConverter;

        ret = plugIn->destroy();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            CLOGE("m_plugIn->destroy() fail(0x%x,%s)", scenario, plugIn->getName());
        }

        obj->m_plugIn = nullptr;
        obj->m_plugInConverter = nullptr;

        CLOGI("%s(0x%x)[refCnt%d] is destroied", plugIn->getName(), scenario, plugIn->getStrongCount());

        ret =  m_globalPlugInFactory->destroy(m_cameraId, plugIn, plugInConverter);
        funcRet |= ret;
        if (ret != NO_ERROR) {
            CLOGE("plugIn destroy failed(cameraId(%d), pipeId(%d), scenario(0x%x,%s)",
                    m_cameraId, m_pipeId, scenario, plugIn->getName());
        }
    }

    m_globalPlugInFactory = NULL;
    m_frameReorderMap.clear();

    ExynosCameraSWPipe::destroy();

    m_transitState(PIPE_STATE_NONE);

    return funcRet;
}

status_t ExynosCameraPipePlugIn::start(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    ret = ExynosCameraSWPipe::start();
    if (ret != NO_ERROR) {
        CLOGE("start was failed, ret(%d)", ret);
        return ret;
    }

    if (!m_supportMultiLibrary) {
        ret = m_start(m_plugIn);
        if (ret != NO_ERROR) {
            CLOGE("m_start() fail");
            return INVALID_OPERATION;
        }
    }

    m_transitState(PIPE_STATE_INIT);

    return ret;
}

status_t ExynosCameraPipePlugIn::stop(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    for (auto item : m_plugInMap) {
        Mutex::Autolock l(m_plugInStartLock);

        int scenario = item.first;
        m_joinPlugInThread(scenario);

        PlugInSP_sptr_t obj = item.second;
        if (obj == nullptr) {
            CLOG_ASSERT("this obj is null(0x%x)", scenario);
        }

        funcRet |= m_stop(obj->m_plugIn);

        m_startPlugInCounterMap[scenario] = 0;
    }

    CLOGI("done to destroy all plugIns");

    ret = ExynosCameraSWPipe::stop();
    funcRet |= ret;
    if (ret != NO_ERROR) {
        CLOGE("stop was failed, ret(%d)", ret);
    }

    CLOGI("done");

    m_transitState(PIPE_STATE_CREATE);

    return funcRet;
}

status_t ExynosCameraPipePlugIn::setupPipe(Map_t *map)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    if (!m_supportMultiLibrary) {
        ret = m_setupPipe(m_plugIn, m_plugInConverter, map);
    }

    m_plugInSetupMap = *map;

    return ret;
}

status_t ExynosCameraPipePlugIn::setParameter(int key, void *data)
{
    // this pipe need scenario
    if (m_supportMultiLibrary) {
        CLOG_ASSERT("invalid call!! cause of multiLibrary plugInPipe!!");
    }

    return m_setParameter(m_plugIn, key, data, m_scenario);
}

status_t ExynosCameraPipePlugIn::setParameter(int key, void *data, int scenario)
{
    // this pipe don't need scenario
    if (m_supportMultiLibrary == false) {
        CLOGW("invalid call!! this pipe is not multilibrary pipe(%x, %x)",
                m_scenario, scenario);
    }

    ExynosCameraPlugInSP_sptr_t plugIn = m_getPlugIn(scenario);
    if (plugIn == NULL) {
        // at first time, plugIn haven't been created..
        CLOGW("key(%d) plugIn(0x%x) is NULL", key, scenario);
    }

    return m_setParameter(plugIn, key, data, scenario);
}

status_t ExynosCameraPipePlugIn::getParameter(int key, void *data)
{
    // this pipe don't need scenario
    if (m_supportMultiLibrary) {
        CLOG_ASSERT("invalid call!! cause of multiLibrary plugInPipe!!");
    }

    return m_getParameter(m_plugIn, key, data, m_scenario);
}

status_t ExynosCameraPipePlugIn::getParameter(int key, void *data, int scenario)
{
    if (m_supportMultiLibrary == false) {
        CLOGW("invalid call!! this pipe is not multilibrary pipe(%x, %x)",
                m_scenario, scenario);
    }

    ExynosCameraPlugInSP_sptr_t plugIn = m_getPlugIn(scenario);
    if (plugIn == NULL) {
        // at first time, plugIn haven't been created..
        CLOGW("key(%d) plugIn(0x%x) is NULL", key, scenario);
    }

    return m_getParameter(plugIn, key, data, scenario);
}

status_t ExynosCameraPipePlugIn::m_run(void)
{
    status_t ret = NO_ERROR;

    Map_t map;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    entity_buffer_state_t dstBufState = ENTITY_BUFFER_STATE_ERROR;
    entity_state_t entityState = ENTITY_STATE_FRAME_DONE;
    handle_status_t handleRet;

    int scenario = 0;
    ExynosCameraPlugInSP_sptr_t          plugIn = nullptr;
    ExynosCameraPlugInConverterSP_sptr_t plugInConverter;

    ret = m_inputFrameQ->waitAndPopProcessQ(&frame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout(%d, %d)",
                    m_inputFrameQ->getSizeOfProcessQ(),
                    frame != NULL ? frame->getFrameCount() : -1);
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (frame == NULL) {
        CLOGE("new frame is NULL");
        return NO_ERROR;
    }

    /* for debuging */
    m_runningFrameCount = frame->getFrameCount();
    m_runningMetaFrameCount = frame->getMetaFrameCount();

    /* check the frame state */
    switch (frame->getFrameState()) {
    case FRAME_STATE_SKIPPED:
    case FRAME_STATE_INVALID:
        CFLOGE(frame, "frame state is invalid");
        goto func_exit;
        break;
    default:
        break;
    }

    // dynamic changing plugIn by scenario
    if (m_supportMultiLibrary) {
        scenario = frame->getPPScenario(m_pipeId);
        if (!scenario) {
            CFLOG_ASSERT(frame, "invalid scenario(%d)", scenario);
        }

        m_joinPlugInThread(scenario);

        PlugInSP_sptr_t item;
        status_t ret = m_getPlugInItem(scenario, &item);
        if (ret != NO_ERROR) {
            CFLOG_ASSERT(frame, "find plugIn(%d) fail", scenario);
        }

        plugIn = item->m_plugIn;
        plugInConverter = item->m_plugInConverter;

        ExynosCameraPlugIn::state st = plugIn->getState();
        switch (st) {
        case ExynosCameraPlugIn::PLUGIN_START:
        case ExynosCameraPlugIn::PLUGIN_PROCESS:
            break;
        default:
            CFLOG_ASSERT(frame, "invalid state(%d) of plugIn(0x%x,%s)",
                    st, scenario, plugIn->getName());
            break;
        }
    } else {
        scenario = m_scenario;
        plugIn = m_plugIn;
        plugInConverter = m_plugInConverter;
    }

    if (plugIn == NULL) {
        CFLOGE(frame, "plugIn is NULL");
        goto func_exit;
    }

    /*
     * 0. Frame reordering for multiple capture
     */
    handleRet = m_reorderFramebyFrameIndex(frame, (PlugInScenario)scenario);
    if (handleRet == PLUGIN_SKIP) {
        CLOGE("[F%d]skip frame", frame->getFrameCount());
        return NO_ERROR;
    } else if (handleRet == PLUGIN_ERROR) {
        CLOGE("[F%d]fail to reorderFrame", frame->getFrameCount());
        goto func_exit;
    }

    /*
     * 1. handling the frame
     *    logical area by each pipeId.
     */
    handleRet = m_handleFrameBefore(frame, plugIn, &map, scenario);
    switch (handleRet) {
    case PLUGIN_SKIP:
        dstBufState = ENTITY_BUFFER_STATE_COMPLETE;
    case PLUGIN_ERROR:
        goto func_exit;
        break;
    default:
        break;
    }

    /*
     * 2. run the plugIn
     */
    /* PROCESS_BEFORE */
    /* default map setting */
    map[PLUGIN_CONVERT_TYPE]        = (Map_data_t)PLUGIN_CONVERT_PROCESS_BEFORE;
    map[PLUGIN_CONVERT_FRAME]       = (Map_data_t)frame.get();
    map[PLUGIN_CONVERT_PARAMETER]   = (Map_data_t)m_parameters;
    map[PLUGIN_CONVERT_CONFIGURATIONS] = (Map_data_t)m_configurations;
    map[PLUGIN_CONVERT_BUFFERSUPPLIER] = (Map_data_t)m_bufferSupplier;
    map[PLUGIN_SCENARIO] = (Map_data_t)scenario;

    ret = plugInConverter->make(&map);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "plugIn->make(map) fail, %s", plugIn->getName());
        goto func_exit;
    }

    /* run process() in plugIn */
    ret = plugIn->process(&map);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "plugIn->process(map) fail, %s", plugIn->getName());
        if (ret == NO_INIT) {
            entityState = ENTITY_STATE_FRAME_SKIP;
        }
        goto func_exit;
    }

    /* PROCESS_AFTER */
    /* default map setting */
    map[PLUGIN_CONVERT_TYPE]        = (Map_data_t)PLUGIN_CONVERT_PROCESS_AFTER;

    ret = plugInConverter->make(&map);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "plugIn->make(map) fail, %s", plugIn->getName());
        goto func_exit;
    }

    handleRet = m_handleFrameAfter(frame, plugIn, &map, scenario);
    switch(handleRet) {
    case PLUGIN_ERROR:
        dstBufState = ENTITY_BUFFER_STATE_ERROR;
        break;
    case PLUGIN_SKIP:
    case PLUGIN_NO_ERROR:
    default:
        dstBufState = ENTITY_BUFFER_STATE_COMPLETE;
        break;
    }

    m_transitState(PIPE_STATE_RUN);

func_exit:

    /* for debuging */
    m_lastFrameCount = frame->getFrameCount();
    m_lastMetaFrameCount = frame->getMetaFrameCount();

    /*
     * 3. push the frame to outputQ
     */
    ret = frame->setDstBufferState(m_pipeId, dstBufState);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "state(%d) fail, %s", dstBufState, plugIn->getName());
    }

    ret = frame->setEntityState(m_pipeId, entityState);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "frame done fail, %s", plugIn->getName());
    }

    m_outputFrameQ->pushProcessQ(&frame);

    if (m_frameDoneQ != NULL)
        m_frameDoneQ->pushProcessQ(&frame);

    // in case of oneshot mode, release plugin's resource
    if (m_supportMultiLibrary
        && plugIn != nullptr
        && m_multiLibraryType == ExynosCameraPipePlugIn::ONESHOT
        && checkLastFrameForMultiFrameCapture(frame) == true) {
        m_frameReorderMap.clear();

        if (frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR) == true) {
            CFLOGV(frame, "skip to plugin stop");
        } else {
            int scenario = frame->getPPScenario(m_pipeId);

            CFLOGI(frame, "last frame[%d/%d] try to stop, plugIn(0x%x,%s)",
                frame->getFrameIndex(),
                frame->getMaxFrameIndex(),
                scenario,
                plugIn->getName());

            ret = m_updateSetCmd(plugIn, PLUGIN_PARAMETER_KEY_STOP, nullptr, scenario);
            if (ret != NO_ERROR) {
                CFLOGE(frame, "last frame[%d/%d] stop failed, plugIn(0x%x,%s)",
                        frame->getFrameIndex(),
                        frame->getMaxFrameIndex(),
                        scenario,
                        plugIn->getName());
            }
        }
    }

    return NO_ERROR;
}

ExynosCameraPipePlugIn::handle_status_t ExynosCameraPipePlugIn::m_reorderFramebyFrameIndex(__unused ExynosCameraFrameSP_sptr_t frame,
                                                                                                        PlugInScenario scenario)
{
    int frameMaxIndex = frame->getMaxFrameIndex();
    int frameCurIndex = frame->getFrameIndex();
    bool skipCurFrame = false;

    switch (scenario) {
    case PLUGIN_SCENARIO_COMBINE_REPROCESSING:
        CLOGV("[F%d T%d]frames re-Ordering(frameCurIndex:%d / frameMaxIndex:%d)",
                frame->getFrameCount(), frame->getFrameType(),
                frameCurIndex, frameMaxIndex);
        break;
    default:
        return PLUGIN_NO_ERROR;
    }

    if (frameMaxIndex <= 1) {
        if (m_frameReorderMap.empty() == false) {
            CLOGW("[F%d T%d]The frames of the previous capture remain. frameReorderMap is not empty(%d). ",
                    frame->getFrameCount(), frame->getFrameType(), m_frameReorderMap.size());
            m_frameReorderMap.clear();
        }
        return PLUGIN_NO_ERROR;
    }

    /* The key value of frameReorderMap must be set to frameMaxIndex. */
    if (m_frameReorderMap.empty() == true) {
        for (int i = 0; i < frameMaxIndex; i++) {
            m_frameReorderMap[i] = NULL;
        }
    }

    if (frameCurIndex < frameMaxIndex) {
        map<int, ExynosCameraFrameSP_sptr_t>::iterator reorderIter = m_frameReorderMap.begin();
        int reorderIndex = reorderIter->first;

        if (frameCurIndex != reorderIndex) {
            /*
             * If frameCurIndex and reorderIndex are different,
             * plugin skip after frame push to m_frameReorderMap.
             */
            CLOGD("[F%d T%d]need to frame reordering(frameCurIndex:%d != reorderIndex:%d)",
                    frame->getFrameCount(), frame->getFrameType(), frameCurIndex, reorderIndex);

            map<int, ExynosCameraFrameSP_sptr_t>::iterator oldReorderIter = m_frameReorderMap.find(frameCurIndex);
            if (oldReorderIter == m_frameReorderMap.end()) {
                CLOGE("[F%d]fail to find frame(%d)", frame->getFrameCount(), frame->getMaxFrameIndex());
                return PLUGIN_ERROR;
            }

            m_frameReorderMap.erase(oldReorderIter);
            m_frameReorderMap[frameCurIndex] = frame;
            skipCurFrame = true;
        } else {
            /*
             * If frameCurIndex and reorderIndex are the same,
             * remove the null frame that matches the reorderIndex in frameReorderMap.
             */
            CLOGD("[F%d T%d]Do not need to frame reordering(frameCurIndex:%d / frameMaxIndex:%d)",
                    frame->getFrameCount(), frame->getFrameType(), frameCurIndex, frameMaxIndex);
            m_frameReorderMap.erase(reorderIter);

            /* Push the next frames of frameReorderMap to inpuFrameQ. */
            ExynosCameraFrameSP_sptr_t reorderFrame = m_frameReorderMap[++reorderIndex];
            while (reorderFrame != NULL) {
                CLOGD("[F%d T%d]old frame push to inputFrameQ(reorderIndex:%d / frameMaxIndex:%d)",
                        reorderFrame->getFrameCount(), reorderFrame->getFrameType(), reorderIndex, frameMaxIndex);

                m_inputFrameQ->pushProcessQ(&reorderFrame);
                m_frameReorderMap.erase(reorderIndex);
                m_frameReorderMap[reorderIndex] = NULL;

                if (reorderIndex < frameMaxIndex) {
                    reorderFrame = m_frameReorderMap[++reorderIndex];
                } else {
                    break;
                }
            }
        }
    } else {
        CLOGE("[F%d T%d]frameCurIndex(%d) can not be larger than frameMaxIndex(%d)",
                frame->getFrameCount(), frame->getFrameType(),
                frameCurIndex, frameMaxIndex);
        return PLUGIN_ERROR;
    }

    return (skipCurFrame == true) ? PLUGIN_SKIP : PLUGIN_NO_ERROR;
}

ExynosCameraPipePlugIn::handle_status_t ExynosCameraPipePlugIn::m_handleFrameBefore(__unused ExynosCameraFrameSP_dptr_t frame,
                                                                                    __unused ExynosCameraPlugInSP_sptr_t plugIn,
                                                                                    __unused Map_t *map,
                                                                                    __unused int scenario)
{
    status_t ret;
    handle_status_t handleRet = PLUGIN_NO_ERROR;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;

    /* Initialize */
    (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
    (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Data_int32_t)1;

    switch (scenario) {
#ifdef USE_DUAL_CAMERA
    case PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING:
        if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
            (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)2;
            (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] =
                (m_configurations->getMode(CONFIGURATION_DUAL_BOKEH_REFOCUS_MODE) == true) ?
                (Data_int32_t)3 : (Data_int32_t)1;
        }
        break;
#ifdef USES_DUAL_CAMERA_SOLUTION_FAKE
    case PLUGIN_SCENARIO_FAKEFUSION_PREVIEW:
    case PLUGIN_SCENARIO_FAKEFUSION_REPROCESSING:
#endif
#ifdef USES_DUAL_CAMERA_SOLUTION_ARCSOFT
    case PLUGIN_SCENARIO_ZOOMFUSION_PREVIEW:
    case PLUGIN_SCENARIO_ZOOMFUSION_REPROCESSING:
    case PLUGIN_SCENARIO_BOKEHFUSION_PREVIEW:
    case PLUGIN_SCENARIO_BOKEHFUSION_REPROCESSING:
#endif
    case PLUGIN_SCENARIO_COMBINE_PREVIEW:
    {
#if 1
        switch (frame->getFrameType()) {
        case FRAME_TYPE_PREVIEW_DUAL_MASTER:
        case FRAME_TYPE_REPROCESSING_DUAL_MASTER:
            (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)2;
            break;
        default:
            (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
            break;
        }
#else

        ExynosCameraBuffer srcBuffer;
        ExynosCameraBuffer dstBuffer;

        ret = frame->getDstBuffer(getPipeId(), &dstBuffer);
        if(ret != NO_ERROR) {
            CLOGE("frame[%d]->getDstBuffer(%d) fail, ret(%d)",
                frame->getFrameCount(), getPipeId(), ret);
            handleRet = PLUGIN_ERROR;
        }

        ret = frame->getSrcBuffer(getPipeId(), &srcBuffer);
        if(ret != NO_ERROR) {
            CLOGE("frame[%d]->getSrcBuffer1(%d) fail, ret(%d)",
                frame->getFrameCount(), getPipeId(), ret);
            handleRet = PLUGIN_ERROR;
        }

        for(int i = 0; i < srcBuffer.planeCount; i++) {
            if(srcBuffer.fd[i] > 0) {
                int copySize = srcBuffer.size[i];
                if(dstBuffer.size[i] < copySize) {
                    CLOGW("dstBuffer.size[%d](%d) < copySize(%d). so, copy only dstBuffer.size[%d]",
                        i, dstBuffer.size[i], copySize, i);
                    copySize = dstBuffer.size[i];
                }

                memcpy(dstBuffer.addr[i], srcBuffer.addr[i], copySize);
            }
        }

        handleRet = PLUGIN_SKIP;
#endif
        break;
    }
#endif // USE_DUAL_CAMERA

#ifdef USES_SW_VDIS
    case PLUGIN_SCENARIO_SWVDIS_PREVIEW:
        ret = m_plugIn->query(map);
        break;
#endif //USES_SW_VDIS
    case PLUGIN_SCENARIO_HIFILLS_REPROCESSING:
    case PLUGIN_SCENARIO_COMBINE_REPROCESSING:
        // MF-still
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
        break;
    case PLUGIN_SCENARIO_HIFI_REPROCESSING:
        // yuv reprocessing
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
        break;
#ifdef USES_CAMERA_EXYNOS_VPL
    case PIPE_NFD:
    case PIPE_NFD_REPROCESSING:
        ret = m_plugIn->query(map);

        newFrame = frame;

        if (m_inputFrameQ->getSizeOfProcessQ() > 0) {
            /* Skip delayed frames */
            do {
                CLOGV("[F%d] NFD pipe skip frame", newFrame->getFrameCount());
                ret = newFrame->setEntityState(m_pipeId, ENTITY_STATE_FRAME_SKIP);
                if (ret != NO_ERROR) {
                    CLOGE("frame->setEntityState(%s, ENTITY_STATE_FRAME_DONE) fail, frameCount(%d)",
                            this->getPipeName(), frame->getFrameCount());
                }

                m_outputFrameQ->pushProcessQ(&newFrame);

                newFrame = NULL;
                ret = m_inputFrameQ->popProcessQ(&newFrame);
                if (ret != NO_ERROR)
                    CLOGE("Failed to popProcessQ. ret %d", ret);
            }  while (m_inputFrameQ->getSizeOfProcessQ() > 0);
        }

        if (newFrame == NULL) {
            CLOGE("frame is NULL");
            handleRet = PLUGIN_ERROR;
        }

        frame = newFrame;
        break;
#endif
#if defined(USES_CAMERA_EXYNOS_LEC)
    case PIPE_PLUGIN_LEC_REPROCESSING:
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Data_int32_t)1;
        break;
#endif
    default:
        CFLOGW(frame, "Unknown scenario, plugIn(0x%x,%s)",
                scenario,
                plugIn->getName());
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Data_int32_t)1;
        break;
    }

    int dstCnt = (Data_int32_t)(*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT];

    if (m_pipeId == PIPE_PLUGIN_POST1_REPROCESSING) {
        ExynosCameraBuffer srcBuffer;
        ExynosCameraBuffer dstBuffer;
        ret = frame->getDstBuffer(getPipeId(), &dstBuffer);
        ret = frame->getSrcBuffer(getPipeId(), &srcBuffer);
        CFLOGE(frame, "dst:%d, scenario%d, (%d,%s) -> (%d,%s)", dstCnt, scenario,
                srcBuffer.index, srcBuffer.bufMgrNm,
                dstBuffer.index, dstBuffer.bufMgrNm);
    }

    //copy meta to dst(default)
    ret = m_copyMeta(frame, dstCnt);
    if (ret != NO_ERROR) {
        CFLOGW(frame, "invalid meta, plugIn(0x%x,%s)",
                scenario,
                plugIn->getName());
    }

    return handleRet;
}

ExynosCameraPipePlugIn::handle_status_t ExynosCameraPipePlugIn::m_handleFrameAfter(__unused ExynosCameraFrameSP_dptr_t frame,
                                                                                   __unused ExynosCameraPlugInSP_sptr_t plugIn,
                                                                                   __unused Map_t *map,
                                                                                   __unused int scenario)
{
    status_t ret = NO_ERROR;
    handle_status_t handleRet = PLUGIN_NO_ERROR;

    switch (scenario) {
#ifdef USES_SW_VDIS
    case PLUGIN_SCENARIO_SWVDIS_PREVIEW:
        {
            buffer_manager_tag_t initTag;
            ExynosCameraBuffer srcBuffer;
            ExynosCameraBuffer dstBuffer;
            entity_buffer_state_t bufferState = ENTITY_BUFFER_STATE_NOREQ;

            int32_t srcBufIndex = -1;
            int32_t dstBufValid = -1;


            //Handle SRC buffer
            ret = frame->getSrcBuffer(m_pipeId, &srcBuffer);
            if (ret < NO_ERROR) {
                CFLOGE(frame, "getBuffer fail, pipeId(%d), ret(%d), plugIn(0x%x,%s)",
                        m_pipeId, ret, frame->getFrameCount(), scenario, plugIn->getName());
            }

            srcBufIndex = (Data_uint32_t)(*map)[PLUGIN_SRC_BUF_USED];

            ret = frame->setSrcBufferState(m_pipeId, ENTITY_BUFFER_STATE_REQUESTED);
            if (ret != NO_ERROR) {
                CFLOGE(frame, "Failed setSrcBufferState(%d), pipeId(%d), ret(%d), plugIn(0x%x,%s)",
                        srcBuffer.index, m_pipeId, ret, frame->getFrameCount());
            }

            if (srcBufIndex != -1) {
                CFLOGV(frame, "release buffer(%d), plugIn(0x%x,%s)", srcBufIndex, scenario, plugIn->getName());

                srcBuffer.index = srcBufIndex;

                bufferState = ENTITY_BUFFER_STATE_COMPLETE;
            } else {
                CFLOGW(frame, "Skip release buffer(%d) buffer_used(%d), plugIn(0x%x,%s)",
                        srcBuffer.index, srcBufIndex, scenario, plugIn->getName());
                bufferState = ENTITY_BUFFER_STATE_PROCESSING;
            }

            ret = frame->setSrcBuffer(m_pipeId, srcBuffer);
            if (ret != NO_ERROR) {
                CFLOGE(frame, "Failed setSrcBuffer(%d), pipeId(%d), ret(%d), plugIn(0x%x,%s)",
                        srcBuffer.index, m_pipeId, ret, scenario, plugIn->getName());
            }

            ret = frame->setSrcBufferState(m_pipeId, bufferState);
            if (ret != NO_ERROR) {
                CFLOGE(frame, "Failed setSrcBufferState(%d), pipeId(%d), ret(%d), plugIn(0x%x,%s)",
                        srcBuffer.index, m_pipeId, ret, scenario, plugIn->getName());
            }

            //Handle DST buffer
            dstBufValid = (Data_uint32_t)(*map)[PLUGIN_DST_BUF_VALID];
            if (dstBufValid >= 0) {
                handleRet = PLUGIN_NO_ERROR;
            } else {
                CFLOGE(frame, "Invalid DST buffer(%d), plugIn(0x%x,%s)", dstBufValid, scenario, plugIn->getName());
                handleRet = PLUGIN_ERROR;
            }
        }
        break;
#endif
    case PLUGIN_SCENARIO_HIFILLS_REPROCESSING:
        {
        }
        break;
#if defined(USES_CAMERA_EXYNOS_LEC)
    case PIPE_PLUGIN_LEC_REPROCESSING:
        ret = frame->setEntityState(m_pipeId, ENTITY_STATE_COMPLETE);
        if (ret < 0) {
            CLOGE("setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                    m_pipeId, ENTITY_STATE_COMPLETE, ret);
        }
        break;
#endif
    default:
        break;
    }

    return handleRet;
}

void ExynosCameraPipePlugIn::m_init(__unused int32_t *nodeNums)
{
    /* get the singleton instance for plugInFactory */
    m_globalPlugInFactory = ExynosCameraSingleton<ExynosCameraFactoryPlugIn>::getInstance();

    m_plugIn = NULL;
    m_plugInConverter = NULL;
    m_multiLibraryType = ExynosCameraPipePlugIn::STREAM;
    m_frameReorderMap.clear();

    // TODO:
    // User could set the value of multiLibraryType by other API.
    // For now, It has dependancy of reprocessing flag.
    if (m_supportMultiLibrary && m_isReprocessing()) {
        m_multiLibraryType = ExynosCameraPipePlugIn::ONESHOT;
    }
}

status_t ExynosCameraPipePlugIn::m_setup(__unused Map_t *map)
{
    status_t ret = NO_ERROR;

    switch (m_pipeId) {
#ifdef USE_DUAL_CAMERA
    case PIPE_FUSION:
        float** outImageRatioList;
        int**   outNeedMarginList;
        int*    sizeOfList;

        /* HACK */
        if ((m_cameraId == CAMERA_ID_BACK_2) || (m_cameraId == CAMERA_ID_BACK_3)) {
            //break;
        }

        outImageRatioList = (Array_float_addr_t)(*map)[PLUGIN_IMAGE_RATIO_LIST];
        outNeedMarginList = (Array_pointer_int_t)(*map)[PLUGIN_NEED_MARGIN_LIST];
        sizeOfList = (Array_buf_t)(*map)[PLUGIN_ZOOM_RATIO_LIST_SIZE];

        if (outImageRatioList == NULL || outNeedMarginList == NULL || sizeOfList == NULL) {
            CLOGE("Ratio list is NUL");
        } else {

            CLOGD("sizeOfList(%d)", sizeOfList[0]);

            for(int i = 0; i < sizeOfList[0]; i++) {
                CLOGD(" ImageRatio[%d](%f)(%f), Margin[%d](%d)(%d)",
                    i, outImageRatioList[0][i], outImageRatioList[1][i],
                    i, outNeedMarginList[0][i], outNeedMarginList[1][i]);
            }
#ifdef USE_ARCSOFT_FUSION_LIB
            m_parameters->setPreviewNeedMarginList(outNeedMarginList, sizeOfList[0]);
            m_parameters->setImageRatioList(outImageRatioList, sizeOfList[0]);
#endif
        }

        break;

    case PIPE_FUSION_FRONT:
    case PIPE_FUSION_REPROCESSING:
        break;
#endif
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_create(int scenario, __unused int32_t *sensorIds)
{
    status_t ret = NO_ERROR;
    ExynosCameraPlugInSP_sptr_t plugIn;
    ExynosCameraPlugInConverterSP_sptr_t plugInConverter;

    if (m_globalPlugInFactory == NULL) {
        CLOGE("m_globalPlugInFactory is NULL!!");
        return INVALID_OPERATION;
    }

    /* create the plugIn by pipeId */
    ret =  m_globalPlugInFactory->create(m_cameraId, m_pipeId, plugIn, plugInConverter, scenario);
    if (ret != NO_ERROR || plugIn == NULL || plugInConverter == NULL) {
        CLOGE("plugIn create failed(cameraId(%d), pipeId(%d), scenario(0x%x)",
                m_cameraId, m_pipeId, scenario);
        return INVALID_OPERATION;
    }

    // keep created plugIn
    if (m_supportMultiLibrary == false) {
        if (m_plugIn) {
            CLOG_ASSERT("already plugIn exist!!! (%d)", scenario);
        }
        m_plugIn = plugIn;
        m_plugInConverter = plugInConverter;
    }

    PlugInSP_sptr_t item;
    item = new PlugInObject(plugIn, plugInConverter);
    m_pushPlugInItem(scenario, item);

    /* run create() in plugIn */
    ret = plugIn->create();
    if (ret != NO_ERROR) {
        CLOGE("plugIn(%s)->create() fail", plugIn->getName());
        return INVALID_OPERATION;
    }

    /* query() in plugIn */
    Map_t map;
    ret = plugIn->query(&map);
    if (ret != NO_ERROR) {
        CLOGE("plugIn(%s)->query() fail", plugIn->getName());
        return INVALID_OPERATION;
    }

#if defined(USES_COMBINE_PLUGIN) && defined(USE_DUAL_CAMERA)
    if (scenario == PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
        map[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)3;
    }
#endif

    CLOGD("Info[scenario:0x%x(%s), v%d.%d, libname:%s, build time:%s %s, buf(src:%d, dst:%d)] multi:%d",
            scenario,
            plugIn->getName(),
            GET_MAJOR_VERSION((Data_uint32_t)map[PLUGIN_VERSION]),
            GET_MINOR_VERSION((Data_uint32_t)map[PLUGIN_VERSION]),
            (Pointer_const_char_t)map[PLUGIN_LIB_NAME],
            (Pointer_const_char_t)map[PLUGIN_BUILD_DATE],
            (Pointer_const_char_t)map[PLUGIN_BUILD_TIME],
            (Data_int32_t)map[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT],
            (Data_int32_t)map[PLUGIN_PLUGIN_MAX_DST_BUF_CNT],
            m_supportMultiLibrary);

    ret = plugInConverter->create(&map);
    if (ret != NO_ERROR) {
        CLOGE("plugInConverter->create() fail, plugIn(0x%x,%s)", scenario, plugIn->getName());
        return INVALID_OPERATION;
    }

    m_frameReorderMap.clear();

    return NO_ERROR;
}

status_t ExynosCameraPipePlugIn::m_start(ExynosCameraPlugInSP_sptr_t plugIn)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    if (plugIn == NULL) {
        CLOGE("plugIn is NULL");
        return INVALID_OPERATION;
    }

    /* run start() in plugIn */
    if (ExynosCameraPlugIn::PLUGIN_START == plugIn->getState()) {
        CLOGE("plugIn(%s) PLUGIN_START skipped, already START", plugIn->getName());
    } else {
        ret = plugIn->start();
        if (ret != NO_ERROR) {
            CLOGE("plugIn(%s)->start() fail", plugIn->getName());
            return INVALID_OPERATION;
        }
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_stop(ExynosCameraPlugInSP_sptr_t plugIn)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    if (plugIn == NULL) {
        CLOGE("plugIn is NULL");
        return INVALID_OPERATION;
    }

    /* run stop() in plugIn */
    if (ExynosCameraPlugIn::PLUGIN_STOP == plugIn->getState()) {
        CLOGE("plugIn(%s) PLUGIN_STOP skipped, already STOP", plugIn->getName());
    } else {
        ret = plugIn->stop();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            CLOGE("plugIn(%s)->stop() fail", plugIn->getName());
        }
    }

    m_frameReorderMap.clear();

    return funcRet;
}

status_t ExynosCameraPipePlugIn::m_setupPipe(ExynosCameraPlugInSP_sptr_t plugIn, ExynosCameraPlugInConverterSP_sptr_t plugInConverter, Map_t *map)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    if (plugInConverter == NULL) {
        CLOGE("plugInConverter is NULL");
        return INVALID_OPERATION;
    }

    if (plugIn == NULL) {
        CLOGE("plugIn is NULL");
        return INVALID_OPERATION;
    }

    if (map == NULL) {
        CLOGE("map is NULL, plugIn(%s)", plugIn->getName());
        return INVALID_OPERATION;
    }

    /* prepare map */
    (*map)[PLUGIN_CONVERT_TYPE]        = (Map_data_t)PLUGIN_CONVERT_SETUP_BEFORE;
    (*map)[PLUGIN_CONVERT_PARAMETER]   = (Map_data_t)m_parameters;
    (*map)[PLUGIN_CONVERT_CONFIGURATIONS]   = (Map_data_t)m_configurations;
    ret = plugInConverter->setup(map);
    if (ret != NO_ERROR) {
        CLOGE("plugInConverter->setup() fail, plugIn(%s)", plugIn->getName());
        return INVALID_OPERATION;
    }

    /* run setup() in plugIn */
    ret = plugIn->setup(map);
    if (ret != NO_ERROR) {
        CLOGE("plugIn(%s)->setup() fail", plugIn->getName());
        return INVALID_OPERATION;
    }

    /* save the result to map */
    (*map)[PLUGIN_CONVERT_TYPE]        = (Map_data_t)PLUGIN_CONVERT_SETUP_AFTER;
    ret = plugInConverter->setup(map);
    if (ret != NO_ERROR) {
        CLOGE("plugInConverter->setup() fail, plugIn(%s)", plugIn->getName());
        return INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_setParameter(ExynosCameraPlugInSP_sptr_t plugIn, int key, void *data, int scenario)
{
    m_updateSetCmd(plugIn, key, data, scenario);

    if (plugIn)
        return plugIn->setParameter(key, data);
    else
        return NO_ERROR;
}

status_t ExynosCameraPipePlugIn::m_getParameter(ExynosCameraPlugInSP_sptr_t plugIn, int key, void *data, int scenario)
{
    m_updateGetCmd(plugIn, key, data, scenario);

    if (plugIn)
        return plugIn->getParameter(key, data);
    else
        return NO_ERROR;
}

ExynosCameraPlugInSP_sptr_t ExynosCameraPipePlugIn::m_getPlugIn(int key)
{
    ExynosCameraPlugInSP_sptr_t plugIn;

    if (m_supportMultiLibrary) {
        PlugInSP_sptr_t item;
        status_t ret = m_getPlugInItem(key, &item);
        if (ret != NO_ERROR) {
            CLOGW("find plugIn(0x%x) fail", key);
        } else {
            plugIn = item->m_plugIn;
        }
    } else {
        plugIn = m_plugIn;
    }

    return plugIn;
}

status_t ExynosCameraPipePlugIn::m_pushPlugInItem(int key, PlugInSP_sptr_t item)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock l(m_plugInMapLock);

    if (m_plugInMap.count(key)) {
        CLOGE("already exist same plugIn.. key(0x%x)", key);
        ret = INVALID_OPERATION;
    } else {
        m_plugInMap[key] = item;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_popPlugInItem(int key, PlugInSP_sptr_t *item)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock l(m_plugInMapLock);

    if (m_plugInMap.count(key)) {
        *item = m_plugInMap[key];
        m_plugInMap.erase(key);
    } else {
        CLOGE("pop plugInMap failed fail key(0x%x)", key);
        ret = INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_getPlugInItem(int key, PlugInSP_sptr_t *item)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock l(m_plugInMapLock);

    if (m_plugInMap.count(key)) {
        *item = m_plugInMap[key];
    } else {
        CLOGW("get plugInMap failed fail key(0x%x)", key);
        ret = INVALID_OPERATION;
    }

    return ret;
}

bool ExynosCameraPipePlugIn::m_findPlugInItem(int key)
{
    Mutex::Autolock l(m_plugInMapLock);

    return m_plugInMap.count(key);
}

status_t ExynosCameraPipePlugIn::m_updateSetCmd(ExynosCameraPlugInSP_sptr_t plugIn, int cmd, void *data, int scenario)
{
    status_t ret = NO_ERROR;

    if (m_supportMultiLibrary) {
    }

    switch(cmd) {
    case PLUGIN_PARAMETER_KEY_PREPARE:
        if (m_supportMultiLibrary) {
            CLOGD("command PLUGIN_PARAMETER_KEY_PREPARE(scenario:0x%x,%d)",
                    scenario, m_startPlugInCounterMap[scenario]);

            // 2. check loaderThread
            Mutex::Autolock l(m_plugInMapLock);
            if (m_loaderPlugInThreadMap.count(scenario)) {
                CLOGD("command PLUGIN_PARAMETER_KEY_PREPARE already load plugin(0x%x)", scenario);
            } else {
                // new
                Thread_t loaderThread;
                Thread_t startThread;
                loaderThread = new ExynosCameraArgThread<ExynosCameraPipePlugIn>(this,
                        &ExynosCameraPipePlugIn::m_loadPlugInThreadFunc, "LoaderPlugInThread", reinterpret_cast<void *>(scenario));
                startThread = new ExynosCameraArgThread<ExynosCameraPipePlugIn>(this,
                        &ExynosCameraPipePlugIn::m_startPlugInThreadFunc, "StartPlugInThread", reinterpret_cast<void *>(scenario));
                loaderThread->run(PRIORITY_URGENT_DISPLAY);
                m_loaderPlugInThreadMap[scenario] = loaderThread;
                m_startPlugInThreadMap[scenario] = startThread;
            }
        }
        break;
    case PLUGIN_PARAMETER_KEY_START:
        if (m_supportMultiLibrary) {
            Mutex::Autolock l(m_plugInStartLock);

            CLOGD("command PLUGIN_PARAMETER_KEY_START(scenario:%d,%d)",
                    scenario, m_startPlugInCounterMap[scenario]);

            if (m_startPlugInCounterMap[scenario] == 0) {
                Thread_t startThread = m_startPlugInThreadMap[scenario];
                if (startThread == nullptr) {
                    CLOGE("startThread(%d) is null..", scenario);
                    break;
                } else {
                    startThread->join();
                }
                startThread->run();
            }
            m_startPlugInCounterMap[scenario]++;
        }
        break;
    case PLUGIN_PARAMETER_KEY_STOP:
        if (m_supportMultiLibrary && plugIn) {
            Mutex::Autolock l(m_plugInStartLock);

            CLOGD("command PLUGIN_PARAMETER_KEY_STOP(scenario:0x%x,%d)",
                    scenario, m_startPlugInCounterMap[scenario]);

            if (m_startPlugInCounterMap[scenario] == 1) {
                /* 0. thread join */
                m_joinPlugInThread(scenario);

                /* 1. stop plugin */
                status_t funcRet = plugIn->setParameter(cmd, data);
                if (funcRet != NO_ERROR) {
                    CLOGE("m_setParameter(0x%x) failed(%d)", scenario, ret);
                }
                ret |= funcRet;

                funcRet = m_stop(plugIn);
                if (funcRet != NO_ERROR) {
                    CLOGE("plugIn(0x%x) is NULL, skipped stop", scenario);
                }
                ret |= funcRet;
            }
            if (m_startPlugInCounterMap[scenario] > 0)
                m_startPlugInCounterMap[scenario]--;
        }
        break;
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_updateGetCmd(ExynosCameraPlugInSP_sptr_t plugIn, int cmd, void *data, int scenario)
{
    status_t ret = NO_ERROR;

    switch(cmd) {
    case PLUGIN_PARAMETER_KEY_GET_SCENARIO:
        {
            CLOGD("command PLUGIN_PARAMETER_KEY_GET_SCENARIO scenario(0x%x)", scenario);
            *(int*)data = m_scenario;
        }
        break;
    default:
        break;
    }

    return ret;
}

status_t ExynosCameraPipePlugIn::m_setScenario(int scenario)
{
    status_t ret = NO_ERROR;
    if (m_scenario != scenario)
        CLOGD("change scenario(0x%x -> 0x%x)", m_scenario, scenario);

    m_scenario = scenario;
    return ret;
}

status_t ExynosCameraPipePlugIn::m_getScenario(int& scenario)
{
    status_t ret = NO_ERROR;
    scenario = m_scenario;
    return ret;
}

bool ExynosCameraPipePlugIn::m_loadPlugInThreadFunc(void *data)
{
    int scenario = (int)(size_t)data;

    CLOGD("loadPlugIn(0x%x) start", scenario);

    // 1. create
    m_create(scenario);

    CLOGD("loadPlugIn(0x%x) end", scenario);

    return false;
}

bool ExynosCameraPipePlugIn::m_startPlugInThreadFunc(void *data)
{
    status_t ret;
    PlugInSP_sptr_t item;
    int scenario = (int)(size_t)data;

    CLOGD("startPlugIn(0x%x) start", scenario);

    Thread_t loaderThread = m_loaderPlugInThreadMap[scenario];
    if (loaderThread == nullptr) {
        CLOGE("can't find loaderThread(0x%x)", scenario);
        goto func_exit;
    }

    // 1. join previous loaderThread
    loaderThread->join();

    ret = m_getPlugInItem(scenario, &item);
    if (ret != NO_ERROR) {
        CLOGE("can't find plugIn(0x%x)", scenario);
    } else {
        ExynosCameraPlugInSP_sptr_t plugIn = item->m_plugIn;

        // 2. setup
        ret = m_setupPipe(plugIn, item->m_plugInConverter, &m_plugInSetupMap);
        if (ret != NO_ERROR) {
            CLOGE("m_start(0x%x,%s) failed(%d)", scenario, plugIn->getName(), ret);
            goto func_exit;
        }

        // 3. m_start
        ret = m_start(item->m_plugIn);
        if (ret != NO_ERROR) {
            CLOGE("m_start(0x%x,%s) failed(%d)", scenario, plugIn->getName(), ret);
            goto func_exit;
        }

        ret = item->m_plugIn->setParameter(PLUGIN_PARAMETER_KEY_START, &scenario);
        if (ret != NO_ERROR) {
            CLOGE("m_setParameter(0x%x,%s) failed(%d)", scenario, plugIn->getName(), ret);
            goto func_exit;
        }
    }

    CLOGD("startPlugIn(0x%x) end", scenario);

func_exit:

    return false;
}

status_t ExynosCameraPipePlugIn::m_copyMeta(ExynosCameraFrameSP_sptr_t frame, int dstCnt)
{
    if (dstCnt <= 0) return NO_ERROR;

    status_t ret;
    ExynosCameraBuffer srcBuffer;
    int pos = OUTPUT_NODE_1;

#ifdef USE_DUAL_CAMERA
    if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
        const struct camera2_shot_ext *shotExt = frame->getConstMeta();
        // update dst meta as actual master camera's meta
        if (frame->getCameraId(OUTPUT_NODE_2) == frame->getDisplayCameraId()) {
            pos = OUTPUT_NODE_2;
        }
    }
#endif

    ret = frame->getSrcBuffer(getPipeId(), &srcBuffer, pos);
    if (ret != NO_ERROR) {
        CFLOGE(frame, "getSrcBuffer(%d) fail, ret(%d)", getPipeId(), ret);
        return BAD_VALUE;
    }

    for (int i = 0; dstCnt > 0 && i < MAX_CAPTURE_NODE; i++) {
        ExynosCameraBuffer dstBuffer;
        ret = frame->getDstBuffer(getPipeId(), &dstBuffer, i);
        if (ret != NO_ERROR) {
            CFLOGE(frame, "getDstBuffer(%d) fail, ret(%d)", getPipeId(), ret);
        } else {
            if (srcBuffer.index >= 0 && dstBuffer.index >= 0) {
                memcpy(dstBuffer.addr[dstBuffer.getMetaPlaneIndex()],
                        srcBuffer.addr[srcBuffer.getMetaPlaneIndex()],
                        dstBuffer.size[dstBuffer.getMetaPlaneIndex()]);
                dstCnt--;
            }
        }
    }

    return ret;
}

void ExynosCameraPipePlugIn::m_joinPlugInThread(int scenario)
{
    CLOGD("wait start");

    Thread_t loaderThread = m_loaderPlugInThreadMap[scenario];
    Thread_t startThread = m_startPlugInThreadMap[scenario];

    CLOGD("wait for loaderThread");
    if (loaderThread) loaderThread->join();

    CLOGD("wait for startThread");
    if (startThread) startThread->join();

    CLOGD("wait done");
}

void ExynosCameraPipePlugIn::dump(void)
{
    ExynosCameraSWPipe::dump();
    CLOGI("running fcount(HAL:%d, DRV:%d)", m_runningFrameCount, m_runningMetaFrameCount);

    if (m_plugIn) {
        m_plugIn->dump();
    } else {
        CLOGW("there's no valid plugIn");
    }

    return;
}

}; /* namespace android */
