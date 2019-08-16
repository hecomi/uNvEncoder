#pragma once

#include <vector>
#include <atomic>
#include <d3d11.h>
#include <wrl/client.h>
#include "nvEncodeAPI.h"
#include "Common.h"


namespace uNvEncoder
{


struct NvencDesc
{
    ComPtr<ID3D11Device> d3d11Device; 
    uint32_t width; 
    uint32_t height;
    uint32_t frameRate;
    bool async;
};


class Nvenc final
{
public:
    explicit Nvenc(const NvencDesc &desc);
    ~Nvenc();
    bool IsValid() const { return encoder_ != nullptr; }
    bool Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame);
    void WaitForCompletion(DWORD duration);
    bool IsEncoding() const { return isEncoding_; }
    void GetEncodedData(std::vector<uint8_t> &data);
    const uint32_t GetWidth() const { return desc_.width; }
    const uint32_t GetHeight() const { return desc_.height; }
    const uint32_t GetFrameRate() const { return desc_.frameRate; }
    const bool IsAsync() const { return desc_.async; }

private:
    NVENCSTATUS LoadModule();
    void UnloadModule();

    void OpenEncodeSession();
    void InitializeEncoder();
    void DestroyEncoder();
    void CreateCompletionEvent();
    void DestroyCompletionEvent();
    void CreateBitstreamBuffer();
    void DestroyBitstreamBuffer();
    void RegisterResource();
    void UnregisterResource();
    void MapInputResource();
    void UnmapInputResource();

    bool EncodeFrame(bool forceIdrFrame);
    void EndEncode();
    void SendEOS();

    const NvencDesc desc_;
    HMODULE module_ = nullptr;
    NV_ENCODE_API_FUNCTION_LIST nvenc_ = { 0 };
    void *encoder_ = nullptr;
    NV_ENC_INPUT_PTR inputResource_ = nullptr;
    NV_ENC_OUTPUT_PTR bitstreamBuffer_ = nullptr;
    NV_ENC_REGISTERED_PTR registeredResource_ = nullptr;
    ComPtr<ID3D11Texture2D> inputTexture_ = nullptr;
    void *completionEvent_ = nullptr;

    std::atomic<bool> isEncoding_ = false;
    unsigned long frame_ = 0U;
};


}
