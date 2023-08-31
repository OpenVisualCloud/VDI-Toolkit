/*
* Copyright (c) 2021, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/


#pragma once

#include "qes_defs.h"
#include "qes_structures.h"
#include "qes_core.h"

#include <vector>
#include <string>


enum ResetEncodeParamsTypes {
    ENCODE_BITRATE,
    ENCODE_ROI,
    ENCODE_REFLIST,
    ENCODE_KEYFRAME
};

enum {
    QES_CODEC_AVC = QES_MAKEFOURCC('A', 'V', 'C', ' '), /*!< AVC, H.264, or MPEG-4, part 10 codec. */
    QES_CODEC_HEVC = QES_MAKEFOURCC('H', 'E', 'V', 'C'), /*!< HEVC codec. */
    QES_CODEC_AV1 = QES_MAKEFOURCC('A', 'V', '1', ' ')  /*!< AV1 codec. */
};

enum {
    QES_FOURCC_NV12 = QES_MAKEFOURCC('N', 'V', '1', '2'),   /*!< NV12 color planes. Native format for 4:2:0/8b Gen hardware implementation. */
    QES_FOURCC_RGB4 = QES_MAKEFOURCC('R', 'G', 'B', '4'),   /*!< RGB4 (RGB32) color planes. BGRA is the order, ‘B’ is 8 MSBs, then 8 bits for ‘G’ channel, then ‘R’ and ‘A’ channels. */
    QES_FOURCC_BGR4 = QES_MAKEFOURCC('B', 'G', 'R', '4'), /*!< RGBA color format. It is similar to MFX_FOURCC_RGB4 but with different order of channels. ‘R’ is 8 MSBs, then 8 bits for ‘G’ channel, then ‘B’ and ‘A’ channels. */
};

enum {
    QES_PROFILE_UNKNOWN = 0, /*!< Unspecified profile. */
    QES_LEVEL_UNKNOWN = 0, /*!< Unspecified level. */

    /*! @{ */
    /* Combined with H.264 profile these flags impose additional constrains. See H.264 specification for the list of constrains. */
    QES_PROFILE_AVC_CONSTRAINT_SET0 = (0x100 << 0),
    QES_PROFILE_AVC_CONSTRAINT_SET1 = (0x100 << 1),
    QES_PROFILE_AVC_CONSTRAINT_SET2 = (0x100 << 2),
    QES_PROFILE_AVC_CONSTRAINT_SET3 = (0x100 << 3),
    QES_PROFILE_AVC_CONSTRAINT_SET4 = (0x100 << 4),
    QES_PROFILE_AVC_CONSTRAINT_SET5 = (0x100 << 5),
    /*! @} */

    /*! @{ */
    /* H.264 Profiles. */
    QES_PROFILE_AVC_BASELINE = 66,
    QES_PROFILE_AVC_MAIN = 77,
    QES_PROFILE_AVC_EXTENDED = 88,
    QES_PROFILE_AVC_HIGH = 100,
    QES_PROFILE_AVC_HIGH10 = 110,
    QES_PROFILE_AVC_HIGH_422 = 122,
    QES_PROFILE_AVC_CONSTRAINED_BASELINE = QES_PROFILE_AVC_BASELINE + QES_PROFILE_AVC_CONSTRAINT_SET1,
    QES_PROFILE_AVC_CONSTRAINED_HIGH = QES_PROFILE_AVC_HIGH + QES_PROFILE_AVC_CONSTRAINT_SET4
    + QES_PROFILE_AVC_CONSTRAINT_SET5,
    QES_PROFILE_AVC_PROGRESSIVE_HIGH = QES_PROFILE_AVC_HIGH + QES_PROFILE_AVC_CONSTRAINT_SET4,
    /*! @} */

    /*! @{ */
    /* H.264 level 1-1.3 */
    QES_LEVEL_AVC_1 = 10,
    QES_LEVEL_AVC_1b = 9,
    QES_LEVEL_AVC_11 = 11,
    QES_LEVEL_AVC_12 = 12,
    QES_LEVEL_AVC_13 = 13,
    /*! @} */
    /*! @{ */
    /* H.264 level 2-2.2 */
    QES_LEVEL_AVC_2 = 20,
    QES_LEVEL_AVC_21 = 21,
    QES_LEVEL_AVC_22 = 22,
    /*! @} */
    /*! @{ */
    /* H.264 level 3-3.2 */
    QES_LEVEL_AVC_3 = 30,
    QES_LEVEL_AVC_31 = 31,
    QES_LEVEL_AVC_32 = 32,
    /*! @} */
    /*! @{ */
    /* H.264 level 4-4.2 */
    QES_LEVEL_AVC_4 = 40,
    QES_LEVEL_AVC_41 = 41,
    QES_LEVEL_AVC_42 = 42,
    /*! @} */
    /*! @{ */
    /* H.264 level 5-5.2 */
    QES_LEVEL_AVC_5 = 50,
    QES_LEVEL_AVC_51 = 51,
    QES_LEVEL_AVC_52 = 52,
    /*! @} */
    /*! @{ */
    /* H.264 level 6-6.2 */
    QES_LEVEL_AVC_6 = 60,
    QES_LEVEL_AVC_61 = 61,
    QES_LEVEL_AVC_62 = 62,
    /*! @} */

    /*! @{ */
    /* HEVC profiles */
    QES_PROFILE_HEVC_MAIN = 1,
    QES_PROFILE_HEVC_MAIN10 = 2,
    QES_PROFILE_HEVC_MAINSP = 3,
    QES_PROFILE_HEVC_REXT = 4,
    QES_PROFILE_HEVC_SCC = 9,
    /*! @} */

    /*! @{ */
    /* HEVC levels */
    QES_LEVEL_HEVC_1 = 10,
    QES_LEVEL_HEVC_2 = 20,
    QES_LEVEL_HEVC_21 = 21,
    QES_LEVEL_HEVC_3 = 30,
    QES_LEVEL_HEVC_31 = 31,
    QES_LEVEL_HEVC_4 = 40,
    QES_LEVEL_HEVC_41 = 41,
    QES_LEVEL_HEVC_5 = 50,
    QES_LEVEL_HEVC_51 = 51,
    QES_LEVEL_HEVC_52 = 52,
    QES_LEVEL_HEVC_6 = 60,
    QES_LEVEL_HEVC_61 = 61,
    QES_LEVEL_HEVC_62 = 62,
    /*! @} */

    /*! @{ */
    /* AV1 Profiles */
    QES_PROFILE_AV1_MAIN = 1,
    QES_PROFILE_AV1_HIGH = 2,
    QES_PROFILE_AV1_PRO = 3,
    /*! @} */

    /*! @{ */
    /* AV1 Levels */
    QES_LEVEL_AV1_2 = 20,
    QES_LEVEL_AV1_21 = 21,
    QES_LEVEL_AV1_22 = 22,
    QES_LEVEL_AV1_23 = 23,
    QES_LEVEL_AV1_3 = 30,
    QES_LEVEL_AV1_31 = 31,
    QES_LEVEL_AV1_32 = 32,
    QES_LEVEL_AV1_33 = 33,
    QES_LEVEL_AV1_4 = 40,
    QES_LEVEL_AV1_41 = 41,
    QES_LEVEL_AV1_42 = 42,
    QES_LEVEL_AV1_43 = 43,
    QES_LEVEL_AV1_5 = 50,
    QES_LEVEL_AV1_51 = 51,
    QES_LEVEL_AV1_52 = 52,
    QES_LEVEL_AV1_53 = 53,
    QES_LEVEL_AV1_6 = 60,
    QES_LEVEL_AV1_61 = 61,
    QES_LEVEL_AV1_62 = 62,
    QES_LEVEL_AV1_63 = 63,
    QES_LEVEL_AV1_7 = 70,
    QES_LEVEL_AV1_71 = 71,
    QES_LEVEL_AV1_72 = 72,
    QES_LEVEL_AV1_73 = 73,
    /*! @} */
};

enum {
    QES_RATECONTROL_CBR = 1, /*!< Use the constant bitrate control algorithm. */
    QES_RATECONTROL_VBR = 2, /*!< Use the variable bitrate control algorithm. */
    QES_RATECONTROL_CQP = 3, /*!< Use the constant quantization parameter algorithm. */
};

typedef struct
{
    unsigned int             CodecID;
    unsigned int             InputFourCC;
    unsigned int             Framerate;
    unsigned int             BitrateKbps;
    unsigned int             MaxBitrateKbps;
    unsigned short           Width; // source picture width
    unsigned short           Height; // source picture height
    unsigned short           NumOfRef;
    unsigned short           TargetUsage;
    unsigned short           GopPicSize;
    unsigned short           RateControlMethod;
    unsigned short           CodecProfile;
    unsigned short           TransferMatrix;
    union
    {
        struct
        {
            unsigned short   QPI;
        };
        struct
        {
            unsigned char    MinQPI;
            unsigned char    MaxQPI;
        };
    } IFrameQP;
    union
    {
        struct
        {
            unsigned short   QPP;
        };
        struct
        {
            unsigned char    MinQPP;
            unsigned char    MaxQPP;
        };
    } PFrameQP;
    union
    {
        struct
        {
            unsigned short   QPB;
        };
        struct
        {
            unsigned char    MinQPB;
            unsigned char    MaxQPB;
        };
    } BFrameQP;
    std::vector<std::string> FrameROI;
    std::vector<std::string> FrameRefList;
} qesEncodeParameters;

class QESLIB_API qesBitstream {
public:
    virtual unsigned char* GetBitstreamData() = 0;
    virtual unsigned int   GetBitstreamDataLength() = 0;
};

class QESLIB_API qesEncode {
public:
    static qesEncode* CreateInstance();
    static void DestroyInstance(qesEncode* pEncode);

public:
    virtual qesStatus EncodeInit(qesEncodeParameters* pParams) = 0;
    virtual qesStatus EncodeFrame(qesFrame* pFrame) = 0;
    virtual qesStatus GetBitstream(qesBitstream** pBS) = 0;
    virtual qesStatus SetCoreModule(qesCore* pCore) = 0;
    virtual qesStatus FlushBuffer() = 0;
    virtual qesStatus ResetEncodeParameters(ResetEncodeParamsTypes resetType, qesEncodeParameters* pParams) = 0;
    virtual qesStatus GetSPSPPS(unsigned char* &spsBuffer, unsigned short &spsBufferSize, unsigned char* &ppsBuffer, unsigned short &ppsBufferSize) = 0;
    virtual qesStatus GetVPS(unsigned char* &vpsBuffer, unsigned short &vpsBufferSize) = 0;
};