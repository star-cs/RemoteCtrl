// WatchDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDlg.h"

#include "ClientController.h"

// CWatchDlg 对话框

IMPLEMENT_DYNAMIC(CWatchDlg, CDialogEx)

CWatchDlg::CWatchDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(133, pParent)
{
	tarWidth = -1;
	tarHeight = -1;
}

CWatchDlg::~CWatchDlg()
{
}

void CWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}

CPoint CWatchDlg::UserPoint2ScreenPoint(CPoint& point, bool isScreen)
{
	if (isScreen)
		ScreenToClient(&point);	//已知的已经是相对坐标

	CRect clientRect;
	m_picture.GetWindowRect(clientRect);

	int width = clientRect.Width();
	int Height = clientRect.Height();

	CPoint cur(point.x* tarWidth / width, point.y * tarHeight / Height);
	TRACE("%d, %d \r\n", cur.x, cur.y);
	return cur;
}


BEGIN_MESSAGE_MAP(CWatchDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDlg::OnStnClickedWatch)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_LOCK_BTN, &CWatchDlg::OnLockBtn)
	ON_COMMAND(ID_UNLOCK_BTN, &CWatchDlg::OnUnlockBtn)
END_MESSAGE_MAP()


// CWatchDlg 消息处理程序

BOOL CWatchDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//// TODO:  在此添加额外的初始化

	m_isFull = false;
	SetTimer(0, 45, NULL);		//nIDEvent，频率50ms，回调函数

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0) {
		CClientController* pController = CClientController::getInstance();
		if (m_isFull) {
			CRect rect;
			m_picture.GetWindowRect(rect);

			if (tarWidth == -1) {
				tarWidth = m_image.GetWidth();
			}

			if (tarHeight == -1) {
				tarHeight = m_image.GetHeight();
			}

			// 界面缩放
			m_image.StretchBlt(
				m_picture.GetDC()->GetSafeHdc(),
				0, 0,
				rect.Width(), rect.Height(),
				SRCCOPY);

			m_picture.InvalidateRect(NULL);		//通知画面重绘
			m_isFull = false;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {
		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 2;
		m_mouse.nButton = 0;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnLButtonDown(nFlags, point);
	}
}

void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 3;
		m_mouse.nButton = 0;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnLButtonUp(nFlags, point);
	}
}


void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 1;
		m_mouse.nButton = 0;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnLButtonDblClk(nFlags, point);
	}
}

void CWatchDlg::OnStnClickedWatch()
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		CPoint point;
		GetCursorPos(&point);
		CPoint cur = UserPoint2ScreenPoint(point, true);

		MOUSEEV m_mouse;
		m_mouse.nAction = 0;
		m_mouse.nButton = 0;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

	}

}

void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 2;
		m_mouse.nButton = 1;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));


		CDialogEx::OnRButtonDown(nFlags, point);
	}
}

void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 3;
		m_mouse.nButton = 1;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnRButtonUp(nFlags, point);
	}
}

void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 1;
		m_mouse.nButton = 1;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));
		
		CDialogEx::OnRButtonDblClk(nFlags, point);
	}
}



void CWatchDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((tarHeight != -1) && (tarWidth != -1)) {

		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint cur = UserPoint2ScreenPoint(point);

		MOUSEEV m_mouse;
		m_mouse.nAction = 1;
		m_mouse.nButton = 3;
		m_mouse.ptXY = cur;

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnMouseMove(nFlags, point);
	}
}


void CWatchDlg::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialogEx::OnOK();
}


void CWatchDlg::OnLockBtn()
{
	CClientController::getInstance()->SendCommandPacket(7);

}


void CWatchDlg::OnUnlockBtn()
{
	CClientController::getInstance()->SendCommandPacket(8);

}
