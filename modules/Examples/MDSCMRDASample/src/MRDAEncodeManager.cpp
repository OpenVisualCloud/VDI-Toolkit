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

#include "MRDAEncodeManager.h"

MRDAEncodeManager::MRDAEncodeManager() : m_pMRDA_handle(nullptr),
                                         m_pTerminateThreadEvent(nullptr),
	                                     m_pThreadHandle(nullptr),
                                         m_dwThreadId(0),
                                         m_nSendFrameCount(0),
                                         m_nReceiveFrameCount(0)
{

}

MRDAEncodeManager::~MRDAEncodeManager()
{
    MRDA_LOG(LOG_INFO, "[thread][%d], ~MRDAEncodeManager", m_uThreadId);
	MediaResourceDirectAccess_Destroy(m_pMRDA_handle);
}

MRDAStatus MRDAEncodeManager::Init(const MRDAEncode_Params &MRDAEncodeParams)
{
    if( EncodeManager::InitAVContext(MRDAEncodeParams.encode_params) < 0 )
    {
		MRDA_LOG(LOG_ERROR, "[thread][%d], InitVideoContext failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
    }

    if (MRDA_STATUS_SUCCESS != InitMRDA(MRDAEncodeParams))
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], InitMRDA failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}

    m_pMediaParams = std::make_shared<MediaParams>();
    if (MRDA_STATUS_SUCCESS != CreateMediaParams(MRDAEncodeParams))
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], CreateMediaParams failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}

    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SetInitParams(m_pMRDA_handle, m_pMediaParams.get()))
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], set init params failed!", m_uThreadId);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_pTerminateThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_pTerminateThreadEvent)
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], Failed to create TerminateThreadsEvent", m_uThreadId);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    m_pThreadHandle = CreateThread(nullptr, 0, ReceiveFrameThreadProc, this, 0, &m_dwThreadId);
    if (m_pThreadHandle == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], Failed to create ReceiveFrame thread", m_uThreadId);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    MRDA_LOG(LOG_INFO, "[thread][%d], MRDAEncodeManager Init Successed!", m_uThreadId);
	return MRDA_STATUS_SUCCESS;
}

MRDAStatus MRDAEncodeManager::Encode(uint8_t* data, uint64_t timestamp)
{
 //#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> st_enc_tp = std::chrono::high_resolution_clock::now();
//#endif
	if (nullptr == data)
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], data is nullptr", m_uThreadId);
		return MRDA_STATUS_INVALID_PARAM;
	}

    // get input buffer
    std::shared_ptr<FrameBufferItem> inBuffer = nullptr;
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_GetBufferForInput(m_pMRDA_handle, inBuffer))
    {
        MRDA_LOG(LOG_WARNING, "[thread][%d], get buffer for input failed!", m_uThreadId);
        Sleep(10); // 10ms
        return MRDA_STATUS_INVALID_DATA;
    }
    if (inBuffer == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], Input buffer is empty!", m_uThreadId);
        return MRDA_STATUS_INVALID_DATA;
    }

    // fill input buffer
    inBuffer->width = m_pMediaParams->encodeParams.frame_width;
    inBuffer->height = m_pMediaParams->encodeParams.frame_height;

    switch (m_pMediaParams->encodeParams.color_format)
    {
    case ColorFormat::COLOR_FORMAT_NV12:
    case ColorFormat::COLOR_FORMAT_YUV420P:
        inBuffer->bufferItem->occupied_size = inBuffer->width * inBuffer->height * 3 / 2;
        break;
    case ColorFormat::COLOR_FORMAT_RGBA32:
        inBuffer->bufferItem->occupied_size = inBuffer->width * inBuffer->height * 4;
        break;
    case ColorFormat::COLOR_FORMAT_NONE:
    default:
        MRDA_LOG(LOG_ERROR, "[thread][%d], ERROR: invalid color format", m_uThreadId);
        return MRDA_STATUS_INVALID_DATA;
    }
    inBuffer->pts = timestamp;
    memcpy(inBuffer->bufferItem->buf_ptr, data, inBuffer->bufferItem->occupied_size);

    // mrda encode
	if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SendFrame(m_pMRDA_handle, inBuffer))
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], MediaResourceDirectAccess_Encode failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}

    inBuffer->uninit();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_nSendFrameCount++;
//#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> ed_enc_tp = std::chrono::high_resolution_clock::now();
    uint64_t enc_duration = std::chrono::duration_cast<std::chrono::microseconds>(ed_enc_tp - st_enc_tp).count();
    MRDA_LOG(LOG_INFO, "[thread][%d], MediaResourceDirectAccess send frame successed, framecount %d, timecost %lldms", m_uThreadId, m_nSendFrameCount, enc_duration/1000);
//#endif
	return MRDA_STATUS_SUCCESS;
}

MRDAStatus MRDAEncodeManager::End_video_output()
{
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SendFrame(m_pMRDA_handle, nullptr))
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], encode frame failed!", m_uThreadId);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (SetEvent(m_pTerminateThreadEvent))
    {
        DWORD res = WaitForSingleObjectEx(m_pThreadHandle, INFINITE, false);
        MRDA_LOG(LOG_INFO, "[thread][%d], WaitForSingleObjectEx, res %ul", m_uThreadId, res);
    }
    CloseHandle(m_pTerminateThreadEvent);
    MRDA_LOG(LOG_INFO, "[thread][%d], ReceiveFrameThread ended, receiveFrameThreadId %d", m_uThreadId, m_dwThreadId);

    //EncodeManager::End_video_output();
    av_write_trailer(EncodeManager::m_pVideoOfmtCtx);
    MRDA_LOG(LOG_INFO, "[thread][%d], End_video_output, av_write_trailer successed", m_uThreadId);

	if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Stop(m_pMRDA_handle))
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], MediaResourceDirectAccess_End_video_output failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}
    MRDA_LOG(LOG_INFO, "[thread][%d], MRDA stopped successed", m_uThreadId);
	return MRDA_STATUS_SUCCESS;
}

MRDAStatus MRDAEncodeManager::InitMRDA(const MRDAEncode_Params &MRDAEncodeParams)
{
    TaskInfo taskInfo = {};
    if (FFmpeg_MRDA == MRDAEncodeParams.encode_params.encode_type)
    {
         taskInfo.taskType = TASKTYPE::taskFFmpegEncode;
	}
	else if (VPL_MRDA == MRDAEncodeParams.encode_params.encode_type)
	{
		taskInfo.taskType = TASKTYPE::taskOneVPLEncode;
	}
	else
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], Unsupported MRDA Task Type: %d", m_uThreadId, MRDAEncodeParams.encode_params.encode_type);
		return MRDA_STATUS_INVALID_PARAM;
	}

    taskInfo.taskStatus = TASKStatus::TASK_STATUS_UNKNOWN;
    taskInfo.taskID = 0;
    taskInfo.taskDevice.deviceType = DeviceType::NONE;
    taskInfo.taskDevice.deviceID = 0;
    taskInfo.ipAddr = "";

    ExternalConfig config = {};
    config.hostSessionAddr = MRDAEncodeParams.hostSessionAddr;
	m_pMRDA_handle = MediaResourceDirectAccess_Init(&taskInfo, &config);
    if (m_pMRDA_handle == nullptr)
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], MediaResourceDirectAccess_Create failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}

	return MRDA_STATUS_SUCCESS;
}

MRDAStatus MRDAEncodeManager::DestroyMRDA()
{
	if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Destroy(&m_pMRDA_handle))
	{
		MRDA_LOG(LOG_ERROR, "[thread][%d], MediaResourceDirectAccess_Destroy failed!", m_uThreadId);
		return MRDA_STATUS_OPERATION_FAIL;
	}

	return MRDA_STATUS_SUCCESS;
}

void MRDAEncodeManager::ReceiveFrameThread()
{
    MRDA_LOG(LOG_INFO, "[thread][%d], ReceiveFrameThread start, receiveFrameThreadId %d", m_uThreadId, m_dwThreadId);
    while (WaitForSingleObjectEx(m_pTerminateThreadEvent, 0, FALSE) == WAIT_TIMEOUT)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(m_pMRDA_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "[thread][%d], Output data not enough!");
            continue;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], get buffer for output failed!", m_uThreadId);
            return;
        }
        // success
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "[thread][%d], Encoding trace log: complete receive frame in sample encode, framecount %d, pts: %llu", m_uThreadId, m_nReceiveFrameCount, outBuffer->pts);
#endif
        int ret = 0;
        AVPacket * pPkt = EncodeManager::m_pPkt;
        if (!pPkt){
            MRDA_LOG(LOG_ERROR, "[thread][%d], AVPacket is nullptr", m_uThreadId);
            return;
        }
        pPkt->data = outBuffer->bufferItem->buf_ptr + sizeof(outBuffer->bufferItem->state);
        pPkt->size = outBuffer->bufferItem->occupied_size;
        pPkt->pts = outBuffer->pts;
        pPkt->dts = pPkt->pts;
        ret = av_interleaved_write_frame(EncodeManager::m_pVideoOfmtCtx, pPkt);
        if (ret < 0)
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], av_interleaved_write_frame failed!", m_uThreadId);
            return;
        }
        av_packet_unref(pPkt);

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(m_pMRDA_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], release output buffer failed!", m_uThreadId);
            return;
        }
        outBuffer->uninit();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_nReceiveFrameCount++;
    }

    FlushFrame();
    return;
}

MRDAStatus MRDAEncodeManager::FlushFrame()
{
    uint32_t nSendFrameCount = GetSendFrameCount();
    MRDA_LOG(LOG_INFO, "[thread][%d], Flush frame, sendframecount %d, receiveframecount %d", m_uThreadId, nSendFrameCount, m_nReceiveFrameCount);
    while (m_nReceiveFrameCount < nSendFrameCount)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(m_pMRDA_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "[thread][%d], In flush process: Output data not enough!");
            continue;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], get buffer for output failed!", m_uThreadId);
            return MRDA_STATUS_OPERATION_FAIL;
        }
        // success
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "[thread][%d], Encoding trace log: complete receive frame in sample encode, framecount %d, pts: %llu", m_nReceiveFrameCount, outBuffer->pts);
#endif
        int ret = 0;
        AVPacket * pPkt = EncodeManager::m_pPkt;
        if (!pPkt){
            MRDA_LOG(LOG_ERROR, "[thread][%d], AVPacket is nullptr", m_uThreadId);
            return MRDA_STATUS_INVALID_DATA;
        }
        pPkt->data = outBuffer->bufferItem->buf_ptr + sizeof(outBuffer->bufferItem->state);
        pPkt->size = outBuffer->bufferItem->occupied_size;
        pPkt->pts = outBuffer->pts;
        pPkt->dts = pPkt->pts;
        ret = av_interleaved_write_frame(EncodeManager::m_pVideoOfmtCtx, pPkt);
        if (ret < 0)
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], av_interleaved_write_frame failed!", m_uThreadId);
            return MRDA_STATUS_OPERATION_FAIL;
        }
        av_packet_unref(pPkt);
        // MRDA_LOG(LOG_INFO, "[thread][%d], Receive frame at pts: %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(m_pMRDA_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "[thread][%d], release output buffer failed!", m_uThreadId);
            return MRDA_STATUS_OPERATION_FAIL;
        }
        outBuffer->uninit();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_nReceiveFrameCount++;
    }
    return MRDA_STATUS_SUCCESS;
}


MRDAStatus MRDAEncodeManager::CreateMediaParams(const MRDAEncode_Params &MRDAEncodeParams)
{
    m_pMediaParams->shareMemoryInfo.totalMemorySize = MRDAEncodeParams.totalMemorySize;
    m_pMediaParams->shareMemoryInfo.bufferNum = MRDAEncodeParams.bufferNum;
    m_pMediaParams->shareMemoryInfo.bufferSize = MRDAEncodeParams.bufferSize;
    m_pMediaParams->shareMemoryInfo.in_mem_dev_path = MRDAEncodeParams.in_mem_dev_path;
    m_pMediaParams->shareMemoryInfo.out_mem_dev_path = MRDAEncodeParams.out_mem_dev_path;
    m_pMediaParams->shareMemoryInfo.in_mem_dev_slot_number = MRDAEncodeParams.in_mem_dev_slot_number;
    m_pMediaParams->shareMemoryInfo.out_mem_dev_slot_number = MRDAEncodeParams.out_mem_dev_slot_number;

    m_pMediaParams->encodeParams.codec_id = StringToCodecID(MRDAEncodeParams.encode_params.codec_id.c_str());
    m_pMediaParams->encodeParams.gop_size = MRDAEncodeParams.encode_params.gop;
    m_pMediaParams->encodeParams.async_depth = MRDAEncodeParams.encode_params.async_depth;
    m_pMediaParams->encodeParams.target_usage = StringToTargetUsage(MRDAEncodeParams.encode_params.target_usage.c_str());
    m_pMediaParams->encodeParams.rc_mode = StringToRCMode(MRDAEncodeParams.encode_params.rc_mode.c_str());
    m_pMediaParams->encodeParams.qp = MRDAEncodeParams.encode_params.qp;
    m_pMediaParams->encodeParams.bit_rate = MRDAEncodeParams.encode_params.bitrate;
    m_pMediaParams->encodeParams.framerate_num = MRDAEncodeParams.encode_params.framerate_num;
    m_pMediaParams->encodeParams.framerate_den = MRDAEncodeParams.encode_params.framerate_den;
    m_pMediaParams->encodeParams.frame_width = MRDAEncodeParams.encode_params.width;
    m_pMediaParams->encodeParams.frame_height = MRDAEncodeParams.encode_params.height;
    m_pMediaParams->encodeParams.color_format = StringToColorFormat(MRDAEncodeParams.encode_params.input_color_format.c_str());
    m_pMediaParams->encodeParams.codec_profile = StringToCodecProfile(MRDAEncodeParams.encode_params.codec_profile.c_str());
    m_pMediaParams->encodeParams.max_b_frames = MRDAEncodeParams.encode_params.max_b_frames;
    m_pMediaParams->encodeParams.frame_num = MRDAEncodeParams.frameNum;
    return MRDA_STATUS_SUCCESS;
}

StreamCodecID MRDAEncodeManager::StringToCodecID(const char *codec_id)
{
	StreamCodecID streamCodecId = StreamCodecID::CodecID_NONE;
    if (!strcmp(codec_id, "h264") || !strcmp(codec_id, "avc"))
    {
        streamCodecId = StreamCodecID::CodecID_AVC;
    }
    else if (!strcmp(codec_id, "h265") || !strcmp(codec_id, "hevc"))
    {
        streamCodecId = StreamCodecID::CodecID_HEVC;
    }
    else if (!strcmp(codec_id, "av1"))
    {
        streamCodecId = StreamCodecID::CodecID_AV1;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], Unsupported codec id: %s", m_uThreadId, codec_id);
    }
    return streamCodecId;
}

TargetUsage MRDAEncodeManager::StringToTargetUsage(const char *target_usage)
{
	TargetUsage targetUsage = TargetUsage::Balanced;
    if (!strcmp(target_usage, "balanced"))
    {
        targetUsage = TargetUsage::Balanced;
    }
    else if (!strcmp(target_usage, "quality"))
    {
        targetUsage = TargetUsage::BestQuality;
    }
    else if (!strcmp(target_usage, "speed"))
    {
        targetUsage = TargetUsage::BestSpeed;
    }
    return targetUsage;
}

uint32_t MRDAEncodeManager::StringToRCMode(const char *rc_mode)
{
    if (!strcmp(rc_mode, "CQP"))
    {
        return 0;
    }
    else if (!strcmp(rc_mode, "VBR"))
    {
        return 1;
    }
    else
    {
		return 0;
	}
}

ColorFormat MRDAEncodeManager::StringToColorFormat(const char *color_format)
{
	ColorFormat colorFormat = ColorFormat::COLOR_FORMAT_NONE;
    if (!strcmp(color_format, "rgb32"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_RGBA32;
    }
    else if (!strcmp(color_format, "yuv420p"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_YUV420P;
    }
    else if (!strcmp(color_format, "nv12"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_NV12;
    }
    return colorFormat;
}

CodecProfile MRDAEncodeManager::StringToCodecProfile(const char *codec_profile)
{
	CodecProfile codecProfile = CodecProfile::PROFILE_NONE;
    if (!strcmp(codec_profile, "avc:main"))
    {
        codecProfile = CodecProfile::PROFILE_AVC_MAIN;
    }
    else if (!strcmp(codec_profile, "avc:high"))
    {
        codecProfile = CodecProfile::PROFILE_AVC_HIGH;
    }
    else if (!strcmp(codec_profile, "hevc:main"))
    {
        codecProfile = CodecProfile::PROFILE_HEVC_MAIN;
    }
    else if (!strcmp(codec_profile, "hevc:main10"))
    {
        codecProfile = CodecProfile::PROFILE_HEVC_MAIN10;
    }
    else if (!strcmp(codec_profile, "av1:main"))
    {
        codecProfile = CodecProfile::PROFILE_AV1_MAIN;
    }
    else if (!strcmp(codec_profile, "av1:high"))
    {
        codecProfile = CodecProfile::PROFILE_AV1_HIGH;
    }
    return codecProfile;
}

uint32_t MRDAEncodeManager::GetSendFrameCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
	return m_nSendFrameCount;
}
uint32_t MRDAEncodeManager::GetReceiveFrameCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nReceiveFrameCount;
}
