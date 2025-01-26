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

#ifndef EXYNOS_CAMERA_UTILS_H
#define EXYNOS_CAMERA_UTILS_H

#include <cutils/properties.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <arm_neon.h>

#include "exynos_format.h"
#include "ExynosRect.h"

#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSensorInfo.h"
#include <linux/videodev2_exynos_media.h>
#include "ExynosCameraBuffer.h"
#include "ExynosCameraList.h"

//#define USE_INTERNAL_ALLOC_DEBUG
#ifdef USE_INTERNAL_ALLOC_DEBUG
//#define ALLOC_INFO_DUMP
#endif
#define MIN(a,b)             (((a) < (b)) ? (a) : (b))
#define CLIP3(a, minV, maxV) (((a) < (minV)) ? (minV) : (((a) > (maxV)) ? (maxV) : (a)))

#define ROUND_OFF(x, dig)           (floor((x) * pow(10.0f, dig)) / pow(10.0f, dig))
#define GET_MAX_NUM(a, b, c) \
    ((a) < (b) ? \
    ((b) < (c) ? (c) : (b)) \
   :((a) < (c) ? (c) : (a)) )

#define SAFE_DELETE(obj) \
    do { \
        if (obj) { \
            delete obj; \
            obj = NULL; \
        } \
    } while(0)

#define MAKE_STRING(s) (char *)(#s)

/* [CameraId]_[Bufffer Manager Name]_[FramcCount]_[Buffer Index]_[Batch Index]_[YYYYMMDD]_[HHMMSS].dump */
#define DEBUG_DUMP_NAME "%sCAM%d_%s_F%d_I%d_B%d_%02d%02d%02d_%02d%02d%02d.dump"

namespace android {

void getLogEnableProperty(void);
int getLogEnablePropertyValue(void);
static int g_propertyValue;

void getDebugInfoPlaneProperty(void);
int getDebugInfoPlanePropertyValue(void);
static int g_debugInfoPlaneValue;

class ExynosCameraConfigurations;
class ExynosCameraRequestManager;
class ExynosCameraRequest;
class ExynosCameraFrame;

typedef sp<ExynosCameraRequest> ExynosCameraRequestSP_sprt_t;
typedef sp<ExynosCameraRequest>& ExynosCameraRequestSP_dptr_t;
typedef sp<ExynosCameraFrame>  ExynosCameraFrameSP_sptr_t;

bool            getCropRect(
                    int  src_w,  int   src_h,
                    int  dst_w,  int   dst_h,
                    int *crop_x, int *crop_y,
                    int *crop_w, int *crop_h,
                    int  zoom);

bool            getCropRect2(
                    int  src_w,     int   src_h,
                    int  dst_w,     int   dst_h,
                    int *new_src_x, int *new_src_y,
                    int *new_src_w, int *new_src_h,
                    int  zoom);

status_t        getCropRectAlign(
                    int  src_w,  int   src_h,
                    int  dst_w,  int   dst_h,
                    int *crop_x, int *crop_y,
                    int *crop_w, int *crop_h,
                    int align_w, int align_h,
                    int  zoom, float zoomRatio);

status_t        getCropRectAlign(
                    int  src_w,  int   src_h,
                    int  dst_w,  int   dst_h,
                    int *crop_x, int *crop_y,
                    int *crop_w, int *crop_h,
                    int align_w, int align_h,
                    float zoomRatio);

uint32_t        bracketsStr2Ints(
                    char *str,
                    uint32_t num,
                    ExynosRect2 *rect2s,
                    int *weights,
                    int mode);
bool            subBracketsStr2Ints(int num, char *str, int *arr);

void            convertingRectToRect2(ExynosRect *rect, ExynosRect2 *rect2);
void            convertingRect2ToRect(ExynosRect2 *rect2, ExynosRect *rect);

bool            isRectNull(ExynosRect *rect);
bool            isRectNull(ExynosRect2 *rect2);
bool            isRectEqual(ExynosRect *rect1, ExynosRect *rect2);
bool            isRectEqual(ExynosRect2 *rect1, ExynosRect2 *rect2);

char            v4l2Format2Char(int v4l2Format, int pos);
ExynosRect2     convertingAndroidArea2HWArea(ExynosRect2 *srcRect, const ExynosRect *regionRect);
ExynosRect2     convertingAndroidArea2HWAreaBcropOut(ExynosRect2 *srcRect, const ExynosRect *regionRect);
ExynosRect2     convertingHWArea2AndroidArea(ExynosRect2 *srcRect, ExynosRect *regionRect);
ExynosRect      convertDstRectBySrcRect(const ExynosRect &srcRect, const ExynosRect &dstRect);

ExynosRect      convertingBufferDst2Rect(ExynosCameraBuffer *buffer, int colorFormat);
void            convertingRectDst2Buffer(ExynosCameraBuffer *buffer, const ExynosRect &rect);
void            copyRectSrc2DstBuffer(ExynosCameraBuffer *srcBuffer, ExynosCameraBuffer *dstBuffer);

/*
 * Control struct camera2_shot_ext
 */
int32_t getMetaDmRequestFrameCount(struct camera2_shot_ext *shot_ext);
int32_t getMetaDmRequestFrameCount(struct camera2_dm *dm);

void setMetaCtlAeTargetFpsRange(struct camera2_shot_ext *shot_ext, uint32_t min, uint32_t max);
void getMetaCtlAeTargetFpsRange(struct camera2_shot_ext *shot_ext, uint32_t *min, uint32_t *max);

void setMetaCtlSensorFrameDuration(struct camera2_shot_ext *shot_ext, uint64_t duration);
void getMetaCtlSensorFrameDuration(struct camera2_shot_ext *shot_ext, uint64_t *duration);

void setMetaCtlAeMode(struct camera2_shot_ext *shot_ext, enum aa_aemode aeMode);
void getMetaCtlAeMode(struct camera2_shot_ext *shot_ext, enum aa_aemode *aeMode);

void setMetaCtlAeLock(struct camera2_shot_ext *shot_ext, bool lock);
void getMetaCtlAeLock(struct camera2_shot_ext *shot_ext, bool *lock);
void setMetaVtMode(struct camera2_shot_ext *shot_ext, enum camera_vt_mode mode);
void setMetaVideoMode(struct camera2_shot_ext *shot_ext, enum aa_videomode mode);

void setMetaCtlExposureCompensation(struct camera2_shot_ext *shot_ext, int32_t expCompensation);
void getMetaCtlExposureCompensation(struct camera2_shot_ext *shot_ext, int32_t *expCompensatione);
#ifdef USE_SUBDIVIDED_EV
void setMetaCtlExposureCompensationStep(struct camera2_shot_ext *shot_ext, float expCompensationStep);
#endif
void setMetaCtlExposureTime(struct camera2_shot_ext *shot_ext, uint64_t exposureTime);
void getMetaCtlExposureTime(struct camera2_shot_ext *shot_ext, uint64_t *exposureTime);
void setMetaCtlCaptureExposureTime(struct camera2_shot_ext *shot_ext, uint32_t exposureTime);
void getMetaCtlCaptureExposureTime(struct camera2_shot_ext *shot_ext, uint32_t *exposureTime);

#ifdef SUPPORT_DEPTH_MAP
void setMetaCtlDisparityMode(struct camera2_shot_ext *shot_ext, enum camera2_disparity_mode disparity_mode);
#endif

void setMetaCtlWbLevel(struct camera2_shot_ext *shot_ext, int32_t wbLevel);
void getMetaCtlWbLevel(struct camera2_shot_ext *shot_ext, int32_t *wbLevel);

void setMetaCtlStatsRoi(struct camera2_shot_ext *shot_ext, int32_t x, int32_t y, int32_t w, int32_t h);
void setMetaCtlMasterCamera(struct camera2_shot_ext *shot_ext, enum aa_cameraMode cameraMode, enum aa_sensorPlace masterCamera);
void getMetaCtlMasterCamera(struct camera2_shot_ext *shot_ext, enum aa_cameraMode *cameraMode, enum aa_sensorPlace *masterCamera);

#ifdef USE_FW_ZOOMRATIO
void setMetaCtlZoom(struct camera2_shot_ext *shot_ext, float data);
void getMetaCtlZoom(struct camera2_shot_ext *shot_ext, float *data);
#endif

status_t setMetaCtlCropRegion(
        struct camera2_shot_ext *shot_ext,
        int x, int y, int w, int h);
status_t setMetaCtlCropRegion(
        struct camera2_shot_ext *shot_ext,
        int zoom,
        int srcW, int srcH,
        int dstW, int dstH, float zoomRatio);
void getMetaCtlCropRegion(
            struct camera2_shot_ext *shot_ext,
            int *x, int *y,
            int *w, int *h);

void setMetaCtlAeRegion(
            struct camera2_shot_ext *shot_ext,
            int x, int y,
            int w, int h,
            int weight);
void getMetaCtlAeRegion(
            struct camera2_shot_ext *shot_ext,
            int *x, int *y,
            int *w, int *h,
            int *weight);

void setMetaCtlAntibandingMode(struct camera2_shot_ext *shot_ext, enum aa_ae_antibanding_mode antibandingMode);
void getMetaCtlAntibandingMode(struct camera2_shot_ext *shot_ext, enum aa_ae_antibanding_mode *antibandingMode);

void setMetaCtlSceneMode(struct camera2_shot_ext *shot_ext, enum aa_mode mode, enum aa_scene_mode sceneMode);
void getMetaCtlSceneMode(struct camera2_shot_ext *shot_ext, enum aa_mode *mode, enum aa_scene_mode *sceneMode);

void setMetaCtlAwbMode(struct camera2_shot_ext *shot_ext, enum aa_awbmode awbMode);
void getMetaCtlAwbMode(struct camera2_shot_ext *shot_ext, enum aa_awbmode *awbMode);
void setMetaCtlAwbLock(struct camera2_shot_ext *shot_ext, bool lock);
void getMetaCtlAwbLock(struct camera2_shot_ext *shot_ext, bool *lock);
void setMetaCtlAfRegion(struct camera2_shot_ext *shot_ext,
                            int x, int y, int w, int h, int weight);
void getMetaCtlAfRegion(struct camera2_shot_ext *shot_ext,
                        int *x, int *y, int *w, int *h, int *weight);

void setMetaCtlColorCorrectionMode(struct camera2_shot_ext *shot_ext, enum colorcorrection_mode mode);
void getMetaCtlColorCorrectionMode(struct camera2_shot_ext *shot_ext, enum colorcorrection_mode *mode);
void setMetaCtlAaEffect(struct camera2_shot_ext *shot_ext, aa_effect_mode_t effect);
void getMetaCtlAaEffect(struct camera2_shot_ext *shot_ext, aa_effect_mode_t *effect);
void setMetaCtlBrightness(struct camera2_shot_ext *shot_ext, int32_t brightness);
void getMetaCtlBrightness(struct camera2_shot_ext *shot_ext, int32_t *brightness);

void setMetaCtlSaturation(struct camera2_shot_ext *shot_ext, int32_t saturation);
void getMetaCtlSaturation(struct camera2_shot_ext *shot_ext, int32_t *saturation);

void setMetaCtlHue(struct camera2_shot_ext *shot_ext, int32_t hue);
void getMetaCtlHue(struct camera2_shot_ext *shot_ext, int32_t *hue);

void setMetaCtlContrast(struct camera2_shot_ext *shot_ext, uint32_t contrast);
void getMetaCtlContrast(struct camera2_shot_ext *shot_ext, uint32_t *contrast);

void setMetaCtlSharpness(struct camera2_shot_ext *shot_ext, enum processing_mode edge_mode, int32_t edge_sharpness,
                            enum processing_mode noise_mode, int32_t noise_sharpness);
void getMetaCtlSharpness(struct camera2_shot_ext *shot_ext, enum processing_mode *mode, int32_t *sharpness,
                            enum processing_mode *noise_mode, int32_t *noise_sharpness);


void setMetaCtlIso(struct camera2_shot_ext *shot_ext, enum aa_isomode mode, uint32_t iso);
void getMetaCtlIso(struct camera2_shot_ext *shot_ext, enum aa_isomode *mode, uint32_t *iso);
void setMetaCtlFdMode(struct camera2_shot_ext *shot_ext, enum facedetect_mode mode);

void setMetaCtlGyro(struct camera2_shot_ext *shot_ext, float x, float y, float z);
#ifdef USE_GYRO_HISTORY_FOR_TNR
void setMetaCtlGyroHistory(struct camera2_shot_ext *shot_ext, float x, float y, float z, nsecs_t timestamp);
#endif
void setMetaCtlAccelerometer(struct camera2_shot_ext *shot_ext, float x, float y, float z);

void setMetaUctlYsumPort(struct camera2_shot_ext *shot_ext, enum mcsc_port ysumPort);

void getStreamFrameValid(struct camera2_stream *shot_stream, uint32_t *fvalid);
void getStreamFrameCount(struct camera2_stream *shot_stream, uint32_t *fcount);

status_t setMetaDmSensorTimeStamp(struct camera2_shot_ext *shot_ext, uint64_t timeStamp);
nsecs_t getMetaDmSensorTimeStamp(struct camera2_shot_ext *shot_ext);
status_t setMetaUdmSensorTimeStampBoot(struct camera2_shot_ext *shot_ext, uint64_t timeStamp);
nsecs_t getMetaUdmSensorTimeStampBoot(struct camera2_shot_ext *shot_ext);

int    getMetaDmAeState(struct camera2_shot_ext *shot_ext);
int    getMetaDmAfState(struct camera2_shot_ext *shot_ext);

void setMetaNodeLeaderRequest(struct camera2_shot_ext* shot_ext, int value);
void setMetaNodeLeaderVideoID(struct camera2_shot_ext* shot_ext, int value);
void setMetaNodeLeaderPixFormat(struct camera2_shot_ext* shot_ext, int value);
void setMetaNodeLeaderPixelSize(struct camera2_shot_ext* shot_ext, int value);
void setMetaNodeLeaderInputSize(struct camera2_shot_ext * shot_ext, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void setMetaNodeLeaderOutputSize(struct camera2_shot_ext * shot_ext, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void setMetaNodeCaptureRequest(struct camera2_shot_ext* shot_ext, int index, int value);
void setMetaNodeCaptureVideoID(struct camera2_shot_ext* shot_ext, int index, int value);
void setMetaNodeCapturePixFormat(struct camera2_shot_ext* shot_ext, int index, int value);
void setMetaNodeCapturePixelSize(struct camera2_shot_ext* shot_ext, int index, int value);
void setMetaNodeCaptureInputSize(struct camera2_shot_ext * shot_ext, int index, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void setMetaNodeCaptureOutputSize(struct camera2_shot_ext * shot_ext, int index, unsigned int x, unsigned int y, unsigned int w, unsigned int h);

void setMetaTnrMode(struct camera2_shot_ext *shot_ext, int value);
void setMetaBypassFd(struct camera2_shot_ext *shot_ext, int value);

void setMetaSetfile(struct camera2_shot_ext *shot_ext, int value);

void setMetaMcscFlip(struct camera2_shot_ext *shot_ext, enum mcsc_port mcscPort, enum camera_flip_mode flipMode);
enum camera_flip_mode getMetaMcscFlip(struct camera2_shot_ext *shot_ext, enum mcsc_port mcscPort);

int mergeSetfileYuvRange(int setfile, int yuvRange);

int getPlaneSizeFlite(int width, int height);
int getBayerLineSize(int width, int bayerFormat);
int getBayerPlaneSize(int width, int height, int bayerFormat);
float getBayerBytesPerPixel(int bayerFormat);

bool directDumpToFile(ExynosCameraBuffer *buffer, uint32_t cameraId, uint32_t frameCount);
bool dumpToFile(char *filename, char *srcBuf, unsigned int size);
bool dumpToFilePacked12(char *filename, char *srcBuf, unsigned int size);
bool dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1, unsigned int size, unsigned int size1);

status_t readFromFile(char *filename, char *dstBuf, uint32_t size);
status_t writeToFile(char *filename, char *dstBuf, uint32_t size);

/* TODO: This functions need to be commonized */
status_t getYuvPlaneSize(int format, unsigned int *size,
                         unsigned int width, unsigned int height,
                         camera_pixel_size pixelSize = CAMERA_PIXEL_SIZE_8BIT, camera_pixel_comp_info pixelCompInfo = NO_COMP);
status_t getV4l2FormatInfo(unsigned int v4l2_pixel_format,
                         unsigned int *bpp, unsigned int *planes,
                         camera_pixel_size pixelSize = CAMERA_PIXEL_SIZE_8BIT);
int getYuvPlaneCount(unsigned int v4l2_pixel_format,
                        camera_pixel_size pixelSize = CAMERA_PIXEL_SIZE_8BIT);
int displayExynosBuffer( ExynosCameraBuffer *buffer);

int checkBit(unsigned int *target, int index);
void clearBit(unsigned int *target, int index, bool isStatePrint = false);
void setBit(unsigned int *target, int index, bool isStatePrint = false);
void resetBit(unsigned int *target, int value, bool isStatePrint = false);
camera_pixel_size getPixelSizeFromBayerFormat(int format, bool isCompressed = 0);
uint32_t getSBWCPayloadSize(uint32_t w, uint32_t h, uint32_t bitsPerPixel);
uint32_t getSBWCHeaderSize(uint32_t w, uint32_t h);

status_t addBayerBuffer(struct ExynosCameraBuffer *srcBuf,
                        struct ExynosCameraBuffer *dstBuf,
                        ExynosRect *dstRect,
                        bool isPacked = false);
status_t addBayerBufferByNeon(struct ExynosCameraBuffer *srcBuf,
                              struct ExynosCameraBuffer *dstBuf,
                              unsigned int copySize);
status_t addBayerBufferByNeonPacked(struct ExynosCameraBuffer *srcBuf,
                                    struct ExynosCameraBuffer *dstBuf,
                                    ExynosRect *dstRect,
                                    unsigned int copySize);
status_t addBayerBufferByCpu(struct ExynosCameraBuffer *srcBuf,
                             struct ExynosCameraBuffer *dstBuf,
                             unsigned int copySize);

char clip(int i);
void convertingYUYVtoRGB888(char *dstBuf, char *srcBuf, int width, int height);

void checkAndroidVersion(void);

int  getFliteNodenum(int cameraId);
int  getFliteCaptureNodenum(int cameraId, int fliteOutputNode);
status_t getCamName(int cameraId, char *name, int nameSize);
bool isLogicalCam(int camServiceId);


int getLogicalCamSyncType(int ServiceID);
int getLogicalCamPhysIDs(int ServiceID, int *physID);
bool isCamPhysIDValid(int camServiceId, int internalId);
#if defined (SUPPORT_DEPTH_MAP) || defined(SUPPORT_PD_IMAGE)
int  getDepthVcNodeNum(int cameraId);
#endif

int  getSensorGyroNodeNum(int cameraId);

int calibratePosition(int w, int new_w, int pos);

status_t updateYsumBuffer(struct ysum_data *ysumdata, ExynosCameraBuffer *dstBuf);
status_t updateHDRBuffer(ExynosCameraBuffer *dstBuf);
void getV4l2Name(char* colorName, size_t length, int colorFormat);

bool checkLastFrameForMultiFrameCapture(ExynosCameraFrameSP_sptr_t frame);
bool checkFirstFrameForMultiFrameCapture(ExynosCameraFrameSP_sptr_t frame);
bool checkNeedYUVMaxDownscaling(ExynosRect* srcRect, ExynosCameraConfigurations *configurations);
bool swapNodeGroupSize(camera2_node_group *nodeGroup);
status_t getMaxSizeFromLUT(int (*table)[SIZE_OF_LUT], int tableSize, int sizeLutIndex, uint32_t *maxWidth, uint32_t *maxHeight);

int getJpegPipeIdIndex(int pipeId);

template <typename THREAD, typename THREADQ>
void stopThreadAndInputQ(THREAD thread, int qCnt, THREADQ inputQ, ...)
{
    /*
     * Note: The type of THREAD must be ExynosCameraThread <template>,
     * and The type of THREAQ must be ExynosCameraList <template> * .
     */

    va_list list;
    THREADQ wakeupQ = inputQ, releaseQ = inputQ;

    if (thread == NULL)
        return;

    thread->requestExit();

    va_start(list, inputQ);
    for (int i = 0; i < qCnt; i++) {
        if (wakeupQ != NULL) {
            wakeupQ->wakeupAll();
        }

        if (i < (qCnt - 1))
            wakeupQ = va_arg(list, THREADQ);
    }
    va_end(list);

    thread->requestExitAndWait();

    va_start(list, inputQ);
    for (int i = 0; i < qCnt; i++) {
        if (releaseQ != NULL) {
            releaseQ->release();
        }

        if (i < (qCnt - 1))
            releaseQ = va_arg(list, THREADQ);
    }
    va_end(list);
};

void setPreviewProperty(bool on);
bool isBackCamera(int32_t cameraId);
bool isFrontCamera(int32_t cameraId);
float getJPEGMaxBPPSize(void);

bool isFastenAeStableSupported(int cameraId);
int  getAvailableStallPort(int startIndex, int stopIndex, int *yuvStreamIdMap);
int updateMetadataUser(struct camera2_shot_ext* src_ext, struct camera2_shot_ext* dst_ext, struct camera2_shot_ext* buffer_dst_ext);
bool    checkVendorYUVStallMeta(__unused ExynosCameraRequestSP_sprt_t request);
bool    getVendorYUVStallMeta(__unused ExynosCameraRequestSP_sprt_t request);
int     checkVendorMFStillCount(__unused ExynosCameraRequestSP_sprt_t request);
void updateVendorSensorConfiguration(ExynosCameraConfigurations  *configurations, int cameraId, int width, int height);
bool    checkLongExposureCaptureMeta(__unused ExynosCameraFrameSP_sptr_t frame);
float getSensorRatio(cameraId_Info *camIdInfo, int cameraId);
bool isContainedOfSensor(enum DUAL_OPERATION_SENSORS target, enum DUAL_OPERATION_SENSORS current);
bool getOverlapInfoFromTwoRects(const ExynosRect2 &r1, const ExynosRect2 &r2, ExynosRect2 *overlapped);
bool checkValidateRect(const ExynosRect &rect);

#ifdef DEBUG_DUMP_IMAGE
int32_t getDumpImagePropertyConfig(void);
void setDumpImagePropertyConfig(int32_t val);
status_t isPipeNeedImageDump(int32_t pipeId);
void setPropertyConfig(int32_t val, char prop_str[]);
int32_t getPropertyConfig(char prop_str[]);

/* [CameraId]_[FramcCount]_[PIPE]_[PIPE_NAME]_[Fmt-]_[PCnt-]_[Meta-]_[WxH]_[YYYYMMDD]_[HHMMSS] */
#define DEBUG_DUMP_IMAGE_NAME "%sCAM%d_F%d_%s_PIPE-%d_%s_Fmt-%s_PCnt-%d_Meta-%d_%dx%d_%02d%02d%02d_%02d%02d%02d"
typedef struct ExynosCameraImageDumpInfo {
    ExynosCameraBuffer  buffer;
    ExynosCameraBuffer  bufferSrc;
    int                 format;
    int                 width;
    int                 height;
    uint32_t            frameCount;
    int                 pipeId;
    int32_t             cameraId;
    int32_t             planeCount;
    char                name[128];
    bool                hasMeta;

    ExynosCameraImageDumpInfo() {
        format = -1;
        width = -1;
        height = -1;
        frameCount = 0;
        pipeId = -1;
        cameraId = 0;
        planeCount = 0;
        hasMeta = false;
        memset(name, 0, sizeof(name));
    }

    ExynosCameraImageDumpInfo& operator =(const ExynosCameraImageDumpInfo &other) {
        this->buffer = other.buffer;
        this->bufferSrc = other.bufferSrc;
        this->format = other.format;
        this->width = other.width;
        this->height = other.height;
        this->frameCount = other.frameCount;
        this->pipeId = other.frameCount;
        this->cameraId = other.cameraId;
        this->hasMeta = other.hasMeta;
        memcpy(this->name, other.name, sizeof(name));

        return *this;
    }
} ExynosCameraImageDumpInfo_t;
#endif

}; /* namespace android */

#ifdef USE_INTERNAL_ALLOC_DEBUG
void* operator new(std::size_t);
void  operator delete(void*);
void* operator new[](std::size_t);
void  operator delete[](void*);
int alloc_info_print(int flag = 0);
#endif

#endif

