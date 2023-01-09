// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

#include "RingBuffer.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>

SignalScatter::RingBuffer::RingBuffer(int capacity)
{
    int power = (int)std::ceil(std::log2(capacity));
    int bufferSize = (int)std::pow(2, power); // Buffer size should be a power of two.

    _bufferSize = bufferSize;
    _bufferMask = bufferSize - 1;
    _buffer = new uint8_t[bufferSize];

    _enqueuePosition = 0;
	_dequeuePosition = 0;
}

SignalScatter::RingBuffer::~RingBuffer()
{
    delete[] _buffer;
}

int SignalScatter::RingBuffer::GetBufferSize()
{
    return _bufferSize;
}

int SignalScatter::RingBuffer::GetCount()
{
    return _enqueuePosition - _dequeuePosition;
}

void SignalScatter::RingBuffer::Clear()
{
    int count = _enqueuePosition - _dequeuePosition;
    Clear(count);
}

void SignalScatter::RingBuffer::Clear(int length)
{
    int position = _dequeuePosition;

    int count =  _enqueuePosition - position;
    length = (length <= count) ? length : count;

    _dequeuePosition = position + length;
}

void SignalScatter::RingBuffer::Slice(int start, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan)
{
    int count = _enqueuePosition - _dequeuePosition;
    Slice(start, count, firstSegmentSpan, secondSegmentSpan);
}

void SignalScatter::RingBuffer::Slice(int start, int length, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan)
{
    int headPosition = _dequeuePosition;
    int startIndex = (headPosition + start) & _bufferMask;
    int endIndex = (headPosition + start + length) & _bufferMask;

    if (startIndex <= endIndex)
    {
        firstSegmentSpan.Pointer = _buffer + startIndex;
        firstSegmentSpan.Length = length;

        secondSegmentSpan.Pointer = _buffer;
        secondSegmentSpan.Length = 0;
    }
    else
    {
        int firstSegmentSize = _bufferSize - startIndex;
        int secondSegmentSize = length - firstSegmentSize;

        firstSegmentSpan.Pointer = _buffer + startIndex;
        firstSegmentSpan.Length = firstSegmentSize;

        secondSegmentSpan.Pointer = _buffer;
        secondSegmentSpan.Length = secondSegmentSize;
    }
}

bool SignalScatter::RingBuffer::TryBulkEnqueue(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    // if (length == 4)
    // {
    //     return TryBulkEnqueueByte4(span);
    // }
    // else if (length == 8)
    // {
    //     return TryBulkEnqueueByte8(span);
    // }
    // else if (length == 16)
    // {
    //     return TryBulkEnqueueByte16(span);        
    // }
    // else if (length == 32)
    // {
    //     return TryBulkEnqueueByte32(span);
    // }
    // else
    {
        int position = _enqueuePosition;
        int count = position - _dequeuePosition;

        if (length <= (_bufferSize - count))
        {
            _enqueuePosition = position + length;

            for (int i = 0; i < length; i++)
            {
                int bufferIndex = (position + i) & _bufferMask;
                _buffer[bufferIndex] = data[i];
            }

            return true;
        }
    }

    std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkDequeue(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length == 4)
    {
        return TryBulkDequeueByte4(span);
    }
    else if (length == 8)
    {
        return TryBulkDequeueByte8(span);
    }
    else if (length == 16)
    {
        return TryBulkDequeueByte16(span);        
    }
    else if (length == 32)
    {
        return TryBulkDequeueByte32(span);
    }
    else
    {
        int position = _dequeuePosition;
        _dequeuePosition = position + length;

        for (int i = 0; i < length; i++)
        {
            int bufferIndex = (position + i) & _bufferMask;
            dest[i] = _buffer[bufferIndex];
        }

        return true;
    }

    std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkEnqueueByte4(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 4) { return false; }

    int position = _enqueuePosition;
    int count = position - _dequeuePosition;

    if (length <= (_bufferSize - count))
    {
        _enqueuePosition = position + length;

        // Loop Unrolling
        {
            int bufferIndex = (position + 0) & _bufferMask;
            _buffer[bufferIndex] = data[0];

            bufferIndex = (position + 1) & _bufferMask;
            _buffer[bufferIndex] = data[1];

            bufferIndex = (position + 2) & _bufferMask;
            _buffer[bufferIndex] = data[2];

            bufferIndex = (position + 3) & _bufferMask;
            _buffer[bufferIndex] = data[3];
        }

        return true;
    }

    std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkEnqueueByte8(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 8) { return false; }

    int position = _enqueuePosition;
    int count = position - _dequeuePosition;

    if (length <= (_bufferSize - count))
    {
        _enqueuePosition = position + length;

        // Loop Unrolling
        {
            int bufferIndex = (position + 0) & _bufferMask;
            _buffer[bufferIndex] = data[0];

            bufferIndex = (position + 1) & _bufferMask;
            _buffer[bufferIndex] = data[1];

            bufferIndex = (position + 2) & _bufferMask;
            _buffer[bufferIndex] = data[2];

            bufferIndex = (position + 3) & _bufferMask;
            _buffer[bufferIndex] = data[3];

            bufferIndex = (position + 4) & _bufferMask;
            _buffer[bufferIndex] = data[4];

            bufferIndex = (position + 5) & _bufferMask;
            _buffer[bufferIndex] = data[5];

            bufferIndex = (position + 6) & _bufferMask;
            _buffer[bufferIndex] = data[6];

            bufferIndex = (position + 7) & _bufferMask;
            _buffer[bufferIndex] = data[7];
        }

        return true;
    }

    std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkEnqueueByte16(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 16) { return false; }

    int position = _enqueuePosition;
    int count = position - _dequeuePosition;

    if (length <= (_bufferSize - count))
    {
        _enqueuePosition = position + length;

        // Loop Unrolling
        {
            int bufferIndex = (position + 0) & _bufferMask;
            _buffer[bufferIndex] = data[0];

            bufferIndex = (position + 1) & _bufferMask;
            _buffer[bufferIndex] = data[1];

            bufferIndex = (position + 2) & _bufferMask;
            _buffer[bufferIndex] = data[2];

            bufferIndex = (position + 3) & _bufferMask;
            _buffer[bufferIndex] = data[3];

            bufferIndex = (position + 4) & _bufferMask;
            _buffer[bufferIndex] = data[4];

            bufferIndex = (position + 5) & _bufferMask;
            _buffer[bufferIndex] = data[5];

            bufferIndex = (position + 6) & _bufferMask;
            _buffer[bufferIndex] = data[6];

            bufferIndex = (position + 7) & _bufferMask;
            _buffer[bufferIndex] = data[7];

            bufferIndex = (position + 8) & _bufferMask;
            _buffer[bufferIndex] = data[8];

            bufferIndex = (position + 9) & _bufferMask;
            _buffer[bufferIndex] = data[9];

            bufferIndex = (position + 10) & _bufferMask;
            _buffer[bufferIndex] = data[10];

            bufferIndex = (position + 11) & _bufferMask;
            _buffer[bufferIndex] = data[11];

            bufferIndex = (position + 12) & _bufferMask;
            _buffer[bufferIndex] = data[12];

            bufferIndex = (position + 13) & _bufferMask;
            _buffer[bufferIndex] = data[13];

            bufferIndex = (position + 14) & _bufferMask;
            _buffer[bufferIndex] = data[14];

            bufferIndex = (position + 15) & _bufferMask;
            _buffer[bufferIndex] = data[15];
        }

        return true;
    }

    std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkEnqueueByte32(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 32) { return false; }

    int position = _enqueuePosition;
    int count = position - _dequeuePosition;

    if (length <= (_bufferSize - count))
    {
        _enqueuePosition = position + length;

        // Loop Unrolling
        {
            int bufferIndex = (position + 0) & _bufferMask;
            _buffer[bufferIndex] = data[0];

            bufferIndex = (position + 1) & _bufferMask;
            _buffer[bufferIndex] = data[1];

            bufferIndex = (position + 2) & _bufferMask;
            _buffer[bufferIndex] = data[2];

            bufferIndex = (position + 3) & _bufferMask;
            _buffer[bufferIndex] = data[3];

            bufferIndex = (position + 4) & _bufferMask;
            _buffer[bufferIndex] = data[4];

            bufferIndex = (position + 5) & _bufferMask;
            _buffer[bufferIndex] = data[5];

            bufferIndex = (position + 6) & _bufferMask;
            _buffer[bufferIndex] = data[6];

            bufferIndex = (position + 7) & _bufferMask;
            _buffer[bufferIndex] = data[7];

            bufferIndex = (position + 8) & _bufferMask;
            _buffer[bufferIndex] = data[8];

            bufferIndex = (position + 9) & _bufferMask;
            _buffer[bufferIndex] = data[9];

            bufferIndex = (position + 10) & _bufferMask;
            _buffer[bufferIndex] = data[10];

            bufferIndex = (position + 11) & _bufferMask;
            _buffer[bufferIndex] = data[11];

            bufferIndex = (position + 12) & _bufferMask;
            _buffer[bufferIndex] = data[12];

            bufferIndex = (position + 13) & _bufferMask;
            _buffer[bufferIndex] = data[13];

            bufferIndex = (position + 14) & _bufferMask;
            _buffer[bufferIndex] = data[14];

            bufferIndex = (position + 15) & _bufferMask;
            _buffer[bufferIndex] = data[15];

            bufferIndex = (position + 16) & _bufferMask;
            _buffer[bufferIndex] = data[16];

            bufferIndex = (position + 17) & _bufferMask;
            _buffer[bufferIndex] = data[17];

            bufferIndex = (position + 18) & _bufferMask;
            _buffer[bufferIndex] = data[18];

            bufferIndex = (position + 19) & _bufferMask;
            _buffer[bufferIndex] = data[19];

            bufferIndex = (position + 20) & _bufferMask;
            _buffer[bufferIndex] = data[20];

            bufferIndex = (position + 21) & _bufferMask;
            _buffer[bufferIndex] = data[21];

            bufferIndex = (position + 22) & _bufferMask;
            _buffer[bufferIndex] = data[22];

            bufferIndex = (position + 23) & _bufferMask;
            _buffer[bufferIndex] = data[23];

            bufferIndex = (position + 24) & _bufferMask;
            _buffer[bufferIndex] = data[24];

            bufferIndex = (position + 25) & _bufferMask;
            _buffer[bufferIndex] = data[25];

            bufferIndex = (position + 26) & _bufferMask;
            _buffer[bufferIndex] = data[26];

            bufferIndex = (position + 27) & _bufferMask;
            _buffer[bufferIndex] = data[27];

            bufferIndex = (position + 28) & _bufferMask;
            _buffer[bufferIndex] = data[28];

            bufferIndex = (position + 29) & _bufferMask;
            _buffer[bufferIndex] = data[29];

            bufferIndex = (position + 30) & _bufferMask;
            _buffer[bufferIndex] = data[30];

            bufferIndex = (position + 31) & _bufferMask;
            _buffer[bufferIndex] = data[31];
        }

        return true;
    }

    std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
    return false;
}

bool SignalScatter::RingBuffer::TryBulkDequeueByte4(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 4) { return false; }

    int position = _dequeuePosition;
    _dequeuePosition = position + length;

    // Loop Unrolling
    {
        int bufferIndex = (position + 0) & _bufferMask;
        dest[0] = _buffer[bufferIndex];

        bufferIndex = (position + 1) & _bufferMask;
        dest[1] = _buffer[bufferIndex];

        bufferIndex = (position + 2) & _bufferMask;
        dest[2] = _buffer[bufferIndex];

        bufferIndex = (position + 3) & _bufferMask;
        dest[3] = _buffer[bufferIndex];
    }

    return true;
}

bool SignalScatter::RingBuffer::TryBulkDequeueByte8(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 8) { return false; }

    int position = _dequeuePosition;
    _dequeuePosition = position + length;

    // Loop Unrolling
    {
        int bufferIndex = (position + 0) & _bufferMask;
        dest[0] = _buffer[bufferIndex];

        bufferIndex = (position + 1) & _bufferMask;
        dest[1] = _buffer[bufferIndex];

        bufferIndex = (position + 2) & _bufferMask;
        dest[2] = _buffer[bufferIndex];

        bufferIndex = (position + 3) & _bufferMask;
        dest[3] = _buffer[bufferIndex];

        bufferIndex = (position + 4) & _bufferMask;
        dest[4] = _buffer[bufferIndex];

        bufferIndex = (position + 5) & _bufferMask;
        dest[5] = _buffer[bufferIndex];

        bufferIndex = (position + 6) & _bufferMask;
        dest[6] = _buffer[bufferIndex];

        bufferIndex = (position + 7) & _bufferMask;
        dest[7] = _buffer[bufferIndex];
    }

    return true;
}

bool SignalScatter::RingBuffer::TryBulkDequeueByte16(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 16) { return false; }

    int position = _dequeuePosition;
    _dequeuePosition = position + length;

    // Loop Unrolling
    {
        int bufferIndex = (position + 0) & _bufferMask;
        dest[0] = _buffer[bufferIndex];

        bufferIndex = (position + 1) & _bufferMask;
        dest[1] = _buffer[bufferIndex];

        bufferIndex = (position + 2) & _bufferMask;
        dest[2] = _buffer[bufferIndex];

        bufferIndex = (position + 3) & _bufferMask;
        dest[3] = _buffer[bufferIndex];

        bufferIndex = (position + 4) & _bufferMask;
        dest[4] = _buffer[bufferIndex];

        bufferIndex = (position + 5) & _bufferMask;
        dest[5] = _buffer[bufferIndex];

        bufferIndex = (position + 6) & _bufferMask;
        dest[6] = _buffer[bufferIndex];

        bufferIndex = (position + 7) & _bufferMask;
        dest[7] = _buffer[bufferIndex];

        bufferIndex = (position + 8) & _bufferMask;
        dest[8] = _buffer[bufferIndex];

        bufferIndex = (position + 9) & _bufferMask;
        dest[9] = _buffer[bufferIndex];

        bufferIndex = (position + 10) & _bufferMask;
        dest[10] = _buffer[bufferIndex];

        bufferIndex = (position + 11) & _bufferMask;
        dest[11] = _buffer[bufferIndex];

        bufferIndex = (position + 12) & _bufferMask;
        dest[12] = _buffer[bufferIndex];

        bufferIndex = (position + 13) & _bufferMask;
        dest[13] = _buffer[bufferIndex];

        bufferIndex = (position + 14) & _bufferMask;
        dest[14] = _buffer[bufferIndex];

        bufferIndex = (position + 15) & _bufferMask;
        dest[15] = _buffer[bufferIndex];
    }

    return true;
}

bool SignalScatter::RingBuffer::TryBulkDequeueByte32(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 32) { return false; }

    int position = _dequeuePosition;
    _dequeuePosition = position + length;

    // Loop Unrolling
    {
        int bufferIndex = (position + 0) & _bufferMask;
        dest[0] = _buffer[bufferIndex];

        bufferIndex = (position + 1) & _bufferMask;
        dest[1] = _buffer[bufferIndex];

        bufferIndex = (position + 2) & _bufferMask;
        dest[2] = _buffer[bufferIndex];

        bufferIndex = (position + 3) & _bufferMask;
        dest[3] = _buffer[bufferIndex];

        bufferIndex = (position + 4) & _bufferMask;
        dest[4] = _buffer[bufferIndex];

        bufferIndex = (position + 5) & _bufferMask;
        dest[5] = _buffer[bufferIndex];

        bufferIndex = (position + 6) & _bufferMask;
        dest[6] = _buffer[bufferIndex];

        bufferIndex = (position + 7) & _bufferMask;
        dest[7] = _buffer[bufferIndex];

        bufferIndex = (position + 8) & _bufferMask;
        dest[8] = _buffer[bufferIndex];

        bufferIndex = (position + 9) & _bufferMask;
        dest[9] = _buffer[bufferIndex];

        bufferIndex = (position + 10) & _bufferMask;
        dest[10] = _buffer[bufferIndex];

        bufferIndex = (position + 11) & _bufferMask;
        dest[11] = _buffer[bufferIndex];

        bufferIndex = (position + 12) & _bufferMask;
        dest[12] = _buffer[bufferIndex];

        bufferIndex = (position + 13) & _bufferMask;
        dest[13] = _buffer[bufferIndex];

        bufferIndex = (position + 14) & _bufferMask;
        dest[14] = _buffer[bufferIndex];

        bufferIndex = (position + 15) & _bufferMask;
        dest[15] = _buffer[bufferIndex];

        bufferIndex = (position + 16) & _bufferMask;
        dest[16] = _buffer[bufferIndex];

        bufferIndex = (position + 17) & _bufferMask;
        dest[17] = _buffer[bufferIndex];

        bufferIndex = (position + 18) & _bufferMask;
        dest[18] = _buffer[bufferIndex];

        bufferIndex = (position + 19) & _bufferMask;
        dest[19] = _buffer[bufferIndex];

        bufferIndex = (position + 20) & _bufferMask;
        dest[20] = _buffer[bufferIndex];

        bufferIndex = (position + 21) & _bufferMask;
        dest[21] = _buffer[bufferIndex];

        bufferIndex = (position + 22) & _bufferMask;
        dest[22] = _buffer[bufferIndex];

        bufferIndex = (position + 23) & _bufferMask;
        dest[23] = _buffer[bufferIndex];

        bufferIndex = (position + 24) & _bufferMask;
        dest[24] = _buffer[bufferIndex];

        bufferIndex = (position + 25) & _bufferMask;
        dest[25] = _buffer[bufferIndex];

        bufferIndex = (position + 26) & _bufferMask;
        dest[26] = _buffer[bufferIndex];

        bufferIndex = (position + 27) & _bufferMask;
        dest[27] = _buffer[bufferIndex];

        bufferIndex = (position + 28) & _bufferMask;
        dest[28] = _buffer[bufferIndex];

        bufferIndex = (position + 29) & _bufferMask;
        dest[29] = _buffer[bufferIndex];

        bufferIndex = (position + 30) & _bufferMask;
        dest[30] = _buffer[bufferIndex];

        bufferIndex = (position + 31) & _bufferMask;
        dest[31] = _buffer[bufferIndex];
    }

    return true;
}