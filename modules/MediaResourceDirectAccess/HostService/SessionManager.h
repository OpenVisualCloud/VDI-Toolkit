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
//! \file SessionManager.h
//! \brief an application run on host, which controls and manages host services.
//! \date 2024-04-10
//!

#ifndef _SESSIONMANAGER_H_
#define _SESSIONMANAGER_H_

#include <grpc/grpc.h>
// #include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
// using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

#include "../protos/MRDAServiceManager.grpc.pb.h"
#include "../protos/MRDAServiceManager.pb.h"

#include "../protos/MRDAService.grpc.pb.h"
#include "../protos/MRDAService.pb.h"

#include "ResourceManager.h"
#include "HostServiceSession.h"
#include "HostService.h"

#include <map>
#include <string>

VDI_USE_MRDALib;

class SessionManagerImpl final : public MRDA::MRDAServiceManager::Service
{
public:
    //!
    //! \brief Construct a new Service Manager Impl object
    //!
    SessionManagerImpl(std::string server_addr);

    //!
    //! \brief Destroy the Service Manager Impl object
    //!
    virtual ~SessionManagerImpl() = default;

    //!
    //! \brief Start the service
    //!
    //! \param [in] context
    //! \param [in] in_taskInfo
    //! \param [in] out_taskInfo
    //! \return Status
    //!
    virtual Status StartService(ServerContext* context, const MRDA::TaskInfo* in_taskInfo, MRDA::TaskInfo* out_taskInfo) override;

    //!
    //! \brief Stop the service
    //!
    //! \param [in] context
    //! \param [in] taskInfo
    //! \param [out] status
    //! \return Status
    //!
    virtual Status StopService(ServerContext* context, const MRDA::TaskInfo* taskInfo, MRDA::TASKStatus* status) override;

    //!
    //! \brief Reset the service
    //!
    //! \param [in] context
    //! \param [in] taskInfo
    //! \param [in] status
    //! \return Status
    //!
    virtual Status ResetService(ServerContext* context, const MRDA::TaskInfo* taskInfo, MRDA::TASKStatus* status) override;

    //!
    //! \brief Start the service with server addr
    //!
    void RunService();

    //!
    //! \brief Stop the service
    //!
    void StopService();

private:
    //!
    //! \brief Generate service addr with task id
    //!
    //! \return std::pair<uint32_t, std::string>
    //!
    std::pair<uint32_t, std::string> GenerateServiceAddr();

    //!
    //! \brief Assign hardware resource to a host service
    //!
    //! \param [out] taskInfo
    //! \return MRDAStatus
    //!
    MRDAStatus AssignResource(TaskInfo* taskInfo);

    //!
    //! \brief Copy taskInfo in to out
    //!
    //! \param [in] in
    //! \param [out] out
    //!
    void CopyTaskInfo(const MRDA::TaskInfo *in, MRDA::TaskInfo *out);

    //!
    //! \brief Get the Server Base Addr object
    //!
    //! \param [in] server_addr
    //! \return string
    //!
    std::string GetServerBaseAddr(std::string server_addr);

private:
    std::string m_server_addr; //!< server address
    std::shared_ptr<ResourceAllocatorStrategy> m_allocator; //!< resource allocator strategy
    std::unique_ptr<ResourceManager> m_resourceManager; //!< resource manager
    std::unique_ptr<Server> m_server; //<! gRPC server handle
    std::map<int32_t, std::pair<std::string, std::shared_ptr<HostServiceSession>>> m_hostServices; //!< for each service addr, assign a host service.
    std::list<std::pair<uint32_t, std::string>> m_reservedAddrs; //!< reservced service addrs
    std::mutex m_addrMutex; //!< mutex for addr
    std::mutex m_servicesMutex; //!< mutex for services
};

#endif //_SESSIONMANAGER_H_