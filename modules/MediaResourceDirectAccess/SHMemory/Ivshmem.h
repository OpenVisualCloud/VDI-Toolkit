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
//! \file Ivshmem.h
//! \brief Inter-VM share memory interface
//! \date 2024-04-01
//!

#ifndef _IVSHMEM_H_
#define _IVSHMEM_H_

#include <windows.h>
#include "../utils/common.h"

VDI_NS_BEGIN

using HANDLE = PVOID;

class Ivshmem
{
public:
	//!
	//! \brief Construct a new Ivshmem object
	//!
	Ivshmem();
    //!
    //! \brief Destroy the Ivshmem object
    //!
	virtual ~Ivshmem();
    //!
    //! \brief Initialize ivshmem
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
	MRDAStatus Init();
    //!
    //! \brief Deinitialize ivshmem
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
	MRDAStatus DeInit();
    //!
    //! \brief Open the ivshmem device
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
	MRDAStatus Open();
    //!
    //! \brief Close the ivshmem device
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
	MRDAStatus Close();
    //!
    //! \brief Get the Memory pointer
    //!
    //! \return PVOID
    //!         the memory pointer
    //!
	inline PVOID  GetMemory() const { return m_memory; }
    //!
    //! \brief Get the Memory size
    //!
    //! \return UINT64
    //!         the memory size
    //!
	inline UINT64 GetSize() const { return m_size; }
    //!
    //! \brief Set the Offset object
    //!
    //! \param [in] offset
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
	inline MRDAStatus SetOffset(UINT64 offset) { m_offset = offset; }
    //!
    //! \brief Get the Offset object
    //!
    //! \return UINT64
    //!         the memory offset
    //!
	inline UINT64 GetOffset() { return m_offset; }

private:
	PVOID m_memory;  //!< mmap pointer, shared memory entry point
	UINT64 m_offset; //!< memory offset
	UINT64 m_size;   //!< shared memory region size
	HANDLE m_handle; //!< device handle
};

VDI_NS_END
#endif // _IVSHMEM_H_