#pragma once

#include "unzip.h"

class FileReader {
public:
    FileReader();
    ~FileReader();

    /**
     * 打开文件。
     * @param[in] filePath 文件路径。
     */
    bool Open(const char* filePath);

    /**
     * 移动文件指针到指定偏移位置。
     * @param[in] offset 距离文件起始位置的偏移。
     * @return true：成功。false：失败。
     */
    bool Seek(unsigned int offset);

    /**
     * 关闭打开的文件。
     */
    void Close();

    /**
     * 读取一个无符号short值。
     * @param[out] value 读取成功则返回读取到的值。
     * @return true：读取成功。false：读取失败。
     */
    bool ReadUShort(unsigned short* value);

    /**
     * 读取一个无符号整型值。
     * @param[out] value 读取成功则返回读取到的值。
     * @return true：读取成功。false：读取失败。
     */
    bool ReadUInt(unsigned int* value);

    /**
     * 读取字节数组。
     * @param[out] buffer 如果成功，则返回的字节数组。
     * @param[in] count 读取的字节数。
     * @return true：读取成功。false：读取失败。
     */
    bool ReadBytes(unsigned char* buffer, int count);

    /**
     * 读取无符号short数组。
     * @param[out] buffer 读取成功，则返回一个short数组。
     * @param[in] size 数组中元素个数。
     * @return true：读取成功。false：读取失败。
     */
    bool ReadUShorts(unsigned short* buffer, int size);

private:
    char* mFilePath;
    FILE* mFP;
};

//////////////////////////////////////////////////////////////////////////
// ZipReader

class ZipReader {
public:
    ZipReader(const char* zipFilePath);
    ~ZipReader();

    /**
     * 打开zip文件。
     */
    bool Open();

    /**
     * 获得zip中某个文件大小。
     * @param[in] fileName 在zip中的文件路径。
     * @return 成功，返回文件大小。失败，返回0。
     */
    uLong GetFileSizeInZip(const char *fileName);

    /**
     * 读取某个文件。
     * @param[in] fileName 在zip中的文件路径。
     * @param[out] 读取到的数据。
     * @param[in] 要读取的数据长度。
     * @return true：成功。false：失败。
     */
    bool ReadBytes(const char *fileName, unsigned char *buffer, size_t len);

    /**
     * 关闭文件。
     */
    bool Close();

private:
    char* mZipFilePath;
    unzFile mUnzFile;
};
