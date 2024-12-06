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
//! \file SessionManager.cpp
//! \brief implement session manager.
//! \date 2024-04-10
//!

#include "SessionManager.h"

constexpr uint32_t PORT_BASE = 50000;
constexpr uint32_t PORT_NUM = 50;

VDI_USE_MRDALib;

SessionManagerImpl::SessionManagerImpl(std::string server_addr)
{
    m_server_addr = server_addr;
    m_allocator = std::make_shared<GPUResourceFirstAllocatorStrategy>();
    m_resourceManager = std::make_unique<ResourceManager>();
    m_server = nullptr;
    m_hostServices.clear();
    std::string server_base_addr = GetServerBaseAddr(server_addr);
    std::unique_lock<std::mutex> lock(m_addrMutex);
    for (uint32_t i = 1; i <= PORT_NUM; i++)
    {
        m_reservedAddrs.push_back(std::make_pair(i, server_base_addr + ":" + std::to_string(PORT_BASE + i)));
    }
}

std::pair<uint32_t, std::string> SessionManagerImpl::GenerateServiceAddr()
{
    std::pair<uint32_t, std::string> addr;
    std::unique_lock<std::mutex> lock(m_addrMutex);
    if (m_reservedAddrs.empty())
    {
        MRDA_LOG(LOG_ERROR, "Failed to generate service address.");
        return std::make_pair(0, "");
    }
    else
    {
        addr = m_reservedAddrs.front();
        m_reservedAddrs.pop_front();
        return addr;
    }
}

MRDAStatus SessionManagerImpl::AssignResource(TaskInfo *taskInfo)
{
    // default strategy: GPU resource first
    if (m_allocator == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create resource allocator.");
        return MRDA_STATUS_INVALID_DATA;
    }
    // init resource manager and set strategy to allocate resource
    if (m_resourceManager == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create resource manager.");
        return MRDA_STATUS_INVALID_DATA;
    }

    m_resourceManager->SetResourceAllocatorStrategy(m_allocator);

    if (m_resourceManager->AllocateResource(taskInfo) != MRDA_STATUS_SUCCESS)
    {
        MRDA_LOG(LOG_ERROR, "Failed to allocate resource.");
        return MRDA_STATUS_INVALID_DATA;
    }

    return MRDA_STATUS_SUCCESS;
}

void SessionManagerImpl::CopyTaskInfo(const MRDA::TaskInfo *in, MRDA::TaskInfo *out)
{
    if (in == nullptr || out == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to copy task info.");
        return;
    }
    out->set_tasktype(in->tasktype());
    out->set_taskstatus(in->taskstatus());
    out->set_taskid(in->taskid());
    out->set_deviceid(in->deviceid());
    out->set_devicetype(in->devicetype());
    out->set_ipaddr(in->ipaddr());
}

std::string SessionManagerImpl::GetServerBaseAddr(std::string server_base_addr)
{
    // Find the position of the colon.
    size_t colon_pos = server_base_addr.find(':');

    // If there is no colon, return the entire input string.
    if (colon_pos == std::string::npos) {
        return server_base_addr;
    }

    // Extract the string before the colon.
    return server_base_addr.substr(0, colon_pos);
}

Status SessionManagerImpl::StartService(ServerContext* context, const MRDA::TaskInfo* in_mrdaInfo, MRDA::TaskInfo* out_mrdaInfo)
{
    // check input parameters
    if (in_mrdaInfo == nullptr || out_mrdaInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get task info.");
        return Status::CANCELLED;
    }
    CopyTaskInfo(in_mrdaInfo, out_mrdaInfo);
    // get service address
    std::pair<uint32_t, std::string> serviceAddr = GenerateServiceAddr();
    if (serviceAddr.second.empty())
    {
        MRDA_LOG(LOG_ERROR, "Failed to generate service address.");
        return Status::CANCELLED;
    }
    // assign resource
    TaskInfo taskInfo;
    taskInfo.taskID = serviceAddr.first;
    if (MRDA_STATUS_SUCCESS != AssignResource(&taskInfo))
    {
        MRDA_LOG(LOG_ERROR, "Failed to assign resource.");
        return Status::CANCELLED;
    }
    // set device information to out_mrdaInfo
    out_mrdaInfo->set_deviceid(taskInfo.taskDevice.deviceID);
    out_mrdaInfo->set_devicetype(static_cast<int32_t>(taskInfo.taskDevice.deviceType));
    // create host service session
    std::shared_ptr<HostServiceSession> hostServiceSession = std::make_shared<HostServiceSession>();
    if (hostServiceSession == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to create host service server.");
        return Status::CANCELLED;
    }
    // Initialize host service session
    if (MRDA_STATUS_SUCCESS != hostServiceSession->Initialize(out_mrdaInfo))
    {
        MRDA_LOG(LOG_ERROR, "Failed to initialize host service.");
        return Status::CANCELLED;
    }

    // start service start thread
    std::thread hostServiceStartThread([=](){
        hostServiceSession->RunService(serviceAddr.second);
    });
    // detach service start thread
    hostServiceStartThread.detach();

    // update host services
    std::unique_lock<std::mutex> lock(m_servicesMutex);
    m_hostServices.insert(std::make_pair(serviceAddr.first, std::make_pair(serviceAddr.second, hostServiceSession)));

    out_mrdaInfo->set_taskid(serviceAddr.first);
    out_mrdaInfo->set_ipaddr(serviceAddr.second);
    out_mrdaInfo->set_taskstatus(static_cast<int32_t>(TASKStatus::TASK_STATUS_INITIALIZED));

    return Status::OK;
}


Status SessionManagerImpl::StopService(ServerContext* context, const MRDA::TaskInfo* taskInfo, MRDA::TASKStatus* status)
{
    if (taskInfo == nullptr || status == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get task info.");
        return Status::CANCELLED;
    }
    // get host service according to task id
    std::unique_lock<std::mutex> lock(m_servicesMutex);
    auto it = m_hostServices.find(taskInfo->taskid());
    if (it == m_hostServices.end())
    {
        MRDA_LOG(LOG_ERROR, "Failed to find host service.");
        status->set_status(static_cast<int32_t>(TASKStatus::TASK_STATUS_ERROR));
        return Status::CANCELLED;
    }
    else
    {
        int32_t taskId = (*it).first;
        std::string serviceAddr = (*it).second.first;
        std::shared_ptr<HostServiceSession> hostService = (*it).second.second;
        hostService->StopService();
        status->set_status(static_cast<int32_t>(TASKStatus::TASK_STATUS_STOPPED));
        // release service and address
        m_hostServices.erase(it);
        std::unique_lock<std::mutex> lock(m_addrMutex);
        m_reservedAddrs.push_back(std::make_pair(taskId, serviceAddr));
        MRDA_LOG(LOG_INFO, "Stop service! task id : %d", taskId);
        return Status::OK;
    }
}

Status SessionManagerImpl::ResetService(ServerContext* context, const MRDA::TaskInfo* taskInfo, MRDA::TASKStatus* status)
{
    //FIXME: implementation needed
    return Status::OK;
}

void SessionManagerImpl::RunService()
{
    if (m_server_addr.empty())
    {
        MRDA_LOG(LOG_ERROR, "Failed to get server address.");
        return;
    }
    std::unique_ptr<ServerBuilder> serverBuilder = std::make_unique<ServerBuilder>();
    serverBuilder->AddListeningPort(m_server_addr, grpc::InsecureServerCredentials());
    serverBuilder->RegisterService(this);
    m_server = serverBuilder->BuildAndStart();
    MRDA_LOG(LOG_INFO, "HostService listening on %s", m_server_addr.c_str());
    m_server->Wait();
}

void SessionManagerImpl::StopService()
{
    if (m_server != nullptr)
    {
        m_server->Shutdown();
    }
}

//!
//! \brief Main function to run session manager on host
//!
//! \param [in] argc
//! \param [in] argv
//! \return int
//!
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        MRDA_LOG(LOG_ERROR, "Usage: %s -addr <serviceIp:port>", argv[0]);
        return -1;
    }
    std::string server_address(argv[2]);
    SessionManagerImpl serviceManager(server_address);
    serviceManager.RunService();
    return 0;
}