#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <bitset>
#include <vector>
#include <queue>
using namespace std;
//nasm的方法
extern "C"
{
    void my_printRed(char *str, int length);   //打印
    void my_printWhite(char *str, int length); //打印
}

typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

#pragma pack(1) /*指定按1字节对齐*/
class FileAndDir
{
public:
    char name[13];  //文件或目录的名字
    char isDir;     //是否是目录  '1'-> 是,  '0'  -> 不是
    char path[256]; // 根目录下的parentDir="/"
    int fileSize;   //文件的大小，如果是目录的话，值为0
    int startClus;  //文件或目录的起始簇
    int directFile; //文件直接子文件数量
    int direcDir;   //文件直接子目录数量
    vector<FileAndDir *> subFDs;
    ~FileAndDir()
    {
        cout << "delete" << name << endl;
        for (vector<FileAndDir *>::iterator it = subFDs.begin(); it != subFDs.end(); ++it)
        {
            FileAndDir *fd = *it;
            if (fd)
            {
                delete fd;
                fd = 0;
            }
        }
        subFDs.clear();
    }
    // 寻找当前目录下名为target的FD，找不到就返回NULL
    FileAndDir *locate(char *target)
    {
        for (vector<FileAndDir *>::iterator it = subFDs.begin(); it != subFDs.end(); ++it)
        {
            FileAndDir *fd = *it;
            if (strcmp(target, fd->name) == 0)
            {
                return fd;
            }
        }
        return NULL;
    }
    void printPathDetail()
    {
        char temp[64];
        sprintf(temp, "%s %d %d:\n", this->path, this->directFile, this->direcDir);
        my_printWhite(temp, strlen(temp));
    }
    // 打印-l选项的行
    void printNameDetail()
    {
        char temp[64];
        if (this->isDir == '1')
        {
            my_printRed(this->name, strlen(this->name));
            sprintf(temp, " %d %d\n", this->directFile, this->direcDir);
            my_printWhite(temp, strlen(temp));
        }
        else
        {
            sprintf(temp, "%s %d\n", this->name, this->fileSize);
            my_printWhite(temp, strlen(temp));
        }
    }
    void LS_L()
    {
        queue<FileAndDir *> dirs; // 目录队列，用于深度优先打印
        this->printPathDetail();
        // 第一遍遍历，输出当前层文件
        if (strcmp(this->path, "/") != 0)
        {
            // 非根目录，打印. 和.. ,直接写死
            my_printRed(".\n..\n", 5);
        }
        for (vector<FileAndDir *>::iterator it = subFDs.begin(); it != subFDs.end(); ++it)
        {
            FileAndDir *fd = *it;
            if (fd->isDir == '1')
            {
                dirs.push(fd);
            }
            fd->printNameDetail();
        }
        my_printWhite("\n", 1);
        while (!dirs.empty())
        {
            dirs.front()->LS_L();
            dirs.pop();
        }
    }
    void LS()
    {
        queue<FileAndDir *> dirs; // 目录队列，用于深度优先打印
        my_printWhite(this->path, strlen(this->path));
        my_printWhite(":\n", 2);
        // 第一遍遍历，输出当前层文件
        if (strcmp(this->path, "/") != 0)
        {
            // 非根目录，打印. 和.. ,直接写死
            my_printRed(".  ..  ", 7);
        }
        for (vector<FileAndDir *>::iterator it = subFDs.begin(); it != subFDs.end(); ++it)
        {
            FileAndDir *fd = *it;
            if (fd->isDir == '1')
            {
                dirs.push(fd);
                my_printRed((*it)->name, strlen((*it)->name));
            }
            else
            {
                my_printWhite((*it)->name, strlen((*it)->name));
            }
            my_printWhite("  ", 2);
        }
        my_printWhite("\n", 1);
        while (!dirs.empty())
        {
            dirs.front()->LS();
            dirs.pop();
        }
    }
    void CAT(FILE *fat12);
};

//偏移11个字节，前11个字节不看
struct BPB
{
    u16 BPB_BytsPerSec; //每扇区字节数
    u8 BPB_SecPerClus;  //每簇扇区数
    u16 BPB_RsvdSecCnt; //Boot记录占用的扇区数
    u8 BPB_NumFATs;     //FAT表个数
    u16 BPB_RootEntCnt; //根目录最大文件数
    u16 BPB_TotSec16;   //总扇区数
    u8 BPB_Media;
    u16 BPB_FATSz16; //FAT占用的扇区数
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32; //如果BPB_TotSec16为0，该值为总扇区数
};
//BPB至此结束，长度25字节

//根目录条目
struct RootEntry
{
    char DIR_Name[11];
    u8 DIR_Attr; //文件属性
    char reserved[10];
    u16 DIR_WrtTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClus; //开始簇号
    u32 DIR_FileSize;
};
//根目录条目结束，32字节

#pragma pack() /*取消指定对齐，恢复缺省对齐*/

// 以下是将文件系统中的文件加载入变量的方法
void readBPB(FILE *);         // 读取BPB配置
void readRootEntries(FILE *); // 读取根目录下文件，递归创建文件树
void readChildEntries(FILE *fat12, FileAndDir *parent, int startClus);
void generateFD(FILE *fat12, FileAndDir *parent, int offset); // 递归体方法
void loadFile(FILE *src, int offset, int length, void *dest); // 将文件加载到数据结构中
int readFAT(FILE *fat12, int numOfClus);
void readFileName(char *src, char *dest, int isDir);
// 以下是控制方法
FileAndDir *locateFD(FileAndDir *currentDir, char *filePath);
void doCAT(FILE *fat12, char *input);
void doLL(char *input);
vector<string> split(const string &s, const string &seperator);
// 以下是核心全局变量
struct BPB *bpb_ptr;
FileAndDir *root_ptr; // 所有的文件or文件夹
int RootDirSectors;   // 根目录所占的扇区数
int DataBaseSectors;  // 数据区起始扇区
int main()
{

    char* s = "/DIR1";
    vector<string> v = split(s, "/"); //可按多个字符来分隔;
    for (vector<string>::size_type i = 0; i != v.size(); ++i)
        cout << v[i] << " ";
    cout << endl;

    FILE *fat12 = fopen("./fat12/a.img", "rb");
    readBPB(fat12);
    readRootEntries(fat12);
    char *input = (char *)malloc(sizeof(char) * 128);
    while (true)
    {
        my_printWhite(">", 1);
        cin.getline(input, 128);
        if (strcmp(input, "exit") == 0)
        {
            break;
        }
        else if (input[0] == 'l' && input[1] == 's')
        {
            doLL(input);
        }
        else if (input[0] == 'c' && input[1] == 'a' && input[2] == 't')
        {
            doCAT(fat12, input);
        }
        else
        {
            char *errorMsg = "Error: Invalid Input. Command must be started with 'ls'、'cat' or 'exit'.\n";
            my_printWhite(errorMsg, strlen(errorMsg));
        }
    }
    // FileAndDir *fd = locateFD(root_ptr, "RIVER.TXT");
    // fd->CAT(fat12);
    return 0;
}

void doCAT(FILE *fat12, char *input)
{
    char *s = (char *)malloc(sizeof(char) * 128);
    strcpy(s, input);
    char *delim = " ";
    char *target = strtok(s, delim);
    int get_cat = 0;
    char *outer_ptr = NULL;
    FileAndDir *fd;
    do
    {
        if (strcmp("cat", target) == 0)
        {
            if (get_cat == 1)
            {
                char *errorMsg = "Syntax Error: multiple 'cat' command.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            get_cat = 1;
        }
        else
        {
            if (!fd)
            {
                char *errorMsg = "Syntax Error: redundancy params for file path.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            fd = locateFD(root_ptr, target);
            if (fd == NULL)
            {
                char *errorMsg = "Error: file or directory not found.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
        }
    } while ((target = strtok(NULL, delim)));
    fd->CAT(fat12);
}

void doLL(char *input)
{
    char *s = (char *)malloc(sizeof(char) * 128);
    strcpy(s, input);
    char *delim = " ";
    char *target = strtok(s, delim);
    int get_l = 0;
    int get_ll = 0;
    int get_ls = 0;
    FileAndDir *fd = root_ptr;
    do
    {
        cout << target << endl;
        if (strcmp("ls", target) == 0)
        {
            if (get_ls == 1)
            {
                char *errorMsg = "Syntax Error: multiple 'ls' command.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            get_ls = 1;
        }
        else if (strcmp("-l", target) == 0)
        {
            if (get_l == 1)
            {
                char *errorMsg = "Syntax Error: 'ls' command with multiple '-l'.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            get_l = 1;
        }
        else if (strcmp("-ll", target) == 0)
        {
            if (get_ll == 1)
            {
                char *errorMsg = "Syntax Error: 'ls' command with multiple '-ll'.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            get_ll = 1;
        }
        else
        {
            if (fd != root_ptr)
            {
                char *errorMsg = "Error: redundancy params for file path.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
            char *temp = (char *)malloc(sizeof(char) * 128);
            strcpy(temp, target);
            fd = locateFD(root_ptr, temp);
            if (fd == NULL)
            {
                char *errorMsg = "Error: file or directory not found.\n";
                my_printWhite(errorMsg, strlen(errorMsg));
                return;
            }
        }
    } while ((target = strtok(NULL, delim)));

    if (fd->isDir == '0')
    {
        char *temp = "Error: not a dir.";
        my_printWhite(temp, strlen(temp));
        return;
    } // 检查，若为文件，没有ls，报错
    if (get_l == 1 || get_ll == 1)
    {
        fd->LS_L();
    }
    else
    {
        fd->LS();
    }
}
/*
将文件加载到数据结构中
*/
void loadFile(FILE *src, int offset, int length, void *dest)
{
    fseek(src, offset, SEEK_SET);
    fread(dest, 1, length, src);
}

/*
读FAT表中的内容，注意内容比较恶心，详见oranges
*/
int readFAT(FILE *fat12, int numOfClus)
{
    int base = bpb_ptr->BPB_RsvdSecCnt * bpb_ptr->BPB_BytsPerSec;
    int offset = (numOfClus / 2) * 3;
    int res = 0;
    int *ptr = &res;
    loadFile(fat12, base + offset, 24, ptr); // 直接把24位加载过来然后忽略高位
    if (numOfClus % 2 == 0)
    { // 偶数簇
        return (res & 0xFFF);
    }
    else
    { // 奇数簇
        return (res & 0xFFF000) >> 12;
    }
}

void readBPB(FILE *fat12)
{
    bpb_ptr = (struct BPB *)malloc(sizeof(struct BPB)); // 为BPB分配空间
    loadFile(fat12, 11, 25, bpb_ptr);                   //load BPB
    // cout << "BPB_BytsPerSec" << bpb_ptr->BPB_BytsPerSec << endl;
    // cout << "BPB_NumFATs" << bpb_ptr->BPB_NumFATs << endl;
    RootDirSectors = ((bpb_ptr->BPB_BytsPerSec - 1) + (bpb_ptr->BPB_RootEntCnt * 32)) / (bpb_ptr->BPB_BytsPerSec);
    DataBaseSectors = bpb_ptr->BPB_RsvdSecCnt + bpb_ptr->BPB_NumFATs * bpb_ptr->BPB_FATSz16 + RootDirSectors;
}

void readRootEntries(FILE *fat12)
{
    if (root_ptr != NULL)
    {
        delete root_ptr;
        root_ptr = NULL;
    }                          // 如果已经加载过了，清空，重新加载，释放内存避免内存泄漏
    root_ptr = new FileAndDir; // 构造根目录
    strcpy(root_ptr->path, "/");
    strcpy(root_ptr->name, "/");
    root_ptr->isDir = '1';
    root_ptr->direcDir = 0;
    root_ptr->directFile = 0;
    // 初始化根目录完成
    int offset = (bpb_ptr->BPB_RsvdSecCnt + bpb_ptr->BPB_NumFATs * bpb_ptr->BPB_FATSz16) * bpb_ptr->BPB_BytsPerSec;

    for (int i = 0; i < bpb_ptr->BPB_RootEntCnt; i++, offset += 32)
    {
        generateFD(fat12, root_ptr, offset);
    }
}

/*
读取子文件Entry，递归创建文件树
*/
void readChildEntries(FILE *fat12, FileAndDir *parent, int startClus)
{
    int offset = DataBaseSectors * bpb_ptr->BPB_BytsPerSec;
    int currentClus = startClus; // 可能有多个簇来保存
    int numOfFile = 0;           // 子文件数
    int numOfDir = 0;            // 子目录数
    while (true)
    {
        int nextClus = readFAT(fat12, currentClus);
        if (nextClus == 0xFF7)
        {
            break; // 坏簇
        }

        int byteBase = offset + (currentClus - 2) * bpb_ptr->BPB_SecPerClus * bpb_ptr->BPB_BytsPerSec;
        for (int index = 0; index < bpb_ptr->BPB_SecPerClus * bpb_ptr->BPB_BytsPerSec; index += 32)
        {
            generateFD(fat12, parent, byteBase + index);
        }

        if (nextClus >= 0xFF8)
        {
            break; // end
        }
        else
        {
            currentClus = nextClus;
        }
    }
}
/*
构造文件/目录实体
parent：父目录
offset：Entry在FAT中的偏移量
抽出一个共同方法，在生成文件树的时候进行复用
*/
void generateFD(FILE *fat12, FileAndDir *parent, int offset)
{
    struct RootEntry rootEntry;
    struct RootEntry *entry_ptr = &rootEntry;
    loadFile(fat12, offset, 32, entry_ptr);
    if (entry_ptr->DIR_Name[0] == '\0')
        return; //过滤非目标文件，要求文件名必须是大写字母或者数字
    int j;
    int valid = 1;
    for (j = 0; j < 11; j++)
    {
        char tem = entry_ptr->DIR_Name[j];
        if (!((tem >= 'A' && tem <= 'Z') || (tem >= '1' && tem <= '9') || tem == ' '))
        {
            valid = 0;
            break;
        }
    }
    if (valid == 0)
        return; //非目标文件不输出

    // cout << "DIR_Name" << entry_ptr->DIR_Name << endl;
    // cout << "DIR_Attr" << (int)entry_ptr->DIR_Attr << endl;
    // cout << "DIR_FileSize" << entry_ptr->DIR_FileSize << endl;
    // cout << "DIR_FstClus" << entry_ptr->DIR_FstClus << endl;
    FileAndDir *fd = new FileAndDir;
    fd->fileSize = entry_ptr->DIR_FileSize;
    fd->startClus = entry_ptr->DIR_FstClus;
    parent->subFDs.push_back(fd);
    if ((int)entry_ptr->DIR_Attr == 0x10)
    { //dir
        // cout << "dir" << endl;
        readFileName(entry_ptr->DIR_Name, fd->name, 1);
        fd->isDir = '1';
        parent->direcDir++;
        strcpy(fd->path, parent->path);
        strcat(fd->path, fd->name);
        strcat(fd->path, "/");
        // cout << "fd_name" << fd->name << endl;
        // cout << "fd_path" << fd->path << endl;
        // 如果是文件夹，递归加载子目录下的文件
        readChildEntries(fat12, fd, fd->startClus);
    }
    else
    { //file
        // cout << "file" << endl;
        readFileName(entry_ptr->DIR_Name, fd->name, 0);
        fd->isDir = '0';
        parent->directFile++;
        strcpy(fd->path, parent->path);
        strcat(fd->path, fd->name);
        // cout << "fd_name" << fd->name << endl;
        // cout << "fd_path" << fd->path << endl;
    }
}
/*
提取文件名，存入到FD中
分文件夹与文件两种情况
*/
void readFileName(char *src, char *dest, int isDir)
{
    char name[13];
    int index = 0;
    if (isDir)
    { // 目录，长度为8
        for (int i = 0; i < 8; ++i)
        {
            if (src[i] != ' ')
            {
                name[index++] = src[i];
            }
        }
    }
    else
    { // 文件，长度为8+3
        for (int i = 0; i < 8; ++i)
        {
            if (src[i] != ' ')
            {
                name[index++] = src[i];
            }
        }
        name[index++] = '.';
        for (int i = 8; i < 11; ++i)
        {
            if (src[i] != ' ')
            {
                name[index++] = src[i];
            }
        }
    }
    name[index++] = '\0';
    strcpy(dest, name);
}

FileAndDir *locateFD(FileAndDir *currentDir, char *filePath)
{
    FileAndDir *res = currentDir;
    char *target = (char *)malloc(sizeof(char) * 64);
    vector<string> v = split(filePath, "/");
    for(int i = 0; i< v.size();++i){
        strcpy(target,v.at(i).c_str());
        res = res->locate(target);
        if (res == NULL)
        {
            break; // 如果找不到就设为NULL，直接返回，外层再处理
        }
    }
    return res;
}

void FileAndDir::CAT(FILE *fat12)
{
    char *temp;
    if (this->isDir == '1')
    {
        temp = "Error: not a file.";
        my_printWhite(temp, strlen(temp));
        return;
    }
    else if (this->startClus < 2)
    {
        return; // empty file
    }
    int offset = DataBaseSectors * bpb_ptr->BPB_BytsPerSec;
    int currentClus = this->startClus;
    int bytsPerClus = bpb_ptr->BPB_BytsPerSec * bpb_ptr->BPB_SecPerClus;
    while (true)
    {
        int nextClus = readFAT(fat12, currentClus);
        if (nextClus == 0xFF7)
        {
            temp = "Error: bad clus.";
            my_printWhite(temp, strlen(temp));
            break; // 坏簇
        }
        int startByte = offset + (currentClus - 2) * bytsPerClus;
        temp = (char *)malloc(bytsPerClus);
        loadFile(fat12, startByte, bytsPerClus, temp);
        strcat(temp, "\0");
        my_printWhite(temp, strlen(temp));
        delete temp; // 避免内存泄漏
        currentClus = nextClus;
        if (nextClus >= 0xFF8)
        {
            break; // end
        }
    }
}

vector<string> split(const string &s, const string &seperator)
{
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while (i != s.size())
    {
        //找到字符串中首个不等于分隔符的字母；
        int flag = 0;
        while (i != s.size() && flag == 0)
        {
            flag = 1;
            for (string_size x = 0; x < seperator.size(); ++x)
            {
                if (s[i] == seperator[x])
                {
                    ++i;
                    flag = 0;
                    break;
                }
            }
        }

        //找到又一个分隔符，将两个分隔符之间的字符串取出；
        flag = 0;
        string_size j = i;
        while (j != s.size() && flag == 0)
        {
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[j] == seperator[x])
                {
                    flag = 1;
                    break;
                }
            if (flag == 0)
                ++j;
        }
        if (i != j)
        {
            result.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return result;
}