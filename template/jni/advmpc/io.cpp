#include "stdafx.h"
#include "io.h"

//////////////////////////////////////////////////////////////////////////
// 

FileReader::FileReader() : mFilePath(NULL), mFP(NULL) {}

FileReader::~FileReader() { Close(); }

// 打开文件。
bool FileReader::Open(const char* filePath) {
    mFilePath = strdup(filePath);
    mFP = fopen(filePath, "r");
}

// 移动文件指针到指定偏移位置。
bool FileReader::Seek(unsigned int offset) {
    if (0 == fseek(mFP, offset, SEEK_SET)) {
        return true;
    } else {
        return false;
    }
}

// 关闭打开的文件。
void FileReader::Close() {
    if (NULL != mFilePath) {
        free(mFilePath);
        mFilePath = NULL;
    }
    if (NULL != mFP) {
        fclose(mFP);
        mFP = NULL;
    }
}

// 读取一个无符号short值。
bool FileReader::ReadUShort(unsigned short* value) {
    size_t ret = fread(value, sizeof(unsigned short), 1, mFP);
    if (ret != sizeof(unsigned short)) {
        return false;
    }
    return true;
}

// 读取一个无符号整型值。
bool FileReader::ReadUInt(unsigned int* value) {
    size_t ret = fread(value, sizeof(unsigned int), 1, mFP);
    if (ret != sizeof(unsigned int)) {
        return false;
    }
    return true;
}

// 读取字节数组。
bool FileReader::ReadBytes(unsigned char* buffer, int count) {
    size_t ret = fread(buffer, sizeof(unsigned char), count, mFP);
    if (ret != (sizeof(unsigned char) * count)) {
        return false;
    }
    return true;
}

// 读取无符号short数组。
bool FileReader::ReadUShorts(unsigned short* buffer, int size) {
    for (int i = 0; i < size; i++) {
        unsigned short value;
        if (!ReadUShort(&value)) {
            return false;
        }
        buffer[i] = value;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// ZipReader

ZipReader::ZipReader(const char* zipFilePath) : mUnzFile(NULL) {
    mZipFilePath = strdup(zipFilePath);
}

ZipReader::~ZipReader() {
    if (NULL != mZipFilePath) {
        free(mZipFilePath);
    }
    Close();
}

// 打开zip文件。
bool ZipReader::Open() {
    mUnzFile = unzOpen(mZipFilePath);
    if (NULL == mUnzFile) {
        return false;
    } else {
        return true;
    }
}

// 获得zip中某个文件大小。
uLong ZipReader::GetFileSizeInZip(const char *fileName) {
    if (UNZ_OK != unzLocateFile(mUnzFile, fileName, false)) {
        MY_LOG_WARNING("未找到文件：%s", fileName);
        return 0;
    }
    unz_file_info info = { 0 };

    if (UNZ_OK != unzGetCurrentFileInfo(mUnzFile, &info, NULL, 0, NULL, 0, NULL, 0)) {
        MY_LOG_WARNING("获得文件信息失败。");
        return 0;
    }
    return info.uncompressed_size;
}

// 读取某个文件。
bool ZipReader::ReadBytes(const char *fileName, unsigned char *buffer, size_t len) {
    if (UNZ_OK != unzLocateFile(mUnzFile, fileName, false)) {
        MY_LOG_WARNING("定位文件失败。");
        return false;
    }
    unz_file_info info = { 0 };

    if (UNZ_OK != unzGetCurrentFileInfo(mUnzFile, &info, NULL, 0, NULL, 0, NULL, 0)) {
        MY_LOG_WARNING("获得文件信息失败。");
        return false;
    }

    if (len > info.uncompressed_size) {
        MY_LOG_WARNING("要读取的长度超过文件的长度。");
        return false;
    }

    // 
    if (UNZ_OK != unzOpenCurrentFile(mUnzFile)) {
        MY_LOG_WARNING("打开当前文件失败。");
        return false;
    }

    int result;
    if ( ((result = unzReadCurrentFile(mUnzFile, buffer, len)) < 0 ) || (result != len) ) {
        MY_LOG_WARNING("读取当前文件失败。result=%d.", result);
        return false;
    } else {
        return true;
    }
}

// 关闭文件。
bool ZipReader::Close() {
    if (NULL == mUnzFile) {
        return true;
    }
    if (UNZ_OK == unzCloseCurrentFile(mUnzFile)) {
        return true;
    }
    return false;
}
