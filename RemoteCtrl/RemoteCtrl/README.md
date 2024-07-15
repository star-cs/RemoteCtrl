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


## 鼠标操作

### SetCursorPos()
用于将鼠标光标移动到屏幕上的指定位置

### mouse_event()

### GetMessageExtraInfo()

### 移动鼠标
当你在Windows API中使用 `SetCursorPos` 和 `mouse_event` 来移动鼠标时，这两个函数的行为有一些关键的不同点：

`SetCursorPos`
```cpp
SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
```
- **作用**：这行代码仅负责移动鼠标光标到 `(mouse.ptXY.x, mouse.ptXY.y)` 指定的坐标位置。
- **不产生事件**：`SetCursorPos` 不会产生任何鼠标事件，这意味着它不会被任何应用程序视为用户输入。应用程序不会接收到与移动相关的WM_MOUSEMOVE消息。

`mouse_event`
```cpp
mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
```
- **作用**：这行代码会发送一个鼠标移动事件到系统，仿佛是用户自然地移动了鼠标。
- **产生事件**：`mouse_event` 使用 `MOUSEEVENTF_MOVE` 标志来模拟鼠标移动，并且它会向系统发送一个WM_MOUSEMOVE消息，因此应用程序会认为这是一个真实的用户输入。
- **额外信息**：传递给 `mouse_event` 的最后一个参数是 `GetMessageExtraInfo()`，这通常用于在64位系统上获取鼠标位置的高32位。这样可以确保在大分辨率屏幕上移动事件的准确性。

`区别总结`
- **事件生成**：`SetCursorPos` 不生成鼠标移动事件，而 `mouse_event` 则会生成。
- **系统通知**：`SetCursorPos` 不通知系统或应用程序鼠标已移动，而 `mouse_event` 会通过事件通知系统和应用程序。
- **用户输入模拟**：`SetCursorPos` 仅移动光标，没有模拟用户输入；`mouse_event` 则完全模拟用户移动鼠标的行为。

在自动化测试或者需要模拟用户交互的脚本中，你可能更倾向于使用 `mouse_event`，因为它能够更准确地模拟用户行为，从而触发应用程序的预期响应。然而，如果你只是想要改变光标的位置，而不关心是否触发相关事件，`SetCursorPos` 就足够了。


## 发送屏幕
```cpp
int SendScreen()
{
    CImage screen;  //GDI

    HDC hScreen = ::GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeigth = GetDeviceCaps(hScreen, VERTRES);

    screen.Create(nWidth, nHeigth, nBitPerPixel);

    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeigth, hScreen, 0, 0, SRCCOPY);

    ReleaseDC(NULL, hScreen);

    //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL) {
        return -1;
    }

    IStream* pStream = NULL;

    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);

    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);

        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);

        PBYTE pData = (PBYTE)GlobalLock(hMem);

        SIZE_T nSize = GlobalSize(hMem);

        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);

        GlobalUnlock(hMem);
    }

    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    
 
    return 0;
}
```

详细分析

1. **创建`CImage`对象**:
   ```cpp
   CImage screen;
   ```
   创建一个`CImage`对象，它将用于存储屏幕的图像。

2. **获取屏幕的设备上下文**:
   ```cpp
   HDC hScreen = ::GetDC(NULL);
   ```
   使用`GetDC`函数获取整个屏幕的设备上下文（DC）。传递`NULL`表示获取整个屏幕的DC。

3. **获取屏幕分辨率和位深度**:
   ```cpp
   int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
   int nWidth = GetDeviceCaps(hScreen, HORZRES);
   int nHeigth = GetDeviceCaps(hScreen, VERTRES);
   ```
   通过调用`GetDeviceCaps`函数，获取屏幕的位深度（每像素的位数）、宽度和高度。

4. **创建位图**:
   ```cpp
   screen.Create(nWidth, nHeigth, nBitPerPixel);
   ```
   使用`CImage`对象的`Create`方法创建一个新的位图，其尺寸和颜色深度与屏幕匹配。

5. **复制屏幕内容到位图**:
   ```cpp
   BitBlt(screen.GetDC(), 0, 0, nWidth, nHeigth, hScreen, 0, 0, SRCCOPY);
   ```
   使用`BitBlt`函数，将屏幕的内容复制到前面创建的位图中。

6. **释放屏幕的DC**:
   ```cpp
   ReleaseDC(NULL, hScreen);
   ```
   释放之前获取的屏幕DC，避免资源泄漏。

7. **分配内存和创建流**:
   ```cpp
   HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
   IStream* pStream = NULL;
   HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
   ```
   使用`GlobalAlloc`函数分配可移动的全局内存。接着，使用`CreateStreamOnHGlobal`函数创建一个基于全局内存的流对象。

8. **保存位图到流**:
   ```cpp
   screen.Save(pStream, Gdiplus::ImageFormatPNG);
   ```
   使用`CImage`对象的`Save`方法将位图保存为PNG格式到之前创建的流中。

9. **定位到流的起始位置**:
   ```cpp
   LARGE_INTEGER bg = { 0 };
   pStream->Seek(bg, STREAM_SEEK_SET, NULL);
   ```
   使用`Seek`方法将流的位置移动到起始位置。

10. **获取内存地址和大小**:
    ```cpp
    PBYTE pData = (PBYTE)GlobalLock(hMem);
    SIZE_T nSize = GlobalSize(hMem);
    ```
    使用`GlobalLock`函数锁定之前分配的内存，获取内存的起始地址和大小。

11. **创建数据包并发送**:
    ```cpp
    CPacket pack(6, pData, nSize);
    CServerSocket::getInstance()->Send(pack);
    ```
    创建一个`CPacket`对象，封装PNG图像数据，然后使用`CServerSocket`的`Send`方法发送这个数据包。

12. **释放资源**:
    ```cpp
    GlobalUnlock(hMem);
    pStream->Release();
    GlobalFree(hMem);
    ```
    解锁和释放之前分配的全局内存，释放流对象。

13. **释放`CImage`的DC**:
    ```cpp
    screen.ReleaseDC();
    ```
    调用`CImage`的`ReleaseDC`方法。但是，通常`CImage`会自动管理其DC，因此这一步可能不是必需的。

14. **返回**:
    ```cpp
    return 0;
    ```
    函数成功执行后返回0。

确保在实际使用中处理好错误条件，比如`GlobalAlloc`失败的情况，以及在`CServerSocket::Send`中可能出现的网络错误。此外，对于多显示器系统，你可能需要修改代码来处理每个显示器，而不是只捕获主显示器的屏幕。