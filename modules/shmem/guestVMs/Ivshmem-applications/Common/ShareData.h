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
 //! \file     ShareData.h
 //! \brief    Define generic shared data structure and interfaces.
 //!

#ifndef _SHAREDATA_H_
#define _SHAREDATA_H_

#include <Windows.h>

enum class DataType {
	VIDEO = 0,
	TEXT = 1,
	UNKNOWN,
};

class ShareData
{
public:
	ShareData():m_dataType(UINT(DataType::UNKNOWN)), m_isWriteDone(0), m_size(0), m_realData(nullptr) {}
	virtual ~ShareData()
	{
		m_dataType = 0;
		m_isWriteDone = 0;
		m_size = 0;
		m_realData = nullptr;
	}

	inline UINT GetDataType() { return m_dataType; }
	inline UINT GetWriteFlag() { return m_isWriteDone; }
	inline UINT64 GetDataSize() { return m_size; }
	inline BYTE* GetRealData() { return m_realData; }

	inline void SetDataType(UINT type) { m_dataType = type; }
	inline void SetWriteFlag(UINT flag) { m_isWriteDone = flag; }
	inline void SetDataSize(UINT64 size) { m_size = size; }
	inline void SetRealData(BYTE* data) { m_realData = data; }

protected:
	// data type
	UINT m_dataType;    // type for the share data.
	// sync flags
	UINT m_isWriteDone; // if the write operation is completed. e.g. 0: not done, 1: done.
	// real data
	UINT64 m_size;      // the real data size
	BYTE* m_realData;   // the real data pointer
};

#endif // !_SHAREDATA_H_