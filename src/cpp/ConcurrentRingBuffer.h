// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

#pragma once

#include "Span.h"
#include <cstdint>
#include <atomic>

namespace SignalScatter
{
    class ConcurrentRingBuffer
    {
    public:
        ConcurrentRingBuffer(int capacity);
        ~ConcurrentRingBuffer();

        int GetBufferSize();
        int GetCount();
        uint8_t GetValue(int index);
        uint8_t GetHeadValue();

        void Clear();
        void Clear(int length);

        void Slice(int start, int length, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan);

        bool TryBulkEnqueue(ByteSpan const& span);
        bool TryBulkDequeue(ByteSpan& span);

        bool TryBulkEnqueueByte4(ByteSpan const& span);
        bool TryBulkEnqueueByte8(ByteSpan const& span);
        bool TryBulkEnqueueByte16(ByteSpan const& span);
        bool TryBulkEnqueueByte32(ByteSpan const& span);

        bool TryBulkDequeueByte4(ByteSpan& span);
        bool TryBulkDequeueByte8(ByteSpan& span);
        bool TryBulkDequeueByte16(ByteSpan& span);
        bool TryBulkDequeueByte32(ByteSpan& span);

    private:
        std::atomic<int>* _sequence;
        uint8_t* _buffer;
        int _bufferMask;
        int _bufferSize;
		std::atomic<int> _enqueuePosition;
		std::atomic<int> _dequeuePosition;
        void SpinOnce();
    };
}