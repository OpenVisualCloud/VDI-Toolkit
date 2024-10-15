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
//! \file HostEncodeService.cpp
//! \brief
//! \date 2024-06-27
//!

#include "HostEncodeService.h"

VDI_NS_BEGIN

HostEncodeService::HostEncodeService()
{
    m_isStop = false;
    m_isEOS = false;
    m_frameNum = 0;
    m_inFrameBufferDataList.clear();
    m_outFrameBufferDataList.clear();
}

HostEncodeService::~HostEncodeService()
{
    if (m_inShmMem != MAP_FAILED) munmap(m_inShmMem, m_inShmSize);
    if (m_outShmMem != MAP_FAILED) munmap(m_outShmMem, m_outShmSize);
    if (m_inShmFile >= 0) close(m_inShmFile);
    if (m_outShmFile >= 0) close(m_outShmFile);
    m_inFrameBufferDataList.clear();
    m_outFrameBufferDataList.clear();
}

MRDAStatus HostEncodeService::SetInitParams(MediaParams *params)
{
    if (params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_mediaParams = std::make_unique<MediaParams>();
    m_mediaParams->encodeParams = params->encodeParams;
    m_mediaParams->shareMemoryInfo = params->shareMemoryInfo;

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostEncodeService::InitShm()
{
    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    ShareMemoryInfo shmInfo = m_mediaParams->shareMemoryInfo;
    std::string in_mem_path = shmInfo.in_mem_dev_path;
    std::string out_mem_path = shmInfo.out_mem_dev_path;
    if (MRDA_STATUS_SUCCESS != GetInShmFilePtr(in_mem_path))
    {
        MRDA_LOG(LOG_ERROR, "Failed to get in shm file ptr!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    if (MRDA_STATUS_SUCCESS != GetOutShmFilePtr(out_mem_path))
    {
        MRDA_LOG(LOG_ERROR, "Failed to get out shm file ptr!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostEncodeService::SendInputData(std::shared_ptr<FrameBufferData> data)
{
    std::unique_lock<std::mutex> lock(m_inMutex);
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input data is null!");
        return MRDA_STATUS_INVALID_DATA;
    }
#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: push back frame in host encoding service input queue, pts: %lu, in dev path: %s", data->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    m_inFrameBufferDataList.push_back(data);
    // MRDA_LOG(LOG_INFO, "Input frame data size is %d", m_inFrameBufferDataList.size());
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostEncodeService::ReceiveOutputData(std::shared_ptr<FrameBufferData> &data)
{
    std::unique_lock<std::mutex> lock(m_outMutex);
    if (m_outFrameBufferDataList.empty())
    {
        // MRDA_LOG(LOG_INFO, "Output data list is empty!");
        return MRDA_STATUS_NOT_ENOUGH_DATA;
    }
    data = m_outFrameBufferDataList.front();
    m_outFrameBufferDataList.pop_front();
#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: pop front frame in host encoding service output queue, pts: %lu, in dev path: %s", data->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    // MRDA_LOG(LOG_INFO, "Output frame data size is %d, pts %llu", m_outFrameBufferDataList.size(), data->Pts());
    return MRDA_STATUS_SUCCESS;
}



MRDAStatus HostEncodeService::GetAvailableOutputBufferFrame(std::shared_ptr<FrameBufferData>& pFrame)
{
    // check an available buffer
    bool isBufferAvailable = false;
    while (!isBufferAvailable)
    {
        if (MRDA_STATUS_SUCCESS != GetAvailBuffer(pFrame))
        {
            // MRDA_LOG(LOG_INFO, "GetAvailBuffer failed\n");
            usleep(5 * 1000);
            continue;
        }
        isBufferAvailable = true;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostEncodeService::GetAvailBuffer(std::shared_ptr<FrameBufferData>& pFrame)
{
    if (m_mediaParams == nullptr || m_outShmMem == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params or out shm mem invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }
    uint32_t bufferNum = m_mediaParams->shareMemoryInfo.bufferNum;
    uint64_t bufferSize = m_mediaParams->shareMemoryInfo.bufferSize;
    for (uint32_t i = 1; i <= bufferNum; i++)
    {
        size_t state_offset = (i - 1) * bufferSize;
        BufferState state = BufferState::BUFFER_STATE_NONE;
        memcpy(&state, m_outShmMem + state_offset, sizeof(uint32_t));
        // find an available frame
        if (state == BufferState::BUFFER_STATE_IDLE)
        {
            std::shared_ptr<MemoryBuffer> memBuffer = std::make_shared<MemoryBuffer>();
            memBuffer->SetBufId(i);
            memBuffer->SetMemOffset(state_offset + sizeof(uint32_t));
            memBuffer->SetStateOffset(state_offset);
            memBuffer->SetBufPtr(nullptr);
            memBuffer->SetSize(bufferSize);
            memBuffer->SetOccupiedSize(0);
            memBuffer->SetState(state);
            pFrame = std::make_shared<FrameBufferData>();
            pFrame->SetWidth(m_mediaParams->encodeParams.frame_width);
            pFrame->SetHeight(m_mediaParams->encodeParams.frame_height);
            pFrame->SetStreamType(InputStreamType::ENCODED);
            pFrame->SetPts(m_frameNum);
            pFrame->SetEOS(m_isEOS);
            pFrame->SetMemBuffer(memBuffer);
            return MRDA_STATUS_SUCCESS;
        }
    }
    return MRDA_STATUS_INVALID_DATA;
}

VDI_NS_END
