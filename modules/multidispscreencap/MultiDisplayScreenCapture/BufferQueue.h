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

#ifndef _BUFFERQUEUE_H_
#define _BUFFERQUEUE_H_

#include "MDSCTypes.h"

#include <queue>
#include <mutex>

class BufferQueue {
public:
    MDSCLIB_API BufferQueue();
    MDSCLIB_API ~BufferQueue();
    MDSCLIB_API bool EnqueueBuffer(CapturedData Data);
    MDSCLIB_API CapturedData DequeueBuffer();
    MDSCLIB_API CapturedData AcquireBuffer();
    MDSCLIB_API int GetSize() { return m_nSize; }
    MDSCLIB_API int GetMaxSize() { return m_nMaxSize; }
    MDSCLIB_API bool SetMaxSize(int psize);
    MDSCLIB_API bool CleanBuffer();
    MDSCLIB_API int GetEnqueueSize();
    MDSCLIB_API int GetDropSize();

private:
    int m_nSize;
    int m_nMaxSize;
    int m_nEnqueueSize;
    int m_nDropSize;
    std::queue<CapturedData> m_qSourceQueue;
    std::mutex m_mQueueMutex;
};

#endif