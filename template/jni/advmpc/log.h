#pragma once

#include <android/log.h>

/**
 * 日志框架。
 * 用途：输入日志。
 */

/*
 用法：
# 定义日志标签。
MY_LOG_TAG := \"hello-jni\"

# 根据构建类型定义默认日志等级。
ifeq ($(APP_OPTIM),release)
MY_LOG_LEVEL := MY_LOG_LEVEL_ERROR
else
MY_LOG_LEVEL := MY_LOG_LEVEL_VERBOSE
endif

# 追加编译标识。
LOCAL_CFLAGS += -DMY_LOG_TAG=$(MY_LOG_TAG)
LOCAL_CFLAGS += -DMY_LOG_LEVEL=$(MY_LOG_LEVEL)

# 动态链接日志库。
LOCAL_LDLIBS += -llog

注意：
如果native_log.h编译进了静态库，那么应该在包含这个静态库的模块中定义宏。
 */

#define MY_LOG_LEVEL_VERBOSE 1
#define MY_LOG_LEVEL_DEBUG 2
#define MY_LOG_LEVEL_INFO 3
#define MY_LOG_LEVEL_WARNING 4
#define MY_LOG_LEVEL_ERROR 5
#define MY_LOG_LEVEL_FATAL 6
#define MY_LOG_LEVEL_SILENT 7

#ifndef MY_LOG_LEVEL
# define MY_LOG_LEVEL MY_LOG_LEVEL_VERBOSE
#endif

#ifndef MY_LOG_TAG
//# define MY_LOG_TAG __FILE__
# define MY_LOG_TAG "debug"
#endif

#define MY_LOG_NOOP (void) 0

// 是否已详细格式输出？
#ifdef MY_LOG_DETAIL
# define MY_LOG_PRINT(level,fmt,...) \
 __android_log_print(level, MY_LOG_TAG, "{%s:%u} %s: [" fmt "]", \
 __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
# else
# define MY_LOG_PRINT(level,fmt,...) \
 __android_log_print(level, MY_LOG_TAG, fmt, ##__VA_ARGS__)
#endif

#if MY_LOG_LEVEL_VERBOSE >= MY_LOG_LEVEL
# define MY_LOG_VERBOSE(fmt,...) \
MY_LOG_PRINT(ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
#else
# define MY_LOG_VERBOSE(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_DEBUG >= MY_LOG_LEVEL
# define MY_LOG_DEBUG(fmt,...) \
MY_LOG_PRINT(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
# define MY_LOG_DEBUG(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_INFO >= MY_LOG_LEVEL
# define MY_LOG_INFO(fmt,...) \
MY_LOG_PRINT(ANDROID_LOG_INFO, fmt, ##__VA_ARGS__)
#else
# define MY_LOG_INFO(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_WARNING >= MY_LOG_LEVEL
# define MY_LOG_WARNING(fmt,...) \
MY_LOG_PRINT(ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)
#else
#   define MY_LOG_WARNING(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_ERROR >= MY_LOG_LEVEL
#  define MY_LOG_ERROR(fmt,...)\
     MY_LOG_PRINT(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
# else
# define MY_LOG_ERROR(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_FATAL >= MY_LOG_LEVEL
# define MY_LOG_FATAL(fmt,...) \
MY_LOG_PRINT(ANDROID_LOG_FATAL, fmt, ##__VA_ARGS__)
#else
# define MY_LOG_FATAL(...) MY_LOG_NOOP
#endif

#if MY_LOG_LEVEL_FATAL >= MY_LOG_LEVEL
# define MY_LOG_ASSERT(expression, fmt, ...) \
if (!(expression)) \
{ \
__android_log_assert(#expression, MY_LOG_TAG, \
fmt, ##__VA_ARGS__); \
}
#else
# define MY_LOG_ASSERT(...) MY_LOG_NOOP
#endif
