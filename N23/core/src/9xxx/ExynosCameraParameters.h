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

#ifndef EXYNOS_CAMERA_PARAMETERS_H
#define EXYNOS_CAMERA_PARAMETERS_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <CameraParameters.h>

#include <videodev2.h>
#include <videodev2_exynos_media.h>
#include <videodev2_exynos_camera.h>
#include <map>

#include "ExynosCameraObject.h"
#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSensorInfoBase.h"
#include "ExynosCameraCounter.h"
#include "fimc-is-metadata.h"
#include "ExynosRect.h"
#include "exynos_format.h"
#include "ExynosExif.h"
#include "ExynosCameraUtils.h"
#include "ExynosCameraActivityControl.h"
#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraConfigurations.h"

#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraEEPRomMapFactory.h"
#include "ExynosCameraMakersNoteFactory.h"
#ifdef USE_SLSI_VENDOR_TAGS
#include "ExynosCameraVendorTags.h"
#endif

#ifdef USES_SENSOR_LISTENER
#include "ExynosCameraSensorListenerWrapper.h"
#endif

#ifdef USES_P3_IN_EXIF
#include "ICCHeader.h"
#endif

#define FW_CUSTOM_OFFSET (1)
#define V4L2_FOURCC_LENGTH 5
#define IS_WBLEVEL_DEFAULT (4)

#define GPS_PROCESSING_METHOD_SIZE 32

#define EXYNOS_CONFIG_DEFINED (-1)
#define EXYNOS_CONFIG_NOTDEFINED (-2)

#define STATE_REG_SUPER_NIGHT_SHOT      (1<<28)
#define STATE_REG_LLHDR_CAPTURE         (1<<27)
#define STATE_REG_MFHDR_CAPTURE         (1<<26)
#define STATE_REG_FLASH_MFDN            (1<<25)
#define STATE_REG_SUPER_RESOLUTION      (1<<24)
#define STATE_REG_LIVE_OUTFOCUS         (1<<22)
#define STATE_REG_BINNING_MODE          (1<<20)
#define STATE_REG_LONG_CAPTURE          (1<<18)
#define STATE_REG_RTHDR_AUTO            (1<<16)
#define STATE_REG_NEED_LLS              (1<<14)
#define STATE_REG_ZOOM                  (1<<12)
#define STATE_REG_RTHDR_ON              (1<<10)
#define STATE_REG_RECORDINGHINT         (1<<8)
#define STATE_REG_DUAL_RECORDINGHINT    (1<<6)
#define STATE_REG_UHD_RECORDING         (1<<4)
#define STATE_REG_DUAL_MODE             (1<<2)
#define STATE_REG_FLAG_REPROCESSING     (1)

#define STATE_STILL_PREVIEW                     (0)
#define STATE_STILL_PREVIEW_WDR_ON              (STATE_REG_RTHDR_ON)
#define STATE_STILL_PREVIEW_WDR_AUTO            (STATE_REG_RTHDR_AUTO)

#define STATE_STILL_CAPTURE                     (STATE_REG_FLAG_REPROCESSING)
#define STATE_STILL_CAPTURE_ZOOM                (STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM)
#define STATE_VIDEO_CAPTURE                     (STATE_REG_FLAG_REPROCESSING|STATE_REG_RECORDINGHINT)
#define STATE_STILL_CAPTURE_LLS                 (STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS)
#define STATE_STILL_CAPTURE_LONG                (STATE_REG_FLAG_REPROCESSING|STATE_REG_LONG_CAPTURE)
#define STATE_STILL_CAPTURE_LLS_ZOOM            (STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS|STATE_REG_ZOOM)
#define STATE_STILL_CAPTURE_SUPER_RESOLUTION    (STATE_REG_FLAG_REPROCESSING|STATE_REG_SUPER_RESOLUTION)
#define STATE_LLHDR_CAPTURE_WDR_ON              (STATE_REG_LLHDR_CAPTURE|STATE_STILL_CAPTURE|STATE_REG_RTHDR_ON)
#define STATE_LLHDR_CAPTURE_WDR_AUTO            (STATE_REG_LLHDR_CAPTURE|STATE_STILL_CAPTURE|STATE_REG_RTHDR_AUTO)
#define STATE_STILL_CAPTURE_SUPER_NIGHT_SHOT    (STATE_REG_FLAG_REPROCESSING|STATE_REG_SUPER_NIGHT_SHOT)
#define STATE_STILL_CAPTURE_LLHDR_ON            (STATE_REG_FLAG_REPROCESSING|STATE_REG_LLHDR_CAPTURE)
#define STATE_STILL_CAPTURE_HDR_ON              (STATE_REG_FLAG_REPROCESSING|STATE_REG_MFHDR_CAPTURE)
#define STATE_STILL_CAPTURE_FLASH_MFDN_ON       (STATE_REG_FLAG_REPROCESSING|STATE_REG_FLASH_MFDN)

#define STATE_STILL_CAPTURE_WDR_ON                 (STATE_REG_RTHDR_ON|STATE_REG_FLAG_REPROCESSING)
#define STATE_STILL_CAPTURE_WDR_ON_ZOOM            (STATE_REG_RTHDR_ON|STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM)
#define STATE_STILL_CAPTURE_WDR_ON_LLS_ZOOM        (STATE_REG_RTHDR_ON|STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS|STATE_REG_ZOOM)
#define STATE_VIDEO_CAPTURE_WDR_ON                 (STATE_REG_RTHDR_ON|STATE_REG_FLAG_REPROCESSING|STATE_REG_RECORDINGHINT)
#define STATE_VIDEO_CAPTURE_WDR_ON_LLS             (STATE_REG_RTHDR_ON|STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS)

#define STATE_STILL_CAPTURE_WDR_AUTO                 (STATE_REG_RTHDR_AUTO|STATE_REG_FLAG_REPROCESSING)
#define STATE_STILL_CAPTURE_WDR_AUTO_ZOOM            (STATE_REG_RTHDR_AUTO|STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM)
#define STATE_VIDEO_CAPTURE_WDR_AUTO                 (STATE_REG_RTHDR_AUTO|STATE_REG_FLAG_REPROCESSING|STATE_REG_RECORDINGHINT)
#define STATE_STILL_CAPTURE_WDR_AUTO_LLS             (STATE_REG_RTHDR_AUTO|STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS)
#define STATE_STILL_CAPTURE_WDR_AUTO_LLS_ZOOM        (STATE_REG_RTHDR_AUTO|STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS|STATE_REG_ZOOM)

#define STATE_VIDEO                             (STATE_REG_RECORDINGHINT)
#define STATE_VIDEO_WDR_ON                      (STATE_REG_RECORDINGHINT|STATE_REG_RTHDR_ON)
#define STATE_VIDEO_WDR_AUTO                    (STATE_REG_RECORDINGHINT|STATE_REG_RTHDR_AUTO)

#define STATE_DUAL_VIDEO                        (STATE_REG_DUAL_RECORDINGHINT|STATE_REG_DUAL_MODE)
#define STATE_DUAL_VIDEO_CAPTURE                (STATE_REG_DUAL_RECORDINGHINT|STATE_REG_DUAL_MODE|STATE_REG_FLAG_REPROCESSING)
#define STATE_DUAL_STILL_PREVIEW                (STATE_REG_DUAL_MODE)
#define STATE_DUAL_STILL_CAPTURE                (STATE_REG_DUAL_MODE|STATE_REG_FLAG_REPROCESSING)

#define STATE_UHD_PREVIEW                       (STATE_REG_UHD_RECORDING)
#define STATE_UHD_PREVIEW_WDR_ON                (STATE_REG_UHD_RECORDING|STATE_REG_RTHDR_ON)
#define STATE_UHD_PREVIEW_WDR_AUTO              (STATE_REG_UHD_RECORDING|STATE_REG_RTHDR_AUTO)
#define STATE_UHD_VIDEO                         (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT)
#define STATE_UHD_VIDEO_WDR_ON                  (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_RTHDR_ON)
#define STATE_UHD_VIDEO_WDR_AUTO                (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_RTHDR_AUTO)

#define STATE_UHD_PREVIEW_CAPTURE               (STATE_REG_UHD_RECORDING|STATE_REG_FLAG_REPROCESSING)
#define STATE_UHD_PREVIEW_CAPTURE_WDR_ON        (STATE_REG_UHD_RECORDING|STATE_REG_FLAG_REPROCESSING|STATE_REG_RTHDR_ON)
#define STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO      (STATE_REG_UHD_RECORDING|STATE_REG_FLAG_REPROCESSING|STATE_REG_RTHDR_AUTO)
#define STATE_UHD_VIDEO_CAPTURE                 (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_FLAG_REPROCESSING)
#define STATE_UHD_VIDEO_CAPTURE_WDR_ON          (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_FLAG_REPROCESSING|STATE_REG_RTHDR_ON)
#define STATE_UHD_VIDEO_CAPTURE_WDR_AUTO        (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_FLAG_REPROCESSING|STATE_REG_RTHDR_AUTO)

#define STATE_STILL_BINNING_PREVIEW             (STATE_REG_BINNING_MODE)
#define STATE_STILL_BINNING_PREVIEW_WDR_AUTO    (STATE_REG_BINNING_MODE|STATE_REG_RTHDR_AUTO)
#define STATE_VIDEO_BINNING                     (STATE_REG_RECORDINGHINT|STATE_REG_BINNING_MODE)
#define STATE_DUAL_STILL_BINING_PREVIEW         (STATE_REG_DUAL_MODE|STATE_REG_BINNING_MODE)
#define STATE_DUAL_VIDEO_BINNING                (STATE_REG_DUAL_RECORDINGHINT|STATE_REG_DUAL_MODE|STATE_REG_BINNING_MODE)

#define STATE_LIVE_OUTFOCUS_PREVIEW             (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_PREVIEW)
#define STATE_LIVE_OUTFOCUS_PREVIEW_WDR_ON      (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_PREVIEW|STATE_REG_RTHDR_ON)
#define STATE_LIVE_OUTFOCUS_PREVIEW_WDR_AUTO    (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_PREVIEW|STATE_REG_RTHDR_AUTO)
#define STATE_LIVE_OUTFOCUS_CAPTURE             (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_CAPTURE)
#define STATE_LIVE_OUTFOCUS_CAPTURE_WDR_ON      (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_CAPTURE|STATE_REG_RTHDR_ON)
#define STATE_LIVE_OUTFOCUS_CAPTURE_WDR_AUTO    (STATE_REG_LIVE_OUTFOCUS|STATE_STILL_CAPTURE|STATE_REG_RTHDR_AUTO)

namespace android {

using namespace std;
using ::android::hardware::camera::common::V1_0::helper::CameraParameters;
using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

class ExynosCameraConfigurations;
class ExynosCameraRequest;

typedef sp<ExynosCameraRequest> ExynosCameraRequestSP_sprt_t;
typedef sp<ExynosCameraRequest>& ExynosCameraRequestSP_dptr_t;

enum HW_INFO_SIZE_TYPE {
    HW_INFO_SENSOR_MARGIN_SIZE,
    HW_INFO_MAX_SENSOR_SIZE,
    HW_INFO_MAX_PREVIEW_SIZE,
    HW_INFO_MAX_PICTURE_SIZE,
    HW_INFO_MAX_THUMBNAIL_SIZE,
    HW_INFO_HW_YUV_SIZE,
    HW_INFO_HW_YUV_INPUT_SIZE,
    HW_INFO_MAX_HW_YUV_SIZE,
    HW_INFO_HW_SENSOR_SIZE,
    HW_INFO_HW_SENSOR_GYRO_SIZE,
    HW_INFO_HW_BNS_SIZE,
    HW_INFO_HW_PREVIEW_SIZE,
    HW_INFO_HW_MAX_PICTURE_SIZE,
    HW_INFO_HW_PICTURE_SIZE,
    HW_INFO_SENSOR_MODE_CHANGE_BDS_SIZE, /* To support remosaic sensor mode change */
    HW_INFO_HW_REMOSAIC_SENSOR_SIZE,     /* To support remosaic capture sensor size */
    HW_INFO_SIZE_MAX,
};

enum HW_INFO_MODE_TYPE {
    HW_INFO_MODE_MAX,
};

enum SUPPORTED_HW_FUNCTION_TYPE {
    SUPPORTED_HW_FUNCTION_SENSOR_STANDBY,
    SUPPORTED_HW_FUNCTION_SENSOR_EXTENDED_MODE,
    SUPPORTED_HW_FUNCTION_SENSOR_REMOSAIC_MODE,
    SUPPORTED_HW_FUNCTION_MAX,
};

typedef enum DUAL_STANDBY_STATE {
    DUAL_STANDBY_STATE_ON,
    DUAL_STANDBY_STATE_ON_READY,
    DUAL_STANDBY_STATE_OFF,
    DUAL_STANDBY_STATE_OFF_READY,
} dual_standby_state_t;

enum DUAL_SOLUTION_MARGIN {
    DUAL_SOLUTION_MARGIN_VALUE_NONE,
    DUAL_SOLUTION_MARGIN_VALUE_20, // 20%
    DUAL_SOLUTION_MARGIN_VALUE_30, // 30%
};

enum PARAM_TYPE {
    PARAM_TYPE_MAIN = 0,
    PARAM_TYPE_SUB,
    PARAM_TYPE_MAX,
};

typedef struct
{
    char     magicPrefix[THUMBNAIL_HISTOGRAM_STAT_MAGIC_PREFIX_SIZE];
    uint32_t bufSize[2];
    char     *bufAddr[2];
}thumbnail_histogram_info_t;

class ExynosCameraParameters : public ExynosCameraObject {
public:
    /* Enumerator */
    enum yuv_output_port_id {
        YUV_0,
        YUV_1,
        YUV_2,
        YUV_MAX, //3

        YUV_STALL_0 = YUV_MAX,
        YUV_STALL_1,
        YUV_STALL_2,
        YUV_STALL_MAX, //6
        YUV_OUTPUT_PORT_ID_MAX = YUV_STALL_MAX,
    };

    enum critical_section_type {
        /* Order is VERY important.
           It indicates the order of entering critical section on run-time.
         */
        CRITICAL_SECTION_TYPE_START,
        CRITICAL_SECTION_TYPE_HWFC = CRITICAL_SECTION_TYPE_START,
        CRITICAL_SECTION_TYPE_VOTF,
        CRITICAL_SECTION_TYPE_END,
    };

    /* Constructor */
    ExynosCameraParameters(cameraId_Info *camIdInfo, int camType,
                            ExynosCameraConfigurations *configurations);


    /* Destructor */
    virtual ~ExynosCameraParameters();

    void            setDefaultCameraInfo(void);

public:
    /* Size configuration */
    status_t        setSize(enum HW_INFO_SIZE_TYPE type, uint32_t width, uint32_t height, int outputPortId = -1);
    status_t        getSize(enum HW_INFO_SIZE_TYPE type, uint32_t *width, uint32_t *height, int outputPortId = -1);
    status_t        resetSize(enum HW_INFO_SIZE_TYPE type);

    /* Function configuration */
    bool            isSupportedFunction(enum SUPPORTED_HW_FUNCTION_TYPE type) const;

    status_t        checkPictureSize(int pictureW, int pctureH, bool isSameSensorSize = false);
    status_t        checkThumbnailSize(int thumbnailW, int thumbnailH);

    status_t        resetYuvSizeRatioId(void);
    status_t        checkYuvSize(const int width, const int height, const int outputPortId, bool reprocessing = false);
    status_t        checkHwYuvSize(const int width, const int height, const int outputPortId, bool isSameSensorSize = false);
#ifdef HAL3_YUVSIZE_BASED_BDS
    status_t        initYuvSizes();
#endif

    /* PreviewPort ID */
    void            setPreviewPortId(int outputPortId);
    bool            isPreviewPortId(int outputPortId);
    int             getPreviewPortId(void);

    /* PreviewCallBackPort ID */
    void            setPreviewCbPortId(int outputPortId);
    bool            isPreviewCbPortId(int outputPortId);
    int             getPreviewCbPortId(void);

    /* RecordingPort ID */
    void            setRecordingPortId(int outputPortId);
    bool            isRecordingPortId(int outputPortId);
    int             getRecordingPortId(void);

    /* Alternative PreviewPort ID */
    void            setAlternativePreviewPortId(int outputPortId);
    bool            isAlternativePreviewPortId(int outputPortId);
    int             getAlternativePreviewPortId(void);

    void            setYuvOutPortId(enum pipeline pipeId, int outputPortId);
    int             getYuvOutPortId(enum pipeline pipeId);

    status_t        getCropSizeImpl(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom, bool use3aaInputCrop, bool update);
    status_t        getStatCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom = true);
    status_t        getPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom = true);
    status_t        getPreviewBdsSize(ExynosRect *dstRect, bool applyZoom = true);
    status_t        getPictureBayerCropSizeImpl(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom, bool is3aaInputCrop);
    status_t        getRemosaicBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom);
    status_t        getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect, bool applyZoom = true);
    status_t        getPictureBdsSize(ExynosRect *dstRect, bool applyZoom = true);
    status_t        getPreviewYuvCropSize(ExynosRect *yuvCropSize);
    status_t        getPictureYuvCropSize(ExynosRect *yuvCropSize);
    status_t        getFastenAeStableSensorSize(int *hwSensorW, int *hwSensorH, int index);
    status_t        getFastenAeStableBcropSize(int *hwBcropW, int *hwBcropH, int index);
    status_t        getFastenAeStableBdsSize(int *hwBdsW, int *hwBdsH, int index);

    status_t        calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect);
    camera_pixel_comp_info  getPixelCompInfo(int pipeID);
    /* Recording AF FOV */
    void            CheckAfFovc(ExynosCameraFrameSP_sptr_t frame,int perframeIndex);
    bool            isUseAfFovCrop(enum pipeline pipeId, struct camera2_shot_ext *shot_ext) ;
    status_t        getAfFovCropSize(int lensPosition, ExynosRect *cropRect, struct camera2_shot_ext *shot_ext);
    void            m_adjustAfFovCrop(ExynosCameraParameters *params,camera2_node_group *node_group_info,struct camera2_shot_ext *shot_ext);

    /* FPS info about stillShot sensor(remosaic sensor) */
    bool            isStillShotSensorFpsSupported(void);
    int             getStillShotSensorFps(void);


private:
    /* Sets the image format for preview-related HW. */
    void            m_setHwPreviewFormat(int colorFormat);
    /* Sets the image format for picture-related HW. */
    void            m_setHwPictureFormat(int colorFormat);
    /* Sets the image pixel size for picture-related HW. */
    void            m_setHwPicturePixelSize(camera_pixel_size pixelSize);

    /* Sets HW Bayer Crop Size */
    void            m_setHwBayerCropRegion(int w, int h, int x, int y);
    /* Sets Bayer Crop Region */
    status_t        m_setParamCropRegion(int srcW, int srcH, int dstW, int dstH);

/*
 * Additional API.
 */
/*
 * Vendor specific APIs
 */
/*
 * Others
 */
    void            m_setExifFixedAttribute(void);
    status_t        m_adjustPreviewFpsRange(uint32_t &newMinFps, uint32_t &newMaxFps);
    status_t        m_adjustPreviewCropSizeForPicture(ExynosRect *inputRect, ExynosRect *previewCropRect);

public:
    /* Returns the image format for FLITE/3AC/3AP bayer */
    int             getBayerFormat(int pipeId);
    /* Returns the image format for picture-related HW. */
    int             getHwPictureFormat(void);
    /* Returns the image pixel size for picture-related HW. */
    camera_pixel_size getHwPicturePixelSize(void);

    /* Returns the image format for preview-related HW. */
    int             getHwPreviewFormat(void);
    /* Returns HW Bayer Crop Size */
    void            getHwBayerCropRegion(int *w, int *h, int *x, int *y);
    /* Set the current crop region info */
    status_t        setCropRegion(int x, int y, int w, int h);
    status_t        setPictureCropRegion(int x, int y, int w, int h);

    void            getHwVraInputSize(int *w, int *h, camera2_shot* shot, mcsc_port dsInputPortId = MCSC_PORT_NONE);
    void            getHw3aaVraInputSize(int dsInW, int dsInH, ExynosRect * dsOutRect);
    int             getHwVraInputFormat(void);
    int             getHW3AFdFormat(void);

    void            setDsInputPortId(int dsInputPortId, bool isReprocessing);
    int             getDsInputPortId(bool isReprocessing);

    void            setYsumPordId(int ysumPortId, struct camera2_shot_ext *shot_ext);
    int             getYsumPordId(void);

    int             getYuvSizeRatioId(void);
/*
 * Additional API.
 */

    /* Get sensor control frame delay count */
    int             getSensorControlDelay(void);

    /* Static flag for LLS */
    bool            getLLSOn(void);
    void            setLLSOn(bool enable);

/*
 * Vendor specific APIs
 */

    /* Gets Intelligent mode */
    bool            getHWVdisMode(void);
    int             getHWVdisFormat(void);
#ifdef SUPPORT_ME
    int             getMeFormat(void);
    void            getMeSize(int *meWidth, int *meHeight);
    int             getLeaderPipeOfMe();
#endif

    void            getYuvVendorSize(int *width, int *height, int outputPortId, ExynosRect ispSize);

    uint32_t        getSensorStandbyDelay(void);

#ifdef USE_BINNING_MODE
    int *           getBinningSizeTable(void);
#endif
    /* Gets ImageUniqueId */
    const char     *getImageUniqueId(void);
#if defined(SENSOR_FW_GET_FROM_FILE)
    ExynosCameraEEPRomMap *getEEPRomMap(void);
#endif
    ExynosCameraMakersNote *getMakersNote(void);

/*
 * Static info
 */
    /* Gets max zoom ratio */
    float           getMaxZoomRatio(void);

    /* Gets current zoom ratio about Bcrop */
    float           getActiveZoomRatio(void);
    /* Sets current zoom ratio about Bcrop */
    void            setActiveZoomRatio(float zoomRatio);
    /* Gets current zoom rect about Bcrop */
    void            getActiveZoomRect(ExynosRect *zoomRect);
    /*Sets current zoom rect about Bcrop */
    void            setActiveZoomRect(ExynosRect zoomRect);

    /* Gets current zoom margin */
    int             getActiveZoomMargin(void);
    /* Sets current zoom margin */
    void            setActiveZoomMargin(int zoomMargin);

    /* Gets current picture zoom ratio about Bcrop */
    float           getPictureActiveZoomRatio(void);
    /* Sets current picture zoom ratio about Bcrop */
    void            setPictureActiveZoomRatio(float zoomRatio);
    /* Gets current picture zoom rect about Bcrop */
    void            getPictureActiveZoomRect(ExynosRect *zoomRect);
    /*Sets current picture zoom rect about Bcrop */
    void            setPictureActiveZoomRect(ExynosRect zoomRect);

    /* Gets current picture zoom margin */
    int             getPictureActiveZoomMargin(void);
    /* Sets current picture zoom margin */
    void            setPictureActiveZoomMargin(int zoomMargin);

    /* Gets FocalLengthIn35mmFilm */
    int             getFocalLengthIn35mmFilm(void);
    /* Gets Logical FocalLengthIn35mmFilm. eg. W, T, UT => W's value */
    int             getLogicalFocalLengthIn35mmFilm(void);

    /* Gets AutoFocus */
    bool            getAutoFocusSupported(void);

    status_t        getFixedExifInfo(exif_attribute_t *exifInfo);
    void            setExifChangedAttribute(exif_attribute_t    *exifInfo,
                                            ExynosRect          *PictureRect,
                                            ExynosRect          *thumbnailRect,
                                            camera2_shot_ext    *shot_ext,
                                            bool                useDebugInfo2 = false);
    void            initThumbNailInfo(void);
    void            setThumbNailInfo(char *bufAddr);

    debug_attribute_t *getDebugAttribute(void);
    debug_attribute_t *getDebug2Attribute(void);

#ifdef USE_BINNING_MODE
    int             getBinningMode(void);
#endif /* USE_BINNING_MODE */
    void            updatePreviewStatRoi(struct camera2_shot_ext *shot_ext, ExynosRect *bCropRect);
    void            updateDisplayStatRoi(ExynosCameraFrameSP_sptr_t frame, struct camera2_shot_ext *shot_ext);
    void            setDisplayStatRoi(int32_t dispCameraId, const ExynosRect &dispRect, const FrameSizeInfoMap_t &sizeMap);
    void            getDisplayStatRoi(int32_t &dispCameraId, ExynosRect &dispRect);
    void            getDisplaySizeInfo(FrameSizeInfoMap_t &sizeMap);
    void            initDisplayStatRoi(void);

public:
    /* For Vendor */
    void            updateHwSensorSize(void);
    void            updateBinningScaleRatio(void);

    status_t        duplicateCtrlMetadata(void *buf);

    status_t        reInit(void);
    void            vendorSpecificConstructor(int cameraId);

private:
    bool            m_isSupportedYuvSize(const int width, const int height, const int outputPortId, int *ratio);
    bool            m_isSupportedPictureSize(const int width, const int height);
    status_t        m_getSizeListIndex(int (*sizelist)[SIZE_OF_LUT], int listMaxSize, int ratio, int *index);
    status_t        m_getPictureSizeList(int *sizeList);
    status_t        m_getPreviewSizeList(int *sizeList);
    bool            m_isSupportedFullSizePicture(void);

    void            m_initMetadata(void);

/*
 * Vendor specific adjust function
 */
    void            m_vendorSpecificDestructor(void);
    status_t        m_getPreviewBdsSize(ExynosRect *dstRect);
    status_t        m_adjustPictureSize(int *newPictureW, int *newPictureH,
                                        int *newHwPictureW, int *newHwPictureH);
    void            m_adjustSensorMargin(int *sensorMarginW, int *sensorMarginH);
    void            m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange);
    void            m_getCropRegion(int *x, int *y, int *w, int *h);
    void            m_getPictureCropRegion(int *x, int *y, int *w, int *h);

    void            m_setExifChangedAttribute(exif_attribute_t    *exifInfo,
                                              ExynosRect          *PictureRect,
                                              ExynosRect          *thumbnailRect,
                                              camera2_shot_ext    *shot_ext,
                                              bool                useDebugInfo2 = false);
    void            m_setExifChangedMakersNote(exif_attribute_t *exifInfo,
                                               struct camera2_shot_ext *shot_ext);

    void            m_getVendorUsePureBayerReprocessing(bool &usePureBayerReprocessing);

    /* H/W Chain Scenario Infos */
    enum HW_CONNECTION_MODE         m_getFlite3aaOtf(void);
    enum HW_CONNECTION_MODE         m_getPaf3aaOtf(void);
    enum HW_CONNECTION_MODE         m_get3aaIspOtf(void);
    enum HW_CONNECTION_MODE         m_get3aaVraOtf(void);
    enum HW_CONNECTION_MODE         m_getIspMcscOtf(void);
    enum HW_CONNECTION_MODE         m_getMcscVraOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessingPaf3AAOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessing3aaIspOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessing3aaVraOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessingIspMcscOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessingMcscVraOtf(void);
#ifdef USE_DUAL_CAMERA
    enum HW_CONNECTION_MODE         m_getIspDcpOtf(void);
    enum HW_CONNECTION_MODE         m_getDcpMcscOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessingIspDcpOtf(void);
    enum HW_CONNECTION_MODE         m_getReprocessingDcpMcscOtf(void);
#endif

    int                 m_adjustDsInputPortId(const int dsInputPortId);

    void                m_setOldBayerFrameLockCount(int count);
    void                m_setNewBayerFrameLockCount(int count);
    int                 m_getSensorInfoCamIdx(void);

public:
    ExynosCameraActivityControl *getActivityControl(void);

    void                getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange);
    void                setSetfileYuvRange(void);
    void                setSetfileYuvRange(bool flagReprocessing, int setfile, int yuvRange);
    status_t            checkSetfileYuvRange(void);

    void                setUseSensorPackedBayer(bool enable);
    bool                getUsePureBayerRemosaicReprocessing(void);
    bool                getUsePureBayerReprocessing(void);
    int32_t             getReprocessingBayerMode(void);

    void                setFastenAeStableOn(bool enable);
    bool                getFastenAeStableOn(void);
    bool                checkFastenAeStableEnable(void);

    int                getTnrMode(void);

    /* Gets BatchSize for HFR */
    int                 getBatchSize(enum pipeline pipeId);
    /* Check to use Service Batch Mode */
    bool                useServiceBatchMode(void);
    /* Decide to enter critical section */

    bool                isDebugInfoPlane(enum pipeline pipeId, int *planeCount);

    bool                isCriticalSection(enum pipeline pipeId,
                                          enum critical_section_type type);

    struct ExynosCameraSensorInfoBase *getSensorStaticInfo();
    const struct ExynosCameraSensorInfoBase *getOtherSensorStaticInfo(int camIdx);

    int32_t             getYuvStreamMaxNum(void);
    int32_t             getInputStreamMaxNum(void);

    bool                getSensorOTFSupported(void);
    bool                isReprocessing(void);
    bool                isSccCapture(void);

    /* True if private reprocessing or YUV reprocessing is supported */
    bool                isSupportZSLInput(void);

    enum HW_CHAIN_TYPE  getHwChainType(void);
    uint32_t            getNumOfMcscInputPorts(void);
    uint32_t            getNumOfMcscOutputPorts(void);

    enum HW_CONNECTION_MODE getHwConnectionMode(
                                enum pipeline srcPipeId,
                                enum pipeline dstPipeId);

    bool                isUse3aaInputCrop(void);
    bool                isUseBayerCompression(void);
    bool                setVideoStreamExistStatus(bool);
    bool                isUseVideoHQISP(void);
    bool                isUse3aaBDSOff();
    bool                check3aaBDSOff();

    bool                isUseIspInputCrop(void);
    bool                isUseMcscInputCrop(void);
    bool                isUseReprocessing3aaInputCrop(void);
    bool                isUseReprocessingIspInputCrop(void);
    bool                isUseReprocessingMcscInputCrop(void);
    bool                isUseEarlyFrameReturn(void);
    bool                isUse3aaDNG(void);
    bool                isUseHWFC(void);
    bool                isUseThumbnailHWFC(void) {return true;};
    bool                isHWFCOnDemand(void);
    bool                isUseRawReverseReprocessing(void);

    int                 getMaxHighSpeedFps(void);

//Added
    int                 getHDRDelay(void) { return HDR_DELAY; }
    int                 getReprocessingBayerHoldCount(void) { return REPROCESSING_BAYER_HOLD_COUNT; }
    int                 getPerFrameControlPipe(void) {return PERFRAME_CONTROL_PIPE; }
    int                 getPerFrameControlReprocessingPipe(void) {return PERFRAME_CONTROL_REPROCESSING_PIPE; }
    int                 getPerFrameInfo3AA(void) { return PERFRAME_INFO_3AA; };
    int                 getPerFrameInfoIsp(void) { return PERFRAME_INFO_ISP; };
    int                 getPerFrameInfoReprocessingPure3AA(void) { return PERFRAME_INFO_PURE_REPROCESSING_3AA; }
    int                 getPerFrameInfoReprocessingPureIsp(void) { return PERFRAME_INFO_PURE_REPROCESSING_ISP; }
    int                 getScalerNodeNumPicture(void) { return PICTURE_GSC_NODE_NUM;}

    bool                needGSCForCapture(int camId);
    bool                getSetFileCtlMode(void);
    bool                getSetFileCtl3AA(void);
    bool                getSetFileCtlISP(void);

#ifdef USE_DUAL_CAMERA
    status_t                 setStandbyState(dual_standby_state_t state);
    dual_standby_state_t     getStandbyState(void);
#endif

#ifdef SUPPORT_DEPTH_MAP
    status_t    getDepthMapSize(int *depthMapW, int *depthMapH);
    void        setDepthMapSize(int depthMapW, int depthMapH);
    bool        isDepthMapSupported(void);
#endif

    int                 getSensorGyroFormat(void);
    bool                isSensorGyroSupported(void);

#ifdef SUPPORT_PD_IMAGE
    bool isPDImageSupported(void);
    status_t getPDImageSize(int &pdImageW, int &pdImageH);
#endif

    bool                checkFaceDetectMeta(struct camera2_shot_ext *shot_ext);
    void                getFaceDetectMeta(camera2_shot_ext *shot_ext);

    int                 getNumOfCaptureFrame(void);

    void                checkPostProcessingCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_resetPreCaptureCondition(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkPreCaptureCondition(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkHIFICapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkRemosaicCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkHIFILLSCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkLongExposureCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkNightShotBayerCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkNightShotYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkSuperNightShotBayerCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkHdrBayerCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkHdrYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkFlashMultiFrameDenoiseYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkBeautyFaceYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkSuperResolutionCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkOisDenoiseYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkSportsYuvCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkCombineSingleCapture(__unused ExynosCameraRequestSP_sprt_t request);
    status_t            m_checkClaheCapture(__unused ExynosCameraRequestSP_sprt_t request);

    /* Packed 12bit + signed 1bit */
    void                getSuperNightBayerFormatIsp(int *v4l2Format, camera_pixel_size *pixelSize) { (*v4l2Format) = V4L2_PIX_FMT_SBGGR12P; (*pixelSize) = CAMERA_PIXEL_SIZE_13BIT; }
#ifdef USE_DUAL_CAMERA
    status_t            m_checkBokehCapture(__unused ExynosCameraRequestSP_sprt_t request);
#endif

    bool                getHfdMode(void);
    bool                getNfdMode(void);
    bool                getGmvMode(void);

    void                getVendorRatioCropSizeForVRA(ExynosRect *ratioCropSize);
    void                getVendorRatioCropSize(ExynosRect *ratioCropSize,
                                                           ExynosRect *mcscSize,
                                                           int portIndex,
                                                           int isReprocessing);
    bool                supportJpegYuvRotation(void);

#ifdef USES_DUAL_CAMERA_SOLUTION_ARCSOFT
    void                setArcSoftImageRatio(float imageRatio) { m_arcSoftImageRatio = imageRatio; }
    float               getArcSoftImageRatio(void) { return m_arcSoftImageRatio; }
#endif

    int                 getSupportedSensorExMode(void);
    int                 getSupportedRemosaicModes(int &numberOfMode);

#ifdef SUPPORT_SENSOR_MODE_CHANGE
    void                sensorModeTransition(bool toggle);
    bool                isSensorModeTransition();
    void                setSensorModeTransitionFrameCount(int fcount);
    int                 getSensorModeTransitionFrameCount(void);
private:
    bool                m_sensorModeTransition;
    int                 m_sensorModeTransitionFrameCount;
public:
#endif

    void                setBayerFrameLock(bool lock);
    bool                getBayerFrameLock(void);
    void                setBayerFrameLockCount(int oldCount, int newCount);

    int                 getOldBayerFrameLockCount(void);
    int                 getNewBayerFrameLockCount(void);

private:
    ExynosCameraConfigurations  *m_configurations;
    int                         m_scenario;
    int                         m_camType;
    struct camera2_shot_ext     m_metadata;         // for preview
    struct camera2_shot_ext     m_captureMetadata;  // for capture
    struct exynos_camera_info   m_cameraInfo;
    struct ExynosCameraSensorInfoBase *m_staticInfo;

    exif_attribute_t            m_exifInfo;
    debug_attribute_t           mDebugInfo;
    debug_attribute_t           mDebugInfo2;
    thumbnail_histogram_info_t  m_thumbHistInfo;
#if defined(SENSOR_FW_GET_FROM_FILE)
    ExynosCameraEEPRomMap      *m_eepromMap;
    bool                        m_flagEEPRomMapRead;
#endif
    ExynosCameraMakersNote     *m_makersNote;

    mutable Mutex               m_parameterLock;
    mutable Mutex               m_staticInfoExifLock;
    mutable Mutex               m_faceDetectMetaLock;

    ExynosCameraActivityControl *m_activityControl;

    uint32_t                    m_width[HW_INFO_SIZE_MAX];
    uint32_t                    m_height[HW_INFO_SIZE_MAX];
    bool                        m_mode[HW_INFO_MODE_MAX];

    int                         m_hwYuvWidth[YUV_OUTPUT_PORT_ID_MAX];
    int                         m_hwYuvHeight[YUV_OUTPUT_PORT_ID_MAX];

    int                         m_hwYuvInputWidth;
    int                         m_hwYuvInputHeight;

    int                         m_depthMapW;
    int                         m_depthMapH;

    bool                        m_LLSOn;

    /* Flags for camera status */
    int                         m_setfile;
    int                         m_yuvRange;
    int                         m_setfileReprocessing;
    int                         m_yuvRangeReprocessing;

#ifdef USE_DUAL_CAMERA
    dual_standby_state_t        m_standbyState;
    mutable Mutex               m_standbyStateLock;
#endif

#ifdef USE_BINNING_MODE
    int                         m_binningProperty;
#endif
    bool                        m_useSizeTable;
    bool                        m_useSensorPackedBayer;
    bool                        m_usePureBayerReprocessing;

    bool                        m_fastenAeStableOn;
    bool                        m_videoStreamExist;
    bool                        m_isLogicalCam;
    int                         m_previewPortId;
    int                         m_previewCbPortId;
    int                         m_alternativePreviewPortId;
    int                         m_recordingPortId;
    int                         m_yuvOutPortId[MAX_PIPE_NUM];
    int                         m_ysumPortId;

    bool                        m_isUniqueIdRead;

    int                         m_previewDsInputPortId;
    int                         m_captureDsInputPortId;
    ExynosRect                  m_activeZoomRect;
    float                       m_activeZoomRatio;
    int                         m_activeZoomMargin;
    ExynosRect                  m_activePictureZoomRect;
    float                       m_activePictureZoomRatio;
    int                         m_activePictureZoomMargin;

    bool                        m_bayerFrameLock;
    int                         m_oldBayerFrameLockCount;
    int                         m_newBayerFrameLockCount;

    int32_t                     m_dispCameraId;
    ExynosRect                  m_dispRect;
    FrameSizeInfoMap_t          m_dispSizeInfoMap;

/* Vedor specific API */
private:
    status_t        m_vendorReInit(void);

#ifdef USE_DUAL_CAMERA
    status_t        m_getFusionSize(int w, int h, ExynosRect *rect, bool flagSrc, int margin);
#endif

/*******************************
 VENDOR PUBLIC:
********************************/

public:
#ifdef USES_SENSOR_LISTENER
    void                                 setGyroData(ExynosCameraSensorListener::Event_t data);
    ExynosCameraSensorListener::Event_t *getGyroData(void);

    void                                 setAccelerometerData(ExynosCameraSensorListener::Event_t data);
    ExynosCameraSensorListener::Event_t *getAccelerometerData(void);
#endif

/* Additional API. */

/* Vendor specific APIs */
    void            setImageUniqueId(char *uniqueId);

/* Vendor specific adjust function */
    void            updateMetaDataParam(__unused struct camera2_shot_ext *shot);
#ifdef USES_HIFI_LLS
    void            m_setHifiLLSMeta(struct camera2_shot_ext *shot);
    int             getHifiLLSMeta(int &brightness, bool &hifillsOn);
#endif

    void            m_setLLSValue(struct camera2_shot_ext *shot);
    int             getLLSValue(void);
#ifdef USE_DUAL_CAMERA
    // TODO: Need to modify interface
    status_t            adjustDualSolutionSize(int targetWidth, int targetHeight);
    status_t            getFusionSize(int w, int h, ExynosRect *srcRect, ExynosRect *dstRect,
                                  int margin = DUAL_SOLUTION_MARGIN_VALUE_NONE);
    void                getDualSolutionSize(int *srcW, int *srcH, int *dstW, int *dstH, int orgW, int orgH, int margin = DUAL_SOLUTION_MARGIN_VALUE_NONE);
#endif
/* Vedor specific member */
private:

#ifdef LLS_CAPTURE
    int                         m_LLSValue;
    int                         m_needLLS_history[LLS_HISTORY_COUNT];
#endif

    int                         m_brightnessValue;
    bool                        m_hifillsOn;
    mutable Mutex               m_hifillsMetaLock;

#ifdef USES_SENSOR_LISTENER
    ExynosCameraSensorListener::Event_t m_gyroListenerData;
    ExynosCameraSensorListener::Event_t m_accelerometerListenerData;
#endif

/* Add vendor rearrange function */
private:
    status_t            m_vendorConstructorInitalize(int cameraId);
    void                m_vendorSWVdisMode(bool *ret);
    void                m_vendorSetExifChangedAttribute(debug_attribute_t &debugInfo,
                                              unsigned int &offset,
                                              bool &isWriteExif,
                                              camera2_shot_ext *shot_ext,
                                              bool useDebugInfo2 = false);


};

}; /* namespace android */

#endif
