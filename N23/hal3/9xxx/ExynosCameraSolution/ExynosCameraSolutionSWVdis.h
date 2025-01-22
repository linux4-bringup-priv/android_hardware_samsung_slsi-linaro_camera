#ifndef EXYNOS_CAMERA_SOLUTION_SW_VDIS_H
#define EXYNOS_CAMERA_SOLUTION_SW_VDIS_H

namespace android {

class ExynosCameraFrame;
class ExynosCameraConfigurations;
class ExynosCameraParameters;
class ExynosCameraFrameFactory;
class ExynosCameraBufferSupplier;

class ExynosCameraSolutionSWVdis
{
public:
    typedef enum {
        SOLUTION_PROCESS_PRE,
        SOLUTION_PROCESS_POST,
        SOLUTION_PROCESS_MAX
    }SOLUTION_PROCESS_TYPE;

public:
    ExynosCameraSolutionSWVdis(int cameraId, int pipeId,
                                       ExynosCameraParameters *parameters,
                                       ExynosCameraConfigurations *configurations);

    virtual ~ExynosCameraSolutionSWVdis();

public:
    status_t configureStream(void);
    status_t storeOriginalSize(void);
    status_t flush(ExynosCameraFrameFactory* frameFactory = NULL);
    status_t setBuffer(ExynosCameraBufferSupplier* bufferSupplier);
    status_t handleFrame(SOLUTION_PROCESS_TYPE type,
                            sp<ExynosCameraFrame> frame,
                            int prevPipeId,
                            int capturePipeId,
                            ExynosCameraFrameFactory *factory);

    int getPipeId(void);
    int getCapturePipeId(void);
    void checkMode(void);
    void getSize(int& w, int& h);
    uint64_t adjustTimeStamp(uint32_t frameCount);

private:
    status_t m_adjustSize(void);
    status_t m_handleFramePreProcess(sp<ExynosCameraFrame> frame,
                            int prevPipeId,
                            int capturePipeId,
                            ExynosCameraFrameFactory *factory);

    status_t m_handleFramePostProcess(sp<ExynosCameraFrame> frame,
                            int prevPipeId,
                            __unused int capturePipeId,
                            __unused ExynosCameraFrameFactory *factory);

    bool     m_isEnabledByUser();
    status_t m_collectTimeStamp(int index, uint64_t timeStamp);
    void     m_makePair(uint32_t frameCount, int bufferIndex);
    int      m_findBufferIndexWith(uint32_t frameCount);
    status_t m_getSize(int& w, int& h);

private:
    int m_cameraId;
    char m_name[256];
    int m_pipeId;
    int m_capturePipeId;
    ExynosCameraConfigurations *m_pConfigurations;
    ExynosCameraParameters *m_pParameters;
    ExynosCameraBufferSupplier* m_pBufferSupplier;
    ExynosCameraFrameFactory* m_pFrameFactory;
    uint64_t m_timeStampList[NUM_SW_VDIS_INTERNAL_BUFFERS + 1];
    map<uint32_t, int> m_frameCountBufferIndexMap;
    mutable Mutex m_frameCountBufferIndexLock;
    ExynosRect m_previewOriginalSize[MAX_NUM_SENSORS];
    ExynosRect m_videoOriginalSize[MAX_NUM_SENSORS];
};

};

#endif

