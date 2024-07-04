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

#ifndef _SCREENMANAGER_H_
#define _SCREENMANAGER_H_

#include <stdio.h>

#include "MDSCTypes.h"
#include "warning.h"
#include "BufferQueue.h"

class MDSCLIB_API ScreenManager
{
    public:
        ScreenManager();
        ~ScreenManager();
        void Clean();
        SCREENCAP_STATUS Initialize();
        SCREENCAP_STATUS SetSingleOuput(bool SingleOutput = false , UINT SingleOutNumber = 0);
        SCREENCAP_STATUS Process(HANDLE TerminateThreadsEvent);
        void WaitForThreadTermination();
        UINT GetOutputCount();
        UINT GetScreenCount();
        BufferQueue* GetBufferQueues();
        DX_RESOURCES* GetDXResource(UINT DisplayNumber);
        SCREENCAP_STATUS SetCaptureFps(UINT fps);
        UINT GetCaptureFps();
        bool IsCaptureTerminated();

    private:
        SCREENCAP_STATUS InitDXResources(DX_RESOURCES *DxRes);
        SCREENCAP_STATUS GetAdapterCount(ID3D11Device *Device);
        void ReleaseDxResources(DX_RESOURCES *DxRes);


        bool m_bSingleOutput;
        INT m_bSingleOutNumber;
        UINT m_uOutputCount;
        UINT m_uScreenCount;
        _Field_size_(m_uScreenCount) HANDLE* m_pScreenHandles;
        _Field_size_(m_uScreenCount) ThreadInputParams* m_pScreenInputParams;
        _Field_size_(m_uScreenCount) BufferQueue* m_pBufferQueues;

        UINT m_CaptureFps;
};

#endif
