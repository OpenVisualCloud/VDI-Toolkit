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

#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#ifdef MDSCLIB_EXPORTS
#define MDSCLIB_API __declspec(dllexport)
#else
#define MDSCLIB_API __declspec(dllimport)
#endif

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <new>
#include <sal.h>
#include <warning.h>
#include <windows.h>
#include <wrl\client.h>


using namespace DirectX;
using Microsoft::WRL::ComPtr;

typedef _Return_type_success_(return == SCREENCAP_SUCCESSED) enum {
  SCREENCAP_SUCCESSED = 0,
  SCREENCAP_CONTINUED = 1,
  SCREENCAP_FAILED    = 2
} SCREENCAP_STATUS;

//
// D3D Resources
//
typedef struct DX_RESOURCES {
  ID3D11Device *Device;
  ID3D11DeviceContext *Context;
} DxResources;

//
// CaptureManager Captured Data
//
typedef struct CAPTURED_DATA {
  ID3D11Texture2D *CapturedTexture;
  uint64_t AcquiredTime;
  DXGI_OUTDUPL_FRAME_INFO FrameInfo;
} CapturedData;

//
// ScreenManager thread input params
//
typedef struct THREAD_INPUT_PARAMS {
  HANDLE TerminateThreadsEvent;
  UINT ScreenNumber;
  DX_RESOURCES DxRes;
  HANDLE BufferQueueHandle;
  UINT CaptureFps;
} ThreadInputParams;

#endif