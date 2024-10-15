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
//! \file HostFFmpegEncodeService.h
//! \brief
//! \date 2024-07-04
//!

#ifdef _FFMPEG_SUPPORT_

#ifndef _HOSTFFMPEGENCODESERVICE_H_
#define _HOSTFFMPEGENCODESERVICE_H_

#include "../HostEncodeService.h"
#include "UtilFFmpeg.h"

VDI_NS_BEGIN

class HostFFmpegEncodeService : public HostEncodeService
{
public:
    //!
    //! \brief Construct a new Host FFmpeg Encode Service object
    //!
    HostFFmpegEncodeService();
    //!
    //! \brief Destroy the Host FFmpeg Encode Service object
    //!
    virtual ~HostFFmpegEncodeService();

    //!
    //! \brief Initialize encoder FFmpeg service
    //!
    //! \return MRDAStatus
    //!
    virtual MRDAStatus Initialize();

private:

    //!
    //! \brief initialize ffmpeg encoding context
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus InitCodec();

    //!
    //! \brief Set ffmpeg encoding parameters
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus SetEncParams();

    //!
    //! \brief Set hardware frame context
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus set_hwframe_ctx();

    //!
    //! \brief Get codec id for ffmpeg Encode
    //!
    //! \param [in] StreamCodecID
    //!             input codec id
    //! \return AVCodecID
    //!         avcodec encoder id
    //!
    AVCodecID GetCodecId(StreamCodecID codecID);

    //!
    //! \brief Get color format for ffmpeg Encode
    //!
    //! \param [in] ColorFormat
    //!             input color format
    //! \return AVPixelFormat
    //!         avcodec encoder color format
    //!
    AVPixelFormat GetColorFormat(ColorFormat colorFormat);

    //!
    //! \brief Get encoder name for ffmpeg Encode
    //!
    //! \param [in] StreamCodecID
    //!             input codec id
    //! \return std::string
    //!         avcodec encoder name
    //!
    std::string GetEncoderName(StreamCodecID codec_id);

    //!
    //! \brief Get encoder profile for ffmpeg Encode
    //!
    //! \param [in] codecProfile
    //!             input codec profile
    //! \return int
    //!         avcodec encoder profile
    //!
    int GetCodecProfile(CodecProfile codecProfile);

    //!
    //! \brief The encode core thread
    //!
    //! \return void*
    //!
    void* EncodeThread();

    //!
    //! \brief Get the Surface For Encode object
    //!
    //! \param [in] frame
    //! \param [out] pSurface
    //! \return MRDAStatus
    //!
    AVFrame* GetSurfaceForEncode(std::shared_ptr<FrameBufferData> frame);

    //!
    //! \brief Fill the input frame data to ffmpeg surface
    //!
    //! \param [in] frame
    //! \param [out] pSurface
    //! \return MRDAStatus
    //!
    MRDAStatus FillFrameToSurface(std::shared_ptr<FrameBufferData> frame, AVFrame* pSurface);

    //!
    //! \brief Color space convert
    //!
    //! \param [in] in_pix_fmt
    //! \param [in] out_pix_fmt
    //! \param [in] frame
    //! \param [in out] pSurface
    //! \return MRDAStatus
    //!
    MRDAStatus ColorSpaceConvert(AVPixelFormat in_pix_fmt, AVPixelFormat out_pix_fmt, std::shared_ptr<FrameBufferData> frame, AVFrame* pSurface);

    //!
    //! \brief Encode one frame
    //!
    //! \param [in] pSurface
    //! \return MRDAStatus
    //!
    MRDAStatus EncodeOneFrame(AVFrame* pSurface);

    //!
    //! \brief Write to output share memory buffer
    //!
    //! \param [in] pBS
    //! \return MRDAStatus
    //!
    MRDAStatus WriteToOutputShareMemoryBuffer(AVPacket* pBS);

private: //AV related
    AVCodecContext       *m_avctx;     //!< AV codec context
    AVBufferRef    *m_hwDeviceCtx;     //!< hardware device context
};

VDI_NS_END
#endif // _HOSTFFMPEGENCODESERVICE_H_

#endif // _FFMPEG_SUPPORT_