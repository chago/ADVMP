#include "stdafx.h"
#include "Common.h"
#include "InterpC.h"
#include "Globals.h"
#include "Utils.h"

#ifdef _AVMP_DEBUG_

void nativeLog(JNIEnv* env, jobject thiz) {
    MY_LOG_INFO("nativeLog, thiz=%p", thiz);
}

jint separatorTest(JNIEnv* env, jobject thiz, jint value) {
    MY_LOG_INFO("separatorTest - value=%d", value);
    jvalue result = BWdvmInterpretPortable(gAdvmp.ycFile->GetSeparatorData(0), env, thiz, value);
    return result.i;
}

// TODO 现在只支持一个类，但是其实可以支持多个类。

/**
 * 注册本地方法。
 */
bool registerNatives(JNIEnv* env) {
    const char* classDesc = "buwai/android/shell/advmptest/MainActivity";
    const JNINativeMethod methods[] = {
        { "separatorTest", "(I)I", (void*) separatorTest },
        { "nativeLog", "()V", (void*) nativeLog }
    };

    jclass clazz = env->FindClass(classDesc);
    if (!clazz) {
        MY_LOG_ERROR("未找到类：%s！", classDesc);
        return false;
    }

    bool bRet = false;
    if ( JNI_OK == env->RegisterNatives(clazz, methods, array_size(methods)) ) {
        bRet = true;
    } else {
        MY_LOG_ERROR("注册类\"%s\"的本地方法失败！", classDesc);
    }
    env->DeleteLocalRef(clazz);
    return bRet;
}

void registerFunctions(JNIEnv* env) {
    if (!registerNatives(env)) {
        MY_LOG_ERROR("注册本地方法失败。");
        return;
    }
}

#else

//+${replaceAll}

#endif

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;

    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    // 注册本地方法。
    registerFunctions(env);

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
