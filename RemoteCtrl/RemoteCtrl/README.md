# 配置
属性-链接器-所有选项-入口点（mainCRTStartup） 子系统（窗口）
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

# 单例模式 辅助类（如CHelper）
在C++中，对于单例模式的实现，使用一个辅助类（如CHelper）来确保单例对象的正确创建和销毁是一种常见做法，尤其是在涉及动态分配和资源管理时。下面解释为什么需要helper类来协助删除单例对象，而不是直接在对象不再需要时调用析构函数：
单例生命周期管理的挑战

    确保单例的唯一性:
    单例模式要求在整个应用程序中只有一个实例存在。这意味着构造函数必须是私有的，防止外部直接创建实例。

    正确地创建和销毁:
    创建单例实例通常发生在类的静态成员函数中（如getInstance()）。但如何确保在适当的时候销毁这个实例呢？

析构函数的限制

析构函数不能由程序员显式调用。它是当对象超出作用域或被显式删除时由编译器自动调用的。
在单例模式下，由于`单例对象通常是全局的或静态的，它的生命周期与程序的生命周期一致。`
这意味着如果没有特殊的机制，单例对象的析构函数将不会在程序正常终止之前被调用。
CHelper的作用

CHelper类的引入解决了上述问题。CHelper有一个构造函数和一个析构函数，它们分别在CHelper对象的创建和销毁时被调用。通过以下步骤，CHelper确保了单例对象的正确生命周期管理：

    创建单例:
        CHelper的构造函数中调用CServerSocket::getInstance()，这会创建或返回已存在的CServerSocket实例。

    销毁单例:
        当CHelper对象的生命周期结束（即，当程序退出时），其析构函数会被调用。
        析构函数中调用CServerSocket::releaseInstance()，这会释放CServerSocket的单例实例。

这种方法的优点是：

    自动管理:
    CHelper的生命周期与程序的生命周期自然同步，确保了单例的创建和销毁在正确的时间点进行。

    防止内存泄漏:
    即使程序员忘记显式调用释放函数，CHelper也会在程序退出时自动调用，避免了内存泄漏的风险。

    易于理解:
    这种模式清晰地表明了单例对象的生命周期管理策略，使得代码更加健壮和易于维护。

总结

使用CHelper类来管理单例对象的生命周期，是C++中一种优雅的解决方案，它确保了资源的正确释放，同时避免了析构函数不能显式调用带来的问题。这种方法尤其适用于那些需要在程序启动时初始化并在程序结束时清理的资源密集型对象，如数据库连接或网络套接字。


# 数据解包后存在多个 CC 内存对齐问题。
```cpp
#pragma pack(push)
#pragma pack(1)	//按照一字节对齐，解决CC的问题
class Packet
{
    ...
    ...
};

#pragma pack(pop)
```
禁止内存对齐。

# (const char*)&pack 不合理

将 Packet 类的实例转换为 BYTE* 指针，意味着你想将整个 Packet 对象的内存布局扁平化为一个字节数组。
这样做可以有多种用途，比如在网络上传输数据或者将数据写入文件。
然而，由于 Packet 包含一个 std::string 成员，这个操作并不像对纯 POD（Plain Old Data）结构体那样直接。

std::string 在内部包含三个部分：指向字符串数据的指针、字符串的大小以及字符串的容量。
这意味着 std::string 并不是一个简单的连续内存块，不能简单地通过取地址的方式将其转换为一个字节指针并保证序列化正确。

可能得到的是 std::string元素的地址。

```cpp
    const char* Data() {
		strOut.resize(Size());
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;	pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd;	pData += 2;
		memcpy(pData, strData.c_str(), strData.size());	pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str(); 
	}

public: 
    std::string strOut;
```

# char 数组赋值
```cpp
    memcpy(finfo->szFileName, fdata.name, strlen(fdata.name));
```




# API笔记
## OutputDebugString();
OutputDebugString 是一个 Windows API 函数，主要用于在调试过程中向调试器输出文本消息。

## _chdir()  
这个函数允许程序在运行时切换目录，这对于访问不同位置的文件或目录非常有用。
```cpp
int _chdir(
   const char *path
);
```

## _finddata_t 结构体
```cpp
struct _finddata_t {
    unsigned int attrib;       // 文件属性
    size_t size;               // 文件大小
    time_t time_create;        // 创建时间
    time_t time_access;        // 上次访问时间
    time_t time_write;         // 上次修改时间
    char name[260];            // 文件名
};
```

> attrib 文件属性
> _A_NORMAL: 文件正常，没有特殊属性（0x00）
> _A_RDONLY: 文件只读（0x01）
> _A_HIDDEN: 文件隐藏（0x02）
> _A_SYSTEM: 文件是系统文件（0x04）
> _A_VOLID: 通常用于表示文件不再存在于卷上（0x08），但此标志的实际含义可能因文件系统而异
> _A_SUBDIR: 文件是一个子目录（0x10）
> _A_ARCH: 文件的存档属性，通常表示文件已被修改或创建（0x20）


## _findfirst()   
在 Windows 平台上用于文件枚举，类似于 Unix 中的 glob() 函数。
```cpp
intptr_t _findfirst(
   const char *filename,     /* 文件名模式 */                // "*" "*.txt" ...
   struct _finddata_t *fileinfo /* 结果信息结构体的指针 */
);
```

## _findnext()
在使用 _findfirst 函数初始化搜索后继续枚举目录中的文件。这个函数允许你遍历当前目录中与指定模式匹配的所有文件。

_findnext 的原型如下：
```cpp
int _findnext(
   intptr_t hFile,       // 文件查找句柄
   struct _finddata_t *fileinfo // 文件信息结构体的指针
);
```

## ShellExecuteA()
启动与 shell 动作关联的操作，比如打开文件、文件夹或者其他应用程序。

```cpp
HINSTANCE ShellExecuteA(
  HWND hwnd,     // 父窗口句柄，通常为 NULL
  LPCSTR lpOperation, // 执行的操作，如 "open"
  LPCSTR lpFile,      // 文件名或可执行文件的路径
  LPCSTR lpParameters,// 参数，可选
  LPCSTR lpDirectory, // 启动目录，可选
  INT nShowCmd        // 显示窗口的方式
);
```

## 文件操作

### fopen()  fopen_s()



### fseek()

### ftell()

### fread()

### fwrite()

### fclose()

