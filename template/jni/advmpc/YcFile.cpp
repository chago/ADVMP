#include "stdafx.h"
#include "log.h"
#include "BitConvert.h"
#include "io.h"
#include "Globals.h"
#include "unzip.h"
#include "YcFile.h"


//////////////////////////////////////////////////////////////////////////
// YcFile

YcFile::YcFile() : mFilePath(NULL) {}

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
// bool YcFile::parse() {
//     FileReader fileReader;
//     
//     if (!fileReader.Open(mFilePath)) {
//         MY_LOG_WARNING("打开文件\"%s\"失败。errno:%d-%s.", mFilePath, errno, strerror(errno));
//         return false;
//     }
// 
//     
//     unsigned char* buffer = new unsigned char[6];
//     if (!fileReader.ReadBytes(buffer, 6)) {
//         delete[] buffer;
//         return false;
//     }
// 
//     // 校验魔术字。
//     char* magic = ToString(buffer, 6);
//     if (0 != strcmp(magic, MAGIC)) {
//         MY_LOG_WARNING("魔术字校验失败。");
//         delete[] buffer;
//         free(magic);
//         return false;
//     }
//     free(magic);
//     delete[] buffer;
// 
//     mYcFormat.header.magic = MAGIC;
// 
//     if (!fileReader.ReadUInt(&mYcFormat.header.methodSize)) {
//         MY_LOG_WARNING("读取方法结构个数失败。");
//         return false;
//     }
// 
//     if (!fileReader.ReadUInt(&mYcFormat.header.methodOffset)) {
//         MY_LOG_WARNING("读取方法段偏移失败。");
//         return false;
//     }
// 
//     if (!fileReader.ReadUInt(&mYcFormat.header.separatorDataSize)) {
//         MY_LOG_WARNING("读取抽离数组size失败。");
//         return false;
//     }
// 
//     if (!fileReader.ReadUInt(&mYcFormat.header.separatorDataOffset)) {
//         MY_LOG_WARNING("读取抽离数据段偏移失败。");
//         return false;
//     }
// 
//     // TODO 这里先不解析AdvmpMethod的数据。
//     
//     unsigned int separatorDataSize = mYcFormat.header.separatorDataSize;
//     // 解析抽离数据。
//     if (0 != separatorDataSize) {
//         mYcFormat.separatorDatas = new SeparatorData*[separatorDataSize];
//         if (!fileReader.Seek(mYcFormat.header.separatorDataOffset)) {
//             MY_LOG_WARNING("设置文件指针位置失败。");
//             return false;
//         }
// 
//         for (int i = 0; i < separatorDataSize; i++) {
//             mYcFormat.separatorDatas[i] = (SeparatorData*) calloc(1, sizeof(SeparatorData));
//             if (!fileReader.ReadUInt(&mYcFormat.separatorDatas[i]->methodIndex)) {
//                 MY_LOG_WARNING("%d. 读取SeparatorData中方法索引失败。", i);
//                 return false;
//             }
//             if (!fileReader.ReadUInt(&mYcFormat.separatorDatas[i]->instSize)) {
//                 MY_LOG_WARNING("%d. 读取SeparatorData中方法指令size失败。", i);
//                 return false;
//             }
//             mYcFormat.separatorDatas[i]->insts = new unsigned short[mYcFormat.separatorDatas[i]->instSize];
//             if (!fileReader.ReadUShorts(mYcFormat.separatorDatas[i]->insts, mYcFormat.separatorDatas[i]->instSize)) {
//                 MY_LOG_WARNING("%d. 读取SeparatorData中方法指令size失败。", i);
//                 return false;
//             }
//         }
//     }
// 
//     return true;
// }

// 解析Yc文件。
bool YcFile::parse(unsigned char* ycData, size_t dataSize) {
    unsigned char* p = ycData;
    unsigned char* end = ycData + dataSize - 1;

    // 校验魔术字。
    char* magic = ToString(p, 6);
    if (0 != strcmp(magic, MAGIC)) {
        MY_LOG_WARNING("invalid magic");
        free(magic);
        return false;
    }
    free(magic);

    p += 6;

    mYcFormat.header.magic = MAGIC;

    mYcFormat.header.size = ToUInt(p, dataSize);
    MY_LOG_INFO("header.size:%u", mYcFormat.header.size);
    p += sizeof(mYcFormat.header.size);

    mYcFormat.header.methodSize = ToUInt(p, dataSize);
    MY_LOG_INFO("header.methodSize:%u", mYcFormat.header.methodSize);
    p += sizeof(mYcFormat.header.methodSize);

    mYcFormat.header.methodOffset = ToUInt(p, dataSize);
    MY_LOG_INFO("header.methodOffset:%u", mYcFormat.header.methodOffset);
    p += sizeof(mYcFormat.header.methodOffset);

    mYcFormat.header.separatorDataSize = ToUInt(p, dataSize);
    MY_LOG_INFO("header.separatorDataSize:%u", mYcFormat.header.separatorDataSize);
    p += sizeof(mYcFormat.header.separatorDataSize);

    mYcFormat.header.separatorDataOffset = ToUInt(p, dataSize);
    MY_LOG_INFO("header.separatorDataOffset:%u", mYcFormat.header.separatorDataOffset);
    p += sizeof(mYcFormat.header.separatorDataOffset);

    // TODO 这里先不解析AdvmpMethod的数据。
    unsigned int methodSize = mYcFormat.header.methodSize;
    if (0 != methodSize) {
        mYcFormat.methods = new AdvmpMethod*[methodSize];
        p = ycData + mYcFormat.header.methodOffset;
    }

    unsigned int separatorDataSize = mYcFormat.header.separatorDataSize;
    // 解析抽离数据。
    if (0 != separatorDataSize) {
        mYcFormat.separatorDatas = new SeparatorData*[separatorDataSize];
        p = ycData + mYcFormat.header.separatorDataOffset;

        for (int i = 0; i < separatorDataSize; i++) {
            mYcFormat.separatorDatas[i] = (SeparatorData*) calloc(1, sizeof(SeparatorData));
            
            mYcFormat.separatorDatas[i]->methodIndex = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->methodIndex);
            MY_LOG_INFO("%d. methodIndex:%u", i, mYcFormat.separatorDatas[i]->methodIndex);

            mYcFormat.separatorDatas[i]->size = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->size);
            MY_LOG_INFO("%d. size:%u", i, mYcFormat.separatorDatas[i]->size);

            mYcFormat.separatorDatas[i]->accessFlag = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->accessFlag);
            MY_LOG_INFO("%d. accessFlag:%u", i, mYcFormat.separatorDatas[i]->accessFlag);

            mYcFormat.separatorDatas[i]->paramSize = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->paramSize);
            MY_LOG_INFO("%d. paramSize:%u", i, mYcFormat.separatorDatas[i]->paramSize);

            mYcFormat.separatorDatas[i]->registerSize = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->registerSize);
            MY_LOG_INFO("%d. registerSize:%u", i, mYcFormat.separatorDatas[i]->registerSize);

            mYcFormat.separatorDatas[i]->paramShortDesc.size = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->paramShortDesc.size);
            MY_LOG_INFO("%d. paramShortDesc.size:%u", i, mYcFormat.separatorDatas[i]->paramShortDesc.size);

            mYcFormat.separatorDatas[i]->paramShortDesc.str = (unsigned char*) calloc(1, mYcFormat.separatorDatas[i]->paramShortDesc.size);
            memcpy(mYcFormat.separatorDatas[i]->paramShortDesc.str, p, mYcFormat.separatorDatas[i]->paramShortDesc.size);
            p += mYcFormat.separatorDatas[i]->paramShortDesc.size;

            mYcFormat.separatorDatas[i]->instSize = ToUInt(p, dataSize);
            p += sizeof(mYcFormat.separatorDatas[i]->instSize);
            MY_LOG_INFO("%d. instSize:%u", i, mYcFormat.separatorDatas[i]->instSize);

            size_t instByteSize = mYcFormat.separatorDatas[i]->instSize * sizeof(unsigned short);
            MY_LOG_INFO("instByteSize:%zd", instByteSize);
            mYcFormat.separatorDatas[i]->insts = new unsigned short[mYcFormat.separatorDatas[i]->instSize];
            memcpy(mYcFormat.separatorDatas[i]->insts, p, instByteSize);
            p += instByteSize;
        }
    }

    MY_LOG_INFO("end:%p, p:%p", end, p);

    return true;
}

// 获得Separator数据
const SeparatorData* YcFile::GetSeparatorData(int index) {
    return mYcFormat.separatorDatas[index];
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
// bool OpenAndParseYc(JNIEnv* env) {
//     YcFile* ycFile = new YcFile(gAdvmp.ycFilePath);
//     if (ycFile->parse()) {
//         gAdvmp.ycFile = ycFile;
//     } else {
//         MY_LOG_ERROR("打开&解析yc文件失败！");
//         return false;
//     }
// }

//////////////////////////////////////////////////////////////////////////

// 释放Yc文件。
uLong ReleaseYcFile(const char* zipPath, unsigned char** buffer) {
    MY_LOG_INFO("zip file:%s", zipPath);
    ZipReader zipReader(zipPath);
    if (!zipReader.Open()) {
        MY_LOG_WARNING("open zip file fail%s", zipPath);
        return false;
    }

    char* filePathInZip = (char*) calloc(strlen(gYcFileName) + strlen("assets") + 1, sizeof(char));
    sprintf(filePathInZip, "assets/%s", gYcFileName);
    MY_LOG_INFO("filePathInZip=%s", filePathInZip);
    uLong fileSize = zipReader.GetFileSizeInZip(filePathInZip);

    uLong bRet = 0;
    if (0 == fileSize) {
        MY_LOG_WARNING("yc file size is 0.");
        goto _ret;
    }

    MY_LOG_INFO("yc file size:%lu", fileSize);
    
    *buffer = (unsigned char*) calloc(sizeof(unsigned char), fileSize);
    if (!zipReader.ReadBytes(filePathInZip, *buffer, fileSize)) {
        MY_LOG_WARNING("read yc file fail.");
        goto _ret;
    }

    bRet = fileSize;

_ret:
    if (NULL != filePathInZip) {
        free(filePathInZip);
    }
    return bRet;
}
