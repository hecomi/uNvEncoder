#include "Encoder.h"
#include "Nvenc.h"


namespace uNvEncoder
{


Encoder::Encoder(const EncoderDesc &desc)
    : desc_(desc)
{
    UNVENC_FUNC_SCOPED_TIMER

    CreateDevice();
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


bool Encoder::IsValid() const
{
    return device_ && nvenc_ && nvenc_->IsValid();
}


void Encoder::CreateDevice()
{
    ComPtr<IDXGIDevice1> dxgiDevice;
    if (FAILED(GetUnityDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) 
    {
        DebugError("Failed to get IDXGIDevice1.");
        return;
    }

    ComPtr<IDXGIAdapter> dxgiAdapter;
    if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter))) 
    {
        DebugError("Failed to get IDXGIAdapter.");
        return;
    }

    constexpr auto driverType = D3D_DRIVER_TYPE_UNKNOWN;
    constexpr auto flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    constexpr D3D_FEATURE_LEVEL featureLevelsRequested[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    constexpr UINT numLevelsRequested = sizeof(featureLevelsRequested) / sizeof(D3D_FEATURE_LEVEL);
    D3D_FEATURE_LEVEL featureLevelsSupported;

    D3D11CreateDevice(
        dxgiAdapter.Get(),
        driverType,
        nullptr,
        flags,
        featureLevelsRequested,
        numLevelsRequested,
        D3D11_SDK_VERSION,
        &device_,
        &featureLevelsSupported,
        nullptr);
}


void Encoder::CreateNvenc()
{
    UNVENC_FUNC_SCOPED_TIMER

    NvencDesc desc = { 0 };
    desc.d3d11Device = device_;
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


bool Encoder::Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame)
{
    UNVENC_FUNC_SCOPED_TIMER

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

    std::lock_guard<std::mutex> lock(encodeMutex_);
    isEncoding_ = true;
    encodeCond_.notify_one();
}


void Encoder::UpdateGetEncodedData()
{
    UNVENC_FUNC_SCOPED_TIMER

    std::vector<NvencEncodedData> data;
    if (nvenc_->GetEncodedData(data))
    {
        std::lock_guard<std::mutex> dataLock(encodeDataListMutex_);
        for (auto &ed : data)
        {
            encodedDataList_.push_back(std::move(ed));
        }
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
