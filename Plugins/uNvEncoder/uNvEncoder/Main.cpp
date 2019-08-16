#include <memory>
#include <string>
#include <d3d11.h>
#include <IUnityInterface.h>
#include "Encoder.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


using namespace uNvEncoder;


namespace uNvEncoder
{
    IUnityInterfaces *g_unity = nullptr;
}


namespace
{
    std::unique_ptr<Encoder> g_encoder;
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
    if (g_encoder || !g_unity) return false;

    EncoderDesc desc;
    desc.width = width;
    desc.height = height;
    desc.frameRate = frameRate;

    try
    {
        g_encoder = std::make_unique<Encoder>(desc);
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
    if (!g_encoder) return false;

    try
    {
        g_encoder.reset();
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
    return g_encoder != nullptr;
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetWidth()
{
    if (!g_encoder) return 0;

    return static_cast<int>(g_encoder->GetWidth());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetHeight()
{
    if (!g_encoder) return 0;

    return static_cast<int>(g_encoder->GetHeight());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetFrameRate()
{
    if (!g_encoder) return 0;

    return static_cast<int>(g_encoder->GetFrameRate());
}


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API uNvEncoderEncode(ID3D11Texture2D *texture, bool forceIdrFrame)
{
    if (!g_encoder) return;

    try
    {
        g_encoder->Encode(texture, forceIdrFrame);
    }
    catch (const std::exception &e)
    {
        g_lastError = e.what();
    }
}


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API uNvEncoderCopyEncodedData()
{
    if (!g_encoder) return;

    return g_encoder->CopyEncodedDataList();
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedDataCount()
{
    if (!g_encoder) return 0;

    return static_cast<int>(g_encoder->GetEncodedDataList().size());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedDataSize(int i)
{
    if (!g_encoder) return 0;

    const auto &list = g_encoder->GetEncodedDataList();
    if (i < 0 || i >= static_cast<int>(list.size())) return 0;

    return static_cast<int>(list.at(i).buffer.size());
}


UNITY_INTERFACE_EXPORT const void * UNITY_INTERFACE_API uNvEncoderGetEncodedDataBuffer(int i)
{
    if (!g_encoder) return nullptr;

    const auto &list = g_encoder->GetEncodedDataList();
    if (i < 0 || i >= static_cast<int>(list.size())) return nullptr;

    return list.at(i).buffer.data();
}


UNITY_INTERFACE_EXPORT const char * UNITY_INTERFACE_API uNvEncoderGetLastError()
{
    return g_lastError.c_str();
}


}