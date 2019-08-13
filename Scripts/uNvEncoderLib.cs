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
    public static extern bool Initialize(int width, int height, int frameRate);
    [DllImport(dllName, EntryPoint = "uNvEncoderFinalize")]
    public static extern bool Finalize();
    [DllImport(dllName, EntryPoint = "uNvEncoderIsInitialized")]
    public static extern bool IsInitialized();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetWidth")]
    public static extern int GetWidth();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetHeight")]
    public static extern int GetHeight();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetFrameRate")]
    public static extern int GetFrameRate();
    [DllImport(dllName, EntryPoint = "uNvEncoderEncode")]
    public static extern bool Encode(IntPtr texturePtr, bool forceIdrFrame);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedSize")]
    public static extern int GetEncodedSize();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedData")]
    public static extern IntPtr GetEncodedData();
    [DllImport(dllName, EntryPoint = "uNvEncoderGetLastError")]
    private static extern IntPtr GetLastErrorInternal();

    public static string GetLastError()
    {
        var ptr = GetLastErrorInternal();
        return Marshal.PtrToStringAnsi(ptr);
    }
}

}
