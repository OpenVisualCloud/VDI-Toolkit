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
//! \file sample_capture_encode.cpp
//! \brief a simple sample encode application to demonstrate MDSC and MRDA API interface usage.
//! \date 2024-11-27
//!


#include "common.h"
#include "MultiDisplayScreenCapture.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include "libswscale/swscale.h"
}

#include <thread>
#include <chrono>

typedef enum INPUTTYPE
{
    CaptureInput = 0,
    FileInput
}InputType;

typedef enum OUTPUTTYPE
{
    StreamOutput = 0,
    FileOutput
}OutputType;

typedef struct INPUTCONFIG
{
    // host session
    std::string hostSessionAddr;
    // I/O
    std::string sourceFile;
    std::string sinkFile;
    std::string rtsp_url;
    INPUTTYPE input_type;
    OUTPUTTYPE output_type;
    INPUTTYPE type;
    // share memory info
    uint64_t     totalMemorySize;    //!< total memory size
    uint32_t     bufferNum;          //!< buffer number
    uint64_t     bufferSize;         //!< buffer size
    std::string  in_mem_dev_path;    //!< input memory dev path
    std::string  out_mem_dev_path;   //!< output memory dev path
    uint32_t     in_mem_dev_slot_number;    //!< input memory dev slot number
    uint32_t     out_mem_dev_slot_number;   //!< output memory dev slot number
    // encoding params
    uint32_t      frameNum;
    StreamCodecID codec_id;             //!< codec id
    uint32_t      gop_size;             //!< the distance between two adjacent intra frame
    uint32_t      async_depth;          //!< Specifies how many asynchronous operations an application performs
    TargetUsage   target_usage;         //!< the preset for quality and performance balance,
                                        //!< [0-12], 0 is best quality, 12 is best performance
    uint32_t rc_mode;                   //!< rate control mode, 0 is CQP mode and 1 is VBR mode
    uint32_t qp;                        //!< quantization value under CQP mode
    uint32_t bit_rate;                  //!< bitrate value under VBR mode
    int32_t  framerate_num;             //!< frame rate numerator
    int32_t  framerate_den;             //!< frame rate denominator
    uint32_t frame_width;               //!< width of frame
    uint32_t frame_height;              //!< height of frame
    ColorFormat  color_format;          //!< pixel color format
    CodecProfile codec_profile;         //!< the profile to create bitstream
    uint32_t max_b_frames;              //!< maximum number of B-frames between non-B-frames
    TASKTYPE encode_type;               //!< encode type, 0 is ffmpeg encode, 1 is oneVPL encode
} InputConfig;

MRDAStatus ReadRawFrame(FILE *f, FrameBufferItem *data, ColorFormat format)
{
    if (f == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: invalid input file");
        return MRDA_STATUS_INVALID_DATA;
    }

    static int cnt = 0;

    if (data->bufferItem == nullptr || data->bufferItem->buf_ptr == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: invalid input buffer");
        return MRDA_STATUS_INVALID_DATA;
    }

    int frame_size = 0;
    switch (format)
    {
    case ColorFormat::COLOR_FORMAT_NV12:
    case ColorFormat::COLOR_FORMAT_YUV420P:
        frame_size = data->width * data->height * 3 / 2;
        break;
    case ColorFormat::COLOR_FORMAT_RGBA32:
        frame_size = data->width * data->height * 4;
        break;
    case ColorFormat::COLOR_FORMAT_NONE:
    default:
        MRDA_LOG(LOG_ERROR, "ERROR: invalid color format");
        return MRDA_STATUS_INVALID_DATA;
    }

    static long file_size = 0;
    static int total_frames = 0;
    if (file_size == 0 || total_frames == 0)
    {
        fseek(f, 0, SEEK_END);
        file_size = ftell(f);
        total_frames = file_size / frame_size;
    }

    // get current frame offset
    long current_frame_offset = (cnt % total_frames) * frame_size;

    fseek(f, current_frame_offset, SEEK_SET);

    // read frame to data buffer
    fread(data->bufferItem->buf_ptr, 1, frame_size, f);
    data->bufferItem->occupied_size = frame_size;

    data->pts = cnt++;
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus ReadCapturedFrame(DX_RESOURCES *DxRes, ID3D11Texture2D *CapturedTexture, FrameBufferItem *data, int pts)
{
    if (CapturedTexture != nullptr)
    {
        D3D11_TEXTURE2D_DESC desc;
        CapturedTexture->GetDesc(&desc);
        data->width = desc.Width;
        data->height = desc.Height;
        data->bufferItem->occupied_size = data->width * data->height * 4;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = DxRes->Context->Map(CapturedTexture, 0, D3D11_MAP_READ, 0, &mapped);
        if (!FAILED(hr) && mapped.pData)
        {
            memcpy(data->bufferItem->buf_ptr, static_cast<uint8_t*>(mapped.pData), data->bufferItem->occupied_size);
            DxRes->Context->Unmap(CapturedTexture, 0);
            data->pts = pts;
        }
        else
        {
            MRDA_LOG(LOG_ERROR, "failed to map texture\n");
            if (CapturedTexture)
            {
                CapturedTexture->Release();
                CapturedTexture = nullptr;
            }
            return MRDA_STATUS_NOT_READY;
        }
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "texture is nullptr\n");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus WriteEncodedFrame(FILE *f, FrameBufferItem *data)
{
    if (f == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: invalid output file");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (data->bufferItem == nullptr || data->bufferItem->buf_ptr == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: invalid output buffer");
        return MRDA_STATUS_INVALID_DATA;
    }

    fwrite(data->bufferItem->buf_ptr + sizeof(data->bufferItem->state), 1, data->bufferItem->occupied_size, f);

    return MRDA_STATUS_SUCCESS;
}

AVCodecID GetFFmpegCodecId(StreamCodecID codecID)
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

MRDAStatus InitVideoStream(InputConfig *inputConfig, AVCodecContext** pCodecCtx, AVFormatContext** pVideoOfmtCtx, AVStream** pVideoStream, AVPacket** pPkt)
{
    const AVCodec* encodec = avcodec_find_encoder(GetFFmpegCodecId(inputConfig->codec_id));
    if (!encodec)
    {
        MRDA_LOG(LOG_ERROR, "avcodec_find_encoder failed!\n");
        return MRDA_STATUS_INVALID_PARAM;
    }

    *pCodecCtx = avcodec_alloc_context3(encodec);
    if (!*pCodecCtx)
    {
        MRDA_LOG(LOG_ERROR, "avcodec_alloc_context3 failed!!\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    (*pCodecCtx)->bit_rate = inputConfig->bit_rate;
    (*pCodecCtx)->width = inputConfig->frame_width;
    (*pCodecCtx)->height = inputConfig->frame_height;
    (*pCodecCtx)->time_base = { 1, inputConfig->framerate_num };
    (*pCodecCtx)->framerate = { inputConfig->framerate_num, 1 };
    (*pCodecCtx)->gop_size = inputConfig->gop_size;
    (*pCodecCtx)->max_b_frames = inputConfig->max_b_frames;
    (*pCodecCtx)->pix_fmt = AV_PIX_FMT_YUV420P;

    int ret = avcodec_open2(*pCodecCtx, encodec, NULL);
    if (ret < 0)
    {
        char errStr[256] = { 0 };
        av_strerror(ret, errStr, sizeof(errStr));
        MRDA_LOG(LOG_ERROR, "error avcodec_open2 (%s)\n", errStr);
        return MRDA_STATUS_OPERATION_FAIL;
    }

    MRDA_LOG(LOG_INFO, "stream url is %s!\n", inputConfig->rtsp_url.c_str());
    if (avformat_alloc_output_context2(pVideoOfmtCtx, NULL, "rtsp", inputConfig->rtsp_url.c_str()) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to alloc rtsp output format context\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    av_opt_set((*pVideoOfmtCtx)->priv_data, "rtsp_transport", "udp", 0);
    *pVideoStream = avformat_new_stream(*pVideoOfmtCtx, NULL);
    if (NULL == *pVideoStream)
    {
        MRDA_LOG(LOG_ERROR, "Failed to add new stream\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (avcodec_parameters_from_context((*pVideoStream)->codecpar, *pCodecCtx) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to copy codec parameters\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    (*pVideoStream)->time_base = (*pCodecCtx)->time_base;
    (*pVideoStream)->r_frame_rate = (*pCodecCtx)->framerate;
    (*pVideoStream)->codecpar->codec_tag = 0;

    if(!((*pVideoOfmtCtx)->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&((*pVideoOfmtCtx)->pb), inputConfig->rtsp_url.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            MRDA_LOG(LOG_ERROR, "Failed to alloc rtsp avio\n");
            return MRDA_STATUS_OPERATION_FAIL;
        }
    }

    MRDA_LOG(LOG_INFO, "Open video output successed\n");

    av_dump_format(*pVideoOfmtCtx, 0, inputConfig->rtsp_url.c_str(), 1);

    if (avformat_write_header(*pVideoOfmtCtx, NULL) < 0)
    {
        MRDA_LOG(LOG_ERROR, "Failed to write header\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    *pPkt = av_packet_alloc();
    if (!*pPkt){
        MRDA_LOG(LOG_ERROR, "Failed to alloc AVPacket\n");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus UnInitVideoStream(AVCodecContext** pCodecCtx, AVFormatContext** pVideoOfmtCtx, AVStream** pVideoStream, AVPacket** pPkt)
{
    av_packet_free(pPkt);
    if (*pVideoOfmtCtx)
    {
        if ((*pVideoOfmtCtx)->pb) {
            avio_close((*pVideoOfmtCtx)->pb);
        }
    }
    avformat_free_context(*pVideoOfmtCtx);
    avcodec_free_context(pCodecCtx);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus streamToRTSP(AVFormatContext** pVideoOfmtCtx, AVPacket** pPkt, const FrameBufferItem *frameBufferItem, int64_t pts)
{
    int ret = 0;
    if (!*pPkt){
        MRDA_LOG(LOG_ERROR, "AVPacket is nullptr\n");
        return MRDA_STATUS_INVALID_HANDLE;
    }
    (*pPkt)->data = frameBufferItem->bufferItem->buf_ptr + sizeof(frameBufferItem->bufferItem->state);
    (*pPkt)->size = frameBufferItem->bufferItem->occupied_size;
    (*pPkt)->pts = pts;
    (*pPkt)->dts = pts;
    ret = av_interleaved_write_frame(*pVideoOfmtCtx, *pPkt);
    if (ret < 0) {
        MRDA_LOG(LOG_ERROR, "av_interleaved_write_frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    av_packet_unref(*pPkt);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus EncodeFrameAsync(MRDAHandle mrda_handle, std::shared_ptr<FrameBufferItem> inBuffer)
{
#ifdef _ENABLE_TRACE_
    if (inBuffer != nullptr) MRDA_LOG(LOG_INFO, "Encoding trace log: begin send frame in sample encode, pts: %llu", inBuffer->pts);
#endif
    // 1. send input frame
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SendFrame(mrda_handle, inBuffer))
    {
        MRDA_LOG(LOG_ERROR, "send input frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // if (inBuffer != nullptr) MRDA_LOG(LOG_INFO, "Send buffer at pts: %llu, buffer id %d", inBuffer->pts, inBuffer->bufferItem->buf_id);

    return MRDA_STATUS_SUCCESS;
}

void ReceiveFrameThread(MRDAHandle mrda_handle, InputConfig *inputConfig)
{
    FILE *fpSink = nullptr;
    fpSink = fopen(inputConfig->sinkFile.c_str(), "wb");
    if (fpSink == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "open file failed!");
        return;
    }

    uint64_t cur_pts = 0;
    while (cur_pts < inputConfig->frameNum - 1)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(mrda_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "Output data not enough!");
            continue;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "get buffer for output failed!");
            return;
        }
        // success
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "Encoding trace log: complete receive frame in sample encode, pts: %llu", outBuffer->pts);
#endif
        WriteEncodedFrame(fpSink, outBuffer.get());
        cur_pts = outBuffer->pts;
        // MRDA_LOG(LOG_INFO, "Receive frame at pts: %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(mrda_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "release output buffer failed!");
            return;
        }
        // MRDA_LOG(LOG_INFO, "Release output buffer pts %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);
        outBuffer->uninit();
    }

    fclose(fpSink);
    return;
}

void ReceiveFrameRTSPThread(MRDAHandle mrda_handle, InputConfig *inputConfig)
{
    // 1. Init ffmpeg rtsp ouput context
    AVCodecContext* pCodecCtx = nullptr;
    AVFormatContext* pVideoOfmtCtx = nullptr;
    AVStream* pVideoStream = nullptr;
    AVPacket* pPkt = nullptr;

    if (MRDA_STATUS_SUCCESS != InitVideoStream(inputConfig, &pCodecCtx, &pVideoOfmtCtx, &pVideoStream, &pPkt)){
        MRDA_LOG(LOG_ERROR, "Failed to init video stream\n");
        return;
    }

    // 2. get output buffer and stream to rtsp server
    uint64_t cur_pts = 0;
    while (cur_pts < inputConfig->frameNum - 1)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(mrda_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "Output data not enough!");
            continue;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "get buffer for output failed!");
            return;
        }
        // success
#ifdef _ENABLE_TRACE_
        MRDA_LOG(LOG_INFO, "Encoding trace log: complete receive frame in sample encode, pts: %llu", outBuffer->pts);
#endif
        if (MRDA_STATUS_SUCCESS != streamToRTSP(&pVideoOfmtCtx, &pPkt, outBuffer.get(), outBuffer->pts))
        {
            MRDA_LOG(LOG_ERROR, "stream to rtsp failed!");
            return;
        }
        cur_pts = outBuffer->pts;
        // MRDA_LOG(LOG_INFO, "Receive frame at pts: %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(mrda_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "release output buffer failed!");
            return;
        }
        // MRDA_LOG(LOG_INFO, "Release output buffer pts %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);
        outBuffer->uninit();
    }

    // 3. Finish stream and Uninit ffmpeg rtsp ouput context
    av_write_trailer(pVideoOfmtCtx);

    if (MRDA_STATUS_SUCCESS != UnInitVideoStream(&pCodecCtx, &pVideoOfmtCtx, &pVideoStream, &pPkt)){
        MRDA_LOG(LOG_ERROR, "Failed to uninit video stream\n");
        return;
    }

    return;
}

StreamCodecID StringToCodecID(const char *codec_id)
{
    StreamCodecID streamCodecId = StreamCodecID::CodecID_NONE;
    if (0 == strcmp(codec_id, "h264") || 0 == strcmp(codec_id, "avc"))
    {
        streamCodecId = StreamCodecID::CodecID_AVC;
    }
    else if (0 == strcmp(codec_id, "h265") || 0 == strcmp(codec_id, "hevc"))
    {
        streamCodecId = StreamCodecID::CodecID_HEVC;
    }
    else if (0 == strcmp(codec_id, "av1"))
    {
        streamCodecId = StreamCodecID::CodecID_AV1;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unsupported codec id: %s", codec_id);
    }
    return streamCodecId;
}

TargetUsage StringToTargetUsage(const char *target_usage)
{
    TargetUsage targetUsage = TargetUsage::Balanced; // default
    if (0 == strcmp(target_usage, "balanced"))
    {
        targetUsage = TargetUsage::Balanced;
    }
    else if (0 == strcmp(target_usage, "quality"))
    {
        targetUsage = TargetUsage::BestQuality;
    }
    else if (0 == strcmp(target_usage, "speed"))
    {
        targetUsage = TargetUsage::BestSpeed;
    }
    return targetUsage;
}

ColorFormat StringToColorFormat(const char *color_format)
{
    ColorFormat colorFormat = ColorFormat::COLOR_FORMAT_NONE;
    if (0 == strcmp(color_format, "rgb32"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_RGBA32;
    }
    else if (0 == strcmp(color_format, "yuv420p"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_YUV420P;
    }
    else if (0 == strcmp(color_format, "nv12"))
    {
        colorFormat = ColorFormat::COLOR_FORMAT_NV12;
    }
    return colorFormat;
}

CodecProfile StringToCodecProfile(const char *codec_profile)
{
    CodecProfile codecProfile = CodecProfile::PROFILE_NONE;
    if (0 == strcmp(codec_profile, "avc:main"))
    {
        codecProfile = CodecProfile::PROFILE_AVC_MAIN;
    }
    else if (0 == strcmp(codec_profile, "avc:high"))
    {
        codecProfile = CodecProfile::PROFILE_AVC_HIGH;
    }
    else if (0 == strcmp(codec_profile, "hevc:main"))
    {
        codecProfile = CodecProfile::PROFILE_HEVC_MAIN;
    }
    else if (0 == strcmp(codec_profile, "hevc:main10"))
    {
        codecProfile = CodecProfile::PROFILE_HEVC_MAIN10;
    }
    else if (0 == strcmp(codec_profile, "av1:main"))
    {
        codecProfile = CodecProfile::PROFILE_AV1_MAIN;
    }
    else if (0 == strcmp(codec_profile, "av1:high"))
    {
        codecProfile = CodecProfile::PROFILE_AV1_HIGH;
    }
    return codecProfile;
}

void PrintHelp()
{
    printf("%s", "Usage: .\\MRDASampleApp.exe [<options>]\n");
    printf("%s", "Options: \n");
    printf("%s", "    [--help]                                 - print help README document. \n");
    printf("%s", "    [--hostSessionAddr host_session_address] - specifies host session address. \n");
    printf("%s", "    [--inputType input_type]                 - specifies the input type. option: capture, file. \n");
    printf("%s", "    [--outputType output_type]               - specifies the ouput type. option: stream, file. \n");
    printf("%s", "    [-i input_file]                          - specifies input file. \n");
    printf("%s", "    [-o output_file]                         - specifies output file. \n");
    printf("%s", "    [-rtsp rtsp_url]                         - specifies output rtsp url. \n");
    printf("%s", "    [--memDevSize memory_device_size]        - specifies memory device size. \n");
    printf("%s", "    [--bufferNum buffer_number]              - specifies number of buffers. \n");
    printf("%s", "    [--bufferSize buffer_size]               - specifies buffer size. \n");
    printf("%s", "    [--inDevPath input_device_path]          - specifies input device path. \n");
    printf("%s", "    [--outDevPath output_device_path]        - specifies output device path. \n");
    printf("%s", "    [--inDevSlotNumber in_dev_slot_number]   - specifies input device slot number. \n");
    printf("%s", "    [--outDevSlotNumber out_dev_slot_number] - specifies output device slot number. \n");
    printf("%s", "    [--frameNum number_of_frames]            - specifies number of frames to process. \n");
    printf("%s", "    [--codecId codec_identifier]             - specifies the codec identifier. option: h265/hevc | h264/avc \n");
    printf("%s", "    [--gopSize group_of_pictures_size]       - specifies the size of group of pictures. \n");
    printf("%s", "    [--asyncDepth asynchronous_depth]        - specifies the asynchronous depth. \n");
    printf("%s", "    [--targetUsage target_usage]             - specifies the target usage. option: balanced | quality | speed \n");
    printf("%s", "    [--rcMode rate_control_mode]             - specifies the rate control mode. option: 0(CQP), 1(VBR) \n");
    printf("%s", "    [--bitrate bitrate value if rcMode is 1] - specifies the bitrate. \n");
    printf("%s", "    [--qp qp value if rcMode is 0]           - specifies the qp value. \n");
    printf("%s", "    [--fps frames_per_second]                - specifies the frames per second. \n");
    printf("%s", "    [--width frame_width]                    - specifies the frame width. \n");
    printf("%s", "    [--height frame_height]                  - specifies the frame height. \n");
    printf("%s", "    [--colorFormat color_format]             - specifies the color format. option: yuv420p, nv12, rgb32 \n");
    printf("%s", "    [--codecProfile codec_profile]           - specifies the codec profile. option: avc:main, avc:high, hevc:main \n");
    printf("%s", "    [--maxBFrames max_b_frames]              - specifies the maximum number of B frames. \n");
    printf("%s", "    [--encodeType encode_type]               - specifies the encode type. option: ffmpeg, oneVPL \n");
    printf("%s", "Examples: ./MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 -i --inputType file --outputType file input.rgba -o output.hevc --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1 --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL \n \
         ./MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 --inputType capture --outputType stream -rtsp rtsp://127.0.0.1:8554/capture --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1 --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL \n");
}

TASKTYPE StringToEncodeType(const char *encode_type_str)
{
    TASKTYPE encode_type = TASKTYPE::NONE;
    if (0 == strcmp(encode_type_str, "ffmpeg"))
    {
        encode_type = TASKTYPE::taskFFmpegEncode;
    }
    else if (0 == strcmp(encode_type_str, "oneVPL"))
    {
        encode_type = TASKTYPE::taskOneVPLEncode;
    }
    return encode_type;
}

InputType StringToInputType(const char *input_type_str)
{
    InputType input_type = InputType::CaptureInput;
    if (0 == strcmp(input_type_str, "capture"))
    {
        input_type = InputType::CaptureInput;
    }
    else if (0 == strcmp(input_type_str, "file"))
    {
        input_type = InputType::FileInput;
    }
    return input_type;
}

OutputType StringToOutputType(const char *output_type_str)
{
    OutputType output_type = OutputType::StreamOutput;
    if (0 == strcmp(output_type_str, "stream"))
    {
        output_type = OutputType::StreamOutput;
    }
    else if (0 == strcmp(output_type_str, "file"))
    {
        output_type = OutputType::FileOutput;
    }
    return output_type;
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
        if (strcmp(argv[i], "--hostSessionAddr") == 0 && i + 1 < argc) {
            inputConfig->hostSessionAddr = argv[++i];
        } else if (strcmp(argv[i], "--inputType") == 0 && i + 1 < argc) {
            inputConfig->input_type = StringToInputType(argv[++i]);
        } else if (strcmp(argv[i], "--outputType") == 0 && i + 1 < argc) {
            inputConfig->output_type = StringToOutputType(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            inputConfig->sourceFile = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            inputConfig->sinkFile = argv[++i];
        } else if (strcmp(argv[i], "-rtsp") == 0 && i + 1 < argc) {
            inputConfig->rtsp_url = argv[++i];
        } else if (strcmp(argv[i], "--memDevSize") == 0 && i + 1 < argc) {
            inputConfig->totalMemorySize = std::atoll(argv[++i]);
        } else if (strcmp(argv[i], "--bufferNum") == 0 && i + 1 < argc) {
            inputConfig->bufferNum = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--bufferSize") == 0 && i + 1 < argc) {
            inputConfig->bufferSize = std::atoll(argv[++i]);
        } else if (strcmp(argv[i], "--inDevPath") == 0 && i + 1 < argc) {
            inputConfig->in_mem_dev_path = argv[++i];
        } else if (strcmp(argv[i], "--outDevPath") == 0 && i + 1 < argc) {
            inputConfig->out_mem_dev_path = argv[++i];
        } else if (strcmp(argv[i], "--inDevSlotNumber") == 0 && i + 1 < argc) {
            inputConfig->in_mem_dev_slot_number = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--outDevSlotNumber") == 0 && i + 1 < argc) {
            inputConfig->out_mem_dev_slot_number = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--frameNum") == 0 && i + 1 < argc) {
            inputConfig->frameNum = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--codecId") == 0 && i + 1 < argc) {
            inputConfig->codec_id = StringToCodecID(argv[++i]);
        } else if (strcmp(argv[i], "--gopSize") == 0 && i + 1 < argc) {
            inputConfig->gop_size = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--asyncDepth") == 0 && i + 1 < argc) {
            inputConfig->async_depth = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--targetUsage") == 0 && i + 1 < argc) {
            inputConfig->target_usage = StringToTargetUsage(argv[++i]);
        } else if (strcmp(argv[i], "--rcMode") == 0 && i + 1 < argc) {
            inputConfig->rc_mode = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (inputConfig->rc_mode == 0 && strcmp(argv[i], "--qp") == 0 && i + 1 < argc) {
            inputConfig->qp = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (inputConfig->rc_mode == 1 && strcmp(argv[i], "--bitrate") == 0 && i + 1 < argc) {
            inputConfig->bit_rate = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            inputConfig->framerate_num = static_cast<uint32_t>(std::atoi(argv[++i]));
            inputConfig->framerate_den = 1;
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            inputConfig->frame_width = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            inputConfig->frame_height = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--colorFormat") == 0 && i + 1 < argc) {
            inputConfig->color_format = StringToColorFormat(argv[++i]);
        } else if (strcmp(argv[i], "--codecProfile") == 0 && i + 1 < argc) {
            inputConfig->codec_profile = StringToCodecProfile(argv[++i]);
        } else if (strcmp(argv[i], "--maxBFrames") == 0 && i + 1 < argc) {
            inputConfig->max_b_frames = static_cast<uint32_t>(std::atoi(argv[++i]));
        } else if (strcmp(argv[i], "--encodeType") == 0 && i + 1 < argc) {
            inputConfig->encode_type = StringToEncodeType(argv[++i]);
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

MRDAStatus CreateMediaParams(InputConfig *inputConfig, MediaParams *mediaParams)
{
    if (inputConfig == nullptr || mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid input parameters");
        return MRDA_STATUS_INVALID_DATA;
    }
    mediaParams->shareMemoryInfo.totalMemorySize = inputConfig->totalMemorySize;
    mediaParams->shareMemoryInfo.bufferNum = inputConfig->bufferNum;
    mediaParams->shareMemoryInfo.bufferSize = inputConfig->bufferSize;
    mediaParams->shareMemoryInfo.in_mem_dev_path = inputConfig->in_mem_dev_path;
    mediaParams->shareMemoryInfo.out_mem_dev_path = inputConfig->out_mem_dev_path;
    mediaParams->shareMemoryInfo.in_mem_dev_slot_number = inputConfig->in_mem_dev_slot_number;
    mediaParams->shareMemoryInfo.out_mem_dev_slot_number = inputConfig->out_mem_dev_slot_number;

    mediaParams->encodeParams.codec_id = inputConfig->codec_id;
    mediaParams->encodeParams.gop_size = inputConfig->gop_size;
    mediaParams->encodeParams.async_depth = inputConfig->async_depth;
    mediaParams->encodeParams.target_usage = inputConfig->target_usage;
    mediaParams->encodeParams.rc_mode = inputConfig->rc_mode;
    mediaParams->encodeParams.qp = inputConfig->qp;
    mediaParams->encodeParams.bit_rate = inputConfig->bit_rate;
    mediaParams->encodeParams.framerate_num = inputConfig->framerate_num;
    mediaParams->encodeParams.framerate_den = inputConfig->framerate_den;
    mediaParams->encodeParams.frame_width = inputConfig->frame_width;
    mediaParams->encodeParams.frame_height = inputConfig->frame_height;
    mediaParams->encodeParams.color_format = inputConfig->color_format;
    mediaParams->encodeParams.codec_profile = inputConfig->codec_profile;
    mediaParams->encodeParams.max_b_frames = inputConfig->max_b_frames;
    mediaParams->encodeParams.frame_num = inputConfig->frameNum;
    return MRDA_STATUS_SUCCESS;
}

int main(int argc, char **argv)
{
    InputConfig inputConfig = {};
    // 1. parse config file
    if (!ParseConfig(argc, argv, &inputConfig))
    {
        return MRDA_STATUS_INVALID_DATA;
    }

    TaskInfo taskInfo = {};
    taskInfo.taskType = inputConfig.encode_type;
    taskInfo.taskStatus = TASKStatus::TASK_STATUS_UNKNOWN;
    taskInfo.taskID = 0;
    taskInfo.taskDevice.deviceType = DeviceType::NONE;
    taskInfo.taskDevice.deviceID = 0;
    taskInfo.ipAddr = "";

    ExternalConfig config = {};
    config.hostSessionAddr = inputConfig.hostSessionAddr;
    // 2. init mrda handle
    MRDAHandle mrda_handle = MediaResourceDirectAccess_Init(&taskInfo, &config);
    if (mrda_handle == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "mrda handle is invalid!");
        return MRDA_STATUS_INVALID_HANDLE;
    }

    std::shared_ptr<MediaParams> mediaParams = std::make_shared<MediaParams>();
    if (mediaParams == nullptr) return MRDA_STATUS_INVALID_DATA;

    if (MRDA_STATUS_SUCCESS != CreateMediaParams(&inputConfig, mediaParams.get()))
    {
        MRDA_LOG(LOG_ERROR, "create media params failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // 3. set init params
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SetInitParams(mrda_handle, mediaParams.get()))
    {
        MRDA_LOG(LOG_ERROR, "set init params failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // 4. Prepare Input
    MultiDisplayScreenCapture MDSCsample;
    BufferQueue* ptrBufferQueue = nullptr;
    DX_RESOURCES *DxRes = nullptr;
    FILE *fpSource = nullptr;

    if (CaptureInput == inputConfig.input_type)
    {
        MDSCsample.Init();
        UINT DispNum = MDSCsample.GetDisplayCount();
         // Currently only support 1 screen
        MDSCsample.SetSingleDisplay(0);
        UINT OutNum = MDSCsample.GetOutputCount();
        MDSCsample.SetCaptureFps(inputConfig.framerate_num);
        MDSCsample.StartCaptureScreen();

        MRDA_LOG(LOG_INFO, "Screen Capture Start, Display Num is %d, Output Num is %d, fps is %d\n",
                DispNum, OutNum, inputConfig.framerate_num);
        ptrBufferQueue = &MDSCsample.GetBufferQueues()[0];
        DxRes = MDSCsample.GetDXResource(0);

        //wait for frames ready
        while (1)
        {
            if (ptrBufferQueue->GetSize() > 0)
            {
                CapturedData CurrentData = ptrBufferQueue->DequeueBuffer();
                if (CurrentData.CapturedTexture)
                {
                    D3D11_TEXTURE2D_DESC desc;
                    CurrentData.CapturedTexture->GetDesc(&desc);
                    CurrentData.CapturedTexture->Release();
                    CurrentData.CapturedTexture = nullptr;
                    MRDA_LOG(LOG_INFO, "Captured frame is ready, width %d, height %d.", desc.Width, desc.Height);
                    break;
                }
                else
                {
                    MRDA_LOG(LOG_ERROR, "Invalid captured frame!");
				    return MRDA_STATUS_INVALID_DATA;
                }
            }
            else
            {
                MRDA_LOG(LOG_WARNING, "Waiting for captured buffer queue ready!");
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    else if (FileInput == inputConfig.input_type)
    {
        fpSource = fopen(inputConfig.sourceFile.c_str(), "rb");
	    if (fpSource == nullptr)
	    {
		    MRDA_LOG(LOG_ERROR, "open input file failed!");
		    return MRDA_STATUS_OPERATION_FAIL;
	    }
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unsupported input type!");
		return MRDA_STATUS_INVALID_PARAM;
    }

    // 5. Start receiveThread
    std::thread receiveThread;
    if (StreamOutput == inputConfig.output_type)
    {
        receiveThread = std::thread(ReceiveFrameRTSPThread, mrda_handle, &inputConfig);
    }
    else if (FileOutput == inputConfig.output_type)
    {
        receiveThread = std::thread(ReceiveFrameThread, mrda_handle, &inputConfig);
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unsupported output type!");
        return MRDA_STATUS_INVALID_PARAM;
    }

    // 6. receive main loop
    uint32_t curFrameNum = 0;
    uint64_t cur_pts = 0;
    std::chrono::high_resolution_clock clock{};
	uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    uint64_t singleExceedTime = 0;

    CapturedData CurrentData = CapturedData{};
    CapturedData LastData = CapturedData{};

    while (curFrameNum < inputConfig.frameNum)
    {
        uint64_t singleStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
        // 6.1 get input buffer
        std::shared_ptr<FrameBufferItem> inBuffer = nullptr;
        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_GetBufferForInput(mrda_handle, inBuffer))
        {
            MRDA_LOG(LOG_WARNING, "get buffer for input failed!");
            Sleep(10); // 10ms
            continue;
        }
        if (inBuffer == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "Input buffer is empty!");
            continue;
        }

        // 6.2 fill input buffer
        if (CaptureInput == inputConfig.input_type)
        {
            // get captured frame and encode last frame if no new captured frame
            int BufferQueueSize = ptrBufferQueue->GetSize();
            if (BufferQueueSize > 0)
            {
                CurrentData = ptrBufferQueue->DequeueBuffer();
                if (LastData.CapturedTexture)
                {
                    LastData.CapturedTexture->Release();
                    LastData.CapturedTexture = nullptr;
                }
                LastData = CurrentData;
            }
            else
            {
                //Fake frame, encode last frame
                if (LastData.CapturedTexture)
                {
                    LastData.AcquiredTime++;
                    CurrentData = LastData;
                }
                else
                {
                    continue;
                }
            }
		    MRDAStatus sts = ReadCapturedFrame(DxRes, CurrentData.CapturedTexture, inBuffer.get(), CurrentData.AcquiredTime);
            if (MRDA_STATUS_SUCCESS != sts)
            {
                if(MRDA_STATUS_NOT_READY == sts)
                    continue;
                else
                    break;
		    }
        }
		else if (FileInput == inputConfig.input_type)
		{
            // read input frame
			if (MRDA_STATUS_SUCCESS != ReadRawFrame(fpSource, inBuffer.get(), inputConfig.color_format))
            {
                MRDA_LOG(LOG_WARNING, "read raw frame invalid!");
                break;
            }
		}
		else
		{
			MRDA_LOG(LOG_ERROR, "Unsupported input type!");
			return MRDA_STATUS_INVALID_PARAM;
		}

        // 6.3 encode one frame
        if (MRDA_STATUS_SUCCESS != EncodeFrameAsync(mrda_handle, inBuffer))
        {

            MRDA_LOG(LOG_ERROR, "encode frame failed!");
            break;
        }

        inBuffer->uninit();
        curFrameNum++;
        uint64_t singleEndTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
        uint64_t singleCost = singleEndTime - singleStartTime;
        uint64_t singleIteral = static_cast<uint64_t>(1000 / float(inputConfig.framerate_num / inputConfig.framerate_den));
        if (singleCost + singleExceedTime < singleIteral)
        {
            uint64_t sleepTime = singleIteral - singleCost - singleExceedTime;
            Sleep(sleepTime);
            singleExceedTime = 0;
        }
        else
        {
            singleExceedTime = singleCost + singleExceedTime - singleIteral;
        }
    }

    // 7. flush encoder
    if (MRDA_STATUS_SUCCESS != EncodeFrameAsync(mrda_handle, nullptr))
    {
        MRDA_LOG(LOG_ERROR, "encode frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    receiveThread.join(); //output resource destroyed in the end of receiveThread

    uint64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    float total_fps = static_cast<float>(curFrameNum) * 1000 / (end - start);
    MRDA_LOG(LOG_INFO, "Encoding trace log: total encode frame num: %d, fps: %f", curFrameNum, total_fps);

    // 8. stop mrda and destroy mrda handle
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Stop(mrda_handle))
    {
        MRDA_LOG(LOG_ERROR, "stop mrda failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Destroy(mrda_handle))
    {
        MRDA_LOG(LOG_ERROR, "stop mrda failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    // 9. stop and destroy input resource
    if (CaptureInput == inputConfig.input_type)
    {
        MDSCsample.TerminateCaptureScreen();
        MDSCsample.DeInit();
    }
	else if (FileInput == inputConfig.input_type)
	{
		fclose(fpSource);
	}
    return MRDA_STATUS_SUCCESS;
}