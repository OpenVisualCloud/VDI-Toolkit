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
//! \file HostService.h
//! \brief
//! \date 2024-04-10
//!

#ifndef _HOST_SERVICE_H_
#define _HOST_SERVICE_H_

#include "../utils/common.h"
#include "../SHMemory/FrameBufferData.h"

#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <string>

VDI_NS_BEGIN

class HostService
{
public:
    //!
    //! \brief Host service Constructor
    //!
    HostService() = default;
    //!
    //! \brief Host service Destructor
    //!
    virtual ~HostService() = default;

    //!
    //! \brief Initialize host service
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
    virtual MRDAStatus SetInitParams(MediaParams *params) = 0;
    //!
    //! \brief Send input data
    //!
    //! \param [in] context
    //! \param [in] stream
    //! \return Status
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    // virtual Status SendInputData(ServerContext* context, ServerReaderWriter<MRDA::TaskStatus, MRDA::MediaParams>* stream) override = 0;

    //!
    //! \brief Send input data
    //!
    //! \param [in] data
    //! \return MRDAStatus
    //!
    virtual MRDAStatus SendInputData(std::shared_ptr<FrameBufferData> data) = 0;

    //!
    //! \brief
    //!
    //! \param [in] context
    //! \param [in] stream
    //! \return Status
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    // virtual Status ReceiveOutputData(ServerContext* context, ServerReaderWriter<MRDA::TaskStatus, MRDA::MediaParams>* stream) override = 0;

    //!
    //! \brief Receive output data
    //!
    //! \param [out] data
    //! \return MRDAStatus
    //!
    virtual MRDAStatus ReceiveOutputData(std::shared_ptr<FrameBufferData> &data) = 0;

protected:
    //!
    //! \brief Get the In Shm File Ptr object
    //!
    //! \param [in] filePath
    //! \return MRDAStatus
    //!
    MRDAStatus GetInShmFilePtr(std::string filePath);

    //!
    //! \brief Get the Out Shm File Ptr object
    //!
    //! \param [in] filePath
    //! \return MRDAStatus
    //!
    MRDAStatus GetOutShmFilePtr(std::string filePath);
        //!
    //! \brief ref frame
    //!
    //! \param [in] frame
    //!
    void RefOutputFrame(std::shared_ptr<FrameBufferData> frame);

    //!
    //! \brief unref frame
    //!
    //! \param [in] frame
    //!
    void UnRefInputFrame(std::shared_ptr<FrameBufferData> frame);

protected:
    std::unique_ptr<MediaParams> m_mediaParams = nullptr; //<! media parameters

    // Shared Memory
    int m_inShmFile = -1; //<! input shared memory file path
    char *m_inShmMem = nullptr; //<! input shared memory buffer
    size_t m_inShmSize = 0; //<! input shared memory buffer size
    int m_outShmFile = -1; //<! output shared memory file path
    char *m_outShmMem = nullptr; //<! output shared memory buffer
    size_t m_outShmSize = 0; //<! output shared memory buffer size
};

VDI_NS_END
#endif // _HOST_SERVICE_H_