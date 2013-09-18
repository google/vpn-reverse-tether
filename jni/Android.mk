LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS    := -std=c99 -O3
LOCAL_MODULE    := forwarder
LOCAL_SRC_FILES := forwarder.c forwarder_jni.c

include $(BUILD_SHARED_LIBRARY)
