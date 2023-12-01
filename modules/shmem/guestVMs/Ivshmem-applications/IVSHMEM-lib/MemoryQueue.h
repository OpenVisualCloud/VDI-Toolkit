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
 //! \file     MemoryQueue.h
 //! \brief    Define a circular queue management for IVSHMEM.
 //!


#ifndef _MEMORYQUEUE_H_
#define _MEMORYQUEUE_H_

#include <cstring>
#include <type_traits>
#include <windows.h>

#include "../Common/Tools.h"

class MemoryQueue
{
public:
	MemoryQueue(BYTE* ptr, UINT64 size, UINT64 offset)
	{
		m_queue = ptr;
		m_totalSize = size;
		m_offset = offset;
	}
	virtual ~MemoryQueue()
	{
		m_queue = nullptr;
		m_totalSize = 0;
		m_offset = 0;
	}

	template <typename T>
	inline void Write(T data, size_t size)
	{
		if (m_queue == nullptr) return;

		UINT64 remains = m_totalSize - m_offset;
		if (remains < size) // segment writing
		{
			if (std::is_pointer<T>::value) {
				memcpy_s(m_queue + m_offset, remains, reinterpret_cast<const void* const>(data), remains);
				memcpy_s(m_queue, size - remains, reinterpret_cast<const void* const>(data + remains), size - remains);
			}
			else {
				memcpy_s(m_queue + m_offset, remains, reinterpret_cast<const void* const>(&data), remains);
				memcpy_s(m_queue, size - remains, reinterpret_cast<const void* const>(&data + remains), size - remains);
			}
			m_offset = size - remains;
		}
		else { // consecutive writing
			if (std::is_pointer<T>::value) {
				memcpy_s(m_queue + m_offset, size, reinterpret_cast<const void* const>(data), size);
			}
			else {
				memcpy_s(m_queue + m_offset, size, reinterpret_cast<const void* const>(&data), size);
			}
			m_offset = (m_offset + size) % m_totalSize;
		}
	}

	template <typename T>
	inline void WriteWithOffset(T data, size_t size, size_t offset)
	{
		if (m_queue == nullptr) return;

		UINT64 remains = m_totalSize - offset;
		if (remains < size) // segment writing
		{
			if (std::is_pointer<T>::value) {
				memcpy_s(m_queue + offset, remains, reinterpret_cast<const void* const>(data), remains);
				memcpy_s(m_queue, size - remains, reinterpret_cast<const void* const>(data + remains), size - remains);
			}
			else {
				memcpy_s(m_queue + offset, remains, reinterpret_cast<const void* const>(&data), remains);
				memcpy_s(m_queue, size - remains, reinterpret_cast<const void* const>(&data + remains), size - remains);
			}
		}
		else { // consecutive writing
			if (std::is_pointer<T>::value) {
				memcpy_s(m_queue + offset, size, reinterpret_cast<const void* const>(data), size);
			}
			else {
				memcpy_s(m_queue + offset, size, reinterpret_cast<const void* const>(&data), size);
			}
		}
	}

	inline UINT64 GetOffset() { return m_offset; }

private:
	BYTE* m_queue;
	UINT64 m_totalSize;
	UINT64 m_offset;
};

#endif // _MEMORYQUEUE_H_
