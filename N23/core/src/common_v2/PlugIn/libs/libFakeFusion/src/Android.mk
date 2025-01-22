# Copyright 2017 The Android Open Source Project

ifeq ($(BOARD_CAMERA_BUILD_SOLUTION_SRC), true)
LOCAL_PATH := $(call my-dir)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := FakeFusion.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynosutils

LOCAL_MODULE := libexynoscamera_fakefusion

LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(LOCAL_PATH)/../include \

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
endif
