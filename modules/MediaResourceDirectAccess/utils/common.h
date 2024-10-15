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
//! \file tools.h
//! \brief common function tools
//! \date 2024-03-29
//!

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <memory>
#include <string>
#include "../API/MediaResourceDirectAccessAPI.h"
#include "../utils/ns_def.h"

constexpr int LOG_INFO = 0;
constexpr int LOG_WARNING = 1;
constexpr int LOG_ERROR = 2;

#ifdef _WINDOWS_OS_
#include <Windows.h>
#define MRDA_LOG(level, format, ...) \
    do { \
        SYSTEMTIME st; \
        GetLocalTime(&st); \
        printf("[%04d-%02d-%02d %02d:%02d:%02d.%03d:%s:%d] ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, __FILE__, __LINE__); \
        if (level == LOG_INFO) { \
            printf("INFO: "); \
        } else if (level == LOG_WARNING) { \
            printf("ERROR: "); \
        } else if (level == LOG_ERROR) { \
            printf("ERROR: "); \
        } \
        printf(format, ##__VA_ARGS__); \
        printf("\n"); \
    } while (false)
#endif

#ifdef _LINUX_OS_
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#define MRDA_LOG(level, format, ...) \
    do { \
        struct timespec now; \
        clock_gettime(CLOCK_REALTIME, &now); \
        struct tm local_time; \
        localtime_r(&now.tv_sec, &local_time); \
        char time_str[20]; \
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_time); \
        fprintf(stderr, "[%s.%03ld:%s:%d] ", time_str, (long int)now.tv_nsec / 1000000, __FILE__, __LINE__); \
        if (level == LOG_INFO) { \
            fprintf(stderr, "INFO: "); \
        } else if (level == LOG_WARNING) { \
            fprintf(stderr, "WARNING: "); \
        } else if (level == LOG_ERROR) { \
            fprintf(stderr, "ERROR: "); \
        } \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (false)
#endif

#define SAFE_DELETE(x) \
  if (NULL != (x)) {   \
    delete (x);        \
    (x) = NULL;        \
  };

#endif // _TOOLS_H_