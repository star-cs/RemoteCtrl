#pragma once
#include "afxdialogex.h"
#include "ClientSocket.h"

#ifdef WM_SEND_PACKET_ACK
#define WM_SEND_PACKET_ACK (WM_USER + 2)	//接收数据包
#endif // WM_SEND_PACKET_ACK

// CWatchDlg 对话框

class CWatchDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWatchDlg)

public:
	CWatchDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	bool isFull() const {
		return m_isFull;
	}

	void setImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}

	CImage& GetImage() {
		return m_image;
	}

	CPoint UserPoint2ScreenPoint(CPoint& point, bool isScreen = false);

	int tarHeight, tarWidth;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	CStatic m_picture;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual void OnOK();
	afx_msg void OnLockBtn();
	afx_msg void OnUnlockBtn();

	afx_msg LRESULT OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam);

	
private:
	bool m_isFull;	// 缓存是否填充
	CImage m_image; // 截图缓存。
};
