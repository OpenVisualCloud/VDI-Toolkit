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

#ifdef QESLIB_EXPORTS
#define QESLIB_API __declspec(dllexport)
#else
#define QESLIB_API __declspec(dllimport)
#endif

#define QES_MAKEFOURCC(A,B,C,D)    ((((int)A))+(((int)B)<<8)+(((int)C)<<16)+(((int)D)<<24))

/*********************************************************************************\
Error message
\*********************************************************************************/
/*! @enum qesStatus Itemizes status codes returned by API functions. */
typedef enum
{
    /* no error */
    QES_ERR_NONE                        = 0,    /*!< No error. */
    /* reserved for unexpected errors */
    QES_ERR_UNKNOWN                     = -1,   /*!< Unknown error. */

    /* error codes <0 */
    QES_ERR_NULL_PTR                    = -2,   /*!< Null pointer. */
    QES_ERR_UNSUPPORTED                 = -3,   /*!< Unsupported feature. */
    QES_ERR_MEMORY_ALLOC                = -4,   /*!< Failed to allocate memory. */
    QES_ERR_NOT_ENOUGH_BUFFER           = -5,   /*!< Insufficient buffer at input/output. */
    QES_ERR_INVALID_HANDLE              = -6,   /*!< Invalid handle. */
    QES_ERR_LOCK_MEMORY                 = -7,   /*!< Failed to lock the memory block. */
    QES_ERR_NOT_INITIALIZED             = -8,   /*!< Member function called before initialization. */
    QES_ERR_NOT_FOUND                   = -9,   /*!< The specified object is not found. */
    QES_ERR_MORE_DATA                   = -10,  /*!< Expect more data at input. */
    QES_ERR_MORE_SURFACE                = -11,  /*!< Expect more surface at output. */
    QES_ERR_ABORTED                     = -12,  /*!< Operation aborted. */
    QES_ERR_DEVICE_LOST                 = -13,  /*!< Lose the hardware acceleration device. */
    QES_ERR_INCOMPATIBLE_VIDEO_PARAM    = -14,  /*!< Incompatible video parameters. */
    QES_ERR_INVALID_VIDEO_PARAM         = -15,  /*!< Invalid video parameters. */
    QES_ERR_UNDEFINED_BEHAVIOR          = -16,  /*!< Undefined behavior. */
    QES_ERR_DEVICE_FAILED               = -17,  /*!< Device operation failure. */
    QES_ERR_MORE_BITSTREAM              = -18,  /*!< Expect more bitstream buffers at output. */
    QES_ERR_GPU_HANG                    = -21,  /*!< Device operation failure caused by GPU hang. */
    QES_ERR_REALLOC_SURFACE             = -22,  /*!< Bigger output surface required. */
    QES_ERR_RESOURCE_MAPPED             = -23,  /*!< Write access is already acquired and user requested
                                                   another write access, or read access with MFX_MEMORY_NO_WAIT flag. */
    QES_ERR_NOT_IMPLEMENTED             = -24,   /*!< Feature or function not implemented. */
    /* warnings >0 */
    QES_WRN_IN_EXECUTION                = 1,    /*!< The previous asynchronous operation is in execution. */
    QES_WRN_DEVICE_BUSY                 = 2,    /*!< The hardware acceleration device is busy. */
    QES_WRN_VIDEO_PARAM_CHANGED         = 3,    /*!< The video parameters are changed during decoding. */
    QES_WRN_PARTIAL_ACCELERATION        = 4,    /*!< Software acceleration is used. */
    QES_WRN_INCOMPATIBLE_VIDEO_PARAM    = 5,    /*!< Incompatible video parameters. */
    QES_WRN_VALUE_NOT_CHANGED           = 6,    /*!< The value is saturated based on its valid range. */
    QES_WRN_OUT_OF_RANGE                = 7,    /*!< The value is out of valid range. */
    QES_WRN_FILTER_SKIPPED              = 10,   /*!< One of requested filters has been skipped. */
    /* low-delay partial output */
    QES_ERR_NONE_PARTIAL_OUTPUT         = 12,   /*!< Frame is not ready, but bitstream contains partial output. */

    /* threading statuses */
    QES_TASK_DONE = QES_ERR_NONE,               /*!< Task has been completed. */
    QES_TASK_WORKING                    = 8,    /*!< There is some more work to do. */
    QES_TASK_BUSY                       = 9,    /*!< Task is waiting for resources. */

    /* plug-in statuses */
    QES_ERR_MORE_DATA_SUBMIT_TASK       = -10000, /*!< Return QES_ERR_MORE_DATA but submit internal asynchronous task. */

} qesStatus;