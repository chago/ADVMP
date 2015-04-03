#include "stdafx.h"
#include "InterpC.h"
#include "Globals.h"

#ifdef _AVMP_DEBUG_
jint separatorTest(jint) {
    return -1;
}

const char* gClassDesc = "com/example/androidhelloworld/MainActivity";

const JNINativeMethod gMethods[] = {
    { "separatorTest", "(I)I", (void*) separatorTest },
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
    return JNI_VERSION_1_4;
}
