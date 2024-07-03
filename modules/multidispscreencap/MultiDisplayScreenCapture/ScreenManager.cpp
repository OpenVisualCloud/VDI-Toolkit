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

#include "ScreenManager.h"
using namespace DirectX;

DWORD WINAPI CaptureProc(void* Param);

//
// Constructor
//
ScreenManager::ScreenManager() : m_bSingleOutput(false),
                                 m_bSingleOutNumber(0),
                                 m_uOutputCount(0),
                                 m_uScreenCount(0),
                                 m_pScreenHandles(nullptr),
                                 m_pScreenInputParams(nullptr),
                                 m_pBufferQueues(nullptr),
                                 m_CaptureFps(30)
{
}

//
// Destructor
//
ScreenManager::~ScreenManager()
{
    Clean();
}

//
// Clean up resources
//
void ScreenManager::Clean()
{
    if (m_pScreenHandles)
    {
        for (UINT i = 0; i < m_uScreenCount; ++i)
        {
            if (m_pScreenHandles[i])
            {
                CloseHandle(m_pScreenHandles[i]);
            }
        }
        delete [] m_pScreenHandles;
        m_pScreenHandles = nullptr;
    }

    if (m_pBufferQueues) {
        delete[] m_pBufferQueues;
        m_pBufferQueues = nullptr;
    }

    if (m_pScreenInputParams)
    {
        for (UINT i = 0; i < m_uScreenCount; ++i)
        {
            CleanDx(&m_pScreenInputParams[i].DxRes);
        }
        delete [] m_pScreenInputParams;
        m_pScreenInputParams = nullptr;
    }

    m_uScreenCount = 0;
}

//
// Clean up DX_RESOURCES
//
void ScreenManager::CleanDx(_Inout_ DX_RESOURCES* Data)
{
    if (Data->Device)
    {
        Data->Device->Release();
        Data->Device = nullptr;
    }

    if (Data->Context)
    {
        Data->Context->Release();
        Data->Context = nullptr;
    }
}

//
// Initialize to Get Valid Adapters Count
//
SCREENCAP_STATUS ScreenManager::Initialize()
{
    // Calculate the valid OutputCount
    SCREENCAP_STATUS Ret = SCREENCAP_SUCCESSED;

    DX_RESOURCES DxRes;
    RtlZeroMemory(&DxRes, sizeof(DX_RESOURCES));
    Ret = InitializeDx(&DxRes);
    if (Ret != SCREENCAP_SUCCESSED)
    {
        return Ret;
    }
    Ret = GetAdapterCount(DxRes.Device);
    if (Ret != SCREENCAP_SUCCESSED)
    {
        return Ret;
    }

    return SCREENCAP_SUCCESSED;
}

//
// Set Single Screen to Capture
//
SCREENCAP_STATUS ScreenManager::SetSingleOuput(bool SingleOutput, UINT SingleOutNumber)
{
    m_bSingleOutput = SingleOutput;
    if (SingleOutNumber >= m_uOutputCount)
    {
        return SCREENCAP_FAILED;
    }
    else
    {
        m_bSingleOutNumber = SingleOutNumber;
        m_uScreenCount = 1;
    }
    return SCREENCAP_SUCCESSED;
}

//
// Start up Screen Capture Process
//
SCREENCAP_STATUS ScreenManager::Process(HANDLE TerminateThreadsEvent)
{
    if (m_uScreenCount == 0)
    {
        return SCREENCAP_FAILED;
    }

    m_pScreenHandles = new (std::nothrow) HANDLE[m_uScreenCount];
    m_pScreenInputParams = new (std::nothrow) ThreadInputParams[m_uScreenCount];
    m_pBufferQueues = new (std::nothrow) BufferQueue[m_uScreenCount];
    if (!m_pScreenHandles || !m_pScreenInputParams)
    {
        return SCREENCAP_FAILED;
    }

    // Create appropriate # of threads for duplication

    SCREENCAP_STATUS Ret = SCREENCAP_SUCCESSED;

    for (UINT i = 0; i < m_uScreenCount; ++i)
    {
        m_pScreenInputParams[i].TerminateThreadsEvent = TerminateThreadsEvent;
        m_pScreenInputParams[i].ScreenNumber = m_bSingleOutput ? m_bSingleOutNumber : i;
        m_pScreenInputParams[i].BufferQueueHandle = (HANDLE) & m_pBufferQueues[i];
        m_pScreenInputParams[i].CaptureFps = m_CaptureFps;

        RtlZeroMemory(&m_pScreenInputParams[i].DxRes, sizeof(DX_RESOURCES));
        Ret = InitializeDx(&m_pScreenInputParams[i].DxRes);
        if (Ret != SCREENCAP_SUCCESSED)
        {
            return Ret;
        }

        DWORD ThreadId;
        m_pScreenHandles[i] = CreateThread(nullptr, 0, CaptureProc, &m_pScreenInputParams[i], 0, &ThreadId);
        if (m_pScreenHandles[i] == nullptr)
        {
            return SCREENCAP_FAILED;
        }
    }

    return Ret;
}

//
// Initialize DX_RESOURCES
//
SCREENCAP_STATUS ScreenManager::InitializeDx(_Out_ DX_RESOURCES* Data)
{
    HRESULT hr = S_OK;

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    // Create device
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
                                D3D11_SDK_VERSION, &Data->Device, &FeatureLevel, &Data->Context);
        if (SUCCEEDED(hr))
        {
            // Device creation success, no need to loop anymore
            break;
        }
    }
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    return SCREENCAP_SUCCESSED;
}

//
// Get Valid Adapters Count
//
SCREENCAP_STATUS ScreenManager::GetAdapterCount(_In_ ID3D11Device* Device)
{
    HRESULT hr = S_OK;

    // Get DXGI resources
    IDXGIDevice* DxgiDevice = nullptr;
    hr = Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    IDXGIOutput* DxgiOutput = nullptr;

    // Count the valid DxgiOutputs
    hr = S_OK;
    for (int i = 0; SUCCEEDED(hr); ++i)
    {
        if (DxgiOutput)
        {
            DxgiOutput->Release();
            DxgiOutput = nullptr;
        }
        hr = DxgiAdapter->EnumOutputs(i, &DxgiOutput);
        if (DxgiOutput && (hr != DXGI_ERROR_NOT_FOUND))
        {
            m_uOutputCount++;
        }
    }

    DxgiAdapter->Release();
    DxgiAdapter = nullptr;

    if (m_uOutputCount == 0)
    {
        // No Output count, temp to retry
        return SCREENCAP_CONTINUED;
    }

    m_uScreenCount = m_uOutputCount;

    return SCREENCAP_SUCCESSED;
}

//
// Wait For Screen Capture Processes Terminate
//
void ScreenManager::WaitForThreadTermination()
{
    if (m_uScreenCount != 0)
    {
        WaitForMultipleObjectsEx(m_uScreenCount, m_pScreenHandles, TRUE, INFINITE, FALSE);
    }
}

//
// Get Output Count
//
UINT ScreenManager::GetOutputCount()
{
    return m_uOutputCount;
}

//
// Get Screen Count
//
UINT ScreenManager::GetScreenCount()
{
    return m_uScreenCount;
}

//
// Get Buffer Queues
//
BufferQueue *ScreenManager::GetBufferQueues()
{
    return m_pBufferQueues;
}

//
// Get DX_RESOURCES
//
DX_RESOURCES* ScreenManager::GetDXResource(UINT DisplayNumber)
{
    return &(m_pScreenInputParams[DisplayNumber].DxRes);
}

//
// Set Capture Fps
//
SCREENCAP_STATUS ScreenManager::SetCaptureFps(UINT fps)
{
    m_CaptureFps = fps;
    return SCREENCAP_SUCCESSED;
}

//
// Get Capture Fps
//
UINT ScreenManager::GetCaptureFps()
{
    return m_CaptureFps;
}

//
// Check is capture process terminated
//
bool ScreenManager::IsCaptureTerminated()
{
    bool CaptureTerminated = false;
    for (UINT i = 0; i < m_uScreenCount; ++i)
    {
        DWORD res = WaitForSingleObjectEx(m_pScreenHandles[i], 1000/m_CaptureFps, false);
        if (res != WAIT_TIMEOUT)
        {
            CaptureTerminated = true;
            break;
        }
    }
    return CaptureTerminated;
}