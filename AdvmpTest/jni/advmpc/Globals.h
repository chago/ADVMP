#pragma once

#include <jni.h>
#include "YcFile.h"

// TODO 这里是调试标志。
#define _AVMP_DEBUG_

extern const char* gYcFileName;

typedef struct _ADVMPGlobals {
    YcFile* ycFile;
    char* ycFilePath;
} ADVMPGlobals;

extern ADVMPGlobals gAdvmp;
