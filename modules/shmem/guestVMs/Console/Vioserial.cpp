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
 //! \file     Vioserial.cpp
 //! \brief    Implement virtio serial APIs.
 //!

#include "Vioserial.h"

VioserDev::~VioserDev()
{
	if (m_dev != INVALID_HANDLE_VALUE) {
		CloseHandle(m_dev);
		m_dev = INVALID_HANDLE_VALUE;
	}
}

VIOSERStatus VioserDev::Init()
{
    HDEVINFO hardwareDevInfo = nullptr;
    SP_DEVICE_INTERFACE_DATA devInterfaceData{};
    PSP_DEVICE_INTERFACE_DETAIL_DATA devInterfaceDetailedData = NULL;
    ULONG length = 0;
    BOOL res = false;

    hardwareDevInfo = SetupDiGetClassDevs(&GUID_VIOSERIAL_PORT, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (hardwareDevInfo == INVALID_HANDLE_VALUE)
    {
        CON_LOG("Failed to get hardware device information!");
        return VIOSERStatus::FAILED;
    }

    ZeroMemory(&devInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    res = SetupDiEnumDeviceInterfaces(hardwareDevInfo, 0, &GUID_VIOSERIAL_PORT, 0, &devInterfaceData);

    if (res == FALSE)
    {
        CON_LOG("Cannot get enumerate device interfaces.");
        SetupDiDestroyDeviceInfoList(hardwareDevInfo);
        return VIOSERStatus::FAILED;
    }

    ULONG reqLength = 0;
    SetupDiGetDeviceInterfaceDetail(hardwareDevInfo, &devInterfaceData, NULL, 0, &reqLength, NULL);
    if (!reqLength)
    {
        CON_LOG("Failed to SetupDiGetDeviceInterfaceDetail!");
        return VIOSERStatus::FAILED;
    }

    devInterfaceDetailedData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, reqLength);

    if (devInterfaceDetailedData == NULL)
    {
        CON_LOG("Failed to allocate memory for device detailed data.");
        SetupDiDestroyDeviceInfoList(hardwareDevInfo);
        return VIOSERStatus::FAILED;
    }

    ZeroMemory(devInterfaceDetailedData, reqLength);
    devInterfaceDetailedData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    res = SetupDiGetDeviceInterfaceDetail(hardwareDevInfo, &devInterfaceData, devInterfaceDetailedData, reqLength, NULL, NULL);

    if (res == FALSE)
    {
        CON_LOG("Failed to get device interface details.");
        SetupDiDestroyDeviceInfoList(hardwareDevInfo);
        LocalFree(devInterfaceDetailedData);
        return VIOSERStatus::FAILED;
    }

    m_dev = CreateFile(devInterfaceDetailedData->DevicePath, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_dev == INVALID_HANDLE_VALUE)
    {
        CON_LOG("Failed to open vioserial device!");
        return VIOSERStatus::FAILED;
    }

    if (devInterfaceDetailedData) LocalFree(devInterfaceDetailedData);

    SetupDiDestroyDeviceInfoList(hardwareDevInfo);

    return VIOSERStatus::SUCCESS;
}

VIOSERStatus VioserDev::Read(PVOID buf, size_t* size)
{
    if (buf == nullptr || size == nullptr)
    {
        CON_LOG("Input buffer or size is empty in read operation!");
        return VIOSERStatus::FAILED;
    }
    bool res = FALSE;
    DWORD ret_size = 0;
    DWORD input_size = *size;

    memset(buf, '\0', input_size);

    res = ReadFile(m_dev, buf, input_size, &ret_size, NULL);
    if (!res)
    {
        CON_LOG("Failed to read vioserial device: Error %d.", GetLastError());
        return VIOSERStatus::FAILED;
    }
    else
    {
        CON_LOG("buf: %s, size %lu", reinterpret_cast<char*>(buf), ret_size);
        *size = ret_size;
        return VIOSERStatus::SUCCESS;
    }
}

VIOSERStatus VioserDev::Write(PVOID buf, size_t* size)
{
    if (buf == nullptr || size == nullptr)
    {
        CON_LOG("Input buffer or size is empty in write operation!");
        return VIOSERStatus::FAILED;
    }

    BOOL res = FALSE;
    ULONG ret_size = 0;
    DWORD input_size = *size;

    res = WriteFile(m_dev, buf, input_size, &ret_size, NULL);

    if (!res)
    {
        CON_LOG("Failed to write vioserial device: Error %d.", GetLastError());
        return VIOSERStatus::FAILED;
    }
    else if (ret_size != input_size)
    {
        CON_LOG("Error: Write vioserial device size incorrect. written = 0x%x, expected = 0x%x", ret_size, input_size);
        *size = ret_size;
        return VIOSERStatus::FAILED;
    }
    else return VIOSERStatus::SUCCESS;
}