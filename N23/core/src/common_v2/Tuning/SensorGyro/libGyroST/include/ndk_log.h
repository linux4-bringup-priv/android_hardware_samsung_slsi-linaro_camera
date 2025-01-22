#ifndef _NDK_LOG_H_
#define _NDK_LOG_H_

#include <android/log.h>

//#define _EIS_DEBUG_LOG_ON_
//#define _EIS_DEBUG_FUNCNAME_

#define	LOG_TAG    "EIS"

#define	LOGD_INIT(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

#ifdef _EIS_DEBUG_LOG_ON_
#define	LOGUNK(...)  __android_log_print(ANDROID_LOG_UNKNOWN,LOG_TAG,__VA_ARGS__)
#define	LOGDEF(...)  __android_log_print(ANDROID_LOG_DEFAULT,LOG_TAG,__VA_ARGS__)
#define	LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define	LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define	LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define	LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define	LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define	LOGF(...)  __android_log_print(ANDROID_FATAL_ERROR,LOG_TAG,__VA_ARGS__)
#define	LOGS(...)  __android_log_print(ANDROID_SILENT_ERROR,LOG_TAG,__VA_ARGS__)
#ifdef _EIS_DEBUG_FUNCNAME_
#define	LOGFE(x)	LOGE(x)
#define LOGFX(x)	LOGW(x)
#else
#define	LOGFE(x)
#define LOGFX(x)
#endif
#else
#define	LOGUNK(...)  
#define	LOGDEF(...)  
#define	LOGV(...)  
#define	LOGD(...)  
#define	LOGI(...)  
#define	LOGW(...)  
#define	LOGE(...)  
#define	LOGF(...)  
#define	LOGS(...)  
#define	LOGFE(x)	LOGE(x)
#define LOGFX(x)	LOGW(x)
#define LOGDC(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#endif
#endif /*!_NDK_LOG_H_*/
