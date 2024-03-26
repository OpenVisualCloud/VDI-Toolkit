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

#include "EncodeManager.h"

EncodeManager::EncodeManager() : m_threadId(0),
                                 m_pts(0),
                                 m_isRtsp(false),
                                 m_rtsp_url(""),
                                 m_output_filename(""),
                                 m_pkt(nullptr),
                                 m_frame(nullptr),
                                 m_codec_ctx(nullptr),
                                 m_sws_ctx(nullptr),
                                 m_video_ofmt_ctx(nullptr),
                                 m_video_stream(nullptr)
{
}

int EncodeManager::Init(const Encode_Params& encode_params, std::string output, bool isRtsp)
{
    m_threadId = encode_params.thread_count;
    m_isRtsp = isRtsp;

    if (Open_encode_context(encode_params) != 0)
    {
        printf("[thread][%d], Could not open encodec packet\n", m_threadId);
        return -1;
    }

    if (Init_swrContext() != 0)
    {
        printf("[thread][%d], Could not init swrContext\n", m_threadId);
        return -1;
    }

    if (m_isRtsp)
    {
        m_rtsp_url = output;
        m_pts = encode_params.st_timestamp;
    }
    else
    {
        m_output_filename = output;
        m_pts = 0;
    }

    if (Open_video_output() != 0)
    {
        printf("[thread][%d], Could not open video output\n", m_threadId);
        return -1;
    }

    m_pkt = av_packet_alloc();
    if (!m_pkt)
    {
        printf("[thread][%d], Could not allocate video packet\n", m_threadId);
        return -1;
    }

    m_frame = av_frame_alloc();
    if (!m_frame)
    {
        printf("[thread][%d], Could not allocate video frame\n", m_threadId);
        return -1;
    }
    m_frame->format = AV_PIX_FMT_YUV420P;
    m_frame->width = encode_params.width;
    m_frame->height = encode_params.height;

    if (av_frame_get_buffer(m_frame, 0) < 0)
    {
        printf("[thread][%d], Could not allocate the video frame data\n", m_threadId);
        return - 1;
    }

    printf("[thread][%d], ffmpeg software encoder with encode_params is enabled\n", m_threadId);
    return 0;
}

EncodeManager::~EncodeManager()
{
    printf("[thread][%d], ~EncodeManager\n", m_threadId);
    av_frame_free(&m_frame);
    av_packet_free(&m_pkt);
    sws_freeContext(m_sws_ctx);
    if (m_video_ofmt_ctx)
    {
        if (m_video_ofmt_ctx->pb) {
            avio_close(m_video_ofmt_ctx->pb);
        }
    }
    avformat_free_context(m_video_ofmt_ctx);
    avcodec_free_context(&m_codec_ctx);
}

int EncodeManager::Encode(uint8_t* data, uint64_t timestamp)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> starttp = std::chrono::high_resolution_clock::now();
    BGRA2YUV(data, m_codec_ctx->width, m_codec_ctx->height);
    std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
    uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();

    int ret = avcodec_send_frame(m_codec_ctx, m_frame);
    if (ret < 0)
    {
        printf("[thread][%d], Error sending a frame for encoding\n", m_threadId);
        return -1;
    }

    while (avcodec_receive_packet(m_codec_ctx, m_pkt) >= 0)
    {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return false;
        else if (ret < 0)
        {
            printf("[thread][%d], Error during encoding\n", m_threadId);
            return -1;
        }

        if (m_isRtsp)
        {
            m_pkt->pts = timestamp - m_pts;
            m_pkt->dts = m_pkt->pts;
        }
        else
        {
            m_pkt->pts = av_rescale_q_rnd(m_pkt->pts, m_codec_ctx->time_base, m_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pkt->dts = av_rescale_q_rnd(m_pkt->dts, m_codec_ctx->time_base, m_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pkt->duration = av_rescale_q(m_pkt->duration, m_codec_ctx->time_base, m_video_stream->time_base);
            m_pts++;
        }
        av_interleaved_write_frame(m_video_ofmt_ctx, m_pkt);
        av_packet_unref(m_pkt);
    }
    return 0;
}

int EncodeManager::Open_encode_context(const Encode_Params& encode_params)
{
    const AVCodec* encodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encodec)
    {
        printf("[thread][%d], avcodec_find_encoder AV_CODEC_ID_H264 failed!\n", m_threadId);
        return -1;
    }

    m_codec_ctx = avcodec_alloc_context3(encodec);
    if (!m_codec_ctx)
    {
        printf("[thread][%d], avcodec_alloc_context3 failed!!\n", m_threadId);
        return -1;
    }

    m_codec_ctx->bit_rate = encode_params.bitrate;
    m_codec_ctx->width = encode_params.width;
    m_codec_ctx->height = encode_params.height;
    /* frames per second */

    m_codec_ctx->time_base = { 1, encode_params.fps };
    m_codec_ctx->framerate = { encode_params.fps, 1 };

    m_codec_ctx->gop_size = encode_params.gop;
    m_codec_ctx->max_b_frames = 0;
    m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    m_codec_ctx->qmin = encode_params.qmin;
    m_codec_ctx->qmax = encode_params.qmax;
    m_codec_ctx->qcompress = encode_params.qcompress;

    m_codec_ctx->thread_count = encode_params.thread_count;

    if (encodec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(m_codec_ctx->priv_data, "tune", "zerolatency", 0);
    }

    int ret = avcodec_open2(m_codec_ctx, encodec, NULL);
    if (ret < 0)
    {
        char errStr[256] = { 0 };
        av_strerror(ret, errStr, sizeof(errStr));
        printf("[thread][%d], error avcodec_open2 (%s)\n", m_threadId, errStr);
        return -1;
    }
    return 0;
}

int EncodeManager::Init_swrContext()
{
    m_sws_ctx = sws_getContext(m_codec_ctx->width, m_codec_ctx->height, AV_PIX_FMT_RGB32, m_codec_ctx->width, m_codec_ctx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!m_sws_ctx) {
        printf("[thread][%d], Failed to get sws context\n", m_threadId);
    }
    return 0;
}

int EncodeManager::BGRA2YUV(uint8_t* bgra_img, int width, int height)
{
    uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
    indata[0] = bgra_img;
    int insize[AV_NUM_DATA_POINTERS] = { 0 };
    insize[0] = width * 4;
    int ret = 0;
    ret = sws_scale(m_sws_ctx, indata, insize, 0, height, m_frame->data, m_frame->linesize);
    m_frame->pts = m_pts;
    return 0;
}

int EncodeManager::Open_video_output()
{
    if (m_isRtsp)
    {
        if (avformat_alloc_output_context2(&m_video_ofmt_ctx, NULL, "rtsp", m_rtsp_url.c_str()) < 0)
        {
            printf("[thread][%d], Failed to alloc rtsp output format context\n", m_threadId);
            return -1;
        }
        av_opt_set(m_video_ofmt_ctx->priv_data, "rtsp_transport", "udp", 0);
    }
    else
    {
        if (avformat_alloc_output_context2(&m_video_ofmt_ctx, NULL, NULL, m_output_filename.c_str()) < 0)
        {
            printf("[thread][%d], Failed to alloc file output format context\n", m_threadId);
            return -1;
        }
    }


    m_video_stream = avformat_new_stream(m_video_ofmt_ctx, NULL);
    if (NULL == m_video_stream)
    {
        printf("[thread][%d], Failed to add new stream\n", m_threadId);
        return -1;
    }
    m_video_stream->id = m_video_ofmt_ctx->nb_streams - 1;

    if (avcodec_parameters_from_context(m_video_stream->codecpar, m_codec_ctx) < 0)
    {
        printf("[thread][%d], Failed to copy codec parameters\n", m_threadId);
        return  -1;
    }
    m_video_stream->time_base = m_codec_ctx->time_base;
    m_video_stream->r_frame_rate = m_codec_ctx->framerate;
    m_video_stream->codecpar->codec_tag = 0;

    if (m_isRtsp)
    {
        if(!(m_video_ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&m_video_ofmt_ctx->pb, m_rtsp_url.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                printf("[thread][%d], Failed to alloc rtsp avio\n", m_threadId);
                return -1;
            }
        }
    }
    else
    {
        if (avio_open(&m_video_ofmt_ctx->pb, m_output_filename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            printf("[thread][%d], Failed to open file avio\n", m_threadId);
            return -1;
        }
    }

    av_dump_format(m_video_ofmt_ctx, 0, m_rtsp_url.c_str(), 1);

    if (avformat_write_header(m_video_ofmt_ctx, NULL) < 0)
    {
        printf("[thread][%d], Failed to write header\n", m_threadId);
        return -1;
    }

    return 0;
}

int EncodeManager::End_video_output()
{
    av_write_trailer(m_video_ofmt_ctx);
    return 0;
}
