//------------------------------------------------------------------------------
/// @file  addbayer.h
/// @ingroup  include
///
/// @brief  Declarations AddBayer header files
/// @internal
/// @author  Bumsuk Kim<bs48.kim@samsung.com>
///
/// @section changelog Change Log
/// 2019/04/25 Bumsuk Kim created
///
/// @section copyright_section Copyright
/// &copy; 2019, Samsung Electronics Co., Ltd.
///
/// @section license_section License
//------------------------------------------------------------------------------

#ifndef ADDBAYER_H_
#define ADDBAYER_H_

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef int int32_t;

enum status_t {
    SUCCESS = 0,
    FAILURE
};

namespace lec {
status_t addBayerBufferByNeon(char* input, char* output, uint32_t copySize);
status_t addBayerBufferByCpu(char* input, char* output, uint32_t copySize);
status_t addBayerBufferByNeonPacked(char* input, char* output, uint32_t copySize);
status_t addBayerBufferByCpuPacked(char* input, char* output, uint32_t copySize);
} // namespace

#endif
