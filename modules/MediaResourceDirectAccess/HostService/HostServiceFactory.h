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
//! \file HostServiceFactory.h
//! \brief host service factory to create service according to task info
//! \date 2024-04-11
//!

#ifndef _HOSTSERVICEFACTORY_H_
#define _HOSTSERVICEFACTORY_H_

#include "HostService.h"
#ifdef _VPL_SUPPORT_
#include "EncodeService/VPLEncode/HostVPLEncodeService.h"
#endif
#ifdef _FFMPEG_SUPPORT_
#include "EncodeService/FFmpegEncode/HostFFmpegEncodeService.h"
#include "DecodeService/FFmpegDecode/HostFFmpegDecodeService.h"
#endif
#include "../protos/MRDAServiceManager.grpc.pb.h"

#include <memory>

VDI_NS_BEGIN

class HostServiceFactory
{
public:
    //!
    //! \brief Construct a new Host Service Factory object
    //!
    HostServiceFactory() {};
    //!
    //! \brief Destroy the Host Service Factory object
    //!
    virtual ~HostServiceFactory() = default;
    //!
    //! \brief Create a Host Service object according to task info
    //!
    //! \param [in] info
    //! \return std::shared_ptr<HostService>
    //!
    inline static std::shared_ptr<HostService> CreateHostService(const MRDA::TaskInfo *info)
    {
        TaskInfo taskInfo = MakeTaskInfoBack(info);

        if (info->devicetype() == static_cast<int32_t>(DeviceType::GPU)
            && info->tasktype() == static_cast<int32_t>(TASKTYPE::taskFFmpegEncode))
        {
#ifdef _FFMPEG_SUPPORT_
            return std::make_shared<HostFFmpegEncodeService>(taskInfo);
#else
            MRDA_LOG(LOG_ERROR, "FFmpeg is not supported");
            return nullptr;
#endif
        }
        else if (info->devicetype() == static_cast<int32_t>(DeviceType::GPU)
            && info->tasktype() == static_cast<int32_t>(TASKTYPE::taskOneVPLEncode))
        {
#ifdef _VPL_SUPPORT_
            return std::make_shared<HostVPLEncodeService>(taskInfo);
#else
            MRDA_LOG(LOG_ERROR, "VPL is not supported");
            return nullptr;
#endif
        }
        else if (info->devicetype() == static_cast<int32_t>(DeviceType::GPU)
            && info->tasktype() == static_cast<int32_t>(TASKTYPE::taskFFmpegDecode))
        {
#ifdef _FFMPEG_SUPPORT_
            return std::make_shared<HostFFmpegDecodeService>(taskInfo);
#else
            MRDA_LOG(LOG_ERROR, "FFmpeg is not supported");
            return nullptr;
#endif
        }
        else
        {
            MRDA_LOG(LOG_ERROR, "Unsupported task type or device type");
            return nullptr;
        }
    }
private:
    //!
    //! \brief Transfer from mrda TaskInfo to TaskInfo
    //!
    inline static TaskInfo MakeTaskInfoBack(const MRDA::TaskInfo *mrda_taskInfo)
    {
        TaskInfo taskInfo;
        taskInfo.taskType = static_cast<TASKTYPE>(mrda_taskInfo->tasktype());
        taskInfo.taskStatus = static_cast<TASKStatus>(mrda_taskInfo->taskstatus());
        taskInfo.taskID = mrda_taskInfo->taskid();
        taskInfo.taskDevice.deviceID = mrda_taskInfo->deviceid();
        taskInfo.taskDevice.deviceType = static_cast<DeviceType>(mrda_taskInfo->devicetype());
        taskInfo.ipAddr = mrda_taskInfo->ipaddr();
        return taskInfo;
    }

};

VDI_NS_END
#endif // _HOSTSERVICEFACTORY_H_