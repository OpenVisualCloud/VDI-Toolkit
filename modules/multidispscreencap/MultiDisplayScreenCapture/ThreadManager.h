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

#ifndef _THREADMANAGER_H_
#define _THREADMANAGER_H_

#include <stdio.h>

#include "CommonTypes.h"
#include "framework.h"
#include "warning.h"
#include "BufferQueue.h"

class MDSCLIB_API THREADMANAGER
{
    public:
        THREADMANAGER();
        ~THREADMANAGER();
        void Clean();
        DUPL_RETURN Initialize();
        DUPL_RETURN SetSingleOuput(bool SingleOutput = false , UINT SingleOutNumber = 0);
        DUPL_RETURN Process(HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent);
        PTR_INFO* GetPointerInfo();
        void WaitForThreadTermination();
        UINT GetOutputCount();
        UINT GetThreadCount();
        BufferQueue* GetBufferQueues();
        DX_RESOURCES* GetDXResource(int thread);
        DUPL_RETURN SetCaptureFps(UINT fps);
        UINT GetCaptureFps();

    private:
        DUPL_RETURN InitializeDx(_Out_ DX_RESOURCES* Data);
        DUPL_RETURN GetAdapterCount(_In_ ID3D11Device* Device);
        void CleanDx(_Inout_ DX_RESOURCES* Data);

        PTR_INFO m_PtrInfo;

        bool m_SingleOutput;
        INT m_SingleOutNumber;
        UINT m_OutputCount;
        UINT m_ThreadCount;
        _Field_size_(m_ThreadCount) HANDLE* m_ThreadHandles;
        _Field_size_(m_ThreadCount) THREAD_DATA* m_ThreadData;
        _Field_size_(m_ThreadCount) BufferQueue* m_BufferQueues;

        UINT m_CaptureFps;
};

#endif
