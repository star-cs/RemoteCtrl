#pragma once
class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize);


    // 开机启动的时候，程序的权限是跟随启动用户的
    // 如果两者权限不一致，则会导致程序启动失败
    // 开机启动对环境变量有影响，如果依赖dll(动态库)则可能启动失败
    // 【复制这些dll到system32下面或者syswow64下面】
    // system32 下面多是64位程序 syswow64下面多是32位程序
    // 【使用静态库，而非动态库】


    //设置开机自启：修改注册表方式(登录过程中启动) 
    //开机自启注册表位置：计算机
    static int WriteRefisterTable(const CString strPath) {
        if (PathFileExists(strPath))return 0;//注册表中已经存在
        
        // 程序静态库生成，所以不需要软链接了，直接复制文件。
        char exePath[MAX_PATH] = "";
        GetModuleFileName(NULL, exePath, MAX_PATH);
        bool ret = CopyFile(exePath, strPath, FALSE);
        if (ret == FALSE) {
            MessageBox(NULL, TEXT("复制文件夹失败，是否权限不足？\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -1;
        }

        //打开注册表开启自启位置
        CString strSubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";//注册表开机自启路径
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("打开注册表失败! 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -2;
        }

        //将可执行文件软连接添加到注册表开启自启路径下
        ret = RegSetValueEx(hKey, TEXT("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("注册表添加文件失败 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -3;
        }
        RegCloseKey(hKey);
        return 0;
    }

    //写入自启动文件方式
    static int WriteStartupDir(const CString& strPath)
    {
        if (PathFileExists(strPath))return 0; // 启动文件已经存在

        CString strCmd = GetCommandLine();
        TRACE("strCmd:%s \r\n", strCmd);        // 运行exe程序的绝对路径
        strCmd.Replace(TEXT("\""), TEXT(""));
        BOOL ret = CopyFile(strCmd, strPath, FALSE);
        if (ret == FALSE)
        {
            MessageBox(NULL, TEXT("复制文件夹失败，是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -1;
        }
        return 0;
    }


    static void ShowError() {
        LPCSTR lpMessageBuf = NULL;
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMessageBuf, 0, NULL);
        OutputDebugString(lpMessageBuf);
        LocalFree((void*)lpMessageBuf);
    }

    static bool IsAdmin() {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken));
        {
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
            ShowError();
            return false;
        }
        CloseHandle(hToken);
        if (len == sizeof(eve)) {
            return eve.TokenIsElevated;
        }

        printf("length of tokenInfomation is %d \r\n", len);
        return false;
    }


    static bool RunAsAdmin()
    {
        //TODO 获取管理员权限，使用该权限创建进程。

        WCHAR sPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, sPath, MAX_PATH);

        STARTUPINFOW si = { 0 };
        si.cb = sizeof(STARTUPINFOW);
        PROCESS_INFORMATION pi = { 0 };

        BOOL ret = CreateProcessWithLogonW((LPCWSTR)(LPWSTR)"Administrator", NULL, NULL, LOGON_WITH_PROFILE,
            NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);

        if (!ret) {
            MessageBox(NULL, TEXT("创建进程失败"), TEXT("程序错误"), MB_OK | MB_ICONERROR);
            ShowError();
            return false;
        }

        MessageBox(NULL, TEXT("进程创建成功"), TEXT("用户状态"), 0);

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    static bool Init()
    {
        HMODULE hModule = ::GetModuleHandle(nullptr);

        if (hModule == nullptr)
        {
            // TODO: 更改错误代码以符合需要
            wprintf(L"错误: GetModuleHandle 失败\n");
            return false;
        }

        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            return false;
        }
        return true;
    }

};

