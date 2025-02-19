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

#include "QESEncodeManager.h"
#include <atlbase.h>

QESEncodeManager::QESEncodeManager() : m_pQESCore(nullptr),
                                       m_pQESEncoder(nullptr),
                                       m_pDxRes(nullptr),
                                       m_bDxResInitialized(false),
                                       m_uFrameNum(0)
{

}

QESEncodeManager::~QESEncodeManager()
{
    DestroyQES();
    if (m_bDxResInitialized)
    {
        if (m_pDxRes)
        {
            ReleaseDxResources(m_pDxRes);
        }
    }
    printf("[thread][%d], ~QESEncodeManager\n", m_uThreadId);
}

qesStatus QESEncodeManager::Init(const Encode_Params& encode_params, DX_RESOURCES* DxRes)
{
    if (EncodeManager::InitAVContext(encode_params) < 0)
    {
        printf("[thread][%d], InitVideoContext failed!\n", m_uThreadId);
        return QES_ERR_UNKNOWN;
    }

    m_pQESEncodeParams = std::make_shared<qesEncodeParameters>();
    if (QES_ERR_NONE != CreateQESEncodeParams(encode_params))
    {
        MRDA_LOG(LOG_ERROR, "[thread][%d], CreateQESEncodeParams failed!", m_uThreadId);
        return QES_ERR_INVALID_VIDEO_PARAM;
    }

    if (QES_ERR_NONE != InitQES(m_pQESEncodeParams.get(), DxRes))
    {
        printf("[thread][%d], InitQES failed!\n", m_uThreadId);
        return QES_ERR_UNKNOWN;
    }

    return QES_ERR_NONE;
}

qesStatus QESEncodeManager::Encode(ID3D11Texture2D* texture, uint64_t timestamp)
{
#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> st_enc_tp = std::chrono::high_resolution_clock::now();
#endif
    if (nullptr == texture)
    {
        printf("[thread][%d], texture is nullptr\n", m_uThreadId);
        return QES_ERR_UNKNOWN;
    }
    qesTextureHandle handle{};
    handle.ptr = texture;
    handle.handleType = QES_DX11_TEXTURE;
    qesFrame* pIFrame = NULL;

    qesStatus sts = m_pQESCore->RegisterTextureToEncoder(&handle, &pIFrame);
    if (QES_ERR_NONE != sts)
    {
        printf("[thread][%d], QESEncode RegisterTextureToEncoder failed\n", m_uThreadId);
        return sts;
    }

    sts = m_pQESEncoder->EncodeFrame(pIFrame);
    if (QES_ERR_NONE == sts)
    {
        do
        {
            qesBitstream* pbs;
            sts = m_pQESEncoder->GetBitstream(&pbs);
            if (QES_ERR_NONE == sts || QES_ERR_NONE_PARTIAL_OUTPUT == sts)
            {
                AVPacket* pPkt = EncodeManager::m_pPkt;
                if (!pPkt) {
                    MRDA_LOG(LOG_ERROR, "[thread][%d], QESEncod AVPacket is nullptr\n", m_uThreadId);
                    return QES_ERR_UNKNOWN;
                }
                pPkt->data = (uint8_t*)pbs->GetBitstreamData();
                pPkt->size = pbs->GetBitstreamDataLength();
                pPkt->pts = timestamp - EncodeManager::m_ulPts;
                pPkt->dts = pPkt->pts;
                int ret = av_interleaved_write_frame(EncodeManager::m_pVideoOfmtCtx, pPkt);
                if (ret < 0)
                {
                    printf("[thread][%d], QESEncod Error write frame\n", m_uThreadId);
                    break;
                }
                av_packet_unref(pPkt);
            }
            else if (sts != QES_ERR_MORE_DATA)
            {
                printf("GetBitsStream() returns error %d\n", sts);
                break;
            }
        } while (sts == QES_ERR_NONE_PARTIAL_OUTPUT);
    }
    else
    {
        printf("[thread][%d], QESEncode EncodeFrame failed\n", m_uThreadId);
    }

    m_pQESCore->UnregisterTexture(pIFrame);

#ifdef _ENABLE_TRACE_
    std::chrono::time_point<std::chrono::high_resolution_clock> ed_enc_tp = std::chrono::high_resolution_clock::now();
    uint64_t enc_timecost = std::chrono::duration_cast<std::chrono::microseconds>(ed_enc_tp - st_enc_tp).count();
    printf("[thread][%d], frame %u, QES Encode frame time cost %fms\n", m_uThreadId, m_uFrameNum, enc_timecost / 1000.0);
#endif

    m_uFrameNum++;
    return QES_ERR_NONE;
}

qesStatus QESEncodeManager::InitDXResources(DX_RESOURCES* DxRes)
{
    static D3D_DRIVER_TYPE D3DDriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_REFERENCE,
        D3D_DRIVER_TYPE_SOFTWARE,
        D3D_DRIVER_TYPE_WARP
    };

    static D3D_FEATURE_LEVEL D3DFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    UINT DriverTypesTotalNum = ARRAYSIZE(D3DDriverTypes);
    UINT FeatureLevelsTotalNum = ARRAYSIZE(D3DFeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;
    HRESULT hr = S_OK;
    for (int i = 0; i < ARRAYSIZE(D3DDriverTypes); i++)
    {
        hr = D3D11CreateDevice(nullptr, D3DDriverTypes[i], nullptr, 0, D3DFeatureLevels, FeatureLevelsTotalNum, D3D11_SDK_VERSION, &DxRes->Device, &FeatureLevel, &DxRes->Context);
        if (SUCCEEDED(hr))
        {
            break;
        }
    }

    if (FAILED(hr))
    {
        return QES_ERR_UNKNOWN;
    }
    return QES_ERR_NONE;
}

void QESEncodeManager::ReleaseDxResources(DX_RESOURCES* DxRes)
{
    if (DxRes->Device)
    {
        DxRes->Device->Release();
        DxRes->Device = nullptr;
    }

    if (DxRes->Context)
    {
        DxRes->Context->Release();
        DxRes->Context = nullptr;
    }
}

DX_RESOURCES* QESEncodeManager::GetDXResource()
{
    return m_pDxRes;
}

qesStatus QESEncodeManager::InitQES(qesEncodeParameters* qesencode_params, DX_RESOURCES* DxRes)
{
    qesStatus sts = QES_ERR_NONE;
    m_pQESCore = m_pQESCore->CreateInstance();
    m_pQESEncoder = m_pQESEncoder->CreateInstance();
    sts = m_pQESEncoder->SetCoreModule(m_pQESCore);
    if (QES_ERR_NONE != sts)
    {
        printf("[thread][%d], Failed to set QES Core Module\n", m_uThreadId);
        return sts;
    }

    if (nullptr == DxRes)
    {
        DxRes = new DX_RESOURCES();
        sts = InitDXResources(DxRes);
        if (QES_ERR_NONE != sts)
        {
            printf("[thread][%d], Failed to set QES Core Module\n", m_uThreadId);
            return sts;
        }
        m_pDxRes = DxRes;
        m_bDxResInitialized = true;
    }

    CComQIPtr<ID3D10Multithread> pMultiThread(DxRes->Context);

    if (pMultiThread)
    {
        pMultiThread->SetMultithreadProtected(true);
    }

    qesDeviceHandle pDevHandle{};
    pDevHandle.handleType = QES_HANDLE_D3D11_DEVICE;
    pDevHandle.ptr = DxRes->Device;
    sts = m_pQESCore->SetDevice(&pDevHandle);
    if (QES_ERR_NONE != sts)
    {
        printf("[thread][%d], Failed to init device from Core!\n", m_uThreadId);
        return sts;
    }

    sts = m_pQESEncoder->EncodeInit(qesencode_params);
    if (QES_ERR_NONE != sts)
    {
        printf("[thread][%d], Failed int QESEncoder with qesencode_params\n", m_uThreadId);
        return sts;
    }
    printf("[thread][%d], Init QES Successed\n", m_uThreadId);
    return QES_ERR_NONE;
}

qesStatus QESEncodeManager::DestroyQES()
{
    m_pQESEncoder->DestroyInstance(m_pQESEncoder);
    m_pQESCore->DestroyInstance(m_pQESCore);
    return QES_ERR_NONE;
}


qesStatus QESEncodeManager::CreateQESEncodeParams(const Encode_Params& encode_params)
{
    m_pQESEncodeParams->Width = encode_params.width;
    m_pQESEncodeParams->Height = encode_params.height;
    m_pQESEncodeParams->InputFourCC = StringToColorFormat(encode_params.input_color_format.c_str());
    m_pQESEncodeParams->CodecID = StringToCodecID(encode_params.codec_id.c_str());
    m_pQESEncodeParams->CodecProfile = StringToCodecProfile(encode_params.codec_profile.c_str());
    m_pQESEncodeParams->BitrateKbps = encode_params.bitrate;
    m_pQESEncodeParams->MaxBitrateKbps = encode_params.bitrate;
    m_pQESEncodeParams->GopPicSize = encode_params.gop;
    m_pQESEncodeParams->NumOfRef = 1;
    m_pQESEncodeParams->Framerate = encode_params.framerate_num;
    m_pQESEncodeParams->TargetUsage = StringToTargetUsage(encode_params.target_usage.c_str());
    m_pQESEncodeParams->RateControlMethod = StringToRCMode(encode_params.rc_mode.c_str());
    if (QES_RATECONTROL_VBR == m_pQESEncodeParams->RateControlMethod)
    {
        m_pQESEncodeParams->IFrameQP.MinQPI = encode_params.qp;
        m_pQESEncodeParams->PFrameQP.MinQPP = encode_params.qp;
        m_pQESEncodeParams->BFrameQP.MinQPB = encode_params.qp;
        m_pQESEncodeParams->IFrameQP.MaxQPI = encode_params.qp;
        m_pQESEncodeParams->PFrameQP.MaxQPP = encode_params.qp;
        m_pQESEncodeParams->BFrameQP.MaxQPB = encode_params.qp;
    }
    else if (QES_RATECONTROL_CQP == m_pQESEncodeParams->RateControlMethod)
    {
        m_pQESEncodeParams->IFrameQP.QPI = encode_params.qp;
        m_pQESEncodeParams->PFrameQP.QPP = encode_params.qp;
        m_pQESEncodeParams->BFrameQP.QPB = encode_params.qp;
    }
    return QES_ERR_NONE;
}

uint32_t QESEncodeManager::StringToCodecID(const char* codec_id)
{
    uint32_t streamCodecId = QES_CODEC_AVC;
    if (!strcmp(codec_id, "h264") || !strcmp(codec_id, "avc"))
    {
        streamCodecId = QES_CODEC_AVC;
    }
    else if (!strcmp(codec_id, "h265") || !strcmp(codec_id, "hevc"))
    {
        streamCodecId = QES_CODEC_HEVC;
    }
    else if (!strcmp(codec_id, "av1"))
    {
        streamCodecId = QES_CODEC_AV1;
    }
    else
    {
        printf("[thread][%d], Unsupported codec id: %s\n", m_uThreadId, codec_id);
    }
    return streamCodecId;
}

uint32_t QESEncodeManager::StringToTargetUsage(const char* target_usage)
{
    uint32_t targetUsage = 0; //MFX_TARGETUSAGE_UNKNOWN
    if (!strcmp(target_usage, "balanced"))
    {
        targetUsage = 4; //MFX_TARGETUSAGE_BALANCED;
    }
    else if (!strcmp(target_usage, "quality"))
    {
        targetUsage = 1; //MFX_TARGETUSAGE_BEST_QUALITY
    }
    else if (!strcmp(target_usage, "speed"))
    {
        targetUsage = 7; //MFX_TARGETUSAGE_BEST_SPEED
    }
    else
    {
        printf("[thread][%d], Unsupported QES TargetUsage: %s\n", m_uThreadId, target_usage);
    }
    return targetUsage;
}

uint32_t QESEncodeManager::StringToRCMode(const char* rc_mode)
{
    uint32_t RCMode = QES_RATECONTROL_VBR;
    if (!strcmp(rc_mode, "CQP"))
    {
        RCMode = QES_RATECONTROL_CQP;
    }
    else if (!strcmp(rc_mode, "VBR"))
    {
        RCMode = QES_RATECONTROL_VBR;
    }
    else
    {
        printf("[thread][%d], Unsupported QES RateControl Mode: %s\n", m_uThreadId, rc_mode);
    }
    return RCMode;
}

uint32_t QESEncodeManager::StringToColorFormat(const char* color_format)
{
    uint32_t colorFormat = QES_FOURCC_RGB4;
    if (!strcmp(color_format, "rgb32"))
    {
        colorFormat = QES_FOURCC_RGB4;
    }
    else if (!strcmp(color_format, "nv12"))
    {
        colorFormat = QES_FOURCC_NV12;
    }
    return colorFormat;
}

uint32_t QESEncodeManager::StringToCodecProfile(const char* codec_profile)
{
    uint32_t codecProfile = QES_PROFILE_UNKNOWN;
    if (!strcmp(codec_profile, "avc:main"))
    {
        codecProfile = QES_PROFILE_AVC_MAIN;
    }
    else if (!strcmp(codec_profile, "avc:high"))
    {
        codecProfile = QES_PROFILE_AVC_HIGH;
    }
    else if (!strcmp(codec_profile, "hevc:main"))
    {
        codecProfile = QES_PROFILE_HEVC_MAIN;
    }
    else if (!strcmp(codec_profile, "hevc:main10"))
    {
        codecProfile = QES_PROFILE_HEVC_MAIN10;
    }
    else if (!strcmp(codec_profile, "av1:main"))
    {
        codecProfile = QES_PROFILE_AV1_MAIN;
    }
    else if (!strcmp(codec_profile, "av1:high"))
    {
        codecProfile = QES_PROFILE_AV1_HIGH;
    }
    return codecProfile;
}
