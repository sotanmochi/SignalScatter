// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

#pragma once

#include "Span.h"
#include <cstdint>

namespace SignalScatter
{
    class RingBuffer
    {
    public:
        RingBuffer(int capacity);
        ~RingBuffer();

        int GetBufferSize();
        int GetCount();

        void Clear();
        void Clear(int length);

        void Slice(int start, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan);
        void Slice(int start, int lenght, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan);

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
        uint8_t* _buffer;
        int _bufferMask;
        int _bufferSize;
		int _enqueuePosition;
		int _dequeuePosition;
    };
}