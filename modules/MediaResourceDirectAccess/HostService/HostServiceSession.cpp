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
//! \file HostServiceSession.cpp
//! \brief Host service server implementation
//! \date 2024-04-19
//!

#include "HostServiceSession.h"

VDI_NS_BEGIN

HostServiceSession::HostServiceSession()
    : m_server(nullptr),
      m_hostService(nullptr),
      m_hostServiceFactory(nullptr) {}

MRDAStatus HostServiceSession::Initialize(MRDA::TaskInfo* taskInfo)
{
    if (taskInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "input task info is invalid");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_hostServiceFactory = std::make_unique<HostServiceFactory>();
    if (m_hostServiceFactory == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create host service factory");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_hostService = m_hostServiceFactory->CreateHostService(taskInfo);
    if (m_hostService == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "failed to create host service");
        return MRDA_STATUS_INVALID_DATA;
    }
    return MRDA_STATUS_SUCCESS;
}

Status HostServiceSession::SetInitParams(ServerContext* context, const MRDA::MediaParams* mrda_mediaParams, MRDA::TaskStatus* mrda_status)
{
    if (mrda_mediaParams == nullptr || mrda_status == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "input data is invalid");
        return Status::CANCELLED;
    }
    MediaParams mediaParams;
    MakeMediaParamsBack(mrda_mediaParams, &mediaParams);
    MRDAStatus st = m_hostService->SetInitParams(&mediaParams);
    if (st != MRDA_STATUS_SUCCESS)
    {
        mrda_status->set_status(static_cast<int32_t>(st));
        return Status::CANCELLED;
    }
    st = m_hostService->Initialize();
    mrda_status->set_status(static_cast<int32_t>(st));
    return Status::OK;
}

Status HostServiceSession::SendInputData(ServerContext* context, const MRDA::BufferInfo* bufferInfo, MRDA::TaskStatus* mrda_taskStatus)
{
    if (bufferInfo == nullptr || mrda_taskStatus == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "input data is invalid");
        return Status::CANCELLED;
    }
    std::shared_ptr<FrameBufferData> buffer = std::make_shared<FrameBufferData>();
    MakeBufferInfoBack(bufferInfo, buffer);
    if (m_hostService == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "host service is not initialized");
        return Status::CANCELLED;
    }

    MRDAStatus st = m_hostService->SendInputData(buffer);
    mrda_taskStatus->set_status(static_cast<int32_t>(st));

    return Status::OK;
}

Status HostServiceSession::ReceiveOutputData(ServerContext* context, const MRDA::Pts* pts, MRDA::BufferInfo* bufferInfo)
{
    if (pts == nullptr || bufferInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "input data is invalid");
        return Status::CANCELLED;
    }
    std::shared_ptr<FrameBufferData> buffer = nullptr;

    if (m_hostService == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "host service is not initialized");
        return Status::CANCELLED;
    }

    MRDAStatus st = m_hostService->ReceiveOutputData(buffer);
    // error
    if (MRDA_STATUS_SUCCESS != st && MRDA_STATUS_NOT_ENOUGH_DATA != st)
    {
        MRDA_LOG(LOG_ERROR, "receive output data failed");
        return Status::CANCELLED;
    }
    // not enough output data
    if (MRDA_STATUS_NOT_ENOUGH_DATA == st) {
        // MRDA_LOG(LOG_INFO, "Receive data empty! please wait!");
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "unavailable data provided.");
    }
    // check pts
    if (MRDA_STATUS_SUCCESS == st && buffer->Pts() != pts->pts())
    {
        MRDA_LOG(LOG_ERROR, "pts is not equal, buffer pts %lu, required pts %lu", buffer->Pts(), pts->pts());
        return Status::CANCELLED;
    }

    MakeBufferInfo(buffer, bufferInfo);

    return Status::OK;
}

MRDAStatus HostServiceSession::MakeBufferInfoBack(const MRDA::BufferInfo *mrda_bufferInfo, std::shared_ptr<FrameBufferData> &buffer)
{
    if (mrda_bufferInfo == nullptr || buffer == nullptr) return MRDA_STATUS_INVALID_DATA;
    MRDA::MemBuffer *mrda_memBuffer = (const_cast<MRDA::BufferInfo*>(mrda_bufferInfo))->mutable_buffer();
    std::shared_ptr<MemoryBuffer> memoryBuffer = std::make_shared<MemoryBuffer>();
    if (memoryBuffer == nullptr || mrda_memBuffer == nullptr) return MRDA_STATUS_INVALID_DATA;

    buffer->SetWidth(mrda_bufferInfo->width());
    buffer->SetHeight(mrda_bufferInfo->height());
    buffer->SetStreamType(static_cast<InputStreamType>(mrda_bufferInfo->type()));
    buffer->SetPts(mrda_bufferInfo->pts());
    buffer->SetEOS(mrda_bufferInfo->iseos());
    memoryBuffer->SetBufId(mrda_memBuffer->buf_id());
    memoryBuffer->SetMemOffset(mrda_memBuffer->mem_offset());
    memoryBuffer->SetOccupiedSize(mrda_memBuffer->occupied_buf_size());
    memoryBuffer->SetStateOffset(mrda_memBuffer->state_offset());
    memoryBuffer->SetSize(mrda_memBuffer->buf_size());
    memoryBuffer->SetState(static_cast<BufferState>(mrda_memBuffer->state()));
    buffer->SetMemBuffer(memoryBuffer);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostServiceSession::MakeBufferInfo(const std::shared_ptr<FrameBufferData> buffer, MRDA::BufferInfo* mrda_bufferInfo)
{
    if (buffer == nullptr || mrda_bufferInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input data!");
        return MRDA_STATUS_INVALID_DATA;
    }
    MRDA::MemBuffer *mrda_memBuffer = mrda_bufferInfo->mutable_buffer();
    std::shared_ptr<MemoryBuffer> memoryBuffer = buffer->MemBuffer();
    if (memoryBuffer == nullptr || mrda_memBuffer == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input data!");
        return MRDA_STATUS_INVALID_DATA;
    }
    mrda_memBuffer->set_buf_id(memoryBuffer->BufId());
    mrda_memBuffer->set_state_offset(memoryBuffer->StateOffset());
    mrda_memBuffer->set_mem_offset(memoryBuffer->MemOffset());
    mrda_memBuffer->set_buf_size(memoryBuffer->Size());
    mrda_memBuffer->set_state(static_cast<int32_t>(memoryBuffer->State()));
    mrda_memBuffer->set_occupied_buf_size(memoryBuffer->OccupiedSize());
    mrda_bufferInfo->set_width(buffer->Width());
    mrda_bufferInfo->set_height(buffer->Height());
    mrda_bufferInfo->set_type(static_cast<int32_t>(buffer->StreamType()));
    mrda_bufferInfo->set_pts(buffer->Pts());
    mrda_bufferInfo->set_iseos(buffer->IsEOS());

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostServiceSession::MakeMediaParamsBack(const MRDA::MediaParams* mrda_mediaParams, MediaParams *params)
{
    if (mrda_mediaParams == nullptr || params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input media params!");
        return MRDA_STATUS_INVALID_DATA;
    }
    MRDA::ShareMemoryInfo *mrda_shmInfo = (const_cast<MRDA::MediaParams*>(mrda_mediaParams))->mutable_share_memory_info();
    MRDA::EncodeParams *mrda_encParams = (const_cast<MRDA::MediaParams*>(mrda_mediaParams))->mutable_enc_params();
    params->shareMemoryInfo.totalMemorySize = mrda_shmInfo->total_memory_size();
    params->shareMemoryInfo.bufferNum = mrda_shmInfo->buffer_num();
    params->shareMemoryInfo.bufferSize = mrda_shmInfo->buffer_size();
    params->shareMemoryInfo.in_mem_dev_path = mrda_shmInfo->in_mem_dev_path();
    params->shareMemoryInfo.out_mem_dev_path = mrda_shmInfo->out_mem_dev_path();

    params->encodeParams.codec_id = static_cast<StreamCodecID>(mrda_encParams->codec_id());
    params->encodeParams.gop_size = mrda_encParams->gop_size();
    params->encodeParams.async_depth = mrda_encParams->async_depth();
    params->encodeParams.target_usage = static_cast<TargetUsage>(mrda_encParams->target_usage());
    params->encodeParams.rc_mode = mrda_encParams->rc_mode();
    params->encodeParams.qp = mrda_encParams->qp();
    params->encodeParams.bit_rate = mrda_encParams->bit_rate();
    params->encodeParams.framerate_num = mrda_encParams->framerate_num();
    params->encodeParams.framerate_den = mrda_encParams->framerate_den();
    params->encodeParams.frame_width = mrda_encParams->frame_width();
    params->encodeParams.frame_height = mrda_encParams->frame_height();
    params->encodeParams.color_format = static_cast<ColorFormat>(mrda_encParams->color_format());
    params->encodeParams.codec_profile = static_cast<CodecProfile>(mrda_encParams->codec_profile());
    params->encodeParams.gop_ref_dist = mrda_encParams->gop_ref_dist();
    params->encodeParams.num_ref_frame = mrda_encParams->num_ref_frame();
    return MRDA_STATUS_SUCCESS;
}

void HostServiceSession::RunService(std::string server_address)
{
    std::unique_ptr<ServerBuilder> serverBuilder = std::make_unique<ServerBuilder>();
    serverBuilder->AddListeningPort(server_address, grpc::InsecureServerCredentials());
    serverBuilder->RegisterService(this);
    m_server = serverBuilder->BuildAndStart();
    MRDA_LOG(LOG_INFO, "Service listening on %s", server_address.c_str());
    m_server->Wait();
}

void HostServiceSession::StopService()
{
    if (m_server != nullptr)
    {
        m_server->Shutdown();
    }
}

VDI_NS_END