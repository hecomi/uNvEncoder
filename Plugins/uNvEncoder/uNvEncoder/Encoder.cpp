#include "Encoder.h"
#include "Nvenc.h"


extern std::string g_lastError;


namespace uNvEncoder
{


Encoder::Encoder(const EncoderDesc &desc)
    : desc_(desc)
{
    UNVENC_FUNC_SCOPED_TIMER

    CreateNvenc();
    StartThread();
}


Encoder::~Encoder()
{
    UNVENC_FUNC_SCOPED_TIMER

    shouldStopEncodeThread_ = true;
    encodeCond_.notify_one();

    if (encodeThread_.joinable())
    {
        encodeThread_.join();
    }
}


void Encoder::CreateNvenc()
{
    UNVENC_FUNC_SCOPED_TIMER

    NvencDesc desc = { 0 };
    desc.d3d11Device = GetUnityDevice();
    desc.width = desc_.width;
    desc.height = desc_.height;
    desc.frameRate = desc_.frameRate;

    nvenc_ = std::make_unique<Nvenc>(desc);
}


void Encoder::StartThread()
{
    UNVENC_FUNC_SCOPED_TIMER

    encodeThread_ = std::thread([&]
    {
        while (!shouldStopEncodeThread_)
        {
            WaitForEncodeRequest();
            UpdateGetEncodedData();
        }
    });
}


bool Encoder::IsEncoding() const
{
    return isEncoding_ || nvenc_->IsEncoding();
}


bool Encoder::Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame)
{
    UNVENC_FUNC_SCOPED_TIMER

    if (IsEncoding()) return false;

    if (nvenc_->Encode(source, forceIdrFrame))
    {
        RequestGetEncodedData();
        return true;
    }

    return false;
}


void Encoder::WaitForEncodeRequest()
{
    UNVENC_FUNC_SCOPED_TIMER

    std::unique_lock<std::mutex> encodeLock(encodeMutex_);
    encodeCond_.wait(encodeLock, [&] 
    { 
        return isEncoding_ || shouldStopEncodeThread_; 
    });
}


void Encoder::RequestGetEncodedData()
{
    UNVENC_FUNC_SCOPED_TIMER

    std::unique_lock<std::mutex> lock(encodeMutex_);
    isEncoding_ = true;
    encodeCond_.notify_one();
}


void Encoder::UpdateGetEncodedData()
{
    UNVENC_FUNC_SCOPED_TIMER

    NvencEncodedData data;
    if (nvenc_->GetEncodedData(data))
    {
        std::lock_guard<std::mutex> dataLock(encodeDataListMutex_);
        encodedDataList_.push_back(std::move(data));
    }

    isEncoding_ = false;
}


void Encoder::CopyEncodedDataList()
{
    UNVENC_FUNC_SCOPED_TIMER

    std::lock_guard<std::mutex> lock(encodeDataListMutex_);

    encodedDataListCopied_ = std::move(encodedDataList_);
    encodedDataList_.clear();
}


const std::vector<NvencEncodedData> & Encoder::GetEncodedDataList() const
{
    UNVENC_FUNC_SCOPED_TIMER

    return encodedDataListCopied_;
}


}
