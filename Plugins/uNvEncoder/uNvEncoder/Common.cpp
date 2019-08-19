#include <d3d11.h>
#include <IUnityInterface.h>
#include <IUnityGraphicsD3D11.h>
#include "Common.h"


namespace uNvEncoder
{


extern IUnityInterfaces *g_unity;
extern std::string g_error;


IUnityInterfaces * GetUnity()
{
    return g_unity;
}


ID3D11Device * GetUnityDevice()
{
    return GetUnity()->Get<IUnityGraphicsD3D11>()->GetDevice();
}


void DebugError(const std::string &error)
{
    ::OutputDebugStringA((error + "\n").c_str());
    g_error = error;
}


ScopedTimer::ScopedTimer(const StartFunc &startFunc, const EndFunc &endFunc)
    : func_(endFunc)
    , start_(std::chrono::high_resolution_clock::now())
{
    startFunc();
}


ScopedTimer::~ScopedTimer()
{
    using namespace std::chrono;
    const auto end = high_resolution_clock::now();
    const auto time = duration_cast<microseconds>(end - start_);
    func_(time);
}


}
