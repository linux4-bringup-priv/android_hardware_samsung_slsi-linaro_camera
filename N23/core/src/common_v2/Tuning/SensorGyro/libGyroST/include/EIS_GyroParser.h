/* *******************************************************************

TItle    : ELG Data Parser
Function : ELGDataParser Parameter Definitions
Author   : Duckchan Seo (duckchan.seo@samsung.com)
           @ Samsung System LSI Sensor Product Development Team
Date     : 2016.08.05 - Init Ver.

Copyright @ 2016 All Rights Reserved

******************************************************************* */
#ifndef _EIS_GYROPARSER_H_
#define _EIS_GYROPARSER_H_

#include "stdafx.h"
#include "EIS_Parameters.h"

#define GYRODATA_NUM        1000//2000//1632//1596 // Max GyroData in ELG Memory
#define GYRODATA_PER_SET    5
#define GYRODATA_PER_UNIT   192
#define GYRODATA_UNIT_NUM   21  // GyroData (((GYRODATA_PER_UNIT-0) x GYRODATA_UNIT_NUM) / GYRODATA_PER_SET) X 2

#ifdef __cplusplus
extern "C" {
#endif
int ParseData(ConfigParam *cp, void* pElgRaw, ELGData *pELG, GyroHeader *pGyroHeader, int nBufIndex);
int ParseBinaryGyro(int nBufIndex, ELGData *pELG, unsigned int* gyro_data, int gyro_data_num, GyroHeader *pGyroHeader);
void SplitBinaryValue(unsigned int src, unsigned int *dst);
#ifdef __cplusplus
}
#endif
#endif /*!_EIS_GYROPARSER_H_*/