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
    DX_RESOURCES *DxRes;
    BufferQueue  *PtrBufferQueue;
    UINT ThreadId;
    InputConfig *inputConfig;
    std::string EncDumpPath;

} EncodeThreadInputParams;

DWORD WINAPI EncodeProc(_In_ void* Param);

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    g_exit = true;
    return TRUE;
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

Encode_Type StringToEncodeType(const char *encode_type_str)
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

BOOL ParseConfig(const std::string configFileName, InputConfig *inputConfig)
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
    inputConfig->bufferNum = json_config.get_uint64("MRDA-bufferNum");
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

BOOL CreateEncodeParams(InputConfig *inputConfig, Encode_Params *encodeParams)
{
    if ( !inputConfig || !encodeParams )
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

BOOL CreateMRDAEncodeParams(InputConfig *inputConfig, MRDAEncode_Params *MRDAEncodeParams)
{
    if ( !inputConfig || !MRDAEncodeParams )
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

DWORD WINAPI EncodeProc(_In_ void* Param)
{
    // Data passed in from thread creation
    EncodeThreadInputParams* TData = reinterpret_cast<EncodeThreadInputParams*>(Param);

    InputConfig* inputConfig = TData->inputConfig;
    Encode_Params EncParams;
    MRDAEncode_Params MRDAEncParams;
    if(!CreateEncodeParams(inputConfig, &EncParams))
    {
        printf("fail to create encode params\n");
    }
    EncParams.threadId = TData->ThreadId;

    if(!CreateMRDAEncodeParams(inputConfig, &MRDAEncParams))
    {
        printf("fail to create mrda encode params\n");
    }

    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        if (TData->PtrBufferQueue->GetSize() > 0)
        {
            CapturedData Data = TData->PtrBufferQueue->DequeueBuffer();

            D3D11_TEXTURE2D_DESC desc;

            if (Data.CapturedTexture)
            {
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
                return -1;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    EncParams.st_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    EncodeManager *video_encode = nullptr;
    if(FFmpeg_SW == TData->inputConfig->encode_type)
    {
        video_encode = new EncodeManager();
        if (video_encode->Init(EncParams))
        {
            printf("[Thread][%d], Failed to init video encoder\n", TData->ThreadId);
            return -1;
        }
    }
    else if(FFmpeg_MRDA == TData->inputConfig->encode_type || VPL_MRDA == TData->inputConfig->encode_type)
    {
        video_encode = new MRDAEncodeManager();
		MRDAEncodeManager* mrda_video_encode = dynamic_cast<MRDAEncodeManager*>(video_encode);
		MRDAEncParams.encode_params = EncParams;
		if (mrda_video_encode->Init(MRDAEncParams))
		{
			printf("[Thread][%d], Failed to init MRDA video encoder\n", TData->ThreadId);
			return -1;
		}
    }
    else if(QES == TData->inputConfig->encode_type)
    {
        video_encode = new QESEncodeManager();
        QESEncodeManager* qes_video_encode = dynamic_cast<QESEncodeManager*>(video_encode);
        if (qes_video_encode->Init(EncParams, TData->DxRes))
        {
            printf("[Thread][%d], Failed to init QES video encoder\n", TData->ThreadId);
            return -1;
        }
    }

    uint32_t n = 0;
    CapturedData CurrentData = CapturedData{};
    CapturedData LastData = CapturedData{};
#ifdef DUMP_RGBA
    std::string CapDumpFileName = TData->inputConfig->capture_dump_path + std::to_string(TData->ThreadId) + ".rgba";
    FILE* fp = fopen(CapDumpFileName.c_str(), "wb");
#endif

    std::chrono::time_point<std::chrono::high_resolution_clock> st_enc_tp = std::chrono::high_resolution_clock::now();

    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT) && n < inputConfig->frameNum)
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> starttp = std::chrono::high_resolution_clock::now();

        int BufferQueueSize = TData->PtrBufferQueue->GetSize();
        if (BufferQueueSize > 0)
        {
            CurrentData = TData->PtrBufferQueue->DequeueBuffer();

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
                continue;
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        D3D11_MAPPED_SUBRESOURCE mapped;

        if (CurrentData.CapturedTexture)
        {
            if (QES == TData->inputConfig->encode_type)
            {
                QESEncodeManager* qes_video_encode = dynamic_cast<QESEncodeManager*>(video_encode);
                qesStatus sts = qes_video_encode->Encode(CurrentData.CapturedTexture, CurrentData.AcquiredTime);
                if (QES_ERR_NONE != sts)
                {
                    printf("[Thread][%d], frame %d, failed to encode QES frame\n", TData->ThreadId, n);
                    break;
                }
            }
            else
            {
                HRESULT hr = TData->DxRes->Context->Map(CurrentData.CapturedTexture, 0, D3D11_MAP_READ, 0, &mapped);
                if (FAILED(hr))
                {
                    printf("[Thread][%d], frame %d, failed to map texture\n", TData->ThreadId, n);
                    CurrentData.CapturedTexture->Release();
                    CurrentData.CapturedTexture = nullptr;
                    continue;
                }

                if (mapped.pData)
                {
                    uint8_t* data = static_cast<uint8_t*>(mapped.pData);
    #ifdef DUMP_RGBA
                    fwrite(data, mapped.DepthPitch, 1, fp);
    #endif
                    if (data != NULL)
                    {
                        int ret = video_encode->Encode(data, CurrentData.AcquiredTime);
                        while (MRDA_STATUS_NOT_READY == ret)
                        {
                            ret = video_encode->Encode(data, CurrentData.AcquiredTime);
                        }
                        if (ret != 0)
                        {
                            CurrentData.CapturedTexture->Release();
                            CurrentData.CapturedTexture = nullptr;
                            break;
                        }
                    }
                }
                else
                {
                    printf("[Thread][%d], frame %d, No valid mapped Texture Data\n", TData->ThreadId, n);
                    CurrentData.CapturedTexture->Release();
                    CurrentData.CapturedTexture = nullptr;
                    continue;
                }

                TData->DxRes->Context->Unmap(CurrentData.CapturedTexture, 0);
            }
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
        uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();
        //printf("thread %d, dequeue frame %d, CurrentData texture ptr %p, AcquiredTime %u, encodeframe costtime %dus\n", TData->ThreadId, n, CurrentData.CapturedTexture, CurrentData.AcquiredTime, timecost);
        uint64_t EncodeInterval = EncParams.framerate_num != 0 ? 1000000 / EncParams.framerate_num : 0;
        if (timecost < EncodeInterval)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(EncodeInterval - timecost));
        }

        n++;
    }

	std::chrono::time_point<std::chrono::high_resolution_clock> ed_enc_tp = std::chrono::high_resolution_clock::now();
    uint64_t enc_duration = std::chrono::duration_cast<std::chrono::microseconds>(ed_enc_tp - st_enc_tp).count();

#ifdef DUMP_RGBA
    fclose(fp);
#endif
    video_encode->End_video_output();

    if (CurrentData.CapturedTexture)
    {
        CurrentData.CapturedTexture->Release();
        CurrentData.CapturedTexture = nullptr;
    }

    delete video_encode;
    video_encode = nullptr;

    float total_fps = static_cast<float>(n) * 1000000 / enc_duration;
    printf("[Thread][%d], Total encode frame num: %d, fps: %f\n", TData->ThreadId, n, total_fps);
    return 0;
}

int main()
{
    printf("MultiDisplay Screen Capture Sample!\n");

    // parse config
    InputConfig inputConfig = {};
    std::string configFilename = "MDSCMRDASample.conf";
    if(!ParseConfig(configFilename, &inputConfig))
    {
        printf("fail to parse config\n");
    }

    // init and start capture
    MultiDisplayScreenCapture MDSCsample;
    MDSCsample.Init();
    UINT DispNum = MDSCsample.GetDisplayCount();
    printf("Display Num is %d", DispNum);

    if (inputConfig.capture_single_display)
    {
        if (inputConfig.capture_single_display_number < DispNum)
        {
            printf("Capture single display Id %d as config selected", inputConfig.capture_single_display_number);
        }
        else
        {
            printf("Invalid single display Id %d in config, capture single disp 0 as default", inputConfig.capture_single_display_number);
            inputConfig.capture_single_display_number = 0;
        }
        MDSCsample.SetSingleDisplay(inputConfig.capture_single_display_number);
    }

    UINT OutNum = MDSCsample.GetOutputCount();
    printf("Output Num is %d\n", OutNum);

    MDSCsample.SetCaptureFps(inputConfig.capture_fps);

    MDSCsample.StartCaptureScreen();

    // init and start encode and stream
    BufferQueue* BufferQueues = MDSCsample.GetBufferQueues();

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
        ThreadData[i].DxRes = MDSCsample.GetDXResource(i);
        ThreadData[i].PtrBufferQueue = &BufferQueues[i];
        ThreadData[i].ThreadId = inputConfig.capture_single_display ? inputConfig.capture_single_display_number : i;
        ThreadData[i].inputConfig = &inputConfig;
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
            DWORD res = WaitForSingleObjectEx(ThreadHandles[i], 1000/inputConfig.framerate_num, false);
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
    MDSCsample.DeInit();

    if (ThreadData)
    {
        delete [] ThreadData;
        ThreadData = nullptr;
    }

    return 0;
}
