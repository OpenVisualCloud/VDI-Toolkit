#pragma once
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

#ifndef _QESENCODEMANAGER_H
#define _QESENCODEMANAGER_H

#include "EncodeManager.h"
#include "Common.h"
#include "qes_core.h"
#include "qes_encode.h"
#include <d3d11.h>

class QESEncodeManager : public EncodeManager
{
public:
    QESEncodeManager();
    virtual ~QESEncodeManager();
    virtual qesStatus Init(const Encode_Params& encode_params, DX_RESOURCES* DxRes);
    virtual qesStatus Encode(ID3D11Texture2D* texture, uint64_t timestamp);
    virtual DX_RESOURCES* GetDXResource();

private:
    qesStatus InitQES(qesEncodeParameters* qesencode_params, DX_RESOURCES* DxRes);
    qesStatus DestroyQES();
    qesStatus CreateQESEncodeParams(const Encode_Params& encode_params);
    qesStatus InitDXResources(DX_RESOURCES* DxRes);
    void ReleaseDxResources(DX_RESOURCES* DxRes);
    uint32_t StringToCodecID(const char* codec_id);
    uint32_t StringToTargetUsage(const char* target_usage);
    uint32_t StringToRCMode(const char* rc_mode);
    uint32_t StringToColorFormat(const char* color_format);
    uint32_t StringToCodecProfile(const char* codec_profile);

private: //QES Related
    qesCore* m_pQESCore;
    qesEncode* m_pQESEncoder;
    std::shared_ptr<qesEncodeParameters> m_pQESEncodeParams;
    DxResources* m_pDxRes;
    bool m_bDxResInitialized;
    uint32_t m_uFrameNum;
};

#endif