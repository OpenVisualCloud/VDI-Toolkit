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

#ifndef _JSONCONFIG_H
#define _JSONCONFIG_H

#include <fstream>
#include "json.hpp"

class JsonConfig
{
public:
    JsonConfig() = default;
    ~JsonConfig() = default;

    bool parse_file(const std::string& file)
    {
        std::ifstream json_file(file);
        if (json_file)
        {
            json_file >> m_json_obj;
        }
        else
        {
            return false;
        }
        return true;
    }

    void parse_string(const std::string& str)
    {
        m_json_obj = nlohmann::json::parse(str);
    }

    int32_t get_int32(const std::string& key)
    {
        return m_json_obj[key];
    }

    uint32_t get_uint32(const std::string& key)
    {
        return m_json_obj[key];
    }

    uint64_t get_uint64(const std::string& key)
    {
        return m_json_obj[key];
    }

    // throws nlohmann::detail::type_error
    std::string get_string(const std::string& key)
    {
        return m_json_obj[key];
    }

    bool get_boolean(const std::string& key)
    {
        return m_json_obj[key];
    }

    float get_float(const std::string& key)
    {
        return m_json_obj[key];
    }

private:
    nlohmann::json m_json_obj;
};

#endif