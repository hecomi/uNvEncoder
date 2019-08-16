#pragma once

#include <wrl/client.h>


namespace uNvEncoder
{

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct IUnityInterfaces * GetUnity();
struct ID3D11Device * GetUnityDevice();

}
