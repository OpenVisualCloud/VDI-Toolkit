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
 //! \file     Vioserial.h
 //! \brief    Define virtio serial APIs.
 //!

#ifndef _VIOSERIAL_H_
#define _VIOSERIAL_H_

#include <stdlib.h>
#include <initguid.h>
#include <stdio.h>
#include <Windows.h>
#include <SetupAPI.h>

#include "Tools.h"

enum class VIOSERStatus {
    SUCCESS = 0,
    FAILED,
    UNKNOWN,
};

using HANDLE = PVOID;

class VioserDev
{
public:
    VioserDev():m_dev(INVALID_HANDLE_VALUE) {}
    virtual ~VioserDev();
    VIOSERStatus Init();
    VIOSERStatus Write(PVOID buf, size_t* size); // [in/out] size
    VIOSERStatus Read(PVOID buf, size_t* size);  // [in/out] size
private:
    HANDLE   m_dev;
};

#endif // !_VIOSERIAL_H_