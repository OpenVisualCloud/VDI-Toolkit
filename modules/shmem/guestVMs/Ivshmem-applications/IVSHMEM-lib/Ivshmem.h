/*
 * Copyright (c) 2023, Intel Corporation
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

 *
 */

 //!
 //! \file     Ivshmem.h
 //! \brief    Define class for Ivshmem driver library.
 //!

#ifndef _IVSHMEM_H_
#define _IVSHMEM_H_

#include <windows.h>

enum class IVSHMEMStatus {
	SUCCESS = 0,
	FAILED,
	UNKNOWN,
};

using HANDLE = PVOID;

class Ivshmem
{
public:
	Ivshmem();
	virtual ~Ivshmem();
	IVSHMEMStatus Init();
	IVSHMEMStatus DeInit();
	IVSHMEMStatus Open();
	IVSHMEMStatus Close();
	inline PVOID  GetMemory() const { return m_memory; }
	inline UINT64 GetSize() const { return m_size; }
	inline IVSHMEMStatus SetOffset(UINT64 offset) { m_offset = offset; }
	inline UINT64 GetOffset() { return m_offset; }

private:
	PVOID m_memory;  // mmap pointer, shared memory entry point
	UINT64 m_offset; // memory offset
	UINT64 m_size;   // shared memory region size
	HANDLE m_handle; // device handle
};

#endif // !_IVSHMEM_H_
