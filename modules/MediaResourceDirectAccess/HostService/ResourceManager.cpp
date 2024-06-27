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
//! \file ResourceManager.cpp
//! \brief implement resource manager
//! \date 2024-04-09
//!

#include "ResourceManager.h"

#include <stdio.h>
#include <cstring>
#include <algorithm>

VDI_NS_BEGIN

constexpr float MAX_GPU_USAGE = 100.0f;
constexpr float MAX_CPU_USAGE = 100.0f;

MRDAStatus ResourceAllocatorStrategy::CheckGPU()
{
    // check i915 driver
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    FILE* fp = popen("cat /sys/class/drm/renderD*/device/uevent | grep 'DRIVER=i915' | wc -l", "r");
    if (fp == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to check GPU device nodes info");
        return MRDA_STATUS_INVALID_DATA;
    }

    if(fgets(buffer, sizeof(buffer), fp) == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get GPU device nodes info");
        pclose(fp);
        return MRDA_STATUS_INVALID_DATA;
    }
    pclose(fp);

    int32_t gpuCount = atoi(buffer);
    if (gpuCount <= 0)
    {
        MRDA_LOG(LOG_ERROR, "No GPU device nodes found");
        return MRDA_STATUS_INVALID_DATA;
    }

    // get xpu-smi info: gpu usage
    for (int32_t i = 0; i < gpuCount; i++)
    {
        char cmd[128];
        sprintf(cmd, "xpu-smi dump -m 0 -d %d | head -n 2 | tail -n 1 | awk -F ',' '{print $NF}'", i);
        fp = popen(cmd, "r");
        if (fp == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "Failed to get GPU device nodes info");
            return MRDA_STATUS_INVALID_DATA;
        }

        memset(buffer, 0, sizeof(buffer));
        if(fgets(buffer, sizeof(buffer), fp) == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "Failed to get GPU device nodes info");
            pclose(fp);
            return MRDA_STATUS_INVALID_DATA;
        }
        pclose(fp);
        float curGpuUsage = atof(buffer);
        if (curGpuUsage < 0 || curGpuUsage > 100)
        {
            MRDA_LOG(LOG_ERROR, "Invalid GPU usage: %f, cpu id %d", curGpuUsage, i);
            return MRDA_STATUS_INVALID_DATA;
        }

        MRDA_LOG(LOG_INFO, "curGpuUsage %f", curGpuUsage);
        auto it = std::find_if(m_gpuUsage.begin(), m_gpuUsage.end(), [i](const std::pair<uint32_t, float>& element) {
        return element.first == i;});
        if (it != m_gpuUsage.end()) {
            // MRDA_LOG(LOG_INFO, "In gpu vector UPDATE : %d, %f", it->first, curGpuUsage);
            it->second = curGpuUsage;
        } else {
            // MRDA_LOG(LOG_INFO, "In gpu vector PUSH : %d, %f", i, curGpuUsage);
            m_gpuUsage.push_back(std::make_pair(i, curGpuUsage));
        }
    }
    return MRDA_STATUS_SUCCESS;
}



MRDAStatus ResourceAllocatorStrategy::CheckCPU()
{
    // get cpu cores number
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    FILE* fp = popen("cat /proc/cpuinfo | grep 'cpu cores' | uniq | cut -d ':' -f 2", "r");
    if (fp == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get cpu cores number");
        return MRDA_STATUS_INVALID_DATA;
    }

    if(fgets(buffer, sizeof(buffer), fp) == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get cpu cores number");
        pclose(fp);
        return MRDA_STATUS_INVALID_DATA;
    }
    pclose(fp);

    int cpuCores = atoi(buffer);

    MRDA_LOG(LOG_INFO, "cpuCores %d", cpuCores);
    if (cpuCores <= 0)
    {
        MRDA_LOG(LOG_ERROR, "Invalid cpu cores number: %d", cpuCores);
        return MRDA_STATUS_INVALID_DATA;
    }

    // get cpu usage
    fp = popen("top -b -n 1 | grep 'Cpu(s)' | uniq | cut -d ':' -f 2 | cut -d ',' -f 4 | awk '{print $1}'", "r");
    if (fp == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get cpu usage");
        return MRDA_STATUS_INVALID_DATA;
    }

    if(fgets(buffer, sizeof(buffer), fp) == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Failed to get cpu usage");
        pclose(fp);
        return MRDA_STATUS_INVALID_DATA;
    }
    pclose(fp);

    float idle_cpuUsage = atof(buffer);
    if (idle_cpuUsage < 0)
    {
        MRDA_LOG(LOG_ERROR, "Invalid cpu usage: %f", idle_cpuUsage);
        return MRDA_STATUS_INVALID_DATA;
    }
    // calculate cpu usage
    float total_cpuUsage = 100 - idle_cpuUsage;
    m_cpuUsage = total_cpuUsage;

    MRDA_LOG(LOG_INFO, "m_cpuUsage %f", m_cpuUsage);

    return MRDA_STATUS_SUCCESS;
}

MRDAStatus CPUResourceFirstAllocatorStrategy::AllocateResource(TaskInfo *taskInfo)
{
    //CPU resource first
    // check gpu and cpu usage VALID
    if (m_gpuUsage.empty() || m_cpuUsage <= 0.0f)
    {
        MRDA_LOG(LOG_ERROR, "Invalid gpu or cpu usage");
        return MRDA_STATUS_INVALID_DATA;
    }

    // assign cpu resoure first
    if (m_cpuUsage < MAX_CPU_USAGE)
    {
        taskInfo->taskDevice = {DeviceType::CPU, 0};
    }
    else
    {
        // if cpu resource full, then consider gpu resource
        for (auto gpuUsage : m_gpuUsage)
        {
            if (gpuUsage.second < MAX_GPU_USAGE)
            {
                // assign gpu
                taskInfo->taskDevice = {DeviceType::GPU, gpuUsage.first};
                break;
            }
        }
    }
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus GPUResourceFirstAllocatorStrategy::AllocateResource(TaskInfo *taskInfo)
{
    //GPU resource first
    // check gpu and cpu usage VALID
    if (m_gpuUsage.empty() || m_cpuUsage <= 0.0f)
    {
        MRDA_LOG(LOG_ERROR, "Invalid gpu or cpu usage");
        return MRDA_STATUS_INVALID_DATA;
    }
    // assign gpu resource first
    for (auto gpuUsage : m_gpuUsage)
    {
        if (gpuUsage.second < MAX_GPU_USAGE)
        {
            // assign gpu
            taskInfo->taskDevice = {DeviceType::GPU, gpuUsage.first};
            break;
        }
    }

    // if gpu resource full, then consider cpu resource
    if (taskInfo->taskDevice.deviceType != DeviceType::GPU)
    {
        if (m_cpuUsage < MAX_CPU_USAGE)
        {
            taskInfo->taskDevice = {DeviceType::CPU, 0};
        }
    }
}

ResourceManager::ResourceManager()
    :m_allocatorStrategy(nullptr)
{
}

MRDAStatus ResourceManager::SetResourceAllocatorStrategy(std::shared_ptr<ResourceAllocatorStrategy> strategy)
{
    if (strategy == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid strategy");
        return MRDA_STATUS_INVALID_DATA;
    }
    m_allocatorStrategy = strategy;
    return MRDA_STATUS_SUCCESS;
}

MRDAStatus ResourceManager::AllocateResource(TaskInfo *taskInfo)
{
    if (taskInfo == nullptr)
    {
        MRDA_LOG(LOG_ERROR, "Invalid task info");
        return MRDA_STATUS_INVALID_DATA;
    }

    if (MRDA_STATUS_SUCCESS != m_allocatorStrategy->CheckCPU() ||
        MRDA_STATUS_SUCCESS != m_allocatorStrategy->CheckGPU())
    {
        MRDA_LOG(LOG_ERROR, "Failed to check GPU or CPU!");
        return MRDA_STATUS_OPERATION_FAIL;
    }

    return m_allocatorStrategy->AllocateResource(taskInfo);
}

VDI_NS_END