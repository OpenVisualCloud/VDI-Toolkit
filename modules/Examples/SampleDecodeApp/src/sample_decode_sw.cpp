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

//!
//! \file sample_decode_sw.cpp
//! \brief a simple sample decode application to use ffmpeg sw decoder
//! \date 2024-12-6
//!

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <chrono>
#include <stdio.h>
#include <thread>

#include "common.h"

typedef struct INPUTCONFIG
{
    // I/O
    std::string sourceFile;
    std::string sinkFile;
    // decoding params
    uint32_t      frameNum;
    AVCodecID codec_id;                 //!< codec id
    int32_t  framerate_num;             //!< frame rate numerator
    int32_t  framerate_den;             //!< frame rate denominator
    uint32_t frame_width;               //!< width of frame
    uint32_t frame_height;              //!< height of frame
    AVPixelFormat  color_format;        //!< pixel color format
    TASKTYPE decode_type;               //!< decode type, 0 is ffmpeg decode, 1 is oneVPL decode
    bool dump_file;                     //!< dump file flag
} InputConfig;

typedef struct DECODERCONTEXT
{
    AVFormatContext *input_ctx;
    AVCodecContext *codec_ctx;
    AVFrame *av_frame;
    AVPacket *av_packet;
    int video_stream_index;
    uint32_t cur_frame_num;
    FILE* output_file;
} DecodeContext;

void PrintHelp()
{
    printf("%s", "Usage: .\\MRDASampleDecodeAppSW.exe [<options>]\n");
    printf("%s", "Options: \n");
    printf("%s", "    [--help]                                 - print help README document. \n");
    printf("%s", "    [-i input_file]                          - specifies input file. \n");
    printf("%s", "    [-o output_file]                         - specifies output file. \n");
    printf("%s", "    [--frameNum number_of_frames]            - specifies number of frames to process. \n");
    printf("%s", "    [--codecId codec_identifier]             - specifies the codec identifier. option: h265/hevc | h264/avc \n");
    printf("%s", "    [--fps frames_per_second]                - specifies the frames per second. \n");
    printf("%s", "    [--width frame_width]                    - specifies the frame width. \n");
    printf("%s", "    [--height frame_height]                  - specifies the frame height. \n");
    printf("%s", "    [--colorFormat color_format]             - specifies the color format. option: yuv420p, nv12, rgb32 \n");
    printf("%s", "    [--decodeType decode_type]               - specifies the decode type. option: ffmpeg \n");
    printf("%s", "    [--dumpFile 1/0]                         - dump file(1) or not(0). default: 0: no dump file \n");
    printf("%s", "Examples: ./MRDASampleDecodeAppSW.exe -i input.h265 -o output.raw --frameNum 3000 --codecId h265 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --decodeType ffmpeg \n");
}

AVCodecID StringToCodecID(const char *codec_id)
{
    if (0 == strcmp(codec_id, "h264") || 0 == strcmp(codec_id, "avc"))
    {
        return AV_CODEC_ID_H264;
    }
    else if (0 == strcmp(codec_id, "h265") || 0 == strcmp(codec_id, "hevc"))
    {
        return AV_CODEC_ID_HEVC;
    }
    else if (0 == strcmp(codec_id, "av1"))
    {
        return AV_CODEC_ID_AV1;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unsupported codec id: %s", codec_id);
        return AV_CODEC_ID_NONE;
    }
    return AV_CODEC_ID_NONE;;
}

AVPixelFormat StringToColorFormat(const char *color_format)
{
    if (0 == strcmp(color_format, "rgb32"))
    {
        return AVPixelFormat::AV_PIX_FMT_BGRA; // DX screen capture format order
    }
    else if (0 == strcmp(color_format, "yuv420p"))
    {
        return AVPixelFormat::AV_PIX_FMT_YUV420P;
    }
    else if (0 == strcmp(color_format, "nv12"))
    {
        return AVPixelFormat::AV_PIX_FMT_NV12;
    }
    return AVPixelFormat::AV_PIX_FMT_NONE;
}

TASKTYPE StringToDecodeType(const char *decode_type_str)
{
    TASKTYPE decode_type = TASKTYPE::NONE;
    if (0 == strcmp(decode_type_str, "ffmpeg"))
    {
        decode_type = TASKTYPE::taskFFmpegDecode;
    }
    else if (0 == strcmp(decode_type_str, "oneVPL"))
    {
        decode_type = TASKTYPE::taskOneVPLDecode;
    }
    return decode_type;
}

bool stringToBool(const std::string& str) {
    if (str == "1") return true;
    else if (str == "0") return false;
    else {
        MRDA_LOG(LOG_ERROR, "Invalid boolean string: %s", str.c_str());
        return false;
    }
}

bool ParseConfig(int argc, char **argv, InputConfig *inputConfig) {
    // Check if the argument count is reasonable compared to InputConfig requirements
    if (argc < 2) {
        MRDA_LOG(LOG_ERROR, "No enough arguments!");
        PrintHelp();
        return false; // Not enough arguments
    }
    // Iterate over argv starting at 1 as index 0 is the program name
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            inputConfig->sourceFile = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            inputConfig->sinkFile = argv[++i];
        } else if (strcmp(argv[i], "--frameNum") == 0 && i + 1 < argc) {
            inputConfig->frameNum = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--codecId") == 0 && i + 1 < argc) {
            inputConfig->codec_id = StringToCodecID(argv[++i]);
        } else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            inputConfig->framerate_num = static_cast<uint32_t>(std::atoi(argv[++i]));
            inputConfig->framerate_den = 1;
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            inputConfig->frame_width = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            inputConfig->frame_height = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--colorFormat") == 0 && i + 1 < argc) {
            inputConfig->color_format = StringToColorFormat(argv[++i]);
        } else if (strcmp(argv[i], "--decodeType") == 0 && i + 1 < argc) {
            inputConfig->decode_type = StringToDecodeType(argv[++i]);
        } else if (strcmp(argv[i], "--dumpFile") == 0 && i + 1 < argc) {
            inputConfig->dump_file = stringToBool(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 && i + 1 < argc) {
            PrintHelp();
            return false;
        }
        else {
            MRDA_LOG(LOG_ERROR, "Unknown argument detected: %s", argv[i]);
            PrintHelp();
            return false; // Unknown argument detected
        }
    }

    return true;
}

MRDAStatus InitDecoder(InputConfig *inputConfig, DecodeContext *decoderContext)
{
    if (avformat_open_input(&decoderContext->input_ctx, inputConfig->sourceFile.c_str(), NULL, NULL) != 0) {
        MRDA_LOG(LOG_ERROR, "Cannot open input file '%s'", inputConfig->sourceFile.c_str());
        return MRDA_STATUS_INVALID_DATA;
    }

    if (avformat_find_stream_info(decoderContext->input_ctx, NULL) < 0) {
        MRDA_LOG(LOG_ERROR, "Cannot find stream information");
        return MRDA_STATUS_INVALID_DATA;
    }

    const AVCodec *dec = nullptr;
    int ret = av_find_best_stream(decoderContext->input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0)
    {
        MRDA_LOG(LOG_ERROR, "Cannot find a video stream in the input file");
        return MRDA_STATUS_INVALID_DATA;
    }
    decoderContext->video_stream_index = ret;

    decoderContext->codec_ctx = avcodec_alloc_context3(dec);
    if (decoderContext->codec_ctx == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Cannot allocate the decoder context");
        return MRDA_STATUS_INVALID_DATA;
    }

    decoderContext->codec_ctx->width = inputConfig->frame_width;
    decoderContext->codec_ctx->height = inputConfig->frame_height;
    if (inputConfig->framerate_den == 0) return MRDA_STATUS_INVALID_DATA;
    AVRational framerate;
    framerate.num = inputConfig->framerate_num;
    framerate.den = inputConfig->framerate_den;
    decoderContext->codec_ctx->framerate = framerate;
    decoderContext->codec_ctx->codec_id = inputConfig->codec_id;

    if (avcodec_parameters_to_context(decoderContext->codec_ctx, decoderContext->input_ctx->streams[decoderContext->video_stream_index]->codecpar) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Cannot copy codec parameters to decoder context");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (avcodec_open2(decoderContext->codec_ctx, dec, NULL) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Cannot open codec");
        return MRDA_STATUS_INVALID_DATA;
    }

    decoderContext->output_file = fopen(inputConfig->sinkFile.c_str(), "wb");
    if (decoderContext->output_file == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Cannot open output file");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus ColorSpaceConvert(AVPixelFormat in_pix_fmt, AVPixelFormat out_pix_fmt, AVFrame *in_frame, AVFrame* out_frame)
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

MRDAStatus ReadOneFrame(DecodeContext *decodeContext)
{
    if (av_read_frame(decodeContext->input_ctx, decodeContext->av_packet) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to read frame");
        return MRDA_STATUS_NOT_ENOUGH_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus DecodeOneFrame(DecodeContext *decodeContext, InputConfig *inputConfig)
{
    if (decodeContext == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid decode context");
        return MRDA_STATUS_INVALID_DATA;
    }

    int ret = avcodec_send_packet(decodeContext->codec_ctx, decodeContext->av_packet);
    if (ret < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to send packet to decoder");
        return MRDA_STATUS_INVALID_DATA;
    }
    while (ret >= 0)
    {
        decodeContext->av_frame = av_frame_alloc();
        if (!decodeContext->av_frame)
        {
            MRDA_LOG(LOG_ERROR, "Failed to allocate frame");
            return MRDA_STATUS_INVALID_DATA;
        }
        ret = avcodec_receive_frame(decodeContext->codec_ctx, decodeContext->av_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            av_frame_free(&decodeContext->av_frame);
            return MRDA_STATUS_SUCCESS;
        }
        else if (ret < 0)
        {
            MRDA_LOG(LOG_ERROR, "Failed to receive frame from decoder");
            av_frame_free(&decodeContext->av_frame);
            return MRDA_STATUS_INVALID_DATA;
        }

        AVFrame *out_frame = nullptr;
        int size           = 0;
        uint8_t *buffer    = nullptr;
        // do CSC
        if (inputConfig->color_format != decodeContext->av_frame->format) {
            out_frame = av_frame_alloc();
            if (!out_frame) {
                MRDA_LOG(LOG_ERROR, "Can not alloc frame");
                av_frame_free(&decodeContext->av_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            out_frame->format = inputConfig->color_format;
            out_frame->width = decodeContext->av_frame->width;
            out_frame->height = decodeContext->av_frame->height;
            if (av_frame_get_buffer(out_frame, 0) < 0)
            {
                MRDA_LOG(LOG_ERROR, "Failed to allocate frame data.");
                av_frame_free(&decodeContext->av_frame);
                av_frame_free(&out_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            if (MRDA_STATUS_SUCCESS != ColorSpaceConvert((AVPixelFormat)decodeContext->av_frame->format, inputConfig->color_format, decodeContext->av_frame, out_frame))
            {
                MRDA_LOG(LOG_ERROR, "ColorSpaceConvert failed");
                av_frame_free(&decodeContext->av_frame);
                av_frame_free(&out_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            size = av_image_get_buffer_size((AVPixelFormat)out_frame->format, out_frame->width,
                                            out_frame->height, 1);
            buffer = (uint8_t *)av_malloc(size);
            if (!buffer) {
                MRDA_LOG(LOG_ERROR, "Can not alloc buffer");
                av_frame_free(&decodeContext->av_frame);
                av_frame_free(&out_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            ret = av_image_copy_to_buffer(buffer, size,
                                        (const uint8_t * const *)out_frame->data,
                                        (const int *)out_frame->linesize, (AVPixelFormat)out_frame->format,
                                        out_frame->width, out_frame->height, 1);
            if (ret < 0) {
                MRDA_LOG(LOG_ERROR, "Can not copy image to buffer");
                av_frame_free(&decodeContext->av_frame);
                av_frame_free(&out_frame);
                av_free(buffer);
                return MRDA_STATUS_INVALID_DATA;
            }
            av_frame_free(&out_frame);
        }
        else
        {
            size = av_image_get_buffer_size((AVPixelFormat)decodeContext->av_frame->format, decodeContext->av_frame->width,
                                            decodeContext->av_frame->height, 1);
            buffer = (uint8_t *)av_malloc(size);
            if (!buffer) {
                MRDA_LOG(LOG_ERROR, "Can not alloc buffer");
                av_frame_free(&decodeContext->av_frame);
                return MRDA_STATUS_INVALID_DATA;
            }
            ret = av_image_copy_to_buffer(buffer, size,
                                        (const uint8_t * const *)decodeContext->av_frame->data,
                                        (const int *)decodeContext->av_frame->linesize, (AVPixelFormat)decodeContext->av_frame->format,
                                        decodeContext->av_frame->width, decodeContext->av_frame->height, 1);
            if (ret < 0) {
                MRDA_LOG(LOG_ERROR, "Can not copy image to buffer");
                av_frame_free(&decodeContext->av_frame);
                av_free(buffer);
                return MRDA_STATUS_INVALID_DATA;
            }
        }

        if (inputConfig->dump_file && ((ret = fwrite(buffer, 1, size, decodeContext->output_file)) < 0)) {
            MRDA_LOG(LOG_ERROR, "Failed to dump raw data.");
            av_free(buffer);
            return MRDA_STATUS_OPERATION_FAIL;
        }
        av_free(buffer);
        av_frame_free(&decodeContext->av_frame);
        decodeContext->cur_frame_num++;
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus DestroyDecoder(DecodeContext *decodeContext)
{
    if (decodeContext->input_ctx)
    {
        avformat_close_input(&decodeContext->input_ctx);
    }
    if (decodeContext->codec_ctx)
    {
        avcodec_free_context(&decodeContext->codec_ctx);
    }
    if (decodeContext->av_packet)
    {
        av_packet_free(&decodeContext->av_packet);
    }
    if (decodeContext->av_frame)
    {
        av_frame_free(&decodeContext->av_frame);
    }
    if (decodeContext->output_file)
    {
        fclose(decodeContext->output_file);
    }

    return MRDA_STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
    InputConfig inputConfig = {};
    DecodeContext decodeContext = {};
    // 1. parse config file
    if (!ParseConfig(argc, argv, &inputConfig))
    {
        return MRDA_STATUS_INVALID_DATA;
    }

    // 2. Init decoder
    if (MRDA_STATUS_SUCCESS != InitDecoder(&inputConfig, &decodeContext))
    {
        MRDA_LOG(LOG_INFO, "Failed to init decoder");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // 3. Decode main loop
    decodeContext.av_packet = av_packet_alloc();
    if (!decodeContext.av_packet) {
        MRDA_LOG(LOG_ERROR, "Failed to allocate AVPacket");
        return MRDA_STATUS_INVALID_DATA;
    }
    std::chrono::high_resolution_clock clock{};
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    uint64_t singleExceedTime   = 0;
    decodeContext.cur_frame_num = 0;
    while (decodeContext.cur_frame_num < inputConfig.frameNum)
    {
        uint64_t singleStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
        // 3.1 Read one frame from source file
        if (MRDA_STATUS_SUCCESS != ReadOneFrame(&decodeContext))
        {
            // MRDA_LOG(LOG_ERROR, "Failed to read one frame");
            break;
        }
        // 3.2 Decode one frame
        if (decodeContext.av_packet->stream_index == decodeContext.video_stream_index)
        {
            if (MRDA_STATUS_SUCCESS != DecodeOneFrame(&decodeContext, &inputConfig))
            {
                MRDA_LOG(LOG_ERROR, "Failed to decode one frame");
                break;
            }
        }
        uint64_t singleEndTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
        uint64_t singleCost = singleEndTime - singleStartTime;
        uint64_t singleInterval = static_cast<uint64_t>(1000 / float(inputConfig.framerate_num / inputConfig.framerate_den));
        if (singleCost + singleExceedTime < singleInterval)
        {
            uint64_t sleepTime = singleInterval - singleCost - singleExceedTime;
            Sleep(sleepTime);
            singleExceedTime = 0;
        }
        else
        {
            singleExceedTime = singleCost + singleExceedTime - singleInterval;
        }
    }
    uint64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    float total_fps = static_cast<float>(decodeContext.cur_frame_num) * 1000 / (end - start);
    MRDA_LOG(LOG_INFO, "MRDA trace log: total decode frame num: %d, fps: %f", decodeContext.cur_frame_num, total_fps);

    // 4. Destroy decoder
    if (MRDA_STATUS_SUCCESS != DestroyDecoder(&decodeContext))
    {
        MRDA_LOG(LOG_INFO, "Failed to destroy decoder");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}