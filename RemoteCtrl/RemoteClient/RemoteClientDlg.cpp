
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#include "ClientSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "WatchDlg.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()

};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CRemoteClientDlg 对话框


CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADD_SERVER, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = CClientSocket::getInstance();

	for (;;) {
		
		if (m_isFull == false) {
			//每发一次命令，接收一次截图
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1, NULL);
			if (ret == 6) {

				BYTE* data = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMen == NULL) {
					TRACE("内存不足！");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
				if (ret == S_OK) {
					ULONG length = 0;
					pStream->Write(data, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);

					m_image.Load(pStream);

					m_isFull = true;
				}
				pStream->Release();
				GlobalFree(hMen);
			}
			else {
				//没发送成功就睡眠，防止CPU持续跑满
				Sleep(1);
			}
		}
		else {
			//缓存还是满的，
			Sleep(1);
		}
		
		
	}
	

	pClient->CloseSocket();

}

void CRemoteClientDlg::threadEntryForDownloadFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

//类的内部函数可以调用相关的控件属性
void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);	// 0号位是列表item名
	
	CFileDialog dlg(FALSE, "*",
		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, this);

	if (dlg.DoModal() == IDOK) {		// 模态

		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox("没有权限保存该文件，或者文件无法创建！！！");
			m_StatusDlg.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		HTREEITEM hSelected = m_tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("[客户端]%s\r\n", LPCSTR(strFile));

		CClientSocket* pClient = CClientSocket::getInstance();
		
		do {
			// CString --> LPCSTR --> BYTE*
			// 发送会调用到 UpdateData()，需要用消息 分离线程
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			
			// MFC框架下，线程函数是CWnd的子函数，已知对应的线程id
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);

			if (ret < 0) {
				AfxMessageBox("执行下载命令失败！");
				TRACE("执行下载命令失败，ret = %d\r\n", ret);
				break;
			}

			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox("文件长度为零，或者无法读取文件！！！");
				break;
			}

			m_StatusDlg.download_process.SetRange(0, 100);

			long long nCount = 0;
			while (nCount < nLength)
			{
				double cur_portion = nCount * 100.0 / nLength ;
				m_StatusDlg.download_process.SetPos(cur_portion);

				CString temp;
				temp.Format(_T("命令执行中 进度 = %f %%"), cur_portion);
				m_StatusDlg.m_info.SetWindowText(temp);

				ret = pClient->DealCommand();
				 
				if (ret < 0) {
					AfxMessageBox("传输失败！！！");
					TRACE("传输失败！！！，ret = %d\r\n", ret);
					break;
				}

				size_t cur_size = pClient->GetPacket().strData.size();

				fwrite(pClient->GetPacket().strData.c_str(), 1, cur_size, pFile);

				nCount += cur_size;
			}

		} while (0);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_StatusDlg.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！"), _T("完成"));
}


int CRemoteClientDlg::SendCommandPacket(int nCmd,bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();

	CClientSocket* pClient = CClientSocket::getInstance();
	//bool ret = pClient->InitSokcet("127.0.0.1");
	bool ret = pClient->InitSokcet(m_server_address, atoi((LPCTSTR)m_nPort));

	if (!ret) {
		AfxMessageBox("网络初始化失败！");
		return -1;
	}

	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);

	int cmd = pClient->DealCommand();

	TRACE("[客户端]ack:%d\n", pClient->GetPacket().sCmd);

	//获取文件夹信息暂时不关闭
	if (bAutoClose) {
		pClient->CloseSocket();
	}

	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUT_TEST, &CRemoteClientDlg::OnBnClickedButTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_COMMAND(ID_DEL_FILE, &CRemoteClientDlg::OnDelFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendMessage)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);
	
	// 初始化窗口
	m_StatusDlg.Create(IDD_DLG_STATUS, this);
	m_StatusDlg.ShowWindow(SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	} 
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedButTest()
{
	SendCommandPacket(2024);
}


//获取磁盘信息
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败"));
	}

	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drives = pClient->GetPacket().strData;

	std::string dr;
	m_tree.DeleteAllItems();
	m_List.DeleteAllItems(); 

	for (size_t i = 0; i < drives.size(); i++)
	{
		if (drives[i] == ',') {
			dr += ":";
			HTREEITEM hTemp =m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			//m_tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drives[i];
	}
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) {
		return;
	}

	//if (m_tree.GetChildItem(hTreeSelected) == NULL)	//没有默认的空节点，说明是一个文件。
	//	return;

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();

	CString strPath = GetPath(hTreeSelected);

	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

	PFILEINFO pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int count = 0;
	while (pFileInfo->HasNext) 
	{
		TRACE("[客户端] FileName = [%s] HasNext=%d IsDirectory=%d IsInvalid=%d \r\n", pFileInfo->szFileName, pFileInfo->HasNext, pFileInfo->IsDirectory, pFileInfo->IsInvalid);
		if (pFileInfo->IsDirectory) {
			//目录添加到左侧的树状里。
			if (CString(pFileInfo->szFileName) == "." || (CString(pFileInfo->szFileName) == ".."))
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\n", pClient->GetPacket().sCmd);
				if (cmd < 0) {
					break;
				}
				pFileInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
				continue;
			}
			
			HTREEITEM hTemp = m_tree.InsertItem(pFileInfo->szFileName, hTreeSelected, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			//文件添加到右侧的列表中。
			m_List.InsertItem(0, pFileInfo->szFileName);
		}
		count++;
		int cmd = pClient->DealCommand();
		//TRACE("ack:%d\r\n", pClient->GetPacket().sCmd);
		if (cmd < 0) {
			break;
		}
		pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
	TRACE("[客户端]file_count = %d\n", count);
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTemp;
	do {
		strTemp = m_tree.GetItemText(hTree);
		
		strRet = strTemp + "\\" +  strRet;
		
		hTree = m_tree.GetParentItem(hTree);

	} while (hTree != NULL);
	return strRet;
}


//避免多次双击同一个节点，每次双击前都得清除掉节点的子节点。
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(hTree);
		if (hSub != NULL) m_tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);

	if (ListSelected < 0)return;
	
	//添加右键菜单显示
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	_beginthread(CRemoteClientDlg::threadEntryForDownloadFile, 0, this);
	BeginWaitCursor();		// 设置光标为等待状态。
	m_StatusDlg.m_info.SetWindowText(_T("命令执行中..."));
	m_StatusDlg.ShowWindow(SW_SHOW);
	m_StatusDlg.CenterWindow(this);
	m_StatusDlg.SetActiveWindow();
}


void CRemoteClientDlg::OnRunFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strPath = m_List.GetItemText(nListSelected, 0);

	HTREEITEM filePath = m_tree.GetSelectedItem();
	strPath = GetPath(filePath) + strPath;

	int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret != 3) {
		AfxMessageBox("启动文件失败！");
		TRACE("启动文件失败，ret = %d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDelFile()
{
	// TODO: 在此添加命令处理程序代码
	int nListSelected = m_List.GetSelectionMark();
	CString strPath = m_List.GetItemText(nListSelected, 0);

	HTREEITEM filePath = m_tree.GetSelectedItem();
	strPath = GetPath(filePath) + strPath;
	int result = AfxMessageBox(_T("你确定要删除这个文件吗？"), MB_YESNO | MB_ICONQUESTION);
	if (result == IDYES) {
		int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
		if (ret != 9) {
			AfxMessageBox("删除文件失败");
			TRACE("删除文件失败，ret = %d\r\n", ret);
			return;
		}
		m_List.DeleteItem(nListSelected);	//列表中删除对应item
	}
}

//消息响应
LRESULT CRemoteClientDlg::OnSendMessage(WPARAM wParam, LPARAM lParam)
{
	int ret = -1;
	if (lParam == NULL) {
		//	6 屏幕监控命令发送
		ret = SendCommandPacket(wParam >> 1, wParam & 1, NULL, 0);
	}
	else { 
		//  4 下载文件命令发送
		CString strFile = (LPCSTR)lParam;
		ret = SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	}
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CWatchDlg dlg(this);
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();			//模态，保证这个按钮不会被反复点击。
}
