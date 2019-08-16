#include "Encoder.h"
#include "Nvenc.h"


namespace uNvEncoder
{


Encoder::Encoder(const EncoderDesc &desc)
    : desc_(desc)
{
    CreateNvenc();
    StartThread();
}


Encoder::~Encoder()
{
    shouldStopThread_ = true;

    if (thread_.joinable())
    {
        thread_.join();
    }
}


void Encoder::CreateNvenc()
{
    NvencDesc desc = { 0 };
    desc.d3d11Device = GetUnityDevice();
    desc.width = desc_.width;
    desc.height = desc_.height;
    desc.frameRate = desc_.frameRate;
    desc.async = true;

    nvenc_ = std::make_unique<Nvenc>(desc);
}


void Encoder::StartThread()
{
    thread_ = std::thread([&]
    {
        while (!shouldStopThread_)
        {
            try
            {
                UpdateData();
            }
            catch (const std::exception& e)
            {
                OutputDebugStringA(e.what());
            }

            std::this_thread::yield();
        }
    });
}


bool Encoder::Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame)
{
    if (isEncodeRequested_) return false;

    if (nvenc_->Encode(source, forceIdrFrame))
    {
        isEncodeRequested_ = true;
        return true;
    }

    return false;
}


void Encoder::UpdateData()
{
    if (!isEncodeRequested_) return;

    std::lock_guard<std::mutex> lock(dataListMutex_);

    EncodedData ed;
    nvenc_->GetEncodedData(ed.buffer);
    if (!ed.buffer.empty())
    {
        encodedDataList_.push_back(ed);
    }

    isEncodeRequested_ = false;
}


void Encoder::CopyEncodedDataList()
{
    std::lock_guard<std::mutex> lock(dataListMutex_);

    encodedDataListCopied_.clear();
    encodedDataListCopied_ = encodedDataList_;
    encodedDataList_.clear();
}


const std::vector<Encoder::EncodedData> & Encoder::GetEncodedDataList() const
{
    return encodedDataListCopied_;
}


}
