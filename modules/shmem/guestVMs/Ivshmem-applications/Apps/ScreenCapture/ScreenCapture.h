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
 //! \file     ScreenCpature.h
 //! \brief    Define screen capture class interfaces.
 //!

#ifndef _SCREENCAPTURE_H_
#define _SCREENCAPTURE_H_


#include "ShareData_ScreenCapture.h"
#include "../../IVSHMEM-lib/Ivshmem.h"
#include "../../IVSHMEM-lib/MemoryQueue.h"
#include "../../Common/Tools.h"
#include "../../Common/PerfProfile.h"

#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <iostream>
#include <chrono>

enum class SCCode {
	SUCCESS = 0,
	FAILED,
	TIMEOUT,
	NOTREADY,
	UNKNOWN,
};

enum class CAPTURESTATUS {
	INIT,
	RUNNING,
	STOPPED,
	DESTROY,
	UNKNOWN,
};

struct CaptureConfig {
	UINT32 fps;
	UINT32 framesNum;
	bool cpuAccessFlag;
};

class ScreenCapture
{
public:
	ScreenCapture();
	virtual ~ScreenCapture();
	SCCode Init(CaptureConfig config);
	SCCode RunScreenCapturing();
	SCCode Destroy();

	inline void SetStatus(CAPTURESTATUS status) { m_status = status; }
	inline CAPTURESTATUS GetStatus() { return m_status; }

private:
	SCCode DeviceInit();
	SCCode CaptureOneFrame();
	SCCode WriteOneFrameToSHMDevice();

private:
	CAPTURESTATUS m_status;
	CaptureConfig m_config;

	IDXGIOutputDuplication* m_pDXGIOutputDup;
	ID3D11Device* m_pDX11Dev;
	ID3D11DeviceContext* m_pDX11DevCtx;

	std::unique_ptr<ShareDataSC> m_shareData;

	std::unique_ptr<Ivshmem> m_memDev;

	std::unique_ptr<MemoryQueue> m_memQueue;

	std::unique_ptr<PerfProfile> m_perf;
};

#endif // !_SCREENCAPTURE_H_