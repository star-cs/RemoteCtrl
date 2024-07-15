// LockDiglog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "LockDiglog.h"


// CLockDiglog 对话框

IMPLEMENT_DYNAMIC(CLockDiglog, CDialog)

CLockDiglog::CLockDiglog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockDiglog::~CLockDiglog()
{
}

void CLockDiglog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockDiglog, CDialog)
END_MESSAGE_MAP()


// CLockDiglog 消息处理程序
