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
 //! \file     Ivshmem.cpp
 //! \brief    Implement class for Ivshmem driver library.
 //!

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <winioctl.h>
#include <SetupAPI.h>

#include "../Common/Tools.h"
#include "../Common/Public.h"
#include "Ivshmem.h"

Ivshmem::Ivshmem()
{
	m_memory = nullptr;
	m_offset = 0;
	m_size = 0;
	m_handle = nullptr;
}

Ivshmem::~Ivshmem()
{
	Close();
	DeInit();
}

IVSHMEMStatus Ivshmem::Init()
{
	HDEVINFO devInfoSet;

	PSP_DEVICE_INTERFACE_DETAIL_DATA devInfDetailData = NULL;

	devInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE);
	SP_DEVICE_INTERFACE_DATA devInfData;
	ZeroMemory(&devInfData, sizeof(SP_DEVICE_INTERFACE_DATA));
	devInfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (SetupDiEnumDeviceInterfaces(devInfoSet, NULL, &GUID_DEVINTERFACE_IVSHMEM, 0, &devInfData) == FALSE)
	{
		DWORD error = GetLastError();
		if (error == ERROR_NO_MORE_ITEMS)
		{
			SHMEM_LOG("Failed to enumerate the device, please check the device!");
			return IVSHMEMStatus::FAILED;
		}

		SHMEM_LOG("Failed to SetupDiEnumDeviceInterfaces");
		return IVSHMEMStatus::FAILED;
	}

	DWORD reqSize = 0;
	SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInfData, NULL, 0, &reqSize, NULL);
	if (!reqSize)
	{
		SHMEM_LOG("Failed to SetupDiGetDeviceInterfaceDetail");
		return IVSHMEMStatus::FAILED;
	}

	devInfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(reqSize);
	if (devInfDetailData == nullptr)
	{
		SHMEM_LOG("Failed to allocate Interface detail data!");
		return IVSHMEMStatus::FAILED;
	}
	ZeroMemory(devInfDetailData, reqSize);
	devInfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	if (!SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInfData, devInfDetailData, reqSize, NULL, NULL))
	{
		SHMEM_LOG("Failed to SetupDiGetDeviceInterfaceDetail");
		return IVSHMEMStatus::FAILED;
	}
	// Open device
	m_handle = CreateFile(devInfDetailData->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, 0);

	if (m_handle == INVALID_HANDLE_VALUE)
	{
		SHMEM_LOG("Failed to create file! returned INVALID_HANDLE_VALUE");
		return IVSHMEMStatus::FAILED;
	}

	if (devInfDetailData) free(devInfDetailData);

	SetupDiDestroyDeviceInfoList(devInfoSet);

	return IVSHMEMStatus::SUCCESS;
}

IVSHMEMStatus Ivshmem::DeInit()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_handle);
	}

	m_handle = nullptr;

	return IVSHMEMStatus::SUCCESS;
}

IVSHMEMStatus Ivshmem::Open()
{
	if (m_handle == INVALID_HANDLE_VALUE) return IVSHMEMStatus::FAILED;

	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_REQUEST_SIZE, nullptr, 0, &m_size, sizeof(m_size), nullptr, nullptr))
	{
		SHMEM_LOG("Failed to request ivshmem size!");
		return IVSHMEMStatus::FAILED;
	}

	if (m_size == 0)
	{
		SHMEM_LOG("ivshmem size is 0, incorrect value, please check again!");
		return IVSHMEMStatus::FAILED;
	}

	IVSHMEM_MMAP_CONFIG map_config = {0};
	map_config.cacheMode = IVSHMEM_CACHE_NONCACHED;
	IVSHMEM_MMAP map;
	ZeroMemory(&map, sizeof(IVSHMEM_MMAP));
	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_REQUEST_MMAP, &map_config, sizeof(IVSHMEM_MMAP_CONFIG), &map, sizeof(IVSHMEM_MMAP), nullptr, nullptr))
	{
		SHMEM_LOG("Failed to request ivshmem map!");
		return IVSHMEMStatus::FAILED;
	}

	if (nullptr == map.ptr)
	{
		SHMEM_LOG("map pointer is NULL, please check again!");
		return IVSHMEMStatus::FAILED;
	}
	// memory pointer assignment
	memset(map.ptr, 0x00, m_size);
	m_memory = map.ptr;

	if (map.size != m_size)
	{
		SHMEM_LOG("Map size is not equal to request size.");
		return IVSHMEMStatus::FAILED;
	}

	return IVSHMEMStatus::SUCCESS;
}

IVSHMEMStatus Ivshmem::Close()
{
	if (nullptr == m_memory) return IVSHMEMStatus::FAILED;

	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_RELEASE_MMAP, nullptr, 0, nullptr, 0, nullptr, nullptr))
	{
		SHMEM_LOG("Failed to release ivshmem map!");
		return IVSHMEMStatus::FAILED;
	}

	m_memory = nullptr;
	m_offset = 0;
	m_size = 0;

	return IVSHMEMStatus::SUCCESS;
}
