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

BufferQueue::BufferQueue() {
    m_Size = 0;
    m_maxSize = 6;
    std::unique_lock<std::mutex> lock(queue_mutex_);
}

bool BufferQueue::EnqueueBuffer(FRAME_DATA frame_data) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    m_SourceQueue.push(frame_data);
    m_Size += 1;
    if (m_Size > m_maxSize) {
        m_SourceQueue.pop();
        m_Size -= 1;
    }
    return true;
}

bool BufferQueue::SetMaxSize(int psize) {
    m_maxSize = psize;
    return true;
}

FRAME_DATA BufferQueue::DequeueBuffer() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    FRAME_DATA FrameData{};
    if (!m_SourceQueue.empty()) {
        FrameData = m_SourceQueue.front();
        if (m_Size >= 1) {
            m_SourceQueue.pop();
            m_Size -= 1;
        }
    }
    return FrameData;
}

FRAME_DATA BufferQueue::AcquireBuffer() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    FRAME_DATA FrameData{};
    if (!m_SourceQueue.empty()) {
        FrameData = m_SourceQueue.front();
    }
    return FrameData;
}