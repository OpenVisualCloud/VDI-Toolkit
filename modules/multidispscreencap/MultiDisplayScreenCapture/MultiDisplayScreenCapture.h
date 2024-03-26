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

 *
 */

#ifndef _MULTIDISPLAYSCREENCAPTURE_H_
#define _MULTIDISPLAYSCREENCAPTURE_H_

#include "framework.h"
#include "ThreadManager.h"


class MDSCLIB_API MultiDisplayScreenCapture {
public:
	MultiDisplayScreenCapture();
	~MultiDisplayScreenCapture();
public:
	DUPL_RETURN Init();
	DUPL_RETURN SetSingleDisplay(int SingleDispNumber);
	UINT GetDisplayCount();
	UINT GetOutputCount();
	BufferQueue *GetBufferQueues();
	DUPL_RETURN StartCaptureScreen();
	DUPL_RETURN TerminateCaptureScreen();
	DUPL_RETURN DeInit();
	DX_RESOURCES *GetDXResource(int thread);
	DUPL_RETURN SetCaptureFps(UINT fps);
	UINT GetCaptureFps();

private:
	THREADMANAGER m_ThreadMgr;

	HANDLE m_UnexpectedErrorEvent;
	HANDLE m_ExpectedErrorEvent;
	HANDLE m_TerminateThreadsEvent;
};

#endif