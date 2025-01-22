# Copyright 2017 The Android Open Source Project

#$(warning #############################################)
#$(warning ########     HiFiLLS PlugIn     #############)
#$(warning #############################################)

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := true
LOCAL_PREBUILT_LIBS := lib32/libhifills.so

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)

else
include $(CLEAR_VARS)

LOCAL_MODULE := libhifills
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif

ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := true
LOCAL_PREBUILT_LIBS := lib32/libENF.so

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)

else
include $(CLEAR_VARS)

LOCAL_MODULE := libENF
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ExynosCameraPlugInHiFiLLS.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynoscamera_plugin libexynosutils libhifills libENF

LOCAL_MODULE := libexynoscamera_hifills_plugin

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/libcutils/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/core/src/common_v2/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libHiFiLLS/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
