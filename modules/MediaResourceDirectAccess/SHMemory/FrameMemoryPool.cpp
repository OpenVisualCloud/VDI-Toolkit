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

 */

//!
//! \file MemoryPool.cpp
//! \brief implement a memory pool to manage the inter-VM share memory
//!        between host and VMs.
//! \date 2024-04-02
//!

#include "FrameMemoryPool.h"

#include <cstring>

VDI_NS_BEGIN

MRDAStatus FrameMemoryPool::AllocateBufferPool()
{
    if (m_bufferPoolCount == 0)
    {
        MRDA_LOG(LOG_ERROR, "invalid buffer pool count!");
        return MRDA_STATUS_INVALID_DATA;
    }
    if (m_bufferSize == 0)
    {
        MRDA_LOG(LOG_ERROR, "invalid buffer size!");
        return MRDA_STATUS_INVALID_DATA;
    }
    // allocate buffer pool
    for (uint32_t i = 1; i <= m_bufferPoolCount; i++)
    {
        std::shared_ptr<MemoryBuffer> buffer = std::make_shared<MemoryBuffer>();
        buffer->SetBufId(i);
        buffer->SetStateOffset((uint64_t)(i - 1) * m_bufferSize);
        buffer->SetMemOffset(buffer->StateOffset() + sizeof(uint32_t));
        buffer->SetBufPtr(static_cast<uint8_t*>(m_shareMemPtr) + (uint64_t)(i - 1) * m_bufferSize);
        buffer->SetSize(m_bufferSize);
        buffer->SetState(BufferState::BUFFER_STATE_IDLE);
        BufferState state = buffer->State();
        memcpy(buffer->BufPtr(), &state, sizeof(BufferState));

        std::shared_ptr<FrameBufferData> bufData = std::make_shared<FrameBufferData>();
        bufData->SetMemBuffer(buffer);
        // external settings
        bufData->SetWidth(0);
        bufData->SetHeight(0);
        bufData->SetStreamType(InputStreamType::UNKNOWN);
        bufData->SetPts(0);
        m_bufferPool.push_back(bufData);
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus FrameMemoryPool::GetBuffer(std::shared_ptr<FrameBufferData> &buffer)
{
    std::lock_guard<std::mutex> lock(m_bufferPoolMutex);
    auto it = m_bufferPool.begin();
    for (; it != m_bufferPool.end(); it++)
    {
        std::shared_ptr<MemoryBuffer> memBuf = (*it)->MemBuffer();
        if (memBuf == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "invalid mem buffer!");
            return MRDA_STATUS_INVALID_DATA;
        }
        // aquire buffer state
        BufferState state = BufferState::BUFFER_STATE_NONE;
        memcpy(&state, memBuf->BufPtr(), sizeof(BufferState));
        if (state == BufferState::BUFFER_STATE_IDLE)
        {
            memBuf->SetState(BufferState::BUFFER_STATE_BUSY);
            // write state to buffer
            memcpy(memBuf->BufPtr(), &state, sizeof(BufferState));
            buffer = *it;
            break;
        }
    }
    if (it == m_bufferPool.end())
    {
        MRDA_LOG(LOG_WARNING, "No idle buffer in buffer pool!");
        return MRDA_STATUS_INVALID_DATA;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus FrameMemoryPool::ReleaseBuffer(std::shared_ptr<FrameBufferData> buffer)
{
    std::lock_guard<std::mutex> lock(m_bufferPoolMutex);
    auto it = m_bufferPool.begin();
    for (; it != m_bufferPool.end(); it++)
    {
        std::shared_ptr<MemoryBuffer> iterMemBuf = (*it)->MemBuffer();
        std::shared_ptr<MemoryBuffer> inputMemBuf = buffer->MemBuffer();
        uint32_t iterId = iterMemBuf->BufId();
        uint32_t inputId = inputMemBuf->BufId();
        if (iterId == inputId) // find the buffer
        {
            iterMemBuf->SetState(BufferState::BUFFER_STATE_IDLE);
            // write state to buffer
            BufferState state = BufferState::BUFFER_STATE_IDLE;
            memcpy(iterMemBuf->BufPtr(), &state, sizeof(BufferState));
            break;
        }
    }
    if (it == m_bufferPool.end())
    {
        MRDA_LOG(LOG_ERROR, "No buffer in buffer pool!");
        return MRDA_STATUS_INVALID;
    }
    else return MRDA_STATUS_SUCCESS;
}

MRDAStatus FrameMemoryPool::GetBufferFromId(uint32_t id, std::shared_ptr<FrameBufferData>& buffer)
{
    std::lock_guard<std::mutex> lock(m_bufferPoolMutex);
    auto it = m_bufferPool.begin();
    for (; it != m_bufferPool.end(); it++)
    {
        std::shared_ptr<MemoryBuffer> memBuf = (*it)->MemBuffer();
        if (memBuf == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "invalid mem buffer!");
            return MRDA_STATUS_INVALID_DATA;
        }
        uint32_t buf_id = memBuf->BufId();
        if (buf_id == id)
        {
            buffer->MemBuffer()->SetBufPtr(memBuf->BufPtr());
            break;
        }
    }
    if (it == m_bufferPool.end())
    {
        MRDA_LOG(LOG_WARNING, "No request buffer in buffer pool!");
        return MRDA_STATUS_INVALID_DATA;
    }
    return MRDA_STATUS_SUCCESS;
}

VDI_NS_END