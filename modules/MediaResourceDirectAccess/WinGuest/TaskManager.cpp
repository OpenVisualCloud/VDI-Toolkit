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
//! \file TaskManager.cpp
//! \brief implement task manager
//! \date 2024-04-07
//!

#include "TaskManager.h"

VDI_NS_BEGIN

TaskManager::TaskManager():
    m_taskInfo(nullptr),
    m_encodeParams(nullptr),
    m_shareMemInfo(nullptr),
    m_inMemoryPool(nullptr),
    m_outMemoryPool(nullptr),
    m_taskManagerSession(nullptr),
    m_dataSender(nullptr),
    m_dataReceiver(nullptr),
    m_taskDataSession(nullptr)
{

}

TaskManager::~TaskManager()
{

}

MRDAStatus TaskManager::Initialize(const TaskInfo *taskInfo, const ExternalConfig *config)
{
    // 1. init task manager session AND get task info
    std::string taskMgrIPaddr = config->hostSessionAddr;
    if (taskMgrIPaddr.empty())
    {
        MRDA_LOG(LOG_ERROR, "invalid host session address!");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_taskManagerSession = std::make_shared<TaskManagerSession_gRPC>(taskMgrIPaddr);
    if (m_taskManagerSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create task manager session");
        return MRDA_STATUS_INVALID_DATA;
    }

    TaskInfo updatedTaskInfo;
    if (MRDA_STATUS_SUCCESS != m_taskManagerSession->StartTask(taskInfo, &updatedTaskInfo))
    {
        MRDA_LOG(LOG_ERROR, "failed to initialize task manager session");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_taskInfo = std::make_shared<TaskInfo>(updatedTaskInfo);

    // 2. init data sender with given task data session
    m_taskDataSession = std::make_shared<TaskDataSession_gRPC>(m_taskInfo);
    if (m_taskDataSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create task data session");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_dataSender = std::make_shared<DataSender>(m_taskDataSession);
    if (m_dataSender == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create data sender");
        return MRDA_STATUS_INVALID_DATA;
    }

    // 3. init data receiver with given task data session
    m_dataReceiver = std::make_shared<DataReceiver>(m_taskDataSession);
    if (m_dataReceiver == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create data receiver");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManager::SetInitParams(const MediaParams *params)
{
    if (params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid media params");
        return MRDA_STATUS_INVALID_DATA;
    }

    m_encodeParams = std::make_shared<EncodeParams>(params->encodeParams);
    m_shareMemInfo = std::make_shared<ShareMemoryInfo>(params->shareMemoryInfo);
    if (m_encodeParams == nullptr || m_shareMemInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to set encode params or share memory info!");
        return MRDA_STATUS_INVALID_DATA;
    }
    // 1. init AND allocate in/out memory pool
    m_inMemoryPool = std::make_shared<FrameMemoryPool>();
    if (m_inMemoryPool == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create in memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != m_inMemoryPool->InitBufferPool(m_shareMemInfo->bufferNum, m_shareMemInfo->bufferSize, m_shareMemInfo->in_mem_dev_slot_number))
    {
        MRDA_LOG(LOG_ERROR, "failed to create in memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != m_inMemoryPool->AllocateBufferPool())
    {
        MRDA_LOG(LOG_ERROR, "failed to allocate in memory pool");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_outMemoryPool = std::make_shared<FrameMemoryPool>();
    if (m_outMemoryPool == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create out memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != m_outMemoryPool->InitBufferPool(m_shareMemInfo->bufferNum, m_shareMemInfo->bufferSize,m_shareMemInfo->out_mem_dev_slot_number))
    {
        MRDA_LOG(LOG_ERROR, "failed to create out memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != m_outMemoryPool->AllocateBufferPool())
    {
        MRDA_LOG(LOG_ERROR, "failed to allocate out memory pool");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // 2. send init params to data sender
    if (MRDA_STATUS_SUCCESS != m_dataSender->SetInitParams(params))
    {
        MRDA_LOG(LOG_ERROR, "failed to set init params data");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManager::SendFrame(const std::shared_ptr<FrameBufferData> data)
{
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input frame data");
        return MRDA_STATUS_INVALID_DATA;
    }

    // send input frame data to data sender
    if (MRDA_STATUS_SUCCESS != m_dataSender->SendFrame(data))
    {
        MRDA_LOG(LOG_ERROR, "failed to send input frame data");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManager::ReceiveFrame(std::shared_ptr<FrameBufferData> &data)
{
    // receive output frame data from data receiver
    MRDAStatus status = m_dataReceiver->ReceiveFrame(data);

    // get buffer data ptr according to buffer id
    if (status == MRDA_STATUS_SUCCESS)
    {
        if (data == nullptr) {
            MRDA_LOG(LOG_ERROR, "Receive frame is empty!");
            return MRDA_STATUS_INVALID_DATA;
        }
        GetBufferFromId(data->MemBuffer()->BufId(), data);
        return status;
    }

    return status;
}

MRDAStatus TaskManager::GetOneInputBuffer(std::shared_ptr<FrameBufferData> &data)
{
    if (m_inMemoryPool == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    // get one idle buffer from in memory pool
    std::shared_ptr<FrameBufferData> buffer = nullptr;
    if (MRDA_STATUS_SUCCESS != m_inMemoryPool->GetBuffer(buffer))
    {
        MRDA_LOG(LOG_ERROR, "failed to get one input buffer");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (m_encodeParams == nullptr) return MRDA_STATUS_INVALID_DATA;
    if (m_taskInfo != nullptr &&
        (m_taskInfo->taskType == TASKTYPE::taskFFmpegEncode
        || m_taskInfo->taskType == TASKTYPE::taskOneVPLEncode)
       )
        buffer->SetStreamType(InputStreamType::RAW);
    else
        buffer->SetStreamType(InputStreamType::UNKNOWN);
    buffer->SetWidth(m_encodeParams->frame_width);
    buffer->SetHeight(m_encodeParams->frame_height);
    buffer->SetPts(0); // default

    data = buffer;

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManager::ReleaseOneOutputBuffer(std::shared_ptr<FrameBufferData> data)
{
    if (m_outMemoryPool == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid output memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    // release one buffer in memory pool
    if (MRDA_STATUS_SUCCESS != m_outMemoryPool->ReleaseBuffer(data))
    {
        MRDA_LOG(LOG_ERROR, "failed to release one input buffer");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManager::GetBufferFromId(uint32_t id, std::shared_ptr<FrameBufferData>& data)
{
    if (m_outMemoryPool == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid output memory pool");
        return MRDA_STATUS_INVALID_DATA;
    }

    // get one buffer in memory pool
    if (MRDA_STATUS_SUCCESS != m_outMemoryPool->GetBufferFromId(id, data))
    {
        MRDA_LOG(LOG_ERROR, "failed to get one output buffer from id");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}


MRDAStatus TaskManager::StopTask()
{
    if (m_taskManagerSession == nullptr || m_taskInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid task manager session or task info");
        return MRDA_STATUS_INVALID_DATA;
    }

    TASKStatus status;
    return m_taskManagerSession->StopTask(m_taskInfo.get(), &status);
}

MRDAStatus TaskManager::ResetTask(const TaskInfo *taskInfo)
{
    if (m_taskManagerSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid task manager session");
        return MRDA_STATUS_INVALID_DATA;
    }

    TASKStatus status;
    return m_taskManagerSession->ResetTask(taskInfo, &status);
}

MRDAStatus TaskManager::Destroy()
{
    if (m_inMemoryPool != nullptr)
    {
        m_inMemoryPool->DestroyBufferPool();
    }
    if (m_outMemoryPool != nullptr)
    {
        m_outMemoryPool->DestroyBufferPool();
    }

    return MRDA_STATUS_SUCCESS;
}

VDI_NS_END