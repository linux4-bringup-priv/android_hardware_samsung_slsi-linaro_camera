# Copyright 2017 The Android Open Source Project

###################################################
# METADATA INTERFACE for 9820
ifneq ($(filter smdk9820 universal9820 maestro9820, $(TARGET_BOOTLOADER_BOARD_NAME)),)
LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS9820_META
endif

# METADATA INTERFACE for 9630
ifneq ($(filter smdk9630 erd9630 maestro9630, $(TARGET_BOOTLOADER_BOARD_NAME)),)
LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS9630_META
endif

# METADATA INTERFACE for 850
ifneq ($(filter smdk850 erd850, $(TARGET_BOOTLOADER_BOARD_NAME)),)
LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS850_META
endif

# METADATA INTERFACE for 3830
ifneq ($(filter erd3830, $(TARGET_BOOTLOADER_BOARD_NAME)),)
LOCAL_CFLAGS += -DUSE_CAMERA_EXYNOS3830_META
endif
###################################################

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_FAKE), true)
LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_FAKE
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libFakeFusion
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libFakeFusion/ExynosCameraPlugInConverterFakeFusion.cpp
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_ARCSOFT), true)
LOCAL_CFLAGS += -DUSES_DUAL_CAMERA_SOLUTION_ARCSOFT
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion

LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion/ExynosCameraPlugInConverterArcsoftFusion.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion/ExynosCameraPlugInConverterArcsoftFusionBokehCapture.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion/ExynosCameraPlugInConverterArcsoftFusionBokehPreview.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion/ExynosCameraPlugInConverterArcsoftFusionZoomCapture.cpp \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libArcsoftFusion/ExynosCameraPlugInConverterArcsoftFusionZoomPreview.cpp
endif

ifeq ($(BOARD_CAMERA_USES_LLS_SOLUTION), true)
LOCAL_CFLAGS += -DUSES_LLS_SOLUTION
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libLLS
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libLLS/ExynosCameraPlugInConverterLowLightShot.cpp
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
LOCAL_CFLAGS += -DUSES_CAMERA_SOLUTION_VDIS
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libVDIS
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libVDIS/ExynosCameraPlugInConverterVDIS.cpp
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_LLS_CAPTURE), true)
LOCAL_CFLAGS += -DUSES_HIFI_LLS_CAPTURE
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libhifills
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libhifills/ExynosCameraPlugInConverterHifills.cpp
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_CAPTURE), true)
LOCAL_CFLAGS += -DUSES_HIFI_CAPTURE
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libNoiseEdge
LOCAL_SRC_FILES += \
	$(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libNoiseEdge/ExynosCameraPlugInConverterHifi.cpp
endif

ifeq ($(BOARD_CAMERA_USES_COMBINE_PLUGIN), true)
LOCAL_CFLAGS += -DUSES_COMBINE_PLUGIN
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libCombine
LOCAL_SRC_FILES += \
    $(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libCombine/ExynosCameraPlugInConverterCombine.cpp \
    $(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libCombine/ExynosCameraPlugInConverterCombineReprocessing.cpp \
    $(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libCombine/ExynosCameraPlugInConverterCombinePreview.cpp
endif

ifeq ($(BOARD_CAMERA_USES_SUPER_RESOLUTION),true)
LOCAL_CFLAGS += -DUSES_SUPER_RESOLUTION
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_VPL), true)
LOCAL_CFLAGS += -DUSES_CAMERA_EXYNOS_VPL
#LOCAL_CFLAGS += -DTEST_INTERFACE
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libVPL
LOCAL_SRC_FILES += \
    $(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libVPL/ExynosCameraPlugInConverterVPL.cpp
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_LEC), true)
LOCAL_CFLAGS += -DUSES_CAMERA_EXYNOS_LEC
LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/converter/libs/libLEC
LOCAL_SRC_FILES += \
    $(CAMERA_SRC_PATH)/core/src/common_v2/PlugIn/converter/libs/libLEC/ExynosCameraPlugInConverterLEC.cpp
endif
