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

 */

//!
//! \file UtilVPL.h
//! \brief uitility VPL functions
//! \date 2024-04-22
//!

#ifndef _UTIL_VPL_H_
#define _UTIL_VPL_H_

#include "../utils/common.h"
#include "vpl/mfx.h"
#include "vpl/mfxstructures.h"
#include "vpl/mfxvideo.h"
#include "vpl/mfxvideo++.h"
#include "vpl/mfxdefs.h"


VDI_NS_BEGIN

#define ALIGN16(value)   (((value + 15) >> 4) << 4)

#define WAIT_100_MILLISECONDS 100

#define BITSTREAM_BUFFER_SIZE 200000000

#define MAJOR_API_VERSION_REQUIRED 2
#define MINOR_API_VERSION_REQUIRED 2

#define VPLVERSION(major, minor) (major << 16 | minor)

VDI_NS_END
#endif //_UTIL_VPL_H_