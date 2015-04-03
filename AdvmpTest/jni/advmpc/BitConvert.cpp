#include "stdafx.h"
#include "BitConvert.h"

// 将字节数组转换为字符串。
char* ToString(unsigned char bytes[], size_t size) {
    char* str = (char*) calloc (size + 1, sizeof(char));
    if (NULL == str) {
        return NULL;
    }
    for (int i = 0; i < size; i++) {
        str[i] = (char)(bytes[i]);
    }
    return str;
}
