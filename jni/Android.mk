LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := my_game
LOCAL_LDLIBS := -ldl -lm
LOCAL_CFLAGS += -D __ANDROID__ -I ../third_party/include/ -Wall ../src/**/*.c -std=gnu99 -w -lGL -lX11 -pthread -lXi -g -O0
# LOCAL_SRC_FILES := ../src/**/*.c
include $(BUILD_SHARED_LIBRARY)
