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

#include "ScreenCapture.h"

//
// Entry point for new screen Capture
//
DWORD WINAPI CaptureProc(void* Param)
{
    ThreadInputParams* TData = reinterpret_cast<ThreadInputParams*>(Param);

    BufferQueue* PtrBufferQueue = (BufferQueue*)TData->BufferQueueHandle;

    int CapturedCount = 0;

    SCREENCAP_STATUS Ret;

    CaptureManager CapMgr;
    Ret = CapMgr.Initialize(TData->DxRes.Device, TData->DxRes.Context, TData->ScreenNumber);
    if (Ret != SCREENCAP_SUCCESSED)
    {
        goto Exit;
    }

    // Get output description
    DXGI_OUTPUT_DESC DesktopDesc;
    RtlZeroMemory(&DesktopDesc, sizeof(DXGI_OUTPUT_DESC));
    CapMgr.GetOutputDesc(&DesktopDesc);

    // Main duplication loop
    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> starttp = std::chrono::high_resolution_clock::now();

        CapturedData Data;
        Ret = CapMgr.CaptureScreen(&Data, 1000.0/TData->CaptureFps);
        if (Ret == SCREENCAP_CONTINUED)
        {
            continue;
        }

        if (Ret == SCREENCAP_FAILED)
        {
            printf("[Thread][%d], Error: Screen capture result failure, captured break with totall num %d\n", TData->ScreenNumber, CapturedCount);
            break;
        }

        CapturedCount++;

        Ret = CapMgr.Release();
        if (Ret != SCREENCAP_SUCCESSED)
        {
            printf("[Thread][%d], Error: Capture manager release failed, frame %d\n", TData->ScreenNumber, CapturedCount);
            break;
        }

        PtrBufferQueue->EnqueueBuffer(Data);

        std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
        uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();

        //printf("thread %d, dequeue frame %d, Data texture ptr %p, AcquiredTime %u, encodeframe costtime %dus\n", TData->ThreadId, num, Data->CapturedTexture, Data->AcquiredTime, timecost);

        int CaptureInterval = TData->CaptureFps != 0 ? 1000000 / TData->CaptureFps : 0;
        if (timecost < CaptureInterval)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(CaptureInterval - timecost));
        }
    }

Exit:

    PtrBufferQueue->CleanBuffer();
    if (Ret != SCREENCAP_SUCCESSED)
    {
        SetEvent(TData->TerminateThreadsEvent);
    }

    return 0;
}