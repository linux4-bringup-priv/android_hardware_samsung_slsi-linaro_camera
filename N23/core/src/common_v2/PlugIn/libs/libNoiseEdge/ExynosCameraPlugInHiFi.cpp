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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPlugInHiFi"
#include <log/log.h>

#include "ExynosCameraPlugInHiFi.h"
#include "yuvreprocessing.h"

#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#define MAX(x, y)   ((x) > (y) ? (x) : (y))

#if 1
using namespace mpi;
using namespace mpi::lls;
#endif
namespace android {

volatile int32_t ExynosCameraPlugInHiFi::initCount = 0;

DECLARE_CREATE_PLUGIN_SYMBOL(ExynosCameraPlugInHiFi);

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInHiFi::m_init(void)
{
    int count = android_atomic_inc(&initCount);

    PLUGIN_LOGD("count(%d)", count);

    if (count == 1) {
        /* do nothing */
    }

    return NO_ERROR;
}


status_t ExynosCameraPlugInHiFi::m_deinit(void)
{
    int count = android_atomic_dec(&initCount);

    PLUGIN_LOGD("count(%d)", count);

    if (count == 0) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInHiFi::m_create(void)
{
    PLUGIN_LOGD("");

    strncpy(m_name, "HIFILLS_PLUGIN", (PLUGIN_NAME_STR_SIZE - 1));
#if 0
    mUser = NULL;
    mNumFaces = 0;
    memset(mFaceInfo, 0, sizeof(mpi::lls::Rect) * MAX_NUMFACES);
    mZoomRatio = ZOOM_LEVEL_MIN;
    memset(&mBufferData, 0, sizeof(mpi::lls::BufferData));
    mDebugLevel = DEBUG_LEVEL_NONE;

    /* Check Debug Level */
    char prop[PROP_VALUE_MAX];
    if (property_get("persist.hifills.debug", prop, "0") > 0) {
        mDebugLevel = atoi(prop);
        PLUGIN_LOGD("[HiFiLLS] mDebugLevel %d", mDebugLevel);
    }
#endif
    return NO_ERROR;
}

status_t ExynosCameraPlugInHiFi::m_destroy(void)
{
    PLUGIN_LOGD("YUVRepro destory");
#if 0
    if (mUser) {
        int ret = deinitialize(mUser);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin deinit failed!!");
        }
        mUser = NULL;
    }
#endif
    return NO_ERROR;
}

status_t ExynosCameraPlugInHiFi::m_setup(Map_t *map)
{
    PLUGIN_LOGD("YUVRepro setup");
    yuv_reprocessing_int();
    return NO_ERROR;
}

status_t ExynosCameraPlugInHiFi::m_process(Map_t *map)
{
    status_t ret = NO_ERROR;
    ALOGD("YUVRepro process");

    int               mapSrcBufCnt      = (Data_int32_t     )(*map)[PLUGIN_SRC_BUF_CNT];
    Array_buf_t       mapSrcBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_PLANE_CNT];
    Array_buf_plane_t mapSrcBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_SIZE];
    Array_buf_rect_t  mapSrcRect        = (Array_buf_rect_t )(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_t       mapSrcV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapSrcBufAddr[2];
    mapSrcBufAddr[0] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];
    mapSrcBufAddr[1] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_2];
    Array_buf_plane_t mapSrcBufFd;
    mapSrcBufFd = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_FD];

    int               mapDstBufCnt      = (Data_int32_t     )(*map)[PLUGIN_DST_BUF_CNT];
    Array_buf_t       mapDstBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_PLANE_CNT];
    Array_buf_plane_t mapDstBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_rect_t  mapDstRect        = (Array_buf_rect_t )(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t       mapDstV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapDstBufAddr     = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_1];
    Array_buf_plane_t mapDstBufFd;
    mapDstBufFd = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_FD];

    //if (mDebugLevel >= DEBUG_LEVEL_LOG) {
        for (int i = 0; i < mapSrcBufCnt; i++) {
            PLUGIN_LOGD("[YUVRepro] src[%d/%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
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
                mapSrcV4L2Format[i],
                mapSrcBufFd[i][0]);
        }

        for (int i = 0; i < mapDstBufCnt; i++) {
	        PLUGIN_LOGD("[YUVRepro] dst[%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
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
	            mapDstV4L2Format[0],
                mapDstBufFd[i][0]);
        }
    //}
    int noise_mode = (*map)[PLUGIN_NOISE_CONTROL_MODE];
    int noise_strength = (*map)[PLUGIN_NOISE_CONTROL_STRENGTH];
    int edge_mode = (*map)[PLUGIN_EDGE_CONTROL_MODE];
    int edge_strength = (*map)[PLUGIN_EDGE_CONTROL_STRENGTH];
    PLUGIN_LOGD("[YUVRepro] NM %d, NS %d, EM %d, ES %d", noise_mode, noise_strength, edge_mode, edge_strength);

    int width = mapSrcRect[0][PLUGIN_ARRAY_RECT_W];
    int height = mapSrcRect[0][PLUGIN_ARRAY_RECT_H];
    char *src = (char*)mapSrcBufAddr[0][0];
    char *dst = (char*)mapDstBufAddr[0];

    yuv_reprocessing(src, dst, width, height, noise_mode, noise_strength, edge_mode, edge_strength);


#if 0

    int frameCurIndex = 0;
    int frameMaxIndex = 0;
    int operationMode = 0;
    int frameCount = 0;
    int64_t timeStamp = 0;
    bool cropFlag = false;
    int rearOrFront = 0;
    int sensorType = 0;
    int hdrMode = 0;
    int sensorW = 0, sensorH = 0;
    int pictureW = 0, pictureH = 0;
    Array_buf_t shutterSpeed;
    Array_buf_t iso;
    Data_pointer_rect_t bCrop;
    Data_pointer_rect_t faceRect;
    int faceCount = 0;
    uint64_t exposureTime;
    uint64_t timeStampBoot;
    float zoomRatio;
    int brightnessValue;
    int inputW = 0, inputH = 0;

    frameCount      = (Data_int32_t       )(*map)[PLUGIN_SRC_FRAMECOUNT];
    timeStamp       = (Data_int64_t       )(*map)[PLUGIN_TIMESTAMP];
    exposureTime    = (Data_uint64_t      )(*map)[PLUGIN_EXPOSURE_TIME];
    timeStampBoot   = (Data_uint64_t      )(*map)[PLUGIN_TIMESTAMPBOOT];
    frameMaxIndex   = (Data_int32_t       )(*map)[PLUGIN_HIFI_TOTAL_BUFFER_NUM];
    frameCurIndex   = (Data_int32_t       )(*map)[PLUGIN_HIFI_CUR_BUFFER_NUM];
    sensorW         = (Data_int32_t       )(*map)[PLUGIN_HIFI_MAX_SENSOR_WIDTH];
    sensorH         = (Data_int32_t       )(*map)[PLUGIN_HIFI_MAX_SENSOR_HEIGHT];
    cropFlag        = (Data_bool_t        )(*map)[PLUGIN_HIFI_CROP_FLAG];
    rearOrFront     = (Data_int32_t       )(*map)[PLUGIN_HIFI_CAMERA_TYPE];
    sensorType      = (Data_int32_t       )(*map)[PLUGIN_HIFI_SENSOR_TYPE];
    hdrMode         = (Data_int32_t       )(*map)[PLUGIN_HIFI_HDR_MODE];
    zoomRatio       = (Data_float_t       )(*map)[PLUGIN_ZOOM_RATIO];
    shutterSpeed    = (Array_buf_t        )(*map)[PLUGIN_HIFI_SHUTTER_SPEED]; /* Short & Long ShutterSpeed */
    brightnessValue = (Data_int32_t       )(*map)[PLUGIN_HIFI_BV];
    iso             = (Array_buf_t        )(*map)[PLUGIN_HIFI_FRAME_ISO];   /* Short & Long ISO */
    bCrop           = (Data_pointer_rect_t)(*map)[PLUGIN_HIFI_FOV_RECT];
    faceCount       = (Data_int32_t       )(*map)[PLUGIN_HIFI_FRAME_FACENUM];
    faceRect        = (Data_pointer_rect_t)(*map)[PLUGIN_HIFI_FRAME_FACERECT];
    inputW          = (Data_int32_t       )(*map)[PLUGIN_HIFI_INPUT_WIDTH];
    inputH          = (Data_int32_t       )(*map)[PLUGIN_HIFI_INPUT_HEIGHT];
    pictureW        = (Data_int32_t       )(*map)[PLUGIN_HIFI_OUTPUT_WIDTH];
    pictureH        = (Data_int32_t       )(*map)[PLUGIN_HIFI_OUTPUT_HEIGHT];

    if (mDebugLevel >= DEBUG_LEVEL_LOG) {
        PLUGIN_LOGD("[HiFiLLS] frame(%d) count(%d/%d) oper(%d) sensor(%d %d) crop_flag(%d) facing(%d) sensorType(%d) hdr(%d) zoom(%f) shutter(%d %d) bv(%d) iso(%d %d) bcrop(%d %d %d %d) input(%d %d) output(%d %d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            operationMode,
            sensorW, sensorH,
            cropFlag,
            rearOrFront,
            sensorType,
            hdrMode,
            zoomRatio,
            shutterSpeed[0], shutterSpeed[1],
            brightnessValue,
            iso[0], iso[1],
            bCrop->x, bCrop->y, bCrop->w, bCrop->h,
            inputW, inputH,
            pictureW, pictureH);

        PLUGIN_LOGD("[HiFiLLS] faceCount(%d)", faceCount);
        for (int i = 0; i < faceCount ; i++) {
            PLUGIN_LOGD("[HiFiLLS] faceRect[%d] (%d %d %d %d)", i, faceRect[i].x, faceRect[i].y, faceRect[i].w, faceRect[i].h);
        }
    }

    PLUGIN_LOGD("[HiFiLLS] frame (%d/%d)", frameCurIndex, frameMaxIndex);

    if (frameCurIndex == 0) {
        /* 1st frame */
        if (mUser == NULL) {
            PLUGIN_LOGD("[HiFiLLS] Plugin init");
            ret = initialize(&mUser);
            PLUGIN_LOGD("[HiFiLLS] Plugin init done");
            if (ret != NO_ERROR) {
                PLUGIN_LOGE("[HiFiLLS] Plugin init failed!!");
                return ret;
            }
        }

        ret = setTotalBufferNum(mUser, frameMaxIndex);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set TotalBufferNum failed!!");
            return ret;
        }

        /* Update Operation Mode */
        int operationMode = LLS_MODE;
        if (zoomRatio >= 4.0f && brightnessValue >= 1536) {
            operationMode = SRZOOM_MODE;
        }
        ret = setOperateMode(mUser, operationMode);
        PLUGIN_LOGD("[HiFiLLS] operationMode %d brightnessValue %d frameMaxIndex %d",
            operationMode, brightnessValue, frameMaxIndex);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set OperateMode failed!!");
            return ret;
        }

        ExtraCameraInfo camera_info;
        memset(&camera_info, 0x0, sizeof(camera_info));
        /* TODO : Check DualMode */
        camera_info.fullWidth = sensorW;
        camera_info.fullHeight = sensorH;
        ret = setExtraCameraInfo(mUser, camera_info);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set ExtraCameraInfo failed!!");
            return ret;
        }

        ret = setCameraInfo(mUser, rearOrFront, sensorType, AP_TYPE_SLSI);
        PLUGIN_LOGD("[HiFiLLS] cameraType %d sensorType %d fullSize %dx%d",
            rearOrFront, sensorType, sensorW, sensorH);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set CameraInfo failed!!");
            return ret;
        }

        ExtraInfo info;
        info.iso[0] = iso[0];
        info.iso[1] = iso[1];
        info.exposureTime = {1000000, (unsigned int)exposureTime};
        info.brightnessValue.den = 256;
        info.brightnessValue.snum = brightnessValue;
        info.shutterSpeed[0] = {1000000, (unsigned int)shutterSpeed[0]};
        info.shutterSpeed[1] = {1000000, (unsigned int)shutterSpeed[1]};
        info.exposureValue = {256, 0};
        PLUGIN_LOGD("[HiFiLLS] zoomRatio %f, mHDRMode %u", zoomRatio, hdrMode);
        mZoomRatio = MIN(MAX(zoomRatio, ZOOM_LEVEL_MIN), ZOOM_LEVEL_MAX);
        info.hdrMode = hdrMode;
        ret = setExtraInfo(mUser, info);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set ExtraInfo failed!!");
            return ret;
        }

        /* Use Face Data of First Frame */
        mNumFaces = MAX(MIN(faceCount, kMaxFaces), 0);
        for (int i = 0 ; i < mNumFaces; ++i) {
            mFaceInfo[i].l = faceRect[i].x;
            mFaceInfo[i].t = faceRect[i].y;
            mFaceInfo[i].r = faceRect[i].w;
            mFaceInfo[i].b = faceRect[i].h;
        }
    }

    ret = setFaceData(mUser, mFaceInfo, mNumFaces);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("[HiFiLLS] Plugin set FaceInfo failed!!");
        return ret;
    }

    int               mapSrcBufCnt      = (Data_int32_t     )(*map)[PLUGIN_SRC_BUF_CNT];
    Array_buf_t       mapSrcBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_PLANE_CNT];
    Array_buf_plane_t mapSrcBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_SIZE];
    Array_buf_rect_t  mapSrcRect        = (Array_buf_rect_t )(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_t       mapSrcV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapSrcBufAddr[2];
    mapSrcBufAddr[0] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];
    mapSrcBufAddr[1] = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_2];
    Array_buf_plane_t mapSrcBufFd;
    mapSrcBufFd = (Array_buf_plane_t)(*map)[PLUGIN_SRC_BUF_FD];

    int               mapDstBufCnt      = (Data_int32_t     )(*map)[PLUGIN_DST_BUF_CNT];
    Array_buf_t       mapDstBufPlaneCnt = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_PLANE_CNT];
    Array_buf_plane_t mapDstBufSize     = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_SIZE];
    Array_buf_rect_t  mapDstRect        = (Array_buf_rect_t )(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t       mapDstV4L2Format  = (Array_buf_t      )(*map)[PLUGIN_DST_BUF_V4L2_FORMAT];
    Array_buf_addr_t  mapDstBufAddr     = (Array_buf_addr_t )(*map)[PLUGIN_DST_BUF_1];
    Array_buf_plane_t mapDstBufFd;
    mapDstBufFd = (Array_buf_plane_t)(*map)[PLUGIN_DST_BUF_FD];

    if (mDebugLevel >= DEBUG_LEVEL_LOG) {
        for (int i = 0; i < mapSrcBufCnt; i++) {
            PLUGIN_LOGD("[HiFiLLS] src[%d/%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
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
                mapSrcV4L2Format[i],
                mapSrcBufFd[i][0]);
        }

        for (int i = 0; i < mapDstBufCnt; i++) {
	        PLUGIN_LOGD("[HiFiLLS] dst[%d]::(addr:%p, P%d, S%d, [%d, %d, %d, %d, %d, %d] format :%d fd %d)",
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
	            mapDstV4L2Format[0],
                mapDstBufFd[i][0]);
        }
    }

    BufferData buffer_data;
    buffer_data.width = mapSrcRect[0][PLUGIN_ARRAY_RECT_W];
    buffer_data.height = mapSrcRect[0][PLUGIN_ARRAY_RECT_H];
    /* NV21 Color Foramt */
    buffer_data.y  = mapSrcBufAddr[0][0];
    buffer_data.uv = mapSrcBufAddr[0][0] + (buffer_data.width * buffer_data.height);
    buffer_data.fdY  = mapSrcBufFd[0][0];
    buffer_data.fdUV = -1;
    buffer_data.zoomRatio = mZoomRatio;
    buffer_data.fov.l = mapSrcRect[0][PLUGIN_ARRAY_RECT_X];
    buffer_data.fov.t = mapSrcRect[0][PLUGIN_ARRAY_RECT_Y];
    buffer_data.fov.r = mapSrcRect[0][PLUGIN_ARRAY_RECT_X] + buffer_data.width;
    buffer_data.fov.b = mapSrcRect[0][PLUGIN_ARRAY_RECT_Y] + buffer_data.height;
    ret = enqueueBuffer(mUser, buffer_data);
    PLUGIN_LOGD("[HiFiLLS] Src %dx%d, %p, %d, (%d, %d, %d, %d)",
        buffer_data.width, buffer_data.height,
        buffer_data.y, buffer_data.fdY,
        buffer_data.fov.l, buffer_data.fov.t, buffer_data.fov.r, buffer_data.fov.b);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("[HiFiLLS] Plugin enqueueBuffer failed!!");
        return ret;
    }

    if (frameCurIndex == (frameMaxIndex - 1)) {
        /* last frame */
        buffer_data.width = mapDstRect[0][PLUGIN_ARRAY_RECT_W];
        buffer_data.height = mapDstRect[0][PLUGIN_ARRAY_RECT_H];
        /* NV21 Color Foramt */
        buffer_data.y = mapDstBufAddr[0];
        buffer_data.uv = mapDstBufAddr[0] + (buffer_data.width * buffer_data.height);
        buffer_data.fdY  = mapDstBufFd[0][0];
        buffer_data.fdUV = -1;
        buffer_data.zoomRatio = 1.0;
        buffer_data.fov.l = mapDstRect[0][PLUGIN_ARRAY_RECT_X];
        buffer_data.fov.t = mapDstRect[0][PLUGIN_ARRAY_RECT_Y];
        buffer_data.fov.r = mapDstRect[0][PLUGIN_ARRAY_RECT_X] + buffer_data.width;
        buffer_data.fov.b = mapDstRect[0][PLUGIN_ARRAY_RECT_Y] + buffer_data.height;
        ret = setOutputBuffer(mUser, buffer_data);
        PLUGIN_LOGD("[HiFiLLS] Dst %dx%d, %p, %d, (%d, %d, %d, %d)",
            buffer_data.width, buffer_data.height,
            buffer_data.y, buffer_data.fdY,
            buffer_data.fov.l, buffer_data.fov.t, buffer_data.fov.r, buffer_data.fov.b);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin set OutputBuffer failed!!");
            return ret;
        }

        PLUGIN_LOGD("[HiFiLLS] Plugin process");
        ret = mpi::lls::process(mUser);
        PLUGIN_LOGD("[HiFiLLS] Plugin process done");
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin process failed!!");
            return ret;
        } else {
            unsigned char *data = new unsigned char[300];
            int size = writeDebugBuffer(mUser, data);
            delete[] data;
        }

        if (mDebugLevel >= DEBUG_LEVEL_FACE) {
            /* To check rect info of face */
            memcpy(&mBufferData, &buffer_data, sizeof(mpi::lls::BufferData));
            drawFaceRects();
        }

        PLUGIN_LOGD("[HiFiLLS] Plugin deinit");
        ret = deinitialize(mUser);
        PLUGIN_LOGD("[HiFiLLS] Plugin deinit done");
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[HiFiLLS] Plugin deinit failed!!");
        }
        mUser = NULL;
    }
#endif
    return ret;
}

status_t ExynosCameraPlugInHiFi::m_setParameter(int key, void *data)
{
    status_t ret = NO_ERROR;

    switch (key) {
    case PLUGIN_PARAMETER_KEY_PREPARE:
        break;
    case PLUGIN_PARAMETER_KEY_START:
#if 0
        if (mUser == NULL) {
            PLUGIN_LOGD("[HiFiLLS] Plugin init");
            ret = initialize(&mUser);
            PLUGIN_LOGD("[HiFiLLS] Plugin init done");
            if (ret != NO_ERROR) {
                PLUGIN_LOGE("[HiFiLLS] Plugin init failed!!");
            }
        }
#endif
        break;
    case PLUGIN_PARAMETER_KEY_STOP:
#if 0
        if (mUser) {
            PLUGIN_LOGD("[HiFiLLS] Plugin deinit");
            ret = deinitialize(mUser);
            PLUGIN_LOGD("[HiFiLLS] Plugin deinit done");
            if (ret != NO_ERROR) {
                PLUGIN_LOGE("[HiFiLLS] Plugin deinit failed!!");
            }
            mUser = NULL;
        }
#endif
        break;
    default:
        PLUGIN_LOGW("Unknown key(%d)", key);
    }

    return ret;
}

status_t ExynosCameraPlugInHiFi::m_getParameter(int key, void *data)
{
    PLUGIN_LOGD("YUVRepro getParameter");
    return NO_ERROR;
}

void ExynosCameraPlugInHiFi::m_dump(void)
{
    PLUGIN_LOGD("YUVRepro dump");
    /* do nothing */
}

status_t ExynosCameraPlugInHiFi::m_query(Map_t *map)
{
    PLUGIN_LOGD("YUVRepro query");
    status_t ret = NO_ERROR;
    if (map != NULL) {
        (*map)[PLUGIN_VERSION]                = (Map_data_t)MAKE_VERSION(1, 0);
        (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "SAMSUNG_HIFI";
        (*map)[PLUGIN_BUILD_DATE]             = 0;//(Map_data_t)__DATE__;
        (*map)[PLUGIN_BUILD_TIME]             = 0;//(Map_data_t)__TIME__;
        (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t)1;
        (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t)1;
    }
    return ret;
}

status_t ExynosCameraPlugInHiFi::m_start(void)
{
    PLUGIN_LOGD("YUVRepro start");
    return NO_ERROR;
}

status_t ExynosCameraPlugInHiFi::m_stop(void)
{
    PLUGIN_LOGD("YUVRepro stop");
    return NO_ERROR;
}

void ExynosCameraPlugInHiFi::rowDraw(int sX, int eX, int Y)
{
#if 0
    int w = mBufferData.width;
    char *y = mBufferData.y;
    char *uv = mBufferData.uv;

    for (int i = sX; i < eX; i++) {
        y[Y * w + i] = Y_BLUE;
    }

    for (int i = sX; i < eX; i += 2) {
        uv[Y / 2 * w + i] = U_BLUE;
    }

    for (int i = sX + 1; i < eX + 1; i += 2) {
        uv[Y / 2 * w + i] = V_BLUE;
    }
#endif
}

void ExynosCameraPlugInHiFi::colDraw(int sY, int eY, int X)
{
#if 0
    int w = mBufferData.width;
    char *y = mBufferData.y;
    char *uv = mBufferData.uv;

    for (int i = sY; i < eY; i++) {
        y[i * w + X] = Y_BLUE;
    }

    for (int i = sY / 2; i < eY / 2; i++) {
        uv[i * w + X] = U_BLUE;
    }

    for (int i = sY / 2; i < eY / 2; i++) {
        uv[i * w + X + 1] = V_BLUE;
    }
#endif
}

void ExynosCameraPlugInHiFi::drawFaceRects(void)
{
#if 0
    int w = (int)(mBufferData.width / mBufferData.zoomRatio);
    int h = (int)(mBufferData.height / mBufferData.zoomRatio);

    for (int i = 0; i < mNumFaces; i++) {
        int l = (int)((mFaceInfo[i].l + 1000) * w / 2000);
        int t = (int)((mFaceInfo[i].t + 1000) * h / 2000);
        int r = (int)((mFaceInfo[i].r + 1000) * w / 2000);
        int b = (int)((mFaceInfo[i].b + 1000) * h / 2000);

        int oftX = (int)((mBufferData.width - w) >> 1);
        int oftY = (int)((mBufferData.height - h) >> 1);

        l += oftX;
        t += oftY;
        r += oftX;
        b += oftY;

        l &= ~1;
        t &= ~1;
        r &= ~1;
        b &= ~1;

        rowDraw(l, r, t);
        rowDraw(l, r, b);
        colDraw(t, b, l);
        colDraw(t, b, r);
    }
#endif
}

}
