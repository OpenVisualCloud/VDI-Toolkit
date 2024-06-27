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
//! \file TaskDataSession_gRPC.h
//! \brief task data session for data streaming using gRPC
//! \date 2024-04-18
//!

#ifndef _TASK_DATA_SESSION_GRPC_H_
#define _TASK_DATA_SESSION_GRPC_H_

#include "TaskDataSession.h"

#include <grpc/grpc.h>
// #include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "../protos/MRDAService.grpc.pb.h"

using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using grpc::Channel;

VDI_NS_BEGIN

class TaskDataSession_gRPC : public TaskDataSession
{
public:
    //!
    //! \brief Construct a new TaskDataSession_gRPC object
    //!
    TaskDataSession_gRPC(){};

    //!
    //! \brief Construct a new TaskDataSession_gRPC object
    //!
    //! \param [in] taskInfo
    //!
    TaskDataSession_gRPC(std::shared_ptr<TaskInfo> taskInfo);

    //!
    //! \brief Destroy the TaskDataSession_gRPC object
    //!
    virtual ~TaskDataSession_gRPC() = default;

    //!
    //! \brief Initialize the Task Data Session object
    //!
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    // MRDAStatus Initialize();
    //!
    //! \brief Set the Init Params object
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus SetInitParams(const MediaParams *params);
    //!
    //! \brief Send the data to the remote host
    //!
    //! \param [in] data
    //!             the data to send
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus SendFrame(const std::shared_ptr<FrameBufferData> data);
    //!
    //! \brief Receive the data from the remote host
    //!
    //! \param [in] data
    //!             the data to Received
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus ReceiveFrame(std::shared_ptr<FrameBufferData> &data);

private:

    //!
    //! \brief Convert media params to MRDA type
    //!
    //! \param [in] params
    //! \return MRDA::MediaParams
    //!
    MRDA::MediaParams MakeMediaParams(const MediaParams *params);

    //!
    //! \brief Convert frame buffer data to MRDA buffer info
    //!
    //! \param [in] data
    //! \return MRDA::BufferInfo
    //!
    MRDA::BufferInfo MakeBufferInfo(const std::shared_ptr<FrameBufferData> data);

    //!
    //! \brief Convert pts to MRDA pts
    //!
    //! \param [in] pts
    //! \return MRDA::Pts
    //!
    MRDA::Pts MakePts(uint64_t pts);

    //!
    //! \brief Convert mrda buffer info to FrameBufferData pointer
    //!
    //! \param [in] info
    //! \return std::shared_ptr<FrameBufferData>
    //!
    std::shared_ptr<FrameBufferData> MakeBufferInfoBack(MRDA::BufferInfo info);

private:
    std::unique_ptr<MRDA::MRDAService::Stub> m_stub; // client gRPC root
};

VDI_NS_END
#endif // _TASK_DATA_SESSION_GRPC_H_