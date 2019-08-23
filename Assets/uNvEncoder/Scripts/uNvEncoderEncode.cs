using UnityEngine;
using UnityEngine.Events;
using UnityEngine.Assertions;
using System.Threading.Tasks;

namespace uNvEncoder
{

public class uNvEncoderEncode : MonoBehaviour
{
    static uNvEncoderEncode instance;

    [System.Serializable]
    public class EncodedCallback : UnityEvent<System.IntPtr, int> {};
    public EncodedCallback onEncoded = new EncodedCallback();

    Task<bool> encodeTask_;

    public bool isValid
    {
        get { return Lib.IsValid(); }
    }

    public int width
    {
        get { return Lib.GetWidth(); }
    }

    public int height
    {
        get { return Lib.GetHeight(); }
    }

    public int frameRate
    {
        get { return Lib.GetFrameRate(); }
    }

    void OnDisable()
    {
    }

    void Update()
    {
        if (!isValid) return;

        Lib.CopyEncodedData();

        int n = Lib.GetEncodedDataCount();
        for (int i = 0; i < n; ++i)
        {
            var size = Lib.GetEncodedDataSize(i);
            var data = Lib.GetEncodedDataBuffer(i);
            onEncoded.Invoke(data, size);
        }
    }

    public void StartEncode(int width, int height, int frameRate)
    {
        Assert.IsNull(instance, "Multiple uNvEncoderEncode instance not allowed.");
        if (instance) return;

        instance = this;

        Lib.Initialize(width, height, frameRate);

        if (!isValid)
        {
            Debug.LogError(Lib.GetLastError());
        }
    }

    public void StopEncode()
    {
        Lib.Finalize();

        if (instance == this) 
        {
            instance = null;
        }
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

        if (!isValid)
        {
            Debug.LogError("uNvEncoder has not been initialized yet.");
            return false;
        }

        var result = Lib.Encode(ptr, forceIdrFrame);
        if (!result)
        {
            Debug.LogError(Lib.GetLastError());
        }

        return result;
    }
}

}
