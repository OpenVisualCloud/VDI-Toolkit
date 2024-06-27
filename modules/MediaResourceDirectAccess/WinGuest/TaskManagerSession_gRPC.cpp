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
//! \file TaskManagerSession_gRPC.cpp
//! \brief implement task manager session to help task manager to
//!        control the status of host services using gRPC.
//! \date 2024-04-08
//!

#include "TaskManagerSession_gRPC.h"

VDI_NS_BEGIN

TaskManagerSession_gRPC::TaskManagerSession_gRPC(std::string ipAddr)
{
    std::shared_ptr<Channel> channel = grpc::CreateChannel(ipAddr, grpc::InsecureChannelCredentials());
    m_stub = MRDA::MRDAServiceManager::NewStub(channel);
}

MRDA::TaskInfo TaskManagerSession_gRPC::MakeTaskInfo(const TaskInfo *taskInfo)
{
    MRDA::TaskInfo mrda_info;
    mrda_info.set_tasktype(static_cast<int32_t>(taskInfo->taskType));
    mrda_info.set_taskstatus(static_cast<int32_t>(taskInfo->taskStatus));
    mrda_info.set_taskid(taskInfo->taskID);
    mrda_info.set_deviceid(taskInfo->taskDevice.deviceID);
    mrda_info.set_devicetype(static_cast<int32_t>(taskInfo->taskDevice.deviceType));
    mrda_info.set_ipaddr(taskInfo->ipAddr);
    return mrda_info;
}

TaskInfo TaskManagerSession_gRPC::MakeTaskInfoBack(const MRDA::TaskInfo *mrda_taskInfo)
{
    TaskInfo taskInfo;
    taskInfo.taskType = static_cast<TASKTYPE>(mrda_taskInfo->tasktype());
    taskInfo.taskStatus = static_cast<TASKStatus>(mrda_taskInfo->taskstatus());
    taskInfo.taskID = mrda_taskInfo->taskid();
    taskInfo.taskDevice.deviceID = mrda_taskInfo->deviceid();
    taskInfo.taskDevice.deviceType = static_cast<DeviceType>(mrda_taskInfo->devicetype());
    taskInfo.ipAddr = mrda_taskInfo->ipaddr();
    return taskInfo;
}

TASKStatus TaskManagerSession_gRPC::MakeTaskStatusBack(const MRDA::TASKStatus *mrda_taskStatus)
{
    return static_cast<TASKStatus>(mrda_taskStatus->status());
}

MRDAStatus TaskManagerSession_gRPC::StartTask(const TaskInfo* in_taskInfo, TaskInfo* out_taskInfo)
{
    ClientContext context;
    MRDA::TaskInfo in_mrda_taskInfo = MakeTaskInfo(in_taskInfo);
    MRDA::TaskInfo out_mrda_taskInfo;
    Status status = m_stub->StartService(&context, in_mrda_taskInfo, &out_mrda_taskInfo);
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to start task!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    *out_taskInfo = MakeTaskInfoBack(&out_mrda_taskInfo);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManagerSession_gRPC::StopTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus)
{
    ClientContext context;
    MRDA::TaskInfo in_mrda_taskInfo = MakeTaskInfo(in_taskInfo);
    MRDA::TASKStatus out_mrda_taskStatus;
    Status status = m_stub->StopService(&context, in_mrda_taskInfo, &out_mrda_taskStatus);
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to start task!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    *out_taskStatus = MakeTaskStatusBack(&out_mrda_taskStatus);
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus TaskManagerSession_gRPC::ResetTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus)
{
    ClientContext context;
    MRDA::TaskInfo in_mrda_taskInfo = MakeTaskInfo(in_taskInfo);
    MRDA::TASKStatus out_mrda_taskStatus;
    Status status = m_stub->ResetService(&context, in_mrda_taskInfo, &out_mrda_taskStatus);
    if (!status.ok())
    {
        MRDA_LOG(LOG_ERROR, "Failed to start task!");
        return MRDA_STATUS_OPERATION_FAIL;
    }
    *out_taskStatus = MakeTaskStatusBack(&out_mrda_taskStatus);
    return MRDA_STATUS_SUCCESS;
}



VDI_NS_END