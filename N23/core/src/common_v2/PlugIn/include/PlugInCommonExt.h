#ifndef PLUG_IN_COMMON_EXT_H
#define PLUG_IN_COMMON_EXT_H

// refer : ExynosCameraUtils.h
#ifdef SAFE_DELETE
#else
#define SAFE_DELETE(obj) \
    do { \
        if (obj) { \
            delete obj; \
            obj = NULL; \
        } \
    } while(0)
#endif

typedef struct {
    int x;
    int y;
    int z;
    float x_bias;
    float y_bias;
    float z_bias;
    uint64_t timestamp;
}plugin_gyro_data_t;

typedef plugin_gyro_data_t* Array_pointer_gyro_data_t;

enum PLUGIN_PARAMETER_KEY {
    PLUGIN_PARAMETER_KEY_PREPARE = 1,
    PLUGIN_PARAMETER_KEY_START,
    PLUGIN_PARAMETER_KEY_STOP,
    PLUGIN_PARAMETER_KEY_GET_SCENARIO,
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
}plugin_rect_t;

typedef plugin_rect_t* Data_pointer_rect_t;

#define PLUGIN_VPL_MAX_FACES (16)
typedef int32_t *  Array_vpl_int32_t;
typedef float *  Array_vpl_float_t;
//typedef float (* Array_vpl_rect_t)[4];
typedef uint32_t * Array_vpl_uint32_t;

#endif //PLUG_IN_COMMON_EXT_H
