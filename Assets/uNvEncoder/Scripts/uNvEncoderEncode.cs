using UnityEngine;
using UnityEngine.Events;
using System.Threading.Tasks;

namespace uNvEncoder
{

public class uNvEncoderEncode : MonoBehaviour
{
    public bool multithreaded = true;
    public int width = 256;
    public int height = 256;
    public int frameRate = 60;

    [System.Serializable]
    public class EncodedCallback : UnityEvent<System.IntPtr, int> {};
    public EncodedCallback onEncoded = new EncodedCallback();

    Task<bool> encodeTask_;

    void OnEnable()
    {
        InitializeEncoder();
    }

    void OnDisable()
    {
        FinalizeEncoder();
    }

    public void InitializeEncoder()
    {
        if (!Lib.Initialize(width, height, frameRate))
        {
            Debug.LogError(Lib.GetLastError());
        }
    }

    public void FinalizeEncoder()
    {
        if (!Lib.Finalize())
        {
            Debug.LogError(Lib.GetLastError());
        }
    }

    public void ReinitializeEncoder()
    {
        FinalizeEncoder();
        InitializeEncoder();
    }

    bool CheckEncoderSetting()
    {
        return
            width == Lib.GetWidth() &&
            height == Lib.GetHeight() &&
            frameRate == Lib.GetFrameRate();
    }

    public void Encode(Texture texture, bool forceIdrFrame)
    {
        if (!texture)
        {
            Debug.LogError("The given texture is invalid.");
            return;
        }

        var ptr = texture.GetNativeTexturePtr();
        Encode(ptr, forceIdrFrame);
    }

    public void Encode(System.IntPtr ptr, bool forceIdrFrame)
    {
        if (ptr == System.IntPtr.Zero)
        {
            Debug.LogError("The given texture pointer is invalid.");
            return;
        }

        if (!Lib.IsInitialized())
        {
            Debug.LogError("uNvEncoder has not been initialized yet.");
            return;
        }

        if (!CheckEncoderSetting())
        {
            Debug.LogError("Encode setting has changed. Please call ReinitializeEncoder().");
            return;
        }

        _Encode(ptr, forceIdrFrame);
    }

    async void _Encode(System.IntPtr ptr, bool forceIdrFrame)
    {
        bool result = false;

        if (encodeTask_ != null && encodeTask_.Status == TaskStatus.Running)
        {
            Debug.LogError("The previous encode has not finished yet.");
            return;
        }

        if (multithreaded)
        {
            encodeTask_ = Task.Run(() => CallEncodeApi(ptr, forceIdrFrame));
            result = await encodeTask_;
        }
        else
        {
            result = CallEncodeApi(ptr, forceIdrFrame);
        }

        if (result) 
        {
            InvokeCallback();
            Debug.Log("Encode() succeeded.");
        }
        else
        {
            Debug.Log("Encode() failed.");
        }
    }

    bool CallEncodeApi(System.IntPtr ptr, bool forceIdrFrame)
    {
        var result = Lib.Encode(ptr, forceIdrFrame);
        if (!result)
        {
            Debug.LogError(Lib.GetLastError());
        }
        return result;
    }

    void InvokeCallback()
    {
        if (onEncoded == null) return;

        var size = Lib.GetEncodedSize();
        var data = Lib.GetEncodedData();
        onEncoded.Invoke(data, size);
    }
}

}
