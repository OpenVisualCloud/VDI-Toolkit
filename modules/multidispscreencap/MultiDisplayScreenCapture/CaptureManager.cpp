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

#include <chrono>

#include "CaptureManager.h"

//
// Constructor
//
CaptureManager::CaptureManager() : m_pDXGIDupl(nullptr),
                                   m_pCapturedTexture(nullptr),
                                   m_uScreenNumber(0),
                                   m_pDevice(nullptr),
                                   m_pContext(nullptr)
{
    RtlZeroMemory(&m_sOutputDesc, sizeof(m_sOutputDesc));
}

//
// Destructor
//
CaptureManager::~CaptureManager()
{
    if (m_pDXGIDupl)
    {
        m_pDXGIDupl->Release();
        m_pDXGIDupl = nullptr;
    }

    if (m_pCapturedTexture)
    {
        m_pCapturedTexture->Release();
        m_pCapturedTexture = nullptr;
    }

    if (m_pDevice)
    {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }

    if (m_pContext)
    {
        m_pContext->Release();
        m_pContext = nullptr;
    }
}

//
// Initialize
//
SCREENCAP_STATUS CaptureManager::Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, UINT Output)
{
    m_uScreenNumber = Output;

    // Take a reference on the device and context
    m_pDevice = Device;
    m_pDevice->AddRef();

    m_pContext = Context;
    m_pContext->AddRef();

    IDXGIDevice* DxgiDevice = nullptr;
    HRESULT hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
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
        printf("Failed to GetParent of DxgiDevice\n");
        return SCREENCAP_FAILED;
    }

    IDXGIOutput* DxgiOutput = nullptr;
    hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    DxgiOutput->GetDesc(&m_sOutputDesc);

    IDXGIOutput1* DxgiOutput1 = nullptr;
    hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
    DxgiOutput->Release();
    DxgiOutput = nullptr;
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    hr = DxgiOutput1->DuplicateOutput(m_pDevice, &m_pDXGIDupl);
    DxgiOutput1->Release();
    DxgiOutput1 = nullptr;
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            printf("Error: DXGI is not currently availble, please check if other binary is in use.\n");
            return SCREENCAP_FAILED;
        }
        return SCREENCAP_FAILED;
    }

    return SCREENCAP_SUCCESSED;
}

//
// Capture Screen Process
//
SCREENCAP_STATUS CaptureManager::CaptureScreen(CapturedData* DataCaptured, UINT TimeOutInMs)
{
    IDXGIResource* DXGIResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO DXGIOutFrameInfo;
    HRESULT hr = m_pDXGIDupl->AcquireNextFrame(TimeOutInMs, &DXGIOutFrameInfo, &DXGIResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        return SCREENCAP_CONTINUED;
    }

    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    if (m_pCapturedTexture)
    {
        m_pCapturedTexture->Release();
        m_pCapturedTexture = nullptr;
    }

    hr = DXGIResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_pCapturedTexture));
    DXGIResource->Release();
    DXGIResource = nullptr;
    if (FAILED(hr))
    {
        printf("Error: Failed to capture current frame\n");
        return SCREENCAP_FAILED;
    }

    DataCaptured->AcquiredTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    D3D11_TEXTURE2D_DESC desc;
    if (m_pCapturedTexture)
    {
        m_pCapturedTexture->GetDesc(&desc);
    }
    else
    {
        printf("Error: Failed to get desc of the captured texture\n");
        return SCREENCAP_FAILED;
    }

    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    hr = m_pDevice->CreateTexture2D(&desc, nullptr, &DataCaptured->CapturedTexture);
    if (FAILED(hr))
    {
        printf("Error: Failed to create the staging texture\n");
        return SCREENCAP_FAILED;
    }

    assert(DataCaptured->CapturedTexture);

    m_pContext->CopyResource(DataCaptured->CapturedTexture, m_pCapturedTexture);

    DataCaptured->FrameInfo = DXGIOutFrameInfo;

    return SCREENCAP_SUCCESSED;
}

//
// Release frame
//
SCREENCAP_STATUS CaptureManager::Release()
{
    if (m_pCapturedTexture)
    {
        m_pCapturedTexture->Release();
        m_pCapturedTexture = nullptr;
    }

    HRESULT hr = m_pDXGIDupl->ReleaseFrame();
    if (FAILED(hr))
    {
        return SCREENCAP_FAILED;
    }

    return SCREENCAP_SUCCESSED;
}

//
// Gets output desc into DescPtr
//
void CaptureManager::GetOutputDesc(DXGI_OUTPUT_DESC* DescPtr)
{
    *DescPtr = m_sOutputDesc;
}
