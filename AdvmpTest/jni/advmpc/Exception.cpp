#include "stdafx.h"
#include "Exception.h"

void dvmThrowNullPointerException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}

void dvmThrowArrayIndexOutOfBoundsException(JNIEnv* env, int length, int index) {
    char* msg = (char*) calloc(100, 1);
    sprintf(msg, "length=%d; index=%d", length, index);
    jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
    free(msg);
}

void dvmThrowArithmeticException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/ArithmeticException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}
