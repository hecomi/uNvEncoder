using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;

namespace uNvEncoder.Examples
{

public class Encoder : MonoBehaviour
{
    [SerializeField]
    uNvEncoderEncode encoder = null;

    [SerializeField]
    RenderTexture texture = null;

    [SerializeField]
    int frameRate = 60;

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

            if (!encoder || !encoder.isValid) continue;

            if (!texture) continue;

            if (!encoder.Encode(texture, forceIdrFrame))
            {
                Debug.LogError("Encode() failed.");
            }
        }
    }
}

}
