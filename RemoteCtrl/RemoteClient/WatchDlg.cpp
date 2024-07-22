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

CPoint CWatchDlg::UserPoint2ScreenPoint(CPoint& point)
{
	m_image.ScreenToClient(&point);

	CRect clientRect;
	m_image.GetWindowRect(clientRect);

	int width = clientRect.Width();
	int Height = clientRect.Height();

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();
	int tarWidth = pPersent->getImage().GetWidth();
	int tarHeight = pPersent->getImage().GetHeight();

	return CPoint(point.x / width * tarWidth, point.y / Height * tarHeight);
}


BEGIN_MESSAGE_MAP(CWatchDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDlg::OnStnClickedWatch)
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



void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	/*CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 3;
	m_mouse.nButton = 0;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));*/

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	/*CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 2;
	m_mouse.nButton = 0;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));*/


	CDialogEx::OnLButtonUp(nFlags, point);
}


void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 1;
	m_mouse.nButton = 0;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CWatchDlg::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);
	CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 0;
	m_mouse.nButton = 0;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

}

void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 2;
	m_mouse.nButton = 2;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 3;
	m_mouse.nButton = 2;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CPoint cur = UserPoint2ScreenPoint(point);

	MOUSEEV m_mouse;
	m_mouse.nAction = 1;
	m_mouse.nButton = 2;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

	CDialogEx::OnRButtonDblClk(nFlags, point);
}



void CWatchDlg::OnMove(int x, int y)
{
	CPoint cur = UserPoint2ScreenPoint(CPoint(x,y));

	MOUSEEV m_mouse;
	m_mouse.nAction = -1;
	m_mouse.nButton = 3;
	m_mouse.ptXY = cur;

	CRemoteClientDlg* pPersent = (CRemoteClientDlg*)GetParent();

	int ret = pPersent->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(MOUSEEV));

	CDialogEx::OnMove(x, y);
}


