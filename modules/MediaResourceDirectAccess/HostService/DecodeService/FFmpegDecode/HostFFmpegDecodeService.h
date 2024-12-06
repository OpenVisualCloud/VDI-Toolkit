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
//! \file HostFFmpegDecodeService.h
//! \brief
//! \date 2024-11-11
//!

#ifdef _FFMPEG_SUPPORT_

#ifndef _HOSTFFMPEGDECODESERVICE_H_
#define _HOSTFFMPEGDECODESERVICE_H_

#include "../HostDecodeService.h"
#include "UtilFFmpeg.h"

VDI_NS_BEGIN

class HostFFmpegDecodeService : public HostDecodeService
{
public:
    //!
    //! \brief Construct a new Host FFmpeg Decode Service object
    //!
    HostFFmpegDecodeService(TaskInfo taskInfo);
    //!
    //! \brief Destroy the Host FFmpeg Decode Service object
    //!
    virtual ~HostFFmpegDecodeService();

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
    MRDAStatus SetDecParams();

    //!
    //! \brief Get codec id for ffmpeg Decode
    //!
    //! \param [in] StreamCodecID
    //!             input codec id
    //! \return AVCodecID
    //!         avcodec encoder id
    //!
    AVCodecID GetCodecId(StreamCodecID codecID);

    //!
    //! \brief Get color format for ffmpeg Decode
    //!
    //! \param [in] ColorFormat
    //!             input color format
    //! \return AVPixelFormat
    //!         avcodec encoder color format
    //!
    AVPixelFormat GetColorFormat(ColorFormat colorFormat);

    //!
    //! \brief The decode core thread
    //!
    //! \return void*
    //!
    void* DecodeThread();

    //!
    //! \brief FrameBufferData packet -> AVPacket
    //!
    //! \param [in] packet
    //! \param [out] AVPacket
    //! \return MRDAStatus
    //!
    AVPacket* GetPacketForDecode(std::shared_ptr<FrameBufferData> packet);

    //!
    //! \brief Color space convert from AVFrame -> AVFrame
    //!
    //! \param [in] in_pix_fmt
    //! \param [in] out_pix_fmt
    //! \param [in] in_frame
    //! \param [in out] out_frame
    //! \return MRDAStatus
    //!
    MRDAStatus ColorSpaceConvert(AVPixelFormat in_pix_fmt, AVPixelFormat out_pix_fmt, AVFrame *in_frame, AVFrame* out_frame);

    //!
    //! \brief Decode one frame
    //!
    //! \param [in] AVPacket
    //! \return MRDAStatus
    //!
    MRDAStatus DecodeOneFrame(AVPacket* packet);

    //!
    //! \brief Write to output share memory buffer
    //!
    //! \param [in] AVFrame
    //! \return MRDAStatus
    //!
    MRDAStatus WriteToOutputShareMemoryBuffer(AVFrame* frame);

    //!
    //! \brief Copy frame to memory
    //!
    //! \param [in] AVFrame
    //! \param [in] FrameBufferData
    //! \return MRDAStatus
    //!
    MRDAStatus CopyFrameToMemory(AVFrame* frame, std::shared_ptr<FrameBufferData> data);

private: //AV related
    AVCodecContext       *m_avctx;     //!< AV codec context
    AVBufferRef    *m_hwDeviceCtx;     //!< hardware device context
    TaskInfo           m_taskInfo;     //!< task info
};

VDI_NS_END
#endif // _HOSTFFMPEGDECODESERVICE_H_

#endif // _FFMPEG_SUPPORT_