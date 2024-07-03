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

#include "MultiDisplayScreenCapture.h"

//
// Constructor
//
MultiDisplayScreenCapture::MultiDisplayScreenCapture() : m_hTerminateCaptureEvent(nullptr)
{

}

//
// Destructor
//
MultiDisplayScreenCapture::~MultiDisplayScreenCapture()
{
    CloseHandle(m_hTerminateCaptureEvent);
}

//
// Init
//
SCREENCAP_STATUS MultiDisplayScreenCapture::Init()
{
    SCREENCAP_STATUS Ret = m_cScreenMgr.Initialize();
    return Ret;
}

//
// Set Single Display
//
SCREENCAP_STATUS MultiDisplayScreenCapture::SetSingleDisplay(int SingleDispNumber)
{
    SCREENCAP_STATUS Ret = m_cScreenMgr.SetSingleOuput(true, SingleDispNumber);
    return Ret;
}

//
// Check Single Display
//
UINT MultiDisplayScreenCapture::GetDisplayCount()
{
	return m_cScreenMgr.GetOutputCount();
}

//
// Check Captured Screen Count
//
UINT MultiDisplayScreenCapture::GetOutputCount()
{
    return m_cScreenMgr.GetScreenCount();
}

//
// Get Buffer Queue
//
BufferQueue* MultiDisplayScreenCapture::GetBufferQueues()
{
    return m_cScreenMgr.GetBufferQueues();
}

//
// Start Capture Screen
//
SCREENCAP_STATUS MultiDisplayScreenCapture::StartCaptureScreen()
{
    m_hTerminateCaptureEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_hTerminateCaptureEvent)
    {
        return SCREENCAP_FAILED;
    }

    RECT DeskBounds;
    UINT OutputCount = 0;
    SCREENCAP_STATUS Ret = m_cScreenMgr.Process(m_hTerminateCaptureEvent);
    if (Ret != SCREENCAP_SUCCESSED)
    {
        return SCREENCAP_FAILED;
    }

    return SCREENCAP_SUCCESSED;
}

//
// Terminate Capture Screen
//
SCREENCAP_STATUS MultiDisplayScreenCapture::TerminateCaptureScreen()
{
    // Make sure all other threads have exited
    if (SetEvent(m_hTerminateCaptureEvent))
    {
        m_cScreenMgr.WaitForThreadTermination();
    }
    else
    {
        return SCREENCAP_FAILED;
    }
    return SCREENCAP_SUCCESSED;
}

//
// DeInit
//
SCREENCAP_STATUS MultiDisplayScreenCapture::DeInit()
{
    TerminateCaptureScreen();
	m_cScreenMgr.Clean();
    return SCREENCAP_SUCCESSED;
}


//
// Get DX Resources according to DisplayNumber
//
DX_RESOURCES *MultiDisplayScreenCapture::GetDXResource(UINT DisplayNumber) {
    return m_cScreenMgr.GetDXResource(DisplayNumber);
}

//
// Set Capture FPS
//
SCREENCAP_STATUS MultiDisplayScreenCapture::SetCaptureFps(UINT fps)
{
    return m_cScreenMgr.SetCaptureFps(fps);
}

//
// Get Capture FPS
//
UINT MultiDisplayScreenCapture::GetCaptureFps()
{
    return m_cScreenMgr.GetCaptureFps();
}

//
// Check if Capture Terminated
//
bool MultiDisplayScreenCapture::CheckIfCaptureTerminated()
{
    return m_cScreenMgr.IsCaptureTerminated();
}