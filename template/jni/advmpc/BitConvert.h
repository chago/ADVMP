#pragma once

/**
 * 将字节数组转换为字符串。
 * @param[in] bytes 字节数组。
 * @param[in] size 字节长度。
 * @return 成功：返回字符串。失败：返回NULL。
 * @note 返回指针值需要调用者释放。
 */
char* ToString(unsigned char bytes[], size_t size);

/**
 * 将字节数组转换为无符号整形。
 * 会从bytes的起始位置开始包括后续3个字节的bytes数据转换为unsigned int。
 * 如果bytes的起始位置后续没有3个字节，那么会找到剩余的字节，然后一起转换为unsigned int。
 * @param[in] bytes 字节数组。
 * @param[in] size 字节长度。
 * @return 返回转换好的整形。如果bytes的长度等于0，那么将返回0。
 */
unsigned int ToUInt(unsigned char bytes[], size_t size);

/**
 * 将字节数组转换为无符号整形。
 * 会将start为起始位置包括后续3个字节的bytes数据转换为unsigned int。
 * 如果start后续没有3个字节，那么会找到剩余的字节，然后一起转换为unsigned int。
 * @param[in] bytes 字节数组。
 * @param[in] size 字节长度。
 * @param[in] start 要转换的起始地址。
 * @return 返回转换好的整形。如果转换失败，则返回0。
 *         只有当start大于等于size时，才会转换失败。
 */
unsigned int ToUInt(unsigned char bytes[], size_t size, unsigned int start);
