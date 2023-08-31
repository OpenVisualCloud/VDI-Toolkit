/*
* Copyright (c) 2021, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <d3d11.h>

typedef struct
{
    unsigned int    FourCC;
    unsigned short  Width;
    unsigned short  Height;
    unsigned short  CropX;
    unsigned short  CropY;
    unsigned short  CropW;
    unsigned short  CropH;
} qesFrameInfo;

typedef struct
{
    unsigned short      Pitch;
    unsigned long long  TimeStamp;

    void*               ptr;
    union {
        unsigned char*  Y;
        unsigned char*  R;
    };
    union {
        unsigned char*  UV;            /* for UV merged formats */
        unsigned char*  G;
    };
    unsigned char*      B;
    unsigned char*      A;
} qesFrameData;

typedef struct
{
    unsigned int  reserved[4];
    qesFrameInfo  Info;
    qesFrameData  Data;
} qesRawFrame;

typedef enum
{
    LOCK_TO_READ  = 0x01,
    LOCK_TO_WRITE = 0x02
} qesLockFrameEnum;

typedef enum {
    QES_HANDLE_DIRECT3D_DEVICE_MANAGER9  = 1,      /*!< Pointer to the IDirect3DDeviceManager9 interface. See Working with Microsoft* DirectX* Applications for more details on how to use this handle. */
    QES_HANDLE_D3D9_DEVICE_MANAGER       = QES_HANDLE_DIRECT3D_DEVICE_MANAGER9, /*!< Pointer to the IDirect3DDeviceManager9 interface. See Working with Microsoft* DirectX* Applications for more details on how to use this handle. */
    QES_HANDLE_RESERVED1                 = 2, /* Reserved.  */
    QES_HANDLE_D3D11_DEVICE              = 3, /*!< Pointer to the ID3D11Device interface. See Working with Microsoft* DirectX* Applications for more details on how to use this handle. */
    QES_HANDLE_VA_DISPLAY                = 4, /*!< Pointer to VADisplay interface. See Working with VA-API Applications for more details on how to use this handle. */
    QES_HANDLE_RESERVED3                 = 5, /* Reserved.  */
    QES_HANDLE_VA_CONFIG_ID              = 6, /*!< Pointer to VAConfigID interface. It represents external VA config for Common Encryption usage model. */
    QES_HANDLE_VA_CONTEXT_ID             = 7, /*!< Pointer to VAContextID interface. It represents external VA context for Common Encryption usage model. */
    QES_HANDLE_CM_DEVICE                 = 8,  /*!< Pointer to CmDevice interface ( Intel(r) C for Metal Runtime ). */
    QES_HANDLE_HDDLUNITE_WORKLOADCONTEXT = 9,  /*!< Pointer to HddlUnite::WorkloadContext interface. */
} qesHandleType;

typedef struct {
    qesHandleType handleType;
    ID3D11Device*         ptr;
} qesDeviceHandle;

typedef enum {
    QES_DX11_TEXTURE
} qesTextureHandleType;

typedef struct {
    qesTextureHandleType handleType;
    void*                ptr;
} qesTextureHandle;