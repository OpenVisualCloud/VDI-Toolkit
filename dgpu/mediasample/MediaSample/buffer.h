#pragma once

#include <queue>
#include <mutex>
#include <d3d11.h>
#include <atlbase.h>

struct Buffer {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    CComPtr<ID3D11Texture2D> ptex_hdl;
};

class BufferQueue {
public:
    BufferQueue();
    ~BufferQueue() = default;
    bool EnqueueBuffer(Buffer buf);
    Buffer DequeueBuffer();
    Buffer AcquireBuffer();
    int GetSize() { return m_Size; }
    int GetMaxSize() { return m_maxSize; }
    bool SetMaxSize(int psize);

private:
    int m_Size;
    int m_maxSize;
    std::queue<Buffer> m_SourceQueue;
    std::mutex queue_mutex_;
};

