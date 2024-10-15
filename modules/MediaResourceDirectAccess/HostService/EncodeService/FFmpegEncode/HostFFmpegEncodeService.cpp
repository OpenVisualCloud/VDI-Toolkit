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
//! \file HostFFmpegEncodeService.cpp
//! \brief implement host FFmpeg encode service
//! \date 2024-07-01
//!

#ifdef _FFMPEG_SUPPORT_

#include "HostFFmpegEncodeService.h"
#include <fstream>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
#include <iostream>

#include <cstring>

#define INITIAL_POOL_SIZE 20

VDI_NS_BEGIN

HostFFmpegEncodeService::HostFFmpegEncodeService()
    :m_avctx(nullptr),
     m_hwDeviceCtx(nullptr)
{
    debug_file = fopen("out_host.hevc", "wb");
}

HostFFmpegEncodeService::~HostFFmpegEncodeService()
{
    avcodec_free_context(&m_avctx);
    av_buffer_unref(&m_hwDeviceCtx);

    m_encodeThread.join();

    fclose(debug_file);
}

MRDAStatus HostFFmpegEncodeService::Initialize()
{
    // init ffmpeg codec
    if (MRDA_STATUS_SUCCESS != InitCodec())
    {
        MRDA_LOG(LOG_ERROR, "Failed to init ffmpeg codec!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // init share memory
    if (MRDA_STATUS_SUCCESS != InitShm())
    {
        MRDA_LOG(LOG_ERROR, "Failed to init share memory!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // start encode thread
    m_encodeThread = std::thread(&HostFFmpegEncodeService::EncodeThread, this);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegEncodeService::set_hwframe_ctx()
{
    AVBufferRef *hw_frames_ref = nullptr;
    AVHWFramesContext *frames_ctx = nullptr;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(m_hwDeviceCtx)))
    {
        MRDA_LOG(LOG_ERROR, "Failed to create VAAPI frame context.");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (m_avctx == nullptr || m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "AV context or media params invalid!");
        return MRDA_STATUS_INVALID_DATA;
    }
    EncodeParams encodeParams = m_mediaParams->encodeParams;
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12; // after CSC, the pixel format is NV12
    frames_ctx->width     = m_avctx->width;
    frames_ctx->height    = m_avctx->height;
    frames_ctx->initial_pool_size = INITIAL_POOL_SIZE;

    if (av_hwframe_ctx_init(hw_frames_ref) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to initialize VAAPI frame context.");
        av_buffer_unref(&hw_frames_ref);
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // set avctx->hw_frames_ctx
    m_avctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!m_avctx->hw_frames_ctx)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create a reference to the hw frame context.");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    av_buffer_unref(&hw_frames_ref);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegEncodeService::SetEncParams()
{
    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params empty!");
        return MRDA_STATUS_INVALID_DATA;
    }

    EncodeParams encodeParams = m_mediaParams->encodeParams;
    m_avctx->width = encodeParams.frame_width;
    m_avctx->height = encodeParams.frame_height;
    if (encodeParams.framerate_den == 0) return MRDA_STATUS_INVALID_DATA;
    m_avctx->time_base = (AVRational){encodeParams.framerate_den, encodeParams.framerate_num};
    m_avctx->framerate = (AVRational){encodeParams.framerate_num, encodeParams.framerate_den};
    m_avctx->sample_aspect_ratio = (AVRational){1, 1};
    m_avctx->pix_fmt   = AV_PIX_FMT_VAAPI;
    m_avctx->codec_id = GetCodecId(encodeParams.codec_id);
    if (AV_CODEC_ID_NONE == m_avctx->codec_id) return MRDA_STATUS_INVALID_DATA;
    m_avctx->codec_type = AVMEDIA_TYPE_VIDEO;
    if (encodeParams.rc_mode == 0)
    {
        av_opt_set(m_avctx->priv_data, "qp", std::to_string(encodeParams.qp).c_str(), AV_OPT_SEARCH_CHILDREN);
    }
    else if (encodeParams.rc_mode == 1)
    {
        m_avctx->bit_rate = encodeParams.bit_rate * 1000; // input paramter (kbps) - ffmpeg (bps)
        m_avctx->rc_min_rate = m_avctx->bit_rate;
        m_avctx->rc_max_rate = m_avctx->bit_rate;
        m_avctx->bit_rate_tolerance = m_avctx->bit_rate;
        m_avctx->rc_buffer_size = m_avctx->bit_rate * 2;
        m_avctx->rc_initial_buffer_occupancy = m_avctx->rc_buffer_size * 3 / 4;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown rc mode!");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_avctx->gop_size = encodeParams.gop_size;
    m_avctx->profile = GetCodecProfile(encodeParams.codec_profile);
    m_avctx->max_b_frames = encodeParams.max_b_frames;
    if (encodeParams.async_depth > 0)
    {
        av_opt_set(m_avctx->priv_data, "async_depth", std::to_string(encodeParams.async_depth).c_str(), 0);
    }
    switch (encodeParams.target_usage)
    {
    case TargetUsage::Balanced:
        av_opt_set(m_avctx->priv_data, "preset", "medium", 0);
        break;
    case TargetUsage::BestQuality:
        av_opt_set(m_avctx->priv_data, "preset", "veryslow", 0);
        break;
    case TargetUsage::BestSpeed:
        av_opt_set(m_avctx->priv_data, "preset", "veryfast", 0);
        break;
    default:
        MRDA_LOG(LOG_ERROR, "Unknown target usage!");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

AVCodecID HostFFmpegEncodeService::GetCodecId(StreamCodecID codecID)
{
    if (codecID == StreamCodecID::CodecID_AV1)
    {
        return AV_CODEC_ID_AV1;
    }
    else if (codecID == StreamCodecID::CodecID_AVC)
    {
        return AV_CODEC_ID_H264;
    }
    else if (codecID == StreamCodecID::CodecID_HEVC)
    {
        return AV_CODEC_ID_HEVC;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown codec id!");
        return AV_CODEC_ID_NONE;
    }
}

AVPixelFormat HostFFmpegEncodeService::GetColorFormat(ColorFormat colorFormat)
{
    if(colorFormat == ColorFormat::COLOR_FORMAT_NV12)
    {
        return AVPixelFormat::AV_PIX_FMT_NV12;
    }
    else if (colorFormat == ColorFormat::COLOR_FORMAT_RGBA32)
    {
        return AVPixelFormat::AV_PIX_FMT_BGRA; // DX screen capture format order
    }
    else if (colorFormat == ColorFormat::COLOR_FORMAT_YUV420P)
    {
        return AVPixelFormat::AV_PIX_FMT_YUV420P;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Invalid color format");
        return AVPixelFormat::AV_PIX_FMT_NONE;
    }
}

std::string HostFFmpegEncodeService::GetEncoderName(StreamCodecID codec_id)
{
    if (codec_id == StreamCodecID::CodecID_AV1)
    {
        return "av1_vaapi";
    }
    else if (codec_id == StreamCodecID::CodecID_AVC)
    {
        return "h264_vaapi";
    }
    else if (codec_id == StreamCodecID::CodecID_HEVC)
    {
        return "hevc_vaapi";
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown codec id!");
        return "";
    }
}

int HostFFmpegEncodeService::GetCodecProfile(CodecProfile codecProfile)
{
    if (codecProfile == CodecProfile::PROFILE_AVC_MAIN)
    {
        return FF_PROFILE_H264_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_AVC_HIGH)
    {
        return FF_PROFILE_H264_HIGH;
    }
    else if (codecProfile == CodecProfile::PROFILE_HEVC_MAIN)
    {
        return FF_PROFILE_HEVC_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_HEVC_MAIN10)
    {
        return FF_PROFILE_HEVC_MAIN_10;
    }
    else if (codecProfile == CodecProfile::PROFILE_AV1_MAIN)
    {
        return FF_PROFILE_AV1_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_AV1_HIGH)
    {
        return FF_PROFILE_AV1_HIGH;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown codec profile!");
        return 0;
    }
}

MRDAStatus HostFFmpegEncodeService::InitCodec()
{
    if (av_hwdevice_ctx_create(&m_hwDeviceCtx, AV_HWDEVICE_TYPE_VAAPI,
                                 nullptr, nullptr, 0) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create VAAPI hardware device.");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params empty!");
        return MRDA_STATUS_INVALID_DATA;
    }

    EncodeParams encodeParams = m_mediaParams->encodeParams;
    std::string encNameStr = GetEncoderName(encodeParams.codec_id);

    const AVCodec *codec = avcodec_find_encoder_by_name(static_cast<const char *>(encNameStr.c_str()));
    if (codec == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Could not find encoder.");
        return MRDA_STATUS_INVALID_DATA;
    }

    m_avctx = avcodec_alloc_context3(codec);
    if (m_avctx == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate codec context!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (MRDA_STATUS_SUCCESS != SetEncParams())
    {
        MRDA_LOG(LOG_ERROR, "Failed to set encoder params");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    /* set hw_frames_ctx for encoder's AVCodecContext */
    if (set_hwframe_ctx() < 0) {
        MRDA_LOG(LOG_ERROR, "Failed to set hwframe context.");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (avcodec_open2(m_avctx, codec, nullptr) < 0) {
        MRDA_LOG(LOG_ERROR, "Cannot open video encoder codec.");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    return MRDA_STATUS_SUCCESS;
}

void* HostFFmpegEncodeService::EncodeThread()
{
    while (!m_isStop)
    {
        // get one frame
        std::shared_ptr<FrameBufferData> frame = nullptr;
        AVFrame *av_frame = nullptr;
        if (m_isEOS == false)
        {
            {
                if (m_inFrameBufferDataList.empty())
                {
                    usleep(5 * 1000);//5ms
                    continue;
                }
                std::unique_lock<std::mutex> lock(m_inMutex);
                frame = m_inFrameBufferDataList.front();
                m_inFrameBufferDataList.pop_front();
#ifdef _ENABLE_TRACE_
                if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: pop front frame in host encoding service input queue, pts: %lu, in dev path: %s", frame->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
                if (frame->IsEOS())
                {
                    MRDA_LOG(LOG_INFO, "Get EOS frame!!!!!!!!\n");
                    m_isEOS = true;
                    continue;
                }
            }
            // get surface for encode
            av_frame = GetSurfaceForEncode(frame);
        }
        // encode one frame
        if (MRDA_STATUS_SUCCESS != EncodeOneFrame(av_frame))
        {
            MRDA_LOG(LOG_ERROR, "EncodeOneFrame failed!");
            m_isStop = true;
            return nullptr;
        }
        else
        {
            // unref frame
            UnRefInputFrame(frame);
        }
    }
    return nullptr;
}


AVFrame* HostFFmpegEncodeService::GetSurfaceForEncode(std::shared_ptr<FrameBufferData> frame)
{
    if (frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input data is null!");
        return nullptr;
    }

    AVFrame *sw_frame = av_frame_alloc();
    if (sw_frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate AVFrame.");
        return nullptr;
    }

    EncodeParams encodeParams = m_mediaParams->encodeParams;
    sw_frame->format = AV_PIX_FMT_NV12; // uniformed color format
    sw_frame->width = frame->Width();
    sw_frame->height = frame->Height();
    sw_frame->pts = frame->Pts();
    // allocate new buffer for video
    if (av_frame_get_buffer(sw_frame, 0) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate frame data.");
        av_frame_free(&sw_frame);
        return nullptr;
    }

    if (MRDA_STATUS_SUCCESS != FillFrameToSurface(frame, sw_frame))
    {
        MRDA_LOG(LOG_ERROR, "FillFrameToSurface failed!");
        av_frame_free(&sw_frame);
        return nullptr;
    }

    AVFrame *hw_frame = av_frame_alloc();
    if (hw_frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate AVFrame.");
        av_frame_free(&sw_frame);
        return nullptr;
    }

    if (av_hwframe_get_buffer(m_avctx->hw_frames_ctx, hw_frame, 0) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate hw frame buffer.");
        av_frame_free(&sw_frame);
        av_frame_free(&hw_frame);
        return nullptr;
    }

    if (av_hwframe_transfer_data(hw_frame, sw_frame, 0) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to transfer data from sw to hw frame.");
        av_frame_free(&sw_frame);
        av_frame_free(&hw_frame);
        return nullptr;
    }
    hw_frame->pts = sw_frame->pts; // copy pts to hw frame

    av_frame_free(&sw_frame);

    return hw_frame;
}

MRDAStatus HostFFmpegEncodeService::ColorSpaceConvert(AVPixelFormat in_pix_fmt, AVPixelFormat out_pix_fmt, std::shared_ptr<FrameBufferData> frame, AVFrame* pSurface)
{
    if (pSurface == nullptr) return MRDA_STATUS_INVALID_DATA;

    int width = pSurface->width;
    int height = pSurface->height;
    struct SwsContext *sws_ctx = sws_getContext(width, height, in_pix_fmt,
                                                width, height, out_pix_fmt,
                                                SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        MRDA_LOG(LOG_ERROR, "Could not initialize sws context");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // construct source data and linesize
    if (frame->MemBuffer() == nullptr) return MRDA_STATUS_INVALID_DATA;
    size_t base_offset = frame->MemBuffer()->MemOffset();
    uint8_t *src_data[4];
    int src_linesize[4];
    uint8_t* base_ptr = reinterpret_cast<uint8_t*>(m_inShmMem) + base_offset;
    switch (in_pix_fmt)
    {
        case AVPixelFormat::AV_PIX_FMT_YUV420P:
            src_data[0] = base_ptr;
            src_data[1] = base_ptr + width * height;
            src_data[2] = base_ptr + width * height + width * height / 4;
            src_data[3] = nullptr;
            src_linesize[0] = width;
            src_linesize[1] = width / 2;
            src_linesize[2] = width / 2;
            src_linesize[3] = 0;
            break;
        case AVPixelFormat::AV_PIX_FMT_BGRA:
            src_data[0] = base_ptr;
            src_data[1] = nullptr;
            src_data[2] = nullptr;
            src_data[3] = nullptr;
            src_linesize[0] = width * 4;
            src_linesize[1] = 0;
            src_linesize[2] = 0;
            src_linesize[3] = 0;
            break;
        case AVPixelFormat::AV_PIX_FMT_NV12:
            pSurface->data[0] = base_ptr;
            pSurface->data[1] = base_ptr + width * height;
            pSurface->linesize[0] = width;
            pSurface->linesize[1] = width;
            return MRDA_STATUS_SUCCESS;
        default:
            return MRDA_STATUS_INVALID_DATA;
    }

    sws_scale(sws_ctx, src_data, src_linesize, 0, height, pSurface->data, pSurface->linesize);
    sws_freeContext(sws_ctx);

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegEncodeService::FillFrameToSurface(std::shared_ptr<FrameBufferData> frame, AVFrame* pSurface)
{
    // offset = memory offset + state offset (sizeof(uint32))
    if (frame->MemBuffer() == nullptr) return MRDA_STATUS_INVALID_DATA;
    size_t base_offset = frame->MemBuffer()->MemOffset();

    EncodeParams encodeParams = m_mediaParams->encodeParams;
    int w = frame->Width();
    int h = frame->Height();
    int i;
    int pitch = 0;
    uint8_t* ptr = nullptr;

    AVPixelFormat in_pix_fmt = GetColorFormat(encodeParams.color_format);
    AVPixelFormat out_pix_fmt = static_cast<AVPixelFormat>(pSurface->format); // output pixel format

    if (MRDA_STATUS_SUCCESS != ColorSpaceConvert(in_pix_fmt, out_pix_fmt, frame, pSurface))
    {
        MRDA_LOG(LOG_ERROR, "ColorSpaceConvert failed");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegEncodeService::EncodeOneFrame(AVFrame* pSurface)
{
    if (pSurface == nullptr && m_isEOS == false)
    {
        MRDA_LOG(LOG_ERROR, "pSurface is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }

    AVPacket *av_pkt = av_packet_alloc();
    if (av_pkt == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "av_pkt is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }
#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr && pSurface != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: begin avcodec send frame in FFmpeg encode service, pts: %lu, in dev path: %s", pSurface->pts, m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    if (avcodec_send_frame(m_avctx, pSurface) < 0)
    {
        MRDA_LOG(LOG_ERROR, "avcodec_send_frame failed");
        av_packet_free(&av_pkt);
        return MRDA_STATUS_INVALID_DATA;
    }
    av_frame_free(&pSurface);
    int ret = 0;
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(m_avctx, av_pkt);
        if (ret == AVERROR(EAGAIN))
        {
            // MRDA_LOG(LOG_INFO, "avcodec_receive_packet EAGAIN");
            break;
        }
        else if (ret == AVERROR_EOF)
        {
            if (m_isEOS)
            {
                m_isStop = true;
                MRDA_LOG(LOG_INFO, "Stop encode thread!!!");
            }
            // MRDA_LOG(LOG_INFO, "avcodec_receive_packet EOF");
            break;
        }
        else if (ret < 0)
        {
            MRDA_LOG(LOG_ERROR, "avcodec_receive_packet failed");
            av_packet_free(&av_pkt);
            m_isStop = true;
            return MRDA_STATUS_INVALID_DATA;
        }
        else
        {
#ifdef _ENABLE_TRACE_
            if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: complete avcodec receive packet in FFmpeg encode service, pts: %lu, in dev path: %s", av_pkt->pts, m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
            WriteToOutputShareMemoryBuffer(av_pkt);
            av_packet_unref(av_pkt);
            // MRDA_LOG(LOG_INFO, "encode one frame, frameNum = %d", m_frameNum);
            m_frameNum++;
        }
    }
    av_packet_free(&av_pkt);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegEncodeService::WriteToOutputShareMemoryBuffer(AVPacket* pBS)
{
    if (pBS == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "pBS is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (m_outShmMem == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "m_outShmMem is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }

    // Get one available buffer frame from output memory pool
    std::shared_ptr<FrameBufferData> data = nullptr;
    if (MRDA_STATUS_SUCCESS != GetAvailableOutputBufferFrame(data))
    {
        MRDA_LOG(LOG_ERROR, "GetAvailableOutputBufferFrame failed\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "data is null\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    // write to out share memory
    RefOutputFrame(data);
    if (data->MemBuffer() == nullptr) return MRDA_STATUS_INVALID_DATA;
    size_t mem_offset = data->MemBuffer()->MemOffset();
    memcpy(m_outShmMem + mem_offset, pBS->data, pBS->size);
    data->MemBuffer()->SetOccupiedSize(pBS->size);

    fwrite(pBS->data, 1, pBS->size, debug_file);
    // update output buffer list
#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: push back frame in host encoding service output queue, pts: %lu, in dev path: %s", data->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outFrameBufferDataList.push_back(data);
    // MRDA_LOG(LOG_INFO, "Push back output buffer at pts %llu", data->Pts());

    return MRDA_STATUS_SUCCESS;
}

VDI_NS_END

#endif // _FFMPEG_SUPPORT_