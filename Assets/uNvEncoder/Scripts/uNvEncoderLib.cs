using System;
using System.Runtime.InteropServices;

#pragma warning disable CS0465

namespace uNvEncoder
{

public static class Lib
{
    public const string dllName = "uNvEncoder";

    // ---

    [DllImport(dllName, EntryPoint = "uNvEncoderInitialize")]
    public static extern void Initialize(int width, int height, int frameRate);
    [DllImport(dllName, EntryPoint = "uNvEncoderFinalize")]
    public static extern void Finalize();
    [DllImport(dllName, EntryPoint = "uNvEncoderIsValid")]
    public static extern bool IsValid();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetWidth")]
    public static extern int GetWidth();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetHeight")]
    public static extern int GetHeight();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetFrameRate")]
    public static extern int GetFrameRate();
    [DllImport(dllName, EntryPoint = "uNvEncoderEncode")]
    public static extern bool Encode(IntPtr texturePtr, bool forceIdrFrame);
    [DllImport(dllName, EntryPoint = "uNvEncoderCopyEncodedData")]
    public static extern void CopyEncodedData();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataCount")]
    public static extern int GetEncodedDataCount();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataSize")]
    public static extern int GetEncodedDataSize(int i);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataBuffer")]
    public static extern IntPtr GetEncodedDataBuffer(int i);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetLastError")]
    private static extern IntPtr GetLastErrorInternal();

    public static string GetLastError()
    {
        var ptr = GetLastErrorInternal();
        return Marshal.PtrToStringAnsi(ptr);
    }
}

}
