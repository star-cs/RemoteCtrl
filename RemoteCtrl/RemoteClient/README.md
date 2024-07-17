# 树形控件

```cpp
//=====关键代码=====

CTreeCtrl m_tree;

//===磁盘驱动信息===

//HTREEITEM 节点结构体
HTREEITEM hTemp =m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);

m_tree.InsertItem("", hTemp, TVI_LAST);
			

//===拼接路径得到文件绝对地址===
CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTemp;
	do {
		strTemp = m_tree.GetItemText(hTree);

		strRet = strTemp + "\\" +  strRet;
		
		hTree = m_tree.GetParentItem(hTree);

	} while (hTree != NULL);
	return strRet;
}

//避免多次双击同一个节点，每次双击前都得清除掉节点的子节点。
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(hTree);
		if (hSub != NULL) m_tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


//
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	
	// 获取鼠标绝对坐标 --> 转换成相对坐标 --> HitTest()获得鼠标指针位置对于的树控件项
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);	//用于确定鼠标指针位置对应的树控件项。
	if (hTreeSelected == NULL) {
		return;
	}


	if (m_tree.GetChildItem(hTreeSelected) == NULL)	//文件不做后续操作。
		return;

	DeleteTreeChildrenItem(hTreeSelected);	//清空

	CString strPath = GetPath(hTreeSelected);

	int nCmd = SendCommandPacket(2, false,(BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	
	PFILEINFO pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pFileInfo->HasNext) {
		if (pFileInfo->IsDirectory) {
			if (CString(pFileInfo->szFileName) == "." || (CString(pFileInfo->szFileName) == ".."))
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\n", pClient->GetPacket().sCmd);
				if (cmd < 0) {
					break;
				}
				pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
		}
		HTREEITEM hTemp = m_tree.InsertItem(pFileInfo->szFileName, hTreeSelected, TVI_LAST);
		if (pFileInfo->IsDirectory) {	//目录都会添加一个空节点，用于区分文件。
			m_tree.InsertItem("", hTemp, TVI_LAST);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\n", pClient->GetPacket().sCmd);
		if (cmd < 0) {
			break;
		}
		  
		pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	
	pClient->CloseSocket();
}
```