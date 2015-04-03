#pragma once

/**
 * 获得APP文件路径。
 * @param[in] env JNI环境。
 * @return 返回APP文件路径。这个路径使用完后需要通过free函数释放内存。
 */
char* GetAppPath(JNIEnv* env);
