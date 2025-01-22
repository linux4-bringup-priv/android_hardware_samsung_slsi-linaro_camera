/* *******************************************************************

	TItle     : Electronic Image Stabilization(EIS)
	Function : Macro & Definitions for Debug
	Author   : Duckchan Seo (duckchan.seo@samsung.com)
			   @ Samsung System LSI Sensor Product Development Team
	Date     : 2016.06.29 - Init Ver.

	Copyright @ 2016 All Rights Reserved

   ******************************************************************* */

#ifndef _DEBUG_UTILS_H_
#define _DEBUG_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#if defined(_PLATFORM_ANDROID_)
#include "ndk_log.h"
#include "Assert.h"
#else
#endif

//#define DEBUG_FUNC_LOG
//#define DEBUG_DATA_LOG
//#define DEBUG_ERROR_WARN
//#define DEBUG_SYSTEM_PAUSE
//#define DEBUG_DATA_DUMP

//#define DEBUG_TEST

#if !defined(_PLATFORM_ANDROID_)	
#ifdef DEBUG_FUNC_LOG
#define LOGFE(x)         printf("%s E \n", x);
#define LOGFX(x)         printf("%s X \n", x);

#else
#define LOGFE(x)
#define LOGFX(x)
#endif

#ifdef DEBUG_DATA_LOG
#define LOGD(fmt, ...)  printf(fmt, __VA_ARGS__);
#else
#define LOGD(fmt, ...)
#endif

#ifdef DEBUG_ERROR_WARN
#define LOGE(fmt, ...) printf("Error: " fmt, __VA_ARGS__);
#define LOGW(fmt, ...) printf("Warning: " fmt, __VA_ARGS__);
#else
#define LOGE(x)
#define LOGW(x)
#endif
#endif

#ifdef DEBUG_SYSTEM_PAUSE
#define SYS_PAUSE system("pause")
#else
#define SYS_PAUSE 
#endif


#ifdef DEBUG_DATA_DUMP
#define DATA_DUMP(buf, size, count, filename) debugDump(buf, size, count,  filename);
#else
#define DATA_DUMP(buf, size, count, filename)
#endif

#if defined(_PLATFORM_ANDROID_)
#define EIS_ASSERT(e) assert(e);
#else
#define EIS_ASSERT(e)
#endif

inline void debugDump(void* buf, int size, int count, const char* filename)
{
	FILE* fpImg;
	fpImg = fopen(filename, "wb");
	fwrite(buf, size, count, fpImg);
	fclose(fpImg);
}

#endif /*!_DEBUG_UTILS_H_*/