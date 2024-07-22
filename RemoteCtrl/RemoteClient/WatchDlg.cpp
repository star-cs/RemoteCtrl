// WatchDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDlg.h"

#include "RemoteClientDlg.h"

// CWatchDlg 对话框

IMPLEMENT_DYNAMIC(CWatchDlg, CDialogEx)

CWatchDlg::CWatchDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{

}

CWatchDlg::~CWatchDlg()
{
}

void CWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_image);
}


BEGIN_MESSAGE_MAP(CWatchDlg, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWatchDlg 消息处理程序

BOOL CWatchDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	MoveWindow(0, 0, 1921, 1081);
	CenterWindow();
	m_image.MoveWindow(0, 0, 1920, 1080);

	SetTimer(0, 45, NULL);		//nIDEvent，频率50ms，回调函数

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0) {
		CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent(); //获取父窗口
		if (pPersent->isFull()) {
			//pPersent->getImage().BitBlt(m_image.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			CRect rect;
			m_image.GetWindowRect(rect);
			// 界面缩放
			pPersent->getImage().StretchBlt(
				m_image.GetDC()->GetSafeHdc(),
				0, 0, 
				rect.Width(), rect.Height(), 
				SRCCOPY);

			//通知画面重绘
			m_image.InvalidateRect(NULL);
			pPersent->getImage().Destroy();
			pPersent->setImageStatus();			//isFull重置为false
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}
