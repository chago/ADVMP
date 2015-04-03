#pragma once

#include <jni.h>
#include "YcFile.h"
#include "unzip.h"

// TODO 这里是调试标志。
#define _AVMP_DEBUG_

extern const char* gYcFileName;

typedef struct _ADVMPGlobals {
    char* apkPath;

    /**
     * 保存yc文件的内容。
     */
    unsigned char* ycData;
    /**
     * yc文件大大小。
     */
    uLong ycSize;

    YcFile* ycFile;
    //char* ycFilePath;
} ADVMPGlobals;

extern ADVMPGlobals gAdvmp;
