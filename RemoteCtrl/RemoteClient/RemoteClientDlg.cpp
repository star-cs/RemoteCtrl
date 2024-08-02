
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
#include "ClientController.h"


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

//类的内部函数可以调用相关的控件属性


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
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADD_SERVER, &CRemoteClientDlg::OnIpnFieldchangedIpaddServer)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACKET_ACK, &CRemoteClientDlg::OnSendPacketMessageAck)
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
	//m_server_address = 0x7F000001;
	//192.168.56.1
	m_server_address = 0xc0a83865;
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort)); 
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


//加载当前被选中句柄的全部文件(使用在删除文件之后重新加载所有文件到显示列表中)
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();

	bool nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTree);
}

//不同nCmd调用不同结果处理函数 strData:结果数据 lParam：附带参数(树节点句柄/文件指针)
void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd)
	{
	case 1:
		Str2DriveTree(strData, m_tree);
		break;
	case 2:
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		MessageBox("打开文件完成！", "操作成功", MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
		MessageBox("删除文件完成!", "操作完成", MB_ICONINFORMATION);
		break;
	case 2024://测试连接
		MessageBox("连接测试成功!", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received!%d\r\n", nCmd);
		break;
	}
}

// =====功能性函数
void CRemoteClientDlg::Str2DriveTree(std::string data, CTreeCtrl& tree)
{
	//只有一个应答包
	std::string drives = data;

	std::string dr;
	m_tree.DeleteAllItems();
	m_List.DeleteAllItems();

	for (size_t i = 0; i < drives.size(); i++)
	{
		if (drives[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drives[i];
	}
}

void CRemoteClientDlg::UpdateFileInfo(FILEINFO& fileInfo, HTREEITEM hTreeSelected)
{
	//最后还有一个应答包
	if (fileInfo.HasNext == false) return;

	TRACE("fileInfo.szFileName = %s\r\n", fileInfo.szFileName);

	if (fileInfo.IsDirectory) {
		//目录添加到左侧的树状里。
		if (CString(fileInfo.szFileName) == "." || (CString(fileInfo.szFileName) == ".."))
		{
			return;
		}
		HTREEITEM hTemp = m_tree.InsertItem(fileInfo.szFileName, hTreeSelected, TVI_LAST);
		m_tree.Expand(hTreeSelected, TVE_EXPAND);//展开选项
	}
	else {
		//文件添加到右侧的列表中。
		m_List.InsertItem(0, fileInfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static long long nLength = 0, index = 0;
	if (nLength == 0) {	//第一个包
		nLength = *(long long*)strData.c_str();
		if (nLength == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件!!!");
			CClientController::getInstance()->DownloadEnd();//结束下载
			return;
		}
	}
	else if ((nLength > 0) && (index >= nLength)) {		//写完了，最后还会发一个结束的通知包
		fclose(pFile);
		nLength = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();//结束下载
	}
	else {
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();

		double cur_portion = index * 100.0 / nLength;
		CClientController::getInstance()->setStatus(cur_portion);
	}
}
// =====


// =====以下函数 消息响应调用函数，负责发送命令
void CRemoteClientDlg::OnBnClickedButTest()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2024);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	CClientController* pController = CClientController::getInstance();

	bool ret = pController->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == false) {
		AfxMessageBox(_T("命令处理失败"));
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

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();

	CString strPath = GetPath(hTreeSelected);

	bool nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);

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
	int nListSelected = m_List.GetSelectionMark();
	CString strPath = m_List.GetItemText(nListSelected, 0);

	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strPath = GetPath(hSelected) + strPath;

	int ret = CClientController::getInstance()->DownFile(strPath);
	if (ret != 1) {
		MessageBox(TEXT("下载失败!"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}

void CRemoteClientDlg::OnRunFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strPath = m_List.GetItemText(nListSelected, 0);

	HTREEITEM filePath = m_tree.GetSelectedItem();
	strPath = GetPath(filePath) + strPath;

	bool ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, 
		true, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret == false) {
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
		bool ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true,
			(BYTE*)(LPCSTR)strPath, strPath.GetLength());
		if (!ret) {
			AfxMessageBox("删除文件失败");
			TRACE("删除文件失败，ret = %d\r\n", ret);
			return;
		}
		LoadFileCurrent();
	}
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddServer(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}
void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort));

}

LRESULT CRemoteClientDlg::OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2)) {
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1)	//对面关闭了套接字
	{
		TRACE("socket is closed!\r\n");
	}
	else
	{
		if (wParam != NULL) {
			CPacket pack = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(pack.sCmd, pack.strData, lParam);
		}
		return 1;
	}
	return 0;
}
