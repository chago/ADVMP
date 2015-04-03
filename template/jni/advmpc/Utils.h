#pragma once

/**
 * 获得APP文件路径。
 * @param[in] env JNI环境。
 * @return 返回APP文件路径。这个路径使用完后需要通过free函数释放内存。
 * @note TODO 这个函数中使用了反射获得APP文件的路径，应该是有兼容性问题的。
 */
char* GetAppPath(JNIEnv* env);
