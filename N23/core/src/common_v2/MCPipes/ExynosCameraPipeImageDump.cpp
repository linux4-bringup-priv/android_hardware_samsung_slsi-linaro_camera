/*
**
** Copyright 2019, Samsung Electronics Co. LTD
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

#ifdef DEBUG_DUMP_IMAGE

bool ExynosCameraMCPipe::m_handleSequentialRawDump(ExynosCameraFrameSP_sptr_t newFrame)
{
#ifdef SEQUENTIAL_RAW_DUMP
    int32_t startFrameCount = -1;
    int32_t frameCountemp = -1;
    int32_t frameCount = -1;
    ExynosCameraFrameSP_sptr_t frame  = NULL;
    bool isFrameFound = false;
    int pipeId;
    bool pipeRequest = false;
    ExynosCameraBuffer buffer;
    status_t ret = NO_ERROR;
    camera2_node_group *nodeGroupInfo = NULL;
    camera2_node_group node_group_info;
    int32_t perframeInfoIndex = -1;

    startFrameCount = getPropertyConfig(SEQUENTIAL_RAW_DUMP_FRAME_COUNT);
    if (startFrameCount > 0) {
        m_RawFrameHoldList.pushProcessQ(&newFrame);
        while (m_RawFrameHoldList.getSizeOfProcessQ() > 0) {
            if (m_RawFrameHoldList.popProcessQ(&frame) != NO_ERROR) {
                CLOGE("getBufferToManageQ fail");
                continue;
            } else {
                frameCount = frame->getFrameCount();
                frameCountemp = startFrameCount + SEQUENTIAL_RAW_DUMP_COUNT - m_sequentialDumpCount + 1;
                CLOGD("startFrameCount = %d, frameCountemp = %d, frameCount = %d", startFrameCount, frameCountemp, frameCount);
                if (frameCount >= frameCountemp) {
                    isFrameFound = true;
                    break;
                }
            }
        }

        if (!isFrameFound) {
            return true;
        }

        if (m_node[OUTPUT_NODE] != NULL) {
            perframeInfoIndex = m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex;
            newFrame->getNodeGroupInfo(&node_group_info, perframeInfoIndex);
            nodeGroupInfo = &node_group_info;
        }

        /* output node dump */
        for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
            pipeId = getPipeId((enum NODE_TYPE)i);
            if (isPipeNeedImageDump(pipeId) == false) {
                continue;
            }

            pipeRequest = newFrame->getRequest(pipeId);
            CFLOGD(newFrame, "pipeId = %d m_node = %p getRequest = %d", pipeId, m_node[i], pipeRequest);
            if (m_node[i] != NULL && pipeRequest) {
                ret = newFrame->getSrcBuffer(getPipeId(), &buffer, i);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                            i, frameCount, ret);
                } else {
                    if (m_sequentialDumpCount > 0) {
                        m_sequentialDumpCount--;
                    } else {
                        //reset when sequential raw dump complete
                        m_sequentialDumpCount = SEQUENTIAL_RAW_DUMP_COUNT;
                        setDumpImagePropertyConfig(0);
                        setPropertyConfig(-1, SEQUENTIAL_RAW_DUMP_FRAME_COUNT);
                        return true;
                    }
                    CFLOGD(newFrame, "start dump: pipeId(%d), frameCount(%d), sequentialRawDump(%d/%d)",
                            i, frameCount, SEQUENTIAL_RAW_DUMP_COUNT - m_sequentialDumpCount, SEQUENTIAL_RAW_DUMP_COUNT);
                    m_prepareBufferForDump(&buffer, m_cameraId, frameCount,
                            pipeId, (enum NODE_TYPE)i, nodeGroupInfo, newFrame);
                }
            }
        }

        /* capture node dump */
        for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
            pipeId = getPipeId((enum NODE_TYPE)i);
            if (isPipeNeedImageDump(pipeId) == false) {
                continue;
            }

            pipeRequest = newFrame->getRequest(pipeId);
            CFLOGD(newFrame, "pipeId = %d m_node = %p getRequest = %d", pipeId, m_node[i], pipeRequest);
            if (m_node[i] != NULL && pipeRequest) {
                ret = newFrame->getDstBuffer(getPipeId(), &buffer, i);
                if (ret != NO_ERROR || buffer.index < 0) {
                    CFLOGE(newFrame, "%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                            i, frameCount, ret);
                } else {
                    if (m_sequentialDumpCount > 0) {
                        m_sequentialDumpCount--;
                    } else {
                        //reset when sequential raw dump complete
                        m_sequentialDumpCount = SEQUENTIAL_RAW_DUMP_COUNT;
                        setDumpImagePropertyConfig(0);
                        setPropertyConfig(-1, SEQUENTIAL_RAW_DUMP_FRAME_COUNT);
                        return true;
                    }
                    CFLOGD(newFrame, "start dump: pipeId(%d), frameCount(%d), sequentialRawDump(%d/%d)",
                            i, frameCount, SEQUENTIAL_RAW_DUMP_COUNT - m_sequentialDumpCount, SEQUENTIAL_RAW_DUMP_COUNT);
                    m_prepareBufferForDump(&buffer, m_cameraId, frameCount,
                            pipeId, (enum NODE_TYPE)i, nodeGroupInfo, newFrame);
                }
            }
        }

        return true;
    } else {
        if (m_RawFrameHoldList.getSizeOfProcessQ() > FRAME_HOLD_MAX_CNT) {
            if (m_RawFrameHoldList.popProcessQ(&frame) != NO_ERROR) {
                CLOGE("getBufferToManageQ fail");
            } else {
                frame  = NULL;
            }
        }
        return true;
    }
#endif

    return false;
}

int32_t ExynosCameraMCPipe::m_getPerFrameIdxFromCaptureNodeType(camera2_node_group *nodeGroupInfo,
                                                                                int32_t captureNodeType)
{
    int32_t perframeIdx = -1;
    uint32_t videoId = 0;

    CLOGV("captureNodeType = %d", captureNodeType);

    if (captureNodeType < OUTPUT_NODE || captureNodeType >= MAX_NODE)
        return  -1;

    if (m_node[captureNodeType] == NULL)
        return  -1;

    videoId = m_deviceInfo->nodeNum[captureNodeType] - FIMC_IS_VIDEO_BAS_NUM;
    CLOGV("videoId = %d", videoId);

    for (int32_t i = 0; i < CAPTURE_NODE_MAX; i++) {
        int32_t perframeVid = -1;

        perframeVid = nodeGroupInfo->capture[i].vid;
        CLOGV("idx(%d) [%d %d]", i, videoId, perframeVid);
        if (perframeVid == videoId) {
            perframeIdx = i;
            return perframeIdx;
        }
    }

    return  perframeIdx;

}

int32_t ExynosCameraMCPipe::m_prepareBufferForDump(ExynosCameraBuffer *srcBuffer,
                                                         uint32_t cameraId,
                                                         uint32_t frameCount,
                                                         int32_t pipeId,
                                                         enum NODE_TYPE nodeType,
                                                         camera2_node_group *nodeGroupInfo,
                                                         ExynosCameraFrameSP_sptr_t newFrame)
{
    ExynosCameraBuffer dumpBuffer;
    const buffer_manager_tag_t initBufTag;
    buffer_manager_tag_t bufTag;
    status_t ret = NO_ERROR;
    int32_t perFrameIdx = -1;
    ExynosCameraImageDumpInfo_t imageDumpInfo;
    time_t rawtime;
    struct tm* timeinfo;
    int planeCount;
    char formatName[8] = {0};
    int bufRetryCnt = 100;
    int32_t planeCountMeta = 0;
    const struct camera2_shot_ext *shot_ext;
    int32_t perframeSrcIndex = -1;

    CLOGV("-IN-");

    if ((srcBuffer == NULL) || (srcBuffer->addr[0] == NULL)) {
        CLOGE("Invalid srcBuffer (%p)", srcBuffer);
        return -1;
    }

    bufTag = initBufTag;
    bufTag.pipeId[0] = PIPE_DUMP_IMAGE;
    bufTag.managerType = BUFFER_MANAGER_ION_TYPE;


    dumpBuffer.handle[0] = NULL;
    dumpBuffer.acquireFence[0] = -1;
    dumpBuffer.releaseFence[0] = -1;

    while (bufRetryCnt > 0) {
        ret = m_bufferSupplier->getBuffer(bufTag, &dumpBuffer);
        if (ret != NO_ERROR || dumpBuffer.index < 0) {
            bufRetryCnt--;
            usleep(10000); //10ms
        } else {
            break;
        }
    }

    if (ret != NO_ERROR || dumpBuffer.index < 0) {
        CLOGE("[B%d P%d B_P%d] Failed to getBuffer. ret %d",
                            dumpBuffer.index,
                            pipeId, PIPE_DUMP_IMAGE, ret);
        return -1;
    }

    if (dumpBuffer.addr[0] == NULL) {
        CLOGE("dumpBuffer need to be mmap for the virtual address(%s)", dumpBuffer.bufMgrNm);
        return -1;
    }

    perFrameIdx = m_getPerFrameIdxFromCaptureNodeType(nodeGroupInfo, nodeType);

    if (nodeGroupInfo) {
        imageDumpInfo.width = nodeGroupInfo->capture[perFrameIdx].output.cropRegion[2];
        imageDumpInfo.height = nodeGroupInfo->capture[perFrameIdx].output.cropRegion[3];
    }

    if (m_node[nodeType]) {
        ret = m_node[nodeType]->getColorFormat(&imageDumpInfo.format, &imageDumpInfo.planeCount);
        if (ret != NO_ERROR) {
            CLOGE("node(%s) getColorFormat fail, ret(%d)",
                     m_deviceInfo->nodeName[nodeType], ret);
        } else {
            formatName[0] = (char)((imageDumpInfo.format >> 0) & 0xFF);
            formatName[1] = (char)((imageDumpInfo.format >> 8) & 0xFF);
            formatName[2] = (char)((imageDumpInfo.format >> 16) & 0xFF);
            formatName[3] = (char)((imageDumpInfo.format >> 24) & 0xFF);
            formatName[4] = '\0';
        }
    }

    imageDumpInfo.frameCount = frameCount;
    imageDumpInfo.buffer = dumpBuffer;
    imageDumpInfo.bufferSrc = *srcBuffer;
    imageDumpInfo.pipeId = pipeId;
    imageDumpInfo.cameraId = m_cameraId;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    planeCountMeta = (srcBuffer->hasMetaPlane) ? 1 : 0;
#ifdef DEBUG_DUMP_METADATA
    if (m_node[OUTPUT_NODE] != NULL) {
        perframeSrcIndex = OUTPUT_NODE; //m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex;
    }

    if (perframeSrcIndex >= 0) {
        imageDumpInfo.hasMeta = true;
        shot_ext = newFrame->getConstMeta(perframeSrcIndex);
    }
#endif

    snprintf(imageDumpInfo.name, sizeof(imageDumpInfo.name), DEBUG_DUMP_IMAGE_NAME, CAMERA_DATA_PATH,
                cameraId, frameCount, m_name, pipeId, m_deviceInfo->nodeName[nodeType],
                formatName, srcBuffer->planeCount - planeCountMeta, imageDumpInfo.hasMeta,
                imageDumpInfo.width, imageDumpInfo.height,
                timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    for (int batchIndex = 0; batchIndex < srcBuffer->batchSize; batchIndex++) {
        int startPlaneIndex = batchIndex * (srcBuffer->planeCount - planeCountMeta);
        int endPlaneIndex = (batchIndex + 1) * (srcBuffer->planeCount - planeCountMeta);

        for (int plane = startPlaneIndex; plane < endPlaneIndex; plane++) {
            CLOGI("[F%d P%d %s], (plane [%d] size(%d) s:%d, e:%d size_dst(%d)",
                    imageDumpInfo.frameCount, imageDumpInfo.pipeId, m_deviceInfo->nodeName[nodeType],
                    plane, srcBuffer->size[plane], startPlaneIndex, endPlaneIndex, dumpBuffer.size[plane]);
            memcpy(dumpBuffer.addr[plane], srcBuffer->addr[plane], srcBuffer->size[plane]);
        }
    }

#ifdef DEBUG_DUMP_METADATA
    if (imageDumpInfo.hasMeta) {
        int32_t metadataPlaneIndex = dumpBuffer.getMetaPlaneIndex();
        memcpy(dumpBuffer.addr[metadataPlaneIndex], shot_ext, sizeof(struct camera2_shot_ext));
    }
#endif


    m_dumpImageQ->pushProcessQ(&imageDumpInfo);
    if ((m_dumpImageQ->getSizeOfProcessQ() > 0) &&
            (m_dumpBufferThread->isRunning() == false)) {
        m_dumpBufferThread->run(PRIORITY_URGENT_DISPLAY);
    }

    return 0;
}

int32_t ExynosCameraMCPipe::m_dumpBufferUtils(ExynosCameraFrameSP_sptr_t newFrame)
{
    int pipeId;
    status_t ret = NO_ERROR;
    ExynosCameraBuffer buffer;
    ExynosCameraImageDumpInfo_t imageDumpInfo;
    camera2_node_group node_group_info;
    uint32_t frameCount;
    camera2_node_group *nodeGroupInfo = NULL;
    bool isNodeGroupInfoAvailable = false;
    int32_t perframeInfoIndex = -1;
    bool pipeRequest = false;
    int32_t perFrameIdx = -1;
    bool isBufferDumped = false;
    int32_t dumpImagePropertyConfig;
    int32_t dumpImageConfigrationVal;

    CLOGV("-IN-");

    if (m_flagTryStop == true)
        return 0;

    if (newFrame == NULL)
        return 0;

    if (getDumpImagePropertyConfig() == 2) {
        bool flagDumpThread = m_handleSequentialRawDump(newFrame);
        if (flagDumpThread == true) {
            // image dump will be happened at m_dumpBufferThread
            return 0;
        }
    }

    dumpImagePropertyConfig = getDumpImagePropertyConfig();
    dumpImageConfigrationVal = m_configurations->getModeValue(CONFIGURATION_DUMP_IMAGE_VALUE);

    if ((dumpImagePropertyConfig <= 0) && (dumpImageConfigrationVal <= 0))
        return 0;

    if (m_dumpBufferCount >= DEBUG_DUMP_MAX_CNT ||
            (m_dumpBufferCount % DEBUG_DUMP_INTERVAL) > 0)
        goto p_err;


    frameCount = newFrame->getFrameCount();

    if (m_node[OUTPUT_NODE] != NULL) {
        perframeInfoIndex = m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex;
        newFrame->getNodeGroupInfo(&node_group_info, perframeInfoIndex);
        nodeGroupInfo = &node_group_info;
        isNodeGroupInfoAvailable = true;
    }

    /* output node dump */
    for (int i = OUTPUT_NODE; i < MAX_OUTPUT_NODE; i++) {
        pipeId = getPipeId((enum NODE_TYPE)i);
        CLOGV("pipeId = %d m_node = %p DUMP_CONDITION = %d", pipeId, m_node[i], isPipeNeedImageDump(pipeId));
        if (m_node[i] != NULL && isPipeNeedImageDump(pipeId)) {
            ret = newFrame->getSrcBuffer(getPipeId(), &buffer, i);
            if (ret != NO_ERROR) {
                CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                         i, frameCount, ret);
            } else {
                CLOGI("start frame(%d) dump(pipeId:%d)", frameCount, i);
                isBufferDumped = true;
                m_prepareBufferForDump(&buffer, m_cameraId, frameCount,
                    pipeId, (enum NODE_TYPE)i, nodeGroupInfo, newFrame);
            }
        }
    }

    /* capture node dump */
    for (int i = CAPTURE_NODE; i < MAX_CAPTURE_NODE; i++) {
        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId >= 0)
            pipeRequest = newFrame->getRequest(pipeId);
        if (nodeGroupInfo) {
            perFrameIdx = m_getPerFrameIdxFromCaptureNodeType(nodeGroupInfo, (enum NODE_TYPE)i);
            if (perFrameIdx >= 0) {
                pipeRequest = nodeGroupInfo->capture[perFrameIdx].request;
            } else {
                CLOGV("perFrameIdx = %d", perFrameIdx);
            }
        } else {
            CLOGV("nodeGroupInfo is NULL");
        }
        CLOGV("pipeId = %d m_node = %p DUMP_CONDITION = %d getRequest = %d", pipeId, m_node[i],
            isPipeNeedImageDump(pipeId), pipeRequest);
        if (m_node[i] != NULL && pipeRequest &&
                isPipeNeedImageDump(pipeId)) {
            ret = newFrame->getDstBuffer(getPipeId(), &buffer, i);
            if (ret != NO_ERROR) {
                CLOGE("%d's Frame get buffer fail, frameCount(%d), ret(%d)",
                         i, frameCount, ret);
            } else {
                CLOGI("start frame(%d) dump(pipeId:%d)", frameCount, i);
                isBufferDumped = true;
                m_prepareBufferForDump(&buffer, m_cameraId, frameCount,
                    pipeId, (enum NODE_TYPE)i, nodeGroupInfo, newFrame);
            }
        }
    }

    if (isBufferDumped)
        m_dumpBufferCount++;

p_err:

    return 0;

}

bool ExynosCameraMCPipe::m_dumpBufferThreadFunc(void)
{
    int pipeId;
    status_t ret = NO_ERROR;
    ExynosCameraImageDumpInfo_t imageDumpInfo;
    ExynosCameraBuffer *buffer;
    bool retVal = false;
    int32_t planeCountMeta = 0;

    FILE *file = NULL;
    char fileName[EXYNOS_CAMERA_NAME_STR_SIZE];

    CLOGV("-IN-");

    if (m_flagTryStop == true)
        return true;

    ret = m_dumpImageQ->waitAndPopProcessQ(&imageDumpInfo);
    if (ret == TIMED_OUT) {
        return ret;
    } else if (ret != NO_ERROR) {
        /* TODO: doing exception handling */
        return ret;
    }

    planeCountMeta = (imageDumpInfo.bufferSrc.hasMetaPlane) ? 1 : 0;

    CLOGI("[F%d P%d], (width [%d] height(%d) Format:%x batchSize = %d bufferSrc.planeCount = %d buffer.planeCount = %d src_hasMetaPlane = %d",
                    imageDumpInfo.frameCount, imageDumpInfo.pipeId,
                    imageDumpInfo.width, imageDumpInfo.height, imageDumpInfo.format,
                    imageDumpInfo.bufferSrc.batchSize, imageDumpInfo.bufferSrc.planeCount,
                    imageDumpInfo.buffer.planeCount, imageDumpInfo.bufferSrc.hasMetaPlane);

    buffer = &imageDumpInfo.buffer;

    for (int batchIndex = 0; batchIndex < imageDumpInfo.bufferSrc.batchSize; batchIndex++) {
        /* skip meta plane */
        int startPlaneIndex = batchIndex * (imageDumpInfo.bufferSrc.planeCount - planeCountMeta);
        int endPlaneIndex = (batchIndex + 1) * (imageDumpInfo.bufferSrc.planeCount - planeCountMeta);

        memset(fileName, 0, sizeof(fileName));
        snprintf(fileName, sizeof(fileName), "%s_batchIndex_%d.raw", imageDumpInfo.name, batchIndex);

        file = fopen(fileName, "w");
        if (file == NULL) {
            CLOGE("ERR(%s):open(%s) fail", __func__, fileName);
            goto p_err;
        }

        fflush(stdout);
        for (int plane = startPlaneIndex; plane < endPlaneIndex; plane++) {
            fwrite(buffer->addr[plane], 1, imageDumpInfo.bufferSrc.size[plane], file);
            CLOGI("filedump(%s, [%d]size(%d) s:%d, e:%d",
                    fileName, plane, imageDumpInfo.bufferSrc.size[plane],
                    startPlaneIndex, endPlaneIndex);
        }
        fflush(file);

        fclose(file);
        file = NULL;

        CLOGD("filedump(%s) is successed!!", fileName);
    }

#ifdef DEBUG_DUMP_METADATA
    if (imageDumpInfo.hasMeta) {
        int32_t metadataPlaneIndex = buffer->getMetaPlaneIndex();
        memset(fileName, 0, sizeof(fileName));
        snprintf(fileName, sizeof(fileName), "%s_meta.raw", imageDumpInfo.name);

        file = fopen(fileName, "w");
        if (file == NULL) {
            CLOGE("ERR(%s):open(%s) fail", __func__, fileName);
            goto p_err;
        }

        fflush(stdout);
        fwrite(buffer->addr[metadataPlaneIndex], 1, sizeof(struct camera2_shot_ext), file);
        fflush(file);

        fclose(file);
        file = NULL;

        CLOGD("filedump(%s) is successed: size = %d!!", fileName, sizeof(struct camera2_shot_ext));
    }
#endif


p_err:
    m_bufferSupplier->putBuffer(imageDumpInfo.buffer);
    if (ret != NO_ERROR) {
        CLOGE("[F%d P%d B%d]Failed to putBuffer. ret %d",
                imageDumpInfo.frameCount,
                imageDumpInfo.pipeId,
                imageDumpInfo.buffer.index,
                ret);
    }

    if (m_dumpImageQ->getSizeOfProcessQ() > 0) {
        CLOGI("run again. m_dumpImageQ size %d", m_dumpImageQ->getSizeOfProcessQ());
        return true;
    } else {
        CLOGI("Complete to m_dumpImageQ");
        return false;
    }

    return false;

}
#endif
}
