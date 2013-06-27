LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
include /home/sam/Desktop/git/android/OpenCV-2.4.2-android-sdk/sdk/native/jni/OpenCV.mk
APP_PLATFORM := android-16
LOCAL_MODULE    := native_sample
LOCAL_SRC_FILES := jni_part.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)
