LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := advmp

LOCAL_SRC_FILES := ioapi.c \
				   unzip.c \
				   Globals.cpp \
				   avmp.cpp \
				   BitConvert.cpp \
				   InterpC.cpp \
				   io.cpp \
				   Utils.cpp \
				   YcFile.cpp 

LOCAL_SRC_FILES += DexOpcodes.cpp \
				   Exception.cpp

LOCAL_LDLIBS := -llog -lz

include $(BUILD_SHARED_LIBRARY)
