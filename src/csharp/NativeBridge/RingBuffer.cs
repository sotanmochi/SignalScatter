// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

using System;
using System.Runtime.InteropServices;

namespace SignalScatter.NativeBridge
{
    public sealed unsafe class RingBuffer : IDisposable
    {
        public int BufferSize => NativeApi.RingBufferGetBufferSize(_handle);
        public int Count => NativeApi.RingBufferGetCount(_handle);

        public bool IsInvalid => _handle.IsInvalid;

        private readonly RingBufferHandle _handle;

        public RingBuffer(int capacity)
        {
            _handle = NativeApi.CreateRingBuffer(capacity);
        }

        public void Dispose() => _handle.Dispose();

        public bool TryBulkEnqueue(ReadOnlySpan<byte> span)
        {
            bool enqueued = false;

            fixed (byte* pointer = span)
            {
                enqueued = NativeApi.RingBufferTryBulkEnqueue(_handle, pointer, span.Length);
            }

            return enqueued;
        }

        public bool TryBulkDequeue(Span<byte> span)
        {
            bool dequeued = false;

            fixed (byte* pointer = span)
            {
                dequeued = NativeApi.RingBufferTryBulkDequeue(_handle, pointer, span.Length);
            }

            return dequeued;
        }
    }

    internal sealed class RingBufferHandle : SafeHandle
    {
        public override bool IsInvalid => IntPtr.Zero == handle;
        
        private RingBufferHandle() : base(invalidHandleValue: IntPtr.Zero, ownsHandle: true)
        {
        }

        protected override bool ReleaseHandle()
        {
            NativeApi.ReleaseRingBuffer(handle);
#if DEVELOPMENT_BUILD
            Console.WriteLine($"RingBufferHandle.ReleaseHandle");
#endif
            return true;
        }
    }
}