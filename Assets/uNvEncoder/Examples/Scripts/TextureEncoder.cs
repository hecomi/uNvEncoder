using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;

namespace uNvEncoder.Examples
{

public class TextureEncoder : MonoBehaviour
{
    public uNvEncoder encoder = null;
    public Texture texture = null;
    public int frameRate = 60;
    public bool forceIdrFrame = true;

    void OnEnable()
    {
        Assert.IsNotNull(encoder);
        Assert.IsNotNull(texture);
        encoder.StartEncode(texture.width, texture.height, frameRate);
        StartCoroutine(EncodeLoop());
    }

    void OnDisable()
    {
        StopAllCoroutines();
        encoder.StopEncode();
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
        if (!encoder || !encoder.isValid) return;

        if (!texture) return;

        if (!encoder.Encode(texture, forceIdrFrame))
        {
            Debug.LogError("Encode() failed.");
        }
    }
}

}
