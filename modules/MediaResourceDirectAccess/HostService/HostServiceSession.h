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
//! \file HostServiceSession.h
//! \brief Host service session definition using gRPC
//! \date 2024-04-19
//!

#ifndef _HOST_SERVICE_SESSION_H_
#define _HOST_SERVICE_SESSION_H_

#include "../utils/common.h"
#include "HostService.h"
#include "HostServiceFactory.h"

#include <grpc/grpc.h>
// #include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
// using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

#include "../protos/MRDAService.grpc.pb.h"
#include "../protos/MRDAService.pb.h"

#include <string>

VDI_NS_BEGIN

class HostServiceSession : public MRDA::MRDAService::Service
{
public:
    //!
    //! \brief Host service session Constructor
    //!
    HostServiceSession();
    //!
    //! \brief Host service session Destructor
    //!
    virtual ~HostServiceSession() = default;

    //!
    //! \brief Initialize with task info
    //!
    //! \param [in] taskInfo
    //! \return MRDAStatus
    //!
    MRDAStatus Initialize(MRDA::TaskInfo* taskInfo);

    //!
    //! \brief Set the Init Params object
    //!
    //! \param [in] context
    //! \param [in] mediaParams
    //! \param [in] status
    //! \return Status
    //!
    virtual Status SetInitParams(ServerContext* context, const MRDA::MediaParams* mediaParams, MRDA::TaskStatus* status) override;
    //!
    //! \brief Send input data
    //!
    //! \param [in] context
    //! \param [in] stream
    //! \return Status
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    // virtual Status SendInputData(ServerContext* context, ServerReaderWriter<MRDA::TaskStatus, MRDA::MediaParams>* stream) override = 0;

    /// FIXME: use unary API as the first step, then use stream as optimization
    //!
    //! \brief Send input data using gRPC
    //!
    //! \param [in] context
    //! \param [in] bufferInfo
    //! \param [out] taskStatus
    //! \return Status
    //!
    virtual Status SendInputData(ServerContext* context, const MRDA::BufferInfo* bufferInfo, MRDA::TaskStatus* taskStatus) override;

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
    //! \brief Receive output data using gRPC
    //!
    //! \param [in] context
    //! \param [in] pts
    //! \param [in] bufferInfo
    //! \return Status
    //!
    virtual Status ReceiveOutputData(ServerContext* context, const MRDA::Pts* pts, MRDA::BufferInfo* bufferInfo) override;

    //!
    //! \brief Get the Host Service Instance object
    //!
    //! \return std::shared_ptr<HostServie>
    //!
    // inline std::shared_ptr<HostService> GetHostServiceInstance()
    // {
    //     return m_hostService;
    // }

    //!
    //! \brief Start the service with server addr
    //!
    //! \param [in] server_address
    //!
    void RunService(std::string server_address);

    //!
    //! \brief Stop the service
    //!
    void StopService();

private:
    //!
    //! \brief Convert mrda buffer info to buffer info
    //!
    //! \param [in] mrda_bufferInfo
    //! \param [out] buffer
    //! \return MRDAStatus
    //!
    MRDAStatus MakeBufferInfoBack(const MRDA::BufferInfo* mrda_bufferInfo, std::shared_ptr<FrameBufferData> &buffer);
    //!
    //! \brief Convert buffer to mrda buffer info
    //!
    //! \param [in] buffer
    //! \param [out] mrda_bufferInfo
    //! \return MRDAStatus
    //!
    MRDAStatus MakeBufferInfo(const std::shared_ptr<FrameBufferData> buffer, MRDA::BufferInfo* mrda_bufferInfo);

    //!
    //! \brief Convert mrda media params to media params
    //!
    //! \param [in] mrda_mediaParams
    //! \param [out] params
    //! \return MRDAStatus
    //!
    MRDAStatus MakeMediaParamsBack(const MRDA::MediaParams* mrda_mediaParams, MediaParams *params);

private:

    std::unique_ptr<Server> m_server; //<! gRPC server handle
    std::shared_ptr<HostService> m_hostService; //<! host service
    std::unique_ptr<HostServiceFactory> m_hostServiceFactory; //<! host service factory

};

VDI_NS_END
#endif // _HOST_SERVICE_SESSION_H_