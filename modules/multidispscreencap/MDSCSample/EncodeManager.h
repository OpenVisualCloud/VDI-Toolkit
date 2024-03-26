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

#ifndef _ENCODEMANAGER_H
#define _ENCODEMANAGER_H

#include <iostream>
#include "d3d9.h"
#include "BufferQueue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

enum Encode_Type
{
    FFmpeg_SW = 0,
    FFmpeg_HW = 1,
    YUV_Raw_Data = 2,
    Point_Cloud = 3
};

struct Encode_Params
{
    Encode_Type encode_type = FFmpeg_SW;
    int width = 1920;
    int height = 1080;
    int qp = 26;
    int bitrate = 3000000;
    int fps = 30;
    int gop = 30;
    int qmin = 10;
    int qmax = 51;
    float qcompress = (float)0.6;
    int thread_count = 0;
    uint64_t st_timestamp = 0;
    bool operator == (const Encode_Params& encode_params)
    {
        return (width == encode_params.width) &&
            (height = encode_params.height) &&
            (qp == encode_params.qp) &&
            (bitrate == encode_params.bitrate) &&
            (fps == encode_params.fps) &&
            (gop == encode_params.gop) &&
            (qmin == encode_params.qmin) &&
            (qmax == encode_params.qmax) &&
            (qcompress == encode_params.qcompress) &&
            (thread_count == encode_params.thread_count) &&
            (st_timestamp == encode_params.st_timestamp) &&
            (thread_count == encode_params.thread_count);
    }
};


class EncodeManager
{
public:
    EncodeManager();
    ~EncodeManager();
    int Init(const Encode_Params& encode_params, std::string output, bool isRtsp);
    int Encode(uint8_t *data, uint64_t timestamp);
    int End_video_output();

private:
    int  Open_encode_context(const Encode_Params& encode_params);
    int  Open_video_output();
    int  Init_swrContext();
    int  BGRA2YUV(uint8_t* bgra_img, int width, int height);

    AVCodecContext* m_codec_ctx;
    SwsContext* m_sws_ctx;
    AVFormatContext* m_video_ofmt_ctx;
    AVStream* m_video_stream;
    std::string m_output_filename;
    std::string m_rtsp_url;
    AVFrame* m_frame;
    AVPacket* m_pkt;
    uint64_t m_pts;
    bool m_isRtsp;
    uint32_t m_threadId;
};

#endif