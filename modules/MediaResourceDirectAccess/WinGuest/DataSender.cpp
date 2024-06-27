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
//! \file DataSender.cpp
//! \brief
//! \date 2024-04-07
//!

#include "DataSender.h"
#include "TaskDataSession_gRPC.h"
#include "TaskDataSession.h"

VDI_NS_BEGIN

DataSender::DataSender()
    :m_taskInfo(nullptr),
    m_taskDataSession(nullptr){}

MRDAStatus DataSender::Initialize(const std::shared_ptr<TaskInfo> taskInfo)
{
    // task info
    if (taskInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task info is invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_taskInfo = taskInfo;
    // task data session created FIXME: gRPC proto type selection
    m_taskDataSession = std::make_unique<TaskDataSession_gRPC>(m_taskInfo);
    if (m_taskDataSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Create task data session failed!");
        return MRDA_STATUS_INVALID_DATA;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus DataSender::SetInitParams(const MediaParams *params)
{
    if (params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "media params invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (m_taskDataSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task data session is not initialized");
        return MRDA_STATUS_INVALID_DATA;
    }

    return m_taskDataSession->SetInitParams(params);
}

MRDAStatus DataSender::SendFrame(const std::shared_ptr<FrameBufferData> data)
{
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Frame buffer data is invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (m_taskDataSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Task data session is not initialized");
        return MRDA_STATUS_INVALID_DATA;
    }

    return m_taskDataSession->SendFrame(data);
}

VDI_NS_END