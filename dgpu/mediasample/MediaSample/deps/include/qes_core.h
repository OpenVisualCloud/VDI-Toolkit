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

#include "qes_defs.h"
#include "qes_structures.h"
#include "qes_frame.h"

class QESLIB_API qesCore
{
public:
    static qesCore* CreateInstance();
    static void DestroyInstance(qesCore* pCore);

public:
    virtual qesStatus InitDevice(int nAdapterNum) = 0;
    virtual qesStatus GetDevice(qesDeviceHandle* pDevice) = 0;
    virtual qesStatus SetDevice(qesDeviceHandle* pDevice) = 0;
    virtual qesStatus RegisterTextureToEncoder(qesTextureHandle* pTexture, qesFrame** pFrame) = 0;
    virtual qesStatus UnregisterTexture(qesFrame* pFrame) = 0;
};