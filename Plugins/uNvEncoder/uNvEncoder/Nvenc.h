#pragma once

#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include "nvEncodeAPI.h"


namespace uNvEncoder
{


template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;


struct NvencDesc
{
    ID3D11Device* d3d11Device; 
    uint32_t width; 
    uint32_t height;
    uint32_t frameRate;
};


class Nvenc final
{
public:
    explicit Nvenc(const NvencDesc &desc);
    ~Nvenc();
    bool IsValid() const { return encoder_ != nullptr; }
    bool Encode(ID3D11Texture2D *source, bool forceIdrFrame);
    bool IsEncoding() const { return isEncoding_; }
    size_t GetEncodedSize() const { return encodedData_.size(); }
    const uint8_t * GetEncodedData() const { return encodedData_.data(); }
    const uint32_t GetWidth() const { return desc_.width; }
    const uint32_t GetHeight() const { return desc_.height; }
    const uint32_t GetFrameRate() const { return desc_.frameRate; }

private:
    NVENCSTATUS LoadModule();
    void UnloadModule();

    void OpenEncodeSession();
    void InitializeEncoder();
    void DestroyEncoder();
    void CreateBitstreamBuffer();
    void DestroyBitstreamBuffer();
    void RegisterResource();
    void UnregisterResource();
    void MapInputResource();
    void UnmapInputResource();

    bool EncodeFrame(bool forceIdrFrame);
    void GetEncodedPacket();
    void EndEncode();
    void SendEOS();

    const NvencDesc desc_;
    HMODULE module_;
    NV_ENCODE_API_FUNCTION_LIST nvenc_;
    void *encoder_;
    NV_ENC_INPUT_PTR inputResource_;
    NV_ENC_OUTPUT_PTR bitstreamBuffer_;
    NV_ENC_REGISTERED_PTR registeredResource_;
    ComPtr<ID3D11Texture2D> inputTexture_;

    std::vector<uint8_t> encodedData_;
    bool isEncoding_ = false;
    unsigned long frame_ = 0U;
};


}
