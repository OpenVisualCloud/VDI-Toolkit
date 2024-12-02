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
//! \file TaskDataSession_gRPC.cpp
//! \brief implement task data session using gRPC proto
//! \date 2024-04-19
//!

#include "TaskDataSession_gRPC.h"

VDI_NS_BEGIN

TaskDataSession_gRPC::TaskDataSession_gRPC(std::shared_ptr<TaskInfo> taskInfo)
{
    std::shared_ptr<Channel> channel = grpc::CreateChannel(taskInfo->ipAddr, grpc::InsecureChannelCredentials());
    m_stub = MRDA::MRDAService::NewStub(channel);
    m_inputQueue.clear();
    m_outputQueue.clear();
    m_taskInfo = taskInfo;
    m_frameNum = 0;
}

TaskDataSession_gRPC::~TaskDataSession_gRPC()
{
    m_sendThread.join();
    m_receiveThread.join();
    m_inputQueue.clear();
    m_outputQueue.clear();
    m_frameNum = 0;
}

MRDA::MediaParams TaskDataSession_gRPC::MakeMediaParams(const MediaParams *params)
{
    MRDA::MediaParams mrda_mediaParams;
    if (params == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input media params!");
        return mrda_mediaParams;
    }
    MRDA::ShareMemoryInfo *mrda_shmInfo = mrda_mediaParams.mutable_share_memory_info();
    MRDA::EncodeParams *mrda_encParams = mrda_mediaParams.mutable_enc_params();
    mrda_shmInfo->set_total_memory_size(params->shareMemoryInfo.totalMemorySize);
    mrda_shmInfo->set_buffer_num(params->shareMemoryInfo.bufferNum);
    mrda_shmInfo->set_buffer_size(params->shareMemoryInfo.bufferSize);
    mrda_shmInfo->set_in_mem_dev_path(params->shareMemoryInfo.in_mem_dev_path);
    mrda_shmInfo->set_out_mem_dev_path(params->shareMemoryInfo.out_mem_dev_path);
    mrda_encParams->set_codec_id(static_cast<uint32_t>(params->encodeParams.codec_id));
    mrda_encParams->set_gop_size(params->encodeParams.gop_size);
    mrda_encParams->set_async_depth(params->encodeParams.async_depth);
    mrda_encParams->set_target_usage(static_cast<uint32_t>(params->encodeParams.target_usage));
    mrda_encParams->set_rc_mode(params->encodeParams.rc_mode);
    mrda_encParams->set_qp(params->encodeParams.qp);
    mrda_encParams->set_bit_rate(params->encodeParams.bit_rate);
    mrda_encParams->set_framerate_num(params->encodeParams.framerate_num);
    mrda_encParams->set_framerate_den(params->encodeParams.framerate_den);
    mrda_encParams->set_frame_width(params->encodeParams.frame_width);
    mrda_encParams->set_frame_height(params->encodeParams.frame_height);
    mrda_encParams->set_color_format(static_cast<uint32_t>(params->encodeParams.color_format));
    mrda_encParams->set_codec_profile(static_cast<uint32_t>(params->encodeParams.codec_profile));
    mrda_encParams->set_max_b_frames(params->encodeParams.max_b_frames);
    mrda_encParams->set_frame_num(params->encodeParams.frame_num);
    MRDA::DecodeParams *mrda_decParams = mrda_mediaParams.mutable_dec_params();
    mrda_decParams->set_codec_id(static_cast<uint32_t>(params->decodeParams.codec_id));
    mrda_decParams->set_frame_width(params->decodeParams.frame_width);
    mrda_decParams->set_frame_height(params->decodeParams.frame_height);
    mrda_decParams->set_color_format(static_cast<uint32_t>(params->decodeParams.color_format));
    mrda_decParams->set_frame_num(params->decodeParams.frame_num);
    mrda_decParams->set_framerate_den(params->decodeParams.framerate_den);
    mrda_decParams->set_framerate_num(params->decodeParams.framerate_num);

    return mrda_mediaParams;
}

MRDA::BufferInfo TaskDataSession_gRPC::MakeBufferInfo(const std::shared_ptr<FrameBufferData> data)
{
    MRDA::BufferInfo mrda_bufferInfo;
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input data!");
        return mrda_bufferInfo;
    }
    MRDA::MemBuffer *mrda_memBuffer = mrda_bufferInfo.mutable_buffer();
    std::shared_ptr<MemoryBuffer> memoryBuffer = data->MemBuffer();
    if (memoryBuffer == nullptr || mrda_memBuffer == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "invalid input data!");
        return mrda_bufferInfo;
    }
    mrda_memBuffer->set_buf_id(memoryBuffer->BufId());
    mrda_memBuffer->set_state_offset(memoryBuffer->StateOffset());
    mrda_memBuffer->set_mem_offset(memoryBuffer->MemOffset());
    mrda_memBuffer->set_buf_size(memoryBuffer->Size());
    mrda_memBuffer->set_state(static_cast<int32_t>(memoryBuffer->State()));
    mrda_memBuffer->set_occupied_buf_size(memoryBuffer->OccupiedSize());
    mrda_bufferInfo.set_width(data->Width());
    mrda_bufferInfo.set_height(data->Height());
    mrda_bufferInfo.set_type(static_cast<int32_t>(data->StreamType()));
    mrda_bufferInfo.set_pts(data->Pts());
    mrda_bufferInfo.set_iseos(data->IsEOS());

    return mrda_bufferInfo;
}

MRDA::Pts TaskDataSession_gRPC::MakePts(uint64_t pts)
{
    MRDA::Pts out_mrda_pts;
    out_mrda_pts.set_pts(pts);
    return out_mrda_pts;
}

std::shared_ptr<FrameBufferData> TaskDataSession_gRPC::MakeBufferInfoBack(MRDA::BufferInfo info)
{
    std::shared_ptr<FrameBufferData> frameBufferData = std::make_shared<FrameBufferData>();
    MRDA::MemBuffer *mrda_memBuffer = info.mutable_buffer();
    std::shared_ptr<MemoryBuffer> memoryBuffer = std::make_shared<MemoryBuffer>();
    if (frameBufferData == nullptr || memoryBuffer == nullptr || mrda_memBuffer == nullptr) return nullptr;

    frameBufferData->SetWidth(info.width());
    frameBufferData->SetHeight(info.height());
    frameBufferData->SetStreamType(static_cast<InputStreamType>(info.type()));
    frameBufferData->SetPts(info.pts());
    frameBufferData->SetEOS(info.iseos());
    memoryBuffer->SetBufId(mrda_memBuffer->buf_id());
    memoryBuffer->SetMemOffset(mrda_memBuffer->mem_offset());
    memoryBuffer->SetStateOffset(mrda_memBuffer->state_offset());
    memoryBuffer->SetSize(mrda_memBuffer->buf_size());
    memoryBuffer->SetState(static_cast<BufferState>(mrda_memBuffer->state()));
    memoryBuffer->SetOccupiedSize(mrda_memBuffer->occupied_buf_size());
    frameBufferData->SetMemBuffer(memoryBuffer);
    return frameBufferData;
}

MRDAStatus TaskDataSession_gRPC::SetInitParams(const MediaParams *params)
{
    if (params == nullptr) return MRDA_STATUS_INVALID_PARAM;

    if (m_taskInfo->taskType == TASKTYPE::taskDecode ||
        m_taskInfo->taskType == TASKTYPE::taskFFmpegDecode ||
        m_taskInfo->taskType == TASKTYPE::taskOneVPLDecode)
    {
        m_frameNum = params->decodeParams.frame_num;
    }
    else if (m_taskInfo->taskType == TASKTYPE::taskEncode ||
             m_taskInfo->taskType == TASKTYPE::taskFFmpegEncode ||
             m_taskInfo->taskType == TASKTYPE::taskOneVPLEncode)
    {
        m_frameNum = params->encodeParams.frame_num;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "invalid task type!");
        return MRDA_STATUS_INVALID_PARAM;
    }
    grpc::ClientContext context;
    MRDA::MediaParams in_mrda_mediaParams = MakeMediaParams(params);
    MRDA::TaskStatus out_mrda_taskStatus;
    Status status = m_stub->SetInitParams(&context, in_mrda_mediaParams, &out_mrda_taskStatus);
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to set init params!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    if (out_mrda_taskStatus.status() != static_cast<int32_t>(MRDA_STATUS_SUCCESS))
    {
        return MRDA_STATUS_INVALID_STATE;
    }
    // start send and receive thread
    m_sendThread = std::thread(&TaskDataSession_gRPC::SendThread, this);
    m_receiveThread = std::thread(&TaskDataSession_gRPC::ReceiveThread, this);
    return MRDA_STATUS_SUCCESS;
}

void TaskDataSession_gRPC::SendThread()
{
    ClientContext inputContext;     // input client context
    MRDA::TaskStatus taskStatus;
    std::shared_ptr<ClientWriter<MRDA::BufferInfo>> writer(m_stub->SendInputData(&inputContext, &taskStatus));
    bool isSendRunning = true;
    while (isSendRunning)
    {
        std::shared_ptr<FrameBufferData> data = nullptr;
        {
        if (m_inputQueue.empty())
        {
            Sleep(50);
            continue;
        }
        std::unique_lock<std::mutex> lock(m_inputMutex);
        data = m_inputQueue.front();
        m_inputQueue.pop_front();
        }

        MRDA::BufferInfo in_mrda_bufferInfo = MakeBufferInfo(data);
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "MRDA trace log: send gRPC frame buffer in VM, pts: %llu", data->Pts());
#endif
        if (!writer->Write(in_mrda_bufferInfo))
        {
            MRDA_LOG(LOG_ERROR, "Failed to write input data!");
            return;
        }
        if (data->IsEOS())
        {
            writer->WritesDone();
            isSendRunning = false;
        }
    }
    Status status = writer->Finish();
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to finish writing input data!");
        return;
    }
    return;
}

void TaskDataSession_gRPC::ReceiveThread()
{
    ClientContext outputContext;                   // output client context
    MRDA::Pts pts = MakePts(static_cast<uint64_t>(m_frameNum));
    std::shared_ptr<ClientReader<MRDA::BufferInfo>> reader(m_stub->ReceiveOutputData(&outputContext, pts));
    int cur_pts = 0;
    bool isReceiveRunning = true;
    while (isReceiveRunning)
    {
        MRDA::BufferInfo out_mrda_bufferInfo;
        if (!reader->Read(&out_mrda_bufferInfo))
        {
            // MRDA_LOG(LOG_ERROR, "Failed to read buffer info!");
            isReceiveRunning = false;
            continue;
        }
        std::shared_ptr<FrameBufferData> data = MakeBufferInfoBack(out_mrda_bufferInfo);
        std::unique_lock<std::mutex> lock(m_outputMutex);
        m_outputQueue.push_back(data);
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "MRDA trace log: receive gRPC frame buffer in VM, pts: %llu", data->Pts());
#endif
    }
    Status status = reader->Finish();
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to finish reading buffer info!");
        return;
    }
}

MRDAStatus TaskDataSession_gRPC::SendFrame(const std::shared_ptr<FrameBufferData> data)
{
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to send input frame!");
        return MRDA_STATUS_INVALID_DATA;
    }
    std::unique_lock<std::mutex> lock(m_inputMutex);
#ifdef _ENABLE_TRACE_
    MRDA_LOG(LOG_INFO, "MRDA trace log: push back frame in input queue in task data session, pts: %llu", data->Pts());
#endif
    m_inputQueue.push_back(data);
    return MRDA_STATUS_SUCCESS;

}

MRDAStatus TaskDataSession_gRPC::ReceiveFrame(std::shared_ptr<FrameBufferData> &data)
{
    {
    while (m_outputQueue.empty())
    {
        Sleep(50);
        continue;
    }
    std::unique_lock<std::mutex> lock(m_outputMutex);
    data = m_outputQueue.front();
    m_outputQueue.pop_front();
#ifdef _ENABLE_TRACE_
    MRDA_LOG(LOG_INFO, "MRDA trace log: pop front frame in output queue in task data session, pts: %llu", data->Pts());
#endif
    }

    return MRDA_STATUS_SUCCESS;
}

VDI_NS_END