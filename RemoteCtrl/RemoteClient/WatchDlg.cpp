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
		m_picture.ScreenToClient(&point);	//已知的已经是相对坐标
	else {
		ClientToScreen(&point);	//转换为全局坐标
		m_picture.ScreenToClient(&point);	//转换成相对于控件的坐标。
	}
	CRect clientRect;
	m_picture.GetWindowRect(clientRect);

	int width = clientRect.Width();
	int Height = clientRect.Height();

	CPoint cur(point.x* tarWidth / width, point.y * tarHeight / Height);
	TRACE("x = %d, y = %d \r\n", cur.x, cur.y);
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

	ON_MESSAGE(WM_SEND_PACKET_ACK, &CWatchDlg::OnSendPacketMessageAck)

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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

		CDialogEx::OnLButtonDblClk(nFlags, point);
	}
}

// 没有用到，鼠标按下抬起，会分别发包。
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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));


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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));
		
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

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&m_mouse, sizeof(m_mouse));

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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);

}


void CWatchDlg::OnUnlockBtn()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);

}

LRESULT CWatchDlg::OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2))
	{
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1)
	{
		//TODO警告
		TRACE("socket is closed!\r\n");
	}
	else
	{
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd)
			{
			case 6:	//显示
			{
				CTool::Byte2Image(m_image, head.strData);

				CRect rect;
				m_picture.GetWindowRect(rect);

				tarWidth = m_image.GetWidth();

				tarHeight = m_image.GetHeight();

				// 界面缩放
				m_image.StretchBlt(
					m_picture.GetDC()->GetSafeHdc(),
					0, 0,
					rect.Width(), rect.Height(),
					SRCCOPY);

				m_picture.InvalidateRect(NULL);		//通知画面重绘
				m_image.Destroy();
				m_isFull = false;
				break;
			}
			case 5://鼠标
			case 7://锁机
			case 8://解锁
			default: 
				break;
			}
		}

		//成功
		return 1;
	}

	return 0;
}
