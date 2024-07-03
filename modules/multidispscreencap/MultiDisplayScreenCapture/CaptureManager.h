/*
 * Copyright (c) 2024, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 *
 */

#ifndef _CAPTUREMANAGER_H_
#define _CAPTUREMANAGER_H_

#include "MDSCTypes.h"
#include "CaptureManager.h"

class CaptureManager
{
    public:
        CaptureManager();
        ~CaptureManager();
        SCREENCAP_STATUS Initialize(ID3D11Device *Device, ID3D11DeviceContext *Context, UINT Output);
        SCREENCAP_STATUS CaptureScreen(CapturedData *DataCaptured, UINT TimeOutInMs);
        SCREENCAP_STATUS Release();
        void GetOutputDesc(DXGI_OUTPUT_DESC* DescPtr);

    private:
        ID3D11Device *m_pDevice;
        ID3D11DeviceContext *m_pContext;
        IDXGIOutputDuplication* m_pDXGIDupl;
        ID3D11Texture2D* m_pCapturedTexture;
        UINT m_uScreenNumber;
        DXGI_OUTPUT_DESC m_sOutputDesc;
};

#endif
