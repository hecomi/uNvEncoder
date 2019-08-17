using UnityEngine;
using UnityEngine.Events;
using UnityEngine.Assertions;
using System.Threading.Tasks;

namespace uNvEncoder
{

public class uNvEncoderEncode : MonoBehaviour
{
    static uNvEncoderEncode instance;

    public int width = 256;
    public int height = 256;
    public int frameRate = 60;

    [System.Serializable]
    public class EncodedCallback : UnityEvent<System.IntPtr, int> {};
    public EncodedCallback onEncoded = new EncodedCallback();

    Task<bool> encodeTask_;

    public bool isEncoding
    {
        get { return Lib.IsEncoding(); }
    }

    void OnEnable()
    {
        Assert.IsNull(instance, "Multiple uNvEncoderEncode instance not allowed.");
        instance = this;
        InitializeEncoder();
    }

    void OnDisable()
    {
        FinalizeEncoder();
        instance = null;
    }

    void Update()
    {
        Lib.CopyEncodedData();

        int n = Lib.GetEncodedDataCount();
        for (int i = 0; i < n; ++i)
        {
            var size = Lib.GetEncodedDataSize(i);
            var data = Lib.GetEncodedDataBuffer(i);
            onEncoded.Invoke(data, size);
        }
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

    public bool Encode(Texture texture, bool forceIdrFrame)
    {
        if (!texture)
        {
            Debug.LogError("The given texture is invalid.");
            return false;
        }

        var ptr = texture.GetNativeTexturePtr();
        return Encode(ptr, forceIdrFrame);
    }

    public bool Encode(System.IntPtr ptr, bool forceIdrFrame)
    {
        if (ptr == System.IntPtr.Zero)
        {
            Debug.LogError("The given texture pointer is invalid.");
            return false;
        }

        if (!Lib.IsInitialized())
        {
            Debug.LogError("uNvEncoder has not been initialized yet.");
            return false;
        }

        if (!CheckEncoderSetting())
        {
            Debug.LogError("Encode setting has changed. Please call ReinitializeEncoder().");
            return false;
        }

        return Lib.Encode(ptr, forceIdrFrame);
    }
}

}
