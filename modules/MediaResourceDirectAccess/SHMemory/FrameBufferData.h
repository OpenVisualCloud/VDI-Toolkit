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

 */

//!
//! \file FrameBufferData.h
//! \brief define frame buffer data class and memory buffer class
//! \date 2024-04-29
//!

#ifndef _FRAME_BUFFER_DATA_H_
#define _FRAME_BUFFER_DATA_H_

#include "../utils/common.h"

VDI_NS_BEGIN

class MemoryBuffer
{
public:
    //!
    //! \brief Construct a new Mem Buffer object
    //!
    //!
    MemoryBuffer()
    {
        m_bufId = 0;
        m_memOffset = 0;
        m_stateOffset = 0;
        m_bufPtr = nullptr;
        m_size = 0;
        m_occupiedSize = 0;
        m_state = BufferState::BUFFER_STATE_NONE;
    }
    //!
    //! \brief Destroy the Mem Buffer object
    //!
    //!
    virtual ~MemoryBuffer()
    {
        m_bufId = 0;
        m_memOffset = 0;
        m_stateOffset = 0;
        m_bufPtr = nullptr;
        m_size = 0;
        m_occupiedSize = 0;
        m_state = BufferState::BUFFER_STATE_NONE;
    }

    //!
    //! \brief Create a Mem Buffer object from a MemBufferItem struct
    //!
    //! \param [in] pItem
    //! \return MRDAStatus
    //!
    inline MRDAStatus CreateMemBuffer(MemBufferItem *pItem)
    {
        if (pItem == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "invalid mem buffer item!");
            return MRDA_STATUS_INVALID_DATA;
        }
        m_bufId = pItem->buf_id;
        m_memOffset = pItem->mem_offset;
        m_stateOffset = pItem->state_offset;
        m_bufPtr = pItem->buf_ptr;
        m_size = pItem->size;
        m_occupiedSize = pItem->occupied_size;
        m_state = pItem->state;
        return MRDA_STATUS_SUCCESS;
    }

    //!
    //! \brief Get/Set buf id
    //!
    //! \return uint32_t
    //!
    inline uint32_t BufId() { return m_bufId; }
    inline void SetBufId(uint32_t bufId) { m_bufId = bufId; }
    //!
    //! \brief Get/Set mem offset
    //!
    //! \return uint64_t
    //!
    inline uint64_t MemOffset() { return m_memOffset; }
    inline void SetMemOffset(uint64_t memOffset) { m_memOffset = memOffset; }
    //!
    //! \brief Get/Set state offset
    //!
    //! \return uint64_t
    //!
    inline uint64_t StateOffset() { return m_stateOffset; }
    inline void SetStateOffset(uint64_t stateOffset) { m_stateOffset = stateOffset; }
    //!
    //! \brief Get/Set buf ptr
    //!
    //! \return uint8_t*
    //!
    inline uint8_t *BufPtr() { return m_bufPtr; }
    inline void SetBufPtr(uint8_t *bufPtr) { m_bufPtr = bufPtr; }
    //!
    //! \brief Get/Set buf size
    //!
    //! \return uint64_t
    //!
    inline uint64_t Size() { return m_size; }
    inline void SetSize(uint64_t size) { m_size = size; }
    //!
    //! \brief Get/Set occupied buf size
    //!
    //! \return uint64_t
    //!
    inline uint64_t OccupiedSize() { return m_occupiedSize; }
    inline void SetOccupiedSize(uint64_t size) { m_occupiedSize = size; }
    //!
    //! \brief Get/Set state
    //!
    //! \return BufferState
    //!
    inline BufferState State() { return m_state; }
    inline void SetState(BufferState state) { m_state = state; }

private:
    uint32_t     m_bufId;            //!< buffer id
    uint64_t     m_memOffset;        //!< memory offset from base addr
    uint64_t     m_stateOffset;      //!< state flag offset from base addr
    uint8_t     *m_bufPtr;           //!< base addr ptr
    uint64_t     m_size;             //!< buffer size
    uint64_t     m_occupiedSize;      //!< occupied buffer size
    BufferState  m_state;            //!< buffer available state
};

class FrameBufferData
{
public:
    //!
    //! \brief Construct a new Frame Buffer Data object
    //!
    //!
    FrameBufferData()
    {
        m_memBuffer = nullptr;
        m_width = 0;
        m_height = 0;
        m_streamType = InputStreamType::UNKNOWN;
        m_pts = 0;
        m_isEOS = false;
    }
    //!
    //! \brief Destroy the Frame Buffer Data object
    //!
    //!
    virtual ~FrameBufferData()
    {
        m_memBuffer = nullptr;
        m_width = 0;
        m_height = 0;
        m_streamType = InputStreamType::UNKNOWN;
        m_pts = 0;
        m_isEOS = false;
    }
    //!
    //! \brief Create a Frame Buffer Data object from a FrameBufferItem struct
    //!
    //! \param [in] item
    //! \return MRDAStatus
    //!
    inline MRDAStatus CreateFrameBufferData(FrameBufferItem *item)
    {
        // last null frame
        if (item == nullptr)
        {
            m_memBuffer = std::make_shared<MemoryBuffer>();
            m_width = 0;
            m_height = 0;
            m_streamType = InputStreamType::RAW;
            m_pts = 0;
            m_isEOS = true;
            return MRDA_STATUS_SUCCESS;
        }
        m_memBuffer = std::make_shared<MemoryBuffer>();
        if (m_memBuffer == nullptr)
        {
            MRDA_LOG(LOG_ERROR, "create mem buffer failed!");
            return MRDA_STATUS_INVALID_DATA;
        }
        m_memBuffer->CreateMemBuffer(item->bufferItem);
        m_width = item->width;
        m_height = item->height;
        m_streamType = item->streamType;
        m_pts = item->pts;
        m_isEOS = item->isEOS;
        return MRDA_STATUS_SUCCESS;
    }
    //!
    //! \brief Get/Set mem buffer
    //!
    //! \return std::shared_ptr<MemoryBuffer>
    //!
    inline std::shared_ptr<MemoryBuffer> MemBuffer() { return m_memBuffer; }
    inline void SetMemBuffer(std::shared_ptr<MemoryBuffer> memBuffer) { m_memBuffer = memBuffer; }
    //!
    //! \brief Get/Set frame width
    //!
    //! \return uint32_t
    //!
    inline uint32_t Width() { return m_width; }
    inline void SetWidth(uint32_t width) { m_width = width; }
    //!
    //! \brief Get/Set frame height
    //!
    //! \return uint32_t
    //!
    inline uint32_t Height() { return m_height; }
    inline void SetHeight(uint32_t height) { m_height = height; }
    //!
    //! \brief Get/Set Input stream type
    //!
    //! \return InputStreamType
    //!
    inline InputStreamType StreamType() { return m_streamType; }
    inline void SetStreamType(InputStreamType streamType) { m_streamType = streamType; }
    //!
    //! \brief Get/Set frame pts
    //!
    //! \return uint64_t
    //!
    inline uint64_t Pts() { return m_pts; }
    inline void SetPts(uint64_t pts) { m_pts = pts; }
    //!
    //! \brief Get/Set frame EOS flag
    //!
    //! \return bool
    //!
    inline bool IsEOS() { return m_isEOS; }
    inline void SetEOS(bool isEos) { m_isEOS = isEos; }


private:
    std::shared_ptr<MemoryBuffer> m_memBuffer;    //!< mem buffer ptr
    uint32_t                   m_width;        //!< frame width
    uint32_t                   m_height;       //!< frame height
    InputStreamType            m_streamType;   //!< input stream type
    uint64_t                   m_pts;          //!< frame pts
    bool                       m_isEOS;        //!< eos flag
};

VDI_NS_END
#endif // _FRAME_BUFFER_DATA_H_