#pragma once
#include "afxdialogex.h"


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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	CStatic m_image;
};
