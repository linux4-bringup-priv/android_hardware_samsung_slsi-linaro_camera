#ifndef MPI_LLS_INTERFACE_H
#define MPI_LLS_INTERFACE_H

#define ZOOM_LEVEL_MIN 1.0
#define ZOOM_LEVEL_MAX 10.0

#define MAX_NUMFACES 10

namespace mpi {

namespace lls {

enum OPERATION_MODE {
    LLS_MODE = 0,
    BEST_PICK_MODE,
    SRZOOM_MODE = 3,
};

struct Rect {
    int l;
    int t;
    int r;
    int b;
};

struct BufferData {
    BufferData() : y(NULL), uv(NULL), fdY(-1), fdUV(-1), width(-1), height(-1), zoomRatio(1.0) {
        fov.l = 0;
        fov.t = 0;
        fov.r = 0;
        fov.b = 0;
    }
    char *y;
    char *uv;
    int fdY;
    int fdUV;
    int width;
    int height;
    float zoomRatio;
    Rect fov;
};

struct Fraction {
    Fraction() = default;
    Fraction(unsigned int _den, unsigned int _num) :
        den(_den), num(_num) {}
    unsigned int den;
    union {
        unsigned int num; /* unsigned numerator */
        int snum; /* signed numerator */
    };
};

struct ExtraInfo {
    unsigned int iso[2];
    unsigned int hdrMode;
    Fraction exposureTime;
    Fraction brightnessValue;
    Fraction shutterSpeed[2];
    Fraction exposureValue;
};

struct ExtraCameraInfo {
    bool isDual;
    int fullWidth;
    int fullHeight;
    int mainSensorType;
    int auxSensorType;
};

/**
 * @brief
 * Constructs new LLSInterface instance
 * in a separate thread.
 *
 * @param[inout] arg  pointer to user argument
 *
  * @return
 */
int construct(void **arg);

/**
 * @brief
 * Destruct LLSInterface instance
 * in a separate thread.
 *
 * @param[inout] arg  pointer to pointer to user argument
 *
  * @return
 */
int deconstruct(void **arg);

/**
 * @brief
 * Initializes LLS usecase
 *
 * @param[inout] arg  pointer to user argument
 *
  * @return
 */
int initialize(void **arg);

/**
 * @brief
 * Deinitializes LLS usecase
 *
 * @param[inout] arg  user argument
 *
  * @return
 */
int deinitialize(void *arg);

/**
 * @brief
 * Indicate maximum amount frames used during LLS
 *
 * @param[in] arg user argument
 * @param[in] num number of frames sent to LLS
 *
 * @return
 */
int setTotalBufferNum(void *arg, int num);

/**
 * @brief
 * Get total number of frames used for LLS
 *
 * @param[in] arg user argument
 *
 * @return >= 0 image num.
 */
int getTotalBufferNum(void *arg);

/**
 * @brief
 * Enqueue input frame to LLS
 *
 * @param[in] arg user argument
 * @param[in] data frame data
 *
  * @return
 */
int enqueueBuffer(void *arg, const BufferData &data);

/**
 * @brief
 * Enqueue output frame to LLS
 *
 * @param[in] arg user argument
 * @param[in] data frame data
 *
  * @return
 */
int setOutputBuffer(void *arg, const BufferData &data);

/**
 * @brief
 * Get input frame information
 *
 * @param[in] arg user argument
 *
  * @return Input buffer data
 */
BufferData getInputBuffer(void *arg);

/**
 * @brief
 * Get output frame information
 *
 * @param[in] arg user argument
 *
  * @return Output buffer data
 */
BufferData getOutputBuffer(void *arg);

/**
 * @brief
 * Set extra info (metadata) for frames sent to LLS
 *
 * @param[in] arg user argument
 * @param[in] info extra information in frames
 *
  * @return
 */
int setExtraInfo(void *arg, const ExtraInfo &info);

/**
 * @brief
 * Write debug information for Exif buffer
 *
 * @param[in] arg user argument
 * @param[in] data pointer to debug buffer
 *
  * @return Amount of bytes written to debug buffer
 */
int writeDebugBuffer(void *arg, unsigned char *p_data);

/**
 * @brief
 * Set extra camera information
 *
 * @param[in] arg user argument
 * @param[in] info extra information of camera
 *
  * @return
 */
int setExtraCameraInfo(void *arg, const ExtraCameraInfo &info);

/**
 * @brief
 * Set camera and platform info
 *
 * @param[in] arg user argument
 * @param[in] type camera sensor type (front, back, etc.)
 * @param[in] sensor sensor model
 * @param[in] platform ID for application processor
 *
  * @return
 */
int setCameraInfo(void *arg, int type, int sensor, int platform);

/**
 * @brief
 * Operation mode LLS
 *
 * @param[in] arg user argument
 * @param[in] mode operate mode information
 *
  * @return
 */
int setOperateMode(void *arg, int mode);

/**
 * @brief
 * Gyro sensor state LLS
 *
 * @param[in] arg user argument
 * @param[in] sensor state information (0 for stationary; others for moving)
 *
  * @return
 */
int setSensorState(void *arg, int sensor_state);

/**
 * @brief
 * Send face rectangles to LLS
 *
 * @param[in] arg user argument
 * @param[in] faces array of face rectangles
 * @param[in] num number of rectangles in the list
 *
  * @return
 */
int setFaceData(void *arg, Rect *key_points, int num);

/**
 * @brief
 * Process LLS algo with the frames enqueued
 *
 * @param[in] arg user argument
 *
  * @return
 */
int process(void *arg);

} // namespace lls

} // namespace mpi

#endif  // MPI_LLS_INTERFACE_H
