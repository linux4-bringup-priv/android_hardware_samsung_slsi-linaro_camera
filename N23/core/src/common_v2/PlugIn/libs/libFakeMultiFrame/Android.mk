# Copyright 2019 The Android Open Source Project

#$(warning #############################################)
#$(warning ########     libexynoscamera_fakemultiframe  )
#$(warning #############################################)

LOCAL_PATH := $(call my-dir)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := src/FakeMultiFrame.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynosutils

LOCAL_MODULE := libexynoscamera_fakemultiframe

LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(BUILD_SHARED_LIBRARY)
