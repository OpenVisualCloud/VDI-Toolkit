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
 //! \file     PerfProfile.h
 //! \brief    Define a performance profile helper class.
 //!

#ifndef _PERFPROFILE_H_
#define _PERFPROFILE_H_

#include <Windows.h>
#include <stdint.h>

#include "Tools.h"

struct Stats
{
	UINT64 framesNum;
	UINT64 avgDuration;
	UINT64 avgFps;
	UINT64 maxDuration;
	UINT64 minDuration;
};

class PerfProfile
{
public:
	PerfProfile(): m_framesNum(0), m_avgDuration(0), m_avgFps(0), m_maxDuration(0), m_minDuration(UINT64_MAX) {}
	virtual ~PerfProfile() {}
	void SetDuration(UINT64 dur)
	{
		if (dur > m_maxDuration) m_maxDuration = dur;
		else if (dur < m_minDuration) m_minDuration = dur;
		m_avgDuration = ((m_avgDuration * m_framesNum) + dur) / (m_framesNum + 1);
		m_avgFps = 1000 / m_avgDuration;
		m_framesNum++;
	}
	void PrintStats()
	{
		SHMEM_LOG("!!Perf data!!: total frames num: %lld, avg fps: %lld, max single duration: %lld, min single duration: %lld",
			m_framesNum, m_avgFps, m_maxDuration, m_minDuration);
	}
private:
	UINT64 m_framesNum;
	UINT64 m_avgDuration;
	UINT64 m_avgFps;
	UINT64 m_maxDuration;
	UINT64 m_minDuration;
};

#endif // !_PERFPROFILE_H_
