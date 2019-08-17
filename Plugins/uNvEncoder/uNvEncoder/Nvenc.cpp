#include <string>
#include <map>
#include "Nvenc.h"


extern std::string g_lastError;


namespace
{


void OutputNvencApiError(const std::string &apiName, NVENCSTATUS status)
{
#define STATUS_STR_PAIR(Code) { Code, #Code },
    static const std::map<NVENCSTATUS, std::string> nvEncStatusErrorNameTable = {
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

    const auto it = nvEncStatusErrorNameTable.find(status);
    const auto statusStr = it != nvEncStatusErrorNameTable.end() ? it->second : "Unknown";
    const auto msg = apiName + " call failed: " + statusStr + "\n";
    ::OutputDebugStringA(msg.c_str());
    g_lastError = msg;
}


template <class Api, class ...Args>
bool CallNvencApi(const std::string &apiName, const Api &api, const Args &... args)
{
    const auto status = api(args...);
    if (status != NV_ENC_SUCCESS)
    {
        OutputNvencApiError(apiName, status);
        return false;
    }
    return true;
}


#define CALL_NVENC_API(Api, ...) CallNvencApi(#Api, Api, __VA_ARGS__)


}


namespace uNvEncoder
{


Nvenc::Nvenc(const NvencDesc &desc)
    : desc_(desc)
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!LoadModule())
    {
        return;
    }

    OpenEncodeSession();
    InitializeEncoder();
    CreateCompletionEvent();
    CreateBitstreamBuffer();
    CreateInputTexture();
    RegisterResource();
    MapInputResource();
}


Nvenc::~Nvenc()
{
    UNVENC_FUNC_SCOPED_TIMER

    EndEncode();
    UnmapInputResource();
    UnregisterResource();
    DestroyBitstreamBuffer();
    DestroyCompletionEvent();
    DestroyEncoder();

    UnloadModule();
}


bool Nvenc::LoadModule()
{
    UNVENC_FUNC_SCOPED_TIMER

#if defined(_WIN64)
    module_ = ::LoadLibraryA("nvEncodeAPI64.dll");
#else
    module_ = ::LoadLibraryA("nvEncodeAPI.dll");
#endif
    if (!module_) return false;

#define CALL_NVENC_API_FROM_DLL(API, ...) \
    using API##Func = decltype(API); \
    const auto API##Address = GetProcAddress(module_, #API); \
    if (!API##Address) return false; \
    const auto API = reinterpret_cast<API##Func*>(API##Address); \
    const auto API##Result = API(__VA_ARGS__); \
    if (API##Result != NV_ENC_SUCCESS) return false;

    uint32_t version = 0;
    CALL_NVENC_API_FROM_DLL(NvEncodeAPIGetMaxSupportedVersion, &version);
    const uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
    if (currentVersion > version) return false;

    nvenc_ = { NV_ENCODE_API_FUNCTION_LIST_VER };
    CALL_NVENC_API_FROM_DLL(NvEncodeAPICreateInstance, &nvenc_);

    if (!nvenc_.nvEncOpenEncodeSession)
    {
        return false;
    }

    return true;
}


void Nvenc::UnloadModule()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!module_) return;

    ::FreeLibrary(module_);
    module_ = nullptr;
}


void Nvenc::OpenEncodeSession()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encSessionParams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
    encSessionParams.device = desc_.d3d11Device.Get();
    encSessionParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    encSessionParams.apiVersion = NVENCAPI_VERSION;
    CALL_NVENC_API(nvenc_.nvEncOpenEncodeSessionEx, &encSessionParams, &encoder_);
}


void Nvenc::InitializeEncoder()
{
    UNVENC_FUNC_SCOPED_TIMER

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
    initParams.enableEncodeAsync = true;

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


void Nvenc::CreateCompletionEvent()
{
    UNVENC_FUNC_SCOPED_TIMER

    completionEvent_ = ::CreateEventA(NULL, FALSE, FALSE, NULL);
    NV_ENC_EVENT_PARAMS eventParams = { NV_ENC_EVENT_PARAMS_VER };
    eventParams.completionEvent = completionEvent_;
    CALL_NVENC_API(nvenc_.nvEncRegisterAsyncEvent, encoder_, &eventParams);
}


void Nvenc::DestroyCompletionEvent()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_EVENT_PARAMS eventParams = { NV_ENC_EVENT_PARAMS_VER };
    eventParams.completionEvent = completionEvent_;
    CALL_NVENC_API(nvenc_.nvEncUnregisterAsyncEvent, encoder_, &eventParams);
    ::CloseHandle(completionEvent_);
}


void Nvenc::CreateBitstreamBuffer()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuffer = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
    CALL_NVENC_API(nvenc_.nvEncCreateBitstreamBuffer, encoder_, &createBitstreamBuffer);
    bitstreamBuffer_ = createBitstreamBuffer.bitstreamBuffer;
}


void Nvenc::CreateInputTexture()
{
    UNVENC_FUNC_SCOPED_TIMER

    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = desc_.width;
    desc.Height = desc_.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc_.d3d11Device->CreateTexture2D(&desc, NULL, &inputTexture_);
}


void Nvenc::RegisterResource()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_REGISTER_RESOURCE registerResource = { NV_ENC_REGISTER_RESOURCE_VER };
    registerResource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
    registerResource.resourceToRegister = inputTexture_.Get();
    registerResource.width = desc_.width;
    registerResource.height = desc_.height;
    registerResource.pitch = 0;
    registerResource.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
    registerResource.bufferUsage = NV_ENC_INPUT_IMAGE;
    CALL_NVENC_API(nvenc_.nvEncRegisterResource, encoder_, &registerResource);

    registeredResource_ = registerResource.registeredResource;
}


void Nvenc::MapInputResource()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_MAP_INPUT_RESOURCE mapInputResource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
    mapInputResource.registeredResource = registeredResource_;
    CALL_NVENC_API(nvenc_.nvEncMapInputResource, encoder_, &mapInputResource);
    inputResource_ = mapInputResource.mappedResource;
}


void Nvenc::UnmapInputResource()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!inputResource_) return;

    CALL_NVENC_API(nvenc_.nvEncUnmapInputResource, encoder_, inputResource_);
}


void Nvenc::UnregisterResource()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!registeredResource_) return;

    CALL_NVENC_API(nvenc_.nvEncUnregisterResource, encoder_, registeredResource_);
}


void Nvenc::DestroyBitstreamBuffer()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!bitstreamBuffer_) return;

    CALL_NVENC_API(nvenc_.nvEncDestroyBitstreamBuffer, encoder_, bitstreamBuffer_);
}


void Nvenc::DestroyEncoder()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!IsValid()) return;

    CALL_NVENC_API(nvenc_.nvEncDestroyEncoder, encoder_);
}


bool Nvenc::Encode(const ComPtr<ID3D11Texture2D> &source, bool forceIdrFrame)
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!IsValid() || isEncoding_) return false;
    isEncoding_ = true;

    CopyToInputTexture(source);

    if (!EncodeInputTexture(forceIdrFrame)) 
    {
        isEncoding_ = false;
        return false;
    }

    return true;
}


bool Nvenc::EncodeInputTexture(bool forceIdrFrame)
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
    picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    picParams.inputBuffer = inputResource_;
    picParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
    picParams.inputWidth = desc_.width;
    picParams.inputHeight = desc_.height;
    picParams.outputBitstream = bitstreamBuffer_;
    picParams.completionEvent = completionEvent_;
    if (forceIdrFrame)
    {
        picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
    }

    const auto status = nvenc_.nvEncEncodePicture(encoder_, &picParams);
    if (status != NV_ENC_SUCCESS && status != NV_ENC_ERR_NEED_MORE_INPUT)
    {
        OutputNvencApiError("nvenc_.nvEncEncodePicture", status);
        return false;
    }

    ++frame_;
    return true;
}


void Nvenc::CopyToInputTexture(const ComPtr<ID3D11Texture2D> &texture)
{
    UNVENC_FUNC_SCOPED_TIMER

    ComPtr<ID3D11DeviceContext> context;
    desc_.d3d11Device->GetImmediateContext(&context);
    context->CopyResource(inputTexture_.Get(), texture.Get());
}


bool Nvenc::WaitForCompletion(DWORD duration)
{
    UNVENC_FUNC_SCOPED_TIMER

    if (::WaitForSingleObject(completionEvent_, duration) == WAIT_FAILED)
    {
        g_lastError = "Failed to wait for encode completion.";
        return false;
    }

    return true;
}


bool Nvenc::GetEncodedData(NvencEncodedData &data)
{
    UNVENC_FUNC_SCOPED_TIMER

    if (frame_ == 0U || !isEncoding_) return false;

    constexpr DWORD duration = 1000;
    if (!WaitForCompletion(duration)) return false;
    
    NV_ENC_LOCK_BITSTREAM lockBitstream = { NV_ENC_LOCK_BITSTREAM_VER };
    lockBitstream.outputBitstream = bitstreamBuffer_;
    lockBitstream.doNotWait = true;
    CALL_NVENC_API(nvenc_.nvEncLockBitstream, encoder_, &lockBitstream);
  
    data.size = lockBitstream.bitstreamSizeInBytes;
    if (data.size > 0)
    {
        data.buffer = std::make_unique<uint8_t[]>(data.size);
        const auto *ptr = lockBitstream.bitstreamBufferPtr;
        memcpy(data.buffer.get(), ptr, data.size);
    }

    CALL_NVENC_API(nvenc_.nvEncUnlockBitstream, encoder_, bitstreamBuffer_);

    isEncoding_ = false;

    return data.size > 0;
}


void Nvenc::EndEncode()
{
    UNVENC_FUNC_SCOPED_TIMER

    if (!IsValid() || frame_ == 0U) return;

    SendEOS();
    NvencEncodedData data;
    GetEncodedData(data);
}


void Nvenc::SendEOS()
{
    UNVENC_FUNC_SCOPED_TIMER

    NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
    picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    picParams.completionEvent = completionEvent_;
    CALL_NVENC_API(nvenc_.nvEncEncodePicture, encoder_, &picParams);
}


}