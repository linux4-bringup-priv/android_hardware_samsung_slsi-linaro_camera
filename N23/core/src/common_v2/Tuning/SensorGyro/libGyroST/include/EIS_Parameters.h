/* *******************************************************************

TItle     : Electronic Image Stabilization(EIS)
Function : EIS Parameters header
Author   : Duckchan Seo (duckchan.seo@samsung.com)
@ Samsung System LSI Sensor Product Development Team
Date     : 2017.06.02 - Init Ver.

Copyright @ 2017 All Rights Reserved

******************************************************************* */

#ifndef _EIS_PARAMETERS_H_
#define _EIS_PARAMETERS_H_

#include <string>
#include <string.h>

using namespace std;

// Define

#ifndef uint8
#define uint8              unsigned char
#endif
#ifndef uint16
#define uint16             unsigned short
#endif
#ifndef uint32
#define uint32             unsigned int
#endif
#ifndef BYTE
#define BYTE               uint8
#endif

#if (_MSC_PLATFORM_TOOLSET < 140) & !defined(_PLATFORM_ANDROID_)
#   define isnan(x) _isnan(x)
#   define isinf(x) (!_finite(x))
#	define __func__ __FUNCDNAME__
#endif

#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#define MIN(a,b)  ((a) > (b) ? (b) : (a))

#define SWAP(x, y, type) { type temp; temp = x; x = y; y = temp;}

#define PI 3.1415926535f

// Gyro
#define GYRO_QUEUE_NUM           600
#define GYRO_DATA_NUM            86


struct ConfigParam {
	//frame info
	int nProcessingFrameIndex;
	int nGyroCLK_Khz;
};

struct ImageData {
	unsigned long timestamp[5];
	int nFrameIndex;
};

struct GyroData {

	int gyro_data_num;

	unsigned long timestamp[GYRO_QUEUE_NUM];

	int nMVg_X[GYRO_QUEUE_NUM];
	int nMVg_Y[GYRO_QUEUE_NUM];
	int nMVg_Z[GYRO_QUEUE_NUM];
	int nFrameIndex;
};


// Gyro
struct ELGData {

	ImageData Image;
	GyroData  Gyro;

	int mode;
	int DataNum_3P9;
	int ToTalGyroDataNum_3P9;
	int GyroData_Per_Unit;
	int GyroData_Per_Set;

	int nFrameIndex;
};

struct GyroHeader {
	unsigned int gyro_sen_start_footer_pilot;
	unsigned int gyro_sen_full_scale_sel;
	unsigned int gyro_cntrl_sel_axes;
	unsigned int gyro_cntrl_all_ts_mode;
	unsigned int gyro_num_internal_ram_chunks;
	unsigned int gyro_mem_intr_cntr;
	unsigned int gyro_sen_sample_rate_enum;
	unsigned int gyro_sen_dlpf_cfg;
	unsigned int gyro_sen_fchoice_b;
	unsigned int gyro_sen_vendor;
	unsigned int gyro_reserved;
	unsigned int gyro_err_enum;
	unsigned int gyro_max_cnt;
	unsigned int gyro_elg_16k_ram_extra_partial_chunk_size;
	unsigned int gyro_internal_mem_one_chunk_size_bytes;
	unsigned int time_stamp_intrpt_cntr;
	unsigned int reserved;
	unsigned int ts_clk_khz;
	unsigned int actually_mspi_clk_khz;
	unsigned int frame_counter;
	unsigned int footer_header_last;
	
	ImageData Image;

	int nFrameIndex;
};


#endif /*!_EIS_PARAMETERS_H_*/

