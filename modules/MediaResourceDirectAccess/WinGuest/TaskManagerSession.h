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
//! \file TaskManagerSession.h
//! \brief task manager session to help task manager to
//!        control the status of host services.
//! \date 2024-04-01
//!

#ifndef _TASK_MANAGER_SESSION_H_
#define _TASK_MANAGER_SESSION_H_

#include "../utils/common.h"

VDI_NS_BEGIN

class TaskManagerSession
{
public:
    //!
    //! \brief Construct a new Task Manager Session object
    //!
    TaskManagerSession(){};
    //!
    //! \brief Destroy the Task Manager Session object
    //!
    virtual ~TaskManagerSession() = default;
    //!
    //! \brief Start the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskInfo
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus StartTask(const TaskInfo* in_taskInfo, TaskInfo* out_taskInfo) = 0;
    //!
    //! \brief Stop the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskStatus
    //! \return MRDAStatus
    //！        MRDA_STATUS_SUCCESS if success, else fail
    //!
    virtual MRDAStatus StopTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus) = 0;
    //!
    //! \brief Reset the task
    //!
    //! \param [in] in_taskInfo
    //! \param [out] out_taskStatus
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fails
    //!
    virtual MRDAStatus ResetTask(const TaskInfo* in_taskInfo, TASKStatus* out_taskStatus) = 0;

};

VDI_NS_END
#endif //_TASK_MANAGER_SESSION_H_