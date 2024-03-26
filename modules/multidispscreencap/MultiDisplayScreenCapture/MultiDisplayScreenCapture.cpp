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

MultiDisplayScreenCapture::MultiDisplayScreenCapture() : m_UnexpectedErrorEvent(nullptr),
                                                         m_ExpectedErrorEvent(nullptr),
                                                         m_TerminateThreadsEvent(nullptr)
{

}

MultiDisplayScreenCapture::~MultiDisplayScreenCapture()
{
    // Clean up
    CloseHandle(m_UnexpectedErrorEvent);
    CloseHandle(m_ExpectedErrorEvent);
    CloseHandle(m_TerminateThreadsEvent);
}

DUPL_RETURN MultiDisplayScreenCapture::Init()
{
    DUPL_RETURN Ret = m_ThreadMgr.Initialize();
    return Ret;
}

DUPL_RETURN MultiDisplayScreenCapture::SetSingleDisplay(int SingleDispNumber)
{
    DUPL_RETURN Ret = m_ThreadMgr.SetSingleOuput(true, SingleDispNumber);
    return Ret;
}

UINT MultiDisplayScreenCapture::GetDisplayCount()
{
	return m_ThreadMgr.GetOutputCount();
}

UINT MultiDisplayScreenCapture::GetOutputCount()
{
    return m_ThreadMgr.GetThreadCount();
}

BufferQueue* MultiDisplayScreenCapture::GetBufferQueues()
{
    return m_ThreadMgr.GetBufferQueues();
}


DUPL_RETURN MultiDisplayScreenCapture::StartCaptureScreen()
{
    // Event used by the threads to signal an unexpected error and we want to quit the app
    m_UnexpectedErrorEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_UnexpectedErrorEvent)
    {
        ProcessFailure(nullptr, L"UnexpectedErrorEvent creation failed", L"Error", E_UNEXPECTED);
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Event for when a thread encounters an expected error
    m_ExpectedErrorEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_ExpectedErrorEvent)
    {
        ProcessFailure(nullptr, L"ExpectedErrorEvent creation failed", L"Error", E_UNEXPECTED);
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Event to tell spawned threads to quit
    m_TerminateThreadsEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_TerminateThreadsEvent)
    {
        ProcessFailure(nullptr, L"TerminateThreadsEvent creation failed", L"Error", E_UNEXPECTED);
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    RECT DeskBounds;
    UINT OutputCount = 0;
    DUPL_RETURN Ret = m_ThreadMgr.Process(m_UnexpectedErrorEvent, m_ExpectedErrorEvent, m_TerminateThreadsEvent);
    if (Ret != DUPL_RETURN_SUCCESS)
    {
        ProcessFailure(nullptr, L"Thread Process failed", L"Error", E_UNEXPECTED);
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    return DUPL_RETURN_SUCCESS;
}

DUPL_RETURN MultiDisplayScreenCapture::TerminateCaptureScreen()
{
    // Make sure all other threads have exited
    if (SetEvent(m_TerminateThreadsEvent))
    {
        m_ThreadMgr.WaitForThreadTermination();
    }
    else
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }
    return DUPL_RETURN_SUCCESS;
}

DUPL_RETURN MultiDisplayScreenCapture::DeInit()
{
    TerminateCaptureScreen();
	m_ThreadMgr.Clean();
    return DUPL_RETURN_SUCCESS;
}

DX_RESOURCES* MultiDisplayScreenCapture::GetDXResource(int thread)
{
    return m_ThreadMgr.GetDXResource(thread);
}

DUPL_RETURN MultiDisplayScreenCapture::SetCaptureFps(UINT fps)
{
    return m_ThreadMgr.SetCaptureFps(fps);
}

UINT MultiDisplayScreenCapture::GetCaptureFps()
{
    return m_ThreadMgr.GetCaptureFps();
}