# Copyright 2019 The Android Open Source Project

#$(warning #############################################)
#$(warning ########     libexynoscamera_combine_reprocessing_plugin  )
#$(warning #############################################)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ExynosCameraPlugInCombine.cpp \
    ExynosCameraPlugInCombineReprocessing.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynoscamera_plugin libexynosutils libexynoscamera_fakemultiframe libexynoscamera_fakefusion

LOCAL_MODULE := libexynoscamera_combine_reprocessing_plugin

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/libcutils/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/core/src/common_v2/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libFakeMultiFrame/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libFakeFusion/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libCombine/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

#$(warning ################################################)
#$(warning #### libexynoscamera_combine_preview_plugin ####)
#$(warning ################################################)

#############################
# builtin combine preview lib
ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := true
LOCAL_PREBUILT_LIBS := lib32/libFakeSceneDetect.so
include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_MODULE := libFakeSceneDetect
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both
include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif

################################
# builtin combine preview plugin lib
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    ExynosCameraPlugInCombine.cpp \
    ExynosCameraPlugInCombinePreview.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynoscamera_plugin libexynosutils libexynoscamera_fakemultiframe libexynoscamera_fakefusion libFakeSceneDetect
LOCAL_MODULE := libexynoscamera_combine_preview_plugin

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/libcutils/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/core/src/common_v2/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/ \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libFakeMultiFrame/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libFakeFusion/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/include \
	$(TOP)/vendor/samsung_slsi/exynos/camera/N23/core/src/common_v2/PlugIn/libs/libCombine/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
