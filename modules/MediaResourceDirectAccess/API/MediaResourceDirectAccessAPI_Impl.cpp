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
//! \file MediaResourceDirectAccessAPI_Impl.cpp
//! \brief Media Resource Direct Access API implementation
//! \date 2024-03-29
//!

#include "MediaResourceDirectAccessAPI.h"
#include "../utils/common.h"

#include "../WinGuest/MediaTask.h"

VDI_USE_MRDALib;

MRDAHandle MediaResourceDirectAccess_Init(const TaskInfo *taskInfo, const ExternalConfig *config)
{
    MediaTask* mediaTask = new MediaTask();
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create mediaTask");
        return nullptr;
    }

    if (MRDA_STATUS_SUCCESS != mediaTask->Initialize(taskInfo, config))
    {
        MRDA_LOG(LOG_ERROR, "Failed to initialize mediaTask");
        return nullptr;
    }

    // MRDA_LOG(LOG_INFO, "mediaTask initialized successfully");
    return mediaTask;
}

MRDAStatus MediaResourceDirectAccess_Stop(MRDAHandle handle)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->Stop();
}

MRDAStatus MediaResourceDirectAccess_Reset(MRDAHandle handle, TaskInfo *taskInfo)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->Reset(taskInfo);
}

MRDAStatus MediaResourceDirectAccess_Destroy(MRDAHandle handle)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    mediaTask->Destroy();
    SAFE_DELETE(mediaTask);

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus MediaResourceDirectAccess_SetInitParams(MRDAHandle handle, const MediaParams *mediaParams)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->SetInitParams(mediaParams);
}

MRDAStatus MediaResourceDirectAccess_GetBufferForInput(MRDAHandle handle, std::shared_ptr<FrameBufferItem> &inputFrameData)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->GetOneInputBuffer(inputFrameData);
}

MRDAStatus MediaResourceDirectAccess_ReleaseOutputBuffer(MRDAHandle handle, std::shared_ptr<FrameBufferItem> outputFrameData)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->ReleaseOutputBuffer(outputFrameData);
}

MRDAStatus MediaResourceDirectAccess_SendFrame(MRDAHandle handle, std::shared_ptr<FrameBufferItem> inputFrameData)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->SendFrame(inputFrameData);
}

MRDAStatus MediaResourceDirectAccess_ReceiveFrame(MRDAHandle handle, std::shared_ptr<FrameBufferItem> &outputFrameData)
{
    MediaTask* mediaTask = (MediaTask*)handle;
    if (mediaTask == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid mediaTask handle");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    return mediaTask->ReceiveFrame(outputFrameData);
}