#ifndef _SEC_EIS_IF_Lib_H_
#define _SEC_EIS_IF_Lib_H_

#include "SEC_EIS_type_def.h"
#include "DebugUtils.h"
#include "EIS_Parameters.h"

#ifdef __cplusplus
extern "C" {
#endif

// setting & processing
int SEC_EIS_Lib_init(void);
int SEC_EIS_Lib_deinit(void);
int SEC_EIS_Lib_runGyroParser(void* pElg1, void* pElg2, unsigned char* pOTP);

#endif
#ifdef __cplusplus
}
#endif

