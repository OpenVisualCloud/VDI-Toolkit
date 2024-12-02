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
//! \file HostDecodeService.h
//! \brief
//! \date 2024-11-11
//!

#ifndef _HOSTDECODESERVICE_H_
#define _HOSTDECODESERVICE_H_

#include "../HostService.h"
#include <thread>
#include <mutex>
#include <list>
#include <map>
#include <unistd.h>

VDI_NS_BEGIN

class HostDecodeService : public HostService
{
public:
    //!
    //! \brief Construct a new Host Decode Service object
    //!
    HostDecodeService();
    //!
    //! \brief Destroy the Host Decode Service object
    //!
    virtual ~HostDecodeService();
    //!
    //! \brief Initialize decode service
    //!
    //! \return MRDAStatus
    //!
    virtual MRDAStatus Initialize() = 0;
    //!
    //! \brief Set the Init Params object
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!
    virtual MRDAStatus SetInitParams(MediaParams *params) override;
    //!
    //! \brief Send input data
    //!
    //! \param [in] data
    //! \return MRDAStatus
    //!
    virtual MRDAStatus SendInputData(std::shared_ptr<FrameBufferData> data) override;
    //!
    //! \brief Receive output data
    //!
    //! \param [out] data
    //! \return MRDAStatus
    //!
    virtual MRDAStatus ReceiveOutputData(std::shared_ptr<FrameBufferData> &data) override;

protected:
    //!
    //! \brief Initialize share memory
    //!
    //! \return MRDAStatus
    //!
    MRDAStatus InitShm();
    //!
    //! \brief Get the Avail Buffer object
    //!
    //! \param [out]
    //! \return MRDAStatus
    //!
    MRDAStatus GetAvailBuffer(std::shared_ptr<FrameBufferData>& pFrame);
    // FIXME: May put output memory pool in host
    //!
    //! \brief Get the Available Output Buffer Frame object
    //!
    //! \param [in] pFrame
    //! \return MRDAStatus
    //!
    MRDAStatus GetAvailableOutputBufferFrame(std::shared_ptr<FrameBufferData>& pFrame);

protected:
    // decode thread related
    bool m_isStop; //<! stop flag
    bool m_isEOS; //<! EOS flag
    uint32_t m_frameNum; //<! frame number
    std::mutex m_inMutex; //<! input list mutex
    std::mutex m_outMutex; //<! output list mutex
    std::list<std::shared_ptr<FrameBufferData>> m_inFrameBufferDataList; //<! frame buffer data list
    std::list<std::shared_ptr<FrameBufferData>> m_outFrameBufferDataList; //<! frame buffer data list
    std::thread m_decodeThread; //<! decode thread
    FILE *debug_file = nullptr;
};

VDI_NS_END
#endif // _HOSTDECODESERVICE_H_
