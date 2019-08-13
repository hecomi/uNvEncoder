#include <string>
#include <map>
#include "Nvenc.h"


namespace uNvEncoder
{


namespace
{


#define STATUS_STR_PAIR(Code) { Code, #Code },
std::map<NVENCSTATUS, std::string> g_nvEncStatusErrorNameTable = {
    STATUS_STR_PAIR(NV_ENC_SUCCESS)
    STATUS_STR_PAIR(NV_ENC_ERR_NO_ENCODE_DEVICE)
    STATUS_STR_PAIR(NV_ENC_ERR_UNSUPPORTED_DEVICE)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_ENCODERDEVICE)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_DEVICE)
    STATUS_STR_PAIR(NV_ENC_ERR_DEVICE_NOT_EXIST)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_PTR)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_EVENT)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_PARAM)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_CALL)
    STATUS_STR_PAIR(NV_ENC_ERR_OUT_OF_MEMORY)
    STATUS_STR_PAIR(NV_ENC_ERR_ENCODER_NOT_INITIALIZED)
    STATUS_STR_PAIR(NV_ENC_ERR_UNSUPPORTED_PARAM)
    STATUS_STR_PAIR(NV_ENC_ERR_LOCK_BUSY)
    STATUS_STR_PAIR(NV_ENC_ERR_NOT_ENOUGH_BUFFER)
    STATUS_STR_PAIR(NV_ENC_ERR_INVALID_VERSION)
    STATUS_STR_PAIR(NV_ENC_ERR_MAP_FAILED)
    STATUS_STR_PAIR(NV_ENC_ERR_NEED_MORE_INPUT)
    STATUS_STR_PAIR(NV_ENC_ERR_ENCODER_BUSY)
    STATUS_STR_PAIR(NV_ENC_ERR_EVENT_NOT_REGISTERD)
    STATUS_STR_PAIR(NV_ENC_ERR_GENERIC)
    STATUS_STR_PAIR(NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY)
    STATUS_STR_PAIR(NV_ENC_ERR_UNIMPLEMENTED)
    STATUS_STR_PAIR(NV_ENC_ERR_RESOURCE_REGISTER_FAILED)
    STATUS_STR_PAIR(NV_ENC_ERR_RESOURCE_NOT_REGISTERED)
    STATUS_STR_PAIR(NV_ENC_ERR_RESOURCE_NOT_MAPPED)
};
#undef STATUS_STR_PAIR


void OutputNvencApiError(const std::string &apiName, NVENCSTATUS status)
{
    const auto &statusStr = g_nvEncStatusErrorNameTable.at(status);
    const auto msg = apiName + " call failed: " + statusStr + "\n";
    ::OutputDebugStringA(msg.c_str());
    throw std::exception(msg.c_str());
}


template <class Api, class ...Args>
void CallNvencApi(const std::string &apiName, const Api &api, const Args &... args)
{
    const auto status = api(args...);
    if (status != NV_ENC_SUCCESS)
    {
        OutputNvencApiError(apiName, status);
    }
}


#define CALL_NVENC_API(Api, ...) CallNvencApi(#Api, Api, __VA_ARGS__)


}


// ---


Nvenc::Nvenc(const NvencDesc& desc)
    : desc_(desc)
{
    const auto status = LoadModule();
    if (status != NV_ENC_SUCCESS)
    {
        OutputNvencApiError("LoadModule", status);
        return;
    }

    OpenEncodeSession();
    InitializeEncoder();
    CreateBitstreamBuffer();
    RegisterResource();
    MapInputResource();
}


Nvenc::~Nvenc()
{
    EndEncode();
    UnmapInputResource();
    UnregisterResource();
    DestroyBitstreamBuffer();
    DestroyEncoder();

    UnloadModule();
}


NVENCSTATUS Nvenc::LoadModule()
{
#if defined(_WIN64)
    module_ = ::LoadLibraryA("nvEncodeAPI64.dll");
#else
    module_ = ::LoadLibraryA("nvEncodeAPI.dll");
#endif
    if (!module_) return NV_ENC_ERR_NO_ENCODE_DEVICE;

#define CALL_NVENC_API_FROM_DLL(API, ...) \
    using API##Func = decltype(API); \
    const auto API##Address = GetProcAddress(module_, #API); \
    if (!API##Address) return _NVENCSTATUS::NV_ENC_ERR_GENERIC; \
    const auto API = reinterpret_cast<API##Func*>(API##Address); \
    const auto API##Result = API(__VA_ARGS__); \
    if (API##Result != NV_ENC_SUCCESS) return API##Result;

    uint32_t version = 0;
    CALL_NVENC_API_FROM_DLL(NvEncodeAPIGetMaxSupportedVersion, &version);
    const uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
    if (currentVersion > version) return NV_ENC_ERR_INVALID_VERSION;

    nvenc_ = { NV_ENCODE_API_FUNCTION_LIST_VER };
    CALL_NVENC_API_FROM_DLL(NvEncodeAPICreateInstance, &nvenc_);

    if (!nvenc_.nvEncOpenEncodeSession)
    {
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    return NV_ENC_SUCCESS;
}


void Nvenc::UnloadModule()
{
    if (!module_) return;

    ::FreeLibrary(module_);
    module_ = nullptr;
}


void Nvenc::OpenEncodeSession()
{
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encSessionParams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
    encSessionParams.device = desc_.d3d11Device;
    encSessionParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    encSessionParams.apiVersion = NVENCAPI_VERSION;
    CALL_NVENC_API(nvenc_.nvEncOpenEncodeSessionEx, &encSessionParams, &encoder_);
}


void Nvenc::InitializeEncoder()
{
    NV_ENC_INITIALIZE_PARAMS initParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    initParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
    initParams.presetGUID = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
    initParams.encodeWidth = desc_.width;
    initParams.encodeHeight = desc_.height;
    initParams.darWidth = desc_.width;
    initParams.darHeight = desc_.height;
    initParams.frameRateNum = desc_.frameRate;
    initParams.frameRateDen = 1;
    initParams.enablePTD = 1;
    initParams.reportSliceOffsets = 0;
    initParams.enableSubFrameWrite = 0;
    initParams.maxEncodeWidth = desc_.width;
    initParams.maxEncodeHeight = desc_.height;
    initParams.enableMEOnlyMode = false;
    initParams.enableOutputInVidmem = false;
    initParams.enableEncodeAsync = false;

    NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
    CALL_NVENC_API(nvenc_.nvEncGetEncodePresetConfig, encoder_, initParams.encodeGUID, initParams.presetGUID, &presetConfig);

    NV_ENC_CONFIG config = { NV_ENC_CONFIG_VER };
    memcpy(&config, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
    config.profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
    config.frameIntervalP = 1;
    config.gopLength = NVENC_INFINITE_GOPLENGTH;
    config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    config.rcParams.constQP = { 28, 31, 25 };
    initParams.encodeConfig = &config;

    config.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
    config.encodeCodecConfig.h264Config.maxNumRefFrames = 0;
    config.encodeCodecConfig.h264Config.idrPeriod = config.gopLength;

    CALL_NVENC_API(nvenc_.nvEncInitializeEncoder, encoder_, &initParams);
}


void Nvenc::CreateBitstreamBuffer()
{
    NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuffer = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
    CALL_NVENC_API(nvenc_.nvEncCreateBitstreamBuffer, encoder_, &createBitstreamBuffer);
    bitstreamBuffer_ = createBitstreamBuffer.bitstreamBuffer;
}


void Nvenc::RegisterResource()
{
    D3D11_TEXTURE2D_DESC tex2dDesc = { 0 };
    tex2dDesc.Width = desc_.width;
    tex2dDesc.Height = desc_.height;
    tex2dDesc.MipLevels = 1;
    tex2dDesc.ArraySize = 1;
    tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex2dDesc.SampleDesc.Count = 1;
    tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
    tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    tex2dDesc.CPUAccessFlags = 0;
    desc_.d3d11Device->CreateTexture2D(&tex2dDesc, NULL, &inputTexture_);

    NV_ENC_REGISTER_RESOURCE registerResource = { NV_ENC_REGISTER_RESOURCE_VER };
    registerResource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
    registerResource.resourceToRegister = inputTexture_.Get();
    registerResource.width = tex2dDesc.Width;
    registerResource.height = tex2dDesc.Height;
    registerResource.pitch = 0;
    registerResource.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
    registerResource.bufferUsage = NV_ENC_INPUT_IMAGE;
    CALL_NVENC_API(nvenc_.nvEncRegisterResource, encoder_, &registerResource);

    registeredResource_ = registerResource.registeredResource;
}


void Nvenc::MapInputResource()
{
    NV_ENC_MAP_INPUT_RESOURCE mapInputResource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
    mapInputResource.registeredResource = registeredResource_;
    CALL_NVENC_API(nvenc_.nvEncMapInputResource, encoder_, &mapInputResource);
    inputResource_ = mapInputResource.mappedResource;
}


void Nvenc::UnmapInputResource()
{
    if (!inputResource_) return;

    CALL_NVENC_API(nvenc_.nvEncUnmapInputResource, encoder_, inputResource_);
}


void Nvenc::UnregisterResource()
{
    if (!registeredResource_) return;

    CALL_NVENC_API(nvenc_.nvEncUnregisterResource, encoder_, registeredResource_);
}


void Nvenc::DestroyBitstreamBuffer()
{
    if (!bitstreamBuffer_) return;

    CALL_NVENC_API(nvenc_.nvEncDestroyBitstreamBuffer, encoder_, bitstreamBuffer_);
}


void Nvenc::DestroyEncoder()
{
    if (!encoder_) return;

    CALL_NVENC_API(nvenc_.nvEncDestroyEncoder, encoder_);
}


bool Nvenc::EncodeFrame(bool forceIdrFrame)
{
    NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
    picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    picParams.inputBuffer = inputResource_;
    picParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
    picParams.inputWidth = desc_.width;
    picParams.inputHeight = desc_.height;
    picParams.outputBitstream = bitstreamBuffer_;
    if (forceIdrFrame)
    {
        picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
    }

    const auto status = nvenc_.nvEncEncodePicture(encoder_, &picParams);
    if (status == NV_ENC_SUCCESS)
    {
        ++frame_;
        GetEncodedPacket();
        return true;
    }
    else if (status == NV_ENC_ERR_NEED_MORE_INPUT)
    {
        // not an error.
        // continue to provide input frames.
        return false;
    }
    else
    {
        OutputNvencApiError("nvenc_.nvEncEncodePicture", status);
        return false;
    }
}


void Nvenc::GetEncodedPacket()
{
    if (frame_ == 0U) return;
    
    NV_ENC_LOCK_BITSTREAM lockBitstream = { NV_ENC_LOCK_BITSTREAM_VER };
    lockBitstream.outputBitstream = bitstreamBuffer_;
    lockBitstream.doNotWait = false;
    CALL_NVENC_API(nvenc_.nvEncLockBitstream, encoder_, &lockBitstream);
  
    auto *data = static_cast<uint8_t*>(lockBitstream.bitstreamBufferPtr);
    encodedData_.resize(lockBitstream.bitstreamSizeInBytes);
    memcpy(&encodedData_[0], data, lockBitstream.bitstreamSizeInBytes);

    CALL_NVENC_API(nvenc_.nvEncUnlockBitstream, encoder_, lockBitstream.outputBitstream);
}


void Nvenc::EndEncode()
{
    if (!encoder_) return;

    SendEOS();
    GetEncodedPacket();
}


void Nvenc::SendEOS()
{
    if (frame_ == 0U) return;

    NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
    picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    CALL_NVENC_API(nvenc_.nvEncEncodePicture, encoder_, &picParams);
}


bool Nvenc::Encode(ID3D11Texture2D *source, bool forceIdrFrame)
{
    if (!encoder_) return false;

    if (isEncoding_) return false;

    {
        ComPtr<ID3D11DeviceContext> context;
        desc_.d3d11Device->GetImmediateContext(&context);
        context->CopyResource(inputTexture_.Get(), source);
    }

    bool result;
    try
    {
        isEncoding_ = true;
        result = EncodeFrame(forceIdrFrame);
    }
    catch (const std::exception &e)
    {
        isEncoding_ = false;
        ::OutputDebugStringA(e.what());
        return false;
    }

    isEncoding_ = false;
    return result;
}


}