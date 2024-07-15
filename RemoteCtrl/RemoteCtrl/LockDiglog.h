#pragma once
#include "afxdialogex.h"


// CLockDiglog 对话框

class CLockDiglog : public CDialog
{
	DECLARE_DYNAMIC(CLockDiglog)

public:
	CLockDiglog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CLockDiglog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
