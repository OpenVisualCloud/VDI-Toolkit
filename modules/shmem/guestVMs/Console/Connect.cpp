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
 //! \file     Connect.cpp
 //! \brief    Implement guest VM connect application
 //!

#include "Vioserial.h"
#include <memory>
#include <string>

constexpr size_t IOSIZE = 1024;

struct SCParams
{
	UINT32 fps;
	UINT32 frames;
};

enum class APPType
{
	SC = 0,//Screen Capture
	UNKNOWN
};


//For example: SC:fps:xxx:frames:xxx
void* ParseInputBuffer(char* buf, APPType &type)
{
	if (buf == nullptr) return nullptr;

	char* nextToken = nullptr;

	char* token = strtok_s(buf, ":", &nextToken);

	if (strcmp(token, "SC") == 0)
	{
		type = APPType::SC;

		token = strtok_s(nullptr, ":", &nextToken);

		std::shared_ptr<SCParams> scParams = std::make_shared<SCParams>();

		while (token != nullptr)
		{
			if (strcmp(token, "fps") == 0)
			{
				token = strtok_s(nullptr, ":", &nextToken);
				scParams->fps = atoi(token);
			}
			else if (strcmp(token, "frames") == 0)
			{
				token = strtok_s(nullptr, ":", &nextToken);
				scParams->frames = atoi(token);
			}
			token = strtok_s(nullptr, ":", &nextToken);
		}

		return scParams.get();
	}
	// ... other applications ...
	return nullptr;
}

void Execute(const char* command, const char* args) {
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::string fullCommand = std::string(command) + " " + std::string(args);

	// create child process
	if (!CreateProcessA(NULL, (LPSTR)fullCommand.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CON_LOG("Failed to execute command: %s", fullCommand.c_str());
		return;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

bool WaitAndSetupConnection(std::shared_ptr<VioserDev> dev)
{
	if (dev.get() == nullptr) return false;

    size_t input_size = IOSIZE;

    char* buf = (char*)malloc(input_size);
    if (buf == nullptr) return false;

    memset(buf, 0, input_size);

	//1. wait for connection
    while (dev->Read(buf, &input_size) != VIOSERStatus::SUCCESS)
    {
        Sleep(1000);
    }
	//2. Parse the buf obtained
	APPType type = APPType::UNKNOWN;
	void *params = ParseInputBuffer(buf, type);

	if (type == APPType::SC)
	{
		SCParams* scParams = static_cast<SCParams*>(params);
		if (scParams == nullptr)
		{
			free(buf);
			return false;
		}

		UINT32 fps = scParams->fps;
		UINT32 frames = scParams->frames;
		CON_LOG("fps: %d, frames: %d", fps, frames);

		//3. Write "Ready" to host
		const char* readyStr = "Ready";
		size_t len = strlen(readyStr);
		if (dev->Write((PVOID)readyStr, &len) != VIOSERStatus::SUCCESS)
		{
			CON_LOG("Failed to write ready string to host!");
			free(buf);
			return false;
		}
		CON_LOG("Write ready string to host!");
		//4. Launch the corresponding cmd
		std::string in_args = "--fps " + std::to_string(fps) + " -n " + std::to_string(frames);
		CON_LOG("Success to start screen capture application!");
		Execute("..\\..\\..\\ivshmem-applications\\x64\\Release\\IVScreenCapture.exe", in_args.c_str());
	}

    return true;
}

int wmain(int argc, wchar_t* argv[])
{
	std::shared_ptr<VioserDev> vioserDev = std::make_shared<VioserDev>();

	if (VIOSERStatus::SUCCESS != vioserDev->Init())
	{
		CON_LOG("Failed to init vioserial device!");
		return -1;
	}

	bool res = WaitAndSetupConnection(vioserDev);

	if (res == true)
	{
		CON_LOG("Finish guest task!");
		return 0;
	}
	else return -1;
}