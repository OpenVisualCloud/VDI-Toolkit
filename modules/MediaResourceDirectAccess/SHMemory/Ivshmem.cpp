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

 //!
 //! \file     Ivshmem.cpp
 //! \brief    Implement class for Ivshmem driver library.
 //!

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <winioctl.h>
#include <SetupAPI.h>
#include <tchar.h>
#include <string>
#include "Public.h"
#include "Ivshmem.h"

VDI_NS_BEGIN

#define MAXINTERFACENUM 10

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

MRDAStatus Ivshmem::Init(const UINT32 slot_number)
{
	HDEVINFO devInfoSet;
	PSP_DEVICE_INTERFACE_DETAIL_DATA devInfDetailData = NULL;

	devInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE);
	SP_DEVICE_INTERFACE_DATA devInfData;
	ZeroMemory(&devInfData, sizeof(SP_DEVICE_INTERFACE_DATA));
	devInfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	SP_DEVINFO_DATA devInfoData;
    ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DWORD index = 0;
    bool deviceFound = false;
	while (SetupDiEnumDeviceInfo(devInfoSet, index, &devInfoData)) {
        DWORD reqSize = 0;
        DWORD dataType;
        TCHAR locationPath[256];
		// ui number match
		bool ret = SetupDiGetDeviceRegistryProperty(devInfoSet, &devInfoData, SPDRP_LOCATION_PATHS,
												&dataType, (PBYTE)locationPath, sizeof(locationPath),
												&reqSize);
		if (ret)
		{
			if (!checkPCIString(slot_number, locationPath))
			{
				index++;
				continue;
			}
			// MRDA_LOG(LOG_INFO, "locationPath %s slot number %d", locationPath, slot_number);
			int interfaceIndex    = 0;
			int maxInterfaceCount = MAXINTERFACENUM;
			// ivshmem guid match to get devInfData
			while (interfaceIndex < maxInterfaceCount &&
				   SetupDiEnumDeviceInterfaces(devInfoSet, &devInfoData,
											   &GUID_DEVINTERFACE_IVSHMEM, interfaceIndex++,
											   &devInfData)) {
				// MRDA_LOG(LOG_INFO, "Device interface found.");
				deviceFound = true;
				break;
			}
			if (deviceFound)
				break;
		}
		index++;
    }

	DWORD reqSize = 0;
	SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInfData, NULL, 0, &reqSize, NULL);
	if (!reqSize)
	{
		MRDA_LOG(LOG_ERROR, "Failed to SetupDiGetDeviceInterfaceDetail");
		return MRDA_STATUS_INVALID;
	}

	devInfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(reqSize);
	if (devInfDetailData == nullptr)
	{
		MRDA_LOG(LOG_ERROR, "Failed to allocate Interface detail data!");
		return MRDA_STATUS_INVALID;
	}
	ZeroMemory(devInfDetailData, reqSize);
	devInfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	if (!SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInfData, devInfDetailData, reqSize, NULL, NULL))
	{
		MRDA_LOG(LOG_ERROR, "Failed to SetupDiGetDeviceInterfaceDetail");
		return MRDA_STATUS_INVALID;
	}
	// Open device
	m_handle = CreateFile(devInfDetailData->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, 0);

	if (m_handle == INVALID_HANDLE_VALUE)
	{
		MRDA_LOG(LOG_ERROR, "Failed to create file! returned INVALID_HANDLE_VALUE");
		return MRDA_STATUS_INVALID;
	}

	if (devInfDetailData) free(devInfDetailData);

	SetupDiDestroyDeviceInfoList(devInfoSet);

	return MRDA_STATUS_SUCCESS;
}

MRDAStatus Ivshmem::DeInit()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_handle);
	}

	m_handle = nullptr;

	return MRDA_STATUS_SUCCESS;
}

MRDAStatus Ivshmem::Open()
{
	if (m_handle == INVALID_HANDLE_VALUE) return MRDA_STATUS_INVALID;

	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_REQUEST_SIZE, nullptr, 0, &m_size, sizeof(m_size), nullptr, nullptr))
	{
		MRDA_LOG(LOG_ERROR, "Failed to request ivshmem size!");
		return MRDA_STATUS_INVALID;
	}

	if (m_size == 0)
	{
		MRDA_LOG(LOG_ERROR, "ivshmem size is 0, incorrect value, please check again!");
		return MRDA_STATUS_INVALID;
	}

	IVSHMEM_MMAP_CONFIG map_config = {0};
	map_config.cacheMode = IVSHMEM_CACHE_NONCACHED;
	IVSHMEM_MMAP map;
	ZeroMemory(&map, sizeof(IVSHMEM_MMAP));
	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_REQUEST_MMAP, &map_config, sizeof(IVSHMEM_MMAP_CONFIG), &map, sizeof(IVSHMEM_MMAP), nullptr, nullptr))
	{
		MRDA_LOG(LOG_ERROR, "Failed to request ivshmem map!");
		return MRDA_STATUS_INVALID;
	}

	if (nullptr == map.ptr)
	{
		MRDA_LOG(LOG_ERROR, "map pointer is NULL, please check again!");
		return MRDA_STATUS_INVALID;
	}
	// memory pointer assignment
	memset(map.ptr, 0x00, m_size);
	m_memory = map.ptr;

	if (map.size != m_size)
	{
		MRDA_LOG(LOG_ERROR, "Map size is not equal to request size.");
		return MRDA_STATUS_INVALID;
	}

	return MRDA_STATUS_SUCCESS;
}

MRDAStatus Ivshmem::Close()
{
	if (nullptr == m_memory) return MRDA_STATUS_INVALID;

	if (!DeviceIoControl(m_handle, IOCTL_IVSHMEM_RELEASE_MMAP, nullptr, 0, nullptr, 0, nullptr, nullptr))
	{
		MRDA_LOG(LOG_ERROR, "Failed to release ivshmem map!");
		return MRDA_STATUS_INVALID;
	}

	m_memory = nullptr;
	m_offset = 0;
	m_size = 0;

	return MRDA_STATUS_SUCCESS;
}

VDI_NS_END