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
 //! \file     ShareData_ScreenCpature.h
 //! \brief    Define screen capture shared data structure and interfaces.
 //!

#ifndef _SHAREDATA_SCREENCAPTURE_H_
#define _SHAREDATA_SCREENCAPTURE_H_

#include "../../Common/ShareData.h"

enum class Format {
	RGBA = 0,
	YUV = 1,
	UNKNOWN,
};

class ShareDataSC : public ShareData
{
public:
	ShareDataSC():m_pitch(0), m_width(0), m_height(0), m_format(UINT(Format::UNKNOWN)), m_pts(0) {}
	virtual ~ShareDataSC()
	{
		m_pitch = 0;
		m_width = 0;
		m_height = 0;
		m_format = UINT(Format::UNKNOWN);
		m_pts = 0;
	}

	inline UINT GetPitch() { return m_pitch; }
	inline UINT GetWidth() { return m_width; }
	inline UINT GetHeight() { return m_height; }
	inline UINT GetFormat() { return m_format; }
	inline UINT64 GetPts() { return m_pts; }

	inline void SetPitch(UINT pitch) { m_pitch = pitch; }
	inline void SetWidth(UINT width) { m_width = width; }
	inline void SetHeight(UINT height) { m_height = height; }
	inline void SetFormat(UINT format) { m_format = format; }
	inline void SetPts(UINT64 pts) { m_pts = pts; }

private:
	UINT m_pitch;  // screen data pitch
	UINT m_width;  // screen data width
	UINT m_height; // screen data height
	UINT m_format; // screen pixel format
	UINT64 m_pts;  // capture fps
};

#endif // !_SHAREDATA_SCREENCAPTURE_H_