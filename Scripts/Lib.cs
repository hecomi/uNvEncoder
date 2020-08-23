using System;
using System.Runtime.InteropServices;

#pragma warning disable CS0465

namespace uNvEncoder
{

public enum DXGI_FORMAT
{
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
};

public static class Lib
{
    public const string dllName = "uNvEncoder";

    // ---

    [DllImport(dllName, EntryPoint = "uNvEncoderCreateEncoder")]
    public static extern int CreateEncoder(int width, int height, DXGI_FORMAT format, int frameRate);
    [DllImport(dllName, EntryPoint = "uNvEncoderDestroyEncoder")]
    public static extern int DestroyEncoder(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderIsValid")]
    public static extern bool IsValid(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetWidth")]
    public static extern int GetWidth(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetHeight")]
    public static extern int GetHeight(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetFrameRate")]
    public static extern int GetFrameRate(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderEncode")]
    public static extern bool Encode(int id, IntPtr texturePtr, bool forceIdrFrame);
    [DllImport(dllName, EntryPoint = "uNvEncoderCopyEncodedData")]
    public static extern void CopyEncodedData(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataCount")]
    public static extern int GetEncodedDataCount(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataSize")]
    public static extern int GetEncodedDataSize(int id, int index);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetEncodedDataBuffer")]
    public static extern IntPtr GetEncodedDataBuffer(int id, int index);
    [DllImport(dllName, EntryPoint = "uNvEncoderGetError")]
    private static extern IntPtr GetErrorInternal(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderHasError")]
    public static extern bool HasError(int id);
    [DllImport(dllName, EntryPoint = "uNvEncoderClearError")]
    public static extern void ClearError(int id);

    public static string GetError(int id)
    {
        var ptr = GetErrorInternal(id);
        return Marshal.PtrToStringAnsi(ptr);
    }
}

}
