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

 // MDSCSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
 //

#include "MultiDisplayScreenCapture.h"
#include "MRDAEncodeManager.h"
#include "QESEncodeManager.h"
#include "JsonConfig.h"
#include "common.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

bool g_exit = false;

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
    std::string hostSessionAddr;                //!< MRDA host session address

    // I/O
    std::string sourceFile;                     //!< input filename
    std::string sinkFile;                       //!< output filename
    std::string rtsp_url;                       //!< rtsp url
    INPUTTYPE input_type;                       //!< input type, capture or file
    OUTPUTTYPE output_type;                     //!< output type, stream or file

    //capture params
    uint32_t capture_fps;                       //!< capture fps
    bool capture_single_display;                //!< capture single display, true of false
    uint32_t capture_single_display_number;     //!< capture single display number
    std::string capture_dump_path;              //!< capture rgba file dump path, valid if DUMP_RGBA is defined

    // share memory info
    uint64_t totalMemorySize;                   //!< total memory size
    uint32_t bufferNum;                         //!< buffer number
    uint64_t bufferSize;                        //!< buffer size
    std::string in_mem_dev_path;                //!< input memory dev path
    std::string out_mem_dev_path;               //!< output memory dev path
    uint32_t in_mem_dev_slot_number;            //!< input memory dev slot number
    uint32_t out_mem_dev_slot_number;           //!< output memory dev slot number

    // encoding params
    uint32_t frameNum;                          //!< total frame number to encode
    std::string codec_id;                       //!< codec id, avc or hevc, h264 or h265
    uint32_t gop_size;                          //!< the distance between two adjacent intra frame
    uint32_t async_depth;                       //!< Specifies how many asynchronous operations an application performs
    std::string target_usage;                   //!< the preset for quality and performance balance,
    //!< balanced, quality, speed
    std::string rc_mode;                        //!< rate control mode, CQP or VBR
    uint32_t qp;                                //!< quantization value under CQP mode
    uint32_t bit_rate;                          //!< bitrate value under VBR mode
    int32_t  framerate_num;                     //!< frame rate numerator
    int32_t  framerate_den;                     //!< frame rate denominator
    uint32_t frame_width;                       //!< width of frame
    uint32_t frame_height;                      //!< height of frame
    std::string  input_color_format;            //!< input pixel color format
    std::string  output_pixel_format;           //!< output pixel color format
    std::string codec_profile;                  //!< the profile to create bitstream
    uint32_t max_b_frames;                      //!< maximum number of B-frames between non-B-frames
    Encode_Type encode_type;                    //!< encode type, ffmpeg-software, ffmpeg-MRDA, vpl-MRDA, QES
} InputConfig;

typedef struct ENCODE_THREAD_INPUT_PARAMS
{
    HANDLE TerminateThreadsEvent;
    DX_RESOURCES* DxRes;
    BufferQueue* PtrBufferQueue;
    UINT ThreadId;
    InputConfig* inputConfig;
    std::string EncDumpPath;

} EncodeThreadInputParams;

DWORD WINAPI EncodeProc(_In_ void* Param);

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    g_exit = true;
    return TRUE;
}

InputType StringToInputType(const char* input_type_str)
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

OutputType StringToOutputType(const char* output_type_str)
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

Encode_Type StringToEncodeType(const char* encode_type_str)
{
    Encode_Type encode_type = FFmpeg_SW;
    if (0 == strcmp(encode_type_str, "ffmpeg-software"))
    {
        encode_type = FFmpeg_SW;
    }
    else if (0 == strcmp(encode_type_str, "ffmpeg-MRDA"))
    {
        encode_type = FFmpeg_MRDA;
    }
    else if (0 == strcmp(encode_type_str, "vpl-MRDA"))
    {
        encode_type = VPL_MRDA;
    }
    else if (0 == strcmp(encode_type_str, "QES"))
    {
        encode_type = QES;
    }
    return encode_type;
}

BOOL ParseConfig(const std::string configFileName, InputConfig* inputConfig)
{
    // parse config
    JsonConfig json_config;
    if (!json_config.parse_file(configFileName))
    {
        printf("Failed to parse config file: %s", configFileName.c_str());
        return FALSE;
    }

    inputConfig->hostSessionAddr = json_config.get_string("MRDA-host-session-address");
    inputConfig->totalMemorySize = json_config.get_uint64("MRDA-memDevSize");
    inputConfig->bufferNum = json_config.get_uint32("MRDA-bufferNum");
    inputConfig->bufferSize = json_config.get_uint64("MRDA-bufferSize");
    inputConfig->in_mem_dev_path = json_config.get_string("MRDA-inDevPath");
    inputConfig->out_mem_dev_path = json_config.get_string("MRDA-outDevPath");
    inputConfig->in_mem_dev_slot_number = json_config.get_uint32("MRDA-inDevSlotNumber");
    inputConfig->out_mem_dev_slot_number = json_config.get_uint32("MRDA-outDevSlotNumber");

    inputConfig->input_type = StringToInputType(json_config.get_string("inputType").c_str());
    inputConfig->output_type = StringToOutputType(json_config.get_string("outputType").c_str());
    inputConfig->sourceFile = json_config.get_string("inputFile");
    inputConfig->sinkFile = json_config.get_string("outputFile");
    inputConfig->rtsp_url = "rtsp://" + json_config.get_string("rtsp-server-ip") + ":" + json_config.get_string("rtsp-server-port") + "/screencap";
    inputConfig->frame_width = json_config.get_uint32("inputWidth");
    inputConfig->frame_height = json_config.get_uint32("inputHeight");
    inputConfig->frameNum = json_config.get_uint32("frameNum");

    inputConfig->capture_fps = json_config.get_uint32("capture-fps");
    inputConfig->capture_single_display = json_config.get_boolean("capture-single-display");
    inputConfig->capture_single_display_number = json_config.get_uint32("capture-single-display-number");
    inputConfig->capture_dump_path = json_config.get_string("capture-dump-path");

    inputConfig->encode_type = StringToEncodeType(json_config.get_string("encode-type").c_str());
    inputConfig->input_color_format = json_config.get_string("inputColorFormat").c_str();
    inputConfig->output_pixel_format = json_config.get_string("outputPixelFormat").c_str();
    inputConfig->codec_id = json_config.get_string("encode-codecId").c_str();
    inputConfig->codec_profile = json_config.get_string("encode-codecProfile").c_str();
    inputConfig->async_depth = json_config.get_uint32("encode-async-depth");
    inputConfig->target_usage = json_config.get_string("encode-target-usage").c_str();
    inputConfig->rc_mode = json_config.get_string("encode-rcMode").c_str();
    inputConfig->qp = json_config.get_uint32("encode-qp");
    inputConfig->bit_rate = json_config.get_uint32("encode-bitrate");
    inputConfig->framerate_num = json_config.get_uint32("encode-fps");
    inputConfig->framerate_den = 1;
    inputConfig->gop_size = json_config.get_uint32("encode-gop");
    inputConfig->max_b_frames = json_config.get_uint32("encode-max-bFrames");
    return TRUE;
}

BOOL CreateEncodeParams(InputConfig* inputConfig, Encode_Params* encodeParams)
{
    if (!inputConfig || !encodeParams)
    {
        printf("NULL config pointer\n");
        return FALSE;
    }
    encodeParams->output_filename = inputConfig->sinkFile;
    encodeParams->rtsp_url = inputConfig->rtsp_url;
    encodeParams->bIsRtsp = inputConfig->output_type == OutputType::StreamOutput;
    encodeParams->encode_type = inputConfig->encode_type;
    encodeParams->width = inputConfig->frame_width;
    encodeParams->height = inputConfig->frame_height;
    encodeParams->codec_id = inputConfig->codec_id;
    encodeParams->rc_mode = inputConfig->rc_mode;
    encodeParams->qp = inputConfig->qp;
    encodeParams->bitrate = inputConfig->bit_rate;
    encodeParams->max_b_frames = inputConfig->max_b_frames;
    encodeParams->framerate_num = inputConfig->framerate_num;
    encodeParams->framerate_den = inputConfig->framerate_den;
    encodeParams->gop = inputConfig->gop_size;
    encodeParams->target_usage = inputConfig->target_usage;
    encodeParams->input_color_format = inputConfig->input_color_format;
    encodeParams->output_pixel_format = inputConfig->output_pixel_format;
    encodeParams->codec_profile = inputConfig->codec_profile;
    encodeParams->async_depth = inputConfig->async_depth;
    return TRUE;
}

BOOL CreateMRDAEncodeParams(InputConfig* inputConfig, MRDAEncode_Params* MRDAEncodeParams)
{
    if (!inputConfig || !MRDAEncodeParams)
    {
        printf("NULL config pointer\n");
        return FALSE;
    }
    MRDAEncodeParams->hostSessionAddr = inputConfig->hostSessionAddr;
    MRDAEncodeParams->totalMemorySize = inputConfig->totalMemorySize;
    MRDAEncodeParams->bufferNum = inputConfig->bufferNum;
    MRDAEncodeParams->bufferSize = inputConfig->bufferSize;
    MRDAEncodeParams->in_mem_dev_path = inputConfig->in_mem_dev_path;
    MRDAEncodeParams->out_mem_dev_path = inputConfig->out_mem_dev_path;
    MRDAEncodeParams->in_mem_dev_slot_number = inputConfig->in_mem_dev_slot_number;
    MRDAEncodeParams->out_mem_dev_slot_number = inputConfig->out_mem_dev_slot_number;
    MRDAEncodeParams->frameNum = inputConfig->frameNum;

    return TRUE;
}

BOOL CreateEncodeManager(EncodeThreadInputParams* TData, EncodeManager** video_encode, Encode_Params& EncParams, MRDAEncode_Params& MRDAEncParams)
{
    if (FFmpeg_SW == TData->inputConfig->encode_type)
    {
        *video_encode = new EncodeManager();
        if ((*video_encode)->Init(EncParams))
        {
            printf("[Thread][%d], Failed to init video encoder\n", TData->ThreadId);
            return FALSE;
        }
    }
    else if (FFmpeg_MRDA == TData->inputConfig->encode_type || VPL_MRDA == TData->inputConfig->encode_type)
    {
        *video_encode = new MRDAEncodeManager();
        MRDAEncodeManager* mrda_video_encode = dynamic_cast<MRDAEncodeManager*>(*video_encode);
        MRDAEncParams.encode_params = EncParams;
        if (mrda_video_encode->Init(MRDAEncParams))
        {
            printf("[Thread][%d], Failed to init MRDA video encoder\n", TData->ThreadId);
            return FALSE;
        }
    }
    else if (QES == TData->inputConfig->encode_type)
    {
        *video_encode = new QESEncodeManager();
        QESEncodeManager* qes_video_encode = dynamic_cast<QESEncodeManager*>(*video_encode);
        if (QES_ERR_NONE != qes_video_encode->Init(EncParams, TData->DxRes))
        {
            printf("[Thread][%d], Failed to init QES video encoder\n", TData->ThreadId);
            return FALSE;
        }
        if (FileInput == TData->inputConfig->input_type)
        {
            TData->DxRes = qes_video_encode->GetDXResource();
        }
    }
    return TRUE;
}

BOOL WaitForCapturedDataReady(EncodeThreadInputParams* TData, Encode_Params& EncParams)
{
    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        if (TData->PtrBufferQueue->GetSize() > 0)
        {
            CapturedData Data = TData->PtrBufferQueue->DequeueBuffer();

            if (Data.CapturedTexture)
            {
                D3D11_TEXTURE2D_DESC desc;
                Data.CapturedTexture->GetDesc(&desc);
                EncParams.width = desc.Width;
                EncParams.height = desc.Height;
                Data.CapturedTexture->Release();
                Data.CapturedTexture = nullptr;
                printf("[thread][%d], The display resolution is %dx%d\n", TData->ThreadId, desc.Width, desc.Height);
                break;
            }
            else
            {
                printf("[Thread][%d], No valid texture in captured data\n", TData->ThreadId);
                return FALSE;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    return TRUE;
}

BOOL ReadRawFrame(FILE* fp, uint8_t* data, uint32_t frame_size, UINT threadId)
{
    if (nullptr == data || nullptr == fp || 0 == frame_size)
    {
        printf("[thread][%d], ReadRawFrame error: invalid input\n", threadId);
        return FALSE;
    }

    static int cnt = 0;
    static long file_size = 0;
    static int total_frames = 0;
    if (file_size == 0 || total_frames == 0)
    {
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        total_frames = file_size / frame_size;
    }

    long current_frame_offset = (cnt % total_frames) * frame_size;
    fseek(fp, current_frame_offset, SEEK_SET);
    size_t read_size = fread(data, 1, frame_size, fp);
    if (read_size != frame_size)
    {
        printf("[thread][%d], ReadRawFrame error: failed to read the complete frame\n", threadId);
        return FALSE;
    }

    cnt++;

    return TRUE;
}

BOOL GetCapturedFrame(BufferQueue* PtrBufferQueue, CapturedData& CurrentData, CapturedData& LastData, UINT threadId, uint32_t FrameNum)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> st_getframe_stp = std::chrono::high_resolution_clock::now();

    int BufferQueueSize = PtrBufferQueue->GetSize();
    if (BufferQueueSize > 0)
    {
        CurrentData = PtrBufferQueue->DequeueBuffer();
#ifdef _ENABLE_TRACE_
        std::chrono::time_point<std::chrono::high_resolution_clock> ed_dequeue_stp = std::chrono::high_resolution_clock::now();
        uint64_t dequeue_timecost = std::chrono::duration_cast<std::chrono::microseconds>(ed_dequeue_stp - st_getframe_stp).count();
        printf("[thread][%d], frame %d, dequeue buffer costtime %fms\n", threadId, FrameNum, dequeue_timecost / 1000.0);
#endif
        if (LastData.CapturedTexture)
        {
            LastData.CapturedTexture->Release();
            LastData.CapturedTexture = nullptr;
        }
        std::chrono::time_point<std::chrono::high_resolution_clock> ed_releaseTex_tp = std::chrono::high_resolution_clock::now();
#ifdef _ENABLE_TRACE_
        uint64_t releaseTex_timecost = std::chrono::duration_cast<std::chrono::microseconds>(ed_releaseTex_tp - ed_dequeue_stp).count();
        printf("[thread][%d], frame %d, release texture costtime %fms\n", threadId, FrameNum, releaseTex_timecost / 1000.0);
#endif
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
            return FALSE;
        }
    }
#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> ed_getframe_tp = std::chrono::high_resolution_clock::now();
    uint64_t getframe_timecost = std::chrono::duration_cast<std::chrono::microseconds>(ed_getframe_tp - st_getframe_stp).count();
    printf("[thread][%d], frame %d, dequeue buffer total costtime %fms\n", threadId, FrameNum, getframe_timecost / 1000.0);
#endif
    return TRUE;
}

MRDAStatus ReadCapturedFrame(DX_RESOURCES* DxRes, ID3D11Texture2D* CapturedTexture, FrameBufferItem* data, int pts)
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

ID3D11Texture2D* CreateD3D11Texture2D(EncodeThreadInputParams* TData, uint8_t* data, int frame_size)
{
    ID3D11Texture2D* texture = nullptr;

    // Describe the texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = TData->inputConfig->frame_width;
    desc.Height = TData->inputConfig->frame_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    if (!strcmp(TData->inputConfig->input_color_format.c_str(), "nv12"))
    {
        desc.Format = DXGI_FORMAT_NV12;
    }
    else if (!strcmp(TData->inputConfig->input_color_format.c_str(), "rgb32"))
    {
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        printf("[thread][%d], Unknown or unsupported color format %s!\n", TData->ThreadId, TData->inputConfig->input_color_format.c_str());
        return nullptr;
    }
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // Set up subresource data
    D3D11_SUBRESOURCE_DATA initData = {};
    if (desc.Format == DXGI_FORMAT_NV12) {
        initData.pSysMem = data;
        initData.SysMemPitch = TData->inputConfig->frame_width;
        initData.SysMemSlicePitch = frame_size;
    }
    else if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) {
        // BGRA format: 4 bytes per pixel
        initData.pSysMem = data;
        initData.SysMemPitch = TData->inputConfig->frame_width * 4;
        initData.SysMemSlicePitch = frame_size;
    }

    // Create the texture
    HRESULT hr = TData->DxRes->Device->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) {
        return nullptr;
    }

    return texture;
}

BOOL EncodeCapturedFrame(EncodeThreadInputParams* TData, CapturedData& CurrentData, EncodeManager* video_encode, uint32_t frameNum, FILE* dump_fp = nullptr)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (CurrentData.CapturedTexture)
    {
        if (QES == TData->inputConfig->encode_type)
        {
            QESEncodeManager* qes_video_encode = dynamic_cast<QESEncodeManager*>(video_encode);
            qesStatus sts = qes_video_encode->Encode(CurrentData.CapturedTexture, CurrentData.AcquiredTime);
            if (QES_ERR_NONE != sts)
            {
                printf("[thread][%d], frame %u, failed to encode QES frame\n", TData->ThreadId, frameNum);
                return FALSE;
            }
        }
        else
        {
            HRESULT hr = TData->DxRes->Context->Map(CurrentData.CapturedTexture, 0, D3D11_MAP_READ, 0, &mapped);
            if (FAILED(hr))
            {
                printf("[thread][%d], frame %u, failed to map texture\n", TData->ThreadId, frameNum);
                CurrentData.CapturedTexture->Release();
                CurrentData.CapturedTexture = nullptr;
                return FALSE;
            }

            if (mapped.pData)
            {
                uint8_t* data = static_cast<uint8_t*>(mapped.pData);
#ifdef DUMP_RGBA
                fwrite(data, mapped.DepthPitch, 1, dump_fp);
#endif
                if (data != NULL)
                {
#ifdef _ENABLE_TRACE_
                    std::chrono::time_point<std::chrono::high_resolution_clock> enc_sttp = std::chrono::high_resolution_clock::now();
#endif
                    if (FFmpeg_SW == TData->inputConfig->encode_type)
                    {
                        if (0 != video_encode->Encode(data, CurrentData.AcquiredTime))
                        {
                            CurrentData.CapturedTexture->Release();
                            CurrentData.CapturedTexture = nullptr;
                            printf("[thread][%d], frame %u, ffmpeg software failed to encode frame\n", TData->ThreadId, frameNum);
                            return FALSE;
                        }
                    }
                    else if (FFmpeg_MRDA == TData->inputConfig->encode_type || VPL_MRDA == TData->inputConfig->encode_type)
                    {
                        MRDAEncodeManager* mrda_video_encode = dynamic_cast<MRDAEncodeManager*>(video_encode);
                        FrameBufferItem* pInputBuffer = mrda_video_encode->GetBufferForInput();
                        if (nullptr == pInputBuffer || mapped.DepthPitch != pInputBuffer->bufferItem->occupied_size)
                        {
                            CurrentData.CapturedTexture->Release();
                            CurrentData.CapturedTexture = nullptr;
                            printf("[thread][%d], frame %u, MRDA failed to get buffer for input\n", TData->ThreadId, frameNum);
                            return FALSE;
                        }
                        pInputBuffer->pts = CurrentData.AcquiredTime;
                        memcpy(pInputBuffer->bufferItem->buf_ptr, data, pInputBuffer->bufferItem->occupied_size);
                        if (0 != mrda_video_encode->Encode())
                        {
                            CurrentData.CapturedTexture->Release();
                            CurrentData.CapturedTexture = nullptr;
                            printf("[thread][%d], frame %u, MRDA failed to encode frame\n", TData->ThreadId, frameNum);
                            return FALSE;
                        }
                    }
#ifdef _ENABLE_TRACE_
                    std::chrono::time_point<std::chrono::high_resolution_clock> enc_endtp = std::chrono::high_resolution_clock::now();
                    uint64_t enc_timecost = std::chrono::duration_cast<std::chrono::microseconds>(enc_endtp - enc_sttp).count();
                    printf("[thread][%d], frame %u, encodeframe costtime %fms\n", TData->ThreadId, frameNum, enc_timecost / 1000.0);
#endif
                }
            }
            else
            {
                printf("[thread][%d], frame %u, No valid mapped Texture Data\n", TData->ThreadId, frameNum);
                CurrentData.CapturedTexture->Release();
                CurrentData.CapturedTexture = nullptr;
                return true;
            }

            TData->DxRes->Context->Unmap(CurrentData.CapturedTexture, 0);
        }
    }
    return TRUE;
}

uint64_t CalRawFrameSize(std::string colorFormat, int width, int height, UINT threadId)
{
    int frame_size = 0;
    if (!strcmp(colorFormat.c_str(), "nv12") || !strcmp(colorFormat.c_str(), "yuv420p"))
    {
        frame_size = width * height * 3 / 2;
    }
    else if (!strcmp(colorFormat.c_str(), "rgb32"))
    {
        frame_size = width * height * 4;
    }
    else
    {
        printf("[thread][%d], Unknown or unsupported color format %s!\n", threadId, colorFormat.c_str());
    }

    return frame_size;
}

BOOL EncodeFileRawFrame(EncodeThreadInputParams* TData, FILE* fpSource, EncodeManager* video_encode, uint8_t* data, uint64_t FrameSize, uint32_t frameNum, FILE* dump_fp = nullptr)
{
#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> enc_sttp = std::chrono::high_resolution_clock::now();
#endif

    if (FFmpeg_SW == TData->inputConfig->encode_type)
    {
        if (ReadRawFrame(fpSource, data, FrameSize, TData->ThreadId))
        {
            if (0 != video_encode->Encode(data, frameNum))
            {
                printf("[thread][%d], frame %u, ffmpeg software failed to encode file raw frame\n", TData->ThreadId, frameNum);
                return FALSE;
            }
        }
    }
    else if (FFmpeg_MRDA == TData->inputConfig->encode_type || VPL_MRDA == TData->inputConfig->encode_type)
    {
        MRDAEncodeManager* mrda_video_encode = dynamic_cast<MRDAEncodeManager*>(video_encode);
        FrameBufferItem* pInputBuffer = mrda_video_encode->GetBufferForInput();
        if (nullptr == pInputBuffer)
        {
            printf("[thread][%d], frame %u, MRDA failed to get buffer for input\n", TData->ThreadId, frameNum);
            return FALSE;
        }
        if (FrameSize != pInputBuffer->bufferItem->occupied_size)
        {
            printf("[thread][%d], frame %u, MRDA failed to get buffer for input\n", TData->ThreadId, frameNum);
            pInputBuffer->uninit();
            return FALSE;
        }
        if (ReadRawFrame(fpSource, pInputBuffer->bufferItem->buf_ptr, FrameSize, TData->ThreadId))
        {
            pInputBuffer->bufferItem->occupied_size = FrameSize;
            pInputBuffer->pts = frameNum;
            if (MRDA_STATUS_SUCCESS != mrda_video_encode->Encode())
            {
                printf("[thread][%d], frame %u, MRDA failed to encode frame\n", TData->ThreadId, frameNum);
                return FALSE;
            }
        }
    }
    else if (QES == TData->inputConfig->encode_type)
    {
        if (ReadRawFrame(fpSource, data, FrameSize, TData->ThreadId))
        {
            QESEncodeManager* qes_video_encode = dynamic_cast<QESEncodeManager*>(video_encode);
            ID3D11Texture2D* texture = CreateD3D11Texture2D(TData, data, FrameSize);
            if (nullptr == texture)
            {
                printf("[thread][%d], frame %u, QES failed to create texture with input raw frame\n", TData->ThreadId, frameNum);
            }
            if (QES_ERR_NONE != qes_video_encode->Encode(texture, frameNum))
            {
                printf("[thread][%d], frame %u, failed to encode QES file raw frame\n", TData->ThreadId, frameNum);
                texture->Release();
                texture = nullptr;
                return FALSE;
            }
            texture->Release();
            texture = nullptr;
        }
    }

#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> enc_endtp = std::chrono::high_resolution_clock::now();
    uint64_t enc_timecost = std::chrono::duration_cast<std::chrono::microseconds>(enc_endtp - enc_sttp).count();
    printf("[thread][%d], frame %u, encodeframe costtime %fms\n", TData->ThreadId, frameNum, enc_timecost / 1000.0);
#endif

    return TRUE;
}

void ControlFPS(uint64_t timecost, uint64_t& Timeout, uint64_t EncodeInterval, uint32_t FrameNum, int ThreadId)
{
    if (timecost < EncodeInterval)
    {
        uint64_t sleep_time = EncodeInterval - timecost;
        if (sleep_time <= Timeout)
        {
            Timeout -= sleep_time;
        }
        else
        {
            sleep_time -= Timeout;
            Timeout = 0;
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
        }
#ifdef _ENABLE_TRACE_
        printf("[thread][%d], frame %u, timeout %fms, sleep_time %fms\n", ThreadId, FrameNum, Timeout / 1000.0, sleep_time / 1000.0);
#endif
    }
    else if (timecost > EncodeInterval)
    {
        Timeout += timecost - EncodeInterval;
    }
}

DWORD WINAPI EncodeProc(_In_ void* Param)
{
    // Data passed in from thread creation
    EncodeThreadInputParams* TData = reinterpret_cast<EncodeThreadInputParams*>(Param);
    InputConfig* inputConfig = TData->inputConfig;
    Encode_Params EncParams;
    MRDAEncode_Params MRDAEncParams;
    FILE* fpSource = nullptr;

    // Prepare Encode Params
    if (!CreateEncodeParams(inputConfig, &EncParams))
    {
        printf("fail to create encode params\n");
    }
    EncParams.threadId = TData->ThreadId;

    if (!CreateMRDAEncodeParams(inputConfig, &MRDAEncParams))
    {
        printf("fail to create mrda encode params\n");
    }

    // Prepare Input Data
    if (inputConfig->input_type == CaptureInput)
    {
        if (!WaitForCapturedDataReady(TData, EncParams))
            return -1;
    }
    else if (inputConfig->input_type == FileInput)
    {
        if (fopen_s(&fpSource, inputConfig->sourceFile.c_str(), "rb") != 0)
        {
            printf("[Thread][%d], open input file %s failed!\n", TData->ThreadId, inputConfig->sourceFile.c_str());
            return -1;
        }
    }
    else
    {
        printf("Error: Unsupported input type\n");
        return -1;
    }

    //Create EncodeManager
    EncodeManager* video_encode = nullptr;
    if (!CreateEncodeManager(TData, &video_encode, EncParams, MRDAEncParams))
        return -1;

    //Start Get Input Frame and Encode Loop
    uint32_t FrameNum = 0;
    CapturedData CurrentData = CapturedData{};
    CapturedData LastData = CapturedData{};
    uint8_t* FileData = nullptr;

    uint64_t FrameSize = CalRawFrameSize(TData->inputConfig->input_color_format, TData->inputConfig->frame_width, TData->inputConfig->frame_height, TData->ThreadId);
    FileData = new uint8_t[FrameSize];

#ifdef DUMP_RGBA
    std::string CapDumpFileName = TData->inputConfig->capture_dump_path + std::to_string(TData->ThreadId) + ".rgba";
    FILE* fpDump = fopen(CapDumpFileName.c_str(), "wb");
#endif

    uint64_t EncodeInterval = EncParams.framerate_num != 0 ? 1000000 / EncParams.framerate_num : 0;
    printf("[thread][%d], framerate_num %d, EncodeInterval %fms\n", TData->ThreadId, EncParams.framerate_num, EncodeInterval / 1000.0);
    uint64_t Timeout = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> st_enc_tp = std::chrono::high_resolution_clock::now();

    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT) && FrameNum < inputConfig->frameNum)
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> starttp = std::chrono::high_resolution_clock::now();

        if (inputConfig->input_type == CaptureInput)
        {
            if (!GetCapturedFrame(TData->PtrBufferQueue, CurrentData, LastData, TData->ThreadId, FrameNum))
                continue;
#ifdef DUMP_RGBA
            if (EncodeCapturedFrame(TData, CurrentData, video_encode, FrameNum, fpDump))
#else
            if (!EncodeCapturedFrame(TData, CurrentData, video_encode, FrameNum))
#endif
                break;
        }
        else if (inputConfig->input_type == FileInput)
        {
            if (!EncodeFileRawFrame(TData, fpSource, video_encode, FileData, FrameSize, FrameNum))
                break;
        }
        else
        {
            printf("Error: Unsupported input type\n");
            return -1;
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
        uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();
#ifdef _ENABLE_TRACE_
        printf("[thread][%d], frame %u, dequeue and encodeframe costtime %fms\n", TData->ThreadId, FrameNum, timecost / 1000.0);
#endif

        ControlFPS(timecost, Timeout, EncodeInterval, FrameNum, TData->ThreadId);

        FrameNum++;
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> ed_enc_tp = std::chrono::high_resolution_clock::now();
    uint64_t enc_duration = std::chrono::duration_cast<std::chrono::microseconds>(ed_enc_tp - st_enc_tp).count();

#ifdef DUMP_RGBA
    fclose(fpDump);
#endif
    video_encode->End_video_output();

    if (CurrentData.CapturedTexture)
    {
        CurrentData.CapturedTexture->Release();
        CurrentData.CapturedTexture = nullptr;
    }

    if (nullptr != FileData)
    {
        delete[]FileData;
        FileData = nullptr;
    }

    delete video_encode;
    video_encode = nullptr;

    if (inputConfig->input_type == FileInput)
    {
        fclose(fpSource);
        fpSource = nullptr;
    }

    float total_fps = static_cast<float>(FrameNum) * 1000000 / enc_duration;
    printf("[thread][%d], Total encode frame num: %u, fps: %f\n", TData->ThreadId, FrameNum, total_fps);
    return 0;
}

int main()
{
    printf("MultiDisplay Screen Capture Sample!\n");

    // parse config
    InputConfig inputConfig = {};
    std::string configFilename = "MDSCMRDASample.conf";
    if (!ParseConfig(configFilename, &inputConfig))
    {
        printf("fail to parse config\n");
    }

    // init and start capture
    std::vector<std::string> InputFileNames;
    MultiDisplayScreenCapture MDSCsample;
    UINT DispNum = 0;
    UINT OutNum = 0;
    BufferQueue* BufferQueues = nullptr;

    if (inputConfig.input_type == CaptureInput)
    {
        MDSCsample.Init();
        DispNum = MDSCsample.GetDisplayCount();
        printf("Display Num is %d\n", DispNum);

        if (inputConfig.capture_single_display)
        {
            if (inputConfig.capture_single_display_number < DispNum)
            {
                printf("Capture single display Id %d as config selected\n", inputConfig.capture_single_display_number);
            }
            else
            {
                printf("Invalid single display Id %d in config, capture single disp 0 as default\n", inputConfig.capture_single_display_number);
                inputConfig.capture_single_display_number = 0;
            }
            MDSCsample.SetSingleDisplay(inputConfig.capture_single_display_number);
        }

        OutNum = MDSCsample.GetOutputCount();
        printf("Output Num is %d\n", OutNum);

        MDSCsample.SetCaptureFps(inputConfig.capture_fps);

        MDSCsample.StartCaptureScreen();

        BufferQueues = MDSCsample.GetBufferQueues();
    }
    else if (inputConfig.input_type == FileInput)
    {
        printf("FileInput: ");
        size_t pos = 0;
        std::string inputfilename = inputConfig.sourceFile;
        while ((pos = inputfilename.find(',')) != std::string::npos)
        {
            std::string filename = inputfilename.substr(0, pos);
            InputFileNames.push_back(filename);
            inputfilename.erase(0, pos + 1);
            OutNum++;
            printf("%s ", filename.c_str());
        }
        if (!inputfilename.empty()) {
            InputFileNames.push_back(inputfilename);
            OutNum++;
            printf("%s ", inputfilename.c_str());
        }
        printf("\n");
    }
    else
    {
        printf("Error: Unsupported input type\n");
        return -1;
    }

    // init and start encode and stream
    EncodeThreadInputParams* ThreadData = new EncodeThreadInputParams[OutNum];
    HANDLE* ThreadHandles = new HANDLE[OutNum];

    HANDLE TerminateEncodeEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!TerminateEncodeEvent)
    {
        printf("Failed to create TerminateThreadsEvent\n");
        return SCREENCAP_FAILED;
    }

    printf("Start to create %d encoding threads\n", OutNum);
    for (UINT i = 0; i < OutNum; ++i)
    {
        ThreadData[i].TerminateThreadsEvent = TerminateEncodeEvent;
        ThreadData[i].DxRes = inputConfig.input_type == CaptureInput ? MDSCsample.GetDXResource(i) : nullptr;
        ThreadData[i].PtrBufferQueue = &BufferQueues[i];
        ThreadData[i].ThreadId = inputConfig.input_type == CaptureInput && inputConfig.capture_single_display ? inputConfig.capture_single_display_number : i;
        ThreadData[i].inputConfig = &inputConfig;
        ThreadData[i].inputConfig->sourceFile = inputConfig.input_type == FileInput ? InputFileNames[i] : "";
        ThreadData[i].EncDumpPath = inputConfig.sinkFile;

        DWORD ThreadId;
        ThreadHandles[i] = CreateThread(nullptr, 0, EncodeProc, &ThreadData[i], 0, &ThreadId);
        if (ThreadHandles[i] == nullptr)
        {
            printf("Failed to create encoding thread %d\n", i);
            return SCREENCAP_FAILED;
        }
        SetThreadPriority(ThreadHandles[i], THREAD_PRIORITY_HIGHEST);
    }

    // check end
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    while (!g_exit)
    {
        if (MDSCsample.CheckIfCaptureTerminated())
        {
            goto Exit;
        }
        for (UINT i = 0; i < OutNum; ++i)
        {
            DWORD res = WaitForSingleObjectEx(ThreadHandles[i], 1000 / inputConfig.framerate_num, false);
            if (res != WAIT_TIMEOUT)
            {
                printf("Checked that encode thread ended %d\n", i);
                goto Exit;
            }
        }
    }

Exit:
    // Make sure all encode threads have exited
    if (SetEvent(TerminateEncodeEvent))
    {
        DWORD res = WaitForMultipleObjectsEx(OutNum, ThreadHandles, TRUE, INFINITE, FALSE);
        printf("All encode thread terminated\n");
    }

    // Clean up
    CloseHandle(TerminateEncodeEvent);

    // Make sure all capture threads have exited
    if (inputConfig.input_type == CaptureInput)
    {
        MDSCsample.TerminateCaptureScreen();
        MDSCsample.DeInit();
        printf("All capture thread terminated\n");
    }

    if (ThreadData)
    {
        delete[] ThreadData;
        ThreadData = nullptr;
    }

    return 0;
}
