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
//! \file sample_decode.cpp
//! \brief a simple sample decode application to demonstrate MRDA API interface usage.
//! \date 2024-11-12
//!


#include "common.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

// #pragma comment(lib, "libWinGuest.lib")

#define MULTITHREADDECODING 1

bool g_dump_file = false;

#include <thread>
#include <chrono>

MRDAStatus OpenFileToInputCtx(const char *file_name, AVFormatContext **input_ctx)
{
    if (file_name == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: invalid input file name");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (avformat_open_input(input_ctx, file_name, nullptr, nullptr) != 0)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: failed to open input file");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus ReadFrame(AVFormatContext *input_ctx, FrameBufferItem *data)
{
    if (input_ctx == nullptr)
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

    AVPacket *av_packet = av_packet_alloc();

    int ret = av_read_frame(input_ctx, av_packet);
    if (ret < 0)
    {
        MRDA_LOG(LOG_ERROR, "ERROR: failed to read frame");
        av_packet_free(&av_packet);
        return MRDA_STATUS_INVALID_DATA;
    }

    memcpy(data->bufferItem->buf_ptr + sizeof(data->bufferItem->state), av_packet->data, av_packet->size);
    data->bufferItem->occupied_size = av_packet->size;
    data->pts = cnt++;

    av_packet_free(&av_packet);

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus WriteDecodedFrame(FILE *f, FrameBufferItem *data)
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

MRDAStatus DecodeFrame(MRDAHandle mrda_handle, FILE *sink, std::shared_ptr<FrameBufferItem> inBuffer, uint64_t *cur_pts)
{
    // 1. send input frame
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_SendFrame(mrda_handle, inBuffer))
    {
        MRDA_LOG(LOG_ERROR, "send input frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // if (inBuffer != nullptr) MRDA_LOG(LOG_INFO, "Send frame at pts: %llu", inBuffer->pts);
    // 2. get output buffer
    while (true)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(mrda_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "Output data not enough!");
            break;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "get buffer for output failed!");
            return MRDA_STATUS_OPERATION_FAIL;
        }
        // success
        if (outBuffer->pts >= 0)
        {
            if (g_dump_file) WriteDecodedFrame(sink, outBuffer.get());
            *cur_pts = outBuffer->pts;
            // MRDA_LOG(LOG_INFO, "Receive frame at pts: %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);
        }

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(mrda_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "release output buffer failed!");
            return MRDA_STATUS_OPERATION_FAIL;
        }
        outBuffer->uninit();
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus DecodeFrameAsync(MRDAHandle mrda_handle, std::shared_ptr<FrameBufferItem> inBuffer)
{
#ifdef _ENABLE_TRACE_
    if (inBuffer != nullptr) MRDA_LOG(LOG_INFO, "MRDA trace log: begin send frame in sample decode, pts: %llu", inBuffer->pts);
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

void ReceiveFrameThread(MRDAHandle mrda_handle, FILE *sink, uint64_t frameNum)
{
    // 2. get output buffer
    uint64_t cur_pts = 0;
    while (cur_pts < frameNum - 1)
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
        if (outBuffer->pts >= 0)
        {
#ifdef _ENABLE_TRACE_
            MRDA_LOG(LOG_INFO, "MRDA trace log: complete receive frame in sample decode, pts: %llu", outBuffer->pts);
#endif
            if (g_dump_file) WriteDecodedFrame(sink, outBuffer.get());
            cur_pts = outBuffer->pts;
            // MRDA_LOG(LOG_INFO, "Receive frame at pts: %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);
        }

        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(mrda_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "release output buffer failed!");
            return;
        }
        // MRDA_LOG(LOG_INFO, "Release output buffer pts %llu, buffer id %d", outBuffer->pts, outBuffer->bufferItem->buf_id);
        outBuffer->uninit();
    }
    return;
}

MRDAStatus FlushFrame(MRDAHandle mrda_handle, FILE *sink, uint64_t cur_pts, uint64_t totalFrameNum)
{
    while (cur_pts < totalFrameNum - 1)
    {
        std::shared_ptr<FrameBufferItem> outBuffer = nullptr;
        MRDAStatus st = MediaResourceDirectAccess_ReceiveFrame(mrda_handle, outBuffer);
        if (st == MRDA_STATUS_NOT_ENOUGH_DATA)
        {
            // MRDA_LOG(LOG_INFO, "In flush process: Output data not enough!");
            continue;
        }
        else if (st != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "get buffer for output failed!");
            return MRDA_STATUS_OPERATION_FAIL;
        }
        // success
        if (outBuffer->pts >= 0)
        {
            if (g_dump_file) WriteDecodedFrame(sink, outBuffer.get());
            // MRDA_LOG(LOG_INFO, "In flush process: Receive frame at pts: %llu", outBuffer->pts);
        }
        cur_pts = outBuffer->pts;
        if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_ReleaseOutputBuffer(mrda_handle, outBuffer))
        {
            MRDA_LOG(LOG_ERROR, "release output buffer failed!");
            return MRDA_STATUS_OPERATION_FAIL;
        }
        outBuffer->uninit();
    }
    return MRDA_STATUS_SUCCESS;
}

typedef struct INPUTCONFIG
{
    // host session
    std::string hostSessionAddr;
    // I/O
    std::string sourceFile;
    std::string sinkFile;
    // share memory info
    uint64_t     totalMemorySize;    //!< total memory size
    uint32_t     bufferNum;          //!< buffer number
    uint64_t     bufferSize;         //!< buffer size
    std::string  in_mem_dev_path;    //!< input memory dev path
    std::string  out_mem_dev_path;   //!< output memory dev path
    uint32_t     in_mem_dev_slot_number;    //!< input memory dev slot number
    uint32_t     out_mem_dev_slot_number;   //!< output memory dev slot number
    // decoding params
    uint32_t      frameNum;
    StreamCodecID codec_id;             //!< codec id
    uint32_t bit_rate;                  //!< bitrate value under VBR mode
    int32_t  framerate_num;             //!< frame rate numerator
    int32_t  framerate_den;             //!< frame rate denominator
    uint32_t frame_width;               //!< width of frame
    uint32_t frame_height;              //!< height of frame
    ColorFormat  color_format;          //!< pixel color format
    TASKTYPE decode_type;               //!< decode type, 0 is ffmpeg decode, 1 is oneVPL decode
} InputConfig;

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
    printf("%s", "Usage: .\\MRDASampleDecodeApp.exe [<options>]\n");
    printf("%s", "Options: \n");
    printf("%s", "    [--help]                                 - print help README document. \n");
    printf("%s", "    [--hostSessionAddr host_session_address] - specifies host session address. \n");
    printf("%s", "    [-i input_file]                          - specifies input file. \n");
    printf("%s", "    [-o output_file]                         - specifies output file. \n");
    printf("%s", "    [--memDevSize memory_device_size]        - specifies memory device size. \n");
    printf("%s", "    [--bufferNum buffer_number]              - specifies number of buffers. \n");
    printf("%s", "    [--bufferSize buffer_size]               - specifies buffer size. \n");
    printf("%s", "    [--inDevPath input_device_path]          - specifies input device path. \n");
    printf("%s", "    [--outDevPath output_device_path]        - specifies output device path. \n");
    printf("%s", "    [--inDevSlotNumber in_dev_slot_number]   - specifies input device slot number. \n");
    printf("%s", "    [--outDevSlotNumber out_dev_slot_number] - specifies output device slot number. \n");
    printf("%s", "    [--frameNum number_of_frames]            - specifies number of frames to process. \n");
    printf("%s", "    [--codecId codec_identifier]             - specifies the codec identifier. option: h265/hevc | h264/avc \n");
    printf("%s", "    [--fps frames_per_second]                - specifies the frames per second. \n");
    printf("%s", "    [--width frame_width]                    - specifies the frame width. \n");
    printf("%s", "    [--height frame_height]                  - specifies the frame height. \n");
    printf("%s", "    [--colorFormat color_format]             - specifies the color format. option: yuv420p, nv12, rgb32 \n");
    printf("%s", "    [--decodeType decode_type]               - specifies the decode type. option: ffmpeg, oneVPL \n");
    printf("%s", "    [--dumpFile 1/0]                         - dump file(1) or not(0). default: 0: no dump file \n");
    printf("%s", "Examples: ./MRDASampleDecodeApp.exe --hostSessionAddr 127.0.0.1:50051 -i input.h265 -o output.raw --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --decodeType ffmpeg \n");
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
        if (strcmp(argv[i], "--hostSessionAddr") == 0 && i + 1 < argc) {
            inputConfig->hostSessionAddr = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            inputConfig->sourceFile = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            inputConfig->sinkFile = argv[++i];
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
            g_dump_file = stringToBool(argv[++i]);
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

    mediaParams->decodeParams.codec_id = inputConfig->codec_id;
    mediaParams->decodeParams.framerate_num = inputConfig->framerate_num;
    mediaParams->decodeParams.framerate_den = inputConfig->framerate_den;
    mediaParams->decodeParams.frame_width = inputConfig->frame_width;
    mediaParams->decodeParams.frame_height = inputConfig->frame_height;
    mediaParams->decodeParams.color_format = inputConfig->color_format;
    mediaParams->decodeParams.frame_num = inputConfig->frameNum;
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
    taskInfo.taskType = inputConfig.decode_type;
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
    // 4. open source and sink file
    // FILE *source = nullptr;
    // source = fopen(inputConfig.sourceFile.c_str(), "rb");
    FILE *sink = nullptr;
    sink = fopen(inputConfig.sinkFile.c_str(), "wb");
    if (sink == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "open file failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    AVFormatContext *input_ctx = nullptr;
    if (MRDA_STATUS_SUCCESS != OpenFileToInputCtx(inputConfig.sourceFile.c_str(), &input_ctx))
    {
        MRDA_LOG(LOG_ERROR, "open file to input ctx failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
#if MULTITHREADDECODING == 1
    std::thread receiveThread(ReceiveFrameThread, mrda_handle, sink, inputConfig.frameNum);
#endif
    // 5. receive main loop
    uint32_t curFrameNum = 0;
    uint64_t cur_pts = 0;
    std::chrono::high_resolution_clock clock{};
	uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    uint64_t singleExceedTime = 0;
    while (curFrameNum < inputConfig.frameNum)
    {
        uint64_t singleStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
        // 5.1 get input buffer
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
        // 5.2 fill input buffer
        if (MRDA_STATUS_SUCCESS != ReadFrame(input_ctx, inBuffer.get()))
        {
            MRDA_LOG(LOG_WARNING, "read frame invalid!");
            break;
        }
        // 5.3 decode one frame
#if MULTITHREADDECODING == 1
        if (MRDA_STATUS_SUCCESS != DecodeFrameAsync(mrda_handle, inBuffer))
        {
            MRDA_LOG(LOG_ERROR, "decode frame failed!");
            break;
        }
#else
        if (MRDA_STATUS_SUCCESS != DecodeFrame(mrda_handle, sink, inBuffer, &cur_pts))
        {
            MRDA_LOG(LOG_ERROR, "decode frame failed!");
            break;
        }
#endif
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
    // 6. flush decoder
    // 6.1 send last empty frame to trigger EOS
#if MULTITHREADDECODING == 1
    if (MRDA_STATUS_SUCCESS != DecodeFrameAsync(mrda_handle, nullptr))
    {
        MRDA_LOG(LOG_ERROR, "decode frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    receiveThread.join();
#else
    if (MRDA_STATUS_SUCCESS != DecodeFrame(mrda_handle, sink, nullptr, &cur_pts))
    {
        MRDA_LOG(LOG_ERROR, "decode frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // 6.2 receive process until all frames are received
    if (MRDA_STATUS_SUCCESS != FlushFrame(mrda_handle, sink, cur_pts, inputConfig.frameNum))
    {
        MRDA_LOG(LOG_ERROR, "flush frame failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
#endif
    uint64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    float total_fps = static_cast<float>(curFrameNum) * 1000 / (end - start);
    MRDA_LOG(LOG_INFO, "MRDA trace log: total decode frame num: %d, fps: %f", curFrameNum, total_fps);

    fclose(sink);
    avformat_close_input(&input_ctx);

    // fclose(source);
    // 7. stop mrda
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Stop(mrda_handle))
    {
        MRDA_LOG(LOG_ERROR, "stop mrda failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // 8. destroy mrda handle
    if (MRDA_STATUS_SUCCESS != MediaResourceDirectAccess_Destroy(mrda_handle))
    {
        MRDA_LOG(LOG_ERROR, "stop mrda failed!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    return MRDA_STATUS_SUCCESS;
}