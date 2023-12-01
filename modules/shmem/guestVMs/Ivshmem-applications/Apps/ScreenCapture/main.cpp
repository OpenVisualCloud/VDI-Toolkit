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
 //! \file     main.cpp
 //! \brief    Main application to call Screen capture process.
 //!


#include "ScreenCapture.h"

struct InputParams {
	UINT32 fps;
	UINT32 frameNum;
};

void PrintHelp()
{
	printf("%s", "Usage: IVScreenCapture.exe [<options>]\n");
	printf("%s", "Options: \n");
	printf("%s", "    [--help, -h]             - print help README document. \n");
	printf("%s", "    [--fps, -f number]       - specifies screen capture fps. \n");
	printf("%s", "    [--framesNum, -n number] - specifies a total number of screen frames to capture.\n");
	printf("%s", "Examples: IVScreenCapture.exe --fps 30 -n 3000 \n");
}

bool ParseParams(int argc, wchar_t* argv[], InputParams* params)
{
	if (argc <= 1) {
		PrintHelp();
		return false;
	}

	for (auto i = 1; i < argc; i++)
	{
		if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--help") == 0)
		{
			PrintHelp();
			return false;
		}
		else if (wcscmp(argv[i], L"-f") == 0 || wcscmp(argv[i], L"--fps") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->fps = _wtoi(argv[i]);
		}
		else if (wcscmp(argv[i], L"-n") == 0 || wcscmp(argv[i], L"--framesNum") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->frameNum = _wtoi(argv[i]);
		}
		else
		{
			PrintHelp();
			return false;
		}
	}
	return true;
}

int wmain(int argc, wchar_t* argv[])
{
	InputParams inputParams = { 30, 30 }; // set default value
	if (false == ParseParams(argc, argv, &inputParams)) return -1;

	UINT32 fps = inputParams.fps;
	UINT32 total_num = inputParams.frameNum;

	std::unique_ptr<ScreenCapture> screenCaptureImpl = std::make_unique<ScreenCapture>();

	CaptureConfig config = { fps, total_num, true };

	if (screenCaptureImpl->Init(config) != SCCode::SUCCESS)
	{
		SHMEM_LOG("Error: Failed to init screen capture in application!");
		return -1;
	}

	screenCaptureImpl->SetStatus(CAPTURESTATUS::RUNNING);

	if (screenCaptureImpl->RunScreenCapturing() != SCCode::SUCCESS)
	{
		SHMEM_LOG("Error: Failed to run screen capturing in application!");
		return -1;
	}

	if (screenCaptureImpl->Destroy() != SCCode::SUCCESS)
	{
		SHMEM_LOG("Error: Failed to destroy screen capture in application!");
		return -1;
	}
	SHMEM_LOG("Debug: Destroy done");
	return 0;
}