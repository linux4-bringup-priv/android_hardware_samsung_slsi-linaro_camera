/*
 * Copyright @ 2019, Samsung Electronics Co. LTD
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

#define LOG_TAG "FakeMultiFrame"

#include "FakeMultiFrame.h"

FakeMultiFrame::FakeMultiFrame()
{
    PLUGIN_LOGD("new FakeMultiFrame object allocated");
}

FakeMultiFrame::~FakeMultiFrame()
{
    PLUGIN_LOGD("new FakeMultiFrame object deallocated");
}

status_t FakeMultiFrame::create(void)
{
    status_t ret = NO_ERROR;

    m_emulationProcessTime = FUSION_PROCESSTIME_STANDARD;
    m_emulationCopyRatio = 1.0f;

    PLUGIN_LOGD("");

    return NO_ERROR;
}

status_t FakeMultiFrame::destroy(void)
{
    status_t ret = NO_ERROR;

    PLUGIN_LOGD("");

    return NO_ERROR;
}

status_t FakeMultiFrame::execute(Map_t *map)
{
    status_t ret = NO_ERROR;
    nsecs_t start = systemTime();

    char *srcYAddr = NULL;
    char *srcCbcrAddr = NULL;
    char *dstYAddr = NULL;
    char *dstCbcrAddr = NULL;

    unsigned int bpp = 0;
    unsigned int planeCount = 1;

    int srcPlaneSize = 0;
    int srcHalfPlaneSize = 0;
    int dstPlaneSize = 0;
    int dstHalfPlaneSize = 0;
    int copySize = 0;

    /* from map */
    int               mapSrcBufCnt = (Data_int32_t)(*map)[PLUGIN_SRC_BUF_CNT];
    Array_buf_t       mapSrcBufPlaneCnt = (Array_buf_t)(*map)[PLUGIN_SRC_BUF_PLANE_CNT];
    Array_buf_plane_t mapSrcBufSize = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_SIZE];
    Array_buf_rect_t  mapSrcRect = (Array_buf_rect_t)(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_t       mapSrcV4L2Format = (Array_buf_t)(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapSrcBufAddr[2];
    mapSrcBufAddr[0] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];           //master
    mapSrcBufAddr[1] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_2];           //slave

    int               mapDstBufCnt = (Data_int32_t)(*map)[PLUGIN_DST_BUF_CNT];
    Array_buf_t       mapDstBufPlaneCnt = (Array_buf_t)(*map)[PLUGIN_DST_BUF_PLANE_CNT];
    Array_buf_plane_t mapDstBufSize = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_rect_t  mapDstRect = (Array_buf_rect_t)(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t       mapDstV4L2Format = (Array_buf_t)(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapDstBufAddr = (Array_buf_addr_t)(*map)[PLUGIN_DST_BUF_1];   //target;

    for (int i = 0; i < mapSrcBufCnt; i++) {
        PLUGIN_LOGD("src[%d/%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d], format:%c%c%c%c",
            i,
            mapSrcBufCnt,
            mapSrcBufAddr[i][0],
            mapSrcBufPlaneCnt[i],
            mapSrcBufSize[i][0],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_X],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_Y],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_W],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_H],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_W],
            mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_H],
            m_v4l2Format2Char(mapSrcV4L2Format[i], 0),
            m_v4l2Format2Char(mapSrcV4L2Format[i], 1),
            m_v4l2Format2Char(mapSrcV4L2Format[i], 2),
            m_v4l2Format2Char(mapSrcV4L2Format[i], 3));
    }

    PLUGIN_LOGD("dst[%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format:%c%c%c%c",
        mapDstBufCnt,
        mapDstBufAddr[0],
        mapDstBufPlaneCnt[0],
        mapDstBufSize[0][0],
        mapDstRect[0][PLUGIN_ARRAY_RECT_X],
        mapDstRect[0][PLUGIN_ARRAY_RECT_Y],
        mapDstRect[0][PLUGIN_ARRAY_RECT_W],
        mapDstRect[0][PLUGIN_ARRAY_RECT_H],
        mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_W],
        mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_H],
        m_v4l2Format2Char(mapDstV4L2Format[0], 0),
        m_v4l2Format2Char(mapDstV4L2Format[0], 1),
        m_v4l2Format2Char(mapDstV4L2Format[0], 2),
        m_v4l2Format2Char(mapDstV4L2Format[0], 3));

    /*
     * if previous emulationProcessTime is slow than 33msec,
     * we need change the next copy time
     *
     * ex :
     * frame 0 :
     * 1.0(copyRatio) = 33333 / 33333(previousFusionProcessTime : init value)
     * 1.0  (copyRatio) = 1.0(copyRatio) * 1.0(m_emulationCopyRatio);
     * m_emulationCopyRatio = 1.0
     * m_emulationProcessTime = 66666

     * frame 1 : because of frame0's low performance, shrink down copyRatio.
     * 0.5(copyRatio) = 33333 / 66666(previousFusionProcessTime)
     * 0.5(copyRatio) = 0.5(copyRatio) * 1.0(m_emulationCopyRatio);
     * m_emulationCopyRatio = 0.5
     * m_emulationProcessTime = 33333

     * frame 2 : acquire the proper copy time
     * 1.0(copyRatio) = 33333 / 33333(previousFusionProcessTime)
     * 0.5(copyRatio) = 1.0(copyRatio) * 0.5(m_emulationCopyRatio);
     * m_emulationCopyRatio = 0.5
     * m_emulationProcessTime = 16666

     * frame 3 : because of frame2's fast performance, increase copyRatio.
     * 2.0(copyRatio) = 33333 / 16666(previousFusionProcessTime)
     * 1.0(copyRatio) = 2.0(copyRatio) * 0.5(m_emulationCopyRatio);
     * m_emulationCopyRatio = 1.0
     * m_emulationProcessTime = 33333
     */
    int previousFusionProcessTime = m_emulationProcessTime;
    if (previousFusionProcessTime <= 0) {
        previousFusionProcessTime = 1;
    }

    float copyRatio = (float)FUSION_PROCESSTIME_STANDARD / (float)previousFusionProcessTime;
    copyRatio = copyRatio * m_emulationCopyRatio;

    if (1.0f <= copyRatio) {
        copyRatio = 1.0f;
    } else if (0.1f < copyRatio) {
        copyRatio -= 0.05f; // threshold value : 5%
    } else {
        PLUGIN_LOGW("copyRatio(%f) is too smaller than 0.1f. previousFusionProcessTime(%d), m_emulationCopyRatio(%f)",
             copyRatio, previousFusionProcessTime, m_emulationCopyRatio);
    }

    PLUGIN_LOGV("copyRatio:%f", copyRatio);
    m_emulationCopyRatio = copyRatio;

    for (int i = 0; i < mapSrcBufCnt; i++) {
        int srcFullW = mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_W];
        int srcFullH = mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_H];
        int dstFullW = mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_W];
        int dstFullH = mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_H];

        srcPlaneSize = mapSrcBufSize[i][0];
        srcHalfPlaneSize = srcPlaneSize / mapSrcBufCnt;

        dstPlaneSize = mapDstBufSize[0][0];
        dstHalfPlaneSize = dstPlaneSize / mapSrcBufCnt;

        copySize = (srcHalfPlaneSize < dstHalfPlaneSize) ? srcHalfPlaneSize : dstHalfPlaneSize;

        ret = V4L2_PIX_2_YUV_INFO(mapSrcV4L2Format[i], &bpp, &planeCount);
        if (ret < 0) {
            PLUGIN_LOGE("getYuvFormatInfo(srcRect[%d].colorFormat(%c%c%c%c)) fail",
                i,
                m_v4l2Format2Char(mapSrcV4L2Format[i], 0),
                m_v4l2Format2Char(mapSrcV4L2Format[i], 1),
                m_v4l2Format2Char(mapSrcV4L2Format[i], 2),
                m_v4l2Format2Char(mapSrcV4L2Format[i], 3));
        }

        srcYAddr    = mapSrcBufAddr[i][0];
        dstYAddr    = mapDstBufAddr[0];

        switch (planeCount) {
        case 1:
            srcCbcrAddr = mapSrcBufAddr[i][0] + srcFullW * srcFullH;
            dstCbcrAddr = mapDstBufAddr[0]    + dstFullW * dstFullH;
            break;
        case 2:
            srcCbcrAddr = mapSrcBufAddr[i][1];
            dstCbcrAddr = mapDstBufAddr[1];
            break;
        default:
            PLUGIN_LOG_ASSERT("Invalid planeCount(%d), assert!!!!", planeCount);
            break;
        }

        PLUGIN_LOGD("fusion emulation running ~~~ memcpy(%p, %p, %d), planeCount(%d) " \
                "by src(%d, %d), dst(%d, %d), previousFusionProcessTime(%d) copyRatio(%f)",
                mapDstBufAddr[0], mapSrcBufAddr[i][0], copySize, planeCount,
                srcFullW, srcFullH,
                dstFullW, dstFullH,
                previousFusionProcessTime, copyRatio);

        /* in case of source 2 */
        if (i == 1) {
            dstYAddr    += dstHalfPlaneSize;
            dstCbcrAddr += dstHalfPlaneSize / 2;
        }

        if (srcFullW == dstFullW && srcFullH == dstFullH) {
            int oldCopySize = copySize;
            copySize = (int)((float)copySize * copyRatio);

            if (oldCopySize < copySize) {
                PLUGIN_LOGW("oldCopySize(%d) < copySize(%d). just adjust oldCopySize",
                     oldCopySize, copySize);
                copySize = oldCopySize;
            }

            switch (planeCount) {
            case 1:
                memcpy(dstYAddr,    srcYAddr,    copySize);
                break;
            case 2:
                memcpy(dstYAddr,    srcYAddr,    copySize);
                memcpy(dstCbcrAddr, srcCbcrAddr, copySize / 2);
                break;
            default:
                PLUGIN_LOGW("Invalid planeCount(%d). so, copy only 1 plane", planeCount);

                memcpy(dstYAddr,    srcYAddr,    copySize);
                break;
            }
        } else {
            int width  = (srcFullW < dstFullW) ? srcFullW : dstFullW;
            int height = (srcFullH < dstFullH) ? srcFullH : dstFullH;

            int oldHeight = height;
            height = (int)((float)height * copyRatio);

            if (oldHeight < height) {
                PLUGIN_LOGW("oldHeight(%d) < height(%d). just adjust oldHeight",
                     oldHeight, height);
                height = oldHeight;
            }

            switch (planeCount) {
            case 1:
                for (int h = 0; h < height / 2; h++) {
                    memcpy(dstYAddr,    srcYAddr,    width);
                    srcYAddr += srcFullW;
                    dstYAddr += dstFullW;
                }
                break;
            case 2:
                for (int h = 0; h < height / 2; h++) {
                    memcpy(dstYAddr,    srcYAddr,    width);
                    srcYAddr += srcFullW;
                    dstYAddr += dstFullW;
                }

                for (int h = 0; h < height / 4; h++) {
                    memcpy(dstCbcrAddr, srcCbcrAddr, width);
                    srcCbcrAddr += srcFullW;
                    dstCbcrAddr += dstFullW;
                }
                break;
            default:
                PLUGIN_LOGW("Invalid planeCount(%d). so, copy only 1 plane", planeCount);

                for (int h = 0; h < height / 2; h++) {
                    memcpy(dstYAddr,    srcYAddr,    width);
                    srcYAddr += srcFullW;
                    dstYAddr += dstFullW;
                }
                break;
            }
        }
    }

    // meta copy from src(wide) to dst.
    int metaPlaneIndex = planeCount;

    char *srcMetaAddr = (char *)mapSrcBufAddr[0][metaPlaneIndex];
    char *dstMetaAddr = (char *)mapDstBufAddr[metaPlaneIndex];
    int   srcMetaSize = mapSrcBufSize[0][metaPlaneIndex];
    int   dstMetaSize = mapDstBufSize[0][metaPlaneIndex];

    m_copyMetaData(srcMetaAddr, srcMetaSize, dstMetaAddr, dstMetaSize);

    nsecs_t end = systemTime();
    m_emulationProcessTime = (end - start) / 1000LL;

    return ret;
}

void FakeMultiFrame::m_copyMetaData(char *srcMetaAddr, int srcMetaSize, char *dstMetaAddr, int dstMetaSize)
{
    if (srcMetaSize <= 0 || srcMetaAddr == NULL ||
        dstMetaSize <= 0 || dstMetaAddr == NULL ||
        srcMetaSize != dstMetaSize) {

        PLUGIN_LOGW("no meta plane. be carefull.. srcMetaSize(%d), dstMetaSize(%d)",
            srcMetaSize, dstMetaSize);
    } else {
        memcpy(dstMetaAddr, srcMetaAddr, dstMetaSize);
    }
}

char FakeMultiFrame::m_v4l2Format2Char(int v4l2Format, int pos)
{
    char c;

    switch (pos) {
    case 0:
        c = (char)((v4l2Format >>  0) & 0xFF);
        break;
    case 1:
        c = (char)((v4l2Format >> 8) & 0xFF);
        break;
    case 2:
        c = (char)((v4l2Format >> 16) & 0xFF);
        break;
    case 3:
        c = (char)((v4l2Format >> 24) & 0xFF);
        break;
    default:
        c = ' ';
        break;
    }

    return c;
}

status_t FakeMultiFrame::init(Map_t *map)
{
    return NO_ERROR;
}

status_t FakeMultiFrame::query(Map_t *map)
{
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "FakeMultiFrameLib";
        (*map)[PLUGIN_BUILD_DATE]             = (Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = (Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)1;
    }

    return NO_ERROR;
}
