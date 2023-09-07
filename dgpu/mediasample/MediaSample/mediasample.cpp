
#pragma comment(lib,"Winmm.lib")

#include <codecvt>
#include <vector>
#include <string>
#include <iterator>
#include <future>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <winsock2.h>
#include <winuser.h>
#include <dxgi1_6.h>
#include "d3d9.h"

#include "mediasample.hpp"

qesStatus EncodeSample::CreateDX11Device(uint32_t nAdapterNum)
{
    HRESULT hres = S_OK;
    CComPtr<ID3D11Device>  pDevice;
    CComPtr<ID3D11DeviceContext>  pDeviceCtx;
    static D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL pFeatureLevelsOut;
    CComPtr<IDXGIFactory6> pDXGIFactory;
    CComPtr<IDXGIAdapter> pAdapter;

    hres = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)(&pDXGIFactory));
    if (FAILED(hres))
        return QES_ERR_INVALID_HANDLE;


    hres = pDXGIFactory->EnumAdapters(nAdapterNum, &pAdapter);
    if (FAILED(hres))
        return QES_ERR_DEVICE_FAILED;

    hres = D3D11CreateDevice(pAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        FeatureLevels,
        sizeof(FeatureLevels) / sizeof(FeatureLevels[0]),
        D3D11_SDK_VERSION,
        &pDevice,
        &pFeatureLevelsOut,
        &pDeviceCtx);

    if (FAILED(hres))
        return QES_ERR_INVALID_HANDLE;

    // turn on multithreading for the Context
    CComQIPtr<ID3D10Multithread> p_mt(pDeviceCtx);

    // don't care about previous state of MultithreadProtection
    if (p_mt)
        (void)p_mt->SetMultithreadProtected(true);

    m_pSourceDxDevice = NULL;
    m_pSourceDxDevice = pDevice;

    CComPtr<IDXGIOutput> dxgi_output;
    hres = pAdapter->EnumOutputs(nAdapterNum, &dxgi_output);
    if (FAILED(hres))
        return QES_ERR_INVALID_HANDLE;

    CComPtr<IDXGIOutput1> dxgi_output1;
    hres = dxgi_output->QueryInterface(__uuidof(dxgi_output1), reinterpret_cast<void**>(&dxgi_output1));
    if (FAILED(hres))
        return QES_ERR_INVALID_HANDLE;

    m_pDxDup = NULL;
    hres = dxgi_output1->DuplicateOutput(pDevice, &m_pDxDup);
    if (FAILED(hres))
        return QES_ERR_INVALID_HANDLE;

    return QES_ERR_NONE;
}

qesStatus EncodeSample::ReInitOutput(uint32_t nAdapterNum)
{
    qesStatus ret = QES_ERR_NONE;
    HRESULT hres = S_OK;

    CComPtr<IDXGIFactory6> pDXGIFactory;
    CComPtr<IDXGIAdapter> pAdapter;

    // release duplicate output interface 
    m_pDxDup = NULL;

    hres = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)(&pDXGIFactory));
    if (FAILED(hres)) {
        printf("CreateDXGIFactory1() failed!\n");
        ret = QES_ERR_INVALID_HANDLE;

    }
    else {

        hres = pDXGIFactory->EnumAdapters(nAdapterNum, &pAdapter);
        if (FAILED(hres)) {
            printf("EnumAdapters() failed!\n");
            ret = QES_ERR_INVALID_HANDLE;
        }
        else {

            CComPtr<IDXGIOutput> dxgi_output;
            hres = pAdapter->EnumOutputs(0, &dxgi_output);
            if (FAILED(hres)) {
                printf("EnumOutputs() failed!\n");
                ret = QES_ERR_INVALID_HANDLE;
            }
            else {
                CComPtr<IDXGIOutput1> dxgi_output1;
                hres = dxgi_output->QueryInterface(__uuidof(dxgi_output1), reinterpret_cast<void**>(&dxgi_output1));
                if (FAILED(hres)) {
                    printf("QueryInterface() for IDXGIOutput1 failed!\n");
                    ret = QES_ERR_INVALID_HANDLE;
                }
                else {
                    hres = dxgi_output1->DuplicateOutput(m_pSourceDxDevice, &m_pDxDup);
                    if (FAILED(hres)) {
                        printf("DuplicateOutput() failed!\n");
                        ret = QES_ERR_INVALID_HANDLE;
                    }
                    else {
                        ret = QES_ERR_NONE;
                    }
                }
            }
        }
    }
    return ret;
}

bool EncodeSample::OpenInputOutputFiles()
{
    if (m_config.GetOutputFileName().size() != 0)
    {
        m_outFile.open(m_config.GetOutputFileName(), ios::out | ios::binary);
        if (!m_outFile.is_open())
        {
            printf("Opening output file failed!\n");
            return false;
        }
    }

    return true;
}

bool EncodeSample::setQueueSize() {
    bool output;
    if (m_fixFrameRate > 0 && m_fixFrameRate < 10) {
        output = m_pqueue->SetMaxSize(m_fixFrameRate);
        printf("queue length is set to: %d \n", m_pqueue->GetMaxSize());
    }
    else {
        output = m_pqueue->SetMaxSize(10);
        printf("buffer queue length is set to 10 by default. \n");
    }
    return output;
}

void EncodeSample::PrintHelp()
{
    printf("Usage: cg_engine_lite.exe -cfg ConfigureFile [<options>]\n");
    printf("Options: \n");
    printf("    [-adapterNum number] - specifies adpter number for processing, starts from 0\n");
    printf("    [-encodeonly 1or0] - 1 for capture several frames first then encode these frames repeatedly 0 or no input means capture & encode simultaneously. 1 shall be used for encode perf test.\n");
    printf("    [-fixFPS number] - specifies a FPS limitation. \n");
    printf("    [-n number] - specifies a total number of frames to be encoded. \n");
    printf("Examples: cg_engine_lite.exe -cfg config.json \n");
}

bool EncodeSample::ParseInputString(vector<wchar_t*>& argList)
{
    if (1 >= argList.size())
    {
        PrintHelp();
        return false;
    }

    size_t nArgNum = argList.size();
    m_adapterNum = 0;
    m_numFrames = 0;
    m_isInited = FALSE;
    m_fixFrameRate = 0;

    for (size_t i = 1; i < (size_t)nArgNum; i++)
    {
        if (0 == wcscmp(argList[i], L"-cfg"))
        {
            if (++i == nArgNum)
            {
                PrintHelp();
                return false;
            }
            m_configFile.assign(argList[i]);
        }
        if (0 == wcscmp(argList[i], L"-adapterNum"))
        {
            if (++i == nArgNum)
            {
                PrintHelp();
                return false;
            }
            m_adapterNum = _wtoi(argList[i]);
        }
        if (0 == wcscmp(argList[i], L"-fixFPS"))
        {
            if (++i == nArgNum)
            {
                PrintHelp();
                return false;
            }
            m_fixFrameRate = _wtoi(argList[i]);
        }
        if (0 == wcscmp(argList[i], L"-n"))
        {
            if (++i == nArgNum)
            {
                PrintHelp();
                return false;
            }
            m_numFrames = _wtoi(argList[i]);
        }
        if (0 == wcscmp(argList[i], L"-encodeonly"))
        {
            if (++i == nArgNum)
            {
                PrintHelp();
                return false;
            }
            int ifLoop = _wtoi(argList[i]);
            if (ifLoop == 1) {
                m_isLoop = true;
            }
            else if (ifLoop == 0) {
                m_isLoop = false;
            }
            else {
                PrintHelp();
                return false;
            }
        }
    }

    return true;
}

qesStatus EncodeSample::Init()
{
    wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    string cfg = converter.to_bytes(m_configFile);
    m_config.Init(cfg.c_str());

    if (!OpenInputOutputFiles())
    {
        printf("OpenInputOutputFiles failed!\n");
        return QES_ERR_ABORTED;
    }

    //qesDeviceType device = QES_HANDLE_D3D11_DEVICE;
    //m_core = m_CoreFactory.CreateCoreInstance(device);
    m_core = m_core->CreateInstance();
    m_encoder = m_encoder->CreateInstance();
    m_encoder->SetCoreModule(m_core);

    qesStatus sts = InitExternalDevice(m_adapterNum);
    if (sts != QES_ERR_NONE)
    {
        printf("InitExternalDevice failed!\n");
        return sts;
    }
#if 0
    sts = m_encoder->EncodeInit(m_encoderPars.GetEncoderSpecPars());
    if (sts != QES_ERR_NONE)
    {
        printf("EncodeInit failed!\n");
        return sts;
    }
#endif

    //m_sockServer = sock_server_init(SOCK_CONN_TYPE_INET_SOCK, 24432);

    return QES_ERR_NONE;
}

qesStatus EncodeSample::SetLoop(bool Loop) {
    m_isLoop = Loop;
    return QES_ERR_NONE;
}

qesStatus EncodeSample::InitExternalDevice(uint32_t adapterNum)
{
    qesStatus sts = QES_ERR_NONE;

    sts = CreateDX11Device(m_adapterNum);
    printf("Adapter num is %d", m_adapterNum);
    if (sts != QES_ERR_NONE)
    {
        printf("Failed to create device!\n");
        return QES_ERR_DEVICE_FAILED;
    }

    qesDeviceHandle pDev{};
    pDev.handleType = QES_HANDLE_D3D11_DEVICE;
    pDev.ptr = m_pSourceDxDevice.p;

    sts = m_core->SetDevice(&pDev);
    if (sts != QES_ERR_NONE)
    {
        printf("Failed to init device from Core!\n");
        return sts;
    }

    return QES_ERR_NONE;
}

qesStatus EncodeSample::BindQueue(BufferQueue* pqueue) {
    if (m_pqueue == nullptr) {
        m_pqueue = pqueue;
        return QES_ERR_NONE;
    }
    return QES_ERR_NOT_FOUND;
}

qesStatus EncodeSample::CaptureTexture() {
    HRESULT hr = S_OK;

    qesStatus sts = QES_ERR_NONE;
    CComPtr<ID3D11Texture2D> captex;

    CComPtr<IDXGIResource> dxgi_res;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    if (m_pDxDup) {
        hr = m_pDxDup->AcquireNextFrame(50, &frame_info, &dxgi_res);
        //printf("a frame is acquired \n");
    }
    else {
        hr = DXGI_ERROR_ACCESS_LOST;
    }


    CComPtr<ID3D11DeviceContext> con = NULL;
    m_pSourceDxDevice->GetImmediateContext(&con);


    if (FAILED(hr)) {
        //printf("yohoho \n");
        bool re_capture = false;
        switch (hr) {
        case DXGI_ERROR_WAIT_TIMEOUT:
            // printf("AcquireNextFrame returns DXGI_ERROR_WAIT_TIMEOUT!\n");
            // continue to next capture
            re_capture = true;
            return QES_ERR_INVALID_HANDLE;

        case DXGI_ERROR_ACCESS_LOST:
        case DXGI_ERROR_INVALID_CALL:
            printf("AcquireNextFrame returns DXGI_ERROR_DEVICE_FAILED!\n");
            // release old  IDXGIOutputDuplication interface and create a new one 
            sts = ReInitOutput(m_adapterNum);
            if (sts != QES_ERR_NONE) {
                printf("ReInitOutput() failed and returned %d!\n", sts);
            }
            // continue to next capture
            re_capture = true;
            return QES_ERR_INVALID_HANDLE;

        case E_INVALIDARG:
        default:
            printf("AcquireNextFrame returns error %x\n", hr);
            // Should we continue next acquire frame or directly return ?
            return QES_ERR_INVALID_HANDLE;
        }

        if (re_capture) {
        }
        else {
            return QES_ERR_DEVICE_FAILED;
        }
    }

    if (dxgi_res != NULL) {
        hr = dxgi_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&captex));
        if (FAILED(hr))
        {
            printf("QueryInterface for ID3D11Texture2D failed");
            return QES_ERR_INVALID_HANDLE;
        }

        CComPtr<ID3D11Texture2D> backupTex;
        D3D11_TEXTURE2D_DESC desc{};
        Buffer buf{};

        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Width = GetSystemMetrics(SM_CXSCREEN);
        desc.Height = GetSystemMetrics(SM_CYSCREEN);

        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ArraySize = 1;
        desc.MipLevels = 1;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;



        if (m_pqueue->GetSize() < m_pqueue->GetMaxSize()) {
            m_pSourceDxDevice->CreateTexture2D(&desc, nullptr, &backupTex);
            con->CopyResource(backupTex, captex);
            buf.ptex_hdl = backupTex;
            m_BackupTex = backupTex;
            m_pqueue->EnqueueBuffer(buf);
        }
        else {
            buf = m_pqueue->DequeueBuffer();
            backupTex = buf.ptex_hdl;
            con->CopyResource(backupTex, captex);
            m_BackupTex = backupTex;
            m_pqueue->EnqueueBuffer(buf);
        }
        m_pDxDup->ReleaseFrame();

        //D3D11_TEXTURE2D_DESC frame_desc;
        //textureDTE->GetDesc(&frame_desc);
    }
    else {
        //Buffer* buf = new Buffer();
        //buf->ptex_hdl = m_BackupTex;
        //m_pqueue->EnqueueBuffer(*buf);
    }
    return QES_ERR_NONE;
}

qesStatus EncodeSample::ProcessOneFrame() {
    if (m_cNeedToCancel == true) {
        printf("capture is finished \n");
    }
    qesFrame* pIFrame;
    CComPtr<ID3D11Texture2D> textureDTE;

    // prepare input surface
    {
        Buffer m_buf;
        // Get new frame         
        if (m_isLoop) {
            m_buf = m_pqueue->DequeueBuffer();
            m_pqueue->EnqueueBuffer(m_buf);
        }
        else {
            m_buf = m_pqueue->AcquireBuffer();
        }
        int testWidth = GetSystemMetrics(SM_CXSCREEN);
        if (m_buf.width == testWidth) {
            textureDTE = m_buf.ptex_hdl;
        }
        else {
            printf("nothing to encode! \n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return QES_ERR_ABORTED;
        }
        //do not use nullptr to register texture
        if (textureDTE == NULL) {
            printf("need another texture! \n");
            return QES_ERR_ABORTED;
        }

        HRESULT hr = S_OK;

        if (!m_isInited)
        {
            D3D11_TEXTURE2D_DESC frame_desc;
            textureDTE->GetDesc(&frame_desc);
            m_sts = m_config.SetEncoderFrameInfoFromDxgiDesc(frame_desc);
            if (m_sts != QES_ERR_NONE)
            {
                printf("SetInputInfoFromDxgiDesc failed!\n");
                return m_sts;
            }
            m_sts = m_encoder->EncodeInit(m_config.GetEncoderSpecPars());
            if (m_sts != QES_ERR_NONE)
            {
                printf("EncodeInit failed!\n");
                return m_sts;
            }
            m_isInited = TRUE;
            //m_pDxDup->ReleaseFrame();
            return QES_ERR_ABORTED;
        }



        qesTextureHandle handle{};
        handle.ptr = textureDTE.p;
        handle.handleType = QES_DX11_TEXTURE;
        //handle.handleType = QES_D3D11_TEXTURE;
        m_sts = m_core->RegisterTextureToEncoder(&handle, &pIFrame);
        if (m_sts != QES_ERR_NONE)
        {
            printf("RegisterTextureToEncoder failed!\n");
            return m_sts;
        }
    }
    m_sts = m_encoder->EncodeFrame(pIFrame);

    if (m_sts != QES_ERR_NONE && m_sts != QES_ERR_MORE_DATA)
    {
        printf("EncodeFrame failed!\n");
        return m_sts;
    }

    if (m_sts == QES_ERR_NONE)
    {
        do
        {
            auto TestInit = std::chrono::steady_clock::now();
            qesBitstream* pbs;
            m_sts = m_encoder->GetBitstream(&pbs);
            //printf("bitstream get\n");
            if (m_sts == QES_ERR_NONE || m_sts == QES_ERR_NONE_PARTIAL_OUTPUT) {

                if (m_outFile.is_open()) {
                    m_outFile.write((char*)pbs->GetBitstreamData(), pbs->GetBitstreamDataLength());
                }
            }
            else if (m_sts != QES_ERR_MORE_DATA) {
                printf("GetBitsStream() returns error %d", m_sts);
                break;
            }
            auto TestEnd = std::chrono::steady_clock::now();
            int Tduration = std::chrono::duration_cast<std::chrono::milliseconds>(TestEnd - TestInit).count();
            //if (Tduration > 15) {
            //    printf("GetBitstream costs %d ms. \n", Tduration);
            //}


        } while (m_sts == QES_ERR_NONE_PARTIAL_OUTPUT);
    }

    m_core->UnregisterTexture(pIFrame);
    //m_pDxDup->ReleaseFrame();
    m_frameNum++;
    //printf("Frame number %d encoded.\n", m_frameNum);

    if (m_numFrames) {
        if (m_frameNum > m_numFrames) {
            m_needToCancel = true;
        }
    }
    return QES_ERR_NONE;
}

int CALLBACK TimeEvent(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dwl, DWORD dw2)
{

    return 1;
}

qesStatus EncodeSample::Run()
{
    m_frameNum = 0;
    qesStatus sts = QES_ERR_NONE;
    m_frametime = 0;
    m_InitTime = m_clock->getTick();
    m_CurrTime = m_clock->getTick();
    m_duration = m_CurrTime - m_InitTime;

    m_FStartTime = m_clock->getTick();
    m_Fduration = m_CurrTime - m_InitTime;

    if (m_fixFrameRate <= 0) {
        printf("Fix frame rate is not detected, starting unlimited mode... \n");
        m_frametime = 0;
    }
    else {
        printf("Detected frame rate fix. Will fix frame rate to %d FPS... \n", m_fixFrameRate);
        m_frametime = (1000 / m_fixFrameRate);
        printf("frametime is %d \n", m_frametime);
    }
    m_needToCancel = false;

    if (m_frametime > 0) {
        while (!m_needToCancel)
        {
            m_FStartTime = m_clock->getTick();
            ProcessOneFrame();
            while (m_clock->getTick() < m_FStartTime + m_frametime) {
                //Sleep(1);
            }
            m_CurrTime = m_clock->getTick();
            m_duration = m_CurrTime - m_InitTime;
            if (m_duration > 1000) {
                printf("%d frames encoded in this sec.\n", m_frameNum - m_CurrFrameNum);
                m_CurrFrameNum = m_frameNum;
                m_InitTime = m_CurrTime;
            }
        }
    }
    else
    {
        while (!m_needToCancel)
        {
            ProcessOneFrame();

            m_CurrTime = m_clock->getTick();
            m_duration = m_CurrTime - m_InitTime;
            if (m_duration > 1000) {
                printf("%d frames encoded in this sec. \n", m_frameNum - m_CurrFrameNum);
                m_CurrFrameNum = m_frameNum;
                m_InitTime = m_CurrTime;
            }
        }
    }


    return QES_ERR_NONE;
}


qesStatus EncodeSample::CaptureThread(int stat) {
    m_cNeedToCancel = false;
    int framenumc = 0;
    while (!m_cNeedToCancel) {
        this->CaptureTexture();
        framenumc += 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        //std::cout << "frame" << framenumc << " is captured" << std::endl;
    }
    return QES_ERR_NONE;
}

qesStatus EncodeSample::Close()
{
    qesStatus sts = QES_ERR_NONE;

    m_encoder->DestroyInstance(m_encoder);
    //m_core->DestroyInstance(m_core);

    if (m_outFile.is_open())
        m_outFile.close();

    return sts;
}

int ICaptureThread(EncodeSample* pengine) {
    bool cNeedToCancel = false;
    int framenumc = 0;
    while (!cNeedToCancel) {
        pengine->CaptureTexture();
        framenumc += 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return 0;
}


int ICaptureThreadLoop(EncodeSample* pengine, int nLoop, bool isLoop) {
    bool cNeedToCancel = false;
    int framenumc = 0;
    int framenumtar = pengine->GetFrameNum();
    qesStatus sts;
    while (((framenumc < nLoop) || (!isLoop)) && (!cNeedToCancel)) {
        sts = pengine->CaptureTexture();
        if (sts == QES_ERR_NONE) {
            framenumc += 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        else {
            //printf("ahaha \n");
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        if (pengine->ifCancel()) {
            cNeedToCancel = true;
        }
    }
    printf("capture is done!!!!!\n");
    return 0;
}

int ICaptureSingleFrame(EncodeSample* pengine) {
    pengine->CaptureTexture();
    return 0;
}


int wmain(int argc, wchar_t* argv[])
{
    vector<wchar_t*> argList(argv, argv + argc);
    int result = 0;
    EncodeSample engine;
    BufferQueue bufqueue;
    AccurateClock clock;
    //bool countLoop = false;
    int countLoopNum = 10;
    //int Width{ GetSystemMetrics(SM_CXSCREEN) };
    //int Heigth{ GetSystemMetrics(SM_CYSCREEN) };
    //std::cout << Width << " X " << Heigth << std::endl;

    if (!engine.ParseInputString(argList))
    {
        printf("ParseInputString failed!\n");
        return -1;
    }

    qesStatus sts = engine.Init();
    if (sts != QES_ERR_NONE)
    {
        printf("Failed to init engine!\n");
        return -1;
    }

    //if (countLoop) {
    //    engine.SetLoop(countLoop);
    //}


    if (sts != QES_ERR_NONE)
    {
        printf("Failed to init engine!\n");
        return -1;
    }

    sts = engine.BindQueue(&bufqueue);
    if (engine.ifLoop()) {
        engine.setQueueSize();
    }
    engine.setClock(&clock);
    if (sts != QES_ERR_NONE)
    {
        printf("Failed to Connect Bufferqueue!\n");
        return -1;
    }

    bool tloop = engine.ifLoop();
    std::future<int> DDACapture = std::async(std::launch::async, ICaptureThreadLoop, &engine, countLoopNum, tloop);
    if (engine.ifLoop()) {
        DDACapture.wait();
    }
    printf("start encoding...\n");
    sts = engine.Run();

    if (sts != QES_ERR_NONE)
    {
        printf("Failed to run engine!\n");
        result = -1;
    }


    sts = engine.Close();

    if (sts != QES_ERR_NONE)
    {
        printf("Failed to close engine!\n");
        result = -1;
    }

    return result;
}
