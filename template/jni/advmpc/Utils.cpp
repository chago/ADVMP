#include "stdafx.h"
#include "Utils.h"

// 获得APP文件路径。
char* GetAppPath(JNIEnv* env) {
    jclass cActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID mActivityThread = env->GetStaticMethodID(cActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject oActivityThread = env->CallStaticObjectMethod(cActivityThread, mActivityThread);

    jfieldID fmBoundApplication = env->GetFieldID(cActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    jobject omBoundApplication = env->GetObjectField(oActivityThread, fmBoundApplication);

    jclass cAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    jfieldID fappInfo = env->GetFieldID(cAppBindData, "appInfo", "Landroid/content/pm/ApplicationInfo;");
    jobject oApplicationInfo = env->GetObjectField(omBoundApplication, fappInfo);

    jclass cApplicationInfo = env->FindClass("android/content/pm/ApplicationInfo");
    jfieldID fsourceDir = env->GetFieldID(cApplicationInfo, "sourceDir", "Ljava/lang/String;");
    jstring sourceDir = (jstring) env->GetObjectField(oApplicationInfo, fsourceDir);

    const char* sourceDir_s = env->GetStringUTFChars(sourceDir, NULL);

    char* cRet = strdup(sourceDir_s);

_ret:
    if (NULL != sourceDir_s) {
        env->ReleaseStringUTFChars(sourceDir, sourceDir_s);
    }
    if (NULL != sourceDir) {
        env->DeleteLocalRef(sourceDir);
    }
    if (NULL != cApplicationInfo) {
        env->DeleteLocalRef(cApplicationInfo);
    }
    if (NULL != cAppBindData) {
        env->DeleteLocalRef(cAppBindData);
    }
    if (NULL != omBoundApplication) {
        env->DeleteLocalRef(omBoundApplication);
    }
    if (NULL != oActivityThread) {
        env->DeleteLocalRef(oActivityThread);
    }
    if (NULL != cActivityThread) {
        env->DeleteLocalRef(cActivityThread);
    }

    return cRet;
}
