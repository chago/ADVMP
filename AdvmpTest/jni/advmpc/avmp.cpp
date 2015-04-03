#include "stdafx.h"
#include "Common.h"
#include "InterpC.h"
#include "Globals.h"
#include "Utils.h"

void nativeLog(JNIEnv* env, jobject thiz) {
    MY_LOG_INFO("nativeLog, thiz=%p", thiz);
}

#ifdef _AVMP_DEBUG_
jint separatorTest(JNIEnv* env, jobject thiz, jint value) {
    MY_LOG_INFO("separatorTest - value=%d", value);
    jvalue result = BWdvmInterpretPortable(gAdvmp.ycFile->GetSeparatorData(0), env, thiz, value);
    return result.i;
}

const char* gClassDesc = "buwai/android/shell/advmptest/MainActivity";

const JNINativeMethod gMethods[] = {
    { "separatorTest", "(I)I", (void*) separatorTest },
    { "nativeLog", "()V", (void*) nativeLog }
};

#else

//+${gClassDesc}
//+${gMethods}

#endif

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;

    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    jclass clazz = env->FindClass(gClassDesc);
    if (!clazz) {
        return JNI_ERR;
    }
    if ( JNI_OK != env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) ) {
        return JNI_ERR;
    }

    // 获得apk路径。
    gAdvmp.apkPath = GetAppPath(env);
    MY_LOG_INFO("apk路径：%s", gAdvmp.apkPath);

    // 释放yc文件。
    gAdvmp.ycSize = ReleaseYcFile(gAdvmp.apkPath, &gAdvmp.ycData);
    if (0 == gAdvmp.ycSize) {
        MY_LOG_WARNING("释放Yc文件失败！");
        goto _ret;
    }

    // 解析yc文件。
    gAdvmp.ycFile = new YcFile;
    if (!gAdvmp.ycFile->parse(gAdvmp.ycData, gAdvmp.ycSize)) {
        MY_LOG_WARNING("解析Yc文件失败。");
        goto _ret;
    }

_ret:
    return JNI_VERSION_1_4;
}
