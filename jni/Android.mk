LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := my_game
LOCAL_LDLIBS += -llog -lEGL -lGLESv3 -landroid -lOpenSLES -lnativewindow
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../third_party/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/ccd/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/cgltf/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/dr_libs/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/glad/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/glfw/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/KHR/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/miniaudio/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/sg_noise/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party/include/gs/external/stb/
LOCAL_CFLAGS += -D__ANDROID__ -std=gnu99 -w -ldl -lm -lGL -lX11 -lXi -pthread
LOCAL_SRC_FILES := $(LOCAL_PATH)/../src/main.c
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/audio/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/bsp/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/entities/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/game/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/graphics/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/shaders/*.c)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../src/util/*.c)
include $(BUILD_SHARED_LIBRARY)
