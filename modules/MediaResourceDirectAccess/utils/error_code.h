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
//! file:   error_code.h
//! brief:  define return type and status.
//!

#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

#include <stdint.h>

//!< handle for MRDA
typedef void *MRDAHandle;
//!< status for MRDA
typedef int32_t MRDAStatus;
#define MRDA_STATUS_SUCCESS           0X00000000
#define MRDA_STATUS_INVALID           0X00000001
#define MRDA_STATUS_INVALID_HANDLE    0X00000002
#define MRDA_STATUS_INVALID_PARAM     0X00000003
#define MRDA_STATUS_INVALID_STATE     0X00000004
#define MRDA_STATUS_TIMEOUT           0X00000005
#define MRDA_STATUS_NOT_SUPPORTED     0X00000006
#define MRDA_STATUS_NOT_IMPLEMENTED   0X00000007
#define MRDA_STATUS_NOT_READY         0X00000008
#define MRDA_STATUS_NOT_FOUND         0X00000009
#define MRDA_STATUS_OPERATION_FAIL    0X0000000A
#define MRDA_STATUS_INVALID_DATA      0X0000000B
#define MRDA_STATUS_NOT_ENOUGH_DATA   0X0000000C

#endif // _ERROR_CODE_H_
