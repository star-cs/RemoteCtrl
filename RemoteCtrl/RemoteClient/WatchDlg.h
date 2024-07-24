#pragma once
#include "afxdialogex.h"
#include "ClientSocket.h"


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
	CPoint UserPoint2ScreenPoint(CPoint& point, bool isScreen = false);

	int tarHeight, tarWidth;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	CStatic m_image;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual void OnOK();
};
