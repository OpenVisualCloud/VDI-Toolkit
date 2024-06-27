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
//! \file TaskDataSession.h
//! \brief manage task data session
//! \date 2024-04-01
//!

#ifndef _TASK_DATA_SESSION_H_
#define _TASK_DATA_SESSION_H_

#include "../utils/common.h"
#include "../SHMemory/FrameBufferData.h"

VDI_NS_BEGIN

class TaskDataSession
{
public:
    //!
    //! \brief Construct a new Task Data Session object
    //!
    TaskDataSession(){};
    //!
    //! \brief Destroy the Task Data Session object
    //!
    virtual ~TaskDataSession() = default;
    //!
    //! \brief Initialize the Task Data Session object
    //!
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    // MRDAStatus Initialize();
    //!
    //! \brief Set the Init Params object
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus SetInitParams(const MediaParams *params) = 0;
    //!
    //! \brief Send the data to the remote host
    //!
    //! \param [in] data
    //!             the data to send
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus SendFrame(const std::shared_ptr<FrameBufferData> data) = 0;
    //!
    //! \brief Receive the data from the remote host
    //!
    //! \param [in] data
    //!             the data to Received
    //! \return MRDAStatus
    //!         MRDA_SUCCESS if success, else fail
    //!
    virtual MRDAStatus ReceiveFrame(std::shared_ptr<FrameBufferData> &data) = 0;
};

VDI_NS_END
#endif // _TASK_DATA_SESSION_H_