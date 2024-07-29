#pragma once
#include <string>
#include "pch.h"
#include <Windows.h>
#include <atlimage.h>

class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i % 15 == 0) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
	}

    static int Byte2Image(CImage& image, const std::string& strBuffer) {
        BYTE* data = (BYTE*)strBuffer.c_str();
        HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMen == NULL) {
            return -1;
        }

        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
        if (ret == S_OK) {
            ULONG length = 0;
            pStream->Write(data, strBuffer.size(), &length);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);

            if (!image.IsNull()) {
                image.Destroy();
            }

            image.Load(pStream);
        }
        pStream->Release();

        return 0;
    }
};

