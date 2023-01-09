// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.
//
// References
//   - https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
//
#include "ConcurrentRingBuffer.h"
#include <atomic>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>

SignalScatter::ConcurrentRingBuffer::ConcurrentRingBuffer(int capacity)
{
    int power = (int)std::ceil(std::log2(capacity));
    int bufferSize = (int)std::pow(2, power); // Buffer size should be a power of two.

    _bufferSize = bufferSize;
    _bufferMask = bufferSize - 1;
    _buffer = new uint8_t[bufferSize];
    _sequence = new std::atomic<int>[bufferSize];

    for (int i = 0; i < bufferSize; i++)
    {
        _sequence[i].store(i, std::memory_order_relaxed);
    }

    _enqueuePosition.store(0, std::memory_order_relaxed);
	_dequeuePosition.store(0, std::memory_order_relaxed);
}

SignalScatter::ConcurrentRingBuffer::~ConcurrentRingBuffer()
{
    delete[] _buffer;
    delete[] _sequence;
}

int SignalScatter::ConcurrentRingBuffer::GetBufferSize()
{
    return _bufferSize;
}

int SignalScatter::ConcurrentRingBuffer::GetCount()
{
    return _enqueuePosition.load(std::memory_order_relaxed) - _dequeuePosition.load(std::memory_order_relaxed);
}

uint8_t SignalScatter::ConcurrentRingBuffer::GetValue(int position)
{
    int bufferPosition = _dequeuePosition.load(std::memory_order_relaxed) + position;
    return _buffer[bufferPosition & _bufferMask];
}

uint8_t SignalScatter::ConcurrentRingBuffer::GetHeadValue()
{
    int position = _dequeuePosition.load(std::memory_order_relaxed);
    return _buffer[position & _bufferMask];
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkEnqueue(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    do
    {
        int position = _enqueuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - position;

        int count = position - _dequeuePosition.load(std::memory_order_relaxed);

        if (diff == 0 && length <= (_bufferSize - count)
        && _enqueuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            for (int i = 0; i < length; i++)
            {
                // int bufferIndex = (position + i) & _bufferMask;
                _buffer[index + i] = data[i];
                _sequence[index + i].store(position + 1 + i, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0 || length > (_bufferSize - count))
        {
            std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkDequeue(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            for (int i = 0; i < length; i++)
            {
                int bufferIndex = (position + i) & _bufferMask;
                dest[i] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + i, std::memory_order_release);
                // dest[i] = _buffer[index + i];
                // _sequence[index + i].store(position + _bufferMask + 1 + i, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0)
        {
            std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

void SignalScatter::ConcurrentRingBuffer::Slice(int start, int length, ByteSpan& firstSegmentSpan, ByteSpan& secondSegmentSpan)
{
    int headPosition = _dequeuePosition.load(std::memory_order_relaxed);;
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

void SignalScatter::ConcurrentRingBuffer::Clear()
{
    int count = _enqueuePosition.load(std::memory_order_relaxed) - _dequeuePosition.load(std::memory_order_relaxed);
    Clear(count);
}

void SignalScatter::ConcurrentRingBuffer::Clear(int length)
{
    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;
        length = (length <= count) ? length : count;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            for (int i = 0; i < length; i++)
            {
                _sequence[index + i].store(position + _bufferMask + 1 + i, std::memory_order_release);
            }
            return;
        }

        SpinOnce();
    }
    while (true);
}

void SignalScatter::ConcurrentRingBuffer::SpinOnce()
{
    // auto start = std::chrono::high_resolution_clock::now();

    // A
    // std::cout << "SpinOnce" << std::endl;
    // std::cout << std::endl;

    // B
    int iteration = 32;
    // ビジーループによる待機
    while (iteration-- > 0)
    {
        std::this_thread::yield(); // yield()呼出により待機側スレッドのCPUリソース占有を避ける
    }

    // // C
    // std::this_thread::sleep_until(std::chrono::microseconds(1));

    // auto elapsed = std::chrono::high_resolution_clock::now() - start;
    // std::cout << "Waited for "
    //         << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
    //         << " microseconds\n";
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkEnqueueByte4(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 4) { return false; }

    do
    {
        int position = _enqueuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - position;

        int count = position - _dequeuePosition.load(std::memory_order_relaxed);

        if (diff == 0 && length <= (_bufferSize - count)
        && _enqueuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                _buffer[index + 0] = data[0];
                _sequence[index + 0].store(position + 1 + 0, std::memory_order_release);

                _buffer[index + 1] = data[1];
                _sequence[index + 1].store(position + 1 + 1, std::memory_order_release);

                _buffer[index + 2] = data[2];
                _sequence[index + 2].store(position + 1 + 2, std::memory_order_release);

                _buffer[index + 3] = data[3];
                _sequence[index + 3].store(position + 1 + 3, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0 || length > (_bufferSize - count))
        {
            // std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkEnqueueByte8(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 8) { return false; }

    do
    {
        int position = _enqueuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - position;

        int count = position - _dequeuePosition.load(std::memory_order_relaxed);

        if (diff == 0 && length <= (_bufferSize - count)
        && _enqueuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                _buffer[index + 0] = data[0];
                _sequence[index + 0].store(position + 1 + 0, std::memory_order_release);

                _buffer[index + 1] = data[1];
                _sequence[index + 1].store(position + 1 + 1, std::memory_order_release);

                _buffer[index + 2] = data[2];
                _sequence[index + 2].store(position + 1 + 2, std::memory_order_release);

                _buffer[index + 3] = data[3];
                _sequence[index + 3].store(position + 1 + 3, std::memory_order_release);

                _buffer[index + 4] = data[4];
                _sequence[index + 4].store(position + 1 + 4, std::memory_order_release);

                _buffer[index + 5] = data[5];
                _sequence[index + 5].store(position + 1 + 5, std::memory_order_release);

                _buffer[index + 6] = data[6];
                _sequence[index + 6].store(position + 1 + 6, std::memory_order_release);

                _buffer[index + 7] = data[7];
                _sequence[index + 7].store(position + 1 + 7, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0 || length > (_bufferSize - count))
        {
            // std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkEnqueueByte16(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 16) { return false; }

    do
    {
        int position = _enqueuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - position;

        int count = position - _dequeuePosition.load(std::memory_order_relaxed);

        if (diff == 0 && length <= (_bufferSize - count)
        && _enqueuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                _buffer[index + 0] = data[0];
                _sequence[index + 0].store(position + 1 + 0, std::memory_order_release);

                _buffer[index + 1] = data[1];
                _sequence[index + 1].store(position + 1 + 1, std::memory_order_release);

                _buffer[index + 2] = data[2];
                _sequence[index + 2].store(position + 1 + 2, std::memory_order_release);

                _buffer[index + 3] = data[3];
                _sequence[index + 3].store(position + 1 + 3, std::memory_order_release);

                _buffer[index + 4] = data[4];
                _sequence[index + 4].store(position + 1 + 4, std::memory_order_release);

                _buffer[index + 5] = data[5];
                _sequence[index + 5].store(position + 1 + 5, std::memory_order_release);

                _buffer[index + 6] = data[6];
                _sequence[index + 6].store(position + 1 + 6, std::memory_order_release);

                _buffer[index + 7] = data[7];
                _sequence[index + 7].store(position + 1 + 7, std::memory_order_release);

                _buffer[index + 8] = data[8];
                _sequence[index + 8].store(position + 1 + 8, std::memory_order_release);

                _buffer[index + 9] = data[9];
                _sequence[index + 9].store(position + 1 + 9, std::memory_order_release);

                _buffer[index + 10] = data[10];
                _sequence[index + 10].store(position + 1 + 10, std::memory_order_release);

                _buffer[index + 11] = data[11];
                _sequence[index + 11].store(position + 1 + 11, std::memory_order_release);

                _buffer[index + 12] = data[12];
                _sequence[index + 12].store(position + 1 + 12, std::memory_order_release);

                _buffer[index + 13] = data[13];
                _sequence[index + 13].store(position + 1 + 13, std::memory_order_release);

                _buffer[index + 14] = data[14];
                _sequence[index + 14].store(position + 1 + 14, std::memory_order_release);

                _buffer[index + 15] = data[15];
                _sequence[index + 15].store(position + 1 + 15, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0 || length > (_bufferSize - count))
        {
            // std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkEnqueueByte32(ByteSpan const& span)
{
    uint8_t* data = span.Pointer;
    int length = span.Length;

    if (length != 32) { return false; }

    do
    {
        int position = _enqueuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - position;

        int count = position - _dequeuePosition.load(std::memory_order_relaxed);

        if (diff == 0 && length <= (_bufferSize - count)
        && _enqueuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                _buffer[index + 0] = data[0];
                _sequence[index + 0].store(position + 1 + 0, std::memory_order_release);

                _buffer[index + 1] = data[1];
                _sequence[index + 1].store(position + 1 + 1, std::memory_order_release);

                _buffer[index + 2] = data[2];
                _sequence[index + 2].store(position + 1 + 2, std::memory_order_release);

                _buffer[index + 3] = data[3];
                _sequence[index + 3].store(position + 1 + 3, std::memory_order_release);

                _buffer[index + 4] = data[4];
                _sequence[index + 4].store(position + 1 + 4, std::memory_order_release);

                _buffer[index + 5] = data[5];
                _sequence[index + 5].store(position + 1 + 5, std::memory_order_release);

                _buffer[index + 6] = data[6];
                _sequence[index + 6].store(position + 1 + 6, std::memory_order_release);

                _buffer[index + 7] = data[7];
                _sequence[index + 7].store(position + 1 + 7, std::memory_order_release);

                _buffer[index + 8] = data[8];
                _sequence[index + 8].store(position + 1 + 8, std::memory_order_release);

                _buffer[index + 9] = data[9];
                _sequence[index + 9].store(position + 1 + 9, std::memory_order_release);

                _buffer[index + 10] = data[10];
                _sequence[index + 10].store(position + 1 + 10, std::memory_order_release);

                _buffer[index + 11] = data[11];
                _sequence[index + 11].store(position + 1 + 11, std::memory_order_release);

                _buffer[index + 12] = data[12];
                _sequence[index + 12].store(position + 1 + 12, std::memory_order_release);

                _buffer[index + 13] = data[13];
                _sequence[index + 13].store(position + 1 + 13, std::memory_order_release);

                _buffer[index + 14] = data[14];
                _sequence[index + 14].store(position + 1 + 14, std::memory_order_release);

                _buffer[index + 15] = data[15];
                _sequence[index + 15].store(position + 1 + 15, std::memory_order_release);

                _buffer[index + 16] = data[16];
                _sequence[index + 16].store(position + 1 + 16, std::memory_order_release);

                _buffer[index + 17] = data[17];
                _sequence[index + 17].store(position + 1 + 17, std::memory_order_release);

                _buffer[index + 18] = data[18];
                _sequence[index + 18].store(position + 1 + 18, std::memory_order_release);

                _buffer[index + 19] = data[19];
                _sequence[index + 19].store(position + 1 + 19, std::memory_order_release);

                _buffer[index + 20] = data[20];
                _sequence[index + 20].store(position + 1 + 20, std::memory_order_release);

                _buffer[index + 21] = data[21];
                _sequence[index + 21].store(position + 1 + 21, std::memory_order_release);

                _buffer[index + 22] = data[22];
                _sequence[index + 22].store(position + 1 + 22, std::memory_order_release);

                _buffer[index + 23] = data[23];
                _sequence[index + 23].store(position + 1 + 23, std::memory_order_release);

                _buffer[index + 24] = data[24];
                _sequence[index + 24].store(position + 1 + 24, std::memory_order_release);

                _buffer[index + 25] = data[25];
                _sequence[index + 25].store(position + 1 + 25, std::memory_order_release);

                _buffer[index + 26] = data[26];
                _sequence[index + 26].store(position + 1 + 26, std::memory_order_release);

                _buffer[index + 27] = data[27];
                _sequence[index + 27].store(position + 1 + 27, std::memory_order_release);

                _buffer[index + 28] = data[28];
                _sequence[index + 28].store(position + 1 + 28, std::memory_order_release);

                _buffer[index + 29] = data[29];
                _sequence[index + 29].store(position + 1 + 29, std::memory_order_release);

                _buffer[index + 30] = data[30];
                _sequence[index + 30].store(position + 1 + 30, std::memory_order_release);

                _buffer[index + 31] = data[31];
                _sequence[index + 31].store(position + 1 + 31, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0 || length > (_bufferSize - count))
        {
            // std::cout << "********************* BulkEnqueue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkDequeueByte4(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 4) { return false; }

    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        // int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                // int nextSequence = position + _bufferMask + 1;

                int bufferIndex = (position + 0) & _bufferMask;
                dest[0] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 0, std::memory_order_release);

                bufferIndex = (position + 1) & _bufferMask;
                dest[1] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 1, std::memory_order_release);

                bufferIndex = (position + 2) & _bufferMask;
                dest[2] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 2, std::memory_order_release);

                bufferIndex = (position + 3) & _bufferMask;
                dest[3] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 3, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0)
        {
            std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkDequeueByte8(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 8) { return false; }

    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        // int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                // int nextSequence = position + _bufferMask + 1;

                int bufferIndex = (position + 0) & _bufferMask;
                dest[0] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 0, std::memory_order_release);

                bufferIndex = (position + 1) & _bufferMask;
                dest[1] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 1, std::memory_order_release);

                bufferIndex = (position + 2) & _bufferMask;
                dest[2] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 2, std::memory_order_release);

                bufferIndex = (position + 3) & _bufferMask;
                dest[3] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 3, std::memory_order_release);

                bufferIndex = (position + 4) & _bufferMask;
                dest[4] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 4, std::memory_order_release);

                bufferIndex = (position + 5) & _bufferMask;
                dest[5] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 5, std::memory_order_release);

                bufferIndex = (position + 6) & _bufferMask;
                dest[6] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 6, std::memory_order_release);

                bufferIndex = (position + 7) & _bufferMask;
                dest[7] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 7, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0)
        {
            std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkDequeueByte16(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 16) { return false; }

    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        // int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                // int nextSequence = position + _bufferMask + 1;

                int bufferIndex = (position + 0) & _bufferMask;
                dest[0] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 0, std::memory_order_release);

                bufferIndex = (position + 1) & _bufferMask;
                dest[1] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 1, std::memory_order_release);

                bufferIndex = (position + 2) & _bufferMask;
                dest[2] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 2, std::memory_order_release);

                bufferIndex = (position + 3) & _bufferMask;
                dest[3] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 3, std::memory_order_release);

                bufferIndex = (position + 4) & _bufferMask;
                dest[4] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 4, std::memory_order_release);

                bufferIndex = (position + 5) & _bufferMask;
                dest[5] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 5, std::memory_order_release);

                bufferIndex = (position + 6) & _bufferMask;
                dest[6] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 6, std::memory_order_release);

                bufferIndex = (position + 7) & _bufferMask;
                dest[7] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 7, std::memory_order_release);

                bufferIndex = (position + 8) & _bufferMask;
                dest[8] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 8, std::memory_order_release);

                bufferIndex = (position + 9) & _bufferMask;
                dest[9] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 9, std::memory_order_release);

                bufferIndex = (position + 10) & _bufferMask;
                dest[10] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 10, std::memory_order_release);

                bufferIndex = (position + 11) & _bufferMask;
                dest[11] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 11, std::memory_order_release);

                bufferIndex = (position + 12) & _bufferMask;
                dest[12] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 12, std::memory_order_release);

                bufferIndex = (position + 13) & _bufferMask;
                dest[13] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 13, std::memory_order_release);

                bufferIndex = (position + 14) & _bufferMask;
                dest[14] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 14, std::memory_order_release);

                bufferIndex = (position + 15) & _bufferMask;
                dest[15] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 15, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0)
        {
            std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}

bool SignalScatter::ConcurrentRingBuffer::TryBulkDequeueByte32(ByteSpan& span)
{
    uint8_t* dest = span.Pointer;
    int length = span.Length;

    if (length != 32) { return false; }

    do
    {
        int position = _dequeuePosition.load(std::memory_order_relaxed);
        int index = position & _bufferMask;
        int sequence = _sequence[index].load(std::memory_order_acquire);
        int diff = sequence - (position + 1);

        // int count =  _enqueuePosition.load(std::memory_order_relaxed) - position;

        if (diff == 0 && _dequeuePosition.compare_exchange_weak(position, position + length, std::memory_order_relaxed))
        {
            // Loop Unrolling
            {
                // int nextSequence = position + _bufferMask + 1;

                int bufferIndex = (position + 0) & _bufferMask;
                dest[0] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 0, std::memory_order_release);

                bufferIndex = (position + 1) & _bufferMask;
                dest[1] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 1, std::memory_order_release);

                bufferIndex = (position + 2) & _bufferMask;
                dest[2] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 2, std::memory_order_release);

                bufferIndex = (position + 3) & _bufferMask;
                dest[3] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 3, std::memory_order_release);

                bufferIndex = (position + 4) & _bufferMask;
                dest[4] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 4, std::memory_order_release);

                bufferIndex = (position + 5) & _bufferMask;
                dest[5] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 5, std::memory_order_release);

                bufferIndex = (position + 6) & _bufferMask;
                dest[6] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 6, std::memory_order_release);

                bufferIndex = (position + 7) & _bufferMask;
                dest[7] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 7, std::memory_order_release);

                bufferIndex = (position + 8) & _bufferMask;
                dest[8] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 8, std::memory_order_release);

                bufferIndex = (position + 9) & _bufferMask;
                dest[9] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 9, std::memory_order_release);

                bufferIndex = (position + 10) & _bufferMask;
                dest[10] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 10, std::memory_order_release);

                bufferIndex = (position + 11) & _bufferMask;
                dest[11] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 11, std::memory_order_release);

                bufferIndex = (position + 12) & _bufferMask;
                dest[12] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 12, std::memory_order_release);

                bufferIndex = (position + 13) & _bufferMask;
                dest[13] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 13, std::memory_order_release);

                bufferIndex = (position + 14) & _bufferMask;
                dest[14] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 14, std::memory_order_release);

                bufferIndex = (position + 15) & _bufferMask;
                dest[15] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 15, std::memory_order_release);

                bufferIndex = (position + 16) & _bufferMask;
                dest[16] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 16, std::memory_order_release);

                bufferIndex = (position + 17) & _bufferMask;
                dest[17] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 17, std::memory_order_release);

                bufferIndex = (position + 18) & _bufferMask;
                dest[18] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 18, std::memory_order_release);

                bufferIndex = (position + 19) & _bufferMask;
                dest[19] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 19, std::memory_order_release);

                bufferIndex = (position + 20) & _bufferMask;
                dest[20] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 20, std::memory_order_release);

                bufferIndex = (position + 21) & _bufferMask;
                dest[21] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 21, std::memory_order_release);

                bufferIndex = (position + 22) & _bufferMask;
                dest[22] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 22, std::memory_order_release);

                bufferIndex = (position + 23) & _bufferMask;
                dest[23] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 23, std::memory_order_release);

                bufferIndex = (position + 24) & _bufferMask;
                dest[24] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 24, std::memory_order_release);

                bufferIndex = (position + 25) & _bufferMask;
                dest[25] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 25, std::memory_order_release);

                bufferIndex = (position + 26) & _bufferMask;
                dest[26] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 26, std::memory_order_release);

                bufferIndex = (position + 27) & _bufferMask;
                dest[27] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 27, std::memory_order_release);

                bufferIndex = (position + 28) & _bufferMask;
                dest[28] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 28, std::memory_order_release);

                bufferIndex = (position + 29) & _bufferMask;
                dest[29] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 29, std::memory_order_release);

                bufferIndex = (position + 30) & _bufferMask;
                dest[30] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 30, std::memory_order_release);

                bufferIndex = (position + 31) & _bufferMask;
                dest[31] = _buffer[bufferIndex];
                _sequence[bufferIndex].store(position + _bufferMask + 1 + 31, std::memory_order_release);
            }
            return true;
        }
        else if (diff < 0)
        {
            std::cout << "********************* BulkDequeue Overflow **********************" << std::endl;
            return false;
        }

        SpinOnce();
    }
    while (true);
}
