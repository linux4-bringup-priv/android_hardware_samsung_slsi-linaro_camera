/*
 * Copyright@ Samsung Electronics Co. LTD
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

#define LOG_TAG "FakeFusion"

#include "FakeFusion.h"

FakeFusion::FakeFusion()
{
    PLUGIN_LOGD("new FakeFusion object allocated");
}

FakeFusion::~FakeFusion()
{
    PLUGIN_LOGD("new FakeFusion object deallocated");
}

status_t FakeFusion::create(void)
{
    status_t ret = NO_ERROR;

    m_emulationProcessTime = FUSION_PROCESSTIME_STANDARD;
    m_emulationCopyRatio = 1.0f;

    PLUGIN_LOGD("");

    return NO_ERROR;
}

status_t FakeFusion::destroy(void)
{
    status_t ret = NO_ERROR;

    PLUGIN_LOGD("");

    return NO_ERROR;
}

status_t FakeFusion::execute(Map_t *map)
{
    status_t ret = NO_ERROR;
    nsecs_t start = systemTime();

    char *srcYAddr    = NULL;
    char *srcCbcrAddr = NULL;
    char *dstYAddr    = NULL;
    char *dstCbcrAddr = NULL;

    unsigned int bpp = 0;
    unsigned int srcPlaneCount  = 1;
    unsigned int dstPlaneCount  = 1;

    /* from map */
    int               mapSrcBufCnt      = (Data_int32_t     )(*map)[PLUGIN_SRC_BUF_CNT];
    Array_buf_t       mapSrcBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_PLANE_CNT];
    Array_buf_plane_t mapSrcBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_SIZE];
    Array_buf_rect_t  mapSrcRect        = (Array_buf_rect_t )(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_t       mapSrcV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapSrcBufAddr[2];
    mapSrcBufAddr[0] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];           //master
    mapSrcBufAddr[1] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_2];           //slave

    int               mapDstBufCnt      = (Data_int32_t     )(*map)[PLUGIN_DST_BUF_CNT];
    Array_buf_t       mapDstBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_PLANE_CNT];
    Array_buf_plane_t mapDstBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_rect_t  mapDstRect        = (Array_buf_rect_t )(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t       mapDstV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapDstBufAddr[3];
    mapDstBufAddr[0] = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_1];   //Fusion;
    mapDstBufAddr[1] = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_2];   //Master;
    mapDstBufAddr[2] = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_3];   //Depth;

    for (int i = 0; i < mapSrcBufCnt; i++) {
        PLUGIN_LOGV("src[%d/%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d",
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
                mapSrcV4L2Format[i]);
    }

    for (int i = 0; i < mapDstBufCnt; i++) {
        PLUGIN_LOGV("dst[%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d",
                mapDstBufCnt,
                mapDstBufAddr[i][0],
                mapDstBufPlaneCnt[i],
                mapDstBufSize[i][0],
                mapDstRect[i][PLUGIN_ARRAY_RECT_X],
                mapDstRect[i][PLUGIN_ARRAY_RECT_Y],
                mapDstRect[i][PLUGIN_ARRAY_RECT_W],
                mapDstRect[i][PLUGIN_ARRAY_RECT_H],
                mapDstRect[i][PLUGIN_ARRAY_RECT_FULL_W],
                mapDstRect[i][PLUGIN_ARRAY_RECT_FULL_H],
                mapDstV4L2Format[i]);
    }

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
    if (previousFusionProcessTime <= 0)
        previousFusionProcessTime = 1;

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


    int dstFullW = mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_W];
    int dstFullH = mapDstRect[0][PLUGIN_ARRAY_RECT_FULL_H];
    int dstHalfPlaneSize = (dstFullW * dstFullH) / mapSrcBufCnt;

    ret = V4L2_PIX_2_YUV_INFO(mapDstV4L2Format[0], &bpp, &dstPlaneCount);
    if (ret < 0) {
        PLUGIN_LOGE("getYuvFormatInfo(dstRect[0].colorFormat(%x)) fail", mapDstV4L2Format[0]);
        return ret;
    }
    m_getYuvBufferAddress(mapDstBufAddr[0], dstFullW, dstFullH, dstPlaneCount, dstYAddr, dstCbcrAddr);

    for (int i = 0; i < mapSrcBufCnt; i++) {
        int srcFullW = mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_W];
        int srcFullH = mapSrcRect[i][PLUGIN_ARRAY_RECT_FULL_H];
        int srcHalfPlaneSize = (srcFullW * srcFullH) / mapSrcBufCnt;
        int copySize = (srcHalfPlaneSize < dstHalfPlaneSize) ? srcHalfPlaneSize : dstHalfPlaneSize;

        ret = V4L2_PIX_2_YUV_INFO(mapSrcV4L2Format[i], &bpp, &srcPlaneCount);
        if (ret < 0) {
            PLUGIN_LOGE("getYuvFormatInfo(srcRect[%d].colorFormat(%x)) fail", i, mapSrcV4L2Format[i]);
            return ret;
        }

        m_getYuvBufferAddress(mapSrcBufAddr[i], srcFullW, srcFullH, srcPlaneCount, srcYAddr, srcCbcrAddr);

        PLUGIN_LOGV("fusion emulation running ~~~ memcpy(%p, %p, %d)" \
                "by src(%d, %d), dst(%d, %d), previousFusionProcessTime(%d) copyRatio(%f)",
                mapDstBufAddr[0], mapSrcBufAddr[i][0], copySize,
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

            memcpy(dstYAddr,    srcYAddr,    copySize);
            memcpy(dstCbcrAddr, srcCbcrAddr, copySize / 2);
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
        }
    }

    // meta copy from src(wide) to dst.
    char *srcMetaAddr = (char *)mapSrcBufAddr[0][srcPlaneCount];
    char *dstMetaAddr = (char *)mapDstBufAddr[0][dstPlaneCount];
    int   srcMetaSize = mapSrcBufSize[0][srcPlaneCount];
    int   dstMetaSize = mapDstBufSize[0][dstPlaneCount];

    m_copyMetaData(srcMetaAddr, srcMetaSize, dstMetaAddr, dstMetaSize);

    /* Only copy master YUV src to master YUV dst */
    for (int dstBufIndex = 1; dstBufIndex < mapDstBufCnt; dstBufIndex++) {
        for (int i = 0; i < dstPlaneCount; i++) {
            int size = (mapSrcBufSize[0][i] < mapDstBufSize[dstBufIndex][i]) ? mapSrcBufSize[0][i] : mapDstBufSize[dstBufIndex][i];
            if (mapDstBufAddr[dstBufIndex][i] != NULL) {
                memcpy(mapDstBufAddr[dstBufIndex][i], mapSrcBufAddr[0][i], size);
            }
        }
    }

    nsecs_t end = systemTime();
    m_emulationProcessTime = (end - start) / 1000LL;

    return ret;
}

void FakeFusion::m_copyMetaData(char *srcMetaAddr, int srcMetaSize, char *dstMetaAddr, int dstMetaSize)
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

void FakeFusion::m_getYuvBufferAddress(Array_buf_addr_t bufferAddr, int width, int height, unsigned int planeCount,
                                            Single_buf_t &yAddr, Single_buf_t &cbcrAddr)
{
    const int yPlane = 0;
    const int cbcrPlane = 1;

    yAddr = bufferAddr[yPlane];

    switch (planeCount) {
    case 1:
        cbcrAddr = bufferAddr[yPlane] + (width * height);
        break;
    case 2:
        cbcrAddr = bufferAddr[cbcrPlane];
        break;
    default:
        PLUGIN_LOG_ASSERT("Invalid planeCount(%d), assert!!!!", planeCount);
        break;
    }
}

status_t FakeFusion::init(Map_t *map)
{
    return NO_ERROR;
}

status_t FakeFusion::query(Map_t *map)
{
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "FakeFusionLib";
        (*map)[PLUGIN_BUILD_DATE]             = (Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = (Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)2;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)1;
    }

    return NO_ERROR;
}
