# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23
CAMERA_SRC_PATH := ../..

#################
# libexynoscamera3

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true

LOCAL_STATIC_LIBRARIES := android.hardware.camera.common@1.0-helper
LOCAL_SHARED_LIBRARIES:= libutils libcutils liblog libui libcamera_metadata libutilscallstack
LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libion_exynos libsync libcsc
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libexynosgraphicbuffer

LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-overloaded-virtual
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable
LOCAL_CFLAGS += -Wno-implicit-fallthrough
LOCAL_CFLAGS += -Wno-error=implicit-fallthrough

LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS3830_META

LOCAL_CFLAGS += -DMAIN_CAMERA_SENSOR_NAME=$(BOARD_BACK_CAMERA_SENSOR)
$(warning MAIN_CAMERA_SENSOR_NAME is $(BOARD_BACK_CAMERA_SENSOR))
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_NAME=$(BOARD_FRONT_CAMERA_SENSOR)
$(warning FRONT_CAMERA_SENSOR_NAME is $(BOARD_FRONT_CAMERA_SENSOR))
LOCAL_CFLAGS += -DSECURE_CAMERA_SENSOR_NAME=$(BOARD_SECURE_CAMERA_SENSOR)
LOCAL_CFLAGS += -DUSE_CAMERA_ESD_RESET
LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)
LOCAL_CFLAGS += -DSECURE_ROTATION=$(BOARD_SECURE_CAMERA_ROTATION)
ifneq ($(BOARD_BACK_1_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_1_CAMERA_SENSOR_NAME=$(BOARD_BACK_1_CAMERA_SENSOR)
$(warning BACK_1_CAMERA_SENSOR_NAME is $(BOARD_BACK_1_CAMERA_SENSOR))
endif
ifneq ($(BOARD_FRONT_1_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_1_CAMERA_SENSOR_NAME=$(BOARD_FRONT_1_CAMERA_SENSOR)
endif
ifneq ($(BOARD_BACK_2_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_2_CAMERA_SENSOR_NAME=$(BOARD_BACK_2_CAMERA_SENSOR)
$(warning BOARD_BACK_2_CAMERA_SENSOR is $(BOARD_BACK_2_CAMERA_SENSOR))
endif
ifneq ($(BOARD_FRONT_2_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_2_CAMERA_SENSOR_NAME=$(BOARD_FRONT_2_CAMERA_SENSOR)
endif
ifneq ($(BOARD_BACK_3_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_3_CAMERA_SENSOR_NAME=$(BOARD_BACK_3_CAMERA_SENSOR)
endif
ifneq ($(BOARD_FRONT_3_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_3_CAMERA_SENSOR_NAME=$(BOARD_FRONT_3_CAMERA_SENSOR)
endif

ifeq ($(BOARD_BACK_1_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_1=2
endif
ifeq ($(BOARD_FRONT_1_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_1=3
endif
ifeq ($(BOARD_BACK_2_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_2=4
endif
ifeq ($(BOARD_FRONT_2_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_2=5
endif
ifeq ($(BOARD_BACK_3_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_3=6
endif
ifeq ($(BOARD_FRONT_3_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_3=7
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA), true)
LOCAL_CFLAGS += -DUSE_DUAL_CAMERA
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_ZOOM_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_ZOOM_MASTER=$(BOARD_DUAL_CAMERA_REAR_ZOOM_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_ZOOM_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_ZOOM_SLAVE=$(BOARD_DUAL_CAMERA_REAR_ZOOM_SLAVE)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_PORTRAIT_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_PORTRAIT_MASTER=$(BOARD_DUAL_CAMERA_REAR_PORTRAIT_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_PORTRAIT_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_PORTRAIT_SLAVE=$(BOARD_DUAL_CAMERA_REAR_PORTRAIT_SLAVE)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_FRONT_PORTRAIT_MASTER=$(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_FRONT_PORTRAIT_SLAVE=$(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_SLAVE)
endif
endif

ifeq ($(BOARD_SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA), true)
LOCAL_CFLAGS += -DSUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_FAKE), true)
LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_FAKE
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_ARCSOFT), true)
LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_ARCSOFT
endif

ifeq ($(BOARD_CAMERA_USES_PIPE_HANDLER), true)
LOCAL_CFLAGS += -DUSE_PIPE_HANDLER
endif

ifeq ($(BOARD_CAMERA_USES_SBWC), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_USES_SBWC
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
LOCAL_CFLAGS += -DUSES_SW_VDIS

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_SUPER_EIS), true)
LOCAL_CFLAGS += -DUSE_SUPER_EIS
endif

endif

ifeq ($(BOARD_CAMERA_USES_REMOSAIC_SENSOR), true)
LOCAL_CFLAGS += -DUSE_REMOSAIC_SENSOR
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_VPL), true)
LOCAL_CFLAGS += -DUSES_VPL_PRELOAD
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_LLS_CAPTURE), true)
LOCAL_CFLAGS += -DUSES_HIFI_LLS
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_CAPTURE), true)
LOCAL_CFLAGS += -DUSES_HIFI
endif

ifeq ($(BOARD_CAMERA_USES_SENSOR_GYRO_FACTORY_MODE), true)
LOCAL_CFLAGS += -DUSES_SENSOR_GYRO_FACTORY_MODE
endif

ifeq ($(BOARD_CAMERA_USES_P3_IN_EXIF), true)
LOCAL_CFLAGS += -DUSES_P3_IN_EXIF
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/P3
endif

ifeq ($(BOARD_CAMERA_USES_CLAHE), true)
LOCAL_CFLAGS += -DUSE_CLAHE_REPROCESSING
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/SensorInfos \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libsync/include \
	$(TOP)/system/core/libion/kernel-headers \
	$(TOP)/system/memory/libion/kernel-headers \
	$(CAMERA_PATH)/core/src/9xxx \
	$(CAMERA_PATH)/core/src/common_v2 \
	$(CAMERA_PATH)/core/src/common_v2/Activities \
	$(CAMERA_PATH)/core/src/common_v2/Buffers \
	$(CAMERA_PATH)/core/src/common_v2/MCPipes \
	$(CAMERA_PATH)/core/src/common_v2/Pipes2 \
	$(CAMERA_PATH)/core/src/common_v2/PostProcessing \
	$(CAMERA_PATH)/core/src/common_v2/OfflineProcessing \
	$(CAMERA_PATH)/core/src/common_v2/SensorInfos \
	$(CAMERA_PATH)/core/src/common_v2/SizeTables \
	$(CAMERA_PATH)/core/src/common_v2/MakersNote \
	$(CAMERA_PATH)/core/src/common_v2/MakersNote/Default \
	$(CAMERA_PATH)/core/src/common_v2/EEPRomMap \
	$(CAMERA_PATH)/core/src/common_v2/Tuning \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/LEDCalibration \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/SensorGyro \
	$(CAMERA_PATH)/hal3/common_v2/Sec \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos5/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/hardware/libhardware_legacy/include/hardware_legacy \
	$(TOP)/bionic \
	$(TOP)/external/expat/lib \
	$(TOP)/frameworks/native/libs/binder/include \
    $(TOP)/frameworks/native/libs/ui/include \
	$(TOP)/hardware/samsung_slsi/exynos/libion/include \
	$(TOP)/external/libcxx/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/frameworks/native/libs/arect/include \
	$(TOP)/frameworks/native/libs/nativebase/include \
	$(TOP)/frameworks/av/include \
	$(TOP)/hardware/interfaces/camera/common/1.0/default/include \

$(warning VAR is $(LOCAL_C_INCLUDES))

ifeq ($(BOARD_CAMERA_USES_SLSI_PLUGIN_N23), true)
LOCAL_CFLAGS += -DUSE_SLSI_PLUGIN
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/converter
endif

ifeq ($(BOARD_CAMERA_USES_SBWC), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_USES_SBWC
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/hal3/9xxx/ExynosCameraSolution
endif

ifeq ($(BOARD_CAMERA_USES_EFD), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_EARLY_FD
endif

ifeq ($(BOARD_CAMERA_USES_3AA_DNG), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_3AA_DNG
endif

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Vendor/$(LOCAL_PROJECT_DIR)
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SensorInfos
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA), true)
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/Fusion \
    $(CAMERA_PATH)/core/src/common_v2/Fusion/DofLut
endif

LOCAL_SRC_FILES = \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraFrame.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraMemory.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraFrameManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraUtils.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraNode.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraNodeJpegHAL.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraFrameSelector.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraFrameFactoryBase.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraCallback.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/SensorInfos/ExynosCameraSensorInfoBase.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PostProcessing/ExynosCameraPP.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PostProcessing/ExynosCameraPPLibcsc.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PostProcessing/ExynosCameraPPJPEG.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PostProcessing/ExynosCameraPPGDC.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PostProcessing/ExynosCameraPPFactory.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MakersNote/ExynosCameraMakersNote.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MakersNote/Default/ExynosCameraMakersNoteDefault.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MakersNote/ExynosCameraMakersNoteFactory.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapDefault.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap2P7SQ.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap6B2.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapGM1SP.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap2X5SP.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap5E9.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMap5E9_OTP.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapOV12A10.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapOV12A10FF.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapOV16885C.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/EEPRomMap/ExynosCameraEEPRomMapFactory.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MCPipes/ExynosCameraMCPipe.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MCPipes/ExynosCameraPipeImageDump.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipe.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraSWPipe.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeFlite.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeGSC.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeJpeg.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeVRA.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeHFD.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipePP.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/Pipes2/ExynosCameraPipePPVendor.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeSWMCSC.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeMultipleJpeg.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Buffers/ExynosCameraBufferManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Buffers/ExynosCameraBufferManagerVendor.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Buffers/ExynosCameraBufferSupplier.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Buffers/ExynosCameraInternalBufferManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Buffers/ExynosCameraServiceBufferManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Activities/ExynosCameraActivityBase.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Activities/ExynosCameraActivityAutofocus.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Activities/ExynosCameraActivityFlash.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Activities/ExynosCameraActivitySpecialCapture.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Activities/ExynosCameraActivityUCTL.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/Activities/ExynosCameraActivityAutofocusVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/Activities/ExynosCameraActivityFlashVendor.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Tuning/LEDCalibration/awb_cal.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Tuning/SensorGyro/ExynosCameraFactoryTestSensorGyro.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Tuning/ExynosCameraFactoryTest.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Tuning/ExynosCameraFactoryTestFactory.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraRequestManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraResourceManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraVendorMetaData.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraStreamManager.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraMetadataConverter.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraTimeLogger.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/ExynosCameraFrameSelectorVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/ExynosCameraMetadataConverterVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/ExynosCameraRequestManagerVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/ExynosCameraStreamManagerVendor.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraActivityControl.cpp\
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCamera.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraParameters.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraConfigurations.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraSizeControl.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameFactory.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameFactoryPreview.cpp \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraFrameFactoryPreviewVendor.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameFactoryVision.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameReprocessingFactory.cpp \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraFrameReprocessingFactoryVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraConfigurationsVendor.cpp \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraParametersVendor.cpp

ifeq ($(BOARD_CAMERA_USES_SLSI_PLUGIN_N23), true)
LOCAL_SHARED_LIBRARIES += libdl libexynoscamera_plugin
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/ExynosCameraPlugInConverter.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipePlugIn.cpp \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFactoryPlugIn.cpp
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA), true)
LOCAL_SRC_FILES += \
    $(CAMERA_SRC_PATH)/core/src/common_v2/Pipes2/ExynosCameraPipeSync.cpp \
    $(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameFactoryPreviewDual.cpp \
    $(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameReprocessingFactoryDual.cpp
endif

ifeq ($(BOARD_CAMERA_USES_REMOSAIC_SENSOR), true)
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/9xxx/ExynosCameraFrameReprocessingFactoryRemosaic.cpp
endif

ifneq ($(filter eng, $(TARGET_BUILD_VARIANT)),)
LOCAL_CFLAGS += -DUSE_DEBUG_PROPERTY
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraProperty.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraLogManager.cpp
endif

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_SRC_FILES += ./Vendor/$(LOCAL_PROJECT_DIR)/ExynosCameraSensorInfo.cpp
else
LOCAL_SRC_FILES += ./SensorInfos/ExynosCameraSensorInfo.cpp
endif

ifeq ($(BOARD_CAMERA_USES_PIPE_HANDLER), true)
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/MCPipes/ExynosCameraPipeHandler.cpp
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/hal3/9xxx/ExynosCameraSolution/ExynosCameraSolutionSWVdis.cpp
endif

ifeq ($(BOARD_CAMERA_USES_SLSI_VENDOR_TAGS), true)
LOCAL_CFLAGS += -DUSE_SLSI_VENDOR_TAGS
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraVendorTags.cpp \
	$(CAMERA_SRC_PATH)/hal3/common_v2/Sec/ExynosCameraVendorUtils.cpp
endif

ifeq ($(BOARD_CAMERA_USES_SENSOR_LISTENER), true)
LOCAL_CFLAGS += -DUSES_SENSOR_LISTENER

LOCAL_SHARED_LIBRARIES += \
    libhardware \
    libsensorndkbridge

LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/SensorListener

LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/SensorListener/ExynosCameraSensorListener.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/SensorListener/ExynosCameraSensorListenerDummy.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/SensorListener/ExynosCameraSensorListenerASensor.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/SensorListener/ExynosCameraSensorListenerWrapper.cpp
endif

ifeq ($(BOARD_USE_GIANT_MSCL), true)
LOCAL_CFLAGS += -DUSE_GIANT_MSCL
LOCAL_SHARED_LIBRARIES += libgiantmscl
endif

LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/OfflineProcessing/ExynosCameraOfflineCapture.cpp

$(foreach file,$(LOCAL_SRC_FILES),$(shell touch '$(LOCAL_PATH)/$(file)'))

LOCAL_LDFLAGS :=  -Wl,-Bsymbolic

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libexynoscamera3

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk

ifeq ($(BOARD_CAMERA_USES_SLSI_PLUGIN_N23), true)
include $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/Android.mk
endif

include $(BUILD_SHARED_LIBRARY)


#################
# camera.exynos850.so

include $(CLEAR_VARS)

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/SensorInfos \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libsync/include \
	$(TOP)/system/core/libion/kernel-headers \
	$(CAMERA_PATH)/core/src/9xxx \
	$(CAMERA_PATH)/core/src/common_v2 \
	$(CAMERA_PATH)/core/src/common_v2/Activities \
	$(CAMERA_PATH)/core/src/common_v2/Buffers \
	$(CAMERA_PATH)/core/src/common_v2/MCPipes \
	$(CAMERA_PATH)/core/src/common_v2/Pipes2 \
	$(CAMERA_PATH)/core/src/common_v2/PostProcessing \
	$(CAMERA_PATH)/core/src/common_v2/SensorInfos \
	$(CAMERA_PATH)/core/src/common_v2/SizeTables \
	$(CAMERA_PATH)/core/src/common_v2/MakersNote \
	$(CAMERA_PATH)/core/src/common_v2/MakersNote/Default \
	$(CAMERA_PATH)/core/src/common_v2/EEPRomMap \
	$(CAMERA_PATH)/core/src/common_v2/Tuning \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/LEDCalibration \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/SensorGyro \
	$(CAMERA_PATH)/hal3/common_v2/Sec \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/hardware/samsung_slsi/exynos5/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi/$(TARGET_BOARD_PLATFORM)/include \
	$(TOP)/external/libcxx/include \
	$(TOP)/bionic \
	$(TOP)/frameworks/native/include \
	$(TOP)/frameworks/native/libs/nativebase/include \
	$(TOP)/frameworks/native/libs/arect/include \
	$(TOP)/frameworks/native/libs/binder/include \
	$(TOP)/frameworks/native/libs/ui/include \
	$(TOP)/hardware/samsung_slsi/exynos/libion/include \
	$(TOP)/hardware/interfaces/camera/common/1.0/default/include

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA), true)
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/Fusion \
    $(CAMERA_PATH)/core/src/common_v2/Fusion/DofLut
endif

ifneq ($(LOCAL_PROJECT_DIR),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Vendor/$(LOCAL_PROJECT_DIR)
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SensorInfos
endif

LOCAL_SRC_FILES:= \
	$(CAMERA_SRC_PATH)/core/src/common_v2/ExynosCameraInterface.cpp

ifneq ($(filter eng, $(TARGET_BUILD_VARIANT)),)
LOCAL_CFLAGS += -DUSE_DEBUG_PROPERTY
endif
$(warning TARGET_PRODUCT is $(TARGET_PRODUCT))
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-overloaded-virtual
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable
LOCAL_CFLAGS += -Wno-implicit-fallthrough
LOCAL_CFLAGS += -Wno-error=implicit-fallthrough

LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS3830_META

LOCAL_CFLAGS += -DMAIN_CAMERA_SENSOR_NAME=$(BOARD_BACK_CAMERA_SENSOR)
$(warning MAIN_CAMERA_SENSOR_NAME is $(BOARD_BACK_CAMERA_SENSOR))
LOCAL_CFLAGS += -DFRONT_CAMERA_SENSOR_NAME=$(BOARD_FRONT_CAMERA_SENSOR)
$(warning FRONT_CAMERA_SENSOR_NAME is $(BOARD_FRONT_CAMERA_SENSOR))
LOCAL_CFLAGS += -DSECURE_CAMERA_SENSOR_NAME=$(BOARD_SECURE_CAMERA_SENSOR)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_0=0
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_0=1
LOCAL_CFLAGS += -DBACK_ROTATION=$(BOARD_BACK_CAMERA_ROTATION)
LOCAL_CFLAGS += -DFRONT_ROTATION=$(BOARD_FRONT_CAMERA_ROTATION)
LOCAL_CFLAGS += -DSECURE_ROTATION=$(BOARD_SECURE_CAMERA_ROTATION)
ifneq ($(BOARD_BACK_1_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_1_CAMERA_SENSOR_NAME=$(BOARD_BACK_1_CAMERA_SENSOR)
$(warning BACK_1_CAMERA_SENSOR_NAME is $(BOARD_BACK_1_CAMERA_SENSOR))
endif
ifneq ($(BOARD_FRONT_1_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_1_CAMERA_SENSOR_NAME=$(BOARD_FRONT_1_CAMERA_SENSOR)
endif
ifneq ($(BOARD_BACK_2_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_2_CAMERA_SENSOR_NAME=$(BOARD_BACK_2_CAMERA_SENSOR)
$(warning BOARD_BACK_2_CAMERA_SENSOR is $(BOARD_BACK_2_CAMERA_SENSOR))
endif
ifneq ($(BOARD_FRONT_2_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_2_CAMERA_SENSOR_NAME=$(BOARD_FRONT_2_CAMERA_SENSOR)
endif
ifneq ($(BOARD_BACK_3_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DBACK_3_CAMERA_SENSOR_NAME=$(BOARD_BACK_3_CAMERA_SENSOR)
endif
ifneq ($(BOARD_FRONT_3_CAMERA_SENSOR), )
LOCAL_CFLAGS += -DFRONT_3_CAMERA_SENSOR_NAME=$(BOARD_FRONT_3_CAMERA_SENSOR)
endif

ifeq ($(BOARD_BACK_1_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_1=2
endif
ifeq ($(BOARD_FRONT_1_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_1=3
endif
ifeq ($(BOARD_BACK_2_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_2=4
endif
ifeq ($(BOARD_FRONT_2_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_2=5
endif
ifeq ($(BOARD_BACK_3_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_BACK_3=6
endif
ifeq ($(BOARD_FRONT_3_CAMERA_SENSOR_OPEN), true)
LOCAL_CFLAGS += -DCAMERA_OPEN_ID_FRONT_3=7
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA), true)
LOCAL_CFLAGS += -DUSE_DUAL_CAMERA
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_ZOOM_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_ZOOM_MASTER=$(BOARD_DUAL_CAMERA_REAR_ZOOM_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_ZOOM_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_ZOOM_SLAVE=$(BOARD_DUAL_CAMERA_REAR_ZOOM_SLAVE)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_PORTRAIT_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_PORTRAIT_MASTER=$(BOARD_DUAL_CAMERA_REAR_PORTRAIT_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_REAR_PORTRAIT_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_REAR_PORTRAIT_SLAVE=$(BOARD_DUAL_CAMERA_REAR_PORTRAIT_SLAVE)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_MASTER)))
LOCAL_CFLAGS += -DDUAL_CAMERA_FRONT_PORTRAIT_MASTER=$(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_MASTER)
endif
ifneq (, $(findstring CAMERA_ID, $(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_SLAVE)))
LOCAL_CFLAGS += -DDUAL_CAMERA_FRONT_PORTRAIT_SLAVE=$(BOARD_DUAL_CAMERA_FRONT_PORTRAIT_SLAVE)
endif
endif

ifeq ($(BOARD_SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA), true)
LOCAL_CFLAGS += -DSUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
endif

ifeq ($(BOARD_CAMERA_USES_EFD), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_EARLY_FD
endif

ifeq ($(BOARD_CAMERA_USES_3AA_DNG), true)
LOCAL_CFLAGS += -DBOARD_CAMERA_3AA_DNG
endif

ifeq ($(BOARD_CAMERA_USES_SLSI_VENDOR_TAGS), true)
LOCAL_CFLAGS += -DUSE_SLSI_VENDOR_TAGS
endif

ifeq ($(BOARD_CAMERA_USES_SENSOR_LISTENER), true)
LOCAL_CFLAGS += -DUSES_SENSOR_LISTENER

LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/SensorListener
endif

ifeq ($(BOARD_CAMERA_USES_SLSI_PLUGIN_N23), true)
LOCAL_CFLAGS += -DUSE_SLSI_PLUGIN
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/converter
endif

LOCAL_CFLAGS += -DUSE_CAMERA_ESD_RESET

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_FAKE), true)
	LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_FAKE
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_ARCSOFT), true)
	LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_ARCSOFT
endif

ifeq ($(BOARD_CAMERA_USES_PIPE_HANDLER), true)
	LOCAL_CFLAGS += -DUSE_PIPE_HANDLER
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
	LOCAL_CFLAGS += -DUSES_SW_VDIS
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/hal3/9xxx/ExynosCameraSolution

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_SUPER_EIS), true)
LOCAL_CFLAGS += -DUSE_SUPER_EIS
endif

endif

ifeq ($(BOARD_CAMERA_USES_REMOSAIC_SENSOR), true)
	LOCAL_CFLAGS += -DUSE_REMOSAIC_SENSOR
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_LLS_CAPTURE), true)
	LOCAL_CFLAGS += -DUSES_HIFI_LLS
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_CAPTURE), true)
	LOCAL_CFLAGS += -DUSES_HIFI
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_VPL), true)
LOCAL_CFLAGS += -DUSES_VPL_PRELOAD
endif

ifeq ($(BOARD_CAMERA_USES_SENSOR_GYRO_FACTORY_MODE), true)
LOCAL_CFLAGS += -DUSES_SENSOR_GYRO_FACTORY_MODE
endif

ifeq ($(BOARD_CAMERA_USES_P3_IN_EXIF), true)
LOCAL_CFLAGS += -DUSES_P3_IN_EXIF
LOCAL_C_INCLUDES += \
	$(CAMERA_PATH)/core/src/common_v2/Tuning/P3
endif

ifeq ($(BOARD_USE_GIANT_MSCL), true)
LOCAL_CFLAGS += -DUSE_GIANT_MSCL
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/graphics/base/giantmscl/include
endif

LOCAL_SHARED_LIBRARIES:= liblog libhardware libutils libion_exynos libhwjpeg libnativewindow libnativewindow libutilscallstack
LOCAL_SHARED_LIBRARIES += libexynoscamera3
LOCAL_SHARED_LIBRARIES += libbase

$(foreach file,$(LOCAL_SRC_FILES),$(shell touch '$(LOCAL_PATH)/$(file)'))

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := camera.$(TARGET_SOC)

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

$(warning #############################################)
$(warning ########       libcamera3       #############)
$(warning #############################################)

include $(call all-makefiles-under, $(LOCAL_PATH))
