#include <memory>
#include <string>
#include <d3d11.h>
#include <IUnityInterface.h>
#include <IUnityGraphics.h>
#include <IUnityGraphicsD3D11.h>
#include "Nvenc.h"

namespace
{

std::unique_ptr<uNvEncoder::Nvenc> g_nvenc;
IUnityInterfaces* g_unity = nullptr;
std::string g_lastError;

}


extern "C"
{


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    g_unity = unityInterfaces;
}


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload()
{
    g_unity = nullptr;
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderInitialize(int width, int height, int frameRate)
{
    if (g_nvenc || !g_unity) return false;

    uNvEncoder::NvencDesc desc;
    desc.width = width;
    desc.height = height;
    desc.frameRate = frameRate;
    desc.d3d11Device = g_unity->Get<IUnityGraphicsD3D11>()->GetDevice();

    try
    {
        g_nvenc = std::make_unique<uNvEncoder::Nvenc>(desc);
    }
    catch (const std::exception &e)
    {
        g_lastError = e.what();
        return false;
    }

    return true;
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderFinalize()
{
    if (!g_nvenc) return false;

    try
    {
        g_nvenc.reset();
    }
    catch (const std::exception &e)
    {
        g_lastError = e.what();
        return false;
    }

    return true;
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderIsInitialized()
{
    return g_nvenc != nullptr;
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetWidth()
{
    if (!g_nvenc) return 0;

    return static_cast<int>(g_nvenc->GetWidth());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetHeight()
{
    if (!g_nvenc) return 0;

    return static_cast<int>(g_nvenc->GetHeight());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetFrameRate()
{
    if (!g_nvenc) return 0;

    return static_cast<int>(g_nvenc->GetFrameRate());
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderEncode(ID3D11Texture2D *texture, bool forceIdrFrame)
{
    if (!g_nvenc) return false;

    bool result;
    try
    {
        result = g_nvenc->Encode(texture, forceIdrFrame);
    }
    catch (const std::exception &e)
    {
        g_lastError = e.what();
        return false;
    }

    return result;
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedSize()
{
    if (!g_nvenc) return 0;

    return static_cast<int>(g_nvenc->GetEncodedSize());
}


UNITY_INTERFACE_EXPORT const void * UNITY_INTERFACE_API uNvEncoderGetEncodedData()
{
    if (!g_nvenc) return nullptr;

    return g_nvenc->GetEncodedData();
}


UNITY_INTERFACE_EXPORT const char * UNITY_INTERFACE_API uNvEncoderGetLastError()
{
    return g_lastError.c_str();
}


}