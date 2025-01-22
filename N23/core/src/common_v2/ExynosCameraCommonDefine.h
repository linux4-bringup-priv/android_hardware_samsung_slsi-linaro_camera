#ifndef EXYNOS_CAMERA_COMMON_DEFINE_H__
#define EXYNOS_CAMERA_COMMON_DEFINE_H__

#include <math.h>

#define BUILD_DATE()   ALOGE("Build Date is (%s) (%s) Process %d bit", __DATE__, __TIME__, sizeof(int*)*8)
#define WHERE_AM_I()   ALOGE("[(%s)%d] ", __FUNCTION__, __LINE__)
#define LOG_DELAY()    usleep(100000)

#define TARGET_ANDROID_VER_MAJ 4
#define TARGET_ANDROID_VER_MIN 4

/* ---------------------------------------------------------- */
/* log */
#define PREFIX_FMT   "[CAM(%d)][%s]-"
#define LOCATION_FMT "(%s[%d]):"
#define PERFRAME_FMT "[%d][FRM:%d(CAM:%d,DRV:%d,T:%d,S:%d)][REQ:%d]:"

#ifdef USE_DEBUG_PROPERTY
// logmanager init
#define LOGMGR_INIT() ExynosCameraLogManager::init()

// common log
#define CLOG_COMMON(_prio, _cameraId, _name, _fmt, ...)  \
        ExynosCameraLogManager::logCommon(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT, _cameraId, _name, \
                                          __FUNCTION__, __LINE__, _fmt, ##__VA_ARGS__)
// common log(perframe)
#define CLOG_COMMON_FRAME(_prio, _frame, _cameraId, _name, _fmt, ...) \
        do {    \
            if (_frame != nullptr) {             \
                ExynosCameraLogManager::logCommon(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT, \
                        _cameraId, _name, __FUNCTION__, __LINE__, PERFRAME_FMT _fmt,    \
                        _frame->getFactoryType(), _frame->getFrameCount(), \
                        _frame->getCameraId(), _frame->getMetaFrameCount(), \
                        _frame->getFrameType(), _frame->getFrameState(), _frame->getRequestKey(), ##__VA_ARGS__); \
            } else {    \
                ExynosCameraLogManager::logCommon(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT, \
                        _cameraId, _name, __FUNCTION__, __LINE__, PERFRAME_FMT _fmt,    \
                        -1, -1, -1, -1, -1, -1, -1, ##__VA_ARGS__); \
            }   \
        } while (0)

// perframe log
#define CLOG_PERFRAME(_type, _cameraId, _name, _frame, _data, _requestKey, _fmt, ...) \
        ExynosCameraLogManager::logPerframe(ANDROID_LOG_VERBOSE, LOG_TAG, PREFIX_FMT LOCATION_FMT PERFRAME_FMT,    \
                                           _cameraId, _name, __FUNCTION__, __LINE__, _fmt,           \
                                           ExynosCameraLogManager::PERFRAME_TYPE_ ## _type,               \
                                           _frame, _data, _requestKey, \
                                           ##__VA_ARGS__)


// performance log
#define CLOG_PERFORMANCE(_type, _cameraId, _factoryType, _time, _pos, _posSubKey, _key, ...) \
        ExynosCameraLogManager::logPerformance(ANDROID_LOG_VERBOSE, LOG_TAG, PREFIX_FMT,    \
                                           _cameraId, _factoryType,                         \
                                           ExynosCameraLogManager::PERFORMANCE_TYPE_ ## _type, \
                                           ExynosCameraLogManager::TM_ ## _time,               \
                                           ExynosCameraLogManager::PERF_ ## _type ## _ ## _pos, \
                                           _posSubKey, _key, ##__VA_ARGS__)
#else
#define LOGMGR_INIT() ((void)0)
#define CLOG_COMMON(_prio, _cameraId, _name, _fmt, ...)  \
        do {                                             \
            if (_prio == ANDROID_LOG_VERBOSE) {          \
                ALOGV(PREFIX_FMT LOCATION_FMT _fmt, _cameraId, _name, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
            } else {                                     \
                LOG_PRI(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT _fmt, _cameraId, _name, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
            }                                            \
        } while(0)
#define CLOG_PERFRAME(_type, _frame, _request, _fmt, ...) ((void)0)
#define CLOG_PERFRAME2(_type, _cameraId, _name, _frame, _request, _fmt, ...) ((void)0)
#define CLOG_PERFORMANCE(_type, _cameraId, _factoryType, _pos, _posSubKey, _key, ...) ((void)0)

// common log(perframe)
#define CLOG_COMMON_FRAME(_prio, _frame, _cameraId, _name, _fmt, ...) \
        do {    \
            if (_frame != nullptr) {             \
                LOG_PRI(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT PERFRAME_FMT _fmt, _cameraId, _name, __FUNCTION__, __LINE__,    \
                        _frame->getFactoryType(), _frame->getFrameCount(), \
                        _frame->getCameraId(), _frame->getMetaFrameCount(), \
                        _frame->getFrameType(), _frame->getFrameState(), _frame->getRequestKey(), ##__VA_ARGS__); \
            } else {    \
                LOG_PRI(_prio, LOG_TAG, PREFIX_FMT LOCATION_FMT PERFRAME_FMT _fmt, _cameraId, _name, __FUNCTION__, __LINE__,    \
                        -1, -1, -1, -1, -1, -1, -1, ##__VA_ARGS__); \
            }   \
        } while (0)
#endif

/**
 * Log with cameraId(member var) and instance name(member var)
 */
#define CLOGD(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_DEBUG, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGV(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_VERBOSE, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGW(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_WARN, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGE(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_ERROR, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGI(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_INFO, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGT(cnt, fmt, ...) \
        if (cnt != 0) CLOGI(fmt, ##__VA_ARGS__)

#define CLOG_ASSERT(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_FATAL, m_cameraId, m_name, fmt, ##__VA_ARGS__)

/**
 * Log with no cameraId and no instance name
 */
#define CLOGD2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_DEBUG, -1, "", fmt, ##__VA_ARGS__)

#define CLOGV2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_VERBOSE, -1, "", fmt, ##__VA_ARGS__)

#define CLOGW2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_WARN, -1, "", fmt, ##__VA_ARGS__)

#define CLOGE2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_ERROR, -1, "", fmt, ##__VA_ARGS__)

#define CLOGI2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_INFO, -1, "", fmt, ##__VA_ARGS__)

#define CLOG_ASSERT2(fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_FATAL, -1, "", fmt, ##__VA_ARGS__)

/**
 * Log with cameraId(param) and instance name(member var)
 */
#define CLOGV3(cameraId, fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_VERBOSE, cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGD3(cameraId, fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_DEBUG, cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGW3(cameraId, fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_WARN, cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGE3(cameraId, fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_ERROR, cameraId, m_name, fmt, ##__VA_ARGS__)

#define CLOGI3(cameraId, fmt, ...) \
        CLOG_COMMON(ANDROID_LOG_INFO, cameraId, m_name, fmt, ##__VA_ARGS__)

/**
 * Log with frame(param) and instance name(member var)
 */
#define CFLOGV(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_VERBOSE, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CFLOGD(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_DEBUG, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CFLOGW(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_WARN, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CFLOGE(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_ERROR, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CFLOGI(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_INFO, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

#define CFLOG_ASSERT(_frame, fmt, ...) \
        CLOG_COMMON_FRAME(ANDROID_LOG_FATAL, _frame, m_cameraId, m_name, fmt, ##__VA_ARGS__)

/* ---------------------------------------------------------- */
/* Debug Timer */
#define DEBUG_TIMER_INIT \
    ExynosCameraDurationTimer debugPPPTimer;
#define DEBUG_TIMER_START \
    debugPPPTimer.start();
#define DEBUG_TIMER_STOP \
    debugPPPTimer.stop(); CLOGD("DEBUG(%s[%d]): DurationTimer #0 (%lld usec)", __FUNCTION__, __LINE__, debugPPPTimer.durationUsecs());

/* Image processing */
#define SWAP(t, x, y) {t tmp = x; x = y; y = tmp;}


/* ---------------------------------------------------------- */
/* Node Prefix */
#define NODE_PREFIX "/dev/video"

/* ---------------------------------------------------------- */
/* Max Camera Name Size */
#define EXYNOS_CAMERA_NAME_STR_SIZE (256)
/* ---------------------------------------------------------- */
#define SIZE_RATIO(w, h)         ((w) * 10 / (h))
#define MAX(x, y)                (((x)<(y))? (y) : (x))

/* ---------------------------------------------------------- */
/* Sensor scenario enum for setInput */
#define SENSOR_SCENARIO_NORMAL          (0)
#define SENSOR_SCENARIO_VISION          (1)
#define SENSOR_SCENARIO_EXTERNAL        (2)
#define SENSOR_SCENARIO_OIS_FACTORY     (3)
#define SENSOR_SCENARIO_READ_ROM        (4)
#define SENSOR_SCENARIO_STANDBY         (5)
#define SENSOR_SCENARIO_ACTIVE_SENSOR_FACTORY (7)
#define SENSOR_SCENARIO_VIRTUAL         (9)
#define SENSOR_SCENARIO_MAX            (10)

#define SENSOR_SIZE_WIDTH_MASK          0xFFFF0000
#define SENSOR_SIZE_WIDTH_SHIFT         16
#define SENSOR_SIZE_HEIGHT_MASK         0xFFFF
#define SENSOR_SIZE_HEIGHT_SHIFT        0

/* ---------------------------------------------------------- */
/* Macro function */
#define XSTRING(x)         #x
#define TO_STRING(x)       (char *)XSTRING(x)
#define TO_SIZE(x,y)       (sizeof(x)/sizeof(y))
#define MAKE_STRING(s)     (char *)(#s)

/* ---------------------------------------------------------- */
/* External ID From Camera Service */
/* 0 ~ 19 : Open ID Section */
/* should be defined in camera config header */
/* #define CAMERA_OPEN_ID_REAR_0                0 */
/* #define CAMERA_OPEN_ID_FRONT_1               1 */
#define CAMERA_SERVICE_ID_OPEN_CAMERA_MAX       20
/* 20 ~ 39 : Dual Hidden ID Section */
#define CAMERA_SERVICE_ID_DUAL_REAR_ZOOM        CAMERA_SERVICE_ID_OPEN_CAMERA_MAX
#define CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_TELE    21 /* Tele Cam Main */ 
#define CAMERA_SERVICE_ID_DUAL_FRONT_PORTRAIT   22
#define CAMERA_SERVICE_ID_DUAL_REAR_PORTRAIT_WIDE    23 /* Wide Cam Main */
/* 40 ~  : Single + TOF Hidden ID Section */
#define CAMERA_SERVICE_ID_LOGICAL_REAR_TOF      40      /* Rear Main + TOF */
#define CAMERA_SERVICE_ID_LOGICAL_FRONT_TOF     41      /* Front Main + TOF */
/* 50 ~  : Single Hidden ID Section */
#define CAMERA_SERVICE_ID_REAR_2                2
#define CAMERA_SERVICE_ID_FRONT_2               51
#define CAMERA_SERVICE_ID_REAR_3                4
#define CAMERA_SERVICE_ID_FRONT_3               53
#define CAMERA_SERVICE_ID_REAR_4                54
#define CAMERA_SERVICE_ID_FRONT_4               55
/* 80 ~ :  Depth Hidden ID Section */
#define CAMERA_SERVICE_ID_REAR_TOF              80
#define CAMERA_SERVICE_ID_FRONT_TOF             81
/* 90 ~ :  Secure Hidden ID Section */
#define CAMERA_SERVICE_ID_IRIS                  90
#define CAMERA_SERVICE_ID_FRONT_SECURE          91
/* ETC. */

/* ---------------------------------------------------------- */
/* Common yuv stall size */
#define YUVSTALL_DSCALED_SIZE_16_9_W    (1920)
#define YUVSTALL_DSCALED_SIZE_16_9_H    (1080)

#define YUVSTALL_DSCALED_SIZE_4_3_W     (1920)
#define YUVSTALL_DSCALED_SIZE_4_3_H     (1440)

#define YUVSTALL_DSCALED_SIZE_1_1_W     (1088)
#define YUVSTALL_DSCALED_SIZE_1_1_H     (1088)

#define YUVSTALL_DSCALED_SIZE_18P5_9_W  (2224)
#define YUVSTALL_DSCALED_SIZE_18P5_9_H  (1080)

#define YUVSTALL_DSCALED_SIZE_19_9_W    (2288)
#define YUVSTALL_DSCALED_SIZE_19_9_H    (1080)

#define YUVSTALL_DSCALED_SIZE_19P5_9_W  (2352)
#define YUVSTALL_DSCALED_SIZE_19P5_9_H  (1080)

/* Replace '__unused' keyword*/
#define UNUSED_VARIABLE(x) (void)(x)

#endif /* EXYNOS_CAMERA_COMMON_DEFINE_H__ */

