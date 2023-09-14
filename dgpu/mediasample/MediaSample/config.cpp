#include "config.h"

#define ALIGN16(x) (((x + 15) >> 4) << 4)

CGConfig::CGConfig()
{
    memset(&m_encoderSpecInfo, 0, sizeof(qesEncoderSpecInfo));
}

CGConfig::~CGConfig()
{
}

qesStatus CGConfig::SetEncoderFrameInfoFromDxgiDesc(D3D11_TEXTURE2D_DESC desc)
{
    if (desc.Width == 0 || desc.Height == 0)
    {
        printf("Invalid width or height!\n");
        return QES_ERR_INVALID_VIDEO_PARAM;
    }
    m_encoderSpecInfo.encodePars.Width = ALIGN16(desc.Width);
    m_encoderSpecInfo.encodePars.Height = ALIGN16(desc.Height);

    if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        m_encoderSpecInfo.encodePars.InputFourCC = QES_FOURCC_BGR4;
    }
    else if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        m_encoderSpecInfo.encodePars.InputFourCC = QES_FOURCC_RGB4;
    }
    else if (desc.Format == DXGI_FORMAT_NV12)
    {
        m_encoderSpecInfo.encodePars.InputFourCC = QES_FOURCC_NV12;
    }
    else
    {
        printf("Invalid input format, please use RGBA|BGRA|NV12!\n");
        return QES_ERR_INVALID_VIDEO_PARAM;
    }

    return QES_ERR_NONE;
}



qesStatus CGConfig::SetEncoderSpecInfoFromJson(const json& j)
{
    if (j.contains("encoder-specific"))
    {
        m_encoderSpecInfo.outFile = j["encoder-specific"].value("outFile", "");
        if (m_encoderSpecInfo.outFile == "")
        {
            printf("Output path not set\n");
            return QES_ERR_INVALID_VIDEO_PARAM;
        }
        auto codecID = j["encoder-specific"].value("codecID", "avc");
        transform(codecID.begin(), codecID.end(), codecID.begin(), ::tolower);
        auto profile = j["encoder-specific"].value("profile", "main");
        transform(profile.begin(), profile.end(), profile.begin(), ::tolower);
        if ((codecID == "avc") || (codecID == "h264"))
        {
            m_encoderSpecInfo.encodePars.CodecID = QES_CODEC_AVC;
            if ((profile == "baseline") || (profile == "constrained baseline"))
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_AVC_CONSTRAINED_BASELINE;
            }
            else if ((profile == "mainline") || (profile == "main"))
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_AVC_MAIN;
            }
            else if (profile == "high")
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_AVC_HIGH;
            }
            else
            {
                printf("Invalid profile for avc, please use baseline|main|high!\n");
                return QES_ERR_INVALID_VIDEO_PARAM;
            }
        }
        else if ((codecID == "hevc") || (codecID == "h265"))
        {
            m_encoderSpecInfo.encodePars.CodecID = QES_CODEC_HEVC;
            if (profile == "main")
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_HEVC_MAIN;
            }
            else if (profile == "main10")
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_HEVC_MAIN10;
            }
            else if (profile == "scc")
            {
                m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_HEVC_SCC;
            }
            else
            {
                printf("Invalid profile for hevc, please use main|main10|scc!\n");
                return QES_ERR_INVALID_VIDEO_PARAM;
            }
        }
        else
        {
            printf("Invalid Codec, please use avc|hevc!\n");
            return QES_ERR_INVALID_VIDEO_PARAM;
        }
        m_encoderSpecInfo.encodePars.BitrateKbps = j["encoder-specific"].value("bitrate", 8000000) / 1000;
        m_encoderSpecInfo.encodePars.MaxBitrateKbps = j["encoder-specific"].value("bitrate", 8000000) / 1000;
        m_encoderSpecInfo.encodePars.GopPicSize = j["encoder-specific"].value("gop", 120);
        m_encoderSpecInfo.encodePars.NumOfRef = j["encoder-specific"].value("numOfRef", 1);
        m_encoderSpecInfo.encodePars.Framerate = j["encoder-specific"].value("framerate", 60);

        auto brc = j["encoder-specific"].value("rcm", "vbr");
        transform(brc.begin(), brc.end(), brc.begin(), ::tolower);
        if (brc == "vbr")
        {
            m_encoderSpecInfo.encodePars.RateControlMethod = QES_RATECONTROL_VBR;
            m_encoderSpecInfo.encodePars.IFrameQP.MinQPI = j["encoder-specific"].value("minQPI", 23);
            m_encoderSpecInfo.encodePars.PFrameQP.MinQPP = j["encoder-specific"].value("minQPP", 23);
            m_encoderSpecInfo.encodePars.BFrameQP.MinQPB = j["encoder-specific"].value("minQPB", 23);
            m_encoderSpecInfo.encodePars.IFrameQP.MaxQPI = j["encoder-specific"].value("maxQPI", 23);
            m_encoderSpecInfo.encodePars.PFrameQP.MaxQPP = j["encoder-specific"].value("maxQPP", 23);
            m_encoderSpecInfo.encodePars.BFrameQP.MaxQPB = j["encoder-specific"].value("maxQPB", 23);
        }
        else if (brc == "cbr")
        {
            m_encoderSpecInfo.encodePars.RateControlMethod = QES_RATECONTROL_CBR;
        }
        else if (brc == "cqp")
        {
            m_encoderSpecInfo.encodePars.RateControlMethod = QES_RATECONTROL_CQP;
            m_encoderSpecInfo.encodePars.IFrameQP.QPI = j["encoder-specific"].value("QPI", 23);
            m_encoderSpecInfo.encodePars.PFrameQP.QPP = j["encoder-specific"].value("QPP", 23);
            m_encoderSpecInfo.encodePars.BFrameQP.QPB = j["encoder-specific"].value("QPB", 23);
        }
        else
        {
            printf("Invalid bitrate mode, please use vbr|cbr|cqp!\n");
            return QES_ERR_INVALID_VIDEO_PARAM;
        }
        m_encoderSpecInfo.roiSettingFile = j["encoder-specific"].value("roiSettingPar", "");
        m_encoderSpecInfo.refListSettingFile = j["encoder-specific"].value("refListSettingPar", "");
        m_encoderSpecInfo.targetFrameSizeSettingFile = j["encoder-specific"].value("targetFrameSizeSettingPar", "");
    }
    else
    {
        printf("\"encoder-specific\" not contained in config, using default settings!\n");
        m_encoderSpecInfo.outFile = "output.h264";
        m_encoderSpecInfo.encodePars.CodecID = QES_CODEC_AVC;
        m_encoderSpecInfo.encodePars.CodecProfile = QES_PROFILE_AVC_MAIN;
        m_encoderSpecInfo.encodePars.BitrateKbps = 8000;
        m_encoderSpecInfo.encodePars.MaxBitrateKbps = 8000;
        m_encoderSpecInfo.encodePars.RateControlMethod = QES_RATECONTROL_VBR;
        m_encoderSpecInfo.encodePars.NumOfRef = 1;
        m_encoderSpecInfo.encodePars.Framerate = 60;
        m_encoderSpecInfo.encodePars.GopPicSize = 120;
        m_encoderSpecInfo.roiSettingFile = "";
        m_encoderSpecInfo.refListSettingFile = "";
        m_encoderSpecInfo.targetFrameSizeSettingFile = "";
    }

    return QES_ERR_NONE;
}



qesStatus CGConfig::Init(const char* configFile)
{
    qesStatus qesSts = QES_ERR_NONE;

    json j;
    ifstream jfile(configFile);
    jfile >> j;

    qesSts = SetEncoderSpecInfoFromJson(j);
    if (qesSts != QES_ERR_NONE)
    {
        return qesSts;
    }


    return qesSts;
}



qesEncodeParameters* CGConfig::GetEncoderSpecPars()
{
    return &m_encoderSpecInfo.encodePars;
}

string CGConfig::GetOutputFileName()
{
    return m_encoderSpecInfo.outFile;
}