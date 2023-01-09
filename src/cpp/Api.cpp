#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

#include "RingBuffer.h"
#include "Span.h"

extern "C"
{

////////////////////
///  RingBuffer  ///
////////////////////

EXPORT_API SignalScatter::RingBuffer* create_ring_buffer(int capacity)
{
    return new SignalScatter::RingBuffer(capacity);
}

EXPORT_API void release_ring_buffer(SignalScatter::RingBuffer* ringBuffer)
{
    delete ringBuffer;
}

EXPORT_API int ring_buffer_get_buffer_size(SignalScatter::RingBuffer* ringBuffer)
{
    return ringBuffer->GetBufferSize();
}

EXPORT_API int ring_buffer_get_count(SignalScatter::RingBuffer* ringBuffer)
{
    return ringBuffer->GetCount();
}

EXPORT_API bool ring_buffer_try_bulk_enqueue(SignalScatter::RingBuffer* ringBuffer, uint8_t* pointer, int length)
{
    SignalScatter::ByteSpan span(pointer, length);
    return ringBuffer->TryBulkEnqueue(span);
}

EXPORT_API bool ring_buffer_try_bulk_dequeue(SignalScatter::RingBuffer* ringBuffer, uint8_t* pointer, int length)
{
    SignalScatter::ByteSpan span(pointer, length);
    return ringBuffer->TryBulkDequeue(span);
}

}