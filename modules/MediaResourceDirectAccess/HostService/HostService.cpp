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
//! \file HostService.cpp
//! \brief Host service implementation
//! \date 2024-07-25
//!

#include "HostService.h"


VDI_NS_BEGIN

MRDAStatus HostService::GetInShmFilePtr(std::string filePath)
{
    if (filePath.empty())
    {
        MRDA_LOG(LOG_ERROR, "File path invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    std::ifstream file(filePath, std::ios::binary);

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    m_inShmSize = fileSize;
    file.seekg(0, std::ios::beg);

    file.close();

    if (m_mediaParams != nullptr && m_mediaParams->shareMemoryInfo.totalMemorySize > m_inShmSize)
    {
        MRDA_LOG(LOG_ERROR, "shm size invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    m_inShmFile = open(filePath.c_str(), O_RDWR);

    void* mapped_memory = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_inShmFile, 0);

    if (mapped_memory == MAP_FAILED) {
        MRDA_LOG(LOG_ERROR, "Failed to map the file into memory!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_inShmMem = (char*)mapped_memory;

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostService::GetOutShmFilePtr(std::string filePath)
{
    if (filePath.empty())
    {
        MRDA_LOG(LOG_ERROR, "File path invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    std::ifstream file(filePath, std::ios::binary);

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    m_outShmSize = fileSize;
    file.seekg(0, std::ios::beg);

    file.close();

    if (m_mediaParams != nullptr && m_mediaParams->shareMemoryInfo.totalMemorySize > m_outShmSize)
    {
        MRDA_LOG(LOG_ERROR, "shm size invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    m_outShmFile = open(filePath.c_str(), O_RDWR);

    void* mapped_memory = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_outShmFile, 0);

    if (mapped_memory == MAP_FAILED) {
        MRDA_LOG(LOG_ERROR, "Failed to map the file into memory!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_outShmMem = (char*)mapped_memory;

    return MRDA_STATUS_SUCCESS;
}

void HostService::RefOutputFrame(std::shared_ptr<FrameBufferData> pFrame)
{
    if (pFrame != nullptr && pFrame->MemBuffer() != nullptr)
    {
        pFrame->MemBuffer()->SetState(BufferState::BUFFER_STATE_BUSY);
        uint32_t state = static_cast<uint32_t>(pFrame->MemBuffer()->State());
        memcpy(m_outShmMem + pFrame->MemBuffer()->StateOffset(), &state, sizeof(uint32_t));
        // MRDA_LOG(LOG_INFO, "Ref output buffer at pts %llu, buffer id %d", pFrame->Pts(), pFrame->MemBuffer()->BufId());
    }
}

void HostService::UnRefInputFrame(std::shared_ptr<FrameBufferData> pFrame)
{
    if (pFrame != nullptr && pFrame->MemBuffer() != nullptr)
    {
        pFrame->MemBuffer()->SetState(BufferState::BUFFER_STATE_IDLE);
        uint32_t state = static_cast<uint32_t>(pFrame->MemBuffer()->State());
        memcpy(m_inShmMem + pFrame->MemBuffer()->StateOffset(), &state, sizeof(uint32_t));
        // MRDA_LOG(LOG_INFO, "UnRef input buffer at pts %llu, buffer id %d", pFrame->Pts(), pFrame->MemBuffer()->BufId());
    }
}

VDI_NS_END