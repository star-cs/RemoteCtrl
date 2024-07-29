
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER+1)

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	
public:
	bool isFull() const {
		return m_isFull;
	}

	CImage& GetImage() {
		return m_image;
	}

	void setImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}

private:
	CImage m_image; // 截图缓存。
	bool m_isFull;	// 缓存是否填充

private:
	void LoadFileInfo();

	CString GetPath(HTREEITEM hTree);

	void DeleteTreeChildrenItem(HTREEITEM hTree);

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_StatusDlg;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnRunFile();
	afx_msg void OnDelFile();
	afx_msg LRESULT OnSendMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnIpnFieldchangedIpaddServer(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
};
