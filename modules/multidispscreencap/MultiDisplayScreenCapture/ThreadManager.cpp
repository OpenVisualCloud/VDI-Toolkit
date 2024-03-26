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

#include "ThreadManager.h"
using namespace DirectX;

DWORD WINAPI DDProc(_In_ void* Param);

THREADMANAGER::THREADMANAGER() : m_SingleOutput(false),
                                 m_SingleOutNumber(0),
                                 m_OutputCount(0),
                                 m_ThreadCount(0),
                                 m_ThreadHandles(nullptr),
                                 m_ThreadData(nullptr),
                                 m_BufferQueues(nullptr),
                                 m_CaptureFps(30)
{
    RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));
}

THREADMANAGER::~THREADMANAGER()
{
    Clean();
}

//
// Clean up resources
//
void THREADMANAGER::Clean()
{
    if (m_PtrInfo.PtrShapeBuffer)
    {
        delete [] m_PtrInfo.PtrShapeBuffer;
        m_PtrInfo.PtrShapeBuffer = nullptr;
    }
    RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));

    if (m_ThreadHandles)
    {
        for (UINT i = 0; i < m_ThreadCount; ++i)
        {
            if (m_ThreadHandles[i])
            {
                CloseHandle(m_ThreadHandles[i]);
            }
        }
        delete [] m_ThreadHandles;
        m_ThreadHandles = nullptr;
    }

    if (m_ThreadData)
    {
        for (UINT i = 0; i < m_ThreadCount; ++i)
        {
            CleanDx(&m_ThreadData[i].DxRes);
        }
        delete [] m_ThreadData;
        m_ThreadData = nullptr;
    }

    if (m_BufferQueues)
    {
        for (UINT i = 0; i < m_ThreadCount; ++i)
        {
            // !!! Need to clear m_BufferQueues[i];
        }
        delete[] m_BufferQueues;
        m_BufferQueues = nullptr;
    }

    m_ThreadCount = 0;
}

//
// Clean up DX_RESOURCES
//
void THREADMANAGER::CleanDx(_Inout_ DX_RESOURCES* Data)
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
DUPL_RETURN THREADMANAGER::Initialize()
{
    // Calculate the valid OutputCount
    DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;

    DX_RESOURCES DxRes;
    RtlZeroMemory(&DxRes, sizeof(DX_RESOURCES));
    Ret = InitializeDx(&DxRes);
    if (Ret != DUPL_RETURN_SUCCESS)
    {
        return Ret;
    }
    Ret = GetAdapterCount(DxRes.Device);
    if (Ret != DUPL_RETURN_SUCCESS)
    {
        return Ret;
    }

    return DUPL_RETURN_SUCCESS;
}

DUPL_RETURN THREADMANAGER::SetSingleOuput(bool SingleOutput, UINT SingleOutNumber)
{
    m_SingleOutput = SingleOutput;
    if (SingleOutNumber >= m_OutputCount)
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }
    else
    {
        m_SingleOutNumber = SingleOutNumber;
        m_ThreadCount = 1;
    }
    return DUPL_RETURN_SUCCESS;
}

//
// Start up threads for DDA
//
DUPL_RETURN THREADMANAGER::Process(HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent)
{
    if (m_ThreadCount == 0)
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    m_ThreadHandles = new (std::nothrow) HANDLE[m_ThreadCount];
    m_ThreadData = new (std::nothrow) THREAD_DATA[m_ThreadCount];
    m_BufferQueues = new (std::nothrow) BufferQueue[m_ThreadCount];
    if (!m_ThreadHandles || !m_ThreadData)
    {
        return ProcessFailure(nullptr, L"Failed to allocate array for threads", L"Error", E_OUTOFMEMORY);
    }

    // Create appropriate # of threads for duplication

    DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;

    for (UINT i = 0; i < m_ThreadCount; ++i)
    {
        m_ThreadData[i].UnexpectedErrorEvent = UnexpectedErrorEvent;
        m_ThreadData[i].ExpectedErrorEvent = ExpectedErrorEvent;
        m_ThreadData[i].TerminateThreadsEvent = TerminateThreadsEvent;
        m_ThreadData[i].Output = m_SingleOutput ? m_SingleOutNumber : i;
        m_ThreadData[i].PtrInfo = &m_PtrInfo;
        m_ThreadData[i].BufferQueueHandle = (HANDLE) & m_BufferQueues[i];
        m_ThreadData[i].CaptureFps = m_CaptureFps;

        RtlZeroMemory(&m_ThreadData[i].DxRes, sizeof(DX_RESOURCES));
        Ret = InitializeDx(&m_ThreadData[i].DxRes);
        if (Ret != DUPL_RETURN_SUCCESS)
        {
            return Ret;
        }

        DWORD ThreadId;
        m_ThreadHandles[i] = CreateThread(nullptr, 0, DDProc, &m_ThreadData[i], 0, &ThreadId);
        if (m_ThreadHandles[i] == nullptr)
        {
            return ProcessFailure(nullptr, L"Failed to create thread", L"Error", E_FAIL);
        }
    }

    return Ret;
}

//
// Get DX_RESOURCES
//
DUPL_RETURN THREADMANAGER::InitializeDx(_Out_ DX_RESOURCES* Data)
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
        return ProcessFailure(nullptr, L"Failed to create device in InitializeDx", L"Error", hr);
    }

    return DUPL_RETURN_SUCCESS;
}

//
// Get Valid Adapters Count
//
DUPL_RETURN THREADMANAGER::GetAdapterCount(_In_ ID3D11Device* Device)
{
    HRESULT hr = S_OK;

    // Get DXGI resources
    IDXGIDevice* DxgiDevice = nullptr;
    hr = Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for DXGI Device", L"Error", hr);
    }

    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(Device, L"Failed to get parent DXGI Adapter", L"Error", hr, SystemTransitionsExpectedErrors);
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
            m_OutputCount++;
        }
    }

    DxgiAdapter->Release();
    DxgiAdapter = nullptr;

    if (m_OutputCount == 0)
    {
        // We could not find any outputs, the system must be in a transition so return expected error
        // so we will attempt to recreate
        return DUPL_RETURN_ERROR_EXPECTED;
    }

    m_ThreadCount = m_OutputCount;

    return DUPL_RETURN_SUCCESS;
}

//
// Getter for the PTR_INFO structure
//
PTR_INFO* THREADMANAGER::GetPointerInfo()
{
    return &m_PtrInfo;
}

//
// Waits infinitely for all spawned threads to terminate
//
void THREADMANAGER::WaitForThreadTermination()
{
    if (m_ThreadCount != 0)
    {
        WaitForMultipleObjectsEx(m_ThreadCount, m_ThreadHandles, TRUE, INFINITE, FALSE);
    }
}

UINT THREADMANAGER::GetOutputCount()
{
    return m_OutputCount;
}

UINT THREADMANAGER::GetThreadCount()
{
    return m_ThreadCount;
}

BufferQueue *THREADMANAGER::GetBufferQueues()
{
    return m_BufferQueues;
}

DX_RESOURCES* THREADMANAGER::GetDXResource(int thread)
{
    return &(m_ThreadData[thread].DxRes);
}

DUPL_RETURN THREADMANAGER::SetCaptureFps(UINT fps)
{
    m_CaptureFps = fps;
    return DUPL_RETURN_SUCCESS;
}

UINT THREADMANAGER::GetCaptureFps()
{
    return m_CaptureFps;
}