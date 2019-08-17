#pragma once

#include <cstdio>
#include <vector>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <d3d11.h>
#include "Common.h"


namespace uNvEncoder
{


struct NvencEncodedData;


struct EncoderDesc
{
    int width; 
    int height;
    int frameRate;
};


class Encoder final
{
public:
    explicit Encoder(const EncoderDesc &desc);
    ~Encoder();
    bool Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame);
    void CopyEncodedDataList();
    const std::vector<NvencEncodedData> & GetEncodedDataList() const;
    const uint32_t GetWidth() const { return desc_.width; }
    const uint32_t GetHeight() const { return desc_.height; }
    const uint32_t GetFrameRate() const { return desc_.frameRate; }
    bool IsEncoding() const;

private:
    void CreateNvenc();
    void StartThread();
    void WaitForEncodeRequest();
    void RequestGetEncodedData();
    void UpdateGetEncodedData();

    const EncoderDesc desc_;
    std::unique_ptr<class Nvenc> nvenc_;
    std::vector<NvencEncodedData> encodedDataList_;
    std::vector<NvencEncodedData> encodedDataListCopied_;
    std::thread encodeThread_;
    std::condition_variable encodeCond_;
    std::mutex encodeMutex_;
    std::mutex encodeDataListMutex_;
    bool shouldStopEncodeThread_ = false;
    bool isEncoding_ = false;
};


}
