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
//! \file FrameMemoryPool.h
//! \brief define a memory pool to manage the share memory
//!        between host and windows VMs.
//! \date 2024-04-01
//!

#ifndef _FRAMEMEMORYPOOL_
#define _FRAMEMEMORYPOOL_

#include "Ivshmem.h"
#include "FrameBufferData.h"

#include <vector>
#include <mutex>

VDI_NS_BEGIN

//!
//! \brief General memory pool template
//!
//!
template<typename T>
class MemoryPool
{
public:
    //!
    //! \brief Construct a new Memory Pool object
    //!
    MemoryPool():
    m_bufferPoolCount(0),
    m_bufferSize(0),
    m_shareMemSize(0),
    m_shareMemPtr(nullptr),
    m_ivshmem(nullptr)
    {
        m_bufferPool.clear();
    }
    //!
    //! \brief Destroy the Memory Pool object
    //!
    virtual ~MemoryPool()
    {
    }
    //!
    //! \brief Destroy the buffer pool
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus DestroyBufferPool()
    {
        m_bufferPool.clear();
        m_bufferPoolCount = 0;
        m_bufferSize = 0;
        m_shareMemSize = 0;
        m_shareMemPtr = nullptr;
        return MRDA_STATUS_SUCCESS;
    }
    //!
    //! \brief Initialize the buffer pool
    //!
    //! \param [in] buffer_num
    //!             number of buffers in the pool
    //! \param [in] buffer_size
    //!             size of each buffer in the pool
    //! \param [in] slot_number
    //!             memory device slot number
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus InitBufferPool(const uint32_t buffer_num, const uint64_t buffer_size, const uint32_t slot_number)
    {
        // initialize ivshmem device
        if (MRDA_STATUS_SUCCESS != InitializeMemoryDevice(slot_number))
        {
            MRDA_LOG(LOG_ERROR, "Failed to initialize ivshmem device!");
            return MRDA_STATUS_INVALID;
        }

        m_bufferPoolCount = buffer_num;
        m_bufferSize = buffer_size;

        m_shareMemSize = GetMemorySize();
        m_shareMemPtr = GetMemoryPtr();
        if (nullptr == m_shareMemPtr || 0 == m_shareMemSize)
        {
            MRDA_LOG(LOG_ERROR, "Failed to get share memory pointer!");
            return MRDA_STATUS_INVALID;
        }

        // reset all share memory
        memset(m_shareMemPtr, 0, m_shareMemSize);

        if (m_bufferPoolCount * m_bufferSize > m_shareMemSize
            || m_bufferPoolCount < 1)
        {
            MRDA_LOG(LOG_ERROR, "Invalid buffer pool parameters!");
            return MRDA_STATUS_INVALID;
        }

        return MRDA_STATUS_SUCCESS;
    }
    //!
    //! \brief Allocate buffer pool
    //!
    //! \return MRDAStatus
    //!
    virtual MRDAStatus AllocateBufferPool() = 0;
    //!
    //! \brief Get one Buffer from buffer pool
    //!
    //! \param [out] buffer
    //!              the buffer to be obtained
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus GetBuffer(std::shared_ptr<T> &buffer) = 0;
    //!
    //! \brief Release one buffer from buffer pool
    //!
    //! \param [in] buffer
    //!              the buffer to be released
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus ReleaseBuffer(std::shared_ptr<T> buffer) = 0;
    //!
    //! \brief Get one Buffer from buffer pool
    //!
    //! \param       [in] id
    //!              request buf id
    //!              [out] buffer
    //!              the buffer to be obtained
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus GetBufferFromId(uint32_t id, std::shared_ptr<T>& buffer) = 0;

private:
    //!
    //! \brief Initialize ivshmem device
    //!
    //! \param       [in] slot_number
    //!              memory device slot number
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus InitializeMemoryDevice(const uint32_t slot_number)
    {
        m_ivshmem = std::make_unique<Ivshmem>();
        if (m_ivshmem->Init(slot_number) != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "Failed to initialize IVSHMEM!");
            return MRDA_STATUS_INVALID;
        }

        if (m_ivshmem->Open() != MRDA_STATUS_SUCCESS)
        {
            MRDA_LOG(LOG_ERROR, "Failed to open IVSHMEM!");
            return MRDA_STATUS_INVALID;
        }
        return MRDA_STATUS_SUCCESS;
    }
    //!
    //! \brief Get the Memory Pointer from ivshmem device
    //!
    //! \return PVOID
    //!         memory pointer from ivshmem device
    //!
    const PVOID GetMemoryPtr() const {return m_ivshmem->GetMemory();}
    //!
    //! \brief Get the Memory Size object
    //!
    //! \return uint64_t
    //!         memory size of the ivshmem device
    //!
    const uint64_t GetMemorySize() const {return m_ivshmem->GetSize();}

protected:
    std::mutex m_bufferPoolMutex; //!< mutex for buffer pool
    std::vector<std::shared_ptr<T>> m_bufferPool; //!< buffer pool
    uint32_t m_bufferPoolCount; //!< number of buffers in the pool
    uint64_t m_bufferSize;      //!< size of each buffer in the pool
    uint64_t m_shareMemSize;    //!< size of share memory
    PVOID m_shareMemPtr;        //!< pointer of share memory
    std::unique_ptr<Ivshmem> m_ivshmem; //!< ivshmem object
};

class FrameMemoryPool : public MemoryPool<FrameBufferData> {
public:
    //!
    //! \brief Construct a new Frame Memory Pool object
    //!
    //!
    FrameMemoryPool(): MemoryPool<FrameBufferData>() {}
    //!
    //! \brief Destroy the Frame Memory Pool object
    //!
    //!
    virtual ~FrameMemoryPool() {}
    //!
    //! \brief Allocate buffer pool
    //!
    //! \return MRDAStatus
    //!
    virtual MRDAStatus AllocateBufferPool() override;
    //!
    //! \brief Get one Buffer from buffer pool
    //!
    //! \param [out] buffer
    //!              the buffer to be obtained
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus GetBuffer(std::shared_ptr<FrameBufferData> &buffer) override;
    //!
    //! \brief Release one buffer from buffer pool
    //!
    //! \param [in] buffer
    //!              the buffer to be released
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus ReleaseBuffer(std::shared_ptr<FrameBufferData> buffer) override;
    //!
    //! \brief Get one Buffer from buffer pool
    //!
    //! \param       [in] id
    //!              request buf id
    //!              [out] buffer
    //!              the buffer to be obtained
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus GetBufferFromId(uint32_t id, std::shared_ptr<FrameBufferData>& buffer) override;
};

VDI_NS_END
#endif // _FRAMEMEMORYPOOL_