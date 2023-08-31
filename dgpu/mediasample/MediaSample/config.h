#pragma once

#include <d3d11.h>
#include <fstream>
#include <string>
#include "qes_encode.h"
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

struct qesEncoderSpecInfo
{
    string outFile;
    string roiSettingFile;
    string refListSettingFile;
    string targetFrameSizeSettingFile;
    qesEncodeParameters encodePars;
};


class CGConfig {
public:
    CGConfig();
    virtual ~CGConfig();
    qesStatus Init(const char* configFile);
    qesStatus SetEncoderFrameInfoFromDxgiDesc(D3D11_TEXTURE2D_DESC desc);
    qesStatus SetEncoderSpecInfoFromJson(const json& j);
    qesEncodeParameters* GetEncoderSpecPars();
    string GetOutputFileName();
private:
    qesEncoderSpecInfo m_encoderSpecInfo;
};
