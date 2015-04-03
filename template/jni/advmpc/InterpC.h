#pragma once

#include <jni.h>
#include "YcFile.h"

/**
 * 字节码解释器。
 * @param[in] Separator 数据。
 * @param[in] env JNI环境。
 * @param[in] thiz 当前对象。
 * @param[in] ... 可变参数，传入调用Java方法的参数。
 * @return 
 */
jvalue BWdvmInterpretPortable(const SeparatorData* separatorData, JNIEnv* env, jobject thiz, ...);
