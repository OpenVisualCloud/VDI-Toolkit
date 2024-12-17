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

#ifndef _MRDAENCODEMANAGER_H
#define _MRDAENCODEMANAGER_H

#include "EncodeManager.h"
#include "Common.h"
#include <mutex>

struct MRDAEncode_Params
{
    // host session
    std::string hostSessionAddr;

    // share memory info
    uint64_t     totalMemorySize;
    uint32_t     bufferNum;
    uint64_t     bufferSize;
    std::string  in_mem_dev_path;
    std::string  out_mem_dev_path;
    uint32_t     in_mem_dev_slot_number;
    uint32_t     out_mem_dev_slot_number;

    // encode params
    uint32_t      frameNum;
    Encode_Params encode_params;
};

class MRDAEncodeManager : public EncodeManager
{
public:
    MRDAEncodeManager();
    virtual ~MRDAEncodeManager();
    virtual MRDAStatus Init(const MRDAEncode_Params &MRDAEncodeParams);
    virtual MRDAStatus Encode(uint8_t *data, uint64_t timestamp) override;
    virtual MRDAStatus End_video_output() override;

    uint32_t GetSendFrameCount();
    uint32_t GetReceiveFrameCount();

private:
    MRDAStatus InitMRDA(const MRDAEncode_Params &MRDAEncodeParams);
	MRDAStatus DestroyMRDA();
    void ReceiveFrameThread();
    static DWORD WINAPI ReceiveFrameThreadProc(LPVOID param) {
        MRDAEncodeManager* pThis = reinterpret_cast<MRDAEncodeManager*>(param);
        pThis->ReceiveFrameThread();
        return 0;
    }
    MRDAStatus FlushFrame();
    int CreateMediaParams(const MRDAEncode_Params &MRDAEncodeParams);
    StreamCodecID StringToCodecID(const char *codec_id);
    TargetUsage StringToTargetUsage(const char *target_usage);
    uint32_t StringToRCMode(const char *rc_mode);
    ColorFormat StringToColorFormat(const char *color_format);
    CodecProfile StringToCodecProfile(const char *codec_profile);

private: //MRDA Related
    MRDAHandle m_pMRDA_handle;
    std::shared_ptr<MediaParams> m_pMediaParams;
    HANDLE m_pTerminateThreadEvent;
    HANDLE m_pThreadHandle;
	DWORD m_dwThreadId;
    uint32_t m_nSendFrameCount;
	uint32_t m_nReceiveFrameCount;
    std::mutex m_mutex;
};

#endif