#ifndef _SEC_EIS_INTERFACE_H_
#define _SEC_EIS_INTERFACE_H_

#if defined(_PLATFORM_ANDROID_)
#include "ndk_log.h"
#else
//#include "win_log.h"
#endif
#include "SEC_EIS_type_def.h"
//#include "SEC_EIS_Lib.h"
#ifdef __cplusplus
extern "C" {
#endif
	extern "C" int SEC_Gyro_SelfTest(void* elg1, void* elg2, unsigned char* otp);


#ifdef __cplusplus
}
#endif

#endif /*!_SEC_EIS_INTERFACE_H_*/
