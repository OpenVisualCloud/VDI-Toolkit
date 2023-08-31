#include <vector>
#include <string>

#include <winsock2.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <atlbase.h>
#include <chrono>

#include "qes_core.h"
#include "config.h"
#include "buffer.h"

class AccurateClock
{
public:
    AccurateClock(__int64 frequence = 1000) :_frequence_(frequence)
    {
        QueryPerformanceFrequency(&_system_freq_);
        QueryPerformanceCounter(&_base_tick_);
    }
    ~AccurateClock() {}

    __int64 getTick() const
    {
        LARGE_INTEGER current_tick;
        QueryPerformanceCounter(&current_tick);
        return (_frequence_ * (current_tick.QuadPart - _base_tick_.QuadPart) / _system_freq_.QuadPart);
    }

private:
    LARGE_INTEGER _base_tick_;
    LARGE_INTEGER _system_freq_;
    __int64 _frequence_;
};

class EncodeSample
{
public:
    void PrintHelp();
    qesStatus CreateDX11Device(uint32_t nAdapterNum);
    bool ParseInputString(vector<wchar_t*>& argList);
    bool OpenInputOutputFiles();

    qesStatus Init();
    qesStatus InitExternalDevice(uint32_t adapterNum);
    qesStatus ReInitOutput(uint32_t adapterNum);
    qesStatus Run();
    qesStatus Close();
    qesStatus BindQueue(BufferQueue* pqueue);
    qesStatus CaptureTexture();
    qesStatus CaptureThread(int stat);
    qesStatus SetLoop(bool Loop);
    CComPtr<ID3D11Device> GetDevice() { return m_pSourceDxDevice; }
    CComPtr<IDXGIOutputDuplication> GetDXDup() { return m_pDxDup; }
    qesStatus ProcessOneFrame();

    int GetFrameNum() { return m_numFrames; }
    bool ifLoop() { return m_isLoop; }
    void setClock(AccurateClock* pclock) { m_clock = pclock; }
    bool ifCancel() { return m_needToCancel; }
    bool setQueueSize();

    CComPtr<ID3D11Device> m_pSourceDxDevice;
    CComPtr<IDXGIOutputDuplication> m_pDxDup;
    CComPtr<ID3D11Texture2D> m_BackupTex;
    //qesCoreFactory m_CoreFactory;
    //qesCoreInterface* m_core;
    qesCore* m_core;
    qesEncode* m_encoder;
    CGConfig m_config;
    wstring m_configFile;
    bool m_isInited;
    uint32_t m_adapterNum;
    uint32_t m_numFrames;
    uint32_t m_frameNum;
    uint32_t m_CurrFrameNum;
    uint32_t m_fixFrameRate;
    bool m_cNeedToCancel = false;
    bool m_isLoop = false;


    qesStatus m_sts = QES_ERR_NONE;
    int m_frametime = 0;
    __int64 m_InitTime = 0;
    __int64 m_CurrTime = 0;
    int m_duration = (m_CurrTime - m_InitTime);
    __int64 m_FStartTime = 0;
    int m_Fduration = (m_CurrTime - m_InitTime);
    bool m_needToCancel = false;


    ofstream m_outFile;
    BufferQueue* m_pqueue;
    AccurateClock* m_clock;
};