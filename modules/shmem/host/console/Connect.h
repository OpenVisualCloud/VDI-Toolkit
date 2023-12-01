/*
 * Copyright (c) 2023, Intel Corporation
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
 //! \file     Connect.h
 //! \brief    Define interface to connect guest VM via vioserial.
 //!

 #ifndef _CONNECT_H_
 #define _CONNECT_H_

#include <string>

enum class TASKStatus
{
    SUCCESS = 0,
    FAILED,
    UNKNOWN
};

enum class APPType
{
    SC = 0,
    UNKNOWN
};

struct InputParams
{
	uint32_t fps;
	uint32_t framesNum;
    std::string vmConfigFilePath;
    std::string enConfigFilePath;
    APPType type;
};

class TaskNode
{
public:
    TaskNode(std::string ip, std::string socketAddr, std::string encCmd, APPType type)
    : m_ip(ip), m_socketAddr(socketAddr), m_encCmd(encCmd), m_type(type), m_sock(0)
    {};
    virtual ~TaskNode()
    {
        m_thread.join();
        close(m_sock);
    }
    TASKStatus SetupConnection();
    TASKStatus StartTask(InputParams params);
    inline std::string GetIp() { return m_ip; }
private:
    void TaskThreadWrapper(uint32_t fps, uint32_t framesNum);
    TASKStatus SendMsg(std::string cmd);
    TASKStatus ReceiveMsg();
    TASKStatus StartEncoding();

    std::string m_ip;
    std::string m_socketAddr;
    std::string m_encCmd;
    APPType m_type;
    int m_sock;
    std::thread m_thread;
};

 #endif // !_CONNECT_H_