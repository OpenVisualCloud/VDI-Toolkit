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
//! \file ResourceManager.h
//! \brief manage host resource to wisely assign hardware
//!        to given task
//! \date 2024-04-09
//!

#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "../utils/common.h"

#include <vector>

VDI_NS_BEGIN

class ResourceAllocatorStrategy
{
public:
    //!
    //! \brief Construct a new Resource Allocator Strategy object
    //!
    ResourceAllocatorStrategy()
    {
        m_gpuUsage.clear();
        m_cpuUsage = 0.0f;
    };
    //!
    //! \brief Destroy the Resource Allocator Strategy object
    //!
    virtual ~ResourceAllocatorStrategy() = default;

    //!
    //! \brief Check GPU resources on host
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    MRDAStatus CheckGPU();

    //!
    //! \brief Check CPU resources on host
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    MRDAStatus CheckCPU();

    //!
    //! \brief allocate resource by task info
    //!
    //! \param [in/out] taskInfo
    //! \return MRDAStatus
    //!
    virtual MRDAStatus AllocateResource(TaskInfo *taskInfo) = 0;
protected:
    std::vector<std::pair<uint32_t, float>> m_gpuUsage; //!< gpu usage
    float m_cpuUsage; //!< cpu usage
};

class CPUResourceFirstAllocatorStrategy : public ResourceAllocatorStrategy
{
public:
    //!
    //! \brief Construct a new CPUResource First Allocator Strategy object
    //!
    CPUResourceFirstAllocatorStrategy() {};
    //!
    //! \brief Destroy the CPUResource First Allocator Strategy object
    //!
    virtual ~CPUResourceFirstAllocatorStrategy() = default;

    //!
    //! \brief allocate resource by task info
    //!
    //! \param [in/out] taskInfo
    //! \return MRDAStatus
    //!
    virtual MRDAStatus AllocateResource(TaskInfo *taskInfo) override;
};

class GPUResourceFirstAllocatorStrategy : public ResourceAllocatorStrategy
{
public:
    //!
    //! \brief Construct a new GPUResource First Allocator Strategy object
    //!
    GPUResourceFirstAllocatorStrategy() {};
    //!
    //! \brief Destroy the GPUResource First Allocator Strategy object
    //!
    virtual ~GPUResourceFirstAllocatorStrategy() = default;

    //!
    //! \brief allocate resource by task info
    //!
    //! \param [in/out] taskInfo
    //! \return MRDAStatus
    //!
    virtual MRDAStatus AllocateResource(TaskInfo *taskInfo) override;
};

class ResourceManager
{
public:
    //!
    //! \brief Construct a new Resource Manager object
    //!
    ResourceManager();
    //!
    //! \brief Destroy the Resource Manager object
    //!
    virtual ~ResourceManager() = default;

    //!
    //! \brief Set the Resource Allocator Strategy object
    //!
    //! \param [in] strategy
    //! \return MRDAStatus
    //!
    MRDAStatus SetResourceAllocatorStrategy(std::shared_ptr<ResourceAllocatorStrategy> strategy);

    //!
    //! \brief allocate resource by task info
    //!
    //! \param [in/out] taskInfo
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    MRDAStatus AllocateResource(TaskInfo *taskInfo);

private:
    std::shared_ptr<ResourceAllocatorStrategy> m_allocatorStrategy; //!< resource allocator strategy
};

#endif // _RESOURCE_MANAGER_H_
VDI_NS_END