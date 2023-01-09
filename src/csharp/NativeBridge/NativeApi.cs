// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

using System;
using System.Runtime.InteropServices;

namespace SignalScatter.NativeBridge
{
    internal static unsafe class NativeApi
    {
        public const string DLL_NAME = "SignalScatter";

        ////////////////////
        ///  RingBuffer  ///
        ////////////////////
        [DllImport(DLL_NAME, EntryPoint = "create_ring_buffer", CallingConvention = CallingConvention.Cdecl)]
        public static extern RingBufferHandle CreateRingBuffer(int capacity);

        [DllImport(DLL_NAME, EntryPoint = "release_ring_buffer", CallingConvention = CallingConvention.Cdecl)]
        public static extern void ReleaseRingBuffer(IntPtr handle);

        [DllImport(DLL_NAME, EntryPoint = "ring_buffer_get_buffer_size", CallingConvention = CallingConvention.Cdecl)]
        public static extern int RingBufferGetBufferSize(RingBufferHandle handle);

        [DllImport(DLL_NAME, EntryPoint = "ring_buffer_get_count", CallingConvention = CallingConvention.Cdecl)]
        public static extern int RingBufferGetCount(RingBufferHandle handle);

        [DllImport(DLL_NAME, EntryPoint = "ring_buffer_try_bulk_enqueue", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool RingBufferTryBulkEnqueue(RingBufferHandle handle, byte* pointer, int length);

        [DllImport(DLL_NAME, EntryPoint = "ring_buffer_try_bulk_dequeue", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool RingBufferTryBulkDequeue(RingBufferHandle handle, byte* pointer, int length);
    }
}