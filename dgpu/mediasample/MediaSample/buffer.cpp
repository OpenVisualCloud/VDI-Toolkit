#include "buffer.h"

BufferQueue::BufferQueue() {
    m_Size = 0;
    m_maxSize = 6;
    std::unique_lock<std::mutex> lock(queue_mutex_);
}

bool BufferQueue::EnqueueBuffer(Buffer buf) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    m_SourceQueue.push(buf);
    m_Size += 1;
    if (m_Size > m_maxSize) {
        m_SourceQueue.pop();
        m_Size -= 1;
    }
    return true;
}

bool BufferQueue::SetMaxSize(int psize) {
    m_maxSize = psize;
    return true;
}

Buffer BufferQueue::DequeueBuffer() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    Buffer buf{};
    if (!m_SourceQueue.empty()) {
        buf = m_SourceQueue.front();
        if (m_Size > 1) {
            m_SourceQueue.pop();
            m_Size -= 1;
        }
        buf.width = GetSystemMetrics(SM_CXSCREEN);
    }
    else {
        buf.width = GetSystemMetrics(SM_CYSCREEN);
    }
    return buf;
}

Buffer BufferQueue::AcquireBuffer() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    Buffer buf{};
    if (!m_SourceQueue.empty()) {
        buf = m_SourceQueue.front();
        buf.width = GetSystemMetrics(SM_CXSCREEN);
        //printf("a buffer is dequeued");
    }
    else {
        buf.width = GetSystemMetrics(SM_CYSCREEN);
    }
    return buf;
}