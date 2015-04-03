#include "stdafx.h"
#include "BitConvert.h"
#include "io.h"
#include "Globals.h"
#include "unzip.h"
#include "YcFile.h"

YcFile::YcFile(const char* filePath)
    : mFilePath(NULL)
{
    mFilePath = strdup(filePath);
}

YcFile::~YcFile() {
    if (NULL != mFilePath) {
        free(mFilePath);
    }
}

// 解析Yc文件。
bool YcFile::parse() {
    FileReader fileReader;
    
    if (!fileReader.Open(mFilePath)) {
        MY_LOG_WARNING("打开文件\"%s\"失败。errno:%d-%s.", mFilePath, errno, strerror(errno));
        return false;
    }

    
    unsigned char* buffer = new unsigned char[6];
    if (!fileReader.ReadBytes(buffer, 6)) {
        delete[] buffer;
        return false;
    }

    // 校验魔术字。
    char* magic = ToString(buffer, 6);
    if (0 != strcmp(magic, MAGIC)) {
        MY_LOG_WARNING("魔术字校验失败。");
        delete[] buffer;
        free(magic);
        return false;
    }
    free(magic);
    delete[] buffer;

    mYcFormat.header.magic = MAGIC;

    if (!fileReader.ReadUInt(&mYcFormat.header.methodSize)) {
        MY_LOG_WARNING("读取方法结构个数失败。");
        return false;
    }

    if (!fileReader.ReadUInt(&mYcFormat.header.methodOffset)) {
        MY_LOG_WARNING("读取方法段偏移失败。");
        return false;
    }

    if (!fileReader.ReadUInt(&mYcFormat.header.separatorDataSize)) {
        MY_LOG_WARNING("读取抽离数组size失败。");
        return false;
    }

    if (!fileReader.ReadUInt(&mYcFormat.header.separatorDataOffset)) {
        MY_LOG_WARNING("读取抽离数据段偏移失败。");
        return false;
    }

    // TODO 这里先不解析AdvmpMethod的数据。
    
    unsigned int separatorDataSize = mYcFormat.header.separatorDataSize;
    // 解析抽离数据。
    if (0 != separatorDataSize) {
        mYcFormat.separatorDatas = new SeparatorData*[separatorDataSize];
        if (!fileReader.Seek(mYcFormat.header.separatorDataOffset)) {
            MY_LOG_WARNING("设置文件指针位置失败。");
            return false;
        }

        for (int i = 0; i < separatorDataSize; i++) {
            mYcFormat.separatorDatas[i] = (SeparatorData*) calloc(1, sizeof(SeparatorData));
            if (!fileReader.ReadUInt(&mYcFormat.separatorDatas[i]->methodIndex)) {
                MY_LOG_WARNING("%d. 读取SeparatorData中方法索引失败。", i);
                return false;
            }
            if (!fileReader.ReadUInt(&mYcFormat.separatorDatas[i]->instSize)) {
                MY_LOG_WARNING("%d. 读取SeparatorData中方法指令size失败。", i);
                return false;
            }
            mYcFormat.separatorDatas[i]->insts = new unsigned short[mYcFormat.separatorDatas[i]->instSize];
            if (!fileReader.ReadUShorts(mYcFormat.separatorDatas[i]->insts, mYcFormat.separatorDatas[i]->instSize)) {
                MY_LOG_WARNING("%d. 读取SeparatorData中方法指令size失败。", i);
                return false;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// 

YcFormat::YcFormat()
    : methods(NULL), separatorDatas(NULL)
{
    memset(&header, 0, sizeof(YcHeader));
}

YcFormat::~YcFormat() {
    if (NULL != methods) {
        unsigned int size = header.methodSize;
        for (int i = 0; i < size; i++) {
            free(methods[i]);
        }
        delete[] methods;
    }
    if (NULL != separatorDatas) {
        unsigned int size = header.separatorDataSize;
        for (int i = 0; i < size; i++) {
            if (NULL != separatorDatas[i]->insts) {
                delete[] separatorDatas[i]->insts;
            }
            free(separatorDatas[i]);
        }
        delete[] separatorDatas;
    }
}

//////////////////////////////////////////////////////////////////////////

// 打开并解析yc文件。
bool OpenAndParseYc(JNIEnv* env) {
    YcFile* ycFile = new YcFile(gAdvmp.ycFilePath);
    if (ycFile->parse()) {
        gAdvmp.ycFile = ycFile;
    } else {
        MY_LOG_ERROR("打开&解析yc文件失败！");
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////

// 释放Yc文件。
uLong ReleaseYcFile(const char* filePath, unsigned char* buffer) {
    MY_LOG_INFO("zip文件：%s", filePath);
    ZipReader zipReader(filePath);
    if (!zipReader.Open()) {
        MY_LOG_WARNING("打开zip文件失败：%s", filePath);
        return false;
    }

    char* filePathInZip = (char*) calloc(strlen(gYcFileName) + strlen("assets") + 1, sizeof(char));
    uLong fileSize = zipReader.GetFileSizeInZip(filePathInZip);

    uLong bRet = 0;
    if (0 == fileSize) {
        goto _ret;
    }
    
    buffer = (unsigned char*) calloc(sizeof(unsigned char), fileSize);
    if (!zipReader.ReadBytes(filePathInZip, buffer, fileSize)) {
        goto _ret;
    }

    bRet = fileSize;

_ret:
    if (NULL != filePathInZip) {
        free(filePathInZip);
    }
    return bRet;
}
