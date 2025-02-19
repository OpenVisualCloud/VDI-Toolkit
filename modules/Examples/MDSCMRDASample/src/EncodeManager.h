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
#include "BufferQueue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
}

enum Encode_Type
{
    FFmpeg_SW = 0,
    FFmpeg_MRDA = 1,
    VPL_MRDA = 2,
    QES = 3,
};

enum Encode_CodecId
{
    AVC = 0,
    HEVC = 1,
};

struct Encode_Params
{
    Encode_Type encode_type = FFmpeg_SW;
    std::string output_filename = "";
    std::string rtsp_url = "";
    bool bIsRtsp = false;
    int width = 1920;
    int height = 1080;
    std::string codec_id = "";
    std::string target_usage = "";
    std::string codec_profile = "";
    std::string input_color_format = "";
    std::string output_pixel_format = "";
    std::string rc_mode = "";
    uint32_t qp = 26;
    uint32_t bitrate = 3000000;
    uint32_t max_b_frames = 0;
    int framerate_num = 30;
    int  framerate_den = 1;
    uint32_t gop = 30;
    uint32_t async_depth = 4;
    int threadId = 0;
    uint64_t st_timestamp = 0;
    bool operator == (const Encode_Params& encode_params)
    {
        return ((encode_type == encode_params.encode_type) &&
               (!strcmp(output_filename.c_str(), encode_params.output_filename.c_str())) &&
               (!strcmp(rtsp_url.c_str(), encode_params.rtsp_url.c_str())) &&
               (bIsRtsp == encode_params.bIsRtsp) &&
               (width == encode_params.width) &&
               (height = encode_params.height) &&
               (!strcmp(codec_id.c_str(), encode_params.codec_id.c_str())) &&
               (!strcmp(target_usage.c_str(), encode_params.target_usage.c_str())) &&
               (!strcmp(codec_profile.c_str(), encode_params.codec_profile.c_str())) &&
               (!strcmp(input_color_format.c_str(), encode_params.input_color_format.c_str())) &&
               (!strcmp(output_pixel_format.c_str(), encode_params.output_pixel_format.c_str())) &&
               (!strcmp(rc_mode.c_str(), encode_params.rc_mode.c_str())) &&
               (qp == encode_params.qp) &&
               (bitrate == encode_params.bitrate) &&
               (max_b_frames == encode_params.max_b_frames) &&
               (framerate_num == encode_params.framerate_num) &&
               (framerate_den == encode_params.framerate_den) &&
               (gop == encode_params.gop) &&
               (async_depth == encode_params.async_depth) &&
               (threadId == encode_params.threadId)) &&
               (st_timestamp == encode_params.st_timestamp);
    }
};

class EncodeManager
{
public:
    EncodeManager();
    virtual ~EncodeManager();
    virtual int Init(const Encode_Params& encode_params);
    virtual int Encode(uint8_t* data, uint64_t timestamp);
    virtual int End_video_output();

protected:
    virtual int InitAVContext(const Encode_Params& encode_params);
    virtual void DestroyAVContext();
    int  Open_encode_context(const Encode_Params& encode_params);
    int  Open_video_output();
    int  Init_swrContext(AVPixelFormat srcFormat, AVPixelFormat dstFormat);
    int  ColorConvert(uint8_t* bgra_img, int width, int height);
    AVCodecID GetCodecId(std::string codecID);
    int GetCodecProfile(std::string codecProfile);
    AVPixelFormat GetCodecColorFormat(std::string colorFormat);

protected:
    AVCodecContext* m_pCodecCtx;
    SwsContext* m_pSwsCtx;
    AVFormatContext* m_pVideoOfmtCtx;
    AVStream* m_pVideoStream;
    std::string m_sOutputFilename;
    std::string m_sRtspUrl;
    AVFrame* m_pFrame;
    AVPacket* m_pPkt;
    uint64_t m_ulPts;
    bool m_bIsRtsp;
    uint32_t m_uThreadId;
    uint32_t m_uFrameNum;
    bool m_bNeedSws;
};

#endif