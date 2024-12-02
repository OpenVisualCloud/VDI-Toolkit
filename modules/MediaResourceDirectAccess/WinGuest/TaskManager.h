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
//! \file TaskManager.h
//! \brief Task Manager, manages memory pools, task manager session,
//!         data sender, data receiver.
//! \date 2024-03-29
//!

#ifndef _TASK_MANAGER_H_
#define _TASK_MANAGER_H_

#include "../utils/common.h"

#include "DataReceiver.h"
#include "DataSender.h"
#include "../SHMemory/FrameMemoryPool.h"
#include "TaskManagerSession.h"
#include "TaskManagerSession_gRPC.h"
#include "TaskDataSession.h"
#include "TaskDataSession_gRPC.h"

VDI_NS_BEGIN

class TaskManager {
public:
    //!
    //! \brief Construct a new Task Manager object
    //!
    TaskManager();
    //!
    //! \brief Destroy the Task Manager object
    //!
    virtual ~TaskManager();

    //!
    //! \brief Initialize the Task Manager
    //!
    //! \param [in] taskInfo
    //!         task info for initialization
    //! \param [in] config
    //!         external configuration
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Initialize(const TaskInfo *taskInfo, const ExternalConfig *config);

    //!
    //! \brief Stop the task
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus StopTask();

    //!
    //! \brief Reset task
    //!
    //! \param [in] taskInfo
    //!         input task info
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus ResetTask(const TaskInfo *taskInfo);

    //!
    //! \brief Destroy task manager
    //!
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus Destroy();

    //!
    //! \brief Set the Init Params
    //!
    //! \param [in] params
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus SetInitParams(const MediaParams *params);

    //!
    //! \brief Send input frame to the task manager
    //!
    //! \param [in] data
    //!         input frame data
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus SendFrame(const std::shared_ptr<FrameBufferData> data);

    //!
    //! \brief Receive output frame from the task manager
    //!
    //! \param [in] data
    //!         output frame data
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus ReceiveFrame(std::shared_ptr<FrameBufferData> &data);

    //!
    //! \brief Get One input Buffer from input frame memory pool
    //!
    //! \param [out] data
    //!              buffer from input frame memory pool
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus GetOneInputBuffer(std::shared_ptr<FrameBufferData> &data);

    //!
    //! \brief Release one buffer from output frame memory pool
    //!
    //! \param [in] data
    //!             buffer from output frame memory pool
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus ReleaseOneOutputBuffer(std::shared_ptr<FrameBufferData> data);
    //!
    //! \brief Get one Buffer from buffer pool
    //!
    //! \param       [in] id
    //!              request buf id
    //!              [out] data
    //!              the buffer to be obtained
    //! \return MRDAStatus
    //!         MRDA_STATUS_SUCCESS if success, else fail
    //!
    MRDAStatus GetBufferFromId(uint32_t id, std::shared_ptr<FrameBufferData>& data);

    //!
    //! \brief Get task type
    //!
    //! \return TASKTYPE
    //!
    inline TASKTYPE TaskType() {
        return m_taskInfo ? m_taskInfo->taskType : TASKTYPE::NONE;
    }

private:
    std::shared_ptr<TaskInfo> m_taskInfo;             //!< task info
    // StreamInfo  m_streamInfo;                      //!< stream info
    std::shared_ptr<EncodeParams> m_encodeParams;     //!< encode parameters
    std::shared_ptr<DecodeParams> m_decodeParams;     //!< decode parameters
    std::shared_ptr<ShareMemoryInfo> m_shareMemInfo;  //!< share memory information

    std::shared_ptr<MemoryPool<FrameBufferData>> m_inMemoryPool;    //!< memory pool for input frame
    std::shared_ptr<MemoryPool<FrameBufferData>> m_outMemoryPool;   //!< memory pool for output frame
    std::shared_ptr<TaskManagerSession> m_taskManagerSession;       //!< task manager session
    std::shared_ptr<DataSender> m_dataSender;                       //!< data sender
    std::shared_ptr<DataReceiver> m_dataReceiver;                   //!< data receiver
    std::shared_ptr<TaskDataSession> m_taskDataSession;             //!< task data session
};

VDI_NS_END
#endif // _TASK_MANAGER_H_