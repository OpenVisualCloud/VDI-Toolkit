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
//! \file HostVPLEncodeService.h
//! \brief
//! \date 2024-04-11
//!

#ifdef _VPL_SUPPORT_

#ifndef _HOSTVPLENCODESERVICE_H_
#define _HOSTVPLENCODESERVICE_H_

#include "../HostEncodeService.h"
#include "UtilVPL.h"

#include "vpl/mfx.h"
#include "vpl/mfxstructures.h"
#include "vpl/mfxvideo.h"
#include "vpl/mfxvideo++.h"
#include "vpl/mfxdefs.h"


VDI_NS_BEGIN

class HostVPLEncodeService : public HostEncodeService
{
public:
    //!
    //! \brief Construct a new Host VPL Encode Service object
    //!
    HostVPLEncodeService(TaskInfo taskInfo);
    //!
    //! \brief Destroy the Host VPL Encode Service object
    //!
    virtual ~HostVPLEncodeService();

    //!
    //! \brief Initialize encoder VPL service
    //!
    //! \return MRDAStatus
    //!
    virtual MRDAStatus Initialize();

private:
    //!
    //! \brief The encode core thread
    //!
    //! \return void*
    //!
    void* EncodeThread();

    //!
    //! \brief initialize mfx encoding context
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus InitMFX();

    //!
    //! \brief initialize mfx encoder
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus InitMFXEncoder();

    //!
    //! \brief Set encode parameters for MFX Encode
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!
    MRDAStatus SetMFXEncParams();

    //!
    //! \brief Get the Codec Id object
    //!
    //! \param [in] codecID
    //! \return int32_t
    //!
    uint32_t GetCodecId(StreamCodecID codecID);

    //!
    //! \brief Get the Codec Profile object
    //!
    //! \param [in] codecProfile
    //! \return uint16_t
    //!
    uint16_t GetCodecProfile(CodecProfile codecProfile);

    //!
    //! \brief Get the Surface For Encode object
    //!
    //! \param [in] frame
    //! \param [out] pSurface
    //! \return MRDAStatus
    //!
    mfxFrameSurface1* GetSurfaceForEncode(std::shared_ptr<FrameBufferData> frame);

    //!
    //! \brief Fill the input frame data to mfx surface
    //!
    //! \param [in] frame
    //! \param [out] pSurface
    //! \return MRDAStatus
    //!
    MRDAStatus FillFrameToSurface(std::shared_ptr<FrameBufferData> frame, mfxFrameSurface1* pSurface);

    //!
    //! \brief Encode one frame
    //!
    //! \param [in] pSurface
    //! \return MRDAStatus
    //!
    MRDAStatus EncodeOneFrame(mfxFrameSurface1* pSurface);

    //!
    //! \brief Write to output share memory buffer
    //!
    //! \param [in] pBS
    //! \return MRDAStatus
    //!
    MRDAStatus WriteToOutputShareMemoryBuffer(mfxBitstream* pBS);

private: //MFX related
    mfxLoader m_loader; //<! MFX loader
    mfxSession m_session; //<! MFX video session
    mfxVideoParam m_mfxVideoParams; //<! MFX encode parameters
    mfxBitstream m_bitstream; //<! MFX bitstream
    TaskInfo     m_taskInfo; //<! Task information
};

VDI_NS_END
#endif // _HOSTVPLENCODESERVICE_H_

#endif // _VPL_SUPPORT_