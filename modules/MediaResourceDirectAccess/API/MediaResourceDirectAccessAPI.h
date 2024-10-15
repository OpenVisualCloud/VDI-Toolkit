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

//!
//! \file MediaResourceDirectAccessAPI.h
//! \brief Define APIs for Media Resource Direct Access Library.
//! \date 2024-03-29
//!

#ifndef _MEDIARESOURCEDIRECTACCESSAPI_H_
#define _MEDIARESOURCEDIRECTACCESSAPI_H_

#include "../utils/error_code.h"
#include <string>
#include <memory>

//!
//! \brief Device type
//!
//!
enum class DeviceType
{
    NONE = -1,
    GPU,
    CPU
};

//!
//! \brief hardware device
//!
//!
typedef struct HWDevice
{
    DeviceType deviceType;
    uint32_t deviceID;
} HWDevice;

//!
//! \brief task type
//!
//!
enum class TASKTYPE
{
    NONE = -1,
    taskFFmpegEncode,
    taskOneVPLEncode,
    taskEncode,
    taskDecode
};

//!
//! \brief task status
//!
//!
enum class TASKStatus
{
    TASK_STATUS_INITIALIZED = 0,
    TASK_STATUS_RUNNING,
    TASK_STATUS_STOPPED,
    TASK_STATUS_RESET,
    TASK_STATUS_ERROR,
    TASK_STATUS_UNKNOWN
};

//!
//! \brief task info
//!
//!
typedef struct TASKINFO
{
    TASKTYPE taskType;
    TASKStatus taskStatus;
    uint32_t taskID;
    HWDevice taskDevice;
    std::string ipAddr;
    //XXX
} TaskInfo;

//!
//! \brief Input stream type
//!
//!
enum class InputStreamType {
    UNKNOWN = -1,
    ENCODED,
    RAW,
    RESERVED
};

//!
//! \brief stream codec id if the stream type is encoded
//!
//!
enum class StreamCodecID {
    CodecID_NONE = 0,
    CodecID_AVC,
    CodecID_HEVC,
    CodecID_AV1
};

//!
//! \brief raw data format if the stream type is raw
//!
//!
enum class ColorFormat {
    COLOR_FORMAT_NONE = 0,
    COLOR_FORMAT_YUV420P,
    COLOR_FORMAT_RGBA32,
    COLOR_FORMAT_NV12
};

//!
//! \brief codec profile
//!
//!
enum class CodecProfile {
    PROFILE_AVC_MAIN = 0,
    PROFILE_AVC_HIGH,
    PROFILE_HEVC_MAIN,
    PROFILE_HEVC_MAIN10,
    PROFILE_AV1_MAIN,
    PROFILE_AV1_HIGH,
    PROFILE_NONE
};

//!
//! \brief target usage for encoding
//!
//!
enum class TargetUsage {
    Unknown = -1,
    BestQuality = 1,
    Balanced = 4,
    BestSpeed = 7
};

//!
//! \brief encode parameters for encoding
//!
//!
typedef struct ENCODEPARAMS {
    StreamCodecID codec_id;             //!< codec id
    uint32_t gop_size;                  //!< the distance between two adjacent intra frame
    uint32_t async_depth;               //!< Specifies how many asynchronous operations an application performs
    TargetUsage target_usage;           //!< the preset for quality and performance balance,
                                        //!< [0-12], 0 is best quality, 12 is best performance
    uint32_t rc_mode;                   //!< rate control mode, 0 is CQP mode and 1 is VBR mode
    uint32_t qp;                        //!< quantization value under CQP mode
    uint32_t bit_rate;                  //!< bitrate value under VBR mode
    int32_t  framerate_num;             //!< frame rate numerator
    int32_t  framerate_den;             //!< frame rate denominator
    uint32_t frame_width;               //!< width of frame
    uint32_t frame_height;              //!< height of frame
    ColorFormat color_format;           //!< pixel color format
    CodecProfile codec_profile;         //!< the profile to create bitstream
    uint32_t max_b_frames;              //!< maximum number of B-frames between non-B-frames
    uint32_t     frame_num;             //!< total frame number
} EncodeParams;

//!
//! \brief decode parameters for decoding
//!
//!
typedef struct DECODEPARAMS {
    // Need further implementation
} DecodeParams;

//!
//! \brief input stream information
//!
//!
// typedef struct STREAMINFO
// {
//     uint32_t            frameWidth;         //!< width of frame
//     uint32_t            frameHeight;        //!< height of frame
//     InputStreamType     streamType;         //!< the type of input stream
//     uint32_t            frameNum;           //!< frame number
// }StreamInfo;

//!
//! \brief Share memory info
//!
//!
typedef struct SHAREMEMORYINFO
{
    uint64_t     totalMemorySize;    //!< total memory size
    uint32_t     bufferNum;          //!< buffer number
    uint64_t     bufferSize;         //!< buffer size
    std::string  in_mem_dev_path;    //!< input memory dev path
    std::string  out_mem_dev_path;   //!< output memory dev path
    uint32_t     in_mem_dev_slot_number;     //!< input memory device slot number
    uint32_t     out_mem_dev_slot_number;    //!< output memory device slot number
}ShareMemoryInfo;

//!
//! \brief media parameters for Media Resource Direct Access Library
//!
//!
typedef struct MEDIAPARAMS {
    ShareMemoryInfo  shareMemoryInfo;    //!< share memory info
    // StreamInfo       streamInfo;      //!< stream information
    EncodeParams     encodeParams;       //!< encode parameters
    // DecodeParams     decodeParams;    //!< decode parameters
} MediaParams;

//!
//! \brief buffer state for memory buffer
//!
enum class BufferState {
    BUFFER_STATE_NONE = 0,
    BUFFER_STATE_BUSY,
    BUFFER_STATE_IDLE
};

//!
//! \brief memory buffer item
//!
typedef struct MEMBUFFERITEM
{
    uint32_t buf_id;
    uint64_t mem_offset;
    uint64_t state_offset; // available sync flag
    uint8_t *buf_ptr;
    uint64_t size;
    uint64_t occupied_size; // occupied buf size
    BufferState state;
    void assign(uint32_t buf_id,
        uint64_t mem_offset,
        uint64_t state_offset,
        uint8_t* buf_ptr,
        uint64_t size,
        uint64_t occupied_size,
        BufferState state) {
        this->buf_id = buf_id;
        this->mem_offset = mem_offset;
        this->state_offset = state_offset;
        this->buf_ptr = buf_ptr;
        this->size = size;
        this->occupied_size = occupied_size;
        this->state = state;
    }
} MemBufferItem;

//!
//! \brief frame buffer data
//!
typedef struct FRAMEBUFFERITEM {
    MemBufferItem *bufferItem;
    uint32_t width;
    uint32_t height;
    InputStreamType streamType;
    uint64_t pts;
    bool isEOS;
    void init(MemBufferItem* bufferItem,
        uint32_t width,
        uint32_t height,
        InputStreamType streamType,
        uint64_t pts,
        bool isEOS) {
        this->bufferItem = new MemBufferItem;
        this->bufferItem->buf_id = bufferItem->buf_id;
        this->bufferItem->buf_ptr = bufferItem->buf_ptr;
        this->bufferItem->mem_offset = bufferItem->mem_offset;
        this->bufferItem->occupied_size = bufferItem->occupied_size;
        this->bufferItem->size = bufferItem->size;
        this->bufferItem->state = bufferItem->state;
        this->bufferItem->state_offset = bufferItem->state_offset;
        this->width = width;
        this->height = height;
        this->streamType = streamType;
        this->pts = pts;
        this->isEOS = isEOS;
    }
    void uninit() {
        if (this->bufferItem) {
            delete this->bufferItem;
            this->bufferItem = nullptr;
        }
    }
} FrameBufferItem;

typedef struct ExternalConfig {
    std::string hostSessionAddr;
} ExternalConfig;

#ifdef _WINDOWS_OS_
#ifdef MRDALIBRARY_EXPORTS
#define MRDALIBRARY_API __declspec(dllexport)
#else
#define MRDALIBRARY_API __declspec(dllimport)
#endif
#else
#define MRDALIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

//!
//! \brief Initialize Media Resource Direct Access Library and get library handle
//!
//! \param [in] taskInfo
//!         task info
//! \param [in] config
//!         external config
//! \return MRDAHandle
//!         handle of Media Resource Direct Access Library
//!
MRDALIBRARY_API MRDAHandle MediaResourceDirectAccess_Init(const TaskInfo *taskInfo, const ExternalConfig *config);

//!
//! \brief Stop the media process
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_Stop(MRDAHandle handle);

//!
//! \brief Reset the media process
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \param [in] taskInfo
//!         task info
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_Reset(MRDAHandle handle, TaskInfo *taskInfo);
//!
//! \brief Destroy Media Resource Direct Access Library and release library handle
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_Destroy(MRDAHandle handle);

//!
//! \brief Set initial media parameters for Media Resource Direct Access Library
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \param [in] mediaParams
//!         media parameters
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_SetInitParams(MRDAHandle handle, const MediaParams *mediaParams);

//!
//! \brief Get buffer which can be used as input of codec process.
//!
//! \param [in] handle
//! \param [out] inputFrameData
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_GetBufferForInput(MRDAHandle handle, std::shared_ptr<FrameBufferItem> &inputFrameData);

//!
//! \brief Release buffer from output memory pool.
//!
//! \param [in] handle
//! \param [in] outputFrameData
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_ReleaseOutputBuffer(MRDAHandle handle, std::shared_ptr<FrameBufferItem> outputFrameData);

//!
//! \brief Send input frame to Media Resource Direct Access Library
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \param [in] inputFrameData
//!         input frame data
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_SendFrame(MRDAHandle handle, std::shared_ptr<FrameBufferItem> inputFrameData);

//!
//! \brief Receive output frame from Media Resource Direct Access Library
//!
//! \param [in] handle
//!         Media Resource Direct Access Library handle
//! \param [out] outputFrameData
//!         output frame data
//! \return MRDAStatus
//!         MRDA_STATUS_SUCCESS if success, else fail
//!
MRDALIBRARY_API MRDAStatus MediaResourceDirectAccess_ReceiveFrame(MRDAHandle handle, std::shared_ptr<FrameBufferItem> &outputFrameData);

#ifdef __cplusplus
}
#endif

#endif // _MEDIARESOURCEDIRECTACCESSAPI_H_