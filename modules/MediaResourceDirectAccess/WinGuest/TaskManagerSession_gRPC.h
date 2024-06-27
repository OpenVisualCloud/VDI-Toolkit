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
//! \file TaskManagerSession_gRPC.h
//! \brief task manager session to help task manager to
//!        control the status of host services using gRPC.
//! \date 2024-04-08
//!

#ifndef _TASK_MANAGER_SESSION_GRPC_H_
#define _TASK_MANAGER_SESSION_GRPC_H_

#include "TaskManagerSession.h"

#include <list>

#include <grpc/grpc.h>
// #include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "../protos/MRDAServiceManager.grpc.pb.h"

using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using grpc::Channel;

VDI_NS_BEGIN

class TaskManagerSession_gRPC : public TaskManagerSession
{
public:
    //!
    //! \brief Construct a new TaskManagerSession_gRPC object
    //!
    //! \param [in] channel
    //! \param [in] db
    //!
    TaskManagerSession_gRPC(std::string ipAddr);

    //!
    //! \brief Destroy the TaskManagerSession_gRPC object
    //!
    virtual ~TaskManagerSession_gRPC() = default;

    //!
    //! \brief Start the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskInfo
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus StartTask(const TaskInfo* in_taskInfo, TaskInfo* out_taskInfo);
    //!
    //! \brief Stop the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskStatus
    //! \return MRDAStatus
    //！        MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus StopTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus);
    //!
    //! \brief Reset the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskStatus
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    virtual MRDAStatus ResetTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus);

private:
    //!
    //! \brief Convert task info to gRPC message
    //!
    //! \param [in] taskInfo
    //! \return MRDA::TaskInfo
    //!
    MRDA::TaskInfo MakeTaskInfo(const TaskInfo *taskInfo);

    //!
    //! \brief Convert back task info from gRPC message
    //!
    //! \param [in] mrda_taskInfo
    //! \return TaskInfo
    //!
    TaskInfo MakeTaskInfoBack(const MRDA::TaskInfo *mrda_taskInfo);

    //!
    //! \brief Convert back task status from gRPC message
    //!
    //! \param [in] mrda_taskStatus
    //! \return TASKStatus
    //!
    TASKStatus MakeTaskStatusBack(const MRDA::TASKStatus *mrda_taskStatus);

private:
    std::unique_ptr<MRDA::MRDAServiceManager::Stub> m_stub;
};

VDI_NS_END
#endif //_TASK_MANAGER_SESSION_GRPC_H_