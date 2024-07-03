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
#include "EncodeManager.h"
#include "JsonConfig.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

typedef struct ENCODE_THREAD_INPUT_PARAMS
{
    HANDLE TerminateThreadsEvent;
    DX_RESOURCES *DxRes;
    BufferQueue  *PtrBufferQueue;
    UINT ThreadId;
    std::string CapDumpPath;
    Encode_Params EncParams;
    bool StreamMode;
    std::string StreamUrl;
    std::string EncDumpPath;

} EncodeThreadInputParams;

bool g_exit = false;

DWORD WINAPI EncodeProc(_In_ void* Param);

DWORD WINAPI EncodeProc(_In_ void* Param)
{
    // Data passed in from thread creation
    EncodeThreadInputParams* TData = reinterpret_cast<EncodeThreadInputParams*>(Param);
    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        if (TData->PtrBufferQueue->GetSize() > 0)
        {
            CapturedData Data = TData->PtrBufferQueue->DequeueBuffer();

            D3D11_TEXTURE2D_DESC desc;

            if (Data.CapturedTexture)
            {
                Data.CapturedTexture->GetDesc(&desc);
                TData->EncParams.width = desc.Width;
                TData->EncParams.height = desc.Height;
                Data.CapturedTexture->Release();
                Data.CapturedTexture = nullptr;
                printf("[Thread][%d], The display resolution is %dx%d\n", TData->ThreadId, desc.Width, desc.Height);
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

    EncodeManager video_encode = EncodeManager{};
    if (TData->StreamMode)
    {
        std::string rtsp_url = TData->StreamUrl + std::to_string(TData->ThreadId);
        if (video_encode.Init(TData->EncParams, rtsp_url, true))
        {
            printf("[Thread][%d], Failed to init stream mode video encoder\n", TData->ThreadId);
            return -1;
        }
    }
    else
    {
        std::string output_filename = TData->EncDumpPath + std::to_string(TData->ThreadId) + ".mp4";
        if (video_encode.Init(TData->EncParams, output_filename, false))
        {
            printf("[Thread][%d], Failed to init file dump mode video encoder\n", TData->ThreadId);
            return -1;
        }
    }

    int n = 0;
    CapturedData CurrentData = CapturedData{};
    CapturedData LastData = CapturedData{};
#ifdef DUMP_RGBA
    std::string CapDumpFileName = TData->CapDumpPath + std::to_string(TData->ThreadId) + ".rgba";
    FILE* fp = fopen(CapDumpFileName.c_str(), "wb");
#endif

    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
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
                    video_encode.Encode(data, CurrentData.AcquiredTime);
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

        std::chrono::time_point<std::chrono::high_resolution_clock> endtp = std::chrono::high_resolution_clock::now();
        uint64_t timecost = std::chrono::duration_cast<std::chrono::microseconds>(endtp - starttp).count();
        //printf("thread %d, dequeue frame %d, CurrentData texture ptr %p, AcquiredTime %u, encodeframe costtime %dus\n", TData->ThreadId, n, CurrentData.CapturedTexture, CurrentData.AcquiredTime, timecost);
        int EncodeInterval = TData->EncParams.fps != 0 ? 1000000 / TData->EncParams.fps : 0;
        if (timecost < EncodeInterval)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(EncodeInterval - timecost));
        }

        n++;
    }
#ifdef DUMP_RGBA
    fclose(fp);
#endif
    video_encode.End_video_output();

    if (CurrentData.CapturedTexture)
    {
        CurrentData.CapturedTexture->Release();
        CurrentData.CapturedTexture = nullptr;
    }

    return 0;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    g_exit = true;
    return TRUE;
}

int main()
{
    printf("MultiDisplay Screen Capture Sample!\n");

    // parse config
    std:: string config_file("MDSCSample.conf");
    JsonConfig json_config;
    if (!json_config.parse_file(config_file))
    {
        printf("Failed to parse config file: %s", config_file.c_str());
        return -1;
    }
    std::string stream_url = "rtsp://" + json_config.get_string("rtsp-server-ip") + ":" + json_config.get_string("rtsp-server-port") + "/screencap";
    uint32_t capture_fps = json_config.get_uint32("capture-fps");
    bool capture_single_display = json_config.get_boolean("capture-single-display");
    uint32_t capture_single_display_number = json_config.get_uint32("capture-single-display-number");
    std::string capture_dump_path = json_config.get_string("capture-dump-path");
    Encode_Params encode_params;
    std::string encode_type(json_config.get_string("encode-type"));
    if (!strcmp(encode_type.c_str(), "ffmpeg-software"))
        encode_params.encode_type = FFmpeg_SW;
    else
    {
        printf("Currently haven't support other encode type, use default ffmpeg-software");
        encode_params.encode_type = FFmpeg_SW;
    }
    encode_params.qp = json_config.get_uint32("encode-qp");
    encode_params.bitrate = json_config.get_uint32("encode-bitrate");
    encode_params.fps = json_config.get_uint32("encode-fps");
    encode_params.gop = json_config.get_uint32("encode-gop");
    encode_params.qmin = json_config.get_uint32("encode-qmin");
    encode_params.qmax = json_config.get_uint32("encode-qmax");
    encode_params.qcompress = json_config.get_float("encode-qcompress");
    bool stream_mode = json_config.get_boolean("stream-mode");
    std::string encode_dump_path = json_config.get_string("encode-dump-path");

    // init and start capture
    MultiDisplayScreenCapture MDSCsample;
    MDSCsample.Init();
    UINT DispNum = MDSCsample.GetDisplayCount();
    printf("Display Num is %d\n", DispNum);

    if (capture_single_display)
    {
        if (capture_single_display_number < DispNum)
        {
            printf("Capture single display Id %d as config selected\n", capture_single_display_number);
        }
        else
        {
            printf("Invalid single display Id %d in config, capture single disp 0 as default\n", capture_single_display_number);
            capture_single_display_number = 0;
        }
        MDSCsample.SetSingleDisplay(capture_single_display_number);
    }

    UINT OutNum = MDSCsample.GetOutputCount();
    printf("Output Num is %d\n", OutNum);

    MDSCsample.SetCaptureFps(capture_fps);

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

    printf("Start to create %d encoding threads \n", OutNum);
    encode_params.st_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (UINT i = 0; i < OutNum; ++i)
    {
        ThreadData[i].TerminateThreadsEvent = TerminateEncodeEvent;
        ThreadData[i].DxRes = MDSCsample.GetDXResource(i);
        ThreadData[i].PtrBufferQueue = &BufferQueues[i];
        ThreadData[i].ThreadId = capture_single_display ? capture_single_display_number : i;
        ThreadData[i].CapDumpPath = capture_dump_path;
        ThreadData[i].EncParams = encode_params;
        ThreadData[i].EncParams.thread_count = ThreadData[i].ThreadId;
        ThreadData[i].StreamMode = stream_mode;
        ThreadData[i].StreamUrl = stream_url;
        ThreadData[i].EncDumpPath = encode_dump_path;

        DWORD ThreadId;
        ThreadHandles[i] = CreateThread(nullptr, 0, EncodeProc, &ThreadData[i], 0, &ThreadId);
        if (ThreadHandles[i] == nullptr)
        {
            printf("Failed to create encoding thread %d\n", i);
            return SCREENCAP_FAILED;
        }
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
            DWORD res = WaitForSingleObjectEx(ThreadHandles[i], 1000/encode_params.fps, false);
            if (res != WAIT_TIMEOUT)
            {
                goto Exit;
            }
        }
    }

Exit:
    // Make sure all encode threads have exited
    if (SetEvent(TerminateEncodeEvent))
    {
        WaitForMultipleObjectsEx(OutNum, ThreadHandles, TRUE, INFINITE, FALSE);
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
