#pragma once
#pragma pack(push)
#pragma pack(1)	//按照一字节对齐，解决CC的问题

#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//nSize 数据大小
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (int j = 0; j < nSize; j++) {
			sSum += BYTE(pData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	//nSize传入是数据大小，传出是获取的数据量
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		//遍历pData，找到以0xFEFF开头的包头。
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		//可以等于nSize，只有控制命令，无包数据。
		if (i + 4 + 2 + 2 > nSize) {	//包数据可能不全，或者包头未能全部接收到。
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (i + nLength > nSize) {		//可能没有对大数据包进行 发送端 分包处理。nLength记录着整个包的长度。
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD temp = 0;
		for (int j = 0; j < strData.size(); j++) {
			temp += BYTE(strData[j]) & 0xFF;
		}

		if (temp == sSum) {
			//nSize = nLength + 2 + 4;
			nSize = i;						//包头前面可能有无效数据，得记录全部，并传出。
			return;
		}
		nSize = 0;
	}

	~CPacket() {}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() {	//数据的大小
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(Size());
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;	pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd;	pData += 2;
		memcpy(pData, strData.c_str(), strData.size());	pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

public:
	//unsigned short	WORD
	//unsigned long		DWORD

	WORD sHead;		//固定为，0xFEFF							2
	DWORD nLength;	//包长度 从控制命令开始，到和检验结束		4
	WORD sCmd;		//控制命令								2
	std::string strData;	//包数据							
	WORD sSum;		//和校验	 校验包数据						2
	std::string strOut;
};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = -1;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击0，双击1，按下2，放开3
	WORD nButton;	//左键0，右键1，中键2
	POINT ptXY;		//坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;         //是否有效
	BOOL IsDirectory;       //是否为目录 0否 1是 -1无效（默认）
	BOOL HasNext;           //是否还有后续 0没有 1有（默认）
	char szFileName[256];   //文件名 
}FILEINFO, * PFILEINFO;
