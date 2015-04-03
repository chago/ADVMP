#pragma once

/**
 * 将字节数组转换为字符串。
 * @return 成功：返回字符串。失败：返回NULL。
 * @note 返回指针值需要调用者释放。
 */
char* ToString(unsigned char bytes[], size_t size);
