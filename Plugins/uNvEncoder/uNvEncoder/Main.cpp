#include <memory>
#include <string>
#include <d3d11.h>
#include <IUnityInterface.h>
#include "Encoder.h"
#include "Nvenc.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


using namespace uNvEncoder;


namespace uNvEncoder
{
    IUnityInterfaces *g_unity = nullptr;
    std::string g_error;
    std::unique_ptr<Encoder> g_encoder;
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


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API uNvEncoderInitialize(int width, int height, int frameRate)
{
    if (g_encoder || !g_unity) return;

    EncoderDesc desc;
    desc.width = width;
    desc.height = height;
    desc.frameRate = frameRate;

    g_encoder = std::make_unique<Encoder>(desc);
}


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API uNvEncoderFinalize()
{
    if (!g_encoder) return;

    g_encoder.reset();
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderIsValid()
{
    return g_encoder && g_encoder->IsValid();
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetWidth()
{
    if (!uNvEncoderIsValid()) return 0;

    return static_cast<int>(g_encoder->GetWidth());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetHeight()
{
    if (!uNvEncoderIsValid()) return 0;

    return static_cast<int>(g_encoder->GetHeight());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetFrameRate()
{
    if (!uNvEncoderIsValid()) return 0;

    return static_cast<int>(g_encoder->GetFrameRate());
}


UNITY_INTERFACE_EXPORT bool UNITY_INTERFACE_API uNvEncoderEncode(ID3D11Texture2D *texture, bool forceIdrFrame)
{
    if (!uNvEncoderIsValid()) return false;

    return g_encoder->Encode(texture, forceIdrFrame);
}


UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API uNvEncoderCopyEncodedData()
{
    if (!uNvEncoderIsValid()) return;

    return g_encoder->CopyEncodedDataList();
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedDataCount()
{
    if (!uNvEncoderIsValid()) return 0;

    return static_cast<int>(g_encoder->GetEncodedDataList().size());
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedDataSize(int i)
{
    if (!uNvEncoderIsValid()) return 0;

    const auto &list = g_encoder->GetEncodedDataList();
    if (i < 0 || i >= static_cast<int>(list.size())) return 0;

    return static_cast<int>(list.at(i).size);
}


UNITY_INTERFACE_EXPORT const void * UNITY_INTERFACE_API uNvEncoderGetEncodedDataBuffer(int i)
{
    if (!uNvEncoderIsValid()) return nullptr;

    const auto &list = g_encoder->GetEncodedDataList();
    if (i < 0 || i >= static_cast<int>(list.size())) return nullptr;

    return list.at(i).buffer.get();
}


UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API uNvEncoderGetEncodedDataIndex(int i)
{
    if (!uNvEncoderIsValid()) return -1;

    const auto &list = g_encoder->GetEncodedDataList();
    if (i < 0 || i >= static_cast<int>(list.size())) return -1;

    return static_cast<int>(list.at(i).index);
}


UNITY_INTERFACE_EXPORT const char * UNITY_INTERFACE_API uNvEncoderGetLastError()
{
    return g_error.c_str();
}


}