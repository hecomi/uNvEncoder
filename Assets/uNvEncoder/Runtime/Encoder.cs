using UnityEngine;
using UnityEngine.Events;

namespace uNvEncoder
{

[System.Serializable]
public class Encoder
{
    public DXGI_FORMAT format = DXGI_FORMAT.DXGI_FORMAT_R8G8B8A8_UNORM;

    [System.Serializable]
    public class EncodedCallback : UnityEvent<System.IntPtr, int> {};
    public EncodedCallback onEncoded = new EncodedCallback();

    public int id { get; private set; } = -1;

    public bool isValid
    {
        get { return Lib.IsValid(id); }
    }

    public int width
    {
        get { return Lib.GetWidth(id); }
    }

    public int height
    {
        get { return Lib.GetHeight(id); }
    }

    public int frameRate
    {
        get { return Lib.GetFrameRate(id); }
    }

    public string error
    {
        get 
        { 
            if (!Lib.HasError(id)) return "";

            var str = Lib.GetError(id); 
            Lib.ClearError(id);
            return str;
        }
    }

    public void Create(int width, int height, int frameRate)
    {
        id = Lib.CreateEncoder(width, height, format, frameRate);

        if (!isValid)
        {
            Debug.LogError(error);
        }
    }

    public void Destroy()
    {
        Lib.DestroyEncoder(id);
    }

    public void Update()
    {
        if (!isValid) return;

        Lib.CopyEncodedData(id);

        int n = Lib.GetEncodedDataCount(id);
        for (int i = 0; i < n; ++i)
        {
            var size = Lib.GetEncodedDataSize(id, i);
            var data = Lib.GetEncodedDataBuffer(id, i);
            onEncoded.Invoke(data, size);
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
        if (!Encode(ptr, forceIdrFrame))
        {
            Debug.LogError(error);
            return false;
        }

        return true;
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

        var result = Lib.Encode(id, ptr, forceIdrFrame);
        if (!result)
        {
            Debug.LogError(error);
        }

        return result;
    }
}

}
