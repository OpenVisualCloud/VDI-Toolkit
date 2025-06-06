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
//! \file MediaTask.cpp
//! \brief implement media task
//! \date 2024-04-02
//!

#include "MediaTask.h"

VDI_NS_BEGIN

MediaTask::MediaTask()
    :m_taskManager(nullptr) {}

MRDAStatus MediaTask::Initialize(const TaskInfo *taskInfo, const ExternalConfig *config)
{
    // check input paramters
    if (taskInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "task info invalid!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    // create task manager
    m_taskManager = std::make_shared<TaskManager>();
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create task manager!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // initialize task manager
    if (m_taskManager->Initialize(taskInfo, config) != MRDA_STATUS_SUCCESS)
    {
        MRDA_LOG(LOG_ERROR, "Failed to initialize task manager!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus MediaTask::Stop()
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    return m_taskManager->StopTask();
}

MRDAStatus MediaTask::Reset(const TaskInfo *taskInfo)
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    return m_taskManager->ResetTask(taskInfo);
}

MRDAStatus MediaTask::Destroy()
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    return m_taskManager->Destroy();
}

MRDAStatus MediaTask::SetInitParams(const MediaParams *params)
{
    if (MRDA_STATUS_SUCCESS != CheckMediaParams(params))
    {
        MRDA_LOG(LOG_ERROR, "Invalid media parameters!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    return m_taskManager->SetInitParams(params);
}

MRDAStatus MediaTask::SendFrame(const std::shared_ptr<FrameBufferItem> data)
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }
    // FrameBufferItem -> FrameBufferData
    std::shared_ptr<FrameBufferData> frameData = std::make_shared<FrameBufferData>();
    if (frameData == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Create frame buffer data failed!");
        return MRDA_STATUS_INVALID_DATA;
    }
    frameData->CreateFrameBufferData(data.get());

    return m_taskManager->SendFrame(frameData);
}

MRDAStatus MediaTask::ReceiveFrame(std::shared_ptr<FrameBufferItem> &data)
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }
    // receive output frame
    std::shared_ptr<FrameBufferData> frameData = nullptr;
    MRDAStatus status = m_taskManager->ReceiveFrame(frameData);
    if (status == MRDA_STATUS_SUCCESS) {
        if (frameData == nullptr) {
            MRDA_LOG(LOG_ERROR, "output buffer is empty!");
            return MRDA_STATUS_INVALID_DATA;
        }
        // transfer FrameBufferData -> FrameBufferItem
        std::shared_ptr<MemoryBuffer> memBuf = frameData->MemBuffer();
        std::shared_ptr<MemBufferItem> item  = std::make_shared<MemBufferItem>();

        data = std::make_shared<FrameBufferItem>();

        item->assign(memBuf->BufId(), memBuf->MemOffset(), memBuf->StateOffset(),
                     memBuf->BufPtr(), memBuf->Size(), memBuf->OccupiedSize(),
                     memBuf->State());
        data->init(item.get(), frameData->Width(), frameData->Height(),
                   frameData->StreamType(), frameData->Pts(), frameData->IsEOS());
    }

    return status;
}

MRDAStatus MediaTask::GetOneInputBuffer(std::shared_ptr<FrameBufferItem> &data)
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }
    // get one input buffer from frame memory pool
    std::shared_ptr<FrameBufferData> frameData = nullptr;
    if (MRDA_STATUS_SUCCESS != m_taskManager->GetOneInputBuffer(frameData))
    {
        MRDA_LOG(LOG_ERROR, "Get One Input Buffer failed!");
        return MRDA_STATUS_INVALID_DATA;
    }
    if (frameData == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input buffer is empty!");
        return MRDA_STATUS_INVALID_DATA;
    }
    // transfer FrameBufferData -> FrameBufferItem
    std::shared_ptr<MemoryBuffer> memBuf = frameData->MemBuffer();
    std::shared_ptr<MemBufferItem> item = std::make_shared<MemBufferItem>();

    data = std::make_shared<FrameBufferItem>();

    item->assign(memBuf->BufId(), memBuf->MemOffset(), memBuf->StateOffset(),
                 memBuf->BufPtr(), memBuf->Size(), memBuf->OccupiedSize(),
                 memBuf->State());
    data->init(item.get(), frameData->Width(), frameData->Height(), frameData->StreamType(), frameData->Pts(), frameData->IsEOS());

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus MediaTask::ReleaseOutputBuffer(std::shared_ptr<FrameBufferItem> data)
{
    if (m_taskManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task manager is nullptr!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    std::shared_ptr<FrameBufferData> frameData = std::make_shared<FrameBufferData>();
    if (frameData == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Create frame buffer data failed!");
        return MRDA_STATUS_INVALID_DATA;
    }
    frameData->CreateFrameBufferData(data.get());

    return m_taskManager->ReleaseOneOutputBuffer(frameData);
}

MRDAStatus MediaTask::CheckMediaParams(const MediaParams *params)
{
    if (params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid media params!");
        return MRDA_STATUS_INVALID_DATA;
    }

    // check encode params
    if ((m_taskManager->TaskType() == TASKTYPE::taskEncode ||
        m_taskManager->TaskType() == TASKTYPE::taskFFmpegEncode ||
        m_taskManager->TaskType() == TASKTYPE::taskOneVPLEncode) &&
     (params->encodeParams.frame_width <= 0 || params->encodeParams.frame_height <= 0
        || params->encodeParams.framerate_den <= 0 || params->encodeParams.gop_size <= 0
        || (params->encodeParams.rc_mode == 1 && params->encodeParams.bit_rate <= 0)
        || (params->encodeParams.rc_mode == 0 && params->encodeParams.qp <= 0)
        || params->encodeParams.codec_id == StreamCodecID::CodecID_NONE
        || params->encodeParams.target_usage == TargetUsage::Unknown
        || params->encodeParams.color_format == ColorFormat::COLOR_FORMAT_NONE
        || params->encodeParams.codec_profile == CodecProfile::PROFILE_NONE))
    {
        MRDA_LOG(LOG_ERROR, "invalid encode parameters setting!");
        return MRDA_STATUS_INVALID_DATA;
    }

    // check share memory info
    if (params->shareMemoryInfo.bufferNum <=0 || params->shareMemoryInfo.bufferSize <=0
        || params->shareMemoryInfo.in_mem_dev_path == "" || params->shareMemoryInfo.out_mem_dev_path == ""
        || params->shareMemoryInfo.totalMemorySize <=0
        || params->shareMemoryInfo.in_mem_dev_slot_number <= 0
        || params->shareMemoryInfo.out_mem_dev_slot_number <= 0)
    {
        MRDA_LOG(LOG_ERROR, "invalid share memory parameters setting!");
        return MRDA_STATUS_INVALID_DATA;
    }

    // check decode params
    if ((m_taskManager->TaskType() == TASKTYPE::taskDecode ||
        m_taskManager->TaskType() == TASKTYPE::taskFFmpegDecode ||
        m_taskManager->TaskType() == TASKTYPE::taskOneVPLDecode) &&
    (params->decodeParams.codec_id == StreamCodecID::CodecID_NONE ||
     params->decodeParams.color_format == ColorFormat::COLOR_FORMAT_NONE ||
     params->decodeParams.frame_num == 0 ||
     params->decodeParams.frame_width <= 0 || params->decodeParams.frame_height <= 0 ||
     params->decodeParams.framerate_den <= 0 || params->decodeParams.framerate_num <= 0))
     {
        MRDA_LOG(LOG_ERROR, "invalid decode parameters setting!");
        return MRDA_STATUS_INVALID_DATA;
     }

    return MRDA_STATUS_SUCCESS;
}


VDI_NS_END