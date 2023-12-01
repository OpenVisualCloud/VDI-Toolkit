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
 //! \file     Connect.cpp
 //! \brief    Main application to connect guest VM via vioserial.
 //!

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>

#include <unistd.h>
#include <memory>
#include <thread>

#include <sys/un.h>
#include <sys/socket.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <vector>
#include <functional>

#include "Connect.h"


struct guestVMInfo
{
    std::string ip;
    std::string socketAddr;
};

void PrintHelp()
{
	printf("%s", "Usage: ./Connect [<options>]\n");
	printf("%s", "Options: \n");
	printf("%s", "    [--help, -h]                        - print help README document. \n");
	printf("%s", "    [--vmconfig, -vmc config_file_path] - specifies vm connection configuration. \n");
    printf("%s", "    [--enconfig, -enc config_file_path] - specifies encoding configuration. \n");
    printf("%s", "    [--type application_type]           - specifies an application type. e.g., SC(ScreenCapture).\n");
	printf("%s", "    [--framesNum, -n number]            - specifies a total number of screen frames to capture if type is SC.\n");
    printf("%s", "    [--fps, -f number]                  - specifies a fps of screen capture if type is SC.\n");
	printf("%s", "Examples: ./Connect --vmconfig ./vmconfig.txt --enconfig ./enconfig.txt --type SC -n 3000 --fps 30 \n");
}

bool ParseParams(int argc, char** argv, InputParams* params)
{
	if (argc <= 1) {
		PrintHelp();
		return false;
	}

	for (auto i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			PrintHelp();
			return false;
		}
		else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fps") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->fps = std::stoi(argv[i]);
		}
		else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--framesNum") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->framesNum = std::stoi(argv[i]);
		}
		else if (strcmp(argv[i], "-vmc") == 0 || strcmp(argv[i], "--vmconfig") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->vmConfigFilePath = argv[i];
		}
        else if (strcmp(argv[i], "-enc") == 0 || strcmp(argv[i], "--enconfig") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else params->enConfigFilePath = argv[i];
		}
        else if (strcmp(argv[i], "--type") == 0)
		{
			if (++i == argc)
			{
				PrintHelp();
				return false;
			}
			else
            {
                if (strcmp(argv[i], "SC") == 0)
                {
                    params->type = APPType::SC;
                }
            }
		}
	}
	return true;
}

bool ParseVMConfig(std::string path, std::vector<guestVMInfo> &guestVMInfos)
{
    std::ifstream vmConfigFile(path);
    if (!vmConfigFile.is_open())
    {
        std::cout << "Failed to open config file!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(vmConfigFile, line))
    {
        std::istringstream iss(line);
        std::string ip;
        std::string addr;
        if (iss >> ip >> addr)
        {
            guestVMInfo vmInfo = {ip, addr};
            guestVMInfos.push_back(vmInfo);
        }
        else
        {
            std::cout << "Invalid line: " << line << std::endl;
            return false;
        }
    }
    return true;
}

bool ParseEncConfig(std::string path, std::vector<std::string> &cmds)
{
    std::ifstream encConfigFile(path);
    if (!encConfigFile.is_open())
    {
        std::cout << "Failed to open config file!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(encConfigFile, line))
    {
        cmds.push_back(line);
    }

    return true;
}

bool CreateTaskNode(std::vector<guestVMInfo> guestVMInfos, std::vector<std::string> cmds, APPType type, std::vector<std::shared_ptr<TaskNode>> &taskNodes)
{
    if (guestVMInfos.size() != cmds.size())
    {
        std::cout << "guest VMs number is not equal to the number of encode instances!" << std::endl;
        return false;
    }
    uint32_t num = guestVMInfos.size();
    for (uint32_t i = 0; i < num; i++)
    {
        std::shared_ptr<TaskNode> node = std::make_shared<TaskNode>(guestVMInfos[i].ip, guestVMInfos[i].socketAddr, cmds[i], type);
        taskNodes.push_back(std::move(node));
    }

    return true;
}

TASKStatus TaskNode::SetupConnection()
{
    m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, m_socketAddr.c_str());

    if (-1 == connect(m_sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)))
    {
        std::cout << "Failed to connect socket! Node ip: " << m_ip.c_str() << std::endl;
        return TASKStatus::FAILED;
    }

    std::cout << "Setup connection OK! socket addr " << m_socketAddr.c_str() << " Node ip " << m_ip.c_str() << std::endl;
    return TASKStatus::SUCCESS;
}

TASKStatus TaskNode::StartTask(InputParams params)
{
    m_thread = std::thread(&TaskNode::TaskThreadWrapper, this, params.fps, params.framesNum);
    return TASKStatus::SUCCESS;
}

void TaskNode::TaskThreadWrapper(uint32_t fps, uint32_t framesNum)
{
    // 1. send msg to start
    if (m_type == APPType::SC)
    {
        std::string msg = "SC:fps:" + std::to_string(fps) + ":frames:" + std::to_string(framesNum);

        if (TASKStatus::SUCCESS != SendMsg(msg))
        {
            std::cout << "Failed to send command for Screen capture! Node ip: " << m_ip.c_str() << std::endl;
            return;
        }
        std::cout << "send msg " << msg.c_str() << " to guest VM, node ip: " << m_ip.c_str() << std::endl;
    }

    // 2. receive ready string
    if (TASKStatus::SUCCESS != ReceiveMsg())
    {
        std::cout << "Failed to receive buf from guest! Node ip: " << m_ip.c_str() << std::endl;
        return;
    }

    // 3. start encoding process
    if (TASKStatus::SUCCESS != StartEncoding())
    {
        std::cout << "Failed to start encoding process! Node ip: " << m_ip.c_str() << std::endl;
        return;
    }
}

TASKStatus TaskNode::SendMsg(std::string msg)
{
    if (-1 == write(m_sock, msg.c_str(), msg.size()))
    {
        std::cout << "Failed to write msg: " << msg.c_str() << std::endl;
        return TASKStatus::FAILED;
    }

    return TASKStatus::SUCCESS;
}

TASKStatus TaskNode::ReceiveMsg()
{
    uint32_t timeout = 10;//5s timeout
    uint32_t cnt = 0;
    char rBuf[1024];
    while (cnt < timeout)
    {
        read(m_sock, rBuf, 10);
        if (strncmp(rBuf, "Ready", 5) == 0) break;

        cnt++;
        usleep(500 * 1000);
    }
    if (cnt == timeout)
    {
        std::cout << "Timeout for waiting the received msg! Node ip: " << m_ip.c_str() << std::endl;
        return TASKStatus::FAILED;
    }
    else
    {
        std::cout << "Received Ready flag from guest VM. Node ip: " << m_ip.c_str() << std::endl;
        return TASKStatus::SUCCESS;
    }
}

TASKStatus TaskNode::StartEncoding()
{
    if (m_encCmd.empty()) return TASKStatus::FAILED;

    sleep(1); // wait for a while to make sure the guest VM has written the memory
    int res = std::system(m_encCmd.c_str());

    if (res == 0) return TASKStatus::SUCCESS;
    else return TASKStatus::FAILED;
}

int main(int argc, char** argv)
{
    // 1. Parse input params
    InputParams inputParams;
    if (false == ParseParams(argc, argv, &inputParams))
    {
        std::cout << "Failed to parse params!" << std::endl;
        return -1;
    }

    // 2. Parse vm config file
    std::vector<guestVMInfo> guestVMInfos;
    if (false == ParseVMConfig(inputParams.vmConfigFilePath, guestVMInfos))
    {
        std::cout << "Failed to parse vm config file!" << std::endl;
        return -1;
    }

    // 3. Parse enc config file
    std::vector<std::string> encCmds;
    if (false == ParseEncConfig(inputParams.enConfigFilePath, encCmds))
    {
        std::cout << "Failed to parse encode config file!" << std::endl;
        return -1;
    }

    // 4. Create task nodes
    std::vector<std::shared_ptr<TaskNode>> taskNodes;
    if (false == CreateTaskNode(guestVMInfos, encCmds, inputParams.type, taskNodes))
    {
        std::cout << "Failed to create task node!" << std::endl;
        return -1;
    }

    // 5. For each node, setup connection and start task
    for (auto node : taskNodes)
    {
        if (TASKStatus::SUCCESS != node->SetupConnection())
        {
            std::cout << "Failed to setup connection! Node ip is " << node->GetIp().c_str() << std::endl;
            continue;
        }
        if (TASKStatus::SUCCESS != node->StartTask(inputParams))
        {
            std::cout << "Failed to start task! Node ip is " << node->GetIp().c_str() << std::endl;
            continue;
        }
    }

    return 0;
}
