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
//! \file HostFFmpegDecodeService.cpp
//! \brief implement host FFmpeg decode service
//! \date 2024-11-11
//!

#ifdef _FFMPEG_SUPPORT_

#include "HostFFmpegDecodeService.h"
#include <fstream>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
#include <iostream>
#include <assert.h>
#include <cstring>

VDI_NS_BEGIN

HostFFmpegDecodeService::HostFFmpegDecodeService(TaskInfo taskInfo)
    :m_avctx(nullptr),
     m_hwDeviceCtx(nullptr)
{
    debug_file = fopen("out_host.nv12", "wb");
    m_taskInfo = taskInfo;
}

HostFFmpegDecodeService::~HostFFmpegDecodeService()
{
    avcodec_free_context(&m_avctx);
    av_buffer_unref(&m_hwDeviceCtx);

    m_decodeThread.join();

    fclose(debug_file);
}

MRDAStatus HostFFmpegDecodeService::Initialize()
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
    // start decode thread
    m_decodeThread = std::thread(&HostFFmpegDecodeService::DecodeThread, this);
    return MRDA_STATUS_SUCCESS;
}

AVCodecID HostFFmpegDecodeService::GetCodecId(StreamCodecID codecID)
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

AVPixelFormat HostFFmpegDecodeService::GetColorFormat(ColorFormat colorFormat)
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

MRDAStatus HostFFmpegDecodeService::SetDecParams()
{
    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Media params empty!");
        return MRDA_STATUS_INVALID_DATA;
    }

    DecodeParams decodeParams = m_mediaParams->decodeParams;
    m_avctx->width = decodeParams.frame_width;
    m_avctx->height = decodeParams.frame_height;
    if (decodeParams.framerate_den == 0) return MRDA_STATUS_INVALID_DATA;
    m_avctx->framerate = (AVRational){decodeParams.framerate_num, decodeParams.framerate_den};
    m_avctx->pix_fmt   = AV_PIX_FMT_VAAPI;
    m_avctx->codec_id = GetCodecId(decodeParams.codec_id);
    if (AV_CODEC_ID_NONE == m_avctx->codec_id) return MRDA_STATUS_INVALID_DATA;
    m_avctx->codec_type = AVMEDIA_TYPE_VIDEO;
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegDecodeService::InitCodec()
{
    int32_t ret = 0;
    m_avctx = avcodec_alloc_context3(NULL);
    if (m_avctx == nullptr) {
        MRDA_LOG(LOG_ERROR, "Failed to allocate memory for codec context");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != SetDecParams()) {
        MRDA_LOG(LOG_ERROR, "Failed to set codec params");
        return MRDA_STATUS_INVALID_DATA;
    }

    const AVCodec *decoder = avcodec_find_decoder(m_avctx->codec_id);
    if (decoder == nullptr) {
        MRDA_LOG(LOG_ERROR, "Failed to find decoder for codec ID %d", m_avctx->codec_id);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // vaapi device type
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VAAPI;
    if ((type = av_hwdevice_find_type_by_name("vaapi")) == AV_HWDEVICE_TYPE_NONE) {
        MRDA_LOG(LOG_ERROR, "Failed to find HW device type vaapi");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    std::string device_str = "/dev/dri/renderD";
    uint32_t device_id = 128 + m_taskInfo.taskDevice.deviceID; // deviceID starts from 0
    device_str += std::to_string(device_id);

    ret = av_hwdevice_ctx_create(&m_hwDeviceCtx, type, device_str.c_str(), NULL, 0);
    if (ret < 0) {
        MRDA_LOG(LOG_ERROR, "Failed to create HW device context");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    const AVCodecHWConfig *config;
    for (int32_t i = 0;; i++) {
        config = avcodec_get_hw_config(decoder, i);
        if (config == nullptr) {
            MRDA_LOG(LOG_ERROR, "Decoder %s does not support device type %s.",
                    decoder->name, av_hwdevice_get_type_name(type));
            return MRDA_STATUS_INVALID_DATA;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            break;
        }
    }

    m_avctx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);
    if (!m_avctx->hw_device_ctx) {
        MRDA_LOG(LOG_ERROR, "Failed to wrap HW device context");
        return MRDA_STATUS_INVALID_DATA;
    }

    ret = avcodec_open2(m_avctx, decoder, NULL);
    if (ret < 0) {
        MRDA_LOG(LOG_ERROR, "Failed to open codec for decoder");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

void* HostFFmpegDecodeService::DecodeThread()
{
    while (!m_isStop)
    {
        // get one packet
        std::shared_ptr<FrameBufferData> packet = nullptr;
        AVPacket *av_packet = nullptr;
        if (m_isEOS == false)
        {
            {
                if (m_inFrameBufferDataList.empty())
                {
                    usleep(5 * 1000);//5ms
                    continue;
                }
                std::unique_lock<std::mutex> lock(m_inMutex);
                packet = m_inFrameBufferDataList.front();
                m_inFrameBufferDataList.pop_front();
#ifdef _ENABLE_TRACE_
                if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "MRDA trace log: pop front frame in host decoding service input queue, pts: %lu, in dev path: %s", packet->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
                if (packet->IsEOS())
                {
                    MRDA_LOG(LOG_INFO, "Get EOS frame!!!!!!!!\n");
                    m_isEOS = true;
                    continue;
                }
            }
            // tranfer packet to AVPacket
            av_packet = GetPacketForDecode(packet);
        }
        // decode one frame
        if (MRDA_STATUS_SUCCESS != DecodeOneFrame(av_packet))
        {
            MRDA_LOG(LOG_ERROR, "DecodeOneFrame failed!");
            m_isStop = true;
            return nullptr;
        }
        else
        {
            // unref frame
            UnRefInputFrame(packet);
        }
    }
    return nullptr;
}


AVPacket* HostFFmpegDecodeService::GetPacketForDecode(std::shared_ptr<FrameBufferData> packet)
{
    if (packet == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input data is null!");
        return nullptr;
    }

    AVPacket *av_packet = av_packet_alloc();
    if (packet == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate AVPacket.");
        return nullptr;
    }
    // packet -> av_packet
    av_packet->stream_index = 0;
    av_packet->pts = packet->Pts();
    av_packet->dts = packet->Pts();
    if (packet->MemBuffer() == nullptr) return nullptr;
    av_packet->size = packet->MemBuffer()->OccupiedSize();
    size_t base_offset = packet->MemBuffer()->MemOffset();
    av_packet->data = reinterpret_cast<uint8_t*>(m_inShmMem) + packet->MemBuffer()->MemOffset();

    return av_packet;
}

MRDAStatus HostFFmpegDecodeService::ColorSpaceConvert(AVPixelFormat in_pix_fmt, AVPixelFormat out_pix_fmt, AVFrame *in_frame, AVFrame* out_frame)
{
    if (in_frame == nullptr) return MRDA_STATUS_INVALID_DATA;

    int width = in_frame->width;
    int height = in_frame->height;
    struct SwsContext *sws_ctx = sws_getContext(width, height, in_pix_fmt,
                                                width, height, out_pix_fmt,
                                                SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        MRDA_LOG(LOG_ERROR, "Could not initialize sws context");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    sws_scale(sws_ctx, in_frame->data, in_frame->linesize, 0, height, out_frame->data, out_frame->linesize);
    sws_freeContext(sws_ctx);

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegDecodeService::DecodeOneFrame(AVPacket* packet)
{
    if (packet == nullptr && m_isEOS == false)
    {
        MRDA_LOG(LOG_ERROR, "packet is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }

#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr && packet != nullptr) MRDA_LOG(LOG_INFO, "MRDA trace log: begin avcodec send packet in FFmpeg decode service, pts: %lu, in dev path: %s", packet->pts, m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    if (avcodec_send_packet(m_avctx, packet) < 0)
    {
        MRDA_LOG(LOG_ERROR, "avcodec_send_packet failed");
        av_packet_free(&packet);
        return MRDA_STATUS_INVALID_DATA;
    }

    av_packet_free(&packet);

    int ret = 0;
    while (ret >= 0)
    {
        AVFrame *hw_frame = av_frame_alloc();
        AVFrame *sw_frame = av_frame_alloc();
        if (hw_frame == nullptr || sw_frame == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "av frame is nullptr");
            return MRDA_STATUS_INVALID_DATA;
        }
        ret = avcodec_receive_frame(m_avctx, hw_frame);
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
                MRDA_LOG(LOG_INFO, "Stop decode thread!!!");
            }
            // MRDA_LOG(LOG_INFO, "avcodec_receive_packet EOF");
            av_frame_free(&hw_frame);
            av_frame_free(&sw_frame);
            break;
        }
        else if (ret < 0)
        {
            MRDA_LOG(LOG_ERROR, "avcodec_receive_frame failed");
            av_frame_free(&hw_frame);
            av_frame_free(&sw_frame);
            m_isStop = true;
            return MRDA_STATUS_INVALID_DATA;
        }
        else
        {
#ifdef _ENABLE_TRACE_
            if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "MRDA trace log: complete avcodec receive frame in FFmpeg decode service, pts: %lu, in dev path: %s", hw_frame->pts, m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
            if ((ret = av_hwframe_transfer_data(sw_frame, hw_frame, 0)) < 0) {
                MRDA_LOG(LOG_ERROR, "Error transferring the data to system memory");
                av_frame_free(&hw_frame);
                av_frame_free(&sw_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            WriteToOutputShareMemoryBuffer(sw_frame);
            av_frame_free(&hw_frame);
            av_frame_free(&sw_frame);
            // MRDA_LOG(LOG_INFO, "decode one frame, frameNum = %d", m_frameNum);
            m_frameNum++;
        }
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegDecodeService::CopyFrameToMemory(AVFrame* frame, std::shared_ptr<FrameBufferData> data)
{
    if (frame == nullptr || data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "frame memory copy input is empty");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (data->MemBuffer() == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "data->MemBuffer() is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }

    size_t mem_offset = data->MemBuffer()->MemOffset();

    int width = frame->width;
    int height = frame->height;
    size_t out_frame_size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width,
                                        frame->height, 1);

    int ret = av_image_copy_to_buffer((uint8_t*)m_outShmMem + mem_offset, out_frame_size,
                                      (const uint8_t * const *)frame->data,
                                      (const int *)frame->linesize, (AVPixelFormat)frame->format,
                                      frame->width, frame->height, 1);
    if (ret < 0)
    {
        MRDA_LOG(LOG_ERROR, "av_image_copy_to_buffer failed");
        return MRDA_STATUS_INVALID_DATA;
    }

    data->MemBuffer()->SetOccupiedSize(out_frame_size);
    // debug
    fwrite(m_outShmMem + mem_offset, 1, out_frame_size, debug_file);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostFFmpegDecodeService::WriteToOutputShareMemoryBuffer(AVFrame* frame)
{
    if (frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "frame is nullptr");
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
    if (m_mediaParams == nullptr) return MRDA_STATUS_INVALID_DATA;

    AVPixelFormat out_pix_fmt = GetColorFormat(m_mediaParams->decodeParams.color_format);

    AVFrame *out_frame = nullptr;
    bool out_frame_alloc = false;
    // No need to CSC
    if (out_pix_fmt == (AVPixelFormat)frame->format)
    {
        out_frame = frame;
    }
    // Do CSC
    else {
        out_frame = av_frame_alloc();
        if (out_frame == nullptr) return MRDA_STATUS_INVALID_DATA;
        out_frame_alloc = true;
        out_frame->format = out_pix_fmt;
        out_frame->width = frame->width;
        out_frame->height = frame->height;
        size_t out_frame_size = av_image_get_buffer_size(out_pix_fmt, frame->width, frame->height, 1);
        // allocate new buffer for video
        if (av_frame_get_buffer(out_frame, 0) < 0)
        {
            MRDA_LOG(LOG_ERROR, "Failed to allocate frame data.");
            av_frame_free(&out_frame);
            return MRDA_STATUS_INVALID_DATA;
        }

        if (MRDA_STATUS_SUCCESS != ColorSpaceConvert((AVPixelFormat)frame->format, out_pix_fmt, frame, out_frame))
        {
            MRDA_LOG(LOG_ERROR, "ColorSpaceConvert failed");
            av_frame_free(&out_frame);
            return MRDA_STATUS_INVALID_DATA;
        }
    }
    RefOutputFrame(data);

    // Copy output frame -> data (m_outShmMem)
    if (MRDA_STATUS_SUCCESS != CopyFrameToMemory(out_frame, data))
    {
        MRDA_LOG(LOG_ERROR, "CopyFrameToMemory failed");
        if (out_frame_alloc) av_frame_free(&out_frame);
        return MRDA_STATUS_INVALID_DATA;
    }

    if (out_frame_alloc) av_frame_free(&out_frame);
    // update output buffer list
#ifdef _ENABLE_TRACE_
    if (m_mediaParams != nullptr) MRDA_LOG(LOG_INFO, "MRDA trace log: push back frame in host decoding service output queue, pts: %lu, in dev path: %s", data->Pts(), m_mediaParams->shareMemoryInfo.in_mem_dev_path.c_str());
#endif
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outFrameBufferDataList.push_back(data);
    // MRDA_LOG(LOG_INFO, "Push back output buffer at pts %llu", data->Pts());

    return MRDA_STATUS_SUCCESS;
}

VDI_NS_END

#endif // _FFMPEG_SUPPORT_