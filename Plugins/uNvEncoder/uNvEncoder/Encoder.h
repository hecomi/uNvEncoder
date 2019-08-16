#pragma once

#include <cstdio>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <d3d11.h>
#include "Common.h"


namespace uNvEncoder
{


struct EncoderDesc
{
    int width; 
    int height;
    int frameRate;
};


class Encoder final
{
public:
    struct EncodedData
    {
        std::vector<uint8_t> buffer;
    };

    explicit Encoder(const EncoderDesc &desc);
    ~Encoder();
    bool Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame);
    void CopyEncodedDataList();
    const std::vector<EncodedData> & GetEncodedDataList() const;
    const uint32_t GetWidth() const { return desc_.width; }
    const uint32_t GetHeight() const { return desc_.height; }
    const uint32_t GetFrameRate() const { return desc_.frameRate; }

private:
    void CreateNvenc();
    void StartThread();
    void UpdateData();

    const EncoderDesc desc_;
    std::unique_ptr<class Nvenc> nvenc_;
    std::vector<EncodedData> encodedDataList_;
    std::vector<EncodedData> encodedDataListCopied_;
    std::thread thread_;
    mutable std::mutex dataListMutex_;
    std::atomic<bool> shouldStopThread_ = false;
    std::atomic<bool> isEncodeRequested_ = false;
};


}
