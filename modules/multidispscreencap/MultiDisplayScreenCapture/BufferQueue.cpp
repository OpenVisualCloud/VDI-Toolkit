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

#include "BufferQueue.h"

//
// Constructor
//
BufferQueue::BufferQueue()
{
    m_nSize = 0;
    m_nMaxSize = 6;
    m_nEnqueueSize = 0;
    m_nDropSize = 0;
    std::unique_lock<std::mutex> lock(m_mQueueMutex);
}

//
// Destructor
//
BufferQueue::~BufferQueue()
{
    CleanBuffer();
}

//
// Enqueue Buffer
//
bool BufferQueue::EnqueueBuffer(CapturedData Data)
{
    std::unique_lock<std::mutex> lock(m_mQueueMutex);
    m_qSourceQueue.push(Data);
    m_nSize += 1;
    m_nEnqueueSize += 1;
    if (m_nSize > m_nMaxSize) {
        CapturedData data = m_qSourceQueue.front();
        if (data.CapturedTexture)
        {
            data.CapturedTexture->Release();
            data.CapturedTexture = nullptr;
        }
        m_qSourceQueue.pop();
        m_nSize -= 1;
        m_nDropSize += 1;
    }
    return true;
}

//
// Set Buffer Queue Max Size
//
bool BufferQueue::SetMaxSize(int psize)
{
    m_nMaxSize = psize;
    return true;
}

//
// Dequeue Buffer
//
CapturedData BufferQueue::DequeueBuffer()
{
    std::unique_lock<std::mutex> lock(m_mQueueMutex);
    CapturedData Data{};
    if (!m_qSourceQueue.empty()) {
        Data = m_qSourceQueue.front();
        if (m_nSize >= 1) {
            m_qSourceQueue.pop();
            m_nSize -= 1;
        }
    }
    return Data;
}

//
// Acquire Buffer
//
CapturedData BufferQueue::AcquireBuffer()
{
    std::unique_lock<std::mutex> lock(m_mQueueMutex);
    CapturedData Data{};
    if (!m_qSourceQueue.empty()) {
        Data = m_qSourceQueue.front();
    }
    return Data;
}

//
// Clean Buffer
//
bool BufferQueue::CleanBuffer()
{
    HRESULT hr = S_OK;
    bool res = true;
    while (m_nSize > 0)
    {
        CapturedData Data = DequeueBuffer();
        if (Data.CapturedTexture)
        {
            hr = Data.CapturedTexture->Release();
            Data.CapturedTexture = nullptr;
            if (FAILED(hr))
            {
                res = false;
            }
        }
    }
    return res;
}

//
// Get Enqueue Size
//
int BufferQueue::GetEnqueueSize()
{
    return m_nEnqueueSize;
}

//
// Get Drop Size
//
int BufferQueue::GetDropSize()
{
    return m_nDropSize;
}
