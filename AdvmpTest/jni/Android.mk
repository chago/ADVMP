ADVMPC_PATH := ../../template/jni/advmpc

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := advmp

LOCAL_C_INCLUDES := $(ADVMPC_PATH)

LOCAL_SRC_FILES := $(ADVMPC_PATH)/ioapi.c \
				   $(ADVMPC_PATH)/unzip.c \
				   $(ADVMPC_PATH)/Globals.cpp \
				   $(ADVMPC_PATH)/avmp.cpp \
				   $(ADVMPC_PATH)/BitConvert.cpp \
				   $(ADVMPC_PATH)/InterpC.cpp \
				   $(ADVMPC_PATH)/io.cpp \
				   $(ADVMPC_PATH)/Utils.cpp \
				   $(ADVMPC_PATH)/YcFile.cpp 

LOCAL_SRC_FILES += $(ADVMPC_PATH)/DexOpcodes.cpp \
				   $(ADVMPC_PATH)/Exception.cpp

LOCAL_LDLIBS := -llog -lz

include $(BUILD_SHARED_LIBRARY)
