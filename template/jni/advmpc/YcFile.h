#pragma once

#include <stdio.h>
#include "unzip.h"

#define MAGIC "YC0000"

typedef struct _StringItem {
    /**
     * 字符串中字符个数。
     */
    unsigned int size;

    /**
     * 字符数组。
     */
    unsigned char* str;
} StringItem;

typedef struct _YcHeader {
    /**
     * 魔术字。
     */
    const char* magic;

    /**
     * 文件头长度。
     */
    unsigned int size;

    /**
     * Method结构的个数。
     */
    unsigned int methodSize;

    /**
     * Method数据距离文件起始的偏移。
     */
    unsigned int methodOffset;

    /**
     * SeparatorData结构的个数。
     */
    unsigned int separatorDataSize;

    /**
     * SeparatorData数据距离文件起始的偏移。
     */
    unsigned int separatorDataOffset;
} YcHeader;

typedef struct _AdvmpMethod {

    /**
     * 方法在method_id_item中的索引。
     */
    int methodIndex;

    unsigned int size;

    /**
     * 方法的访问标志。
     */
    unsigned int accessFlag;

    /**
     * 方法所属类。
     * 以"Ljava/lang/System;"这样的格式保存。
     */
    //public String definingClass;

    /**
     * 方法名。
     */
    //public String name;

    /**
     * 方法签名。
     */
    //public String sig;
} AdvmpMethod;

typedef struct _SeparatorData {

    /**
     * 这个索引表示当前SeparatorData结构在
     * SeparatorData结构数组中的索引。
     */
    unsigned int methodIndex;

    /**
     * 当前结构的大小。
     */
    unsigned int size;

    /**
     * 方法的访问标志。
     */
    unsigned int accessFlag;

    /**
     * 参数个数。
     */
    unsigned int paramSize;

    /**
     * 寄存器个数。
     */
    unsigned int registerSize;

    /**
     * 参数的短类型描述。
     */
    StringItem paramShortDesc;

    /**
     * 指令数组元素个数。
     */
    unsigned int instSize;

    /**
     * 抽取出来的方法指令。
     */
    unsigned short* insts;

    /**
     * 方法的try...catch块。
     */
    //public List<TryBlock> tryBlocks;

    /**
     * 方法的debug信息。
     */
    //public List<DebugItem> debugItems;

} SeparatorData;

class YcFormat {
public:
    /**
     * 文件头。
     */
    YcHeader header;

    /**
     * 这是一个指针数组。
     */
    AdvmpMethod** methods;

    /**
     * 抽离器数据。
     * 这是一个指针数组。
     */
    SeparatorData** separatorDatas;

    YcFormat();
    ~YcFormat();
};

//////////////////////////////////////////////////////////////////////////

class YcFile {
public:
    YcFile();
    YcFile(const char* filePath);

    ~YcFile();

    /**
     * 解析Yc文件。
     * @return true：解析成功。false：解析失败。
     */
    //bool parse();

    /**
     * 解析Yc文件。
     * @param[in] ycData yc文件数据。
     * @param[in] dataSize 数据长度。
     * @return true：解析成功。false：解析失败。
     */
    bool parse(unsigned char* ycData, size_t dataSize);

    /**
     * 解析内存中的Yc文件。
     * @param[in] 保存在内存中的Yc文件数据。
     * @return true：解析成功。false：解析失败。
     */
    bool parse(unsigned char* bytes);

    YcFormat mYcFormat;

    /**
     * 获得Separator数据。
     * @param[in] index SeparatorData数组中的索引。
     * @return 返回Separator数据。
     */
    const SeparatorData* GetSeparatorData(int index);

private:
    char* mFilePath;
};

/**
 * 释放Yc文件。
 * @param[in] filePath zip文件路径。
 * @param[out] buffer Yc文件的数据。
 * @return 成功：返回Yc文件的长度。失败。返回0。
 * @note buffer用完后需要用free函数释放内存。
 */
uLong ReleaseYcFile(const char* zipPath, unsigned char** buffer);

/**
 * 打开并解析yc文件。
 * @return true：成功。false：失败。
 */
//bool OpenAndParseYc();
