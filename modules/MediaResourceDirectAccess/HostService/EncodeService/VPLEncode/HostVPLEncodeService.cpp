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

 */

//!
//! \file HostVPLEncodeService.cpp
//! \brief implement host vpl encode service
//! \date 2024-04-11
//!

#ifdef _VPL_SUPPORT_

#include "HostVPLEncodeService.h"
#include <fstream>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
#include <iostream>

#include <cstring>

VDI_NS_BEGIN

HostVPLEncodeService::HostVPLEncodeService(TaskInfo taskInfo)
{
    debug_file = fopen("out_host.hevc", "wb");
    m_taskInfo = taskInfo;
}

HostVPLEncodeService::~HostVPLEncodeService()
{
    if (m_session)
    {
        MFXVideoENCODE_Close(m_session);
        MFXClose(m_session);
    }
    if (m_loader)
    {
        MFXUnload(m_loader);
    }

    m_encodeThread.join();

    fclose(debug_file);
}

MRDAStatus HostVPLEncodeService::Initialize()
{
    // init mfx encode
    if (MRDA_STATUS_SUCCESS != InitMFX())
    {
        MRDA_LOG(LOG_ERROR, "Failed to init mfx context!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (MRDA_STATUS_SUCCESS != SetMFXEncParams())
    {
        MRDA_LOG(LOG_ERROR, "Failed to set mfx encoder params!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    if (MRDA_STATUS_SUCCESS != InitMFXEncoder())
    {
        MRDA_LOG(LOG_ERROR, "Failed to init mfx encoder!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // init share memory
    if (MRDA_STATUS_SUCCESS != InitShm())
    {
        MRDA_LOG(LOG_ERROR, "Failed to init share memory!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    // start encode thread
    m_encodeThread = std::thread(&HostVPLEncodeService::EncodeThread, this);
    return MRDA_STATUS_SUCCESS;
}

mfxFrameSurface1* HostVPLEncodeService::GetSurfaceForEncode(std::shared_ptr<FrameBufferData> frame)
{
    if (frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input data is null!");
        return nullptr;
    }
    mfxFrameSurface1 *pSurface = nullptr;
    if (MFX_ERR_NONE != MFXMemory_GetSurfaceForEncode(m_session, &pSurface))
    {
        MRDA_LOG(LOG_ERROR, "Get surface for encode failed!");
        return nullptr;
    }
    if (MRDA_STATUS_SUCCESS != FillFrameToSurface(frame, pSurface))
    {
        MRDA_LOG(LOG_ERROR, "Fill frame to surface failed!");
        return nullptr;
    }

    return pSurface;
}

MRDAStatus HostVPLEncodeService::FillFrameToSurface(std::shared_ptr<FrameBufferData> frame, mfxFrameSurface1* pSurface)
{
    if (frame == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Input data is null!");
        return MRDA_STATUS_INVALID_DATA;
    }
    // Map makes surface writable by CPU for all implementations
    mfxFrameSurface1* surface = pSurface;
    if (surface == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Surface is null!");
        return MRDA_STATUS_INVALID_DATA;
    }
    mfxStatus sts = surface->FrameInterface->Map(surface, MFX_MAP_WRITE);
    if (sts != MFX_ERR_NONE) {
        printf("mfxFrameSurfaceInterface->Map failed (%d)\n", sts);
        return sts;
    }

    // Fill surface with data from frame
    mfxFrameInfo *info = &surface->Info;
    mfxFrameData *data = &surface->Data;

    mfxU16 w = info->CropW;
    mfxU16 h = info->CropH;
    mfxU16 pitch = 0, i = 0;
    size_t bytes_read = 0;
    mfxU8 *ptr = nullptr;
    // offset = memory offset + state offset (sizeof(uint32))
    if (frame->MemBuffer() == nullptr) return MRDA_STATUS_INVALID_DATA;
    size_t base_offset = frame->MemBuffer()->MemOffset();

    switch (info->FourCC) {
        case MFX_FOURCC_I420: {
            // read luminance plane (Y)
            pitch = data->Pitch;
            ptr   = data->Y;
            char* in_shm_offset_y = m_inShmMem + base_offset;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset_y + i * w, w);
            }

            // read chrominance (U, V)
            pitch /= 2;
            h /= 2;
            w /= 2;
            ptr = data->U;
            char* in_shm_offset_u = m_inShmMem + base_offset + w * h;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset_u + i * w, w);
            }

            ptr = data->V;
            char* in_shm_offset_v = m_inShmMem + base_offset + w * h * 5 / 4;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset_v + i * w, w);
            }
            break;
        }
        case MFX_FOURCC_NV12: {
            // Y
            pitch = data->Pitch;
            ptr   = data->Y;
            char* in_shm_offset_y = m_inShmMem + base_offset;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset_y + i * w, w);
            }
            // UV
            ptr = data->UV;
            char* in_shm_offset_uv = m_inShmMem + base_offset + w * h;
            h /= 2;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset_uv + i * w, w);
            }
            break;
        }
        case MFX_FOURCC_RGB4: {
            // Y
            ptr   = data->B;
            char* in_shm_offset = m_inShmMem + base_offset;
            pitch = data->Pitch;
            for (i = 0; i < h; i++) {
                memcpy(ptr + i * pitch, in_shm_offset + i * pitch, pitch);
            }
            break;
        }
        default:
            MRDA_LOG(LOG_ERROR, "Unsupported FourCC code, skip LoadRawFrame\n");
            break;
    }

    // Unmap/release returns local device access for all implementations
    sts = surface->FrameInterface->Unmap(surface);
    if (sts != MFX_ERR_NONE) {
        MRDA_LOG(LOG_ERROR, "mfxFrameSurfaceInterface->Unmap failed (%d)\n", sts);
        return sts;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostVPLEncodeService::WriteToOutputShareMemoryBuffer(mfxBitstream* pBS)
{
    if (pBS == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "pBS is null\n");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (m_outShmMem == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "m_outShmMem is null\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    // Get one available buffer frame from output memory pool
    std::shared_ptr<FrameBufferData> data = nullptr;
    if (MRDA_STATUS_SUCCESS != GetAvailableOutputBufferFrame(data))
    {
        MRDA_LOG(LOG_ERROR, "GetAvailableOutputBufferFrame failed\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    if (data == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "data is null\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    // write to out share memory
    RefOutputFrame(data);
    if (data->MemBuffer() == nullptr) return MRDA_STATUS_INVALID_DATA;
    size_t mem_offset = data->MemBuffer()->MemOffset();
    memcpy(m_outShmMem + mem_offset, pBS->Data + pBS->DataOffset, pBS->DataLength);
    data->MemBuffer()->SetOccupiedSize(pBS->DataLength);

    fwrite(pBS->Data + pBS->DataOffset, 1, pBS->DataLength, debug_file);
    // update output buffer list
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outFrameBufferDataList.push_back(data);
    // MRDA_LOG(LOG_INFO, "Push back output buffer at pts %llu", data->Pts());

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostVPLEncodeService::EncodeOneFrame(mfxFrameSurface1* pSurface)
{
    if (pSurface == nullptr && m_isEOS == false)
    {
        MRDA_LOG(LOG_ERROR, "pSurface is null\n");
        return MRDA_STATUS_INVALID_DATA;
    }
    // Encode frame async VPL
    mfxBitstream pBS = {};
    pBS.MaxLength = BITSTREAM_BUFFER_SIZE;
    pBS.Data      = (mfxU8 *)calloc(pBS.MaxLength, sizeof(mfxU8));
    mfxSyncPoint syncp = {};
    mfxStatus sts = MFXVideoENCODE_EncodeFrameAsync(m_session,
                                                    nullptr,
                                                    m_isEOS ? nullptr : pSurface,
                                                    &pBS,
                                                    &syncp);
    // release pSurface
    if (!m_isEOS)
    {
        if (MFX_ERR_NONE != pSurface->FrameInterface->Release(pSurface))
        {
            MRDA_LOG(LOG_ERROR, "Release frame failed\n");
            return MRDA_STATUS_OPERATION_FAIL;
        }
    }
    switch (sts) {
        case MFX_ERR_NONE:
            // MFX_ERR_NONE and syncp indicate output is available
            if (syncp) {
                // Encode output is not available on CPU until sync operation
                // completes
                do {
                    // MRDA_LOG(LOG_INFO, "Sync operation!");
                    sts = MFXVideoCORE_SyncOperation(m_session, syncp, WAIT_100_MILLISECONDS);
                    if (MFX_ERR_NONE == sts) {
                        WriteToOutputShareMemoryBuffer(&pBS);
                        if (pBS.Data)
                            free(pBS.Data);
                        m_frameNum++;
                    }
                } while (sts == MFX_WRN_IN_EXECUTION);
            }
            break;
        case MFX_ERR_NOT_ENOUGH_BUFFER:
            // This example deliberatly uses a large output buffer with immediate
            // write to disk for simplicity. Handle when frame size exceeds
            // available buffer here
            MRDA_LOG(LOG_ERROR, "Encode async return MFX_ERR_NOT_ENOUGH_BUFFER!!");
            break;
        case MFX_ERR_MORE_DATA:
            // The function requires more data to generate any output
            if (m_isEOS == true)
            {
                m_isStop = true;
                MRDA_LOG(LOG_INFO, "Stop encode thread!!!");
                break;
            }
        case MFX_ERR_DEVICE_LOST:
            // For non-CPU implementations,
            // Cleanup if device is lost
            break;
        case MFX_WRN_DEVICE_BUSY:
            // For non-CPU implementations,
            // Wait a few milliseconds then try again
            break;
        default:
            MRDA_LOG(LOG_ERROR, "unknown status %d\n", sts);
            m_isStop = true;
            break;
    }

    return MRDA_STATUS_SUCCESS;
}

void* HostVPLEncodeService::EncodeThread()
{
    while (!m_isStop)
    {
        // get one frame
        std::shared_ptr<FrameBufferData> frame = nullptr;
        mfxFrameSurface1 *pSurface = nullptr;
        if (m_isEOS == false)
        {
            {
                if (m_inFrameBufferDataList.empty())
                {
                    usleep(5 * 1000);//5ms
                    continue;
                }
                std::unique_lock<std::mutex> lock(m_inMutex);
                frame = m_inFrameBufferDataList.front();
                m_inFrameBufferDataList.pop_front();
                if (frame->IsEOS())
                {
                    MRDA_LOG(LOG_INFO, "Get EOS frame!!!!!!!!\n");
                    m_isEOS = true;
                    continue;
                }
            }
            // get surface for encode
            pSurface = GetSurfaceForEncode(frame);
        }
        // encode one frame
        if (MRDA_STATUS_SUCCESS != EncodeOneFrame(pSurface))
        {
            MRDA_LOG(LOG_ERROR, "EncodeOneFrame failed!");
            m_isStop = true;
            return nullptr;
        }
        else
        {
            // unref frame
            UnRefInputFrame(frame);
        }
    }
    return nullptr;
}

MRDAStatus HostVPLEncodeService::InitMFX()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxConfig cfg[3];
    mfxVariant cfgVal[3];

    // Initialize VPL session
    m_loader = MFXLoad();
    if (NULL == m_loader)
    {
        MRDA_LOG(LOG_ERROR, "MFXLoad failed -- is implementation in path?");
        return MRDA_STATUS_INVALID_DATA;
    }

    // Implementation used must be the type requested from command line
    cfg[0] = MFXCreateConfig(m_loader);
    if (NULL == cfg[0])
    {
        MRDA_LOG(LOG_ERROR, "MFXCreateConfig failed");
        return MRDA_STATUS_INVALID_DATA;
    }
    cfgVal[0].Type     = MFX_VARIANT_TYPE_U32;
    cfgVal[0].Data.U32 = MFX_IMPL_TYPE_HARDWARE;
    sts = MFXSetConfigFilterProperty(cfg[0],
        (mfxU8 *)"mfxImplDescription.Impl",
        cfgVal[0]);
    if (MFX_ERR_NONE != sts)
    {
        MRDA_LOG(LOG_ERROR, "MFXSetConfigFilterProperty failed for Impl");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "m_mediaParams is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }
    // Implementation must provide an encoder type
    cfg[1] = MFXCreateConfig(m_loader);
    if (NULL == cfg[1])
    {
        MRDA_LOG(LOG_ERROR, "MFXCreateConfig failed");
        return MRDA_STATUS_INVALID_DATA;
    }
    cfgVal[1].Type     = MFX_VARIANT_TYPE_U32;
    cfgVal[1].Data.U32 = GetCodecId(m_mediaParams->encodeParams.codec_id);
    sts = MFXSetConfigFilterProperty(cfg[1],
        (mfxU8 *)"mfxImplDescription.mfxEncoderDescription.encoder.CodecID",
        cfgVal[1]);
    if (MFX_ERR_NONE != sts)
    {
        MRDA_LOG(LOG_ERROR, "MFXSetConfigFilterProperty failed for encoder CodecID");
        return MRDA_STATUS_INVALID_DATA;
    }

    // Implementation used must provide API version 2.2 or newer
    cfg[2] = MFXCreateConfig(m_loader);
    if (NULL == cfg[2])
    {
        MRDA_LOG(LOG_ERROR, "MFXCreateConfig failed");
        return MRDA_STATUS_INVALID_DATA;
    }
    cfgVal[2].Type     = MFX_VARIANT_TYPE_U32;
    cfgVal[2].Data.U32 = VPLVERSION(MAJOR_API_VERSION_REQUIRED, MINOR_API_VERSION_REQUIRED);
    sts = MFXSetConfigFilterProperty(cfg[2],
        (mfxU8 *)"mfxImplDescription.ApiVersion.Version",
        cfgVal[2]);
    if (MFX_ERR_NONE != sts)
    {
        MRDA_LOG(LOG_ERROR, "MFXSetConfigFilterProperty failed for API version");
        return MRDA_STATUS_INVALID_DATA;
    }
    // Create session
    sts = MFXCreateSession(m_loader, 0, &m_session);
    if (MFX_ERR_NONE != sts)
    {
        MRDA_LOG(LOG_ERROR, "Cannot create session -- no implementations meet selection criteria");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostVPLEncodeService::InitMFXEncoder()
{
    // Initialize encoder
    mfxStatus st = MFXVideoENCODE_Init(m_session, &m_mfxVideoParams);
    if (MFX_ERR_NONE != st)
    {
        MRDA_LOG(LOG_ERROR, "Encode init failed");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus HostVPLEncodeService::SetMFXEncParams()
{
    if (m_mediaParams == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "m_mediaParams is nullptr");
        return MRDA_STATUS_INVALID_DATA;
    }
    EncodeParams encodeParams = m_mediaParams->encodeParams;
    memset(&m_mfxVideoParams, 0, sizeof(m_mfxVideoParams));

    // codec id
    m_mfxVideoParams.mfx.CodecId = GetCodecId(encodeParams.codec_id);

    // io pattern
    m_mfxVideoParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    // gop size
    m_mfxVideoParams.mfx.GopPicSize = encodeParams.gop_size;
    m_mfxVideoParams.AsyncDepth = encodeParams.async_depth;

    // target usage
    m_mfxVideoParams.mfx.TargetUsage = static_cast<uint16_t>(encodeParams.target_usage);

    //rate control mode, qp, bitrate settings
    if(encodeParams.rc_mode== 0)
    {
        m_mfxVideoParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_mfxVideoParams.mfx.QPI = encodeParams.qp;
        m_mfxVideoParams.mfx.QPP = encodeParams.qp;
        m_mfxVideoParams.mfx.QPB = encodeParams.qp;
    }
    else if(encodeParams.rc_mode == 1)
    {
        m_mfxVideoParams.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        m_mfxVideoParams.mfx.TargetKbps = encodeParams.bit_rate;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Invalid rc_mode");
        return MRDA_STATUS_INVALID_DATA;
    }

    // frame info: width, height, fps
    m_mfxVideoParams.mfx.FrameInfo.FrameRateExtN = encodeParams.framerate_num;
    m_mfxVideoParams.mfx.FrameInfo.FrameRateExtD = encodeParams.framerate_den;
    m_mfxVideoParams.mfx.FrameInfo.Width  = ALIGN16(encodeParams.frame_width);
    m_mfxVideoParams.mfx.FrameInfo.Height = ALIGN16(encodeParams.frame_height);
    m_mfxVideoParams.mfx.FrameInfo.CropX = 0;
    m_mfxVideoParams.mfx.FrameInfo.CropY = 0;
    m_mfxVideoParams.mfx.FrameInfo.CropW = encodeParams.frame_width;
    m_mfxVideoParams.mfx.FrameInfo.CropH = encodeParams.frame_height;

    // FourCC & Chroma format
    if(encodeParams.color_format == ColorFormat::COLOR_FORMAT_NV12)
    {
        m_mfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        m_mfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }
    else if (encodeParams.color_format == ColorFormat::COLOR_FORMAT_RGBA32)
    {
        m_mfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
        m_mfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
    }
    else if (encodeParams.color_format == ColorFormat::COLOR_FORMAT_YUV420P)
    {
        m_mfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_I420;
        m_mfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Invalid color format");
        return MRDA_STATUS_INVALID_DATA;
    }

    // codec profile
    m_mfxVideoParams.mfx.CodecProfile = GetCodecProfile(encodeParams.codec_profile);

    if (encodeParams.max_b_frames == 0)
    {
        m_mfxVideoParams.mfx.GopRefDist = 1;
        m_mfxVideoParams.mfx.NumRefFrame = 1;
    }
    else if (encodeParams.max_b_frames == 1)
    {
        m_mfxVideoParams.mfx.GopRefDist = 2;
        m_mfxVideoParams.mfx.NumRefFrame = 2;
    }
    else if (encodeParams.max_b_frames == 2)
    {
        m_mfxVideoParams.mfx.GopRefDist = 3;
        m_mfxVideoParams.mfx.NumRefFrame = 2;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Invalid max_b_frames");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

uint32_t HostVPLEncodeService::GetCodecId(StreamCodecID codecID)
{
    if (codecID == StreamCodecID::CodecID_AV1)
    {
        return MFX_CODEC_AV1;
    }
    else if (codecID == StreamCodecID::CodecID_AVC)
    {
        return MFX_CODEC_AVC;
    }
    else if (codecID == StreamCodecID::CodecID_HEVC)
    {
        return MFX_CODEC_HEVC;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown codec id!");
        return 0;
    }
}

uint16_t HostVPLEncodeService::GetCodecProfile(CodecProfile codecProfile)
{
    if (codecProfile == CodecProfile::PROFILE_AVC_MAIN)
    {
        return MFX_PROFILE_AVC_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_AVC_HIGH)
    {
        return MFX_PROFILE_AVC_HIGH;
    }
    else if (codecProfile == CodecProfile::PROFILE_HEVC_MAIN)
    {
        return MFX_PROFILE_HEVC_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_HEVC_MAIN10)
    {
        return MFX_PROFILE_HEVC_MAIN10;
    }
    else if (codecProfile == CodecProfile::PROFILE_AV1_MAIN)
    {
        return MFX_PROFILE_AV1_MAIN;
    }
    else if (codecProfile == CodecProfile::PROFILE_AV1_HIGH)
    {
        return MFX_PROFILE_AV1_HIGH;
    }
    else
    {
        MRDA_LOG(LOG_ERROR, "Unknown codec profile!");
        return 0;
    }
}

VDI_NS_END

#endif // _VPL_SUPPORT_