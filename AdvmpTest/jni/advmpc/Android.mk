LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

DEVFLAG := _static

LOCAL_MODULE    := advmpc

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../minizip

LOCAL_SRC_FILES := ioapi.c \
				   unzip.c \
				   Globals.cpp \
				   avmp.cpp \
				   BitConvert.cpp \
				   InterpC.cpp \
				   io.cpp \
				   Utils.cpp \
				   YcFile.cpp \

LOCAL_LDLIBS := -llog -lz

include $(BUILD_SHARED_LIBRARY)
