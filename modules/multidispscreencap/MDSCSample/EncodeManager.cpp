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

EncodeManager::EncodeManager() : m_uThreadId(0),
                                 m_ulPts(0),
                                 m_bIsRtsp(false),
                                 m_sRtspUrl(""),
                                 m_sOutputFilename(""),
                                 m_pPkt(nullptr),
                                 m_pFrame(nullptr),
                                 m_pCodecCtx(nullptr),
                                 m_pSwsCtx(nullptr),
                                 m_pVideoOfmtCtx(nullptr),
                                 m_pVideoStream(nullptr)
{
}

int EncodeManager::Init(const Encode_Params& encode_params, std::string output, bool isRtsp)
{
    m_uThreadId = encode_params.thread_count;
    m_bIsRtsp = isRtsp;

    if (Open_encode_context(encode_params) != 0)
    {
        printf("[thread][%d], Could not open encodec packet\n", m_uThreadId);
        return -1;
    }

    if (Init_swrContext() != 0)
    {
        printf("[thread][%d], Could not init swrContext\n", m_uThreadId);
        return -1;
    }

    if (m_bIsRtsp)
    {
        m_sRtspUrl = output;
        m_ulPts = encode_params.st_timestamp;
    }
    else
    {
        m_sOutputFilename = output;
        m_ulPts = 0;
    }

    if (Open_video_output() != 0)
    {
        printf("[thread][%d], Could not open video output\n", m_uThreadId);
        return -1;
    }

    m_pPkt = av_packet_alloc();
    if (!m_pPkt)
    {
        printf("[thread][%d], Could not allocate video packet\n", m_uThreadId);
        return -1;
    }

    m_pFrame = av_frame_alloc();
    if (!m_pFrame)
    {
        printf("[thread][%d], Could not allocate video frame\n", m_uThreadId);
        return -1;
    }
    m_pFrame->format = AV_PIX_FMT_YUV420P;
    m_pFrame->width = encode_params.width;
    m_pFrame->height = encode_params.height;

    if (av_frame_get_buffer(m_pFrame, 0) < 0)
    {
        printf("[thread][%d], Could not allocate the video frame data\n", m_uThreadId);
        return - 1;
    }

    printf("[thread][%d], ffmpeg software encoder with encode_params is enabled\n", m_uThreadId);
    return 0;
}

EncodeManager::~EncodeManager()
{
    printf("[thread][%d], ~EncodeManager\n", m_uThreadId);
    av_frame_free(&m_pFrame);
    av_packet_free(&m_pPkt);
    sws_freeContext(m_pSwsCtx);
    if (m_pVideoOfmtCtx)
    {
        if (m_pVideoOfmtCtx->pb) {
            avio_close(m_pVideoOfmtCtx->pb);
        }
    }
    avformat_free_context(m_pVideoOfmtCtx);
    avcodec_free_context(&m_pCodecCtx);
}

int EncodeManager::Encode(uint8_t* data, uint64_t timestamp)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> starttp = std::chrono::high_resolution_clock::now();
    BGRA2YUV(data, m_pCodecCtx->width, m_pCodecCtx->height);
    std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
    uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();

    int ret = avcodec_send_frame(m_pCodecCtx, m_pFrame);
    if (ret < 0)
    {
        printf("[thread][%d], Error sending a frame for encoding\n", m_uThreadId);
        return -1;
    }

    while (avcodec_receive_packet(m_pCodecCtx, m_pPkt) >= 0)
    {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return false;
        else if (ret < 0)
        {
            printf("[thread][%d], Error during encoding\n", m_uThreadId);
            return -1;
        }

        if (m_bIsRtsp)
        {
            m_pPkt->pts = timestamp - m_ulPts;
            m_pPkt->dts = m_pPkt->pts;
        }
        else
        {
            m_pPkt->pts = av_rescale_q_rnd(m_pPkt->pts, m_pCodecCtx->time_base, m_pVideoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pPkt->dts = av_rescale_q_rnd(m_pPkt->dts, m_pCodecCtx->time_base, m_pVideoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pPkt->duration = av_rescale_q(m_pPkt->duration, m_pCodecCtx->time_base, m_pVideoStream->time_base);
            m_ulPts++;
        }
        av_interleaved_write_frame(m_pVideoOfmtCtx, m_pPkt);
        av_packet_unref(m_pPkt);
    }
    return 0;
}

int EncodeManager::Open_encode_context(const Encode_Params& encode_params)
{
    const AVCodec* encodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encodec)
    {
        printf("[thread][%d], avcodec_find_encoder AV_CODEC_ID_H264 failed!\n", m_uThreadId);
        return -1;
    }

    m_pCodecCtx = avcodec_alloc_context3(encodec);
    if (!m_pCodecCtx)
    {
        printf("[thread][%d], avcodec_alloc_context3 failed!!\n", m_uThreadId);
        return -1;
    }

    m_pCodecCtx->bit_rate = encode_params.bitrate;
    m_pCodecCtx->width = encode_params.width;
    m_pCodecCtx->height = encode_params.height;
    /* frames per second */

    m_pCodecCtx->time_base = { 1, encode_params.fps };
    m_pCodecCtx->framerate = { encode_params.fps, 1 };

    m_pCodecCtx->gop_size = encode_params.gop;
    m_pCodecCtx->max_b_frames = 0;
    m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    m_pCodecCtx->qmin = encode_params.qmin;
    m_pCodecCtx->qmax = encode_params.qmax;
    m_pCodecCtx->qcompress = encode_params.qcompress;

    m_pCodecCtx->thread_count = encode_params.thread_count;

    if (encodec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0);
    }

    int ret = avcodec_open2(m_pCodecCtx, encodec, NULL);
    if (ret < 0)
    {
        char errStr[256] = { 0 };
        av_strerror(ret, errStr, sizeof(errStr));
        printf("[thread][%d], error avcodec_open2 (%s)\n", m_uThreadId, errStr);
        return -1;
    }
    return 0;
}

int EncodeManager::Init_swrContext()
{
    m_pSwsCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, AV_PIX_FMT_RGB32, m_pCodecCtx->width, m_pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!m_pSwsCtx) {
        printf("[thread][%d], Failed to get sws context\n", m_uThreadId);
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
    ret = sws_scale(m_pSwsCtx, indata, insize, 0, height, m_pFrame->data, m_pFrame->linesize);
    m_pFrame->pts = m_ulPts;
    return 0;
}

int EncodeManager::Open_video_output()
{
    if (m_bIsRtsp)
    {
        if (avformat_alloc_output_context2(&m_pVideoOfmtCtx, NULL, "rtsp", m_sRtspUrl.c_str()) < 0)
        {
            printf("[thread][%d], Failed to alloc rtsp output format context\n", m_uThreadId);
            return -1;
        }
        av_opt_set(m_pVideoOfmtCtx->priv_data, "rtsp_transport", "udp", 0);
    }
    else
    {
        if (avformat_alloc_output_context2(&m_pVideoOfmtCtx, NULL, NULL, m_sOutputFilename.c_str()) < 0)
        {
            printf("[thread][%d], Failed to alloc file output format context\n", m_uThreadId);
            return -1;
        }
    }


    m_pVideoStream = avformat_new_stream(m_pVideoOfmtCtx, NULL);
    if (NULL == m_pVideoStream)
    {
        printf("[thread][%d], Failed to add new stream\n", m_uThreadId);
        return -1;
    }
    m_pVideoStream->id = m_pVideoOfmtCtx->nb_streams - 1;

    if (avcodec_parameters_from_context(m_pVideoStream->codecpar, m_pCodecCtx) < 0)
    {
        printf("[thread][%d], Failed to copy codec parameters\n", m_uThreadId);
        return  -1;
    }
    m_pVideoStream->time_base = m_pCodecCtx->time_base;
    m_pVideoStream->r_frame_rate = m_pCodecCtx->framerate;
    m_pVideoStream->codecpar->codec_tag = 0;

    if (m_bIsRtsp)
    {
        if(!(m_pVideoOfmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&m_pVideoOfmtCtx->pb, m_sRtspUrl.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                printf("[thread][%d], Failed to alloc rtsp avio\n", m_uThreadId);
                return -1;
            }
        }
    }
    else
    {
        if (avio_open(&m_pVideoOfmtCtx->pb, m_sOutputFilename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            printf("[thread][%d], Failed to open file avio\n", m_uThreadId);
            return -1;
        }
    }

    av_dump_format(m_pVideoOfmtCtx, 0, m_sRtspUrl.c_str(), 1);

    if (avformat_write_header(m_pVideoOfmtCtx, NULL) < 0)
    {
        printf("[thread][%d], Failed to write header\n", m_uThreadId);
        return -1;
    }

    return 0;
}

int EncodeManager::End_video_output()
{
    av_write_trailer(m_pVideoOfmtCtx);
    return 0;
}
