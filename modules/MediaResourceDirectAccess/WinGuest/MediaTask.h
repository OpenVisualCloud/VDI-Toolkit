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
//! \file MediaTask.h
//! \brief Media Task, called by MRDALib APIs, to setup task manager,
//!         and to manage media process and media buffer data.
//! \date 2024-03-29
//!


#ifndef _MEDIA_TASK_H_
#define _MEDIA_TASK_H_

#include "../utils/common.h"

#include "TaskManager.h"

VDI_NS_BEGIN

class MediaTask {
public:
    //!
    //! \brief Construct a new Media Task object
    //!
    MediaTask();
    //!
    //! \brief Destroy the Media Task object
    //!
    virtual ~MediaTask() = default;

    //!
    //! \brief Initialize the Media Task
    //!
    //! \param [in] taskInfo
    //!         input task info
    //! \param [in] config
    //!         external configuration
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Initialize(const TaskInfo *taskInfo, const ExternalConfig *config);

    //!
    //! \brief Stop the process of Media Task
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Stop();

    //!
    //! \brief Reset the process of Media Task
    //!
    //! \param [in] taskInfo
    //!         input task info
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Reset(const TaskInfo *taskInfo);

    //!
    //! \brief Destroy the Media Task
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Destroy();

    //!
    //! \brief Set the Init Params object
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus SetInitParams(const MediaParams *params);

    //!
    //! \brief Send input frame to Media Task
    //!
    //! \param [in] data
    //!         input frame data
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus SendFrame(const std::shared_ptr<FrameBufferItem> data);

    //!
    //! \brief Get One input Buffer
    //!
    //! \param [out] data
    //!              buffer from input frame memory pool
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus GetOneInputBuffer(std::shared_ptr<FrameBufferItem> &data);

    //!
    //! \brief Release one output buffer
    //!
    //! \param [in] data
    //!              buffer needed release
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus ReleaseOutputBuffer(std::shared_ptr<FrameBufferItem> data);

    //！
    //! \brief Receive output frame from Media Task
    //!
    //! \param [out] data
    //!         output frame data
    //! \return MRDAStatus
    //！         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus ReceiveFrame(std::shared_ptr<FrameBufferItem> &data);

private:
    //!
    //! \brief Check value of media parameters
    //!
    //! \param [in] params
    //!         media parameters
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus CheckMediaParams(const MediaParams *params);

private:
    std::shared_ptr<TaskManager> m_taskManager;  //!< task manager

};

VDI_NS_END
#endif // _MEDIA_TASK_H_