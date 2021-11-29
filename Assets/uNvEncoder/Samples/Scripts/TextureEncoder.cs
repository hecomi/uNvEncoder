using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;

namespace uNvEncoder.Examples
{

public class TextureEncoder : MonoBehaviour
{
    public Encoder encoder = new Encoder();
    public Texture texture = null;
    public EncoderDesc setting = new EncoderDesc
    {
        width = 1024,
        height = 1024,
        frameRate = 60,
        format = uNvEncoder.Format.R8G8B8A8_UNORM,
        bitRate = 2000000,
        maxFrameSize = 2000000/60,
    };
    public bool forceIdrFrame = true;

    void OnEnable()
    {
        Assert.IsNotNull(texture);
        setting.width = texture.width;
        setting.height = texture.height;
        encoder.Create(setting);
        StartCoroutine(EncodeLoop());
    }

    void OnDisable()
    {
        StopAllCoroutines();
        encoder.Destroy();
    }

    IEnumerator EncodeLoop()
    {
        for (;;)
        {
            yield return new WaitForEndOfFrame();
            Encode();
        }
    }

    void Encode()
    {
        if (!texture) return;

        encoder.Update();
        encoder.Encode(texture, forceIdrFrame);
    }
}

}
