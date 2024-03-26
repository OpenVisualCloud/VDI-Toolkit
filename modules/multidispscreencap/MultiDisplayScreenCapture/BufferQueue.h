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

#include "CommonTypes.h"
#include "framework.h"

#include <queue>
#include <mutex>

class BufferQueue {
public:
    MDSCLIB_API BufferQueue();
    MDSCLIB_API ~BufferQueue() = default;
    MDSCLIB_API bool EnqueueBuffer(FRAME_DATA frame_data);
    MDSCLIB_API FRAME_DATA DequeueBuffer();
    MDSCLIB_API FRAME_DATA AcquireBuffer();
    MDSCLIB_API int GetSize() { return m_Size; }
    MDSCLIB_API int GetMaxSize() { return m_maxSize; }
    MDSCLIB_API bool SetMaxSize(int psize);

private:
    int m_Size;
    int m_maxSize;
    std::queue<FRAME_DATA> m_SourceQueue;
    std::mutex queue_mutex_;
};

#endif