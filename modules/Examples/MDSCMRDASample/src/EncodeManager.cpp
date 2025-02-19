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
#include <string>

EncodeManager::EncodeManager() : m_uThreadId(0),
                                 m_ulPts(0),
                                 m_uFrameNum(0),
                                 m_bIsRtsp(false),
                                 m_bNeedSws(false),
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

EncodeManager::~EncodeManager()
{
    printf("[thread][%d], ~EncodeManager\n", m_uThreadId);
    DestroyAVContext();
}

int EncodeManager::Init(const Encode_Params& encode_params)
{
    if (InitAVContext(encode_params) < 0)
    {
        printf("[thread][%d], Failed to init EncodeManager\n", m_uThreadId);
        return -1;
    }

    if (strcmp(encode_params.input_color_format.c_str(), encode_params.output_pixel_format.c_str()))
    {
        m_bNeedSws = true;
        if (Init_swrContext(GetCodecColorFormat(encode_params.input_color_format), GetCodecColorFormat(encode_params.output_pixel_format)) < 0)
        {
            printf("[thread][%d], Failed to init swrContext\n", m_uThreadId);
            return -1;
        }
    }

    return 0;
}

int EncodeManager::InitAVContext(const Encode_Params& encode_params)
{
    m_uThreadId = encode_params.threadId;
    m_bIsRtsp = encode_params.bIsRtsp;

    if (Open_encode_context(encode_params) != 0)
    {
        printf("[thread][%d], Could not open encodec packet\n", m_uThreadId);
        return -1;
    }

    if (m_bIsRtsp)
    {
        m_sRtspUrl = encode_params.rtsp_url + std::to_string(m_uThreadId);
        m_ulPts = encode_params.st_timestamp;
    }
    else
    {
        m_sOutputFilename = std::to_string(m_uThreadId) + "_" + encode_params.output_filename;
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
    m_pFrame->format = GetCodecColorFormat(encode_params.output_pixel_format);
    m_pFrame->width = encode_params.width;
    m_pFrame->height = encode_params.height;

    if (av_frame_get_buffer(m_pFrame, 0) < 0)
    {
        printf("[thread][%d], Could not allocate the video frame data\n", m_uThreadId);
        return -1;
    }

    printf("[thread][%d], InitAVContext successed\n", m_uThreadId);
    return 0;
}

void EncodeManager::DestroyAVContext()
{
    printf("[thread][%d], DestroyAVContext\n", m_uThreadId);
    if (m_pFrame)
    {
        av_frame_free(&m_pFrame);
    }
    if (m_pPkt)
    {
        av_packet_free(&m_pPkt);
    }
    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
    }
    if (m_pVideoOfmtCtx)
    {
        if (m_pVideoOfmtCtx->pb) {
            avio_close(m_pVideoOfmtCtx->pb);
        }
    }
    if (m_pVideoOfmtCtx)
    {
        avformat_free_context(m_pVideoOfmtCtx);
    }
    if (m_pCodecCtx)
    {
        avcodec_free_context(&m_pCodecCtx);
    }

    return;
}

int EncodeManager::Encode(uint8_t* data, uint64_t timestamp)
{
#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> cv_starttp = std::chrono::high_resolution_clock::now();
#endif
    if (m_bNeedSws)
    {
        ColorConvert(data, m_pCodecCtx->width, m_pCodecCtx->height);
    }
    else
    {
        av_image_fill_arrays(m_pFrame->data, m_pFrame->linesize, data, m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height, 1);
        m_pFrame->pts = m_ulPts;
    }
#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> cv_endtp = std::chrono::high_resolution_clock::now();
    uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(cv_endtp - cv_starttp).count();
    printf("[thread][%d], frame %u, ColorConvert time cost %fms\n", m_uThreadId, m_uFrameNum, timecost / 1000.0);
#endif

    int ret = avcodec_send_frame(m_pCodecCtx, m_pFrame);
    if (ret < 0)
    {
        printf("[thread][%d], Error sending a frame for encoding\n", m_uThreadId);
        return -1;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(m_pCodecCtx, m_pPkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            m_uFrameNum++;
            return 0;
        }
        else if (ret < 0)
        {
            printf("[thread][%d], frame %u, Error during encoding\n", m_uThreadId, m_uFrameNum);
            m_uFrameNum++;
            return -1;
        }

        if (m_bIsRtsp)
        {
            m_pPkt->pts = timestamp - m_ulPts;
            m_pPkt->dts = m_pPkt->pts;

            std::chrono::time_point<std::chrono::high_resolution_clock> enc_endtp = std::chrono::high_resolution_clock::now();
#ifdef _ENABLE_TRACE_
            uint64_t enc_timecost = std::chrono::duration_cast<std::chrono::microseconds>(enc_endtp - cv_endtp).count();
            printf("[thread][%d], frame %u, rtsp mode Encode time cost %fms\n", m_uThreadId, m_uFrameNum, enc_timecost / 1000.0);
#endif
        }
        else
        {
            m_pPkt->pts = av_rescale_q_rnd(m_pPkt->pts, m_pCodecCtx->time_base, m_pVideoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pPkt->dts = av_rescale_q_rnd(m_pPkt->dts, m_pCodecCtx->time_base, m_pVideoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            m_pPkt->duration = av_rescale_q(m_pPkt->duration, m_pCodecCtx->time_base, m_pVideoStream->time_base);
            m_ulPts++;
        }
#ifdef _ENABLE_TRACE_
        std::chrono::time_point<std::chrono::high_resolution_clock> wt_starttp = std::chrono::high_resolution_clock::now();
#endif
        av_interleaved_write_frame(m_pVideoOfmtCtx, m_pPkt);
        av_packet_unref(m_pPkt);
#ifdef _ENABLE_TRACE_
        std::chrono::time_point<std::chrono::high_resolution_clock> wt_endtp = std::chrono::high_resolution_clock::now();
        uint64_t wt_timecost = std::chrono::duration_cast<std::chrono::microseconds>(wt_endtp - wt_starttp).count();
        printf("[thread][%d], frame %u, write frame time cost %fms\n", m_uThreadId, m_uFrameNum, wt_timecost / 1000.0);
#endif
    }
    m_uFrameNum++;
    return 0;
}

int EncodeManager::Open_encode_context(const Encode_Params& encode_params)
{
    int ret = 0;
    const AVCodec* encodec = avcodec_find_encoder(GetCodecId(encode_params.codec_id));
    if (!encodec)
    {
        printf("[thread][%d], avcodec_find_encoder failed, codec_id %s!\n", m_uThreadId, encode_params.codec_id.c_str());
        return -1;
    }

    m_pCodecCtx = avcodec_alloc_context3(encodec);
    if (!m_pCodecCtx)
    {
        printf("[thread][%d], avcodec_alloc_context3 failed!\n", m_uThreadId);
        return -1;
    }

    m_pCodecCtx->width = encode_params.width;
    m_pCodecCtx->height = encode_params.height;
    /* frames per second */

    m_pCodecCtx->time_base = { encode_params.framerate_den, encode_params.framerate_num };
    m_pCodecCtx->framerate = { encode_params.framerate_num, encode_params.framerate_den };

    m_pCodecCtx->gop_size = encode_params.gop;
    m_pCodecCtx->max_b_frames = encode_params.max_b_frames;
    m_pCodecCtx->pix_fmt = GetCodecColorFormat(encode_params.output_pixel_format);
    m_pCodecCtx->profile = GetCodecProfile(encode_params.codec_profile);

    if (!strcmp(encode_params.rc_mode.c_str(), "VBR"))
    {
        // CBR
        m_pCodecCtx->bit_rate = encode_params.bitrate * 1000;
        m_pCodecCtx->rc_min_rate = m_pCodecCtx->bit_rate;
        m_pCodecCtx->rc_max_rate = m_pCodecCtx->bit_rate;
        m_pCodecCtx->bit_rate_tolerance = m_pCodecCtx->bit_rate;
        m_pCodecCtx->rc_buffer_size = m_pCodecCtx->bit_rate * 2;
        m_pCodecCtx->rc_initial_buffer_occupancy = m_pCodecCtx->rc_buffer_size * 3 / 4;
    }
    else if (!strcmp(encode_params.rc_mode.c_str(), "CQP"))
    {
        // CQP
        ret = av_opt_set(m_pCodecCtx->priv_data, "qp", std::to_string(encode_params.qp).c_str(), AV_OPT_SEARCH_CHILDREN);
    }
    else
    {
        printf("[thread][%d], Unknown rc mode %s!", m_uThreadId, encode_params.rc_mode.c_str());
        return -1;
    }

    if (!strcmp(encode_params.target_usage.c_str(), "balanced"))
    {
        ret = av_opt_set(m_pCodecCtx->priv_data, "preset", "medium", 0);
    }
    else if (!strcmp(encode_params.target_usage.c_str(), "quality"))
    {
        ret = av_opt_set(m_pCodecCtx->priv_data, "preset", "veryslow", 0);
    }
    else if (!strcmp(encode_params.target_usage.c_str(), "speed"))
    {
        ret = av_opt_set(m_pCodecCtx->priv_data, "preset", "veryfast", 0);
    }

    if (encodec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0);
    }

    if (ret < 0)
    {
        char errStr[256] = { 0 };
        av_strerror(ret, errStr, sizeof(errStr));
        printf("[thread][%d], error av_opt_set (%s)", m_uThreadId, errStr);
        return -1;
    }

    ret = avcodec_open2(m_pCodecCtx, encodec, NULL);
    if (ret < 0)
    {
        char errStr[256] = { 0 };
        av_strerror(ret, errStr, sizeof(errStr));
        printf("[thread][%d], error avcodec_open2 (%s)", m_uThreadId, errStr);
        return -1;
    }
    return 0;
}

int EncodeManager::Init_swrContext(AVPixelFormat srcFormat, AVPixelFormat dstFormat)
{
    m_pSwsCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, srcFormat, m_pCodecCtx->width, m_pCodecCtx->height, dstFormat, SWS_BILINEAR, NULL, NULL, NULL);
    if (!m_pSwsCtx)
    {
        printf("[thread][%d], Failed to get sws context", m_uThreadId);
    }
    return 0;
}

int EncodeManager::ColorConvert(uint8_t* bgra_img, int width, int height)
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
        if (!(m_pVideoOfmtCtx->oformat->flags & AVFMT_NOFILE))
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

AVCodecID EncodeManager::GetCodecId(std::string codecID)
{
    if (!_stricmp(codecID.c_str(), "avc") || !strcmp(codecID.c_str(), "h264"))
    {
        return AV_CODEC_ID_H264;
    }
    else if (!_stricmp(codecID.c_str(), "hevc") || !strcmp(codecID.c_str(), "h265"))
    {
        return AV_CODEC_ID_HEVC;
    }
    else
    {
        printf("[thread][%d], Unknown or unsupported codec id!\n", m_uThreadId);
        return AV_CODEC_ID_NONE;
    }
}

int EncodeManager::GetCodecProfile(std::string codecProfile)
{
    if (!strcmp(codecProfile.c_str(), "avc:main"))
    {
        return FF_PROFILE_H264_MAIN;
    }
    else if (!strcmp(codecProfile.c_str(), "avc:high"))
    {
        return FF_PROFILE_H264_HIGH;
    }
    else if (!strcmp(codecProfile.c_str(), "hevc:main"))
    {
        return FF_PROFILE_HEVC_MAIN;
    }
    else if (!strcmp(codecProfile.c_str(), "hevc:main10"))
    {
        return FF_PROFILE_HEVC_MAIN_10;
    }
    else
    {
        printf("[thread][%d], Unknown or unsupported codec profile!\n", m_uThreadId);
        return 0;
    }
}

AVPixelFormat EncodeManager::GetCodecColorFormat(std::string colorFormat)
{
    if (!strcmp(colorFormat.c_str(), "nv12"))
    {
        return AV_PIX_FMT_NV12;
    }
    else if (!strcmp(colorFormat.c_str(), "yuv420p"))
    {
        return AV_PIX_FMT_YUV420P;
    }
    else if (!strcmp(colorFormat.c_str(), "rgb32"))
    {
        return AV_PIX_FMT_BGRA;
    }
    else
    {
        printf("[thread][%d], Unknown or unsupported color format!\n", m_uThreadId);
        return AV_PIX_FMT_NONE;
    }
}