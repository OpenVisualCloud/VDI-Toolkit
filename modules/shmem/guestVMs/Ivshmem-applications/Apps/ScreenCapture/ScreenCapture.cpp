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
 //! \file     ScreenCpature.cpp
 //! \brief    screen capture class implementation.
 //!

#include "ScreenCapture.h"

ScreenCapture::ScreenCapture()
{
	m_status = CAPTURESTATUS::UNKNOWN;
	m_config = { 0,0,true };
	m_pDXGIOutputDup = nullptr;
	m_pDX11Dev = nullptr;
	m_pDX11DevCtx = nullptr;
	m_shareData = nullptr;
	m_memDev = nullptr;
	m_memQueue = nullptr;
	m_perf = nullptr;
}

ScreenCapture::~ScreenCapture()
{
	Destroy();
	m_config = { 0,0,true };
	m_status = CAPTURESTATUS::DESTROY;

	m_memDev->Close();
	m_memDev->DeInit();
}

SCCode ScreenCapture::Init(CaptureConfig config)
{
	m_config = config;

	m_memDev = std::make_unique<Ivshmem>();
	if (m_memDev->Init() != IVSHMEMStatus::SUCCESS)
	{
		SHMEM_LOG("Error: Failed to init ivshmem device!");
		return SCCode::FAILED;
	}
	if (m_memDev->Open() != IVSHMEMStatus::SUCCESS)
	{
		SHMEM_LOG("Error: Failed to open and request mmap!");
		return SCCode::FAILED;
	}

	BYTE* ptr = (BYTE*)m_memDev->GetMemory();
	UINT64 totalSize = m_memDev->GetSize();
	UINT64 offset = m_memDev->GetOffset();
	m_memQueue = std::make_unique<MemoryQueue>(ptr, totalSize, offset);
	m_shareData = std::make_unique<ShareDataSC>();
	m_perf = std::make_unique<PerfProfile>();

	SCCode res = DeviceInit();
	if (res == SCCode::SUCCESS) m_status = CAPTURESTATUS::INIT;
	return res;
}

SCCode ScreenCapture::DeviceInit()
{
	HRESULT hr;
	// supported driver types
	D3D_DRIVER_TYPE D3DDriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
		D3D_DRIVER_TYPE_WARP,
	};

	UINT numsD3DDriverTypes = ARRAYSIZE(D3DDriverTypes);
	// supported feature levels
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};

	UINT numsFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	//1. Create D3D device
	for (UINT index = 0; index < numsD3DDriverTypes; index++)
	{
		hr = D3D11CreateDevice(nullptr,
			D3DDriverTypes[index],
			nullptr, 0,
			featureLevels,
			numsFeatureLevels,
			D3D11_SDK_VERSION,
			&m_pDX11Dev,
			&FeatureLevel,
			&m_pDX11DevCtx);

		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (FAILED(hr))
	{
		SHMEM_LOG("Error: Failed to create D3D device!");
		return SCCode::FAILED;
	}

	IDXGIDevice* pDXGIDevice = nullptr;
	//2. Get DXGI device
	hr = m_pDX11Dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&pDXGIDevice));
	if (FAILED(hr))
	{
		SHMEM_LOG("Error: Failed to get DXGI device!");
		return SCCode::FAILED;
	}

	IDXGIAdapter* pDXGIAdapter = nullptr;
	//3. Get DXGI adapter
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&pDXGIAdapter));
	pDXGIDevice->Release();
	pDXGIDevice = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to get DXGI adapter!");
		return SCCode::FAILED;
	}

	IDXGIOutput* pDXGIOutput = nullptr;
	//4. Get output
	hr = pDXGIAdapter->EnumOutputs(0, &pDXGIOutput);
	pDXGIAdapter->Release();
	pDXGIAdapter = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to get an enum output!");
		return SCCode::FAILED;
	}

	IDXGIOutput1* pDXGIOutput1 = nullptr;
	//5. QI for Output1
	hr = pDXGIOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&pDXGIOutput1));
	pDXGIOutput->Release();
	pDXGIOutput = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to query interface for output 1!");
		return SCCode::FAILED;
	}

	//6. Create desktop duplication
	hr = pDXGIOutput1->DuplicateOutput(m_pDX11Dev, &m_pDXGIOutputDup);
	pDXGIOutput1->Release();
	pDXGIOutput1 = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to get duplicate output!");
		return SCCode::FAILED;
	}

	return SCCode::SUCCESS;
}

SCCode ScreenCapture::RunScreenCapturing()
{
	UINT64 cur_count = 0;

	while (m_status != CAPTURESTATUS::STOPPED && cur_count < m_config.framesNum)
	{
		// Screen capture at certain fps
		std::chrono::high_resolution_clock clock{};
		UINT64 start = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
		SCCode res = CaptureOneFrame();
		switch (res)
		{
		case SCCode::SUCCESS:
			// SHMEM_LOG("Info: Success to capture one frame!");
			break;
		case SCCode::FAILED:
		case SCCode::UNKNOWN:
			SHMEM_LOG("Error: Failed to capture one frame!");
			return SCCode::FAILED;
		case SCCode::TIMEOUT:
			SHMEM_LOG("Warning: Timeout in capture one frame!");
			continue;
		case SCCode::NOTREADY:
			SHMEM_LOG("Warning: NotReady in capture one frame!");
			continue;
		default:
			break;
		}
		UINT64 end = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
		if (m_config.fps <= 0)
		{
			SHMEM_LOG("Error: the FPS configuration is not correctly set!");
			return SCCode::FAILED;
		}
		if (end - start < 1000 / m_config.fps) Sleep(1000 / m_config.fps - DWORD(end - start));
		else SHMEM_LOG("Warning: the fps of screen capture is less than %d", m_config.fps);
		UINT64 end1 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
		SHMEM_LOG("Debug: Run one frame at pts %lld, duration at %lld", cur_count, end1 - start);
		m_perf->SetDuration(end1 - start);
		cur_count++;
	}

	m_perf->PrintStats();

	return SCCode::SUCCESS;
}

SCCode ScreenCapture::CaptureOneFrame()
{
	HRESULT hr;

	if (m_pDXGIOutputDup == nullptr)
	{
		SHMEM_LOG("Error: m_pDXGIOutputDup is empty!");
		return SCCode::FAILED;
	}
	// 1. acquire next frame
	IDXGIResource* pDXGIResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	hr = m_pDXGIOutputDup->AcquireNextFrame(1000 / m_config.fps, &frameInfo, &pDXGIResource);
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{
			if (pDXGIResource) {
				pDXGIResource->Release();
				pDXGIResource = nullptr;
			}
			hr = m_pDXGIOutputDup->ReleaseFrame();
			return SCCode::TIMEOUT;
		}
		else
		{
			SHMEM_LOG("Error: Failed to acquire next frame!");
			return SCCode::FAILED;
		}
	}
	SHMEM_LOG("frameInfo.LastPresentTime.QuadPart %lld", frameInfo.LastPresentTime.QuadPart);
	static bool isReady = false;
	if (isReady == false && frameInfo.LastPresentTime.QuadPart == 0) // WR: skip first frame which is black
	{
		if (pDXGIResource) {
			pDXGIResource->Release();
			pDXGIResource = nullptr;
		}
		hr = m_pDXGIOutputDup->ReleaseFrame();
		isReady = true;
		return SCCode::NOTREADY;
	}

	if (pDXGIResource == nullptr)
	{
		SHMEM_LOG("Error: pDXGIResource is empty!");
		return SCCode::FAILED;
	}

	ID3D11Texture2D* pD3D11Texture = nullptr;
	//2. query next frame staging buffer
	hr = pDXGIResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pD3D11Texture));
	pDXGIResource->Release();
	pDXGIResource = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to query interface for next frame staging buffer!");
		return SCCode::FAILED;
	}

	D3D11_TEXTURE2D_DESC desc;
	//3. copy old description
	if (pD3D11Texture)
	{
		pD3D11Texture->GetDesc(&desc);
	}
	else
	{
		SHMEM_LOG("Error: Failed to copy texture description!");
		return SCCode::FAILED;
	}

	//4. create a new staging buffer for fill frame image
	D3D11_TEXTURE2D_DESC CopyTextureDesc{};
	CopyTextureDesc.Width = desc.Width;
	CopyTextureDesc.Height = desc.Height;
	CopyTextureDesc.MipLevels = 1;
	CopyTextureDesc.ArraySize = 1;
	CopyTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	CopyTextureDesc.SampleDesc.Count = 1;
	CopyTextureDesc.SampleDesc.Quality = 0;
	CopyTextureDesc.Usage = D3D11_USAGE_STAGING;
	CopyTextureDesc.BindFlags = 0;
	CopyTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	CopyTextureDesc.MiscFlags = 0;

	if (m_pDX11Dev == nullptr)
	{
		SHMEM_LOG("Error: m_pDX11Dev is empty!");
		return SCCode::FAILED;
	}
	ID3D11Texture2D* pCopyBuffer = nullptr;
	hr = m_pDX11Dev->CreateTexture2D(&CopyTextureDesc, nullptr, &pCopyBuffer);
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to create a new staging buffer for fill frame image!");
		return SCCode::FAILED;
	}

	//5. copy next staging buffer to new staging buffer
	if (pD3D11Texture)
	{
		if (m_pDX11DevCtx == nullptr)
		{
			SHMEM_LOG("Error: m_pDX11DevCtx is empty!");
			return SCCode::FAILED;
		}
		m_pDX11DevCtx->CopyResource(pCopyBuffer, pD3D11Texture);
		pD3D11Texture->Release();
		pD3D11Texture = nullptr;
	}

	IDXGISurface* pCopySurface = nullptr;
	//6. create staging buffer for map bits
	if (pCopyBuffer == nullptr)
	{
		SHMEM_LOG("Error: pCopyBuffer is empty!");
		return SCCode::FAILED;
	}
	hr = pCopyBuffer->QueryInterface(__uuidof(IDXGISurface), (void**)&pCopySurface);
	pCopyBuffer->Release();
	pCopyBuffer = nullptr;
	if (FAILED(hr)) {
		SHMEM_LOG("Error: Failed to create staging buffer for map bits!");
		return SCCode::FAILED;
	}

	DXGI_MAPPED_RECT mappedSurface;
	//7. copy bits to user space
	if (pCopySurface == nullptr)
	{
		SHMEM_LOG("Error: pCopySurface is empty!");
		return SCCode::FAILED;
	}
	hr = pCopySurface->Map(&mappedSurface, DXGI_MAP_READ);
	if (FAILED(hr)) {
		pCopySurface->Release();
		pCopySurface = nullptr;
		SHMEM_LOG("Error: Failed to copy bits to user space!");
		return SCCode::FAILED;
	}
	else // success
	{
		m_shareData->SetDataType(UINT(DataType::VIDEO));
		m_shareData->SetFormat(UINT(Format::RGBA));
		m_shareData->SetWidth(CopyTextureDesc.Width);
		m_shareData->SetHeight(CopyTextureDesc.Height);
		// add real data
		UINT pitch = mappedSurface.Pitch;
		BYTE* shmem = mappedSurface.pBits;
		UINT64 mem_size = static_cast<UINT64>(pitch) * desc.Height;
		m_shareData->SetPitch(pitch);
		m_shareData->SetDataSize(mem_size);
		m_shareData->SetRealData(shmem);
		static UINT64 pts = 0;
		m_shareData->SetPts(pts++);

		//Write one frame to ivshmem device memory
		if (WriteOneFrameToSHMDevice() != SCCode::SUCCESS)
		{
			pCopySurface->Unmap();
			pCopySurface->Release();
			pCopySurface = nullptr;
			SHMEM_LOG("Error: Failed to write one frame to ivshmem device memory!");
			return SCCode::FAILED;
		}
	}
	pCopySurface->Unmap();
	pCopySurface->Release();
	pCopySurface = nullptr;
	if (m_pDXGIOutputDup == nullptr)
	{
		SHMEM_LOG("Error: m_pDXGIOutputDup is empty!");
		return SCCode::FAILED;
	}
	m_pDXGIOutputDup->ReleaseFrame();

	return SCCode::SUCCESS;
}

SCCode ScreenCapture::WriteOneFrameToSHMDevice()
{
	if (m_shareData.get() == nullptr) return SCCode::FAILED;
	// get share data info
	UINT dataType = m_shareData->GetDataType();
	UINT64 pts = m_shareData->GetPts();
	UINT format = m_shareData->GetFormat();
	UINT pitch = m_shareData->GetPitch();
	UINT width = m_shareData->GetWidth();
	UINT height = m_shareData->GetHeight();
	UINT64 dataSize = m_shareData->GetDataSize();
	BYTE* realData = m_shareData->GetRealData();
	UINT writeFlag = 0; // write the flag at the last

	// write to share memory region, circular queue
	// -- pts -- dataType -- format -- pitch -- width -- height -- size -[offset record]- write flag -- rawdata
    m_memQueue->Write(pts, sizeof(pts) / sizeof(BYTE));
	m_memQueue->Write(dataType, sizeof(dataType) / sizeof(BYTE));
	m_memQueue->Write(format, sizeof(format) / sizeof(BYTE));
	m_memQueue->Write(pitch, sizeof(pitch) / sizeof(BYTE));
	m_memQueue->Write(width, sizeof(width) / sizeof(BYTE));
	m_memQueue->Write(height, sizeof(height) / sizeof(BYTE));
	m_memQueue->Write(dataSize, sizeof(dataSize) / sizeof(BYTE));
	size_t offset = m_memQueue->GetOffset();
	m_memQueue->Write(writeFlag, sizeof(writeFlag) / sizeof(BYTE));
    m_memQueue->Write(realData, dataSize);
	writeFlag = 1;
	m_memQueue->WriteWithOffset(writeFlag, sizeof(writeFlag), offset);
	SHMEM_LOG("Info: Write pts %lld, width %d, height %d, offset %lld", pts, width, height, m_memQueue->GetOffset());

	return SCCode::SUCCESS;
}

SCCode ScreenCapture::Destroy()
{
	if (m_pDXGIOutputDup)
	{
		m_pDXGIOutputDup->Release();
		m_pDXGIOutputDup = nullptr;
	}

	if (m_pDX11Dev)
	{
		m_pDX11Dev->Release();
		m_pDX11Dev = nullptr;
	}

	if (m_pDX11DevCtx)
	{
		m_pDX11DevCtx->Release();
		m_pDX11DevCtx = nullptr;
	}

	return SCCode::SUCCESS;
}